# AYAstorm 全体処理回復計画(設計書 + 工程表)

- 状態: **AYA approve 済(2026-07-17)・大順序確定 = T 系 → E 系(やり切る)→ Phase 2(並列処理)再開**
- 起草: 2026-07-17 設計実装者(実測データ同日採取)
- 位置づけ: `docs/vknative_architecture.md`(以下「基本設計」)の **§5 移行表の後半(M5b/M5c/M6)を実測に基づき再編成**する上位工程書。基本設計の §1〜§4(資源モデル: bindless / mega-buffer / DrawData / bucket)と doctrine は**有効のまま**。本書 approve 時に基本設計 §5 へ相互参照を追記する。
- **本書の各段は独立セッションへの handoff を前提に切ってある**(§4 分担と依存)。

---

## 0. 診断(実測 2026-07-17・crowd + 鏡・RTX 5090)

### 0.1 ガン = 単一 main thread 直列パイプライン + per-frame 時間予算の配給制

実測: `cpu main=95%・他 19 コアは一桁%` = 20 コア機が実質 1 コア動作。main に直列で刺さっているもの: emission ×4 pass(camera/shadow/probe/鏡)・texture 生成/upload(予算 2〜5ms/frame = llviewerdisplay.cpp:934)・geometry rebuild(50ms/秒予算は createObjects のみ = pipeline.cpp:2494。updateGeom の mBuildQ1 と postSort の可視 dirty group rebuild は無予算で frame 内直列全量実行 = 嵐がそのまま frame time になる)・avatar bake(LLTexLayer)・updateImages・object 更新(idle)。

**毒の本体 = 全 throughput が fps に結合**していること。予算が per-frame なので、fps 低下 → texture/rebuild の毎秒処理量低下 → 重い状態が長引く → fps 低いまま(自己絞殺ループ)。「HUD/アバター完全表示 60 秒(キャッシュ有)」と「crowd 7〜13fps」は同一病理の 2 症状。

**GL 6 倍(同一品質・AYA 実測)の構造的説明**: GL viewer = main + ドライバのワーカー軍団 + LLImageGLThread の並列トポロジー。VK 移植はドライバ軍団を捨て、texture thread も無効化し(llimagegl.cpp:361-365)、全てを main に集約した上で単価を memo で磨いた。管が 1 本のままでは単価をいくら磨いても 1 コア分しか出ない。

### 0.2 frame 全量会計(settled・48.8ms/frame・fps 20.5・会計残差 1.6ms)

VkPerf `ph`/`fam` 欄(2026-07-17 実装)による。

| ブロック | ms/frame | 内訳・根拠 |
|---|---|---|
| emission 4 重発行 | 24.4 | shadow 6.9 / probe 5.9 / 鏡 4.7 / camera 不透明 4.1 / alpha 2.8。全 pass 合計 ~31-57k draws/frame × **実測単価 ~0.8-0.9µs/draw** |
| idle(render 外 tick) | 9.5 | 分解未了(S6) |
| lighting CPU | 4.5 | fullscreen/local light 発行(トレース未了 = E4) |
| img(texture 更新 main 分) | 4.0 | S1 で ≈0 化 |
| ui | 2.3 | |
| cull + sort + gupd | 2.1 | **octree walk は既に小さい = GPU frustum cull(旧 M5b)は主敵でない** |
| occlusion query | (emission 内に分散) | occl pass ~4.1k draws/frame = 退役価値あり(旧 M5c 維持) |

per-draw 単価 ~0.9µs の中身(perf 実測・main thread): ScenePerDrawCache hash 照合 ~7% + acquireDeferredUtilOverrideSlot 2.9% + buildAndOverride/ensure/populate ~4% + pipeline/bind/setBuffer memo ~4% + gGL エミュ層。**GL 期に存在しなかった自前簿記**。

### 0.3 texture 60 秒の連鎖(トレース済)

①キャッシュは圧縮 j2c 保存 = ヒットでも全 decode(opj = プロセス CPU ~12%)②生成/upload は main 専属(LLImageGLThread 無効 = llimagegl.cpp:361-365)③予算 2〜5ms/frame(llviewertexturelist.cpp:1126-1158 の break)= 10fps で毎秒 50ms ぶんしか処理できない ④1 枚毎に staging 生成 + one-shot vkQueueSubmit(submitOneShotVk 呼出 8 箇所)+ 256 超で main が fence 待ちブロック(llvkloader.cpp:6241-6246)。→ 毎秒 20〜50 枚 × 数百枚 = 60 秒。

### 0.4 確定済みの規律(全段共通)

- **品質を下げて速くするのは最適化ではない**(AYA 2026-07-17 厳命)。全段の gate は「視覚同一 + validation 0」を含む。品質トレードが不可避なら性能案の顔をさせず product 決裁として明示する。
- 施策評価の第一問は「main から仕事を引き剥がすか / fps との結合を切るか」。frame ms はその次。
- kill switch = 工事足場・gate PASS 後即削除(基本設計 §5 の運用)。MT 系の退化は既存 `AYASTORM_MT_THREADS=1` の意味論に従う。

---

## 1. 治療原理

1. **T(トポロジー)系が先、E(発行)系が後**: worker 化で throughput と fps を切り離してから、main に残る描画本体の per-draw 簿記を構造削除する。E 系の配当は 4 重発行(camera/shadow/probe/鏡)全部に掛かる。
2. **queue は PE thread 単独所有のまま**(MT-1 資産)。worker は submit しない — one-shot job を peEnqueue するだけ(llvkloader.cpp:6279 → :616)。**新しい queue 同期は発明しない**。
3. **「prepare on worker / publish on main」の 2 相パターン**を T 系の共通形にする: worker は「まだ誰にも見えていない新規リソース」だけを扱い(共有状態ゼロ)、main が frame 頭に publish queue を drain して field 差替 + heap slot 更新(bindless heap・購読簿は main 専有のまま)。寿命は既存 fence 遅延破棄の規律。
4. E 系の到達形は基本設計 §2 のとおり(永続 bucket + indirect)。M5a で実証済の template/ring/購読 patch 機構を全族へ拡張する。

---

## 2. 工程表

### T 系(トポロジー手術)— ✅ 全段完遂(2026-07-17・T1 `ff2d31472c` / T2 `7adc5f56e1`+`73d465b43d` / T3 `9a7e5e8924` / T5 計測 `0400f9f656` / T4 `bc4bf32d19` / T5 処置 = pri skip `8861538295` + cleanDeadObjects `1433566671`。分解の残 = aChar 2.05ms/f と net = Phase 2 送り・台帳参照)

| 段 | 内容 | 消える病理 | gate | 依存 |
|---|---|---|---|---|
| **T1** | **texture 生成/upload の worker 常駐化**: mCreateTextureList を予算なしで worker queue へ。worker = vmaCreateImage → staging 書込 → copy cmd(worker 専用 CommandPool)→ peEnqueue。mip は 1 texture 1 cmd に集約。publish は main が frame 頭で drain(field 差替 + updateVkHeapSlot + postCreateTexture)。one-shot の fence pool/free queue に mutex。backpressure は worker 側へ | 予算配給制・main の img 4ms・upload 単価(mip 毎 submit) | **HUD/アバター完全表示 60 秒 → 1 桁秒(AYA 体感計測)**+ ph 表 img≈0 + 視覚同一 + validation 0 | なし(即着手可) |
| **T2** | **geometry rebuild の worker 化**: `LLVolumeGeometryManager::rebuildGeom`/`genDrawInfo` の頂点 fill(`LLFace::getGeometryVolume` の loop 群)を worker job 化(基本設計 §3 の worker 仕事①。genVolumeGeometry という関数は HEAD に不存在 = 実体はこの 3 者)。mega-buffer 書込の同期(pool は main 前提 = §1.2)をこの段で設計 | rebuild 嵐の frame 直列刺さり(postSort/updateGeom)・TP/ロード時の gupd/sort 嵐 | ロード中の fps 崩れ幅縮小 + 視覚同一 + validation 0。**gate 解釈**: T2 後の gupd 残余には geometry publish 簿記(VkPerf geo 欄 pub_ms)を含む = T1 の img 残余と同型。gupd≈0 判定は `gupd − geo pub_ms` で行う | T1(one-shot mutex・publish 様式を共有) |
| **T3** | **avatar bake(LLTexLayer 合成)の main 離脱** | idle/img 内の bake 時間・アバター表示遅延の残り | アバター表示時間 + 視覚同一 | T1 |
| **T4** | **decode 供給の増強 + 優先度是正**(opj 並列度・self-avatar/HUD の優先) | decode 律速・優先度飢餓 | キャッシュ消去後の表示時間 | T1 と独立(readonly 調査は並列可) |
| **T5** | **idle 9.5ms の分解と処置**(object 更新/interp/network の実測分解 → 処置は分解結果で起案) | 未帰属の render 外 main 時間 | 分解表の提出(処置は別決裁) | なし(計測のみ・並列可) |

**T1 の既知の罠(JIT 設計時に対策を明記すること)**: ①LLImageRaw の寿命 = worker job が LLPointer 保持(worker が読む間に main が捨てない)②LLViewerFetchedTexture 死と in-flight job の競合 = job が texture を LLPointer 保持 ③discard 変更(redundant_load/scaleDown 判定 = llviewertexturelist.cpp:1134-1152)は publish 時に main で再評価 ④vma allocator の thread 安全 flag 実読(llvkloader.cpp:2328- の VmaAllocatorCreateInfo に EXTERNALLY_SYNCHRONIZED が無いこと)⑤休眠 lane 機構(MT-2b 残置)の転用可否は実装時トレースで決定 ⑥`AYASTORM_MT_THREADS=1` で inline 退化(既存 switch 意味論)。

### E 系(発行構造 = v2。M5a の bucket/template/ring/購読 patch を全族へ)

> **現在地(2026-07-21・HEAD `3ac6b1d5df` を file:line で実トレース検証)**: E1 ✅ / E2 materials bindless ✅(`7374ffd992`)/ **E2 残 = 13-pass MDI 合流 = scope-down(実装せず・下記裁定)** / E3-0 計器 ✅(`b98d4a63f6`)/ E3-1 palette dedup ✅(`f802ba487f`)/ **E3-2b shadow rigged collapse = scope-down(実装せず・下記裁定)** / E4 降格(計器のみ)。E3-2a(palette UBO→SSBO)= CLAIM 認定・全面 revert 済(HEAD palette は UBO 継続)。表外の機会的完了 = terrain MDI collapse(`bb710e5e4f`)/ emi 計器+emissive 一括(`6264c10f7f`)/ emissive collapse(`d8a0bec7f9`)。**🔴 戦略的結論(2026-07-21): E 系「per-draw issue collapse」レバーは全滅確定 = E1(alpha)/E2(mat)/E3-2b(rig shadow)全てゼロ。共通根 = per-draw issue は ~10% で、残り(VB/index bind・modelview・palette)は全て既に dedup 済**。次の本物のレバーは issue collapse ではなく draw COUNT 削減 / dedup 後も残る per-draw の構造薄化 / 別費目(aChar 等)= 次ラウンドは標的 hot 自己実測から。
>
> **E2 残 scope-down 裁定(2026-07-21・settled 計測 3 窓・NOT crowd・build 密地)**: mat pool(POOL_MATERIALS = 未 collapse の 11 pass の主住処)= **0.46-0.48ms/f・713 draws/f・0.66µs/draw**。bump pool = 0 draws(該当幾何なし)。camera MDI run 長(既 collapse 済 simple/fullbright の rec/call)= **3.85 = 短い**。判定 = **E1 と同じ配当ゼロ級・実装非推奨**。理由 =(1)E2b bindless が既に本丸(set 費目)を除去済で mat は 0.66µs/draw(E1 の 1.7-2.1 から大幅減)=MDI が削るのは残る発行系のみ(2)E1 が同一機構(bindless simple/fullbright collapse)を ON/OFF 実測 → 発行系は per-draw の 7-10%・配当ゼロと確定済(3)「E2 後に run が伸びる器」の期待は不発(bindless 化後も run=3.85 と短い・spatial group/VB/region 断片で切れる)(4)削り代 ≈ 0.66µs×10%×713 ≈ 0.03-0.07ms/f = 16.7ms frame の 0.4% 以下。mat は static 幾何ゆえ crowd で増えない(crowd 負荷は aChar/alpha/rig shadow = E3 側)。**申告: ON/OFF 実装での決定的 close は未実施 = E1 同一機構 prior + settled 計測からの裁定**。詳細 = memory `handoff_e2_residual_mdi_scope_down`。

| 段 | 内容 | 実測根拠 | gate | 依存 |
|---|---|---|---|---|
| **E1** | ✅ **完(2026-07-18・commit `8ad6534c8b`)・判定 = 発行 collapse は配当ゼロ**: 機構(ソート順 ring append + run 境界 MDI)は実装・視覚同一・crash 0 だが、同一 2 地点 ON/OFF A/B で per-draw 単価不変(1.7-2.1µs)。alp 計器 12 費目分解の答え = **alpha CPU の本丸 = set(per-draw universal set 簿記 = material/pbr/rigged 系の normal/spec per-draw 差し替え)39%/28-35% + emi(emissive 節)19%/26-28%・発行系は 7-10% = 元から安い**。collapse 機構は E2 後に run が伸びる器として温存(switch 配下)。副産物 = bindless への binding 100-103 不正 write の実バグ fix(VVL crash 根絶)+ alp 計器恒久化。emissive 節は台帳起票 | alpha = camera pool 最大(実測 crowd 3.3-6.0ms/f・シーン変動大) | (完)視覚同一・crash 0・validation baseline 化。**配当は E2 送り** | M5a 資産(済) |
| **E2** | **✅ materials bindless = 完(2026-07-19・`7374ffd992`)/ 🔵 残 = 13-pass MDI 合流(未着手)**。**完了分(E2a/b/c)**: map 持ち material 変種(24/32 program)の diffuse/bump/spec を heap(set2+DrawData .x/.y/.z)経由化・bindless 判定を SPIR-V 真実源(set=2 reflection)へ。実測 = fam mat 1.73→1.03µs/draw(41% 減)・material pool CPU 43% 減・alp set/draw 31% 減。switch `AYASTORM_E2_MAT` は gate 後撤去 = 既定 ON(bindless 非活性 GPU は非 heap)。**残(13-pass MDI 合流)= scope-down(2026-07-21・実装せず)**: camera MDI collapse は現状 **2/13 pass のみ**(`isCameraMdiPass`@llvkbucket.cpp:157 = PASS_SIMPLE/FULLBRIGHT)だが、settled 計測で残 pass(mat pool)= 0.66µs/draw(E2b bindless で set 費目除去済)・run 3.85 と短く、MDI collapse の削り代 ≈ 0.03-0.07ms/f = E1 と同じ配当ゼロ級と裁定 → 実装せず E3-2b へ pivot(上記バナー裁定) | mat 実測 0.46-0.48ms/f・713 draws/f・0.66µs/draw(2026-07-21 settled) | (scope-down)実装せず | — |
| **E3** | **✅ E3-0 計器(`b98d4a63f6`)+ E3-1 palette dedup(`f802ba487f`)= 完 / 🔴 E3-2a = CLAIM・全面 revert / 🚫 E3-2b = scope-down(実装せず・2026-07-21)**。**E3-1**: palette upload を (avatar,skinHash)→slice cache で frame 内 dedup(uploadMatrixPalette allow_dedup@lldrawpool.cpp:1685,1712)= upload −51%。**E3-2a**(palette UBO→SSBO 経路移行)= オラクル無しで CLAIM → 全面 revert・設計者解雇(HEAD palette は UBO 継続)。**E3-2b(shadow rigged collapse)= scope-down**: 一時 srig 計器(読み取り専用・裁定後 revert 済)で crowd settled 実測 = same-palette run avg 4.22(母集団は実在・−76% draw count 可)だが、**コードトレースで collapse が time を削れないと決定論的に証明** = run 内の VB/index bind(bindVertexBufferVk vb_skip @llvkloader.cpp:8181)・modelview(applyModelMatrix gGLLastMatrix @lldrawpool.cpp:1559)・palette(uploadMatrixPalette same-key early-return @:1749)は**全て既に dedup 済**で、collapse が消すのは per-draw issue(setBuffer ループ+vkCmdDrawIndexed)= E1/E2 が ~10%・配当ゼロと実測済のカテゴリ ≈ 0.04ms/f。camera rigged 0.03ms/f = 非標的 | 実測(crowd settled): rig shadow 0.53ms/f・658 draws/f・same-palette run avg 4.22・max 60-114 | (scope-down)実装せず | — |
| **E4** | **降格・保留(2026-07-19 実測裁定・lgt 計器 `6218ed3503`)**: 旧「lighting 4.5-9ms」は phase 境界の誤読 = ph light の 44-86% は renderGeomPostDeferred(forward 本体・alp/fam 領域)。純 lighting は 1.6-3.3ms/f・fullscreen/bind は瑣末。残標的 = loc/spot per-light cube(~3.2ms/f @光源密・spot 13.8µs/draw)は台帳へ(indirect 全族展開時に同族回収) | 純 lighting 1.6-3.3ms/f | (降格) | — |
| **E5** | (自動配当)probe/鏡/shadow は E1〜E4 の係数 — 専用段なし | 4 重発行 ×24.4ms | — | — |

### 旧 M 表の再配置(基本設計 §5 への差分)

- **M5b(GPU frustum cull)= 降格・無期限保留**: cull+sort 実測 2.1ms = 主敵でない。E 系完了後に残余があれば再起案。
- **M5c の occlusion query 機構退役 = E 系の後に維持**(occl ~4.1k draws/frame の発行 + 状態機械が実在)。HiZ 化はその時に JIT 設計。
- **M6(frame graph・旧経路削除・cvar 掃除)= 🔴 本命が BLOCKED(2026-07-21 trace)**。M6 の「旧経路」= per-draw descriptor 経路(`buildAndOverrideScenePerDrawSet`・GL ではない)だが、HEAD 実トレースで **load-bearing**(bucket MDI は 2/13 pass のみ・残 11 pass + rigged/alpha/materials/GLTF は per-draw 現役)。**M6 前提「bucket が全 pass 覆えば per-draw 削除可」は E 系裁定「13-pass MDI = 配当ゼロで作らない」と正面衝突** = per-draw は恒久共存物・旧経路削除は現戦略下で実行不能。**AYA 決裁要**: (a) per-draw 恒久受容 + M6 旧経路削除撤回 / (b) 非 bindless GPU 切り捨て(VK1.2 最低要件)で非 bindless 分岐のみ削除。cvar 掃除の内訳・状態は Task #1-10 参照(dead switch 4 本は実装→revert・reflog dd1d357774)。詳細 = memory `handoff_designer_dismissed_eseries_close_m6_blocked`。
- ✅ `AYASTORM_INDIRECT` switch = E 系 gate 後 撤去済(`1c4f87ccbb`・collapse 恒久 ON)。

---

## 3. 全体検収(このプランの成功条件)

1. crowd settled で **GL 同品質 6 倍差の解消曲線**を段毎に記録(ph/fam 表 + AYA 体感)。
2. **HUD/アバター完全表示: 60 秒 → 1 桁秒**(キャッシュ有)。キャッシュ無は T4 後に別途。
3. fps と load の結合切断の実証: ロード中でも settled でも ph 表の img/gupd ≈ 0。
4. 視覚同一 + validation 0 は全段の前提(品質トレード禁止)。

## 4. 分担と依存(複数セッション運用)

- **大順序は AYA 決定(2026-07-17)= T 系完遂 → E 系完遂 → Phase 2(並列処理)再開**。E 系の実装は T 系 gate 後に開始(E4/T5 の readonly トレースのみ先行並列可)。
- T 系内の並列: T4/T5 は T1 と独立(AYA が担当セッションを分ける場合のみ実装並列・1 セッション内は直列の既存規律)。
- **handoff 様式(全段共通)**: 着手時 readiness 宣言(本書 + 基本設計該当節 + 対象ソース file:line 帯の実読)→ JIT 詳細設計(触る file・データ構造・罠・gate・**縮小・省略・解釈申告欄**)→ AYA 承認 → 実装 → gate。
- 各段の前提トレース済み事実は本書 §0 に集約してある(handoff 斜め読み禁止・引用 file:line は着手時に HEAD で再確認)。

## 5. 縮小・省略・解釈申告(本書自体の)

1. T 系の「予算配給制の廃止」は publish(main 側 drain)には適用しない — publish は field swap + 16B 書込で軽量のため。実測で重ければ上限を付ける(その時は申告)。
2. idle 9.5ms(T5)と lighting(E4)は**分解が先・処置は後決裁** — 中身を知らずに処置を書かない。
3. 基本設計 §5 の旧 M5b/M5c 行の書き換えは本書 approve 後に実施(stale 化防止)。
4. **品質トレード台帳(2026-07-21・締めで棚卸し)= 全景は memory `project_quality_tradeoff_ledger`**。既定 ON で品質を落としている 4 機構(鏡 hero probe 6x / 動的 probe `AYASTORM_PROBE_RT_FACES` 3x / 遠 shadow `AYASTORM_SHADOW_RR` / shadow sub-texel cull)。**処分 = PENDING(AYA 受容寄りだが確定保留)**: 低 FPS 機ほど frame skip の視覚被害が大きい(10FPS=1f 100ms → 鏡 600ms 固着)ため、**低スペック実機の見た目評価まで受容可否を確定できない**。旧「鏡 rate 計上済」は #1 のみで #2-4 は未計上だった(訂正)。productization ギャップ = #2-4 は env のみ(GUI 非到達)= 恒久化するなら設定昇格が正攻法。
5. 未 commit の作業(計測基盤 fam/ph・rigged カウンタ・鏡 rate)の commit タイミングは AYA 指示待ち。
