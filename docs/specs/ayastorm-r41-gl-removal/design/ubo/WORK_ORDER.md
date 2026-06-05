# r41 UBO 94 項目 作業順序 + sub-work 詳細資料 (= WORK_ORDER.md)

**位置付け**: r41 Phase 2 前提条件 = 全 94 UBO 本実装の trace 順 + sub-work 工程資料。`design/09-phase-roadmap.md` §2.1 サマリの詳細版、各 UBO file 個別資料との双方向 link で進捗蓄積。

**起案契機**: 2026-06-06 AYA literal「Phase 2 前提条件は 94 UBO 全件、条件が揃っていく過程で進む順番を Layer 体系で並び替え、各項目に sub-work を示し、設計と工程に照らして資料化、各 UBO 作業完了で UBO 資料に反映していく」record

**起案規律**:
- design-phase 規律: `indra/` 配下改変ゼロ、Read/Grep/spec doc 化のみ (= memory `feedback_design_phase_no_code_write`)
- 推論禁止、不明明示 (= memory `feedback_admit_unknown`)
- 4 原則 死守 (= 3 OS 共通 / Core 分散 / OpenGL を殺さない / r41 dual-path)
- 命名統一: A-1 / B-1〜B-31 / C-1〜C-62 literal 維持 (= memory `feedback_no_scope_shrink`)
- 各 UBO 作業完了時に該当 `<UBO名>.md` §12 (= Phase 2 sub-work 進捗) に反映

---

## §0. 用語 + 略語

| 略語 | 定義 |
|---|---|
| L0-L5 | Layer 体系 (= trace 順、L0 = 全項目前提) |
| L1a / L1b | L1 内 sub-Layer (= a: 即 verify 可、b: 一括 verify) |
| Tier α / β / γ | B 判定内の昇格難易度 (= setter 特定済 / 推定済 / cadence mismatch / debug trigger 不明) |
| sub-work | 各項目内の 7 dimension 作業 |
| dim (1)-(7) | sub-work 7 dimension (= 後述 §1.2) |
| A 確定 | READINESS §1 判定 (1)+(2)+(3)+(4)+(5) 全充足 |
| (5) cold launch validation | 実機通電 + AYA live verify による visual 整合確認 |
| A 候補 | (1)+(2)+(3)+(4) 充足、(5) 未確定 (= 実機通電待ち) |
| 即 verify | 個別 UBO 通電後即描画 verify 可能 (= L1a / L2 / L3 / L5) |
| group verify | group 全件揃って初めて整合 visual (= L4 各 group) |
| 一括 verify | 50+ file 拡大完了後の総合確認 (= L1b per-shader UBO block 拡大) |
| D1 / D2 / D3 / D4 | 調査手法 (= setter Grep / data source backtrack / cadence verify / member pack 突合) |
| 4 原則 | (1) Core 分散 (= C1-C6) / (2) 3 OS 共通 (= OS-1〜OS-10) / (3) Phase 2/3 範囲 / (4) OpenGL を殺さない (= O3-2 / r41 dual-path) |

---

## §1. Layer 体系 + 順序判定軸

### §1.1 Layer 体系

| Layer | 内容 | UBO 件数目安 | verify 単位 | 着手契機 |
|---|---|---|---|---|
| **L0** | 横断 protocol 4 件 確立 (= 別カウント、L1-L5 各項目の前提) | 4 protocol | regression 確認のみ (= 既存通電 UBO 維持) | 本資料 §2 完成後即着手 |
| **L1a** | 横断 protocol 影響大 UBO (= LLStaticHashedString 経由 + 独立) | **3** (= CAS / Clip / VisualizeBuffersF) | 個別 UBO (= 即 verify) | L0 完了後 |
| **L1b** | per-shader UBO block 拡大 (= FrameViewProj / FrameLights) | **2** (= 50+ file 一括) | 一括 verify | L0 完了後、L1a 並列可 |
| **L2** | B Tier α (= setter 特定済、minor verify) | **4** (= PbrTerrainV / AOUtil / MotionBlur / DeferredUtil) | 個別 UBO (= 即 verify) | L1 完了後 |
| **L3** | B Tier β (= setter 推定済、Grep 確定要) | **20** | 個別 UBO (= 即 verify) | L1 完了後、L2 並列可 |
| **L4** | C 16 group (= cross-UBO 同期 / pair / sequential pipeline) | **62** (= 16 group) | group verify | L1-L3 進行中も独立 group は並列可 |
| **L5** | A-1 + B Tier γ (= cadence mismatch + debug trigger 不明) | **3** (= Skin_GLTFJoints + FsObjectIdF + NormaldebugV) | 個別 UBO (= 即 verify) | A-1 = Phase 1.F+ real bone matrix 接続、B Tier γ = cadence mismatch 解消後 |

UBO 項目数合計 (= 横断 protocol 除く) = 3 + 2 + 4 + 20 + 62 + 3 = **94 件** (= INDEX §2 一致、AYA literal 94 項目維持) ✅

### §1.2 sub-work 7 dimension (= 全項目共通 template)

| dim | 名称 | 内容 |
|---|---|---|
| **(1)** | 前提条件 | 待ち項目 (= 他 UBO / 横断 protocol) を明示、解消 trigger 記述 |
| **(2)** | 不明事項 | READINESS §3/§4 + UBO file §10 から継承、未解消事項列挙 |
| **(3)** | 調査手法 | D1 setter Grep / D2 data source backtrack / D3 cadence verify / D4 member pack 突合 から該当選択 |
| **(4)** | 設計 task | 4 経路 (= register / write / flush / shader 接続) の設計内容 |
| **(5)** | 工程 task | trace 順内位置 + 並列可能性 + 推定工数 (= S/M/L) |
| **(6)** | A 確定条件 | (5) cold launch validation で確認する内容 + verify 単位 (= 即 / group / 一括) |
| **(7)** | 4 原則 gate | 3 OS 共通 / Core 分散 / OpenGL を殺さない / r41 dual-path 各原則整合 + violation 検知点 |

### §1.3 順序判定軸 (= Layer 内序列)

| 軸 | 内容 | 優先順 |
|---|---|---|
| **A1** 前提依存解消 | 待ち項目の有無 (= 横断 protocol / cross-UBO 同期) | 最優先 (= 待ち UBO は後ろ) |
| **A2** 横断 protocol 影響範囲 | 影響先 UBO 数 (= LLStaticHashedString 5 件 / per-shader 50+ file) | 影響大は早期 (= 後段の前提) |
| **A3** Vulkan validation risk | binding 衝突 / SPIR-V cross-stage 共有 / packing 整合 | risk 大は早期 + canary 厚 |
| **A4** AYA 既存機能 risk | r14-r30 機能 (= aya_sss_skin_flag / r20 SSS / r30 Cinematic 13 cvar 等) 影響 | 既存 risk 大は慎重、暫定 default 値で破綻回避 |
| **A5** verify 単位サイズ | 個別 < group < 一括 | 個別は早期 (= 進捗体感) |
| **A6** 工数 (S/M/L) | S (= 数時間) / M (= 半日) / L (= 1 日 +) | 同 Layer 内では S 優先 (= 着手体感) |

---

## §2. L0 横断 protocol 4 件 詳細

**(4) 設計 task の形式**: L0 = protocol 設計のため、L1-L5 UBO の「register / write / flush / shader 接続」4 経路に代えて **protocol-A/B/C/D 4 設計要素** を採用。L1-L5 で UBO 単位の (4) は 4 経路、L0 で protocol 単位の (4) は protocol 4 要素となる。

**起案規律**:
- 推論部分は **[要追加調査]** マーク + 不明事項で明示 (= memory `feedback_admit_unknown`)
- AYA 設計判断要部分は **[要 AYA 判断]** マーク + 明示
- 全 protocol で AYA review + 修正 cycle 後に C-3〜C-7 着手 (= L0 protocol が L1-L5 の前提)

---

### §2.1 L0-1: name-based dispatch logic 確立

**背景**: r41 Vulkan 化で UBO は set/binding ペアで host 識別。同 set/binding 値を持つ複数 UBO が存在する場合、program (= shader) ごとに正しい UBO を bind する必要 (= 既存 Skin_GLTFJoints 通電で既に問題が発生していた可能性)。

**影響箇所** (= READINESS §3 / §4 + RELATIONS.md §8 集約):
- set=1 binding=0 排他 = MaterialUBO (新規 10-member) と MaterialUBO_Legacy (既存) 2 UBO
- set=2 binding=0 共有 6 UBO = PerDrawUBO_AvatarSkin / AvatarVelocity / ClipPlane / LightParams / ObjectSkin / SkinnedVelocity
- set=3 binding 衝突 3 site:
  - binding=0: AtmoExtraUBO_Legacy + Asset_GLTFNodes
  - binding=1: SkyVParamUBO_Legacy + Asset_GLTFMaterials
  - binding=2: SkyFParamUBO_Legacy + Skin_GLTFJoints
- 合計 11 UBO に dispatch protocol 適用必要

#### (1) 前提条件
- なし (= L0 protocol、横断的に先行)
- ただし既存 Skin_GLTFJoints / PerDrawUBO_LightParams pilot 通電時に既に name-based dispatch logic が存在する可能性あり、要 verify

#### (2) 不明事項
- 既存 pilot 通電 UBO の dispatch logic 実装場所 (= LLPipeline / LLGLSLShader / 他) **[要追加調査]**
- set=3 binding 衝突 3 site が r41 設計上の意図か事故か **[要 AYA 判断]**
- runtime dispatch overhead の許容範囲 (= per-bind 検索 vs 起動時マップ事前構築) **[要 AYA 判断]**
- 既存 pilot 通電 UBO の dispatch logic が name-based か binding-based か **[要追加調査]**

#### (3) 調査手法
- **D1 setter Grep**: `set = 1, binding`, `set = 2, binding`, `set = 3, binding` 識別 logic Grep (= host C++ 側)
- **D2 既存実装読解**: `llvkloader.cpp` Skin_GLTFJoints 通電部 (= PC-N-5/11/15c 5 setter site) で name-based dispatch 既存有無確認
- **D4 layout 突合**: set/binding 値の全 UBO mapping table 作成 (= INDEX.md 集約)

#### (4) 設計 task (= protocol-A/B/C/D 形式)
- **protocol-A** host dispatch logic 仕様: 入力 = program ID + bind target slot、出力 = UBO buffer handle + offset。program 識別は LLGLSLShader の name (= shader file basename) ベース
- **protocol-B** shader-side UBO block 名称規約: GLSL `layout(set=N, binding=M) uniform <UBO_NAME>` の `<UBO_NAME>` = INDEX.md 記載 UBO 名 1:1 mapping (= 既存規約 verify 要)
- **protocol-C** set=3 binding 衝突 3 site 解消方針: 候補 = (i) 新規 binding allocation で衝突解消 / (ii) program 識別 runtime dispatch で同 binding 共存 / (iii) 設計再考 (= 衝突 UBO 統合) **[要 AYA 判断]**
- **protocol-D** 既存 pilot 通電 UBO logic 整合: Skin_GLTFJoints (= binding=2、Sky と衝突) + PerDrawUBO_LightParams (= binding=0、6 UBO 共有) で既存 dispatch がどう動作しているか verify

#### (5) 工程 task
- trace 順内位置: L0 protocol 1 件目
- 並列性: L0-2 / L0-3 / L0-4 並列可 (= 互いに独立 protocol)
- 推定工数: **M** (= 半日、既存 logic 読解 + 設計策定 + AYA review)

#### (6) A 確定条件
- 全 11 UBO (= 排他 2 + 共有 6 + 衝突 3) の dispatch protocol 仕様確定
- AYA literal 承認
- (5) cold launch validation = L0 protocol 自体は実機 verify 不要、L1 以降の UBO 通電時に effective verify (= L1 UBO 通電で dispatch logic が動作確認される)

#### (7) 4 原則 gate
- **原則 1 (Core 分散)**: dispatch logic が render thread 専有でない (= 起動時マップ事前構築なら lock-free) → ✅ 設計時 gate
- **原則 2 (3 OS 共通)**: name-based dispatch は OS 独立、Vulkan layer API のみ使用 → ✅
- **原則 3 (Phase 2/3)**: protocol = Phase 2 内 (= Template A R3 spec doc) → ✅
- **原則 4 (OpenGL を殺さない)**: GL path は uniform binding で別経路、本 protocol = Vulkan path 専用、GL path 影響なし → ✅

---

### §2.2 L0-2: LLStaticHashedString UBO redirect 経路確立

**背景**: 既存コードに散在する `LLStaticHashedString` uniform 名経由の uniform 設定 (= `LLGLSLShader::uniformN(LLStaticHashedString, ...)`) を、UBO 経由の write に redirect する経路を確立。

**影響 UBO** (= READINESS §3 / §4 集約、5+ 確認済):
- CASParamUBO_Legacy (= `pipeline.cpp:9182-9200` LLStaticHashedString uniform4uiv/uniform2f)
- ClipFParamUBO_Legacy (= `llmaniptranslate.cpp:1715-1716` LLStaticHashedString)
- LuminanceFParamUBO_Legacy (= `pipeline.cpp:8754` LLStaticHashedString)
- PerProgramUBO_VisualizeBuffersF (= `pipeline.cpp:8709/8711` LLStaticHashedString)
- ExposureFParamUBO_Legacy (= `pipeline.cpp:8815/8863-8865` 4 setter)
- 追加候補 (= 要 Grep): その他 LLStaticHashedString 経由 setter 全件

#### (1) 前提条件
- L0-1 name-based dispatch logic 確立 (= UBO 識別 = redirect 先解決の前提) → ただし dispatch logic と redirect は独立 protocol で並列可能性あり、要 AYA 判断

#### (2) 不明事項
- LLStaticHashedString → UBO member の mapping は静的 (= code-gen) か動的 (= runtime introspection) か **[要 AYA 判断]**
- shader 側 uniform 名 と UBO member 名の対応規約 (= 1:1 か命名規則変換か) **[要追加調査]**
- 既存 OpenGL path の `LLGLSLShader::uniformN(LLStaticHashedString, ...)` 実装読解 **[要追加調査]**
- LLStaticHashedString instance の全 grep 件数 (= 5+ 確認済、追加候補列挙 要 Grep) **[要追加調査]**

#### (3) 調査手法
- **D1 setter Grep**: `LLStaticHashedString` instance 全 grep + uniform 設定 site 列挙
- **D2 既存実装読解**: `LLGLSLShader::uniformN(LLStaticHashedString, ...)` 実装内容 (= 関数 body 全 path 追跡)
- **D4 mapping 突合**: 各 LLStaticHashedString 名と UBO member 名の対応表作成

#### (4) 設計 task (= protocol-A/B/C/D 形式)
- **protocol-A** intercept 経路実装方針: `LLGLSLShader::uniformN(LLStaticHashedString, ...)` 内で UBO write 経路を併走 (= dual-write、既存 OpenGL uniform 設定を温存しつつ UBO write を裏で実施)
- **protocol-B** mapping table 構築方式: 候補 = (i) code-gen で起動時に静的構築 / (ii) shader binary introspection で動的構築 / (iii) ハードコード mapping table **[要 AYA 判断]**
- **protocol-C** 既存 OpenGL uniform 設定との dual-write 整合: 原則 4 (OpenGL を殺さない) gate で GL path の uniformN 設定を維持しつつ Vulkan path 用 UBO write を追加
- **protocol-D** cadence trigger: redirect 経路の dirty trigger (= per-uniform-set ごと UBO write or per-program-bind 一括 write) **[要 AYA 判断]**

#### (5) 工程 task
- trace 順内位置: L0 protocol 2 件目
- 並列性: L0-1 / L0-3 / L0-4 並列可、ただし L0-1 dispatch logic 確定後の方が redirect 設計容易
- 推定工数: **M-L** (= 半日〜1 日、intercept logic 設計 + mapping table 方式判断 + AYA review)

#### (6) A 確定条件
- 5+ UBO (= 確認済 5 + 追加 grep 後) 全件 redirect 経路設計確定
- mapping table 構築方式確定 (= code-gen / introspection / ハードコード から AYA 判断)
- AYA literal 承認
- (5) cold launch validation = L1a で各 UBO 通電時に redirect 経路が effective か実機 verify

#### (7) 4 原則 gate
- **原則 1 (Core 分散)**: intercept logic が thread safe (= 既存 OpenGL uniform 設定も同等の thread safety) → ✅
- **原則 2 (3 OS 共通)**: LLStaticHashedString は既存 LL 内部 hash、OS 独立 → ✅
- **原則 3 (Phase 2/3)**: redirect 経路 = Phase 2 内 → ✅
- **原則 4 (OpenGL を殺さない)**: dual-write 経路で GL path 既存 uniform 設定を温存 → ✅ **重要 gate**

---

### §2.3 L0-3: per-shader UBO block 拡大方式確立

**背景**: FrameViewProj / FrameLights は多 shader file が uniform consume するため、shader file 側で UBO block 宣言を拡大する必要。Frame UBO 拡大対象 50+ file 規模、upstream merge conflict risk あり。

**影響範囲** (= READINESS §4.4 集約):
- FrameViewProj = 50+ shader file (= 主要 deferred / forward pass 全般)
- FrameLights = 現 8 件確認済 + 残 lighting shader (= 全 lighting consume = pointLightF / spotLightF / multiPointLightF / softenLightF 等)
- upstream merge conflict risk (= 上流 OpenGL shader が同 file を改修した際の conflict)

#### (1) 前提条件
- L0-1 name-based dispatch logic 確立 (= UBO block 名 = 識別子前提)
- 上流 OpenGL shader path との共存方針 (= 原則 4 gate)

#### (2) 不明事項
- shader build pipeline の include 機能の限界 (= 既存 include header の使用範囲) **[要追加調査]**
- preprocessor inject 方式の前例 (= 既存 LL shader build に同様の機能あるか) **[要追加調査]**
- upstream merge conflict 検知 + 自動回避方式 **[要 AYA 判断]**
- FrameLights 拡大対象 file 全件 (= 現 8 件確認 + 残何件か) **[要追加調査]**

#### (3) 調査手法
- **D2 build pipeline 読解**: shader build pipeline / preprocessor 構造読解 (= `llrender` 配下 shader loader 機構)
- **D1 既存 include Grep**: 既存 shader file 内 `#include` directive 全 grep (= 既存 include header pattern 把握)
- **D4 upstream pattern 突合**: 上流 OpenGL shader path との共存方針 verify

#### (4) 設計 task (= protocol-A/B/C/D 形式)
- **protocol-A** include header 設計: 例 = `frame_ubo.glsl` で FrameViewProj + FrameLights block 統一宣言、各 shader file の冒頭に `#include "frame_ubo.glsl"` 1 行追加
- **protocol-B** preprocessor inject 方式: 候補 = (i) static include (= 各 shader file に 1 行手動追加) / (ii) build 時 macro 経由自動 inject / (iii) shader loader 段階で自動挿入 **[要 AYA 判断]**
- **protocol-C** GL path 用 uniform 宣言との dual 共存: `#ifdef LL_VULKAN_GLSL` gate で UBO block 宣言、否なら uniform 個別宣言 (= 原則 4 = OpenGL 維持期間中の dual-path)、`LL_VULKAN_GLSL` は GLSL 専用 macro (= memory `project_r41_phase1b_vulkan_host_gate` 既知)
- **protocol-D** upstream merge 時の conflict 自動検出 strategy: 候補 = (i) CI で conflict marker 検出 / (ii) 手動 review / (iii) include header 化で本体 file 改変ゼロ化 **[要 AYA 判断]**

#### (5) 工程 task
- trace 順内位置: L0 protocol 3 件目
- 並列性: L0-1 / L0-2 / L0-4 並列可、ただし L0-1 dispatch logic 確定後の方が UBO 名称規約が固まり拡大容易
- 推定工数: **L** (= 1 日 +、shader build pipeline 全般読解 + 試験的 1 shader file pilot 拡大方式策定 + 50+ file 拡大方式策定)

#### (6) A 確定条件
- 50+ file 拡大方式設計確定 (= include header 設計 + inject 方式 + GL path 共存 gate)
- 試験的 1 shader file での pilot 拡大 doc 起案 (= 別途 Phase 1.G 候補、本 protocol では仕様策定のみ)
- AYA literal 承認
- (5) cold launch validation = L1b 着手時に 50+ file 拡大完了後の一括 verify

#### (7) 4 原則 gate
- **原則 1 (Core 分散)**: shader build pipeline は build phase、render thread 独立 → ✅
- **原則 2 (3 OS 共通)**: shader pipeline は OS 共通、glslang 等 cross-platform → ✅
- **原則 3 (Phase 2/3)**: include header 設計 + pilot 拡大方式策定 = Phase 2 内、本格 50+ file 拡大 = Phase 2 L1b 着手時
- **原則 4 (OpenGL を殺さない)**: `#ifdef LL_VULKAN_GLSL` gate で GL path uniform 個別宣言を維持 → ✅ **重要 gate**

---

### §2.4 L0-4: cadence 再評価 + 再分類

**背景**: 多数 UBO で cadence (= PerFrame / PerProgram / PerDraw / SINGLETON) が member 単位で矛盾する dim あり (= 例: VelocityVParamUBO_Legacy `last_object_matrix` = per-object per-frame 変化 vs PerProgram cadence)。Phase 2 着手前に cadence 再評価して member を再分類する必要。

**影響 UBO** (= READINESS §3 / §4 + RELATIONS.md §5 集約):
- velocity 系 6 UBO (= VelocityVParamUBO_Legacy / PerProgramUBO_VelocityAlphaV / PerDrawUBO_AvatarSkin / AvatarVelocity / ObjectSkin / SkinnedVelocity)
- GLTF material 系 3 UBO (= PbrOpaqueVParamUBO_Legacy / PerProgramUBO_PbrAlphaV / MaterialUBO の `texture_normal_transform` / `texture_metallic_roughness_transform`)
- per-frame 変化 member (= camPosLocal in SkyV / CloudsV、time in Stars 系、blend_factor in SunDisc / Stars 等)
- PerProgramUBO_FsObjectIdF (= cadence_tag=1 PerProgram vs r21 self rigged picker per-draw 値の **重大 mismatch**)

#### (1) 前提条件
- なし (= L0 protocol、独立)
- ただし cadence 再分類結果が L1-L5 各項目 (3) 調査手法 + (4) 設計 task に伝播するため、**L0-4 早期完了が L1-L5 全項目の前提**

#### (2) 不明事項
- cadence 矛盾の許容範囲 (= per-program stale でも実害ゼロな case あるか) **[要 AYA 判断]**
- UBO 分割 (= sliced UBO) の cost (= ring buffer 容量増加 / dispatch overhead) **[要追加調査 + AYA 判断]**
- PerDraw 移行候補 UBO の具体的 member 列挙 **[要追加調査]**
- 既存 pilot 通電 UBO (= Skin_GLTFJoints) の cadence 妥当性は維持されているか **[要 verify]**

#### (3) 調査手法
- **D3 cadence verify**: 各 UBO の member ごとに per-frame 変化頻度を code 上 trigger 解析 (= 計測ではなく setter call site の frequency 解析)
- **D4 member pack 突合**: 各 UBO の member 配置 vs cadence (= 同 cadence member 集約か混在か)

#### (4) 設計 task (= protocol-A/B/C/D 形式)
- **protocol-A** cadence 矛盾 verify protocol: 各 UBO の member ごとに setter call 頻度 (= per-frame / per-program-bind / per-draw / 起動時 1 回) を code 上 trigger 解析
- **protocol-B** 再分類 strategy: 候補 = (i) sliced UBO (= cadence 別に UBO 分割) / (ii) PerDraw 移行 (= 当該 member を PerDraw UBO へ移行) / (iii) stale 許容 (= per-program でも実害ゼロな case) **[要 AYA 判断]**
- **protocol-C** 再分類影響範囲: UBO 構造変更が他 UBO 関係 (= RELATIONS.md §4 data source 17 系列 + §5 dirty 連動 group 20+) に伝播するか verify
- **protocol-D** 再分類後の verify protocol: L1-L5 各項目 (3) 調査手法に L0-4 結果を統合 (= 各 UBO の (3) D3 cadence verify で L0-4 protocol-A を呼び出し)

#### (5) 工程 task
- trace 順内位置: L0 protocol 4 件目
- 並列性: L0-1 / L0-2 / L0-3 並列可、ただし L0-4 結果が L1-L5 各項目に伝播するため早期完了望ましい
- 推定工数: **M-L** (= 半日〜1 日、多 UBO で member 単位 verify + 再分類方針策定 + AYA review)

#### (6) A 確定条件
- cadence 矛盾候補 UBO 全件 (= velocity 6 + GLTF material 3 + per-frame 変化 member UBO + FsObjectIdF = ~15 UBO) verify 完了
- 再分類方針確定 (= sliced UBO / PerDraw 移行 / stale 許容 のいずれを各 UBO に適用するか)
- AYA literal 承認
- (5) cold launch validation = L0 protocol 自体は実機 verify 不要、L1-L5 着手時に cadence 妥当性が effective verify

#### (7) 4 原則 gate
- **原則 1 (Core 分散)**: cadence 再分類が分散実現に資する (= PerDraw 移行で per-draw 並列化容易、sliced UBO で write 並列化容易) → ✅ **重要 gate**
- **原則 2 (3 OS 共通)**: cadence 概念は OS 独立 → ✅
- **原則 3 (Phase 2/3)**: cadence 再評価 = Phase 2 内、ただし member 単位 sliced UBO 化が新規 UBO 起案を伴う場合は Phase 3 移管検討 **[要 AYA 判断]**
- **原則 4 (OpenGL を殺さない)**: UBO 構造変更は Vulkan path のみ、GL path uniform は影響なし → ✅

---

### §2.5 L0 4 protocol サマリ + AYA review check list

#### §2.5.1 4 protocol サマリ

| protocol | 工数 | 並列性 | 重要 gate | AYA 判断要 |
|---|---|---|---|---|
| **L0-1** name-based dispatch | M | 全並列可 | 原則 1 | set=3 binding 衝突解消方針 + dispatch overhead 許容 |
| **L0-2** LLStaticHashedString redirect | M-L | 全並列可、L0-1 後望ましい | 原則 4 (= dual-write 経路) | mapping table 構築方式 + cadence trigger |
| **L0-3** per-shader UBO block 拡大 | L | 全並列可、L0-1 後望ましい | 原則 4 (= `LL_VULKAN_GLSL` gate) | preprocessor inject 方式 + upstream merge conflict strategy |
| **L0-4** cadence 再評価 | M-L | 全並列可、早期完了望ましい (= L1-L5 前提) | 原則 1 (= 分散) | 再分類 strategy + sliced UBO 化の Phase 3 移管判断 |

#### §2.5.2 AYA review 必要事項 (= L0 全 protocol 横断、6 件)

1. **set=3 binding 衝突 3 site** = 意図か事故か、解消方針 (= 新規 binding / runtime dispatch / 設計再考) [L0-1 protocol-C]
2. **LLStaticHashedString mapping table 構築方式** = code-gen / introspection / ハードコード [L0-2 protocol-B]
3. **shader build pipeline preprocessor inject 方式** = static include / build macro / shader loader 自動 [L0-3 protocol-B]
4. **cadence 再分類 strategy** = sliced UBO / PerDraw 移行 / stale 許容、各 UBO 適用判断 [L0-4 protocol-B]
5. **upstream merge conflict 自動検出 strategy** [L0-3 protocol-D]
6. **sliced UBO 化が Phase 3 移管対象か Phase 2 内か** [L0-4 (7) 原則 3]

#### §2.5.3 L0 完了後の C-3 着手条件

- 上記 AYA review 6 件完了 + protocol 仕様 4 件全件確定
- C-3〜C-7 各項目 (3) 調査手法に L0-1〜L0-4 protocol 結果を統合 (= 各項目 sub-work で L0 protocol を呼び出し)

---

## §3. L1〜L5 各項目 trace 順 + sub-work

**起案規律**:
- 各 UBO sub-work = 7 dim 適用 (= §1.2 template)
- 各 UBO 起案結果は該当 `<UBO名>.md` §12 に同期反映 (= C-9 並走、§5.3 protocol)
- AYA literal 命名 (= A-1 / B-1〜B-31 / C-1〜C-62) 維持
- 推論部分 **[要追加調査]** / **[要 AYA 判断]** マーク (= memory `feedback_admit_unknown`)
- 全項目 (6) A 確定条件に **visual regression ゼロ** 含む (= §5.4 policy)

**構成** (= 合計 94 件 ✅):
- §3.1 L1a (= 横断 protocol 影響大 UBO、**3 件**、C-3)
- §3.2 L1b (= per-shader UBO block 拡大、**2 件**、C-4)
- §3.3 L2 (= B Tier α setter 特定済、**4 件**、C-4)
- §3.4 L3 (= B Tier β setter 推定済、**20 件**、C-5)
- §3.5 L4 (= C 16 group、**62 件**、C-6、大型)
- §3.6 L5 (= A-1 + B Tier γ、**3 件**、C-7)

---

### §3.1 L1a: 横断 protocol 影響大 UBO (= 3 件)

**Layer 定義**: LLStaticHashedString redirect 経路 (= L0-2) 影響大 + 他 UBO 依存なし独立 UBO
**前提条件**: L0-1 (= name-based dispatch) + L0-2 (= LLStaticHashedString redirect) 確立
**verify 単位**: 個別 UBO (= 即 verify)
**trace 順判定**: pilot 検証性 (= 最小 1 member 優先) → visual 判定容易性 → debug-only 補強

#### §3.1.1 L1a-1: ClipFParamUBO_Legacy (= clip plane vec4)

**AYA literal 命名 mapping**: READINESS B 判定 (= §4.2 cadence mismatch B、AYA 単純配列で B-23 相当)

**位置付け**: L1a 1 件目 = LLStaticHashedString redirect 経路の **pilot 検証 UBO** (= 1 member only、副作用最小、redirect pilot として最適)

**UBO 概要**:
- set=3 binding=32 (= 衝突なし)
- 1 member: `clip_plane vec4`
- setter site: `llmaniptranslate.cpp:1715-1716` LLStaticHashedString uniform4fv
- shader: `class1/interface/clipF.glsl:46` (= 既存 LL_VULKAN_GLSL block)
- owner program: `gClipProgram` (= manip translate 専用)
- size: 256 B (= std140 16 B + device pad)

##### (1) 前提条件
- L0-1 name-based dispatch logic 確立 (= UBO 識別)
- L0-2 LLStaticHashedString UBO redirect 経路確立 (= uniform4fv intercept + mapping table)

##### (2) 不明事項
- LLStaticHashedString uniform4fv の UBO redirect 経路 (= CAS と共通課題、L0-2 で解消) **[L0-2 待ち]**
- gClipProgram link 構造 (= clip 専用 program か共有 program か) **[要追加調査]**
- clip_plane 座標系 (= world / view / projection) **[要追加調査]**

##### (3) 調査手法
- **D1 setter 詳細読解**: `llmaniptranslate.cpp:1715-1716` UBO redirect 経路通電後の動作 verify
- **D2 既存実装読解**: `gClipProgram` shader link + uniform binding 構造

##### (4) 設計 task (= 4 経路)
- **register**: PerProgram cadence triple-buffer slot (= manip translate visible 時のみ active)
- **write**: L0-2 redirect 経路で `llmaniptranslate.cpp:1716` LLStaticHashedString uniform4fv call を intercept → `forwardToUboUpload` → `writeProgramUbo`
- **flush**: `gClipProgram` bind 単位 (= manip translate visible 時)
- **shader 接続**: `class1/interface/clipF.glsl:46` 既存 LL_VULKAN_GLSL block を活性化 (= 改変ゼロ、include 不要)

##### (5) 工程 task
- trace 順内位置: L1a 1 件目 (= pilot 検証用)
- 並列性: L1a-2 / L1a-3 並列可、ただし L1a-1 で redirect 経路初期 verify 完了後の方が L1a-2/3 設計確実
- 推定工数: **S** (= 数時間、1 member only + 単純 single setter)

##### (6) A 確定条件
- mUseUBO ON + shader LL_VULKAN_GLSL block 活性化 + setter 通電
- AYA live verify: **manip translate 起動時 clip plane visual が既存と同一** (= visual regression ゼロ、§5.4 policy)
- cold launch validation: Vulkan validation 0 件 (= OS-7 gate)
- LLStaticHashedString redirect 経路 pilot verify 完了 (= L1a-2/3 への前提検証)

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: `gClipProgram` = render thread 単独 pass → ✅
- **原則 2 (3 OS 共通)**: set=3 binding=32 = OS 独立 → ✅
- **原則 3 (Phase 2/3)**: Phase 2 内 → ✅
- **原則 4 (OpenGL を殺さない)**: `clipF.glsl #else` block uniform 個別宣言維持 (= L0-2 dual-write) → ✅
- **visual regression ゼロ**: clip plane visual 同一 → AYA live verify で確認

---

#### §3.1.2 L1a-2: CASParamUBO_Legacy (= AMD CAS sharpening)

**AYA literal 命名 mapping**: READINESS B 判定 (= §4.2 cadence mismatch B、AYA 単純配列で B-26 相当)

**位置付け**: L1a 2 件目 = sharpening effect が visual で容易判定可能、uvec4 packing 整合 verify を含む

**UBO 概要**:
- set=3 binding=12 (= 衝突なし)
- 5 member: `out_screen_res vec2` + pad x2 + `cas_param_0/1 uvec4` x2
- setter site: `pipeline.cpp:9182-9200` LLStaticHashedString uniform4uiv/uniform2f × 3 call
- shader: `class1/deferred/CASF.glsl:52` (= 既存 LL_VULKAN_GLSL block)
- owner program: `sharpen_shader` (= CAS post-process pass)
- size: 256 B (= std140 48 B + device pad)

##### (1) 前提条件
- L0-1 + L0-2 確立
- L1a-1 pilot verify 完了 (= LLStaticHashedString redirect 経路 effective 確認)

##### (2) 不明事項
- LLStaticHashedString uniform4uiv (= uint vector) と uniform2f の UBO write path 整合 **[L0-2 protocol-D]**
- `cas_sharpness()` data source = cvar `RenderSharpness` 候補 **[要追加調査]**
- blueprint comment member_count 不一致「6 member」記述根拠 **[要追加調査]**
- CAS pass bind 単位 (= 1 frame 1 回 bind か multi-bind か、multi-bind なら PerProgram cadence stale risk) **[要追加調査]**
- uvec4 packing 整合 (= AMD FidelityFX `varAU4` packing と std140 uvec4 align 16B 一致) **[要 verify]**

##### (3) 調査手法
- **D1 setter 詳細読解**: `pipeline.cpp:9182-9200` UBO redirect 後動作 verify
- **D2 既存実装読解**: AMD FidelityFX CAS の `CasSetup` packing 規約 + std140 uvec4 align
- **D3 cadence verify**: CAS pass bind 頻度
- **D4 packing 突合**: uvec4 / uint4 packing 整合

##### (4) 設計 task (= 4 経路)
- **register**: PerProgram cadence triple-buffer slot (= CAS pass active 時)
- **write**: L0-2 redirect 経路で `pipeline.cpp:9196-9199` 3 LLStaticHashedString call を intercept → `forwardToUboUpload` → `writeProgramUbo` (= uvec4 + vec2 mixed)
- **flush**: `sharpen_shader` bind 単位 (= CAS pass 起動時)
- **shader 接続**: `class1/deferred/CASF.glsl:52` 既存 LL_VULKAN_GLSL block 活性化 (= 改変ゼロ)

##### (5) 工程 task
- trace 順内位置: L1a 2 件目
- 並列性: L1a-1 pilot 完了後着手、L1a-3 並列可
- 推定工数: **S-M** (= 数時間〜半日、uvec4 packing verify 含む)

##### (6) A 確定条件
- mUseUBO ON + shader 活性化 + 3 setter call 通電
- AYA live verify: **CAS sharpening effect が visual で既存と同一強度・色合い** (= visual regression ゼロ、§5.4 policy)
- cold launch validation: Vulkan validation 0 件
- uvec4 packing 整合 verify 完了 (= 既存 OpenGL CAS 出力と Vulkan UBO 経由出力の bit-exact 比較が理想、AYA visual judgement で許容)
- blueprint comment member_count 不一致解消 (= 5 が正、blueprint コメント訂正)

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: CAS post-process pass = render thread 単独 → ✅
- **原則 2 (3 OS 共通)**: AMD FidelityFX CAS は OS 独立 → ✅
- **原則 3 (Phase 2/3)**: Phase 2 内 → ✅
- **原則 4 (OpenGL を殺さない)**: `CASF.glsl #else` block uniform 個別宣言維持 → ✅
- **visual regression ゼロ**: CAS sharpening visual 同一 → AYA live verify (= 副作用 risk = uvec4 packing 不整合で sharpening 強度ずれ可能性、verify 必須)

---

#### §3.1.3 L1a-3: PerProgramUBO_VisualizeBuffersF (= debug buffer visualize)

**AYA literal 命名 mapping**: READINESS B 判定 (= §4.1 setter 不明、AYA 単純配列で B-15 相当)

**位置付け**: L1a 3 件目 = debug-only feature、verify 優先度低だが LLStaticHashedString redirect 経路 + 同名 `mipLevel` collision verify として価値

**UBO 概要**:
- set=2 binding=16 (= 衝突なし、ただし PerDraw 6 UBO 共有帯)
- 4 member: `mipLevel float` + 3 tail pad float
- setter site: `pipeline.cpp:8709/8711` LLStaticHashedString uniform1f (= `gDeferredBufferVisualProgram` 専用)
- shader: `class1/deferred/postDeferredVisualizeBuffers.glsl:40` (= 既存 LL_VULKAN_GLSL block)
- owner program: `gDeferredBufferVisualProgram` (= debug visualize 専用)
- size: 256 B (= std140 16 B + device pad)

##### (1) 前提条件
- L0-1 + L0-2 確立
- L1a-1 + L1a-2 pilot verify 完了 (= redirect 経路成熟確認)

##### (2) 不明事項
- LLStaticHashedString 直接 setter pattern の UBO 化対応 (= 31 setter 経路で LLShaderMgr enum 経由でない LLStaticHashedString 直接呼出が UBO 化 redirect されるか) **[L0-2 protocol-A 確認]**
- `gDeferredBufferVisualProgram` debug-only path cold launch 確認 (= 通常 release で発火しない、Phase 2 cold launch 時に validation 確認できるか) **[要 verify]**
- 同名 `mipLevel` 多 shader (= `radianceGenF.glsl` / `screenSpaceReflUtil.glsl`) 別 UBO 設計 verify **[要追加調査]**
- 同 shader file 内同時 consume UBO 一覧 (= `postDeferredVisualizeBuffers.glsl` 内全 UBO declaration grep) **[要追加調査]**
- tail pad 12 B = 将来 member 追加意図か std140 padding のみか **[要 verify]**

##### (3) 調査手法
- **D1 setter 詳細読解**: `pipeline.cpp:8709/8711` UBO redirect 後動作 verify
- **D2 既存実装読解**: `gDeferredBufferVisualProgram` link 構造 + debug menu trigger
- **D4 packing 突合**: 同名 `mipLevel` 別 UBO collision 検証

##### (4) 設計 task (= 4 経路)
- **register**: PerProgram cadence triple-buffer slot (= debug visualize mode active 時のみ)
- **write**: L0-2 redirect 経路で `pipeline.cpp:8709/8711` 2 LLStaticHashedString call を intercept
- **flush**: `gDeferredBufferVisualProgram` bind 単位 (= debug visualize trigger 時)
- **shader 接続**: `class1/deferred/postDeferredVisualizeBuffers.glsl:40` 既存 LL_VULKAN_GLSL block 活性化

##### (5) 工程 task
- trace 順内位置: L1a 3 件目 (= debug-only で優先度低、pilot 検証完了後の補強)
- 並列性: L1a-1 / L1a-2 と並列可
- 推定工数: **S** (= 数時間、debug menu trigger 確認 + 2 setter call の redirect 検証)

##### (6) A 確定条件
- mUseUBO ON + shader 活性化 + 2 setter call 通電
- AYA live verify: **debug menu 経由で buffer visualize 起動 → visualize 出力が既存と同一** (= debug-only ゆえ trigger に menu 操作必要、verify protocol 要事前 AYA 確認、visual regression ゼロ §5.4)
- cold launch validation: Vulkan validation 0 件 (= debug path で warning なし)
- 同名 `mipLevel` 別 UBO collision 解消 verify (= radianceGen / screenSpaceReflUtil の別 UBO 設計確認)

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: `gDeferredBufferVisualProgram` = debug 単独 pass → ✅
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内 → ✅
- **原則 4 (OpenGL を殺さない)**: `postDeferredVisualizeBuffers.glsl #else` block 維持 → ✅
- **visual regression ゼロ**: debug visualize 出力同一、通常 release path に影響なし

---

#### §3.1.4 L1a サマリ + 横断観点

##### §3.1.4.1 L1a 3 UBO サマリ

| L1a-N | UBO | trace 位置 | 工数 | verify 単位 | 主要 risk |
|---|---|---|---|---|---|
| **L1a-1** | ClipFParamUBO_Legacy | pilot 検証用 | S | 個別 (= 即) | redirect 経路 pilot 検証性 |
| **L1a-2** | CASParamUBO_Legacy | sharpening visual | S-M | 個別 (= 即) | uvec4 packing 整合 |
| **L1a-3** | PerProgramUBO_VisualizeBuffersF | debug only 補強 | S | 個別 (= 即、debug 経由) | LLStaticHashedString 直接 setter pattern |

##### §3.1.4.2 L1a 3 UBO 共通の (4) 設計 task 共通項

- L0-2 LLStaticHashedString redirect 経路に全 3 UBO が依存、L0-2 完成度が L1a 全件の品質を決定
- (4) shader 接続 = 既存 LL_VULKAN_GLSL block を活性化 (= 改変ゼロ、`#ifdef LL_VULKAN_GLSL` gate 既存)
- (4) write = 既存 LLStaticHashedString uniformN call を L0-2 redirect 経路で intercept (= dual-write、原則 4 維持)

##### §3.1.4.3 L1a 完了後の unblocking 関係

- **L1a-1 pilot 検証完了** = LLStaticHashedString redirect 経路 effective 確認 → L1a-2/3 + L2 + L4 §3.10 (= Luminance / Exposure / Tonemap) 着手 unblocking
- **L1a-2 CAS visual verify 完了** = uvec4 packing 整合確認 → L4 §3.10 post-process chain redirect 経路適用 unblocking
- **L1a-3 完了** = LLStaticHashedString 直接 setter pattern 全 case 対応確認 → L0-2 protocol-A 設計確定

---

### §3.2 L1b: per-shader UBO block 拡大 UBO (= 2 件)

**Layer 定義**: per-shader UBO block 宣言拡大 (= L0-3) 影響大 + Frame cadence (= per-frame、shell + write 通電済) + 50+ shader file 拡大対象
**前提条件**: L0-1 (= dispatch logic、既 set=0 binding=0/1 固定) + L0-3 (= per-shader UBO block 拡大方式) 確立
**verify 単位**: **一括 verify** (= 50+ file 拡大完了後の総合確認、§5.4 visual regression ゼロ)
**trace 順判定**: L1b-1 FrameViewProj 先 (= matrix 系 fundamental) → L1b-2 FrameLights 後 (= lighting 系拡大)

#### §3.2.1 L1b-1: FrameViewProj (= camera/projection 9 member)

**AYA literal 命名 mapping**: READINESS §4.4 (= 通電済だが拡大未完、AYA 単純配列で B-30 相当)

**位置付け**: per-frame matrix 系 UBO、shell + write 経路通電済 (= Phase 1.A PA-8 + 1.C PC-7γ-1)、本 phase 主作業 = per-shader UBO block 50+ file 拡大

**UBO 概要**:
- set=0 binding=0 (= Frame 帯固定、衝突なし)
- 9 member: modelview / projection / modelview_projection / inv_proj / proj_mat / last_modelview matrices + env_mat (mat3) + normal_matrix (mat3) + screen_res (vec2)
- size: 512 B (= std140 496 + device pad)
- setter site: `LLRender::syncMatrices()` (= `llrender.cpp:1028/1050/1073/1088/1097/1115` 等 31 setter 経路)
- shader: 50+ file (= blueprint origin 5 + 確認済 3 = 8 file + 残 ~42 file)
- 通電状態: **shell + write 経路通電済**

##### (1) 前提条件
- L0-1 dispatch logic (= set=0 binding=0 固定、衝突なし、軽量)
- L0-3 per-shader UBO block 拡大方式確立 (= include header / inject 方式)
- L0-4 cadence 再評価 (= per-frame member 全件矛盾なし確認)

##### (2) 不明事項
- `screen_res` setter call site (= `LLShaderMgr::DEFERRED_SCREEN_RES` 経由汎用 setter 経路想定) **[要追加調査]**
- `env_mat` setter call site (= `LLShaderMgr::DEFERRED_ENV_MAT` 経由) **[要追加調査]**
- `last_modelview_matrix` setter call site (= velocity buffer 用、`pipeline.cpp` 内 motion blur 経路) **[要追加調査]**
- per-shader UBO block 拡大対象 file 全列挙 (= grep `modelview_matrix` 50+ file 中 UBO block 宣言済 8 + 未済 ~42) **[要追加調査]**
- `FRAMES_IN_FLIGHT` 定数値 + triple-buffer 経路 verify **[要 verify]**
- `flushFrameUbos` 内 GPU upload 経路 (= `vkCmdUpdateBuffer` vs mapped pointer + memory barrier) **[要 verify]**

##### (3) 調査手法
- **D1 setter Grep**: `screen_res` / `env_mat` / `last_modelview_matrix` 3 setter call site
- **D2 既存実装読解**: `LLRender::syncMatrices` 31 setter 経路 + per-shader UBO block 既存 8 file pattern
- **D4 突合**: 50+ file 拡大対象列挙 (= raw uniform vs UBO block 宣言状況集約)

##### (4) 設計 task (= 4 経路)
- **register**: per-frame cadence triple-buffer、`sFrameUboInstances` 通電済 (= Phase 1.C PC-7γ-1、`llvkloader.cpp:4089`)
- **write**: 既存 31 setter 経路 (= uniformMatrix4fv / uniformMatrix3fv / uniform2f) の `forwardToUboUpload` PER_FRAME case 経路 (= 既通電、cold launch verify のみ)
- **flush**: `flushFrameUbos()` frame start 単位 (= 既通電、`llvkloader.cpp:5117`)
- **shader 接続**: **本 phase 主作業** = 50+ shader file に `#ifdef LL_VULKAN_GLSL` block 内 UBO block 宣言追加、`#else` block で既存 raw uniform 宣言維持 (= L0-3 include header / inject 方式準拠、原則 4 維持)

##### (5) 工程 task
- trace 順内位置: L1b 1 件目 (= matrix 系 fundamental)
- 並列性: L1b-2 並列可、ただし L1b-1 の per-shader 拡大方式確立後の方が L1b-2 拡大確実
- 推定工数: **L** (= 1 日 +、50+ file 拡大 + 各 file 動作確認 + setter 3 件追加特定)

##### (6) A 確定条件
- per-shader UBO block 拡大 50+ file 全件完了
- `mUseUBO=true` cold launch + Vulkan validation 0 件
- AYA live verify: **全 deferred / forward pass の描画が既存と同一** (= visual regression ゼロ §5.4、一括 verify)
- screen_res / env_mat / last_modelview_matrix setter site 完全特定
- triple-buffer 経路 `FRAMES_IN_FLIGHT` 確認

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: per-frame UBO write = render thread 単独、frame start 1 回 → ✅
- **原則 2 (3 OS 共通)**: glslang preprocessor は cross-platform、shader file 拡大 OS 独立 → ✅
- **原則 3 (Phase 2/3)**: 50+ file 拡大 = Phase 2 内、本格化 = L1b 着手時 → ✅
- **原則 4 (OpenGL を殺さない)**: `#ifdef LL_VULKAN_GLSL` gate で `#else` raw uniform 宣言全 file 維持 → ✅ **重要 gate** (= L0-3 protocol-C 整合)
- **visual regression ゼロ**: 全 shader 描画同一 → AYA live 一括 verify (= 拡大未完中間状態は visual 不整合許容、出荷時ゼロ §5.4)

---

#### §3.2.2 L1b-2: FrameLights (= sun/moon/8 local lights 9 member)

**AYA literal 命名 mapping**: READINESS §4.4 (= 通電済だが拡大未完、AYA 単純配列で B-31 相当)

**位置付け**: per-frame lighting 系 UBO、shell + write 経路通電済、本 phase 主作業 = 全 lighting shader UBO block 拡大

**UBO 概要**:
- set=0 binding=1 (= Frame 帯固定、衝突なし)
- 9 member: `sun_up_factor` (int) + `sun_dir` (vec3) + `moon_dir` (vec3) + `waterPlane` (vec4) + 5 array stride=16 × 8 (= light_position/direction/attenuation/diffuse/deferred_attenuation)
- size: 768 B (= Frame 系最大、std140 704 + device pad)
- setter site: `pipeline.cpp::setupHWLights` 推定 (= 要 verify)
- shader: 全 lighting shader (= 確認済 3 = sumLightsV / sumLightsSpecularV / waterFogF + 残)
- 通電状態: **shell + write 経路通電済**

##### (1) 前提条件
- L0-1 dispatch logic (= set=0 binding=1 固定、衝突なし)
- L0-3 per-shader UBO block 拡大方式確立
- L0-4 cadence 再評価 (= per-frame light state、frame 開始時確定)

##### (2) 不明事項
- `light_position[8]` 等 4 array setter call site (= `pipeline.cpp::setupHWLights` 内 `uniform4fv(LIGHT_POSITION, 8, ...)` 推定) **[要追加調査]**
- `sun_dir` / `moon_dir` setter (= `LLShaderMgr::SUN_DIR` / `MOON_DIR` 経由) **[要追加調査]**
- `waterPlane` setter (= water rendering 経路) **[要追加調査]**
- `sun_up_factor` setter (= day/night transition 用 int) **[要追加調査]**
- UBO block 宣言済 shader 全列挙 (= 確認済 3 + 残 lighting/environment shader) **[要追加調査]**
- 8 array stride=16 host data layout (= std140 stride 16 適用済 verify) **[要 verify]**
- dirty 連動 detail (= FrameAtmosphere_Lighting との連動 trigger) **[要 verify]**

##### (3) 調査手法
- **D1 setter Grep**: 4 array setter + 4 single setter (= sun_dir / moon_dir / waterPlane / sun_up_factor)
- **D2 既存実装読解**: `LLPipeline::setupHWLights` + `LLEnvironment::isDaytime`
- **D3 cadence verify**: per-frame member の更新 trigger
- **D4 突合**: 8 array stride=16 host vs std140 layout

##### (4) 設計 task (= 4 経路)
- **register**: per-frame cadence triple-buffer、`sFrameUboInstances` 通電済
- **write**: 既存 setter (= `uniform4fv` array / `uniform3fv` / `uniform1i` / `uniform2fv`) の `forwardToUboUpload` PER_FRAME case 経路 (= 既通電)
- **flush**: `flushFrameUbos()` frame start 単位 (= L1b-1 共通)
- **shader 接続**: 全 lighting shader (= `class1/lighting/*` + `class1/environment/*` + `class3/deferred/softenLightF` 等) に `#ifdef LL_VULKAN_GLSL` block 内 UBO block 宣言追加

##### (5) 工程 task
- trace 順内位置: L1b 2 件目
- 並列性: L1b-1 並列可、L1b-1 で拡大方式確立後の方が L1b-2 確実
- 推定工数: **L** (= 1 日 +、lighting shader 全 file 拡大 + array setter 多数特定)

##### (6) A 確定条件
- 全 lighting shader UBO block 拡大完了
- `mUseUBO=true` cold launch + Vulkan validation 0 件
- AYA live verify: **8 local light + sun/moon の lighting が既存と同一** (= visual regression ゼロ §5.4、一括 verify)
- 4 array setter + 4 single setter 完全特定
- 8 array stride=16 整合 verify (= host C++ data layout vs std140)

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: per-frame lighting write = render thread 単独、frame start 1 回 → ✅
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内 → ✅
- **原則 4 (OpenGL を殺さない)**: `#ifdef LL_VULKAN_GLSL` gate で raw uniform array 宣言維持 → ✅
- **visual regression ゼロ**: lighting visual 同一 → 副作用 risk = 8 array stride 不整合で light 位置ずれ可能性、verify 必須

---

#### §3.2.3 L1b サマリ + 横断観点

##### §3.2.3.1 L1b 2 UBO サマリ

| L1b-N | UBO | trace 位置 | 工数 | verify 単位 | 主要 risk |
|---|---|---|---|---|---|
| **L1b-1** | FrameViewProj | matrix fundamental | L | 一括 (= 50+ file 完了後) | per-shader 拡大 + setter 3 件追加特定 |
| **L1b-2** | FrameLights | lighting wide | L | 一括 (= 全 lighting shader 完了後) | 8 array stride 整合 + array setter 多数特定 |

##### §3.2.3.2 L1b 2 UBO 共通の (4) 設計 task 共通項

- shell + write 経路通電済 (= Phase 1.A PA-8 + 1.C PC-7γ-1)、本 phase 主作業 = per-shader UBO block 拡大
- L0-3 per-shader UBO block 拡大方式 (= include header / inject 方式) に全 2 UBO が依存
- (4) shader 接続 = 50+ file の `#ifdef LL_VULKAN_GLSL` block 追加 + `#else` raw uniform 維持 (= dual-path、原則 4)
- verify 単位 = **一括** (= 50+ file 全件揃って初めて整合 visual、§5.4 出荷時 visual regression ゼロ)

##### §3.2.3.3 L1b 完了後の unblocking 関係

- L1b-1 + L1b-2 完了 = per-frame Frame UBO 系全件通電 → L4 §3.7 sky/cloud/atmospheric (= FrameAtmosphere_Lighting 連動) 着手 unblocking
- per-shader 拡大方式確立 = L4 §3.5 GLTF texture transform / §3.6 water 系等の per-shader UBO block 化に同方式流用

---

### §3.3 L2: B Tier α setter 特定済 (= 4 件)

**Layer 定義**: B 判定中 setter 特定済 + minor verify で A 昇格可能 + 独立 UBO
**前提条件**: L0-1 (= dispatch logic) + L0-4 (= cadence 妥当性 verify)、L1a-1 LLStaticHashedString redirect pilot 完了 (= L2-2/3 で活用)
**verify 単位**: 個別 UBO (= 即 verify)
**trace 順判定**: setter 完全特定確実性 + visual 影響容易性 + 不明事項少順

#### §3.3.1 L2-1: PerProgramUBO_PbrTerrainV (= terrain texture transform + region scale)

**AYA literal 命名 mapping**: READINESS §4.1 (= setter 特定済例外、AYA 単純配列で B-13 相当)

**位置付け**: L2 1 件目 = setter 完全特定済 (= `lldrawpoolterrain.cpp:558/586` 2 setter)、terrain visual で容易判定

**UBO 概要**:
- set=2 binding=24 (= 衝突なし)
- 5 member: `terrain_texture_transforms[5]` vec4 array (= 80 B) + `region_scale` float + pad x3
- size: 256 B (= std140 96 + device pad)
- setter site: `lldrawpoolterrain.cpp:558` `LLShaderMgr::TERRAIN_TEXTURE_TRANSFORMS` uniform4fv count=5 + `:586` `LLShaderMgr::REGION_SCALE` uniform1f
- shader: `class1/deferred/pbrterrainV.glsl:79` LL_VULKAN_GLSL block

##### (1) 前提条件
- L0-1 + L0-4 確立

##### (2) 不明事項
- terrain pool dirty trigger 詳細 (= region 切替時 + heightmap update 時) **[要追加調査]**
- `region_scale` heightmap 側 declared-but-unused 状況 **[要追加調査]**
- PerProgram cadence の region 切替対応 (= region 切替が program rebind を伴うか独立 dirty 必要か) **[要追加調査]**
- `TerrainVParamUBO_Legacy` (= set=3 binding=61) との関係 (= 別 UBO で同 terrain data duplicate か PBR/legacy 切替で別 program か) **[要追加調査]**
- `transform_vec4_count` 上限 (= 必ず 5 か variable か、UBO size 80 B 整合) **[要 verify]**

##### (3) 調査手法
- **D1 setter 詳細読解**: `lldrawpoolterrain.cpp:558/586` UBO redirect 後動作
- **D3 cadence verify**: region 切替 + heightmap update trigger
- **D4 突合**: TerrainVParamUBO_Legacy との data source 関係

##### (4) 設計 task (= 4 経路)
- **register**: PerProgram cadence triple-buffer (= terrain pool active 時)
- **write**: `lldrawpoolterrain.cpp:558/586` 2 setter call を `forwardToUboUpload` → `writeProgramUbo` (= 80 B memcpy + 4 B memcpy)
- **flush**: terrain pool bind 単位 (= region 切替時 dirty)
- **shader 接続**: `class1/deferred/pbrterrainV.glsl:79` 既存 LL_VULKAN_GLSL block 活性化

##### (5) 工程 task
- trace 順内位置: L2 1 件目 (= setter 完全特定済で確実)
- 並列性: L2-2 / L2-3 / L2-4 並列可
- 推定工数: **S-M** (= 半日、region 切替 trigger 確認含む)

##### (6) A 確定条件
- mUseUBO ON + shader 活性化 + 2 setter 通電
- AYA live verify: **PBR terrain region の texture transform / region_scale が既存と同一描画** (= visual regression ゼロ §5.4)
- Vulkan validation 0 件
- transform_vec4_count = 5 固定 verify (= UBO size 80 B 整合)

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: terrain pool = render thread 単独 → ✅
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内 → ✅
- **原則 4 (OpenGL を殺さない)**: `pbrterrainV.glsl #else` block uniform 個別宣言維持 → ✅
- **visual regression ゼロ**: terrain region 描画同一 → AYA live verify

---

#### §3.3.2 L2-2: AOUtilParamUBO_Legacy (= SSAO 4 cvar)

**AYA literal 命名 mapping**: READINESS §4.2 (= setter 特定済、AYA 単純配列で B-25 相当)

**位置付け**: L2 2 件目 = setter 完全特定済 (= `pipeline.cpp:10636-10643` 4 setter)、SSAO visual で判定可能 (= subtle)

**UBO 概要**:
- set=3 binding=8 (= 衝突なし)
- 4 member: `ssao_radius` / `ssao_max_radius` / `ssao_factor` / `ssao_factor_inv` (= 全 float)
- size: 256 B (= std140 16 + device pad)
- setter site: `pipeline.cpp:10636-10643` `LLShaderMgr::DEFERRED_SSAO_*` uniform1f × 4
- shader: `class1/deferred/aoUtil.glsl:40` LL_VULKAN_GLSL block (= SSAO consume program に link)

##### (1) 前提条件
- L0-1 + L0-4 確立

##### (2) 不明事項
- consumer program 列挙 (= aoUtil.glsl link 先 SSAO consume program 全件) **[要追加調査]**
- `screen_to_target_scale_factor` divisor Vulkan path 整合 (= per-frame 変動、PerProgram cadence で stale risk) **[要 verify]**

##### (3) 調査手法
- **D1 setter 詳細読解**: `pipeline.cpp:10636-10643` UBO redirect 後動作
- **D2 既存実装読解**: aoUtil.glsl link 先 program 全列挙
- **D3 cadence verify**: screen_to_target_scale_factor 変動頻度 vs PerProgram cadence

##### (4) 設計 task (= 4 経路)
- **register**: PerProgram cadence triple-buffer (= 各 SSAO consume program で個別 register、seen_program_hashes 重複排除)
- **write**: `pipeline.cpp:10636-10643` 4 uniform1f call を `forwardToUboUpload` → `writeProgramUbo`
- **flush**: SSAO consume program bind 単位
- **shader 接続**: `class1/deferred/aoUtil.glsl:40` 既存 LL_VULKAN_GLSL block 活性化

##### (5) 工程 task
- trace 順内位置: L2 2 件目
- 並列性: L2-1 / L2-3 / L2-4 並列可
- 推定工数: **S-M** (= 半日、consumer program 列挙 + cadence verify)

##### (6) A 確定条件
- mUseUBO ON + shader 活性化 + 4 setter 通電
- AYA live verify: **SSAO 効果が既存と同一強度・範囲** (= visual regression ゼロ §5.4、subtle effect ゆえ慎重判定)
- Vulkan validation 0 件
- consumer program 全件 register verify

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: SSAO pass = render thread 単独 → ✅
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内 → ✅
- **原則 4 (OpenGL を殺さない)**: `aoUtil.glsl #else` block uniform 個別宣言維持 → ✅
- **visual regression ゼロ**: SSAO 効果同一 (= subtle effect の verify 慎重、stale risk あれば fluctuation 検知)

---

#### §3.3.3 L2-3: MotionBlurFParamUBO_Legacy (= motion blur strength 1 member)

**AYA literal 命名 mapping**: READINESS §4.2 (= setter 推定済、AYA 単純配列で B-24 相当)

**位置付け**: L2 3 件目 = 1 member only + cvar 連動明確、setter 行特定要

**UBO 概要**:
- set=3 binding=27 (= 衝突なし)
- 1 member: `motion_blur_strength` int
- size: 256 B (= std140 16 + device pad)
- setter site: `pipeline.cpp:10240` 周辺 (= `LLCachedControl<S32> motion_blur_strength` 取得、setter 行直接 grep 要)
- shader: `class1/deferred/motionBlurF.glsl:66` LL_VULKAN_GLSL block

##### (1) 前提条件
- L0-1 + L0-4 確立

##### (2) 不明事項
- `uniform1i(MOTION_BLUR_STRENGTH, ...)` setter 行 (= `pipeline.cpp:10240` 周辺の正確な行) **[要 D1 Grep]**
- dirty trigger 詳細 (= `LLCachedControl` から `forwardToUboUpload` までの bridge logic) **[要追加調査]**
- set=3 帯 layout 内 binding 上限 (= V3a 設計と Legacy UBO 配置整合) **[要 verify]**

##### (3) 調査手法
- **D1 setter Grep**: `MOTION_BLUR_STRENGTH` uniform1i call の正確な行特定
- **D2 既存実装読解**: `LLCachedControl<S32>` の subscribe pattern + dirty trigger

##### (4) 設計 task (= 4 経路)
- **register**: PerProgram cadence triple-buffer (= motion blur program active 時)
- **write**: setter 1 uniform1i call を `forwardToUboUpload` → `writeProgramUbo` (= 4 B memcpy)
- **flush**: motion blur program bind 単位 (= cvar 変更時 dirty)
- **shader 接続**: `class1/deferred/motionBlurF.glsl:66` 既存 LL_VULKAN_GLSL block 活性化

##### (5) 工程 task
- trace 順内位置: L2 3 件目
- 並列性: L2-1 / L2-2 / L2-4 並列可
- 推定工数: **S** (= 数時間、1 member + setter 行特定のみ)

##### (6) A 確定条件
- mUseUBO ON + shader 活性化 + setter 通電
- AYA live verify: **motion blur 効果が既存と同一強度** (= visual regression ゼロ §5.4)
- Vulkan validation 0 件
- cvar `RenderMotionBlurStrength` 変更時 dirty trigger 反映 verify

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: motion blur pass = render thread 単独 → ✅
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内 → ✅
- **原則 4 (OpenGL を殺さない)**: `motionBlurF.glsl #else` block uniform 維持 → ✅
- **visual regression ゼロ**: motion blur 同一強度 → AYA live verify (= cvar default 32 で base 判定、変更時動作確認)

---

#### §3.3.4 L2-4: DeferredUtilParamUBO_Legacy (= projector params + waterSign)

**AYA literal 命名 mapping**: READINESS §4.2 (= setter 部分特定済、AYA 単純配列で B-29 相当)

**位置付け**: L2 4 件目 = 8 member + 複数 setter site (= projector + waterSign)、不明事項多、L2 内最後

**UBO 概要**:
- set=3 binding=6 (= 衝突なし)
- 8 member: projector params 6 (= proj_n / proj_focus / proj_p / proj_lod / proj_range / proj_ambiance) + `waterSign` + pad
- size: 256 B (= std140 48 + device pad)
- setter site 部分特定済:
  - projector: `pipeline.cpp:12180-12184` (= proj_p / proj_n / proj_range) + `:12255-12257` (= proj_focus / PROJECTOR_AMBIENT_LOD)
  - waterSign: `lldrawpoolwaterexclusion.cpp:73-74` + `lldrawpoolalpha.cpp:101/118/123`
- shader: `class1/deferred/deferredUtil.glsl:99` LL_VULKAN_GLSL block (= global scope helper、複数 deferred lighting program で link)

##### (1) 前提条件
- L0-1 + L0-4 確立
- L2-3 完了 (= setter 行特定 D1 pattern 確立)

##### (2) 不明事項
- `PROJECTOR_NEAR` の所属 UBO (= `pipeline.cpp:12180` で set 済だが本 UBO に proj_near member 不在) **[要 AYA 判断 / 設計再考]**
- `PROJECTOR_AMBIENT_LOD` enum write 先 member (= proj_ambiance / proj_lod のどちらか、推定 proj_lod) **[要 verify]**
- `waterSign` PerProgram vs PerDraw 適合 (= 1 program 内 multi-draw 時の stale data risk、PerDraw cadence 移行検討) **[要 AYA 判断]**
- deferredUtil.glsl link 先 program 全列挙 (= 複数 deferred lighting program、PerProgram register 各 program で走る確認) **[要追加調査]**

##### (3) 調査手法
- **D1 setter 詳細読解**: 11 setter site (= projector 5 + waterSign 4 + PROJECTOR_NEAR 1 + 1)
- **D2 既存実装読解**: deferredUtil.glsl link 先 program 全列挙
- **D3 cadence verify**: waterSign per-draw 変動 vs PerProgram cadence 整合
- **D4 突合**: `PerProgramUBO_SpotLightF` 重複宣言禁止整合 (= `spotLightF.glsl:67-71` literal)

##### (4) 設計 task (= 4 経路)
- **register**: PerProgram cadence triple-buffer (= 各 deferred lighting program で register、seen_program_hashes 重複排除)
- **write**: 11 setter call を `forwardToUboUpload` → `writeProgramUbo` (= projector 5 + waterSign 多 site)
- **flush**: 各 deferred lighting program bind 単位
- **shader 接続**: `class1/deferred/deferredUtil.glsl:99` 既存 LL_VULKAN_GLSL block 活性化、`PerProgramUBO_SpotLightF` 重複宣言禁止整合

##### (5) 工程 task
- trace 順内位置: L2 4 件目 (= L2 内最複雑、L2 締め)
- 並列性: L2-1 / L2-2 / L2-3 並列可
- 推定工数: **M** (= 半日 +、11 setter + waterSign cadence 判断 + 重複禁止整合 verify)

##### (6) A 確定条件
- mUseUBO ON + shader 活性化 + 11 setter 通電
- AYA live verify: **projector light + water exclusion + alpha pool の描画が既存と同一** (= visual regression ゼロ §5.4)
- Vulkan validation 0 件
- waterSign cadence 妥当性確定 (= PerProgram 維持 or PerDraw 移行 AYA 判断)
- `PROJECTOR_NEAR` 所属 UBO 明確化 (= 別 UBO 担当 verify)
- `PerProgramUBO_SpotLightF` 重複宣言禁止整合 verify

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: 各 deferred lighting program = render thread 単独 → ✅
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内、waterSign cadence 再分類 = L0-4 結果に依存 → ✅
- **原則 4 (OpenGL を殺さない)**: `deferredUtil.glsl #else` block 維持 → ✅
- **visual regression ゼロ**: projector + water exclusion + alpha 描画同一 (= waterSign stale risk あれば above/below water 切替で artifact 検知)

---

#### §3.3.5 L2 サマリ + 横断観点

##### §3.3.5.1 L2 4 UBO サマリ

| L2-N | UBO | trace 位置 | 工数 | verify 単位 | 主要 risk |
|---|---|---|---|---|---|
| **L2-1** | PerProgramUBO_PbrTerrainV | terrain visual 容易 | S-M | 個別 | region 切替 trigger |
| **L2-2** | AOUtilParamUBO_Legacy | SSAO subtle | S-M | 個別 | screen_to_target_scale_factor cadence |
| **L2-3** | MotionBlurFParamUBO_Legacy | 1 member 単純 | S | 個別 | setter 行特定 |
| **L2-4** | DeferredUtilParamUBO_Legacy | projector + waterSign 複雑 | M | 個別 | waterSign cadence + PROJECTOR_NEAR 所属 |

##### §3.3.5.2 L2 4 UBO 共通の (4) 設計 task 共通項

- setter 特定済 (= 完全 or 部分)、本 phase 主作業 = PerProgram cadence register / write 経路通電
- (4) shader 接続 = 既存 LL_VULKAN_GLSL block 活性化 (= 改変ゼロ、原則 4 維持)
- (4) write = 既存 setter (= uniform4fv / uniform1f / uniform3fv) の `forwardToUboUpload` PER_PROGRAM case 経路通電

##### §3.3.5.3 L2 完了後の unblocking 関係

- L2-1 完了 = terrain region 切替 trigger pattern 確立 → L4 §3.5 GLTF texture transform (= region 連動の cadence pattern 流用)
- L2-2 完了 = SSAO consumer program 列挙 pattern 確立 → L3 SoftenLight / BlurLightF (= 同 SSAO 系) 着手 unblocking
- L2-4 完了 = `waterSign` cadence 判断 + `PROJECTOR_NEAR` 所属確定 → L4 §3.6 water 系 + §3.2 light cvar / spot/point light 着手 unblocking

---

### §3.4 L3: B Tier β setter 推定済 (= 20 件)

**Layer 定義**: B 判定中 setter 推定済 (= 完全 grep 未取得) + Grep 確定で A 昇格可能 + 独立 UBO + PerProgram cadence (= 全件共通)
**前提条件**: L0-1 (= dispatch logic) + L0-4 (= cadence 妥当性 verify)
**verify 単位**: 個別 UBO (= 即 verify)
**trace 順判定**: setter 推定確実性 + 工数 (S/M) + AYA 既存機能 risk (= 後段配置)

**全件共通の (4) 設計 task 共通項**:
- register: PerProgram cadence triple-buffer (= 該当 program active 時)
- write: 既存 setter (uniformN call) を `forwardToUboUpload` PER_PROGRAM case 経路 (= L0-1 確立後)
- flush: 該当 program bind 単位
- shader 接続: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= 改変ゼロ、原則 4 維持)

**全件共通の (7) 4 原則 gate**:
- 原則 1 (Core 分散): 各 program = render thread 単独 → ✅
- 原則 2 (3 OS 共通): ✅
- 原則 3 (Phase 2/3): Phase 2 内 → ✅
- 原則 4 (OpenGL を殺さない): 各 shader `#else` block uniform 個別宣言維持 → ✅
- visual regression ゼロ: AYA live verify (= §5.4 policy)

#### §3.4.1 L3-1: PerProgramUBO_AlphaParams (= near_clip 1 active member)

**AYA 命名**: B-6 (= READINESS §4.1、setter 推定済)
**位置付け**: 1 active member only、setter 推定 (`LLShaderMgr::NEAR_CLIP` / `LLViewerCamera::getNear()`)

**UBO 概要**: set=2 binding=3 / 4 member (1 active near_clip + 3 pad) / 256 B / setter 不明 (= `NEAR_CLIP` enum 候補) / shader `class1/deferred/alphaV.glsl:145`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: `NEAR_CLIP` uniform 書込 site (= grep 要) [要追加調査] / `LLViewerCamera::getNear()` data source verify [要 verify]
- **(3) 調査手法**: D1 (`LLShaderMgr::NEAR_CLIP` setter grep) + D2 (`LLViewerCamera` near plane state)
- **(5) 工程**: trace L3-1、工数 **S** (= 数時間)、L3-2〜L3-9 並列可
- **(6) A 確定**: setter 通電 + Vulkan validation 0 + AYA live verify (= alpha pass visual 既存と同一、visual regression ゼロ §5.4)

---

#### §3.4.2 L3-2: PerProgramUBO_PostDeferredV (= tc_scale 1 active member)

**AYA 命名**: B-14 (= READINESS §4.1、setter 推定済)
**位置付け**: 1 active member (vec2 tc_scale)、`FXAA_TC_SCALE` 共有 or 別 enum 不明

**UBO 概要**: set=2 binding=7 / 2 member (1 active tc_scale + 1 pad) / 256 B / setter 不明 (= `FXAA_TC_SCALE` 共有候補) / shader `class1/deferred/postDeferredV.glsl:46`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: `tc_scale` 直接 setter (= FXAA_TC_SCALE 共有か別 enum か) [要追加調査] / postDeferredV/F pair 関係 [要 verify] / `_pad_pdv0` vec2 将来 member 追加意図 [要 verify]
- **(3) 調査手法**: D1 (`FXAA_TC_SCALE` setter grep + tc_scale 直接 grep)
- **(5) 工程**: trace L3-2、工数 **S**、L3-1 / L3-3〜L3-9 並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= post-deferred V pass 描画既存と同一、visual regression ゼロ §5.4)

---

#### §3.4.3 L3-3: ScreenSpaceReflPostFParamUBO_Legacy (= SSR zNear/zFar)

**AYA 命名**: B-18 (= READINESS §4.1、setter 推定済)
**位置付け**: 2 member (zNear/zFar、camera 由来)、memory `project_transparent_ssao_ssr_no_work` 注記 (= no scheduled work、既存維持)

**UBO 概要**: set=3 binding=28 / 2 member / 256 B / setter 不明 (= `LLViewerCamera::getNear()/getFar()` 由来) / shader `class3/deferred/screenSpaceReflPostF.glsl:57`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: `zNear` / `zFar` setter (= LLViewerCamera 由来明示) [要追加調査] / FrameViewProj から derive 可能性 (= 重複 owner risk) [要 verify]
- **(3) 調査手法**: D1 (`zNear` / `zFar` setter grep) + D4 (FrameViewProj との重複 owner verify)
- **(5) 工程**: trace L3-3、工数 **S**、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= SSR effect 既存と同一、glass 限定で勝つ既存挙動維持、visual regression ゼロ §5.4)

---

#### §3.4.4 L3-4: SimpleColorFParamUBO_Legacy (= waterSign 1 member)

**AYA 命名**: B-21 (= READINESS §4.1、setter 推定済)
**位置付け**: 1 member only (= waterSign +1/-1、最小 UBO)、PerProgramUBO_WaterHazeV.above_water との data 共有候補

**UBO 概要**: set=3 binding=41 / 1 member / 256 B / setter 不明 (= waterSign LLStaticHashedString) / shader `class1/objects/simpleColorF.glsl:64`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: `waterSign` setter (= camera Z vs water plane Z 判定実装) [要追加調査] / PerProgramUBO_WaterHazeV.above_water との data 共有候補 [要 verify] / 1 member の存在意義 [要 AYA 判断]
- **(3) 調査手法**: D1 (waterSign setter grep) + D4 (above_water との data 共有 verify)
- **(5) 工程**: trace L3-4、工数 **S**、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= simple color pass の水中/水上判定 visual 既存と同一、visual regression ゼロ §5.4)

---

#### §3.4.5 L3-5: SnapshotFrameFParamUBO_Legacy (= snapshot UI border)

**AYA 命名**: B-19 (= READINESS §4.1、setter 推定済)
**位置付け**: snapshot UI 専用 (= debug-like、AYA 撮影機能関連)、3 member

**UBO 概要**: set=3 binding=38 / 3 member (frame_rect/border_color/border_thickness) / 256 B / setter 不明 (= snapshot UI floater) / shader `class1/post/snapshotFrameF.glsl:33`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: snapshot UI floater (= LLSnapshotFloater / llsnapshotlivepreview.cpp) 内 setter call site [要追加調査] / snapshot UI 起動 timing [要 verify]
- **(3) 調査手法**: D1 (frame_rect / border_color / border_thickness setter grep) + D2 (snapshot UI floater 構造)
- **(5) 工程**: trace L3-5、工数 **S** (= UI 起動 timing 確認含む)、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= snapshot UI 起動時 border 描画既存と同一、visual regression ゼロ §5.4、AYA r30 撮影描画章関連)

---

#### §3.4.6 L3-6: DofCombineFParamUBO_Legacy (= DoF combine 3 float)

**AYA 命名**: B-2 (= READINESS §4.1、setter 推定済)
**位置付け**: DoF combine post-process pass、`dof_width/height` reserved list 漏れ確認要、CASParamUBO_Legacy/PerProgramUBO_CofF と data source 共有可能性

**UBO 概要**: set=3 binding=24 / 3 member (res_scale/dof_width/dof_height) / 256 B / setter 不明 (= `DOF_RES_SCALE` / `DOF_WIDTH` enum) / shader `class1/deferred/dofCombineF.glsl:98`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: `DOF_RES_SCALE` / `DOF_WIDTH` 実 setter site (= pipeline.cpp DoF combine pass 推定) [要追加調査] / `dof_height` reserved list 漏れ確認 [要 verify] / CASParamUBO_Legacy / PerProgramUBO_CofF と data source 共有可能性 [要 verify]
- **(3) 調査手法**: D1 (`DOF_RES_SCALE` / `DOF_WIDTH` setter grep) + D4 (CAS / CofF と data source 共有 verify)
- **(5) 工程**: trace L3-6、工数 **S**、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= DoF combine pass 既存と同一、visual regression ゼロ §5.4)

---

#### §3.4.7 L3-7: NormgenFParamUBO_Legacy (= bump-to-normal generation)

**AYA 命名**: B-4 (= READINESS §4.1、setter 推定済)
**位置付け**: bump generation 専用、texture upload 時 1 回 dispatch、cache 後再利用

**UBO 概要**: set=3 binding=33 / 4 member (stepX/stepY/norm_scale/bump_code) / 256 B / setter 不明 (= `LLBumpImageList::onSourceLoaded` 候補) / shader `class1/deferred/normgenF.glsl:53`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: bump-to-normal 生成 dispatcher (= `LLBumpImageList::onSourceLoaded` 候補) [要追加調査] / `stepX/stepY` setter site [要追加調査] / `bump_code` enum 値域 [要 verify]
- **(3) 調査手法**: D1 (`LLBumpImageList` 内 setter grep + bump_code / norm_scale / stepX / stepY uniform setter site)
- **(5) 工程**: trace L3-7、工数 **S** (= texture upload trigger 確認)、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= bump material 描画既存と同一、visual regression ゼロ §5.4)

---

#### §3.4.8 L3-8: AvatarClothVParamUBO_Legacy (= cloth simulation 3 vec4)

**AYA 命名**: B-1 (= READINESS §4.1、setter 推定済)
**位置付け**: avatar cloth simulation (= wind/sinwave/gravity)、cloth simulation tick cadence 不明

**UBO 概要**: set=3 binding=57 / 3 member (gWindDir/gSinWaveParams/gGravity vec4) / 256 B / setter 不明 (= `AVATAR_WIND` / `AVATAR_SINWAVE` / `AVATAR_GRAVITY` enum、推定 lldrawpoolavatar.cpp) / shader `class1/deferred/avatarV.glsl:109`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: writer call site (= `lldrawpoolavatar.cpp` cloth simulation tick update 推定) [要追加調査] / `gSinWaveParams` 4 component 意味 (= phase/amplitude/frequency packed?) [要 verify] / cloth simulation tick cadence (= per-frame か独立 tick か) [要 verify]
- **(3) 調査手法**: D1 (`AVATAR_WIND` / `AVATAR_SINWAVE` / `AVATAR_GRAVITY` setter grep) + D3 (cloth simulation tick 頻度)
- **(5) 工程**: trace L3-8、工数 **S**、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= avatar cloth animation 既存と同一、visual regression ゼロ §5.4)

---

#### §3.4.9 L3-9: TerrainVParamUBO_Legacy (= terrain texgen 2 vec4)

**AYA 命名**: B-22 (= READINESS §4.1、setter 推定済)
**位置付け**: terrain texgen object-linear plane (= 古典 OpenGL `glTexGen(GL_OBJECT_PLANE)` 移植経路)、PerProgramUBO_PbrTerrainV と data source 共有候補

**UBO 概要**: set=3 binding=61 / 2 member (object_plane_s/t vec4) / 256 B / setter 不明 (= LLDrawPoolTerrain texgen 経路) / shader `class1/deferred/terrainV.glsl:108`

- **(1) 前提条件**: L0-1 + L0-4 + L2-1 完了 (= terrain pool 経路 pattern 確立)
- **(2) 不明事項**: `object_plane_s/t` setter (= LLDrawPoolTerrain texgen 経路) [要追加調査] / `PerProgramUBO_PbrTerrainV` との data source 共有 [要 verify] / OpenGL 古典 `glTexGen` 移植経路 [要 verify]
- **(3) 調査手法**: D1 (object_plane_s/t setter grep + texgen 関連 setter) + D4 (PbrTerrainV との data 共有 verify)
- **(5) 工程**: trace L3-9 (= L2-1 完了後)、工数 **S**、L3 内独立並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= terrain texgen 描画既存と同一、visual regression ゼロ §5.4)

---

#### §3.4.10 L3-10: PerProgramUBO_FullbrightShinyV (= texture_matrix1 cubemap)

**AYA 命名**: B-10 (= READINESS §4.1、setter 推定済)
**位置付け**: shiny cubemap UV transform (= mat4 64 B)、`LLShaderMgr::TEXTURE_MATRIX1` 経由候補

**UBO 概要**: set=2 binding=8 / 1 member (texture_matrix1 mat4) / 256 B / setter 不明 (= `LLShaderMgr::TEXTURE_MATRIX1` 経由) / shader `class1/deferred/fullbrightShinyV.glsl:60`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: `texture_matrix1` uniformMatrix4fv setter site (= `LLShaderMgr::TEXTURE_MATRIX1` 経由) [要追加調査] / shiny cubemap 6 face 個別 transform か全 face 共通 [要 verify]
- **(3) 調査手法**: D1 (`TEXTURE_MATRIX1` setter grep) + D2 (cubemap orientation 計算経路)
- **(5) 工程**: trace L3-10、工数 **S**、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= fullbright shiny cubemap 描画既存と同一、visual regression ゼロ §5.4)

---

#### §3.4.11 L3-11: PerProgramUBO_FxaaF (= NVIDIA FXAA constant)

**AYA 命名**: B-11 (= READINESS §4.1、setter 推定済)
**位置付け**: NVIDIA FXAA 3.11 公式 constant (= rcp_screen_res / rcp_frame_opt / rcp_frame_opt2)、viewport resize trigger

**UBO 概要**: set=2 binding=9 / 5 member (vec2 + vec4×2 + pad×2) / 256 B / setter 不明 (= `LLPipeline::renderFXAA` 推定) / shader `class1/deferred/fxaaF.glsl:2126`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: FXAA constant setter (= `LLPipeline::renderFXAA` 内) [要追加調査] / viewport resize trigger (= window resize 連動) [要 verify]
- **(3) 調査手法**: D1 (`rcp_screen_res` / `rcp_frame_opt` / `rcp_frame_opt2` setter grep) + D2 (`LLPipeline::renderFXAA` 構造)
- **(5) 工程**: trace L3-11、工数 **S-M** (= FXAA pass 経路確認)、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= FXAA antialiasing 効果既存と同一、visual regression ゼロ §5.4)

---

#### §3.4.12 L3-12: PerProgramUBO_CofF (= DoF Circle of Confusion)

**AYA 命名**: B-8 (= READINESS §4.1、setter 推定済)
**位置付け**: DoF CoF computation pass (= depth_cutoff / norm_cutoff / focal_distance / blur_constant / tan_pixel_angle / magnification)、focus / fov 変化 dirty trigger、memory `project_transparent_dof_design_constraint` 注記 (= 透過 DoF 構造制約)

**UBO 概要**: set=2 binding=21 / 8 member (6 active + 2 pad) / 256 B / setter 不明 (= `LLPipeline::generateExposure` / `renderDoF` 推定) / shader `class1/deferred/cofF.glsl:56`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: DoF CoF parameter setter site (= `LLPipeline::generateExposure` / `renderDoF` 推定) [要追加調査] / focus / fov 変化 dirty trigger [要 verify] / 透過 DoF 構造制約 (memory) との整合 [要 AYA 判断]
- **(3) 調査手法**: D1 (depth_cutoff / focal_distance / blur_constant 等 setter grep) + D2 (`LLPipeline::renderDoF` 構造)
- **(5) 工程**: trace L3-12、工数 **S-M** (= 6 setter 特定 + focus trigger verify)、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= DoF CoF computation 既存と同一、focus pull 動作 visual 同一、visual regression ゼロ §5.4)

---

#### §3.4.13 L3-13: PerProgramUBO_BlurLightF (= SSAO blur kernel)

**AYA 命名**: B-7 (= READINESS §4.1、setter 推定済)
**位置付け**: SSAO blur kernel + delta + dist_factor + blur_size + kern_scale、AOUtilParamUBO_Legacy / SoftenLightParamUBO_Legacy と data source 共有候補

**UBO 概要**: set=2 binding=22 / 8 member (5 active + 3 pad、vec3[4] kern array) / 256 B / setter 不明 (= `gPipeline.mSSAOParams` 推定) / shader `class1/deferred/blurLightF.glsl:64`

- **(1) 前提条件**: L0-1 + L0-4 + L2-2 完了 (= AOUtil 経路 pattern 確立)
- **(2) 不明事項**: SSAO blur kernel setter site (= `LLPipeline::doSSAO` 等候補) [要追加調査] / AOUtil / SoftenLight との data source 共有可能性 [要 verify]
- **(3) 調査手法**: D1 (`blur_size` / `kern` / `dist_factor` setter grep) + D4 (AOUtil / SoftenLight との data 共有 verify)
- **(5) 工程**: trace L3-13 (= L2-2 後)、工数 **S-M**、L3 内独立並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= SSAO blur 効果既存と同一、visual regression ゼロ §5.4)

---

#### §3.4.14 L3-14: PerProgramUBO_VolumetricLightF (= godray pipeline)

**AYA 命名**: B-16 (= READINESS §4.1、setter 推定済)
**位置付け**: godray pipeline (= `pipeline.cpp doRenderGodrays`)、`seconds60` BD legacy dead uniform、godray cvar (RenderGodraysRes / Multiplier / FalloffMultiplier)

**UBO 概要**: set=2 binding=18 / 4 member (godray_res/godray_multiplier/falloff_multiplier/seconds60 dead) / 256 B / setter 不明 (= `doRenderGodrays` 推定) / shader `class3/deferred/volumetricLightF.glsl:122`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: godray cvar setter site (= `pipeline.cpp doRenderGodrays`) [要追加調査] / `seconds60` BD legacy dead 確認 (= UBO 起電時に 0 値 write OK か) [要 verify] / godray cvar 全件 (RenderGodraysRes / Multiplier / FalloffMultiplier) verify [要 verify]
- **(3) 調査手法**: D1 (`GODRAY_RES` / `GODRAY_MULTIPLIER` / `FALLOFF_MULTIPLIER` setter grep) + D2 (`doRenderGodrays` 構造)
- **(5) 工程**: trace L3-14、工数 **S-M**、L3-15 (godrays F) 並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= volumetric godray 描画既存と同一、AYA r15 連動、visual regression ゼロ §5.4)

---

#### §3.4.15 L3-15: PerProgramUBO_GodraysF (= AYAstorm r15 godray cvar 3 件)

**AYA 命名**: B-12 (= READINESS §4.1、setter 推定済)
**位置付け**: AYAstorm r15 godray 章 cvar (= `aya_r15_godrays_enabled` / `_phase_exponent` / `_strength`)、godraysV は uniform 不使用 = F 単独 attach

**UBO 概要**: set=2 binding=17 / 3 member (AYAstorm r15 cvar 3 件) / 256 B / setter 不明 (= `pipeline.cpp doRenderGodrays` 推定) / shader `class1/deferred/godraysF.glsl:124`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: AYAstorm r15 cvar setter (= `pipeline.cpp:5357` `aya_r15_in_cinematic` 周辺 + direct cvar 読出) [要追加調査] / shader `// offset 0/16/32 + 12 pad` vs codegen packed 整合 [要 verify]
- **(3) 調査手法**: D1 (`aya_r15_godrays_enabled` / `_phase_exponent` / `_strength` setter grep) + D4 (shader vs codegen offset 整合)
- **(5) 工程**: trace L3-15、工数 **S-M**、L3-14 並列可、AYA r15 既存機能維持必須 (= memory `project_ayastorm_r30_bd_improvement_phase`)
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= godray 描画 + 強度・位相既存と同一、AYA r15 章機能維持、visual regression ゼロ §5.4)

---

#### §3.4.16 L3-16: PreviewVParamUBO_Legacy (= preview render light array)

**AYA 命名**: B-5 (= READINESS §4.1、setter 推定済)
**位置付け**: preview render pipeline (= inventory item / texture preview 等)、768 B 大物 UBO、4×8 light array

**UBO 概要**: set=3 binding=40 / 7 member (texture_matrix0 mat4 + ambient/color vec4 + 4 light array×8) / 768 B (大物) / setter 不明 (= preview pipeline) / shader `class1/objects/previewV.glsl:47`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: preview render pipeline setter (= `LLImageGL` preview / `LLViewerObject` preview / texture preview UI) [要追加調査] / 8-light fixed array 上限 [要 verify] / 768 B 大物 UBO の ring buffer 配置 [要 verify]
- **(3) 調査手法**: D1 (preview program setter grep) + D2 (`LLFloater*Preview` 構造) + D4 (8 array stride 整合)
- **(5) 工程**: trace L3-16、工数 **M** (= 大物 UBO + 4 array setter 多数)、L3-17/18/19/20 並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= preview render (= inventory item / texture preview) 既存と同一、visual regression ゼロ §5.4)

---

#### §3.4.17 L3-17: SoftenLightParamUBO_Legacy (= AYAstorm translucency + SSAO)

**AYA 命名**: B-29 (= READINESS §4.3 SoftenLight 単独 B、setter 推定済)
**位置付け**: AYAstorm 独自 translucency 系 (aya_translucency_params/tint) + SSAO 系 + blur 系、memory `project_aya_visual_realism_alpha_protect` 整合 (= frag_color.a=0 必須)、AYAstorm r14+ 視覚表現章独自実装

**UBO 概要**: set=3 binding=5 / 8 member (AYAstorm translucency 2 + SSAO 4 + blur 2、mat3 含む) / 256 B / setter 不明 (= AYAstorm 独自 host setter) / shader `class3/deferred/softenLightF.glsl:57`

- **(1) 前提条件**: L0-1 + L0-4 + L2-2 完了 (= AOUtil 経路 pattern 確立)
- **(2) 不明事項**: AYAstorm 独自 host setter 経路 [要追加調査] / AOUtilParamUBO_Legacy / GaussianFParamUBO_Legacy / DeferredUtilParamUBO_Legacy と data source 共有可能性 [要 verify] / member_count 表記揺れ (= metadata=8 vs blueprint=9) [要 verify]
- **(3) 調査手法**: D1 (`aya_translucency_params` / `aya_translucency_tint` / `blur_size` / `blur_fidelity` / `ssao_irradiance_*` / `ssao_effect_mat` setter grep) + D4 (AOUtil / Gaussian / DeferredUtil との data 共有 verify)
- **(5) 工程**: trace L3-17、工数 **M** (= AYAstorm 独自 + AOUtil 関係 verify)、AYAstorm r14+ 視覚表現機能維持必須
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= AYAstorm translucency 効果 + SSAO 既存と同一、frag_color.a=0 維持確認、AYAstorm 視覚表現章機能維持、visual regression ゼロ §5.4)

---

#### §3.4.18 L3-18: RlvFParamUBO_Legacy (= RLVa Sphere effect)

**AYA 命名**: B-17 (= READINESS §4.1、setter 推定済)
**位置付け**: RLVa (RestrainedLove API) Sphere effect、memory `project_ayastorm_rlv_user_base` ユーザー層 RLV ヘビー含む前提、機能維持必須、bvec2→uvec2 promote (η-7 phase 1 範式)

**UBO 概要**: set=3 binding=56 / 9 member (vec4×3 + vec2 + uvec2 promote + int mode + pad×3) / 256 B / setter 不明 (= `RlvHandler` / `RlvActions`) / shader `class1/deferred/rlvF.glsl:62`

- **(1) 前提条件**: L0-1 + L0-4
- **(2) 不明事項**: `RlvHandler` / `RlvActions` 内 sphere effect uniform 書込 site [要追加調査] / `ESphereMode` enum 定義 [要 verify] / `rlvEffectParam3_uvec` bvec2→uvec2 promote cast 整合 [要 verify] / RLV 機能維持 (memory `project_ayastorm_rlv_user_base`) **[要 AYA 判断 = 機能維持必須]**
- **(3) 調査手法**: D1 (`rlvEffectParam1/2/4/5` / `rlvEffectMode` setter grep + `RlvHandler` / `RlvActions` 内 sphere effect 関連 grep) + D4 (uvec2 promote cast 整合)
- **(5) 工程**: trace L3-18、工数 **M** (= RLVa core 調査 + uvec2 promote verify)、AYAstorm RLV ユーザー機能維持必須
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= RLVa Sphere effect (blur/blend/color overlay) 既存と同一、RLV ヘビーユーザー機能維持、visual regression ゼロ §5.4)

---

#### §3.4.19 L3-19: ShadowUtilParamUBO_Legacy (= shadow_matrix[6] + bias/offset)

**AYA 命名**: B-20 (= READINESS §4.1、setter 推定済)
**位置付け**: 最大 size Legacy UBO (= 512 B)、shadow_matrix[6] mat4 array、cinematic_bd multi-site identical 同期、memory `project_bd_biaserror_pitfall` 注記 (= BD shadow_bias)、cadence 妥当性 question (= PerFrame 候補)

**UBO 概要**: set=3 binding=7 / 12 member (mat4[6] + vec4 + vec2×2 + float×5 + pad×3) / **512 B (= 最大 size)** / setter 不明 (= `LLPipeline::generateSunShadow` 推定) / shader `class1/deferred/shadowUtil.glsl:80` + `cinematic_bd/...:105`

- **(1) 前提条件**: L0-1 + L0-4 (= cadence 妥当性 verify: PerFrame 候補)
- **(2) 不明事項**: 9 setter site (= `LLPipeline::generateSunShadow` + shadow_bias / offset / softness cvar 等) [要追加調査] / shadow_matrix order (= sun cascade 4 + spot 2 順序) [要 verify] / cinematic_bd multi-site identical 同期 [要 verify] / cadence (PerProgram vs PerFrame) 再評価 [要 L0-4 結果反映] / memory `project_bd_biaserror_pitfall` 整合 [要 AYA 判断]
- **(3) 調査手法**: D1 (`shadow_matrix` / `shadow_bias` / `shadow_offset` / `shadow_softness` / `spot_shadow_bias/offset` setter grep + `LLPipeline::generateSunShadow` 内 setter) + D3 (cadence 再評価) + D4 (cinematic_bd 同期)
- **(5) 工程**: trace L3-19、工数 **M** (= 9 setter + shared include + cadence 再評価)、shadow render 全 program 共有ゆえ広範影響
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= sun shadow 4 cascade + spot shadow 描画既存と同一、shadow bias 既存挙動維持、BD shadow_bias pitfall 整合、visual regression ゼロ §5.4)

---

#### §3.4.20 L3-20: GlobalFParamUBO_Legacy (= mirror_flag/clipSign、shell 通電済)

**AYA 命名**: B-27 (= READINESS §4.2、setter 推定済、**shell 通電済 + write 経路通電済**)
**位置付け**: shell + write 通電済 (= Phase 1.A PA-8 + 1.C PC-7γ-1)、本 phase 主作業 = setter 完全特定、mirror pass setup 経路、ClipFParamUBO_Legacy との機能重複整理

**UBO 概要**: set=3 binding=11 / 4 member (2 active mirror_flag/clipSign + 2 pad) / 256 B / setter 不明 (= `MIRROR_FLAG` / `CLIP_SIGN` reserved enum、推定 `LLHeroProbeManager` 経路) / shader `class1/deferred/globalF.glsl:32` (+ pbropaqueF.glsl)

- **(1) 前提条件**: L0-1 + L0-4 (= 既 shell + write 経路通電済、L1a-1 pilot 経路と同様 cold launch verify)
- **(2) 不明事項**: `MIRROR_FLAG` / `CLIP_SIGN` 実 setter call site (= `LLHeroProbeManager::isMirrorPass()` 経由推定) [要追加調査] / mirror pass setup 経路 [要 verify] / `clipSign` vs ClipFParamUBO_Legacy 機能重複整理 [要 AYA 判断]
- **(3) 調査手法**: D1 (`MIRROR_FLAG` / `CLIP_SIGN` setter grep + `LLHeroProbeManager` 内 mirror state grep) + D4 (ClipFParamUBO_Legacy との機能重複 verify)
- **(5) 工程**: trace L3-20 (= L3 締め、shell 通電済で cold launch verify のみ)、工数 **S** (= 既通電、setter 行特定のみ)、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= mirror pass + reflection clip 描画既存と同一、visual regression ゼロ §5.4) + ClipFParamUBO_Legacy 機能重複整理 (= AYA 判断)

---

#### §3.4.21 L3 サマリ + 横断観点

##### §3.4.21.1 L3 20 UBO サマリ (= trace 順)

| L3-N | UBO | binding | size | active member | 工数 | 主要 risk / 特異点 |
|---|---|---|---|---|---|---|
| **L3-1** | PerProgramUBO_AlphaParams | 2/3 | 256 | 1 (near_clip) | S | NEAR_CLIP setter 特定 |
| **L3-2** | PerProgramUBO_PostDeferredV | 2/7 | 256 | 1 (tc_scale) | S | FXAA_TC_SCALE 共有判断 |
| **L3-3** | ScreenSpaceReflPostFParamUBO_Legacy | 3/28 | 256 | 2 (zNear/zFar) | S | FrameViewProj 重複 owner risk |
| **L3-4** | SimpleColorFParamUBO_Legacy | 3/41 | 256 | 1 (waterSign) | S | WaterHazeV.above_water 共有候補 |
| **L3-5** | SnapshotFrameFParamUBO_Legacy | 3/38 | 256 | 3 (frame_rect/border) | S | snapshot UI trigger 時のみ active |
| **L3-6** | DofCombineFParamUBO_Legacy | 3/24 | 256 | 3 (DoF combine) | S | dof_height reserved list 漏れ |
| **L3-7** | NormgenFParamUBO_Legacy | 3/33 | 256 | 4 (bump generation) | S | bump-to-normal dispatcher 特定 |
| **L3-8** | AvatarClothVParamUBO_Legacy | 3/57 | 256 | 3 vec4 (cloth) | S | cloth simulation tick cadence |
| **L3-9** | TerrainVParamUBO_Legacy | 3/61 | 256 | 2 vec4 (texgen) | S | PbrTerrainV との data 共有 |
| **L3-10** | PerProgramUBO_FullbrightShinyV | 2/8 | 256 | 1 (texture_matrix1) | S | TEXTURE_MATRIX1 setter |
| **L3-11** | PerProgramUBO_FxaaF | 2/9 | 256 | 3 (FXAA constant) | S-M | viewport resize trigger |
| **L3-12** | PerProgramUBO_CofF | 2/21 | 256 | 6 (DoF CoF) | S-M | 透過 DoF 構造制約 (memory) |
| **L3-13** | PerProgramUBO_BlurLightF | 2/22 | 256 | 5 (SSAO blur) | S-M | AOUtil / SoftenLight data 共有 |
| **L3-14** | PerProgramUBO_VolumetricLightF | 2/18 | 256 | 3 (godray) | S-M | seconds60 dead uniform |
| **L3-15** | PerProgramUBO_GodraysF | 2/17 | 256 | 3 (AYA r15) | S-M | AYAstorm r15 cvar 機能維持 |
| **L3-16** | PreviewVParamUBO_Legacy | 3/40 | **768** | 7 (light array) | M | 大物 UBO + 8-light fixed array |
| **L3-17** | SoftenLightParamUBO_Legacy | 3/5 | 256 | 8 (AYA translucency + SSAO) | M | AYAstorm r14+ 視覚表現章独自 |
| **L3-18** | RlvFParamUBO_Legacy | 3/56 | 256 | 6 (RLVa Sphere) | M | RLV ユーザー機能維持 + uvec2 promote |
| **L3-19** | ShadowUtilParamUBO_Legacy | 3/7 | **512** | 9 (shadow_matrix[6]) | M | 最大 size + cinematic_bd 同期 + cadence 再評価 |
| **L3-20** | GlobalFParamUBO_Legacy | 3/11 | 256 | 2 (mirror_flag/clipSign) | S | 既 shell + write 通電済、ClipF 機能重複整理 |

##### §3.4.21.2 L3 完了後の unblocking 関係

- L3-1〜L3-15 完了 = PerProgram cadence setter pattern 確立 → L4 §3.5-§3.16 各 group 内独立 UBO の通電 pattern 流用
- L3-9 (TerrainV) + L2-1 (PbrTerrainV) 完了 = terrain 系 data source 関係確定 → L4 §3.5 GLTF texture transform 整理 unblocking
- L3-13 (BlurLightF) + L3-17 (SoftenLight) + L2-2 (AOUtil) 完了 = SSAO 系 data source 共有 pattern 確立 → 同一 cvar dispatch protocol L4 への流用
- L3-19 (ShadowUtil) cadence 再評価 = L4 §3.3 shadow_target_width 3 UBO triple-write 設計 unblocking
- L3-20 (GlobalF) `clipSign` vs ClipF 機能重複整理 = L1a-1 (ClipF) 整合確認

##### §3.4.21.3 L3 全件共通注記

- **全件 PerProgram cadence**、L0-4 結果 (= cadence 再分類 strategy) に依存して PerFrame / PerDraw / sliced UBO 化 candidate あり (= 特に ShadowUtil)
- **全件 shader `#ifdef LL_VULKAN_GLSL` block 既存**、改変ゼロで活性化のみ (= L0-3 の per-shader 拡大対象外、原則 4 維持)
- **全件 256 B (大半) or 768/512 B (大物 = PreviewV / ShadowUtil)**、ring buffer 配置影響あり

---

### §3.5 L4: C 16 group (= 62 件)

[**C-6 で起案、大型・分割可能性**]

予定 16 group:
- §3.5.1 aya_sss_skin_flag 3 UBO (= 3 件)
- §3.5.2 aya_visual_realism + chroma_str + light cvar (= 7 件)
- §3.5.3 shadow_target_width 3 UBO (= 3 件)
- §3.5.4 box_center/box_size 2 UBO (= 2 件)
- §3.5.5 GLTF texture transform 3 UBO (= 3 件)
- §3.5.6 water 系 5 UBO (= 5 件)
- §3.5.7 sky/cloud/atmospheric 10 UBO (= 10 件)
- §3.5.8 velocity 5 UBO (= 5 件)
- §3.5.9 reflection probe / IBL 5 UBO (= 5 件)
- §3.5.10 post-process chain 6 UBO (= 6 件)
- §3.5.11 glow chain 4 UBO (= 4 件)
- §3.5.12 SMAA 2 UBO (= 2 件)
- §3.5.13 pathfinding 2 UBO (= 2 件)
- §3.5.14 GLTF asset 2 UBO (= 2 件)
- §3.5.15 set=2 binding=0 共有 (残) 2 UBO (= 2 件)
- §3.5.16 MultiLight 1 UBO (= 1 件)

---

### §3.6 L5: A-1 + B Tier γ (= 3 件)

**Layer 定義**: A 判定 + B 判定中 cadence mismatch / debug trigger 不明 + Phase 1.F+ 持越 or 後段配置
**前提条件**: L0-1 + L0-4 結果反映 (= 特に L5-2 cadence 再分類)
**verify 単位**: 個別 UBO (= 即 verify)
**trace 順判定**: A 判定先 + B Tier γ 順 (= 不明事項解消順)

#### §3.6.1 L5-1: Skin_GLTFJoints (= A-1、唯一の A 判定、Phase 1.F+ 持越)

**AYA 命名**: **A-1** (= 唯一の A 判定、pilot real data 通電済 2026-06-06)
**位置付け**: pilot real data 通電済 (= Phase 1.E PC-N-5/11/15c)、本 phase 主作業 = Phase 1.F+ real bone matrix payload 書込み接続 + 実 PBR shader consume verify

**UBO 概要**:
- set=3 binding=2 (= **binding 衝突候補 with SkyFParamUBO_Legacy、L0-1 protocol-C 解消対象**)
- 1 member: `gltf_joints[1024]` vec4 array
- size: **16384 B (= 全 UBO 中最大、Vulkan 1.3 min uniform block 上限)**
- setter 完全特定済: `llvkloader.cpp:5824` (register) / `:3180` (wire) / `:5899` (write) / `:5250` (flush) / `:5883` (unregister)
- shader: `class1/gltf/pbrmetallicroughnessV.glsl:284-287` (= literal extract source)
- 通電状態: **pilot real data 通電済**

##### (1) 前提条件
- L0-1 dispatch logic (= set=3 binding=2 衝突解消、SkyF と同 binding)
- L0-3 per-shader 拡大方式 (= pbrmetallicroughnessV consume 拡大は Skin 系独立経路)
- L0-4 cadence verify (= PerSkin cadence、既確立、cadence_tag=4)
- Phase 1.F+ avatar Vulkan draw 通電 phase (= 本 UBO 完成の前提)

##### (2) 不明事項
- real bone matrix payload pack 経路 (= `LL::GLTF::Skin` bone matrix → mat3x4 palette pack) **[要追加調査]**
- 実 PBR shader consume verify (= `gltf_joints[i*3 + 0..2]` access pattern 動作) **[要 verify]**
- worker thread 並列化での `writeSkinUbo` dispatch 経路 (= PC-N-15a/b infra 流用) **[要 verify]**

##### (3) 調査手法
- D1 (`LL::GLTF::Skin` bone matrix 取得経路)
- D2 (`writeSkinUbo` 既経路 + worker thread dispatch infra)
- D4 (mat3x4 palette pack 整合)

##### (4) 設計 task (= 4 経路)
- **register**: PerSkin cadence、`registerSkinUbo` 既経路 (= `llvkloader.cpp:5824`、Phase 1.D PC-N-5 起案済)
- **write**: `writeSkinUbo(real_skin, ubo::block_hash::Skin_GLTFJoints, 0, palette_data, 16384)` で mat3x4 palette pack 書込み (= Phase 1.F+ 持越作業)
- **flush**: `flushSkinUbos(sCurrentSkin)` 既経路 (= `llvkloader.cpp:5250/6662`)
- **shader 接続**: `class1/gltf/pbrmetallicroughnessV.glsl:284-287` 既 consume (= literal extract source、改変なし)

##### (5) 工程 task
- trace 順内位置: L5 1 件目 (= A-1、Phase 1.F+ 持越)
- 並列性: L5-2 / L5-3 並列可
- 推定工数: **M** (= 半日 +、real bone matrix payload pack + Phase 1.F+ 並走)

##### (6) A 確定条件 (= **唯一の A 判定の (5) cold launch validation 既達成**)
- real bone matrix payload 書込み通電 + PBR shader consume verify
- AYA live verify: **rigged avatar (= GLTF skinned mesh) 描画が既存と同一** (= visual regression ゼロ §5.4、rig 動作)
- Vulkan validation 0 件 (= 既 Phase 1.E で達成)
- Phase 1.F+ 完了 (= 本 UBO 完成 = Phase 1.F+ 完成と等価)
- 5 setter site (= `llvkloader.cpp:5824/3180/5899/5250/5883`) cold launch 動作確認 (= 既 Phase 1.E で AYA literal「通常通りに描画されてます」record 2026-06-06)

##### (7) 4 原則 gate
- 原則 1 (Core 分散): worker thread 並列化 infra で write 並列化 (= PC-N-15a/b) → ✅ **強い寄与**
- 原則 2 (3 OS 共通): Vulkan API のみ、OS 独立 → ✅
- 原則 3 (Phase 2/3): A-1 = Phase 1.F+ 持越、本 phase は Phase 2 前提条件 → ✅
- 原則 4 (OpenGL を殺さない): `LL::GLTF::Skin` 既存 OpenGL skinning 経路維持 → ✅
- visual regression ゼロ: rigged avatar 描画同一 → AYA live verify (= 既 Phase 1.E で確認、real bone matrix 接続後再 verify)

---

#### §3.6.2 L5-2: PerProgramUBO_FsObjectIdF (= cadence mismatch 重大)

**AYA 命名**: B-9 (= READINESS §4.1、cadence mismatch 重大、B Tier γ)
**位置付け**: cadence mismatch 重大 (= cadence_tag=1 PerProgram vs r21 self rigged picker per-draw write semantics)、AYA r21 機能関連 (= memory `project_ayastorm_r21_self_rigged_picker`)

**UBO 概要**:
- set=2 binding=13 (= 衝突なし)
- 1 member: `object_id_packed` vec4
- size: 256 B
- setter 不明 (= r21 per-draw write 経路、`LLDrawInfo::mFSPickerLocalID` 経由推定)
- shader: `class1/deferred/fsObjectIDF.glsl:33`

##### (1) 前提条件
- L0-1 + **L0-4 cadence 再分類 strategy 必須** (= PerProgram → PerDraw 移行候補、L0-4 protocol-B 結果反映後着手)

##### (2) 不明事項
- `object_id_packed` setter site (= r21 self rigged picker 経路、`LLDrawInfo::mFSPickerLocalID` 経由) **[要追加調査]**
- vec4 16B 内 object ID pack 内訳 (= 32-bit object ID + 12 B 何か) **[要 verify]**
- cadence mismatch 解消 strategy (= PerProgram 維持で per-draw flush / PerDraw 移行 / sliced UBO) **[要 AYA 判断 = L0-4 protocol-B]**
- r21 self rigged picker semantics 維持 (= memory `project_ayastorm_r21_self_rigged_picker` 機能維持必須) **[要 AYA 判断]**

##### (3) 調査手法
- D1 (`object_id_packed` / `FS_OBJECT_ID_PACKED` setter grep + r21 picker 経路 grep)
- D2 (`LLDrawInfo::mFSPickerLocalID` 経路)
- D3 (cadence mismatch verify = per-draw 頻度 vs PerProgram)
- D4 (vec4 pack 内訳 = shader unpack code)

##### (4) 設計 task (= 4 経路)
- **register**: cadence 再分類後決定 (= PerProgram or PerDraw、L0-4 結果)
- **write**: `LLDrawInfo::mFSPickerLocalID` → `writeProgramUbo` or `writeDrawUbo`
- **flush**: cadence 依存
- **shader 接続**: `class1/deferred/fsObjectIDF.glsl:33` 既存 LL_VULKAN_GLSL block 活性化

##### (5) 工程 task
- trace 順内位置: L5 2 件目 (= L0-4 cadence 結果伝播後着手)
- 並列性: L5-1 / L5-3 並列可、ただし L0-4 結果待ち
- 推定工数: **M** (= cadence 再分類 + r21 semantics verify)

##### (6) A 確定条件
- cadence 再分類確定 (= AYA 判断、L0-4 結果反映)
- setter 通電 + Vulkan validation 0 件
- AYA live verify: **r21 self rigged picker 機能が既存と同一 (= 自己 rigged mesh 識別動作)** (= visual regression ゼロ §5.4、AYA r21 機能維持必須)
- r21 picker semantics 維持 (= memory `project_ayastorm_r21_self_rigged_picker`)

##### (7) 4 原則 gate
- 原則 1 (Core 分散): cadence 再分類で PerDraw 移行 = 分散容易 → ✅ **原則 1 寄与**
- 原則 2 (3 OS 共通): ✅
- 原則 3 (Phase 2/3): cadence 再分類 = Phase 2 内、sliced UBO 化なら Phase 3 移管検討
- 原則 4 (OpenGL を殺さない): r21 OpenGL picker 経路維持 → ✅
- visual regression ゼロ: r21 picker 機能維持 → AYA live verify

---

#### §3.6.3 L5-3: NormaldebugVParamUBO_Legacy (= debug normal visualization)

**AYA 命名**: B-3 (= READINESS §4.1、debug 用途、検証優先度低、B Tier γ)
**位置付け**: debug 用途 (= world space normal 可視化)、debug menu trigger 不明、Phase 2 最終 batch 候補

**UBO 概要**:
- set=3 binding=37 (= 衝突なし)
- 1 member: `debug_normal_draw_length` float
- size: 256 B
- setter 不明 (= debug menu trigger、`DEBUG_NORMAL_DRAW_LENGTH` enum reserved)
- shader: `class1/interface/normaldebugV.glsl:56`

##### (1) 前提条件
- L0-1 + L0-4

##### (2) 不明事項
- debug menu trigger (= `RenderDebugNormalScale` cvar / Debug menu の Normals visualization) **[要追加調査]**
- `DEBUG_NORMAL_DRAW_LENGTH` setter 行 (= grep で `uniform1f` 直接 call site 未取得) **[要 D1 Grep]**
- normal debug 通常 release 起動有無 (= debug-only path) **[要 verify]**

##### (3) 調査手法
- D1 (`DEBUG_NORMAL_DRAW_LENGTH` setter grep + `RenderDebugNormalScale` cvar grep)
- D2 (debug menu XUI + cvar binding)

##### (4) 設計 task (= 4 経路)
- **register**: PerProgram cadence triple-buffer (= normaldebug program active 時)
- **write**: setter 1 uniform1f call を `forwardToUboUpload` → `writeProgramUbo` (= 4 B memcpy)
- **flush**: normaldebug program bind 単位 (= debug menu 起動時)
- **shader 接続**: `class1/interface/normaldebugV.glsl:56` 既存 LL_VULKAN_GLSL block 活性化

##### (5) 工程 task
- trace 順内位置: L5 3 件目 (= debug-only、検証優先度低、L5 + Phase 2 前提条件全 94 UBO 締め)
- 並列性: L5-1 / L5-2 並列可
- 推定工数: **S** (= 1 member + setter 行特定 + debug menu trigger 確認)

##### (6) A 確定条件
- setter 通電 + Vulkan validation 0 件
- AYA live verify: **debug menu 経由 normal 可視化起動 → normal 描画が既存と同一** (= debug-only ゆえ通常 release path 影響なし、visual regression ゼロ §5.4、verify protocol 要事前 AYA 確認)

##### (7) 4 原則 gate
- 原則 1 (Core 分散): debug program = render thread 単独 → ✅
- 原則 2 (3 OS 共通): ✅
- 原則 3 (Phase 2/3): Phase 2 内 → ✅
- 原則 4 (OpenGL を殺さない): `normaldebugV.glsl #else` block 維持 → ✅
- visual regression ゼロ: debug normal 可視化既存と同一 → AYA live verify

---

#### §3.6.4 L5 サマリ + 横断観点

##### §3.6.4.1 L5 3 UBO サマリ

| L5-N | UBO | trace 位置 | 工数 | verify 単位 | 主要 risk / 特異点 |
|---|---|---|---|---|---|
| **L5-1** | **A-1 Skin_GLTFJoints** | Phase 1.F+ 持越 | M | 個別 (= 即) | **唯一 A 判定 / 16384 B 最大 / pilot real data 通電済 / set=3 binding=2 衝突解消対象** |
| **L5-2** | PerProgramUBO_FsObjectIdF | L0-4 後着手 | M | 個別 (= 即) | cadence mismatch 重大 / r21 機能維持必須 |
| **L5-3** | NormaldebugVParamUBO_Legacy | L5 締め | S | 個別 (= 即、debug 経由) | debug-only、検証優先度低、Phase 2 最終 batch |

##### §3.6.4.2 L5 完了 = Phase 2 前提条件 94 UBO 全件 A 化達成

- L5-1 完了 = Phase 1.F+ 完了 = avatar Vulkan draw 通電完成
- L5-2 完了 = cadence 再分類 + r21 機能維持
- L5-3 完了 = **Phase 2 前提条件 94 UBO 全件 A 化達成** = Phase 2 設計 phase 本格着手 unblocking

---

## §4. 4 原則 gate 整合 + violation 検知 protocol

[**C-8 で起案 (= 全 §完成後の総仕上げ)**]

予定構成:
- §4.1 原則 1: Core プロセス分散実現 (= C1-C6 設計制約)
- §4.2 原則 2: 3 OS 共通 (= OS-1〜OS-10 gate)
- §4.3 原則 3: Phase 2/3 範囲明確 (= Template A R3-R6 所属)
- §4.4 原則 4: OpenGL を殺さない (= O3-2 採用、r41 dual-path 出荷、OpenGL 撤廃は r42 移管)
- §4.5 各項目評価 protocol = sub-work (7) で全 94 項目逐次 check
- §4.6 violation 検知時の対応 = 提案撤回 / 設計再考 / AYA literal 確認

---

## §5. AYA review + 修正 cycle + commit + 各 UBO file 反映 protocol

### §5.1 AYA review cycle

1. 各 commit 単位完了時に AYA literal 確認
2. 修正指示があれば該当 §更新 + 該当 `<UBO名>.md` §12 同期更新
3. 修正なければ次 commit 単位起案

### §5.2 commit 単位 (= 進行 plan)

| 単位 | 内容 | 着手 timing | 想定 session |
|---|---|---|---|
| **C-1** | WORK_ORDER.md skeleton + §0 用語 + §1 Layer 体系 + §4/§5/§A skeleton | 本 step | 本 session ✅ |
| **C-2** | §2 L0 横断 protocol 4 件 詳細 (= sub-work 7 dim) | C-1 完了後 | 本 / 次 session |
| **C-3** | §3.1 L1a 横断 protocol 影響大 UBO trace 順 + sub-work (= ~5 件) | C-2 完了後 | 次 session 候補 |
| **C-4** | §3.2 L1b per-shader UBO block 拡大 + §3.3 L2 B Tier α trace 順 + sub-work | C-3 並列可 | 次 session 候補 |
| **C-5** | §3.4 L3 B Tier β trace 順 + sub-work (= ~15 件) | C-3/C-4 完了後 | 次 session 候補 |
| **C-6** | §3.5 L4 C 16 group trace 順 + sub-work (= 大型、分割可能性、~62 件) | C-3-C-5 進行中も独立 group は並列可 | 次 session 候補 (= 複数 session 想定) |
| **C-7** | §3.6 L5 A-1 + B Tier γ + 残り trace 順 + sub-work (= ~6 件) | C-6 部分完了後 | 次 session 候補 |
| **C-8** | §4 4 原則 gate 整合 + 09-phase-roadmap.md §2.1 サマリ訂正 | 全 §完成後 | 最終 session |
| **C-9** | 各 `<UBO名>.md` §12 Phase 2 sub-work 進捗 新規追加 (= 94 file) | §3 起案途中で 1 UBO ずつ並走 | C-3〜C-7 と並走 |

### §5.3 各 UBO file §12 反映 protocol

- 各 §3 起案途中で該当 UBO の sub-work 7 dim を該当 `<UBO名>.md` 末尾 §12 として追記
- §12 内容 = §3 該当項目 sub-work 7 dim をそのまま転記 + 進捗 status (= 起案済 / 調査中 / 完成 / verify 待ち)
- AYA 修正指示があれば §3 + §12 両方同期更新 (= single source of truth = §3、§12 = 個別 UBO ナビゲーション向け)

### §5.4 visual regression policy (= AYA literal 2026-06-06 追加条件)

**AYA literal**: 「**現状の見た目とほぼ変わらない描画が望まれる**」

**位置付け**: 4 原則 (= Core 分散 / 3 OS 共通 / Phase 範囲 / OpenGL を殺さない) に加え、**visual regression ゼロを Phase 2 全工程の必須条件**として全 94 UBO に追加適用。

**運用**:
- 全項目 sub-work (6) A 確定条件 に「**visual regression ゼロ (= 現状の見た目とほぼ変わらない描画)**」を含む
- L1a / L1b / L2 / L3 / L5 (= 個別 verify / 一括 verify) = 各 UBO 通電後 AYA live verify で確認
- L4 (= group verify) = group 全件通電後 AYA live verify で確認
- visual regression 検知時 = 該当 UBO の (4) 設計 task 見直し、(7) 4 原則 gate (= 特に原則 4 OpenGL を殺さない dual-path 維持) 再検証

**例外**: 既存 bug fix (= AYA 明示承認)、性能改善で visual 副作用が許容範囲 (= AYA 明示判断) のみ例外。

**含意 (= L0 / L1-L5 横断)**:
- L0-2 LLStaticHashedString redirect = **dual-write 経路必須** (= GL path 既存 uniform 設定維持で visual 同一保証)
- L0-3 per-shader UBO block 拡大 = **`LL_VULKAN_GLSL` gate 必須** (= GL path 既存 uniform 宣言維持で visual 同一保証)
- L0-4 cadence 再評価 = **stale 許容判断は visual regression risk を含めて判定** (= per-program stale で visual 副作用ゼロの厳格確認)
- L4 C group 中間状態 = **group 完成前の部分通電 visual 不整合は許容、ただし出荷時 visual regression ゼロ**

---

## §A. メタ情報

### §A.1 起案 source

- READINESS.md §1 判定基準 + §3 C 判定 16 group + §4 B 判定 31 件 + §5 集計 + §6 着手順序ヒント
- RELATIONS.md §4 data source 17 系列 + §5 dirty 連動 group 20+ + §8 不明事項 15 dimension
- 各 `<UBO名>.md` §10 不明事項 + §11 他 UBO 関係 + §6 既存 setter call site + §7 現状通電状態 (= 94 file)
- INDEX.md (= 全 94 UBO summary + cadence_tag mapping)

### §A.2 完成 verify

- C-1 = §0 用語 + §1 Layer 体系 + §4/§5/§A skeleton (= 本 step) ✅
- C-2 = §2 L0 横断 protocol 4 件 詳細
- C-3〜C-7 = §3 L1-L5 全 94 項目 trace 順 + sub-work
- C-8 = §4 4 原則 gate + roadmap §2.1 サマリ訂正
- C-9 = 各 `<UBO名>.md` §12 反映 (= C-3〜C-7 並走)

### §A.3 関連 doc

- INDEX.md (= 全 94 UBO summary)
- READINESS.md (= A/B/C 判定 + 不明事項詳細)
- RELATIONS.md (= 関係図 7 dimension)
- 各 `<UBO名>.md` (= 個別 UBO 詳細資料、§12 = 本 doc 進捗反映、C-9)
- design/09-phase-roadmap.md §2.1 (= 本 doc サマリ + link、C-8 で訂正)
- handoff `phase2-prep/handoff-phase2-prep-relations-readiness-complete.md` (= 前 session handoff)
