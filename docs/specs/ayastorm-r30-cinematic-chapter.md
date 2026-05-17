# AYAstorm r30 — 撮影描画 mode 新設 (Cinematic) + 3 モード再起動切替統一

**作成日**: 2026-05-17
**最終更新**: 2026-05-17 (α 実機検証で軸 A 仮説 falsify、章構成を Cinematic 主軸に再設計)
**ステータス**: 章レベル骨子 (個別 phase の詳細 spec は P1 着手時に別ファイルで起こす)
**前提**: なし — r25-r29 (3D stream 章) は別オーナーに委譲 (2026-05-17)、AYA さんは r30 を即着手可能
**章スコープ**: r30 系 (複数 release で段階構築、章クローズは P6+ で AYA 色到達時点)
**オーナー**: AYA さん主軸 (r25-r29 系は別オーナー、本章とは独立並走)

このドキュメントは AYAstorm r30+ で着手する **撮影描画章** の章レベル骨子。

- **主軸 (Cinematic)**: velocity buffer / Volumetric Light / Motion Blur / BD DoF chain を BD から borrow して撮影専用 mode を新設、BD クラスの絵を写真撮影 viewer として実現、最終的に Cinematic 用 AYA 色で BD を超える
- **インフラ (P1)**: 3 モード (Firestorm View / AYAstorm View / Cinematic) すべてを再起動切替に統一、Cinematic を View Mode UI に枠で先行追加。pipeline は起動時 1 回構築、ランタイム gate 無し

個別 phase の実装詳細は、各 phase 着手時に `ayastorm-r3x-...` 系の独立 spec で展開する。

---

## 1. Core thesis

### 1.1 「BlackDragon を真似て BlackDragon を超える」

撮影描画の精細さでは BD が SL viewer 界のリファレンス。BD の絵作りを **エンジン部分だけ** 取り込み、AYAstorm 独自の色作りで上書きして、最終的に BD を超える絵を目指す。

### 1.2 三位一体ポジショニング

AYAstorm の独自ポジションは Firestorm fork 群の中で唯一の三位一体構成:

| 軸 | 担当 viewer | AYAstorm での扱い |
|---|---|---|
| Usability / UX | Firestorm | base 維持、UI を一切変更しない |
| 撮影描画の精細さ (エンジン) | BlackDragon (NiranV Dean) | shader + pipeline 連鎖 + tone 支配 + default cvar を Cinematic mode に取り込む (**UI は取り込まない**) |
| 色作り (絵作り vision) | AYA | AYAstorm View では r14+ 色を引き続き使用 / Cinematic では新規探索 (P6+) |

**章の到達目標**: 「描画レベルが格段に向上したエンジン (Cinematic)」を手に入れ、BD と並走できる撮影 viewer ポジションを確立する。その上で Cinematic 用 AYA 色で BD を超える。

BD UI (Machinima Sidebar / Photo Tools panel / `panel_machinima.xml`) は AYAstorm に持ち込まない / Firestorm 流儀に翻訳もしない (AYA さん明示判断 2026-05-17)。

### 1.3 r14+ 章積み残しの再定義 — 本丸は Cinematic で達成する

AYA さんは r14+ 視覚表現章で本来 AYAstorm View を BD クラスに近づけたかったが諦めた経緯がある。当初の r30 計画では「P1 で AYAstorm View に SMAA/SSR を default ON することで積み残しを回収する」想定だったが、**2026-05-17 の α 実機検証で仮説が falsify された**:

- α 検証: AYA さん実機で `RenderFSAAType=2, RenderFSAASamples=3, RenderScreenSpaceReflections=1` を手動投入
- SMAA Ultra は featuretable tier 経由で既に有効化されており差分は SSR のみ
- AYA さん主観判定: 「たしかに効いてるんですが、びっくりするほどではないというか。。気づく人はほとんどないだろう差ですね」(SSR 単体は決定的な絵の引き上げにならない)

この結果から **shader plumbing の死蔵解消だけでは AYAstorm View は生き返らない** ことが確定。BD クラスの絵を作っているのは shader plumbing ではなく velocity buffer / Volumetric Light / Motion Blur / BD DoF chain 側であり、これらは AYAstorm View に default ON するには重すぎる。

よって r14+ 章の本丸 (「写真を撮るに値する空気と空間」) は **Cinematic mode で達成する** と再定義する。AYAstorm View は r14+ で積み上げた色 (Kelvin/aerial/godrays 等) を浴びる「普段使い」mode として据え置く。

### 1.4 モード構成 (View Mode UI 拡張、3 モード全部再起動切替に統一)

| モード | 切替 | pipeline | 色作り | 用途 |
|---|---|---|---|---|
| Firestorm View | 再起動 | 上流 Firestorm そのまま | 色付け無し | 低スペック / preset 厳密派 |
| AYAstorm View | 再起動 | 上流 Firestorm そのまま + r14+ 色 shader | r14+ 色 (Kelvin/LUT/aerial/godrays/translucency/SSS 等) | 普段使い |
| **Cinematic (P2+ 新設)** | 再起動 | + velocity buffer + Volumetric Light + Motion Blur + BD DoF chain | Cinematic 用 AYA 色 (P6+ で新規探索) | 撮影 mode、r14+ thesis の本丸はここで達成 |

**全モード再起動切替の意味**:
- pipeline は起動時 1 回構築、ランタイム gate 一切なし
- r17 Kelvin gate のような per-frame switch 保守地獄を完全回避
- ユーザーは「自分が普段どのモードで使うか」を起動時に確定 (低スペックは Firestorm View で固定、撮影時は Cinematic に切替再起動)
- Firestorm 自身も PBR/Forward を再起動切替している前例に合致 (SL viewer 流儀から外れない)
- 現状 AYAstorm/Firestorm View 間は即時切替だが、Cinematic 投入と合わせて 3 モード一貫の再起動切替に統一する

---

## 2. 出荷条件 (Acceptance criteria)

### 2.1 Cinematic の出荷条件

Cinematic モードを「初回 ship」する条件は **BD で撮った写真と並べて、第三者ブラインドで BD と AYAstorm Cinematic が見分けられない (もしくは AYAstorm の方が好評)** 水準。半端な状態で ship しない。

これが守れない場合の選択肢:
- **延期**: P5 ゲートを通過するまで Cinematic 単体タグを切らない (内部 dogfood のみ)
- **撤退**: P4 までで BD 同等が見えない場合、章スコープを「BD と並走できる photo viewer」から「Firestorm + AYA 色の選択肢提示」に縮小し、Cinematic 名乗りを取り下げる

「ある程度動くから ship する」は不採用 (`feedback_feature_value_in_main_usecase.md` を章スコープに適用)。

### 2.2 既存 2 モードの不可侵

- **Firestorm View**: 上流 Firestorm そのまま、章作業中に描画変更を一切加えない (低スペック / preset 厳密派の逃げ場)
- **AYAstorm View**: r14+ で積み上げた色作りを維持、章作業中に絵作りを変更しない (普段使いの安定枠)

### 2.3 3 OS 同時 ship

Linux / macOS / Windows いずれかが落ちる状態では release を切らない (`project_ayastorm_three_platforms.md`)。Mac は OpenGL deprecation が進んでいるため、shader 取り込み時点で Mac kill-switch を併設する判断は phase ごとに留保。

---

## 3. Phase 構成 (r30〜r35+)

各 phase は **1 release = 1 phase** を原則とする。release 番号は r25-r29 の 3D stream 章完了タイミングで確定。

### P1 インフラ — 3 モード再起動切替統一 + Cinematic 枠先行追加 (想定 r30)

**実装**:
- View Mode UI の切替を **3 モードとも再起動切替** に統一 (現状 AYAstorm/Firestorm View 間は即時切替)
- 再起動切替 dialog を View Mode 切替時に必須化
- pipeline 構築コード (LLPipeline / LLViewerShaderMgr 等) の起動時 1 回構築に View Mode 分岐を入れる
- View Mode UI に **Cinematic 選択肢を追加** (preview / disabled 表記、選択しても pipeline は AYAstorm View 相当で動作)

**取り込み shader**: **ゼロ**

**ship 判定**:
- 3 モード切替が再起動を経て確実に動作
- 既存 AYAstorm View / Firestorm View 双方で従前と同じ絵が出ること (regression なし)
- Cinematic 枠が UI に見えており、選択しても破綻しないこと

**注**: P1 では AYAstorm View に shader plumbing 引き上げ (SMAA/SSR default ON) は **行わない**。α 検証で絵が動かないと確認済み。

### P2 Cinematic モード骨格 + velocity buffer + SMAA T2x (想定 r31)

**スコープ確定** (§7-7 案 A 採用、2026-05-17): velocity buffer + SMAA T2x を **両方 P2 で完成**。視覚で確認しながら P3 以降を判断する方が後段スムーズという AYA 判断による。工数: velocity 5 日 + T2x 4-5 日 = **9-10 日**。

**取り込み**:
- velocity buffer 一式 (BD から borrow、9 ファイル):
  - `avatarVelocityF.glsl`, `avatarVelocityV.glsl`
  - `skinnedVelocityV.glsl`, `skinnedVelocityAlphaV.glsl`
  - `velocityF.glsl`, `velocityV.glsl`, `velocityFuncV.glsl`
  - `velocityAlphaF.glsl`, `velocityAlphaV.glsl`
- velocity buffer 生成 pipeline (gbuffer に per-pixel velocity vector を書き込む path) を C++ 側に新規追加
- **SMAA T2x 完成**:
  - `SMAAResolve{V,F}.glsl` を AYAstorm 自作 (BD 未存在、SMAA reference impl から起こす、§7-7)
  - `SMAA.glsl` `SMAA_DECODE_VELOCITY` macro 1 行 + Cinematic 用 `SMAA_REPROJECTION` permutation 追加
  - `mSMAAHistory` RT + `resolveSMAAT2x()` + `applySMAA()` T2x 分岐 + jitter projection 経路 (`gGLProjection` を毎フレーム ±0.5px shift)
  - `gSMAAResolveProgram[4]` + `gSMAANeighborhoodBlendT2xProgram[4]` 8 program load
  - `RenderFSAAType` enum 拡張 (`3 = SMAA T2x`)、Cinematic 起動で auto-set
- **Cinematic mode で SMAA T2x + SSR を default ON** (velocity buffer 入力が揃うので SMAA は T2x に格上げ、SSR は Cinematic の重さの中では微差が誤差で吸収される想定)
- Cinematic 選択肢を preview から実質有効に昇格

**ship 判定**:
- Cinematic モード起動時に SMAA T2x が機能 (camera pan で edge AA 改善が目視可能、sub-pixel jitter blend で「ジャギが時間方向に滲んで消える」)
- velocity buffer 取り込みで AYAstorm View / Firestorm View に regression が無い (Cinematic 専用 path に閉じ込められている)
- 3 OS で動作

### P3 写真品質の核 — Volumetric Lighting (想定 r32)

**取り込み**:
- `class3/deferred/volumetricLightF.glsl` (BD 本体)
- `class1/deferred/volumetricLightF.glsl` (BD class1 stub)
- Volumetric Light 5 cvar (BD default 値を Cinematic で踏襲)
- pipeline 連鎖位置の決定 (deferred → post 連鎖でどこに挿入するか)

**ship 判定**:
- Cinematic モードで「光が空間を満たす」感が出ている (室内 / 屋外で BD と並列比較)
- High-Res snapshot で volumetric が破綻しない (撮影 viewer 核ユースケース)
- AYAstorm View / Firestorm View に regression が無い

### P4 映像品質 — Motion Blur + BD DoF chain (想定 r33)

**取り込み**:
- Motion Blur (Geenz 改良): `class1/deferred/motionBlur{F,V}.glsl`
- BD 独自 DoF chain: `dofCombineF.glsl`, `postDeferredHQDoFF.glsl`, `postDeferredNoDoFF.glsl`
- DoF-only Chromatic Aberration cvar (`RenderDepthOfFieldChroma`, `RenderChromaStrength`) + 関連 shader 改修

**ship 判定**:
- Cinematic の動画 / カメラ pan が BD 同等品質
- DoF 撮影で色収差が cinematic らしく乗る
- High-Res snapshot 時の HQ DoF が BD 同等で破綻しない

### P5 BD 同等到達ゲート (想定 r34) — **Cinematic 初回正式 ship**

**実装**:
- View Mode UI で Cinematic を `preview / experimental` 表記から **正式モード** に昇格
- P2〜P4 で追加した Cinematic 用 cvar 群を Preferences の AYAstorm セクションに整理統合 (既存 Firestorm Photo Tools floater は据え置き、Cinematic 専用 floater / Sidebar は **作らない**)
- BD UI (Machinima Sidebar / Photo Tools panel / `panel_machinima.xml`) は **取り込まない / 翻訳もしない**

**ship 判定 (= 章にとって最重要ゲート)**:
- **第三者ブラインド A/B**: BD で撮った写真と AYAstorm Cinematic で撮った写真を並べて、SL コミュニティ内の photo 系ユーザー複数名に判定してもらい、BD 同等以上の判定を得る
- 判定に届かない場合は **ship しない**。P6 に進まず、P1〜P5 のどこに不足があるかを再 survey して差分 release を切る

このゲートを通過した時点で Cinematic を「正式モード」として release notes / README で告知する。

### P6+ BlackDragon を超える (想定 r35+ 複数 release)

**実装**: AYA が完成した Cinematic pipeline の絵を実機で見て、**新規に** AYA 色を起こす。

**重要 — 既存 AYA 色 (Kelvin/LUT/aerial 等) を Cinematic に流用しない**:
- AYAstorm View が出している色は AYAstorm View の pipeline で見たときに最適化された色作り
- Cinematic pipeline は velocity / Volumetric / Motion Blur / BD DoF で全く別の絵を出すため、その絵を見てから新規に色を載せる
- LUT / tone / Kelvin 系の cvar は技術的には借用可能だが、**Cinematic 用の値 / cube / curve を独立に設計**し、AYAstorm View の値とは別軸で持つ

**ship 判定**:
- BD と並べて「BD とは別の色だが BD と同等以上に美しい」と評価される AYA 独自色到達
- 1 release で完成しない可能性が高い、複数 release にわたる継続探索を許容
- ゴール条件は「Cinematic AYAstorm でしか撮れない絵」の確立

---

## 4. BD 取り込み候補ファイル (LGPL 互換確認済み、2026-05-17 BD repo 静的 survey 結果)

### 4.1 取り込み実務 (全 phase 共通)

BD は全ファイル LL viewerlgpl 標準 header (LGPL 2.1 only)、AYAstorm と完全同一。Firestorm 自身が過去複数回 BD から借りた前例あり (`0bcf5a46a4` avatar 最適化、`591957ce34` Tofu Buzzard SSR + NiranV 無効化、`12270490d3` outer ring menu、DoF free aim、mouselook attachment fix)。NiranV Dean は既に `doc/contributions.txt:1175` と `floater_about.xml:168` にクレジット済み。

- ファイル header を byte-for-byte 維持 (`$LicenseInfo:firstyear=2007&license=viewerlgpl$` 等)
- header 直下に provenance コメント追加: `// AYAstorm: imported from BlackDragon Viewer (NiranV Dean), <commit-sha>, <date>`
- `screenSpaceReflUtil.glsl` 内の `// Based on https://imanolfotia.com/blog/1` 保持
- Commit message は Firestorm idiom (例: `Import velocity buffer suite from Black Dragon Viewer (NiranV Dean)`)
- NiranV への通知は法的不要、慣習として GitHub issue 一本入れる (SL viewer 界隈の personal goodwill)

### 4.2 取り込みリスト (phase 別)

| Phase | ファイル / 対象 | 出典 | 備考 |
|---|---|---|---|
| P1 | **(取り込みなし)** | — | View Mode UI と pipeline 構築 path の C++ 改修のみ |
| P2 | velocity buffer 一式: `avatarVelocity{F,V}.glsl`, `skinnedVelocity{V,AlphaV}.glsl`, `velocity{F,V,Alpha{F,V},FuncV}.glsl` (9 ファイル) | BlackDragon | per-pixel velocity vector を gbuffer に書き込む。SMAA T2x の入力として必須 |
| P2 | `SMAAResolveF.glsl`, `SMAAResolveV.glsl` (2 ファイル) | **AYAstorm 自作** | T2x resolve pass。BD repo 内に未存在 (Tier-3 trace 確認、§7-7)、SMAA reference impl (iryoku/smaa, Crytek/Jimenez SIGGRAPH 2011) から起こす |
| P2 | `SMAA.glsl` 改修 (`SMAA_DECODE_VELOCITY` macro + `SMAA_REPROJECTION` permutation) | BD diff + AYAstorm 拡張 | BD 唯一の真 diff (1 行) + Cinematic 用 T2x permutation 追加 |
| P3 | `class3/deferred/volumetricLightF.glsl` + `class1/deferred/volumetricLightF.glsl` + 5 cvar | BlackDragon | 体積照明 |
| P4 | `class1/deferred/motionBlur{F,V}.glsl` | BlackDragon (Geenz 改良) | motion blur |
| P4 | `dofCombineF.glsl`, `postDeferredHQDoFF.glsl`, `postDeferredNoDoFF.glsl` | BlackDragon | BD 独自 HQ DoF chain (High-Res snapshot 用) |
| P4 | `RenderDepthOfFieldChroma`, `RenderChromaStrength` cvar + 関連 shader 改修 | BlackDragon | DoF-only chromatic aberration |
| P5 | Cinematic 用 cvar 群 + Preferences 整備 | AYAstorm 独自 | **BD UI は取り込まない**。Cinematic 専用 cvar のみ Preferences AYAstorm セクションに追加 |

### 4.3 BD repo 静的 survey 結果 (2026-05-17)

- BD repo: `https://github.com/NiranV/Black-Dragon-Viewer`
- License: LGPL-2.1 (AYAstorm/Firestorm/LL viewerlgpl と同一)
- 最新 commit: `995a1354d8` (Version to 5.6.2, 2026-04-19)
- 注目 commit: `bb462f6422` "Fixed: SMAA TAA not actually turning on" (BD ですら最近まで TAA が実質 OFF だった、shader plumbing と pipeline 構築の細部で挙動が決まることの裏付け)
- **Firestorm/AYAstorm 側に既存** (LL upstream 経由、BD borrow 不要):
  - SMAA 全 8 ファイル
  - SSR `screenSpaceReflPost{F,V}.glsl`, `screenSpaceReflUtil.glsl` (class1 stub + class3 本体)
- **Firestorm/AYAstorm 側に不存在** (BD 真 borrow 必須):
  - velocity buffer 一式 9 ファイル
  - Volumetric Light (class1 + class3)
  - Motion Blur (class1)
  - BD 独自 DoF chain (`dofCombineF`, `postDeferredHQDoFF`, `postDeferredNoDoFF`)
- **BD repo にも存在せず AYAstorm が自作する**:
  - `SMAAResolve{V,F}.glsl` (T2x resolve pass、SMAA reference impl から起こす、§7-7 詳細)

### 4.4 SMAA/SSR を AYAstorm View に default ON しない判断 (α 検証結果)

shader plumbing は存在するが、α 検証 (2026-05-17) で AYAstorm View に SSR を ON してみても AYA さん主観で「気づく人はほとんどない差」だった。SMAA は featuretable Ultra tier 経由で既に有効化されていたため AYAstorm View で追加的に default ON する意味も薄い。よって AYAstorm View の絵を引き上げる手段としては SMAA/SSR は採用しない。Cinematic mode 内では P2 で SMAA T2x + SSR を default ON するが、これは velocity / Volumetric / Motion Blur / BD DoF と一緒に乗せた絵の中での話。

---

## 5. 既存 AYA 色資産との関係

### 5.1 AYAstorm View の色は据え置き

r14+ で積み上げた色資産は AYAstorm View モードで引き続き動作する:

- r14 sun dazzle / volumetric atmosphere
- r15 godrays
- r16 aerial perspective
- r17 Kelvin 色温度
- r18 cloud volumetric
- r19 translucency
- r20 SSS

r30 章で AYAstorm View の絵作りには手を加えない (Cinematic 開発が AYAstorm View に regression を出さないことだけ確認)。

### 5.2 Cinematic 用 AYA 色は P6+ で新規探索

Cinematic pipeline (velocity buffer + Volumetric Light + Motion Blur + BD DoF) は AYAstorm View pipeline と全く違う絵を出す。

- 同じ Kelvin 値を載せても、Volumetric Light が空間を満たしている空間では別の色になる
- 同じ LUT を載せても、SMAA T2x で sub-pixel が落ち着いた絵では別の見え方になる
- aerial perspective の遠景色も volumetric light が空間を満たしていれば違う乗り方をする

よって P6+ の AYA 色は:
- **新規 LUT 探索** (Cinematic 用 .cube を独立に curate)
- **新規 tone curve** (Cinematic 用 tone mapper パラメタを独立設計)
- **新規 Kelvin 曲線** (Cinematic で elevation→K mapping を再評価、AYAstorm View の値を流用しない)

技術的な cvar / shader 機構は借用可能だが、**値は独立**。

### 5.3 cvar 名前空間

Cinematic 用色設定は cvar prefix で分離:
- 既存: `AYAR17ColorTemperatureEnabled`, `AYAR16AerialPerspectiveStrength` 等 (AYAstorm View 用)
- Cinematic 用 (P6+ で導入): `AYACinematic...` 系で新規 namespace

これにより設定 export/import や preset 切替時に View モードと Cinematic モードの値が混ざらない。

---

## 6. 検証手順

### 6.1 P1 検証 (3 モード再起動切替統一)

- 3 モード (Firestorm View / AYAstorm View / Cinematic) を順に切替再起動し、それぞれ起動できること
- AYAstorm View で従前と同じ絵が出ていること (色作り regression なし)
- Firestorm View で従前 Firestorm 同等の low-spec 性能を維持
- Cinematic を選択しても破綻せず、AYAstorm View 相当の絵で動作

### 6.2 P2〜P4 phase 内検証

各 phase の ship 判定は AYA 実機 + 1〜2 名の dogfood ユーザー (要相談) で行う。BD 同等性の主観判定は P5 まで持ち越す。

- A/B 撮影: 同 region / 同 EEP / 同 camera 位置で AYAstorm Cinematic と BlackDragon を撮影し並列比較
- High-Res snapshot で velocity / volumetric / motion blur が破綻しないことを確認
- 3 OS 全てで artifact が出ないことを確認
- AYAstorm View / Firestorm View に regression が無いことを確認

### 6.3 P5 ブラインドレビュー (BD 同等到達ゲート)

- SL コミュニティの photo 系ユーザー複数名に協力依頼
- BD で撮った写真と AYAstorm Cinematic で撮った写真を **どちらか分からない状態** で並べる
- 「どちらが BD っぽい」「どちらが綺麗」を聞く
- BD と区別が付かない or AYAstorm の方が好評 → ship gate 通過
- BD 寄りと判定される → P1〜P5 のどこに不足があるか再 survey

### 6.4 P6+ 色到達判定

- AYA 主観判定が一次基準 (色作りは AYA vision)
- 二次基準として「BD と並べて AYAstorm Cinematic が選ばれる比率」を photo 系ユーザーで取る
- 「BD と同等以上に綺麗」かつ「BD とは違う色」の両立で章クローズ

### 6.5 性能 baseline

各 phase で FPS regression を計測。Cinematic は重さを許容する設計だが、**AYAstorm View / Firestorm View に regression を出さない** ことは必須 (Cinematic 専用 path に閉じ込める設計の検証も兼ねる)。

---

## 7. リスクと撤退条件

### 7.1 主要リスク

| リスク | 対策 |
|---|---|
| 3 モード再起動切替化が user experience を損なう (毎回再起動で煩雑) | ユーザーは「自分が普段どのモードで使うか」を起動時に確定する運用前提。低スペックユーザーは Firestorm View で固定、撮影時のみ Cinematic に切替 |
| BD から shader を取り込んでも絵が変わらない (Firestorm が複数借りていながら絵が変わらない前例あり、α 検証でも SSR 単体は無効と確認済み) | shader 単発でなく、**default 全 ON 設計 + post pipeline 連鎖 + tone 支配の最終段** を総体として組む (UI は対象外、描画エンジン部分のみ)。phase ごとに「絵が動いた」確認 |
| Mac OpenGL deprecation で Cinematic shader が動かない | Mac kill-switch を phase ごとに併設。最悪 Cinematic = Linux/Win 限定の判断もあり得る (AYA に確認) |
| P5 ブラインドで BD 同等に届かない | ship せず差分 release で P1〜P5 を再走、それでも届かない場合は章スコープを「BD と並走できる photo viewer」から取り下げ |
| 工数が想定を大幅超過 | r25-r29 (3D stream) で十分に章を完結させてから着手、r30 章自体の長さは複数 release を許容 |
| AYA 色作り工数が長期化 (P6+) | release ごとに「Cinematic 用色 incremental」で出して 1 release で完成させない。AYA View 色が壊れない限り incremental で問題なし |

### 7.2 撤退条件

以下のいずれかが発生した場合、章リーダー (AYA) に再判断を仰ぐ:

- P4 完了時点で BD 同等の見込みが立たない → P6 進まず差分 release で P1-P5 再走、それでも届かない場合 Cinematic 名乗りを取り下げ
- Mac OpenGL deprecation で取り込み shader が動かず kill-switch では塞ぎきれない → Cinematic = Linux/Win 限定の判断を AYA に仰ぐ
- 章が r35-r36 を大幅超過する見込みになる → 再判断

---

## 8. 工数感

### 8.1 phase 間隔

「ゼロから書く」よりは明らかに軽い (shader は BD から borrow)。ただし「shader 単発取り込みで形になる」軽さでもない (α 検証で実証済み):

- BD の絵作りは **shader + default 全 ON 設計 + post pipeline 連鎖 + tone 支配の最終段** の総体 (BD では更に撮影 UI もこの総体の一部だが、AYAstorm では UI を取り込み対象外とするため除外)
- shader だけでは再現できない (Firestorm が複数借りていながら絵が変わらないのが証拠、α 検証で SSR 単体無効も確認)
- 実質工数は **デザイン・調整・default cvar 設計** が中心 (UI 翻訳工数は発生しない)

各 phase の工数感:

| Phase | 工数 | 主軸 |
|---|---|---|
| P1 | 小〜中 | View Mode 再起動切替化 (pipeline 構築 path 改修) + Cinematic 枠 UI 追加。shader 取り込み無し |
| P2 | 大 (9-10 日) | velocity buffer 取り込み + gbuffer pipeline 改修 + Cinematic mode 骨格 + SMAA T2x 完成 (resolve shader 自作含む、§7-7 案 A 採用) |
| P3 | 中 | Volumetric Light 取り込み + pipeline 連鎖位置決定 |
| P4 | 中 | Motion Blur + BD DoF chain 取り込み |
| P5 | 中 | Cinematic cvar 群の Preferences 統合 + ブラインドレビュー (UI 翻訳無し) |
| P6+ | 大 (複数 release) | Cinematic 用 AYA 色の新規探索 |

### 8.2 章クローズまでの時間軸

- AYA さんの次の release は **r30 P1**、r25-r29 を待たず即着手可能 (2026-05-17 オーナー委譲済み)
- r30 (P1) 着手後、P5 BD 同等到達ゲート通過まで **半年級**
- P6+ AYA 色到達まで含めると **1 年級** の章
- 章クローズは P6+ で「Cinematic AYAstorm でしか撮れない絵」の到達時点

### 8.3 r25-r29 (3D stream 章) との関係

r25-r29 は **別オーナーに委譲済み** (AYA さん 2026-05-17 確定)。AYA さんと Claude は r20 系に手を入れず、r30 系と並走で進む。version 番号衝突 (r25-r29 が ship 中に r30 着手) の調整は AYA さん側で別途。

撮影描画系の話 (BD borrow、Cinematic 等) は本章で扱う。3D stream 系の話は別オーナー宛 (`project_ayastorm_release_chapters.md`)。

r30 着手前に前倒し可能な作業:
- BD repo 静的 survey の継続 (取り込み対象 shader 周辺の C++ pipeline 構築コードの依存マップ作成)
- NiranV への connectivity (issue 一本入れて存在を知らせる程度)

### 8.4 α 検証 (済、2026-05-17) のまとめ

- AYA さん実機で `RenderFSAAType=2, RenderFSAASamples=3, RenderScreenSpaceReflections=1` を手動投入
- SMAA Ultra は featuretable tier 経由で既に有効化されていたため差分は SSR のみ
- 結果: SSR ON でも「気づく人はほとんどない差」、shader plumbing 死蔵解消だけでは AYAstorm View は引き上がらないと確定
- 余録: `RenderScreenSpaceReflections=TRUE` を投入しても再起動で FALSE に戻る挙動を観測。featuretable tier 上書きまたは `handleReflectionProbeDetailChanged` signal 経路が原因の可能性、章本体とは別件で後追い

---

## 9. 同種報告のさばき方

r30 P1 ship 前に「AYAstorm の絵が BD のように精細でない」「preset が壊れている」系の報告が来た場合:

- **preset 厳密派 (BlackDragon 系ユーザー)** からの「壊れている」報告:
  → 「Firestorm View モードに切り替えてください」で対応完結 (`project_ayastorm_user_segments.md`)
- **BD class の精細さを求める報告**:
  → 「Cinematic モードを r30+ で実装予定です、それまで AYAstorm View / Firestorm View の選択でお願いします」で対応
- **3D stream 系 (r25-r29) の報告**:
  → 別オーナーに委譲済み、AYA さん側でさばかない。「r20 系は別オーナー担当です」と案内のみ
- r30 P1 ship 後は「View Mode 切替が 3 モードとも再起動切替になりました、Cinematic 枠を preview 追加しました」を release notes で告知
- r30 P5 ship 後は「Cinematic モードもお試しください」が選択肢に加わる

---

## 10. 関連 memory / spec

- `project_ayastorm_release_chapters.md` — 章構成全体 (r10-r30+)
- `project_ayastorm_r30_cinematic_chapter.md` — 本章の memory 要約 (本 spec の short form)
- `project_ayastorm_user_segments.md` — BD 系ユーザー層の特性
- `project_ayastorm_visual_realism_chapter.md` — r14+ AYAstorm View 章の core thesis (r30 では Cinematic で本丸達成、AYAstorm View は据え置き)
- `feedback_feature_value_in_main_usecase.md` — 「動いた ≠ 効いた」の章スコープ適用 (α 検証で SSR 単体が該当)
- `project_ayastorm_three_platforms.md` — 3 OS 同時 ship 原則
