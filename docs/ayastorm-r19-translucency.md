# AYAstorm r19: 薄物の透過 (B 軸第 1 弾、translucency / subsurface 風)

**作成日**: 2026-05-13 (初版起票、r19 albedo fidelity drop からの差し替え)
**対象**: AYAstorm `feature/aya-r19-translucency-spec-draft`
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §5 B 軸 (物質色) の第 1 弾、A 軸完走 (r14 + r15 + r16 + r18 = `v7.2.4-ayastorm-r18` 想定) の次。元の r19 (アルベド忠実度) は `feature/aya-r19-albedo-fidelity-spec-draft` に frozen archive として温存

> **本書の役割**: r19 個別の **計画スナップショット**。`feedback_release_with_user_feedback.md` 流儀で「完璧な spec を組まず、shader 触りながら追記」運用。
> 章全体の位置づけは `docs/ayastorm-visual-realism-roadmap.md`、A 軸 spec は r14-r18 各 spec、章 thesis は memory `project_ayastorm_visual_realism_chapter.md` を参照。

---

## 1. ゴール

A 軸で「**空気と空間**」を物理的に積み終えた。B 軸ではその空気の中に置かれる **物質本来の色と質感** を取りに行く。r19 では B 軸の入口として **薄物の透過** を解く。

具体的に出したい体感:

- **葉が太陽に向かって透ける** — 木陰で空を見上げると、葉が太陽光を透過して縁が明るく光る、葉脈や輪郭の質感が出る
- **白い布カーテンに光が回る** — 窓辺の薄布カーテンの裏側に立つと、光が透過してカーテンが内側から発光しているように見える
- **人物の耳・鼻翼・指先が透ける** — 強い順光で人物の耳の縁が赤く透ける、SS で「肌が生きている」感じになる
- **紙・蝋燭・薄い陶器の半透明感** — 薄い物質の質感が出る

写真撮影テーマ「物質が見せる本来の色と空気と雰囲気」直球。光が物質を貫通する効果は **instant A/B で誰でも分かる** 視覚インパクトを持ち、r19 P1.a (albedo fidelity) で躓いた「数学的に正しいが視認困難」問題を構造的に解消する。

技術スコープ:
- **wrap-around diffuse 系の back light 簡易透過** を 1〜複数 shader path に追加、太陽光の透過成分を normal 反対側からも受け取る
- 物理正確 SSS (thickness map / multi-scatter) は **採用しない** (r14-r18 と同様、最小手で最大体感)

---

## 2. 設計制約

`docs/ayastorm-visual-realism-roadmap.md` §2 の境界条件を継承:

- **保つ**: PBR / deferred の shader interface (frag_data layout、GBuffer flag、uniform 命名)、WindLight preset 互換 (preset 値は input 契約として保持)、tonemap / postDeferredGammaCorrect の取付け点、HDR scene buffer の信号特性
- **書き換える**: legacy / PBR の lit 計算に **wrap-around diffuse 項** を加算する形で透過光を表現、shader 内部の light response を物理的により正確な形に
- **言い換え**: 物質の lit 計算に「光が背面から透けて回り込む」項を最小手で足す、preset / asset 仕様変更は要求しない

### GBuffer flag 不採用

- **HAS_ATMOS_LINEAR 形式の 5 値化 flag は採用しない** (r19 P1.a frozen で 1 度使ったが今回不要、混乱回避)
- writer 修正ではなく **lit 経路 (lighting pass / shader 内 lit 計算) 側** に透過項を加算する形 → GBuffer 信号構造に手を入れない

### Master switch + 個別 switch + intensity

- master `AYAVisualRealismEnabled` (U32、r18 で確立) を共有 — Firestorm View / AYAstorm View 切替で r14-r19 全機能を一括 on/off
- 個別 sentinel `AYAR19TranslucencyEnabled` (Boolean、default TRUE) を r16/r18 と同形で配置
- 強度段階 `AYAR19TranslucencyIntensity` (U32、0-3 段階、default 1) — 0=OFF / 1=控えめ (default) / 2=標準 / 3=強め。preset 互換破壊を避けつつ「強く効かせたい配信者」の要望に応える。memory `feedback_combo_box_u32_cvar.md` (combo_box は U32 が安定) 準拠

### 永久 drop

- **物理正確 SSS** (thickness map / multi-scatter / diffusion profile) — heavy 計算、preset 制作変更要求、roadmap §6 と同方針
- **preset 制作変更要求** — viewer 単独で完結
- **PBR pipeline 根本書き換え** — input/output 契約は維持
- **LUT / color grade による「写真風 look」誤魔化し** — 章 thesis 整合
- **暗がり依存のリアリティ偽装** — 章 thesis 整合

---

## 3. スコープ

### 含む (P0 Survey 完了で確定)

P0 Survey で **r19 P1.a 失敗の学習を直接適用**: 「主流ユースケースで instant A/B 視認可能か」を起票時点でゲート化し、対象 shader path を **机上 listing ではなく実機トレース** で全棚卸しする。

P0 Survey ターゲット:
1. **アルゴリズム選定** [**Round 1 で確定: Wrap + Back combo**]: wrap-around diffuse (Burley wrap) と back light transmission (`max(0, -N·L) * max(0, V·L)`) の **加算合成**。葉/布/耳の 3 ケース全てで instant A/B 視認可能な形を選ぶ (Back-only は布の柔らかさが出ず、Wrap-only は「透ける」絵にならない)。thickness map fake SSS は heavy で drop
2. **対象 shader path** [**Round 1 で確定: softenLightF 一点 (案 A)**]: 太陽 sun_contrib に対する加算で完結。PBR ブランチ (`pbrBaseLight` 呼び出し) と Legacy ブランチ (`sun_contrib = min(da, scol) * sunlit_linear`) の **両方** に同じ式を差し込む。局所灯 (point/spot light) は r19 スコープ外、太陽光主導の絵が r19 のゴール。**diagnostic 着色で実機ヒット確認は依然必須** (R1 リスク防止)
3. **lit 計算注入点** [**Round 1 で確定: softenLightF.glsl の sun_contrib 加算**]: Legacy 側は `sun_contrib += transmit` の単純加算、PBR 側は `pbrBaseLight` 呼び出し前後で `transmit` を計算し戻り値に加算 (helper を増やさない最小差分)
4. **WindLight 互換性**: sun_dir / moonlight の入力経路で透過項が暴れないか、Legacy Midday preset で副作用が出ないか
5. **HDR scene buffer 整合**: 加算項が `clampHDRRange` で潰れないか、saturate / clamp が必要か

### 含まない (drop)

- **物理正確 SSS** (B 軸第 2 弾以降 / 永久 drop 候補) — thickness map / diffusion profile / multi-scatter
- **アルベド忠実度** (frozen archive、`docs/ayastorm-r19-albedo-fidelity.md`) — sustained 検証で効果再確認できれば将来復活
- **material response 全体** (B 軸第 2 弾以降、r20+) — roughness / metallic の物理整合
- **カメラ表現** (C 軸、r21+) — DoF / auto-exposure / grain
- **preset 制作・新 preset 出荷** (AYA 方針: preset を作り直さない)
- **heavy raymarch / 重い shader 拡張** (roadmap §6 で永久 drop)
- **GBuffer flag 拡張** (HAS_ATMOS_LINEAR は r19 frozen で 1 度試した、今回は使わない)

### 永久 drop

- LUT / color grade による「写真風 look」誤魔化し (roadmap §6)
- 暗がり依存のリアリティ偽装 (章 thesis に反する)
- WindLight preset 互換破壊
- 物理正確 SSS (heavy 計算、preset 仕様変更要求)

---

## 4. フェーズ分解

viewer-only の改修。配信側 / SIM 側変更なし。r14-r18 と同流儀。

### P0: 実装箇所調査 + spec 確定 (Round 1 完了 2026-05-13、残: shader 実機トレース / 3 OS / preset)

Round 1 確定事項:
- **アルゴリズム = Wrap + Back combo** (Burley wrap + 背面透過項の加算合成)
- **注入点 = `class3/deferred/softenLightF.glsl` の sun_contrib 加算 1 点** (PBR/Legacy 両ブランチに同形式で差し込む)
- **r19 では太陽光のみ対応** (局所灯への展開は r20+ 検討)

残調査 (Round 2 以降):
1. **diagnostic 着色で実機ヒット確認** (R1 防止): softenLightF の Legacy 経路 / PBR 経路に `frag_color.rgb = vec3(1,0,0)` を順次入れて、葉 / 白布 / 肌 / 紙 / 蝋燭 がどちらの経路を踏むか実機確認
2. **3 OS 互換性**: GLSL のみで完結 (`.metal` ファイル不在を確認済)、`dot/clamp/pow` の OS 横断挙動確認
3. **HDR / preset 互換性**: Legacy Midday 等で副作用が出ないか pinpoint 除外要否、`clampHDRRange` が wrap+back 加算後の値を潰さないか

### P1.a: 最小 fix 実装 (P0 Round 1 確定済、残調査と並行可)

確定: `class3/deferred/softenLightF.glsl` 1 ファイルに Wrap + Back combo を sun_contrib に加算。

実装イメージ (太陽光、softenLightF Legacy ブランチ):
```glsl
// uniform: vec3 aya_translucency_params; // (wrap, k_back, k_view) を C++ から intensity で push
// uniform: vec3 aya_translucency_tint;   // 葉緑/肌赤の傾向は単一 tint で控えめに統一 (1 件で過剰応答せず)
// uniform: float aya_translucency_strength; // 0 で完全 no-op

vec3 L = light_dir.xyz;
vec3 N = gb.normal;
vec3 V = -normalize(pos.xyz);
float NdotL = dot(N, L);

// Wrap 項: nl_wrap = (NdotL + w) / (1 + w)
float w = aya_translucency_params.x;
float nl_wrap = max((NdotL + w) / (1.0 + w), 0.0);
// wrap 効果は sun_contrib の da を nl_wrap に置換する形で適用
// (Legacy 側 da = clamp(NdotL, 0, 1) → nl_wrap に差し替え、scol との min は維持)

// Back-light transmission: 太陽の真裏かつ太陽方向を見る時にだけ強く出る
float back  = pow(max(-NdotL, 0.0), aya_translucency_params.y);
float view  = pow(max(dot(V, L), 0.0), aya_translucency_params.z);
vec3 transmit = back * view * aya_translucency_tint * aya_translucency_strength * sunlit_linear;

// Legacy 側: sun_contrib = min(da_wrap, scol) * sunlit_linear + transmit
// PBR 側:    color = pbrBaseLight(...) + transmit * baseColor.rgb;
//   (pbrBaseLight 結果に同形 transmit を加算、baseColor で物質色を保持)
```

- intensity 0/1/2/3 に応じて `(w, k_back, k_view, strength)` を C++ 側でテーブル化
  - 0: strength=0 (完全 no-op)
  - 1 (default): w=0.15, k_back=2.0, k_view=4.0, strength=0.6
  - 2: w=0.25, k_back=1.5, k_view=3.0, strength=1.0
  - 3: w=0.35, k_back=1.2, k_view=2.5, strength=1.5
- `LLPipeline::bindDeferredShader` (slow path 末尾) と `bindDeferredShaderFast` (fast path) の両方で uniform push、live toggle 対応
- master AND 個別 sentinel AND intensity > 0 で gate
- Legacy Midday は r17/r18 同思想で pinpoint 除外検討 (P1.b で sustained 観測)

### P1.b: 体感調整 / drop 判断 (条件付)

P1.a の Linux 実機検証で:
- **instant A/B で葉/白布/耳の透け感が視認できる** こと (r19 P1.a 失敗の主因をここでゲート、視認できなければ即 drop)
- sustained viewing で違和感なし、preset 互換維持
- 副作用が大きい場合は intensity の default を下げる / 対象 path を絞る

### P2: 3 OS ビルド + 体感確認

AYA が Linux フルビルド + 体感確認。問題なければ macOS / Windows ビルドへ。`feedback_release_with_user_feedback.md` 流儀。

### P3: tag / release

r14-r18 と同様、**公開は r19 単独でせず B 軸完走時 (r19-r21 想定) に一括公開判断** の運用。B 軸 3 リリース完走時に `v7.2.4-ayastorm-r21` 等で tag。

---

## 5. 受け入れ条件 (P0 Survey 完了で確定)

- [ ] `AYAVisualRealismEnabled = 1` (= AYAstorm View) かつ `AYAR19TranslucencyEnabled = 1` で:
  - **instant A/B で**葉が太陽に向かって透ける効果が視認できる (r19 P1.a 失敗の主因をゲート条件化)
  - **instant A/B で**白布カーテン裏に光が回るのが視認できる
  - **instant A/B で**人物の耳の縁の透け感が視認できる
  - r14/r15/r16/r18 の効果が壊れない
  - sustained viewing で違和感なし
- [ ] `AYAVisualRealismEnabled = 0` (= Firestorm View) で r13 までの lit 計算に戻る (wrap 項完全 no-op)
- [ ] `AYAR19TranslucencyEnabled = 0` で master ON でも r19 fix が完全 no-op
- [ ] `AYAR19TranslucencyIntensity = 0` で intensity ゼロ = wrap 項完全 no-op
- [ ] `AYAR19TranslucencyIntensity = 1/2/3` で段階的に強度が増える
- [ ] WindLight preset (朝・昼・夕・夜・昼間レガシー) 互換維持、特に Legacy Midday で副作用なし
- [ ] PBR material 表示が破壊されない (`pbropaqueF.glsl` 修正範囲は wrap 項加算のみ、interface 不変)
- [ ] 3 OS でビルド + 起動 + 表現確認 (P2 — Linux 優先、macOS / Windows は B 軸完走時 tag/release で一括)
- [ ] FPS 影響が ±5% 以内 (wrap 計算は 1 dot + 1 mad / pixel 程度、計算コスト微小)

---

## 6. リスク

| ID | リスク | 対策 / 現ステータス |
|---|---|---|
| R1 | **対象 shader path 棚卸し漏れ** (r19 P1.a 全 drop の主因と同じ事故再発)。葉や肌や白布が実機で踏む path が `materialF` 以外にも分散していて修正が一部に偏ると instant A/B 視認できず drop ループに入る | **P0 Survey で diagnostic 着色を 1 shader ずつ実機トレース必須化** (机上 listing 禁止)。`feedback_doubt_self_first.md` + 「効果出るシーン」を起票時点で実機確認 |
| R2 | wrap 項が物理的に派手すぎて preset 互換破壊 (特に Legacy Midday で sun_dir が水平に近く wrap が暴れる) | intensity slider (default 1 = 控えめ) + r17/r18 と同思想の Legacy Midday pinpoint 除外を検討 |
| R3 | アバター肌 path (avatarF) と prim 系 (materialF/diffuseF/pbropaqueF) で wrap 効果が統一されず、対象オブジェクトごとに効き目が違う | P0 で対象 path 全棚卸し時点で「全 path に同じ wrap 項を入れる」設計、shader header の lit helper に注入することで構造的に統一する案を検討 |
| R4 | PBR shader (`pbropaqueF.glsl`) への変更が PBR spec から逸脱 (Burley wrap は PBR ではなく ad-hoc 拡張) | r19 sentinel で隔離、Firestorm View / AYAR19 OFF で PBR spec 通りの挙動に完全復元できる構造を維持 |
| R5 | 3 OS で wrap の出方が違う (Metal vs OpenGL の dot/clamp 数値差、normal 精度差) | 計算式を OS 中立に保つ、`HAS_METAL` 経路の差分を P0 で確認 |
| R6 | sustained viewing で違和感 (常時 wrap が乗ると「全てが透けて見える」CG 感が出る、章 thesis の「写真らしさ」と逆方向) | intensity 既定値を控えめに、P1.b で sustained 検証必須、必要なら材質判定 (alpha 値 / spec 値 / vertex_color 値) で wrap を on/off するヒューリスティクスを追加検討 |
| R7 | `feedback_instant_ab_vs_sustained.md` の罠 — instant A/B でだけ派手で sustained で違和感 (r14 / r17 の drop pattern) | P1.b で **instant A/B と sustained の両方で OK 必須**、片方だけ PASS では release しない |

---

## 7. 更新履歴

- 2026-05-13 (初版起票): r19 albedo fidelity を P1.b 実機検証で全 drop (BoM avatar 経路は writer 修正効くと確認したが bare prim 別 path / 数学的差分 5-6% で instant A/B 視認困難、`docs/ayastorm-r19-albedo-fidelity.md` §7 で経緯記録)。AYA との対話で **B 軸第 1 弾を「薄物の透過」に差し替え** を確定 (`docs/ayastorm-visual-realism-roadmap.md` §5 B 軸候補「薄物の subsurface scattering: 葉・カーテン・薄い布」直接接続)。差し替え理由は (1) 写真撮影テーマ「物質が見せる本来の色と空気」直球、(2) instant A/B で誰でも分かる視覚インパクト (葉が太陽に透ける / 白布カーテン裏に光が回る / 人物の耳が透ける)、(3) r19 P1.a の「数学的に正しいが視認困難」問題を構造的に解消。次は P0 Survey (アルゴリズム選定 + 対象 shader path 実機トレース)
- 2026-05-13 (P0 Round 1 完了): **アルゴリズム = Wrap+Back combo**、**注入点 = `class3/deferred/softenLightF.glsl` の sun_contrib 加算 1 点** を確定。`class1/deferred/deferredUtil.glsl:457 pbrPunctual` と `softenLightF.glsl:213-238 Legacy sun_contrib` を読み、(A) softenLightF 一点 (B) pbrPunctual (C) 両方 (D) 全 4 lit shader の 4 案で比較 → r19 は太陽光主導 / 局所灯透過は二次効果という整理で **(A) 案 + Wrap+Back 加算合成** を採用。Wrap-only は「透ける絵」にならず、Back-only は布の柔らかさが出ない、両者の和で 3 ケース (葉/布/耳) を同一式でカバー。intensity 0-3 のテーブル化方針も §4 P1.a に明記。次は P0 Round 2 (diagnostic 着色での実機トレース) または P1.a 実装着手
- 2026-05-13 (P0 Round 3 = 全 deferred 経路の routing trace 完了): canary 着色で「壁/Mesh 服/avatar/耳/葉/地面」が踏む writer shader → softenLightF 分岐の対応を 2 時間トレース。結果として **softenLightF への一点注入で Legacy 系 (壁/avatar/耳/旧服 = materialF) と PBR Mesh 服 (= pbropaqueF) の両方をカバーできる** ことを確定。`gbufferFlag` 値で `HAS_PBR` / `HAS_ATMOS` / `HAS_HDRI` / `SKIP_ATMOS` の 4 分岐に分かれ、PBR 服は `HAS_PBR` 分岐 (`pbrBaseLight()` 後) に transmit を、Legacy は wrap で `da` を拡張 + transmit を `baseColor.rgb` 乗算後に加算する 2 ヶ所注入方式に確定。routing 全分岐表は `docs/ayastorm-deferred-shader-routing.md` に永続化 (再解析回避)。葉/地面は forward alpha と pbrterrain で softenLightF を経由しない可能性が残るので P1.b 実機で確認
- 2026-05-13 (P1.a 実装): settings.xml に `AYAR19TranslucencyEnabled` (Boolean, default 1) と `AYAR19TranslucencyIntensity` (U32, default 1, range 0-3) を追加。`class3/deferred/softenLightF.glsl` に uniform 3 件 (`aya_translucency_params` vec4 = (wrap, k_back, k_view, strength), `aya_translucency_tint` vec3) + ヘルパー 2 件 (`ayaTranslucencyWrap`, `ayaTranslucencyTransmit`) を追加し、PBR 分岐 (`pbrBaseLight()` 後) と Legacy 分岐 (`da` への wrap 適用 + `baseColor.rgb` 乗算後の transmit 加算) の 2 ヶ所に注入。`pipeline.cpp` の `renderDeferredLighting` softenLightF bind 直後に intensity tier (0=OFF/1=控えめ/2=標準/3=強め) の lookup table から uniform を push。OFF 経路は strength=0 で shader 側 short-circuit。次は P1.b で 3 OS 実機検証 (葉/白布/耳の instant A/B + sustained viewing 違和感確認)
- 2026-05-13 (P1.b Linux 実機受入 PASS): AYA さん Linux 実機で全項目 PASS。検証材料は板プリム単体 (薄物 Mesh 検証は本質的に板プリムの延長と判定して skip)。確認結果: (a) instant A/B = `AYAR19TranslucencyEnabled` ON/OFF で板プリム裏面の wrap 視認可、`AYAR19TranslucencyIntensity` 0/1/2/3 で段階差出る、板プリムの厚み変化でも透け具合変化を観測 (= `scol` 経由 self-shadow による副次的 thickness 依存性が機能)、(b) sustained viewing 5-10 分の歩き回りで違和感なし、(c) WindLight preset (朝/昼/夕/夜/Legacy Midday) 切替で破綻なし、(d) r14-r18 効果 (雲 volumetric / 大気) 維持、(e) `AYAVisualRealismEnabled = 0` で r13 までの絵に完全復元、(f) `AYAR19TranslucencyEnabled = 0` で master ON のまま r19 だけ pinpoint 無効化可能。検証中に判明した scope 外事項: prim transparency > 0 は SL の forward alpha 経路 (`alphaF`) に逃げ softenLightF を経由しないため r19 と無関係、Intensity 変化が反映されない (= forward 経路への注入は P2 候補 / r20 検討)。AYA さん指摘により「平面 prim では subsurface 感が原理的に出ない」とした僕の初期説明を訂正、`scol` 経由の厚み依存性は副次効果として確かに機能している。3 OS 残: macOS / Windows は B 軸完走時 (r19-r21) で一括 tag/release
