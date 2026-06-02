# r41 UBO 全体設計 Chapter 02: 命名規則

**起案日**: 2026-06-03
**位置付け**: 設計 doc 群 (chapter 01-NN) を通じて使用する **UBO 命名規則の唯一 source**。新規 UBO 名 / C++ 識別子 / GLSL block 名 / 接頭辞 / 互換性 suffix を本 doc に集約。
**pre-requisite**: `01-overview.md` (用語定義: storage lifetime / cadence / logical binding / physical instance)

---

## §1 命名規則を最初に確定する理由

inventory §3 で確認した通り、現状の GLSL UBO blueprint は 4 系統の命名が混在:

- `Frame*` (set=0 帯、3 個)
- `MaterialUBO` / `MaterialUBO_Legacy` (set=1 帯、2 個)
- `PerProgramUBO_*` / `PerDrawUBO_*` (set=2 帯、26 個、binding 0-25) — 注: 設計 doc 群で慣性的に「25 個」と書かれた箇所あり (= inventory §3.3 header / chapter 01 §3.3 / chapter 05)、本数値 26 が正 (= 表本体 binding 0-25 で 26 unique entry を確認、2026-06-03 査読)
- `<Name>UBO_Legacy` (set=3 帯、54 個)

これらは **起源 sub-step ごとに ad-hoc に命名された結果** で、cadence と命名が **必ずしも対応していない**。設計 doc 群で「PerProgramUBO_GammaCorrect は per-program cadence」「FrameViewProj は per-frame cadence」と書くたびに自明性が無い。

**= 設計議論を進める前に命名規則を確定する** ことで、cadence と prefix の対応 / Codegen-UBO 生成規則 / Legacy 識別が一意になる。

AYA 指示 (2026-06-03):
> 「少なくとも名前は C++ は同名を使ったわけだし表作って最初に命名規則なり定義しないと作業がはかどらない」
> 「命名規則だけど、あなたに決めてもらっていいです、わたしはこだわりはない」

= **本 chapter は Claude 起案で確定**。AYA 確認後に変更があれば update。

---

## §2 命名規則 (= 確定版)

### §2.1 GLSL UBO block 名

GLSL 中で `layout(set=N, binding=M) uniform <BlockName> { ... }` の `<BlockName>` 部分の命名規則。

| cadence | prefix | 例 | 由来 |
|---|---|---|---|
| per-frame | `Frame` | `FrameViewProj` / `FrameLights` / `FrameAtmosphere` | 既存命名を踏襲 |
| per-program | `Program_` | `Program_GammaCorrect` / `Program_AlphaParams` | `PerProgramUBO_` → `Program_` に短縮 |
| per-draw | `Draw_` | `Draw_LightParams` / `Draw_MultiLight` | `PerDrawUBO_` → `Draw_` に短縮 |
| per-draw (material dirty flag、chapter 05 §6 G1 確定) | `Material` | `MaterialUBO` (既存) → `MaterialPBR` / `MaterialLegacyBlinn` 等に細分検討 | 既存命名を踏襲 (cadence と命名は独立軸、§3.2.2) |
| per-asset (GLTF) | `Asset_` | `Asset_GLTFNodes` / `Asset_GLTFMaterials` | 新規 |
| per-skin (GLTF rigged) | `Skin_` | `Skin_GLTFJoints` | 新規 |
| singleton (manager 等) | `Global_` | `Global_ReflectionProbes` | 新規 |

**規則の根拠**:
- `Frame*` / `Material*` は既存命名を温存 (= upstream 取込互換性、原則 1)
- `Per*UBO_` の `Per` + `UBO_` は冗長 (cadence prefix で文脈確定 + UBO は文脈で自明) → 短縮
- `Asset_` / `Skin_` / `Global_` は物理 instance 軸の owner 種別を兼ねた prefix (= cadence と owner 軸が一致するケースの簡易表記)

### §2.2 Legacy 識別 suffix

既存 GLSL blueprint で **Codegen-UBO 移行前** の状態を識別する suffix。

| 状態 | suffix | 用法 |
|---|---|---|
| η-6 / η-13 期に作られ host 側 redirect 未整備の blueprint | `_Legacy` | 既存命名そのまま温存 (例: `WaterFogUBO_Legacy`) |
| η-24+ で parse error 解消用に作られた blueprint | (なし) | 既存 `PerProgramUBO_*` / `PerDrawUBO_*` から `Program_*` / `Draw_*` に rename 検討 |
| Codegen-UBO で **build-time 生成** される完成形 | (なし) | cadence prefix のみ |
| 移行途中 (Legacy と新形が一時並存) | `_v2` | 例: `Program_GammaCorrect_v2` (旧 `PerProgramUBO_GammaCorrect` を残しつつ新形を試作) |

**Codegen-UBO 完成形では `_Legacy` / `_v2` は **存在しない** はず**。これらは migration 中の過渡状態のみで使う。

### §2.3 C++ 識別子

#### §2.3.1 `LLGLSLShader::UB_*` enum (= logical binding 識別子)

GLSL block 名から `UB_` prefix + ALL_CAPS_SNAKE で機械的変換:

| GLSL block 名 | C++ enum 値 |
|---|---|
| `Global_ReflectionProbes` | `UB_GLOBAL_REFLECTION_PROBES` |
| `Asset_GLTFNodes` | `UB_ASSET_GLTF_NODES` |
| `Asset_GLTFMaterials` | `UB_ASSET_GLTF_MATERIALS` |
| `Skin_GLTFJoints` | `UB_SKIN_GLTF_JOINTS` |
| `FrameViewProj` | `UB_FRAME_VIEW_PROJ` |
| `Program_GammaCorrect` | `UB_PROGRAM_GAMMA_CORRECT` |
| `Draw_LightParams` | `UB_DRAW_LIGHT_PARAMS` |
| `MaterialUBO` | `UB_MATERIAL` |

**規則**:
- cadence prefix も enum 値に含める (= prefix 衝突回避)
- `UBO` 接尾辞は enum 名から除外 (`UB_` prefix で自明)

#### §2.3.2 物理 instance owner 命名

owner class 内の UBO buffer 識別子は cadence prefix なし、用途名で表記:

| owner class | UBO member 名 | 対応 GLSL block |
|---|---|---|
| `LLReflectionMapManager` | `mUBO` (既存) | `Global_ReflectionProbes` |
| `gltf::Asset` | `mNodesUBO` / `mMaterialsUBO` (既存) | `Asset_GLTFNodes` / `Asset_GLTFMaterials` |
| `gltf::Skin` | `mUBO` (既存) | `Skin_GLTFJoints` |
| `LLGLSLShader` (Codegen-UBO で新設) | `mFrameUBO` / `mProgramUBO` / `mDrawUBO` (TBD chapter 06) | `Frame*` / `Program_*` / `Draw_*` |

**規則**:
- 既存 owner の member 名は **改名しない** (原則 1: upstream 取込)
- 新設 owner の member 名は cadence prefix を付与 (= 役割明示)

### §2.4 Codegen-UBO 生成識別子

build-time pre-process で生成される識別子の命名。

| 生成物 | 識別子規則 | 例 |
|---|---|---|
| GLSL block 内 member offset 定数 | `<BlockName>_<MemberName>_OFFSET` | `FrameViewProj_view_OFFSET` |
| C++ lookup table 構造体 | `<BlockName>Layout` | `FrameViewProjLayout` |
| C++ block size 定数 | `<BlockName>_SIZE` | `FrameViewProj_SIZE` |
| 生成ヘッダファイル | `ubo_layout_<blockname>.inl` | `ubo_layout_frameviewproj.inl` |

**規則**:
- 生成物は全て **machine-derivable from GLSL block 名** (= codegen の決定性)
- 生成ヘッダは `indra/newview/generated/ubo/` 配下 (build dir、commit しない、要 chapter 08 確定)

---

## §3 既存 84 UBO blueprint の rename 表 (= 移行 mapping)

inventory §3 の現状 84 個に対する命名規則適用後の最終名。**Codegen-UBO 完成時点での到達名**。

### §3.1 set=0 帯 (3 個 → 全て温存)

| 現名 | 新名 (= 同じ) | cadence | owner | 備考 |
|---|---|---|---|---|
| `FrameViewProj` | `FrameViewProj` | per-frame | `LLGLSLShader` の per-frame slot | 既存命名と規則が一致、変更なし |
| `FrameLights` | `FrameLights` | per-frame | 同上 | 同上 |
| `FrameAtmosphere_Lighting` | `FrameAtmosphere` | per-frame | 同上 | `_Lighting` suffix を削除 (atmospheric には lighting 以外無いため自明)。**移行注**: rename 完了は Codegen-UBO 出力 phase (chapter 09 phase roadmap 参照)、それまでは inventory §3.1 / chapter 05 §3.1 / handoff docs / reference-shader-location-map / 既存 GLSL (`#ifdef LL_VULKAN_GLSL` 内側) は旧名 `FrameAtmosphere_Lighting` を保持 (= 既知 / 過渡期許容) |

### §3.2 set=1 帯 (2 個 → 再構成検討)

| 現名 | 新名 | cadence | 備考 |
|---|---|---|---|
| `MaterialUBO` | `MaterialUBO` (暫定) | per-draw (material dirty flag、chapter 01 §5 確定事項 #12) | 既存名温存 (`Material*` 例外、§4 警告対象外)、chapter 05 で member 細分 / 分割を検討 |
| `MaterialUBO_Legacy` | `MaterialLegacyBlinn` (案) | per-draw (material dirty flag、chapter 01 §5 確定事項 #12) | `_Legacy` は移行完了時に剥がす、別 program 用と確認できれば独立名へ |

**cadence 注**: chapter 01 §5 確定事項 #12 = per-material cadence は per-draw + dirty flag に統合 (= 独立軸として持たない)、cadence 軸は 5 分類 (per-frame / per-program / per-draw / per-asset / per-skin) に縮約。本 chapter §2.1 と同じ表記を §3.2 にも適用。

### §3.3 set=2 帯 (26 個、binding 0-25 → `PerProgramUBO_` / `PerDrawUBO_` rename)

**個数注**: 設計 doc 群で慣性的に「25 個」と書かれた箇所あり (= inventory §3.3 header / chapter 01 §3.3 / chapter 05 §3.3 等)、実数は **26** (= 表本体 binding 0-25、unique entry 26、本査読 2026-06-03 確認)。inventory + chapter 01 + chapter 05 は別 task で後追い修正対象。

| 現名 | 新名 | cadence |
|---|---|---|
| `PerDrawUBO_LightParams` | `Draw_LightParams` | per-draw |
| `PerDrawUBO_MultiLight` | `Draw_MultiLight` | per-draw |
| `PerProgramUBO_GammaCorrect` | `Program_GammaCorrect` | per-program |
| `PerProgramUBO_AlphaParams` | `Program_AlphaParams` | per-program |
| `PerProgramUBO_ColorGrading` | `Program_ColorGrading` | per-program |
| `PerProgramUBO_PointLightV` | `Program_PointLightV` | per-program |
| `PerProgramUBO_ShadowAlphaMaskV` | `Program_ShadowAlphaMaskV` | per-program |
| `PerProgramUBO_PostDeferredV` | `Program_PostDeferredV` | per-program |
| `PerProgramUBO_FullbrightShinyV` | `Program_FullbrightShinyV` | per-program |
| `PerProgramUBO_FxaaF` | `Program_FxaaF` | per-program |
| `PerProgramUBO_SpotLightF` | `Program_SpotLightF` | per-program |
| `PerProgramUBO_PbrAlphaV` | `Program_PbrAlphaV` | per-program |
| `PerProgramUBO_PostDeferredNoDoFF` | `Program_PostDeferredNoDoFF` | per-program |
| `PerProgramUBO_FsObjectIdF` | `Program_FsObjectIdF` | per-program |
| `PerProgramUBO_ShadowCubeV` | `Program_ShadowCubeV` | per-program |
| `PerProgramUBO_WaterHazeV` | `Program_WaterHazeV` | per-program |
| `PerProgramUBO_VisualizeBuffersF` | `Program_VisualizeBuffersF` | per-program |
| `PerProgramUBO_GodraysF` | `Program_GodraysF` | per-program |
| `PerProgramUBO_VolumetricLightF` | `Program_VolumetricLightF` | per-program |
| `PerProgramUBO_VelocityAlphaV` | `Program_VelocityAlphaV` | per-program |
| `PerProgramUBO_PostDeferredF` | `Program_PostDeferredF` | per-program |
| `PerProgramUBO_CofF` | `Program_CofF` | per-program |
| `PerProgramUBO_BlurLightF` | `Program_BlurLightF` | per-program |
| `PerProgramUBO_WaterF` | `Program_WaterF` | per-program |
| `PerProgramUBO_PbrTerrainV` | `Program_PbrTerrainV` | per-program |
| `PerProgramUBO_PointLightF` | `Program_PointLightF` | per-program |

**rename 規則**: `PerProgramUBO_<X>` → `Program_<X>` / `PerDrawUBO_<X>` → `Draw_<X>` (機械的)

### §3.4 set=3 帯 (54 個 → 全件 `<Name>UBO_Legacy` → `Program_<Name>` 機械的 rename)

**chapter 05 §4 で E3 (rename だけ) 採用** (= 2026-06-03 AYA 確認)。set=2 (`Program_*`) との統合 / 分割の個別判定は **本 chapter では行わない**。全件 `<Name>UBO_Legacy` → `Program_<Name>` の機械的 rename のみ実施。

**完全 list 参照モデル**: 本 chapter §3.4 は rename **規則** のみを定義 (= 機械的 1:1 変換)、全 54 件の完全 mapping は inventory §3.4 (= 旧名 list 完全版) と本規則の組合せで decidable。完全 mapping 表が必要なら chapter 05 §4 (existing-inventory-link) が canonical (= rename 表の完全版を持つ)。本 chapter は規則の唯一 source、chapter 05 は適用結果の唯一 source。

rename pattern 例 (= 全 54 件は inventory §3.4 全件を同パターンで、完全表は chapter 05 §4):

| 旧名 | 新名 |
|---|---|
| `AtmoExtraUBO_Legacy` | `Program_AtmoExtra` |
| `SkyVParamUBO_Legacy` | `Program_SkyVParam` |
| `SkyFParamUBO_Legacy` | `Program_SkyFParam` |
| `CloudsVParamUBO_Legacy` | `Program_CloudsVParam` |
| `WaterFogUBO_Legacy` | `Program_WaterFog` |
| `TonemapUBO_Legacy` | `Program_Tonemap` |
| ... (残 48 件、inventory §3.4 全件を同パターンで) | ... |

統合判定の旧 3 基準 (host C++ redirect 対象 member 重複 / shader file 専属性 / 複数 program 共有) は **migration 進捗で実 upload cost を測ってから再評価** する持越事項として chapter 09 後半 / chapter 10 に移管 (= chapter 05 §4.4)。

---

## §4 命名規則違反検知

build-time check として以下を **Codegen-UBO pipeline で自動検証** (chapter 08 で確定予定):

- `Frame` prefix + non per-frame cadence の宣言 → error
- `Program_` prefix + non per-program cadence の宣言 → error
- `Draw_` prefix + non per-draw cadence の宣言 → error
- `UBO` suffix が残っている → warning (`_Legacy` **および `Material*` family 例外**)
- `Per*UBO_` prefix が残っている → warning (移行未完成)

**`Material*` 例外**: §3.2 表で `MaterialUBO` 新名は `UBO` suffix を温存 (= 原則 1 upstream 取込互換)、§4 warning ルールの対象外。`Material*` prefix family (`MaterialUBO` / `MaterialLegacyBlinn` 等) は per-draw cadence (chapter 01 §5 #12、material dirty flag) として §2.1 表で定義済。

---

## §5 命名規則 update 規律

- 本 chapter は **設計 doc 全 chapter で参照される唯一 source**。命名 update は本 chapter から行い、他 chapter は後追い反映
- 新 prefix / 新 suffix の追加は AYA 確認後 (= 設計判断ではなく言語規約のため、新設は最小限に)
- §3 の rename 表は **migration 進捗で枝項目が完了マークされる** live 表 (chapter 09 phase roadmap と同期)

---

**= 本 chapter は Codegen-UBO pipeline (chapter 04 / 08) と redirect 層 (chapter 06) の input。次 chapter (03 cadence-classification) で cadence 軸の詳細を詰める**。
