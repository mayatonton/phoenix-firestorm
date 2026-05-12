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
1. **アルゴリズム選定**: wrap-around diffuse (Burley wrap) vs back light transmission (`max(0, -N·L)` 系) vs thickness map fake SSS のいずれを採用するか — `feedback_prefer_defaults_over_config.md` で 1 つに絞る
2. **対象 shader path 全棚卸し**: 葉 / 布 / 肌 / 紙 / 蝋燭 / 薄陶器 がそれぞれ実機で踏む shader path を **diagnostic 着色で実機トレース** (r19 P1.a で `materialF` だけ修正して bare prim を取り逃した事故を構造的に防ぐ)。候補: `materialF`, `diffuseF`, `pbropaqueF`, `avatarF`, `bumpF`, `diffuseAlphaMaskF`, `treeF`
3. **lit 計算注入点**: 各 shader の `nl = max(0, dot(N, L))` 直後に wrap 項を加算する形が最小手か、それとも `softenLightF` の sunlit 経路に統一注入するか
4. **WindLight 互換性**: sun_dir / moonlight の入力経路で透過項が暴れないか、Legacy Midday preset で副作用が出ないか
5. **HDR scene buffer 整合**: 加算項が HDR range を超えないか、saturate / clamp が必要か

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

### P0: 実装箇所調査 + spec 確定 (起票時点、未着手)

調査対象:

1. **アルゴリズム選定**: wrap-around diffuse / back light transmission / fake SSS の 3 候補から効果と実装コストで 1 つ確定
2. **対象 shader path 実機トレース** (r19 P1.a 学習を直接適用):
   - 葉 = `treeF` ? `materialF` ? `pbropaqueF` ?
   - 白布 = `materialF` BLEND / `diffuseAlphaMaskF` ?
   - 人物肌 (classic avatar / BoM) = `avatarF` ? `materialF` ?
   - 紙 / 蝋燭 / 薄陶器 = どの path ?
   - **方法**: diagnostic 着色 (`frag_color.rgb = vec3(1,0,0)` 等) を 1 shader ずつ入れて実機で対象オブジェクトが赤くなるか確認
3. **lit 計算注入点**: 各 shader 内で `nl = max(0, dot(N, L))` の **直後に wrap 項を加算** する案、もしくは `softenLightF` の sunlit 経路に統一注入する案、どちらが最小差分か
4. **3 OS 互換性**: `HAS_METAL` / `.metal` 経路の関数互換、`max(0, dot(N, L))` の OS 横断挙動
5. **HDR / preset 互換性**: Legacy Midday 等で副作用が出ないか pinpoint 除外要否を確認 (r17 / r18 の同思想)

### P1.a: 最小 fix 実装 (P0 確定後)

P0 で選んだアルゴリズム + 対象 path に対して最小差分で wrap 項を実装。

実装イメージ (案、P0 で確定):
```glsl
// 例: wrap-around diffuse (Burley wrap)
float wrap = 0.5; // r19 intensity から取得
float nl_wrap = (dot(N, L) + wrap) / (1.0 + wrap);
nl_wrap = max(nl_wrap, 0.0);
// 既存 nl の代わりに nl_wrap を使う、強度 cvar で blend
```

- `LLPipeline::bindDeferredShader` (slow path 末尾) と `bindDeferredShaderFast` (fast path) の両方で uniform push、live toggle 対応
- master AND 個別 sentinel AND intensity > 0 で gate
- intensity = 1 (default) で控えめ、3 で強め
- Legacy Midday は r17/r18 同思想で除外検討

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
