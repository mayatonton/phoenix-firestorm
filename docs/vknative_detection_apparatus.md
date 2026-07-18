# 検知装置アーキテクチャ(描画不体裁の機械検知体制)— 装置設計 v1

## ⚡ 担当者向けクイックリファレンス(まずここだけ読めば使える)

**鉄則: 描画がおかしい時、コードを推理する前に装置に聞く。装置が犯人を名指しするまで fix を書かない。**

1. **診断起動**: `AYASTORM_VKC=1 AYASTORM_PERF_LOG=5 ~/ayastorm/ayastorm`(通常起動では装置はほぼ無音・コストゼロ。`AYASTORM_VKC=1` で全装置 ON)
2. **症状を再現**(移動なら移動・外見編集なら外見編集)→ 終了 → `~/.ayastorm_x64/logs/AYAstorm.log` の `VKC` 行を読む。
3. **行の読み方**:
   - `VKC-SUM 10s skips=… cause{…}` — 10 秒毎の失敗全集計。**ここに出ない失敗種は存在しない**(出ないのに症状がある = 装置のギャップ → 装置を伸ばす。fix 仮説に行かない)。
   - `VKC skip cause=… obj=… tex=… av=…` — draw 不発火とその原因・対象(L1)。
   - `VKC map_evict_unpaired site=… obj=… recs=…` — 生存オブジェクトの draw record が剥奪されたまま再登録されていない = 「消えた」族(L2)。site が剥奪経路を名指し。
   - `VKC geoab_* attr=… maxdiff=… verdict=… obj=…` — worker 幾何出力と旧経路の byte 照合(L3)。**verdict=src** = カーネル無罪・入力データが worker 読取り後に変わった / **verdict=kernel** = カーネルの数式バグ / `geoab_input_drift fields=0x…` = stage↔apply 間で入力パラメータが変化。
4. **fix を書いたら**: 同じ診断起動で該当 cause が 0 になること + 全層沈黙が機械検収。視覚確認はその後の最終 gate。

実例(2026-07-18): 「カメラ移動で透明の穴」— 前任は視覚と推理で 6 連打・全外し。装置導入後、起動 2 回で `geoab_source_drift attr=pos maxdiff=4.8`(= worker が変異中の volume を読む)まで機械確定した。

---

- 発注: AYA(2026-07-18)「あまりにも簡単に描画不体裁が起きる。ソースは正しいはず→おかしい→写真見せろ→これでどう? のあてずっぽうループを何とかしないとプロジェクトは前に行かない。このためなら 1 週間払う価値がある」
- 目的: **描画不体裁を AYA の目でなく装置が identity 付きで名指しする体制**。全バグ検知ではない。「簡単に起きる族」に恒久の網を張り、あてずっぽうループを構造的に不可能にする。
- 先行実証: LLVKContract v1.2(`indra/llrender/llvkcontract.{h,cpp}`)は「描かない」族(髪消失・ちらつき)を起動 1 回で名指し・起動 1 回で検収した。本設計はその守備範囲を「消えた」「間違って描いた」へ拡張する。

## 0. 統治規律(装置と同格の成果物)

1. **装置が犯人を名指しするまで fix を打たない。** AYA からの不体裁報告は「fix せよ」の入力ではなく「装置に穴がある」の報告。log が沈黙していたら、先に伸ばすのは装置であって fix 仮説ではない。
2. **AYA 起動は 2 種のみ**: ①装置が名指しした後の fix 検収 ②装置拡張の稼働確認。「これでどう?」起動は禁止。
3. **成功基準**: AYA が異常を 1 回報告 → 次の起動 1 回で log が対象(obj/face/frame/原因段)を名指しする。できなければ装置側の欠陥として扱う。
4. **gate 基準の拡張**: 従来「視覚同一 + validation 0」に「**標準ストレス走行(カメラ移動・TP・LOD 嵐)下で全層オラクル沈黙**」を追加。オラクルなき経路移行は受け入れない(移行 = L3 の A/B 参照実装を伴うこと)。

## 1. 装置の階層

| 層 | 名 | 守備範囲(症状族) | 状態 |
|---|---|---|---|
| L1 | 発火契約(LLVKContract v1.2) | draw が発火しない / fallback で描いた / fire-skip 振動 | **既設・実証済** |
| L2 | draw-map 連続性オラクル(sentinel) | **生存物の draw record が剥奪されたまま再登録されない = 消える・穴** | 本設計・**恒久** |
| L3 | 内容オラクル(移行 A/B 照合) | **経路移行後のカーネルが同一入力で違う内容を書く / 入力が非同期窓で drift** | 本設計・移行ごと(第 1 号 = T2 fill カーネル) |
| L4 | 束縛 checksum(誤 texture/UBO 内容) | 誤 lighting・誤色(描かれるが状態が違う) | 未設計・必要になった時点で起工 |

3 層で「描かない(L1)/描くのをやめた(L2)/違う内容を描く(L3)」を覆う。本プロジェクトで実際に湧いた不体裁は全てこのどれかに落ちる(髪消失=L1・ちらつき=L1・穴=L2/L3・E2 白=L1 fallback 計上・誤 lighting=L4 予備)。

## 2. L2: draw-map 連続性オラクル(恒久・常時 ON)

### 2.1 守る不変条件
**「生存中の drawable からの draw record 剥奪は、同 frame 内の再登録とペアでなければならない」**(pre-T2 の暗黙意味論。T2 worker 化で再登録が数 frame 後に延びた = 穴の第一容疑)。

### 2.2 剥奪サイト台帳(実トレース済・2026-07-18)
| site | 場所 | 分類 |
|---|---|---|
| STRIP_DESTROY | lldrawable.cpp:200(destroy) | 終端・正当(監視のみ) |
| STRIP_CLEANUP | lldrawable.cpp:273(cleanupReferences) | 終端・正当 |
| STRIP_DELETE_FACES | lldrawable.cpp:506(deleteFaces ← regenFaces 経由) | **生存中・要ペア(第一容疑)** |
| CLEAR_GROUP_DTOR | llspatialpartition.cpp:139 | 終端・正当 |
| CLEAR_REBUILD_GENERIC | llspatialpartition.cpp:402(非 volume partition の rebuildGeom) | 同 frame 再登録前提・要ペア |
| CLEAR_LAST_ELEMENT | llspatialpartition.cpp:491 | 空群・正当 |
| CLEAR_ZOMBIE | llspatialpartition.cpp:851 | 終端・正当 |
| CLEAR_DESTROY_GL | llspatialpartition.cpp:927 | rebuild マーク済・要ペア |
| CLEAR_APPLY | llvovolume.cpp(applyGeoStaged: clear→registerFace 同一関数) | 自己ペア(監視で自己検証) |

実装済(2026-07-18)。追加で llvopartgroup.cpp ×2 / llvograss.cpp ×1 の clearDrawMap も CLEAR_REBUILD_GENERIC で配線(ただし particle/grass の record は mSrcDrawable を持たないため enumeration は素通り = L2 の identity 追跡は volume 族 record のみ)。terminal site(destroy/cleanup/dtor/last_element/zombie)は pending を作らず既存 pending の回収のみ。剥奪時に drawable が isDead なら site に関わらず terminal 扱い。

### 2.3 機構(LLVKContract 拡張)
- API 追加(`llvkcontract.h`):
  - `evictNotify(U32 site, const void* group, const void* drawable, U32 obj_local_id, U32 face_count)` — 剥奪時。終端 site は pending 登録せず既存 pending の解消のみ(dangling 防止)。
  - `registerNotify(const void* group, const void* drawable)` — LLVolumeGeometryManager::registerFace(llvovolume.cpp:6009)と generic 再登録点に設置。
  - frame 末 reconcile(既存 `frameBegin()` 内): pending の gap カウントを進め、gap ≥ 1 で cause `map_evict_unpaired`(site 名・gap・obj LocalID 付き・pow2 escalation)、再登録された entry は gap 分布(gap=0/1/2/3+)を 10s 集計へ。
- key = drawable ポインタ(**deref 禁止・照合のみ**)。identity(LocalID・face 数)は剥奪時に整数で捕獲、文字列化は発報時のみ(移動嵐でのコスト回避)。
- 終端 site(destroy/cleanup/dtor)が pending を必ず回収するため、解放済み drawable が pending に残らない。
- コスト: 剥奪/登録イベント時の hash map 操作 + frame 末の pending 走査(通常 0〜数百 entry)。draw hot path には一切触れない。

### 2.4 これが鳴ると分かること
`map_evict_unpaired site=STRIP_DELETE_FACES gap=3 obj=344387 n=8192` = 「regenFaces で剥奪された生存 obj が 3 frame 描かれていない」— 穴の機構・規模・identity が 1 起動で確定。前任の inline 強制ペアリング fix(llspatialpartition.cpp:180 `mVkForceInlineRebuild`)が実際に閉じているか否かも同時に実測される。

## 3. L3: 内容オラクル第 1 号 = T2 fill カーネル A/B 照合

### 3.1 守る不変条件
**「経路移行は同一入力に対して同一出力を書く」**。worker 経路 = `LLFace::buildVkGeoFill/runVkGeoFill`(llface.cpp:2170/2668)、参照 = 旧 `LLFace::getGeometryVolume`(llface.cpp:1129)。両者は同じ SIMD 系譜(index 転記・xform4a・planarProjection 等が同形)で、同一入力なら byte 同一が期待できる作り。

### 3.2 機構
実行点 = `applyGeoStaged` 成功直後(llvovolume.cpp:5766・main thread・fields 適用済・buffer 内容 = worker 出力)。標本 face(既定: job 毎 max 4 face・hash 回転で face クラスを巡回・`AYASTORM_GEOAB=full` で全数、`=0` で OFF)に対し:

1. **入力 drift 検出**: 現時点の入力で `buildVkGeoFill` を再実行(scratch LLGeoFaceFill)→ stage 時に保存した fill の param 部(mMatVert/mMatNormal/mTC[]/mColorRGBA/… dst ポインタ以外)と比較。不一致 → `geoab_input_drift field=<bitmask>` = 非同期窓で入力が動いた族。
2. **カーネル照合**: worker 出力領域(mDst* 各属性 × mNumVertices/mNumIndices 分)を scratch へ copy → `getGeometryVolume(force_rebuild=true)` を実行(旧経路が同領域へ書く)→ 属性毎 memcmp。不一致 → `geoab_kernel_mismatch attr=pos|norm|tc0|tc1|tc2|color|emissive|tangent|weights|index bytes=N first_off=… maxdiff=…` + face provenance(obj LocalID・face idx・flags: rigged/planar/texgen/bump/texanim/texmat・LOD・num_verts)。
3. 実装形(2026-07-18・設計から改良): 旧経路の strider 書込み先は mega slice でなく **staging バッファ**(`mMappedData`)と判明。よって「worker 出力(mega slice)vs 旧経路出力(staging)」を別メモリ同士で直接比較でき、**表示内容は一切置換されない純観測**になった。staging の stale 内容による偽陽性は「照合前に worker 出力を staging へ prime」で殺す(旧経路が書かない領域は prime 値のまま = 無音)。照合後は MappedRegion 簿記を空に戻し flush を発生させない(A/B 前に簿記が非空の buffer は照合自体を skip = 安全側)。

このため stage 時の `LLGeoFaceFill` を apply まで保持する(現行 `LLGeoStagedRebuild::mFills` は既に job に同居 = 追加保持コストなし)。

### 3.3 罠(実装時に必ず踏むトレース)
- `getGeometryVolume` の副作用(setState(GLOBAL)・mTexExtents 等)は正規 rebuild と同一の副作用 = 安全側。ただし GLTF selected 分岐(llface.cpp:1216 の再帰)は標本から除外(`tep->isSelected()` は skip)。
- 比較は byte 厳格から始め、maxdiff レポートで 1-ulp noise と実バグを仕分ける。noise 実在が判明した属性のみ epsilon 化(申告の上)。
- worker が同 fill を apply 後に再度触らないこと(現行 publish→apply で終端 = 触らない)を実装時に再確認。

### 3.4 これが鳴ると分かること
- `geoab_kernel_mismatch attr=tc0 texgen=planar rigged=0 …` = 939 行カーネルの特定 face クラスの内容バグが属性名・face クラス付きで名指し(前任の最有力仮説の直接検証)。
- `geoab_input_drift` = 非同期窓の入力ずれ族。
- 沈黙 = カーネル内容は白(仮説を 1 起動で棄却できる — これも成果)。

## 4. 穴バグへの初適用(本体制の実証運転)

1. L2 + L3 を実装・ビルド・3-path deploy(fix 仮説は一切実装しない)。
2. AYA 起動 1 回(カメラ移動で穴を再現 → 数分 → 終了 OK)。log = `~/.ayastorm_x64/logs/AYAstorm.log`。
3. 判定表:

| log 出力 | 確定すること | 次の一手 |
|---|---|---|
| L2 `map_evict_unpaired`(site・gap 付き) | 剥奪→再登録の窓が実在(前任 fix が閉じていない) | 当該 site の同 frame ペアリングを機構化 |
| L3 `geoab_kernel_mismatch` | カーネル内容バグ(属性・face クラス名指し) | 当該属性の kernel 差分を旧コードと突合して fix |
| L3 `geoab_input_drift` | 非同期入力窓 | stage 時 snapshot の設計是正 |
| 全装置沈黙のまま穴再現 | 症状族が 3 層の外(装置ギャップ) | 網を拡張(fire-level 連続性 or L4)— fix 仮説には行かない |

4. 名指し → fix → 起動 1 回で検収(オラクル沈黙 + AYA 視覚)。

## 4.5 運転モード(AYA 要望 2026-07-18 夜・初回起動で「重い」判明)

- **通常起動(既定)**: 装置は counting + `VKC-SUM`(1 行/10s・失敗ゼロなら無音)のみ。per-event WARNS・L2 sentinel・L3 A/B は全 OFF ≒ 追加コストほぼゼロ。
- **診断起動**: `AYASTORM_VKC=1` を付けて起動(例: `AYASTORM_VKC=1 AYASTORM_PERF_LOG=5 ~/ayastorm/ayastorm`)→ 全装置 full。`AYASTORM_GEOAB=full/0` は L3 の明示上書き。
- **判別実験モード**: `AYASTORM_GEOAB=repair` = L3 を全数化し、不一致検出時に参照出力(旧経路の正内容)をその場で buffer へ反映(`repaired=1` を log)。**穴が repair で消える → 幾何内容(source drift 族)が犯人と機械確定 / 消えない → 幾何は無罪確定**。適用範囲 = 入力 drift なし面のみ(volume 差替え面はサイズ危険のため非修復・申告)。診断専用(恒久 fix ではない)。
- `AYASTORM_PERF_LOG` に束ねない理由: AYA が常用するフラグのため「取る/取らない」の分離が成立しない。

## 4.6 初回運転の結果(2026-07-18 夜・穴再現起動 1 回)

3 層全て発報・skips=0 維持。判定表の複数行が同時に鳴った:
- **L3 `geoab_kernel_mismatch attr=norm`(全件 norm・非 rigged 面・maxdiff 最大 2.7・NaN 混入例あり)**: 両カーネルの norm 計算は同一 SIMD 系譜のため、最有力仮説 = 「volume の mNormals 内容が worker 読取り時と apply 時で異なる」(未確定 volume の読取り競合)。**断定せず装置を増強 = 3-way 判別**(worker-then vs 旧経路-now vs カーネル-now)を実装。次回診断起動の `verdict=src|kernel|both` が機械確定する。
- **L2 `evict{clear_apply}` が移動と比例**(静止 47→移動 430/10s)・`map_evict_long`(4frame+ 放置)が移動窓 396/10s。**strip_delete_faces はほぼ沈黙(1 件)= 前任が疑った strip 窓は主犯でない公算大**。機構仮説(未突合)= apply の clear→staged 再登録が、非同期窓中に group へ加わった drawable の record を巻き添え消滅させる。
- L3 `geoab_input_drift` = volume swapped(LOD 切替)+ fields=0x200(色)— 非同期入力窓の実在を定量化。

## 5. 恒久化と撤去

- L2 = 恒久・常時 ON(VKC-SUM に同居。失敗ゼロなら無音・コストは剥奪イベント時のみ)。
- L3 = T2 移行の gate 装置。**T2 gate 確定後も既定 ON の標本照合のまま残す**(コスト無視可・以後の geometry 改修の恒久見張り)か縮退させるかは gate 時に AYA 決裁。次の移行(E2 等)では同型の A/B を移行側が建てる(規律 §0-4)。
- 本 doc は装置の台帳を兼ねる(サイト表・cause 名は実装と同期して更新)。

## 縮小・省略・解釈申告(設計+実装)

- L4(束縛 checksum)は設計しない(必要事象が出た時点で起工)— 「全部検知」はしない、の合意に基づく。
- L2 の「visible 判定」は不要と解釈(剥奪ペアリング不変条件はカメラと無関係に成立すべき。オフスクリーンの未ペア剥奪も違反として数える)。
- L2 の identity 追跡は mSrcDrawable を持つ record(= volume registerFace 経路)のみ。particle/grass/terrain の record は素通り(設計スコープ通り・穴の対象族は volume)。
- L2 既知の許容偽陽性: ①face が正当に全 pass 喪失(全透明化等)②RLV/選択非表示で registerFace が early-out する obj — いずれも「生存中に draw map から消えた」事実の報告としては真で、obj id で識別可能。頻度実測で判断。
- L3 表示置換なし(staging 比較に改良・§3.2)。設計時の「旧内容が残る」記述は撤回。
- L3 は apply 成功 job のみ・buffer に既存 MappedRegion がある場合は skip・staging 確保失敗属性は無音 skip。
- L3 の drift 検査で `buildVkGeoFill` を再実行するため標本 face に stage 同等の副作用(GLOBAL/TEXTURE_ANIM state・mTexExtents 更新)が再発生する(正規 rebuild と同一の作用 = 安全側と解釈)。
- L3 の実行は volume 族のみ(worker 化されたのが volume 族のみのため)。terrain 等の generic 経路は L2 のみ。
- 標準ストレス走行の「スクリプト化(自動カメラ走行)」は本設計に含めない(現状は AYA の手動走行 1 回で足りる。起動回数が再び問題化したら別途起工)。
- VKC-SUM は「失敗ゼロ窓は無音」を維持するため、gap0 のみの窓の gap 統計は次の発報窓に繰越で表示される。
