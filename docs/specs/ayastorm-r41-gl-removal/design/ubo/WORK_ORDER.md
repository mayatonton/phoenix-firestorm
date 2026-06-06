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

**Layer 定義**: C 判定 (= 他 UBO 依存、cross-UBO 同期 / pair / sequential 必須)、READINESS.md §3 16 group 詳細 + RELATIONS.md §5.1 dirty 連動 group 由来
**前提条件**: L0-1〜L0-4 全件確立、L1〜L3 経路 pattern 確立 + cadence 再評価結果反映 (= L0-4 依存 group 多数)、L1a-1 LLStaticHashedString redirect pilot 完了 (= setter redirect 形式利用)
**verify 単位**: **group verify** (= group 全件揃って初めて整合 visual、中間状態は暫定 default 値で破綻回避、AYA literal 2026-06-06)
**trace 順判定**: AYA 既存機能 risk 順 + 機能独立性 (= group 単位の cross-UBO 同期完結性) + group 内 UBO 数 + L0-4 cadence 再評価依存度

**全件共通の (4) 設計 task 共通項** (= group 単位で確立):
- register: 各 UBO 個別 register、program 識別で該当 UBO のみ wire (= L0-1 dispatch protocol)
- write: 1 cvar/state setter call で group 内 N UBO 同時 forward → 各 program 識別で該当 UBO のみ `writeProgramUbo` (= cross-UBO 同期 protocol、RELATIONS.md §5.1 trigger 由来)
- flush: 各 UBO 個別 (= 該当 program bind 単位)
- shader 接続: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= 改変ゼロ、原則 4 維持)
- cross-UBO 同期 protocol: trigger event (= cvar 変化 / preset 切替 / frame 開始 等) で group 内 N UBO 全 dirty (= RELATIONS.md §5.1 各 trigger entry 由来)

**全件共通の (7) 4 原則 gate**:
- 原則 1 (Core 分散): 各 UBO 担当 program = render thread 単独 → ✅、ただし group 内 cross-UBO 同期 logic は 1 setter call に集約 (= 分散粒度維持)
- 原則 2 (3 OS 共通): ✅
- 原則 3 (Phase 2/3): Phase 2 内、L0-4 結果 cadence 再分類対象あり → ✅
- 原則 4 (OpenGL を殺さない): 各 shader `#else` block uniform 個別宣言維持 → ✅
- visual regression ゼロ: AYA live verify (= §5.4 policy)、group verify 単位、中間状態は暫定 default 値で破綻回避

#### §3.5.1 L4-1: aya_sss_skin_flag 3 UBO triple-write group (= AYAstorm r20 SSS skin flag)

**AYA literal 命名 mapping**: READINESS §3.1 (= cross-UBO 3 UBO triple-write、AYA 単純配列で C-1〜C-3 相当)

**位置付け**: L4 group 1 件目 = AYAstorm r20 SSS pipeline 直結、機能維持必須 (= memory `project_ayastorm_visual_realism_chapter` + `project_skin_hash_collision_bom_body`)、3 UBO triple-write 設計判断 + set=1 binding=0 排他切替 (= MaterialUBO ↔ MaterialUBO_Legacy)

**group 概要**:
- 3 UBO 重複格納:
  - **MaterialUBO_Legacy** (set=1 binding=0、256B、8 member、`aya_sss_skin_flag` offset=56) — `class3/deferred/materialF.glsl:38` singleton site
  - **PBROpaqueExtraUBO_Legacy** (set=3 binding=13、256B、4 member = 1 active + 3 pad、`aya_sss_skin_flag` offset=0) — `class1/deferred/pbropaqueF.glsl:160` singleton site (= 起案 sub-step η-5 (b-1) で bare uniform から wrap)
  - **AvatarFParamUBO_Legacy** (set=3 binding=54、256B、4 member = 1 active + 3 pad、`aya_sss_skin_flag` offset=0) — `class1/deferred/avatarF.glsl:76` singleton site
- data source: AYAstorm r20 Phase C `aya_sss_skin_flag` (= `llshadermgr.cpp:1611` `mReservedUniforms.push_back("aya_sss_skin_flag"); // <FS:AYA r20 Phase C>` literal + `llshadermgr.h:138` `AYA_SSS_SKIN_FLAG` enum)
- 3 UBO 1 setter で同時 dirty (= RELATIONS.md §5.1「AYA r20 SSS skin flag cvar 変化 → 3 UBO triple-write」trigger entry)

##### (1) 前提条件
- L0-1 (= dispatch logic、特に set=1 binding=0 排他で MaterialUBO ↔ MaterialUBO_Legacy 切替、program 識別で 3 UBO 個別 wire)
- L0-2 (= LLStaticHashedString UBO redirect 経路、setter redirect 形式利用)
- L0-4 (= cadence 妥当性、AvatarFParamUBO_Legacy / PBROpaqueExtraUBO_Legacy は per-draw 性質を PerProgram で運ぶ stale data risk あり、結果反映待ち)
- L1a-1 (= ClipFParamUBO_Legacy LLStaticHashedString redirect pilot 完了、同 pattern 利用)

##### (2) 不明事項
- `AYA_SSS_SKIN_FLAG` uniform setter call site (= `llshadermgr.cpp:1611` reserved 登録 + `llshadermgr.h:138` enum 登録のみ確認、writer 直接 call site 全件 grep 未取得) **[要追加調査]**
- 3 UBO 同時 write vs program 識別で 1 UBO のみ write 設計選択 (= 全 SSS 関連 program 跨ぎ host 側 dispatch 設計) **[要 AYA 判断]**
- AvatarFParamUBO_Legacy cadence 妥当性 (= SSS skin flag は LLDrawInfo 単位 per-draw attribute、PerProgram cadence で write すると 1 program 内 multi-draw で stale data risk) **[要 L0-4 結果反映 / 要 AYA 判断 = PerDraw 降格検討]**
- PBROpaqueExtraUBO_Legacy cadence 妥当性 (= 同上、material 切替 trigger で per-draw 変化) **[要 L0-4 結果反映]**
- AYAstorm r20 SSS skin 判定 trigger (= LLMaterial flag / texture detect / cvar / LLDrawInfo opt-in) **[要追加調査]**
- bare uniform 残存 (= 3 shader `#else` block bare uniform `aya_sss_skin_flag` setter が OpenGL 経路で host C++ 側に残存しているか、特に PBROpaqueExtra は wrap 起案経緯ゆえ要確認) **[要追加調査]**
- SkinSSSPrototypeFParamUBO_Legacy (= §3.5.2 group 所属、set=3 binding=30) との data source 関係 (= 同 r20 SSS pipeline、別 member 名 `aya_visual_realism_enabled_skinsss_legacy` で AtmoExtra 連動 = §3.5.2 と交差) **[要 verify]**
- MaterialUBO_Legacy の他 7 member (= `morphFactor` / `specular_color` / `camPosLocal` / `emissive_brightness` / `is_mirror` / `env_intensity` / pad) は本 group の triple-write 対象外、それぞれ別 setter 経路で write → 同 UBO 内別 offset を別 trigger で部分 write する dirty 粒度設計 **[要 verify / 要 AYA 判断]**
- `is_mirror` setter site (= `LLVOVolume::setReflectionProbeIsMirror` host 側 `uniform1f` 直接 call 未取得) **[要追加調査]**

##### (3) 調査手法
- **D1 setter Grep**: `AYA_SSS_SKIN_FLAG` enum / `aya_sss_skin_flag` literal uniform writer site 全件 (= `indra/newview` + `indra/llrender` + `indra/newview/lldrawpoolavatar.cpp` + `pipeline.cpp` SSS 経路)
- **D2 既存実装読解**: AYAstorm r20 SSS 章実装 (= memory `reference_attachment_rendering_routing` + `reference_deferred_shader_routing` + `project_skin_hash_collision_bom_body`)、SSS skin 判定 trigger 経路
- **D3 cadence verify**: AvatarF / PBROpaqueExtra per-draw 性質 vs PerProgram cadence stale data risk 実測 (= 1 program 内 multi-draw avatar/PBR 描画で同 UBO 値再 write 必要性)
- **D4 突合**: 3 UBO 同 `aya_sss_skin_flag` offset/size 整合性 (= MaterialUBO_Legacy offset=56 vs PBROpaqueExtra offset=0 vs AvatarF offset=0、全 4B float 整合) + SkinSSSPrototypeFParamUBO_Legacy との data source 別系統 verify

##### (4) 設計 task (= 4 経路、group 単位)
- **register**: 3 UBO 個別 register (= PerProgram cadence triple-buffer)、program 識別で MaterialUBO/Legacy 排他 + PBR opaque program に PBROpaqueExtra + avatar program に AvatarF を limit
- **write**: 1 `AYA_SSS_SKIN_FLAG` setter call → host C++ で program ID 識別 → 該当 program の所属 UBO に `writeProgramUbo` (= MaterialUBO_Legacy bound program なら set=1 binding=0、PBR opaque program なら set=3 binding=13、avatar program なら set=3 binding=54)
- **flush**: 3 UBO 個別 (= 該当 program bind 単位、cmdbuf 経路で triple-buffer 経由 `vkCmdBindDescriptorSets`)
- **shader 接続**: 既存 LL_VULKAN_GLSL block 活性化 (= materialF.glsl:38 + pbropaqueF.glsl:160 + avatarF.glsl:76、改変ゼロ、原則 4 維持)
- **cross-UBO 同期 protocol**: 1 setter call で 3 UBO 全 dirty (= host 側 dispatch logic で SSS skin flag setter を受けて 3 program 系全 dirty bit を立てる、RELATIONS.md §5.1 trigger 実装)
- **MaterialUBO_Legacy 部分 write 設計**: MaterialUBO_Legacy 内 `aya_sss_skin_flag` offset=56 (= 1 member) のみ本 group triggers で write、他 7 member は別 group / 別 trigger (= `is_mirror` / `emissive_brightness` 等の各 setter) で部分 write → 同 UBO 内 member 別 dirty 粒度設計必須

##### (5) 工程 task
- trace 順内位置: L4 group 1 件目 (= AYA 既存機能 risk 大、独立着手可、§3.5.2 visual realism と交差は SkinSSS 経由のみ)
- group 内 並列性: 3 UBO 個別 register 並列可、ただし write 経路 1 setter 集約ゆえ dispatch logic 単一実装
- group 間 並列性: §3.5.3 (shadow_target_width)〜§3.5.16 と並列可 (= 異 cvar/data source)、ただし §3.5.2 SkinSSS との交差は要 verify
- 推定工数: **M** (= 半日 +、setter site 特定 + 3 UBO triple-write dispatch + cadence 再評価 + 3 program 識別 + 3 path visual verify)

##### (6) A 確定条件
- mUseUBO ON + 3 shader 活性化 + setter 通電 (= 3 UBO triple-write)
- AYA live verify: **AYAstorm r20 SSS skin 描画 (= 顔/肌の subsurface scattering 効果) が既存と同一** (avatar + PBR opaque + material 3 path 全件 visual regression ゼロ §5.4、AYAstorm 視覚表現章機能維持必須)
- Vulkan validation 0 件 (= 3 UBO 同 set=1+set=3 配線 + 排他切替 SPIR-V validation + cross-UBO 同期 logic validation)
- 3 UBO 同 `aya_sss_skin_flag` 値同期 verify (= per-frame 整合 + cvar/state 変化時連動 dirty + program 切替時値継承)
- bare uniform 残存ゼロ確認 (= OpenGL 経路でも UBO 経由化済み、3 shader `#else` block bare uniform setter が host C++ 側に残存しない)
- AvatarF / PBROpaqueExtra cadence 判定確定 (= PerProgram 維持 or PerDraw 移行 AYA 判断、L0-4 結果反映)
- MaterialUBO_Legacy 部分 write logic verify (= 本 group の `aya_sss_skin_flag` offset=56 write が他 member dirty を無効化しない、別 group trigger との独立性確認)
- **verify 単位 = group verify** (= 3 UBO 揃って初めて整合 visual、中間状態 (= 1 UBO のみ通電) は暫定 default 値 = `0.0` で破綻回避 = SSS off で描画継続)

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: 3 program 個別 = render thread 単独 → ✅、cross-UBO 同期 logic は 1 setter call に集約 (= dispatch 粒度維持、3 UBO write 並列化候補 = 将来 Core 化で各 UBO 別 thread で write 可能、設計原則 (2) Core 分散実現整合)
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内、AvatarF / PBROpaqueExtra cadence 再分類は L0-4 結果依存 → ✅
- **原則 4 (OpenGL を殺さない)**: 3 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能 → ✅
- **visual regression ゼロ**: AYAstorm r20 SSS skin 描画同一 → AYA live verify、AYAstorm 視覚表現章独自機能維持必須 (= memory `project_ayastorm_visual_realism_chapter` 整合)

---

#### §3.5.2 L4-2: aya_visual_realism + chroma_str + light cvar 7 UBO cross-write group (= 3 sub-cluster 統合)

**AYA literal 命名 mapping**: READINESS §3.2 (= cross-UBO 2 UBO cross-write × 3 sub-cluster、AYA 単純配列で C-5〜C-11 相当)

**位置付け**: L4 group 2 件目 = AYAstorm 視覚表現章 / r30 BD 改善 / light cvar の cross-UBO 同期 protocol が 3 sub-cluster で同居、それぞれ独立 data source ながら「1 cvar → N UBO 同時 dirty」共通 pattern (= RELATIONS.md §5.1 trigger 3 entry)

**group 概要** (= 3 sub-cluster 7 UBO):

**sub-cluster (a) visual_realism 2 UBO** (= AYAstorm r14+ 視覚表現章 + r20 SSS 連動):
- **AtmoExtraUBO_Legacy** (set=3 binding=0、256B、10 member、`aya_visual_realism_enabled` offset=24) — `atmosphericsFuncs.glsl:85` + `skyV.glsl:112` + `skinSSSF.glsl` + `cloudsV.glsl` 4 site
- **SkinSSSPrototypeFParamUBO_Legacy** (set=3 binding=30、256B、7 member、`aya_visual_realism_enabled_skinsss_legacy` offset=44) — `skinSSSF.glsl:80` singleton site (= η-6 phase 2-A rename 範式)
- data source: AYAstorm r14+ `aya_visual_realism_enabled` cvar (= 推定 `AYAVisualRealismEnabled` 等、`llshadermgr.cpp:1614/1616/1841` reserved)
- 2 UBO 同 cvar で 1 setter → 2 UBO 同時 dirty (= RELATIONS.md §5.1「AYA r14+ visual realism enable cvar 変化」trigger)
- **§3.5.1 group との交差**: SkinSSS は r20 SSS pipeline 直結 (= §3.5.1 AvatarF と同 r20 SSS 章)、ただし member 名異 (= `aya_visual_realism_enabled_skinsss_legacy` vs `aya_sss_skin_flag`)、data source 別系統

**sub-cluster (b) chroma_str 2 UBO** (= AYAstorm r30 P4 BD 改善 chroma aberration):
- **PerProgramUBO_PostDeferredF** (set=2 binding=20、256B、2 member、`chroma_str` offset=4) — `postDeferredF.glsl:108` + `postDeferredHQDoFF.glsl` 2 site
- **PerProgramUBO_PostDeferredNoDoFF** (set=2 binding=12、256B、4 member = 1 active + 3 pad、`chroma_str` offset=0) — `postDeferredNoDoFF.glsl:81` singleton site
- data source: `RenderChromaStrength` cvar (= AYAstorm r30 P4 step 4 BD chroma_str、`pipeline.cpp:10060` + `:9555-9557` 2 setter site)
- HAS_DOF_CHROMA permutation 切替で本 UBO ↔ NoDoFF 切替 (= 2 program 別 UBO instance、同 cvar で 2 UBO 同時 dirty 必須)

**sub-cluster (c) light cvar 3 UBO** (= sun_wash / falloff / global_light_strength + V/F pair):
- **PerProgramUBO_PointLightF** (set=2 binding=25、256B、5 member、`viewport` + `sun_wash` (dead) + `falloff` + `global_light_strength`) — `pointLightF.glsl:53` singleton site
- **PerProgramUBO_SpotLightF** (set=2 binding=10、256B、10 member = 統合 = projector 6 + sun_wash + falloff + global_light_strength + 他) — `spotLightF.glsl:74` singleton site
- **PerProgramUBO_PointLightV** (set=2 binding=5、256B、2 member = `center` vec3 + `size` float) — `pointLightV.glsl:63` + `spotLightF.glsl:151` (= declared-but-unused cross-stage 共有、η-28-C type 3 範式)
- data source: `RenderGlobalLightStrength` + `RenderDeferredSunWash` cvar + per-light `LIGHT_FALLOFF` / `LIGHT_CENTER` / `LIGHT_SIZE` (= `pipeline.cpp:10628/10705/10712/11486-11487/11548-11549/11551/11625-11628` 多 setter site)
- 3 UBO 同 cvar (sun_wash/global_light_strength) で double/triple-write + V/F pair + cross-stage declared-but-unused (= RELATIONS.md §5.1「RenderGlobalLightStrength/RenderDeferredSunWash cvar 変化」trigger)

##### (1) 前提条件
- L0-1 (= dispatch logic、特に SpotLightF/PointLightF 別 program 識別 + PointLightV cross-stage 共有 binding=5 の declared-but-unused 対応、η-28-C type 3 範式 verify)
- L0-2 (= LLStaticHashedString UBO redirect 経路、setter redirect 形式利用)
- L0-3 (= per-shader UBO block 拡大方式、atmosphericsFuncs.glsl は windlight consumer 全 program に link = snippet shader)
- L0-4 (= cadence 妥当性、SkinSSS multi-pass dirty pattern + light per-light dirty trigger 再評価)
- L1a-1 (= ClipFParamUBO_Legacy LLStaticHashedString pilot 完了、setter redirect pattern 利用)
- L1b-2 (= FrameLights per-shader block 拡大完了、light 系 UBO 経路 pattern 利用)
- L3-15 (= PerProgramUBO_GodraysF AYAstorm cvar setter pattern 確立)、L3-19 (= ShadowUtilParamUBO_Legacy 大物 UBO multi-site identical 同期 pattern 確立)

##### (2) 不明事項

**sub-cluster (a) visual_realism**:
- `aya_visual_realism_enabled` setter call site (= AYA cvar 推定、`llshadermgr.cpp` reserved 登録のみ確認、writer 直接 call site 全件 grep 未取得) **[要追加調査]**
- AYAstorm r14 / r16 cvar 別 member (= `aya_r14_volumetric_atmosphere_enabled` / `aya_r14_strength` / `aya_r16_aerial_perspective_enabled` / `aya_r16_strength`) の writer site **[要追加調査]**
- SkinSSS multi-pass `aya_blur_dir` 値設定 logic (= horizontal vec2(1,0) / vertical vec2(0,1) host 側選択) **[要追加調査]**
- SkinSSS PerProgram cadence vs multi-pass 2 連続 dirty (= cadence_tag=1 で 1 frame 内 2 回連続 dirty 妥当性、PerDraw 移行候補) **[要 L0-4 結果反映 / 要 AYA 判断]**
- AtmoExtra binding=0 衝突 (= Asset_GLTFNodes と同 set=3 binding=0、cadence 別経路で subset 分離か) **[要 L0-1 結果反映]**
- §3.5.1 SSS skin flag との cross-reference (= AvatarFParamUBO_Legacy/PBROpaqueExtra と SkinSSS が同 r20 SSS pipeline、別 member 名で並列存在の意図) **[要 AYA 判断]**

**sub-cluster (b) chroma_str**:
- HAS_DOF_CHROMA permutation 切替条件 (= 2 program 切替 logic、frame 内動的切替 vs 起動時固定) **[要 verify]**
- vignette path (= `pipeline.cpp:10381`) で `chroma_str` setter が別系統存在か **[要追加調査]**
- DofCombineFParamUBO_Legacy / PerProgramUBO_CofF / PerProgramUBO_PostDeferredV (§3.5.10 group) との DOF data source 共有 (= `CameraDoFResScale` を本 group の res_scale と共有か別 UBO か) **[要 verify / 要 §3.5.10 group 整合]**
- PostDeferredF tail pad (= chroma_str offset=4 後の暗黙 pad) と NoDoFF tail pad (= `_pad_nodof0/1/2`) layout 差分の意図 **[要 verify]**

**sub-cluster (c) light cvar**:
- SpotLightF `far_clip` setter call site (= `DEFERRED_FAR_CLIP` enum / pipeline.cpp 内 setter 行未取得) **[要追加調査]**
- PointLightV cross-stage declared-but-unused (= spotLightF.glsl:151 で binding=5 を frag stage 別 UBO 経由) η-28-C type 3 範式の Vulkan SPIR-V validation 影響 **[要 L0-1 結果反映 / 要 Phase 2 cold launch verify]**
- per-light dirty trigger (= LIGHT_CENTER/SIZE/FALLOFF は per-light 変化、PerProgram cadence で program 内 multi-light 描画時 stale risk) **[要 L0-4 結果反映 / 要 AYA 判断 = PerDraw 降格検討]**
- multi-spot (= `pipeline.cpp:11625-11628` `gDeferredMultiSpotLightProgram`) は本 group の PointLightV か PerDrawUBO_MultiLight (§3.5.16) どちらに属するか **[要 §3.5.16 group 整合]**
- `viewport` member の shader 本体使用箇所 (= pointLightF.glsl 内 grep 未取得、screen-space `gl_FragCoord` 連動推定) **[要 verify]**
- PerProgramUBO_PointLightF 内 `sun_wash` (= dead uniform、shader 本体未参照) の UBO write 維持 vs 削除 **[要 AYA 判断 = layout 不変契約上維持必須だが host 側自由度あり]**

##### (3) 調査手法

**sub-cluster (a)**:
- D1 setter Grep: `aya_visual_realism_enabled` / `aya_r14_*` / `aya_r16_*` / `aya_blur_dir` / `aya_strength` / `aya_glow_*` 全 uniform setter site
- D2 既存実装読解: AYAstorm r14 / r16 / r20 章実装 (= memory `project_ayastorm_r14_pivot_to_light` + `project_ayastorm_visual_realism_chapter` + `project_aya_visual_realism_alpha_protect`)
- D3 cadence verify: SkinSSS multi-pass 2 連続 dirty pattern + AtmoExtra sky preset cadence
- D4 突合: AtmoExtra `aya_visual_realism_enabled` (int、offset=24) vs SkinSSS `aya_visual_realism_enabled_skinsss_legacy` (int、offset=44) data source 同期性 verify

**sub-cluster (b)**:
- D1 setter Grep: `RenderChromaStrength` cvar listener + 2 setter site (= `pipeline.cpp:10060/9555`) 詳細読解 + vignette path setter 別 grep
- D2 既存実装読解: AYAstorm r30 P4 BD chroma_str 実装経緯 (= memory なし、source comment literal `<AYAstorm r30 P4 step 4>` 由来)
- D3 cadence verify: HAS_DOF_CHROMA permutation 切替 trigger
- D4 突合: PostDeferredF offset=4 vs NoDoFF offset=0 同 `chroma_str` data source 同期性

**sub-cluster (c)**:
- D1 setter Grep: `RenderGlobalLightStrength` / `RenderDeferredSunWash` cvar listener + `LIGHT_CENTER/SIZE/FALLOFF` 全 3 program (point/spot/multi-spot) setter site + `DEFERRED_FAR_CLIP` enum
- D2 既存実装読解: η-28-C type 3 範式設計 doc + η-27 1d/1e-A + η-28 Phase 2d-α 設計経緯
- D3 cadence verify: per-light dirty trigger 経路 (= light volume bind の都度 UBO write を `writeProgramUbo` 経由で実施するか) + PointLightV `center/size` per-light 変化対応
- D4 突合: SpotLightF `sun_wash`/`falloff`/`global_light_strength` (offset=28/40/44) vs PointLightF (offset=16/20/24) 同 cvar data source 同期性 + multi-spot `pipeline.cpp:11625-11628` setter 経路で SpotLightF/PointLightF どちら UBO 書込

##### (4) 設計 task (= 4 経路、3 sub-cluster 個別 + group 全体統合)

**共通**:
- register: 7 UBO 個別 register (= PerProgram cadence triple-buffer)、program 識別で各 UBO を該当 program のみ wire
- shader 接続: 既存 LL_VULKAN_GLSL block 活性化 (= 7 file (AtmoExtra=4 + SkinSSS=1 + PostDeferredF=1 + NoDoFF=1 + PointLightF=1 + SpotLightF=1 + PointLightV=1) 改変ゼロ、原則 4 維持)

**sub-cluster (a) visual_realism cross-UBO 同期 protocol**:
- write: `aya_visual_realism_enabled` cvar listener → host C++ で 2 UBO 同時 dirty (= AtmoExtra `aya_visual_realism_enabled` offset=24 + SkinSSS `aya_visual_realism_enabled_skinsss_legacy` offset=44 同値書込み)
- flush: 2 UBO 個別 (= AtmoExtra = windlight consumer 全 program、SkinSSS = skinSSSF program 単独)
- AtmoExtra 部分 write 設計: 同 UBO 内 `aya_visual_realism_enabled` (offset=24) のみ本 group trigger、他 9 member (= lightnorm/haze_horizon/cloud_shadow/sun_moon_glow_factor/aya_r14_*/aya_r16_*) は §3.5.7 sky/cloud group trigger で部分 write
- AtmoExtra binding=0 衝突解決: L0-1 dispatch protocol で Asset_GLTFNodes (cadence=3 PerAsset) と subset 分離

**sub-cluster (b) chroma_str cross-UBO 同期 protocol**:
- write: `RenderChromaStrength` cvar listener → host C++ で 2 UBO 同時 dirty (= PostDeferredF offset=4 + NoDoFF offset=0 同値書込み)
- flush: 2 UBO 個別 (= PostDeferredF = postDeferredF/HQDoFF program、NoDoFF = postDeferredNoDoFF program、HAS_DOF_CHROMA permutation 動的切替で別 program 別 UBO instance bind)
- PostDeferredF 部分 write 設計: 同 UBO 内 `chroma_str` (offset=4) + `res_scale` (offset=0) 両 member 本 group trigger、ただし `res_scale` は別 DOF data source (= `CameraDoFResScale`、§3.5.10 group 整合 verify 要)

**sub-cluster (c) light cvar cross-UBO 同期 protocol**:
- write: `RenderGlobalLightStrength` + `RenderDeferredSunWash` cvar listener → host C++ で 2 UBO 同時 dirty (= PointLightF `sun_wash`/`global_light_strength` (offset=16/24) + SpotLightF `sun_wash`/`global_light_strength` (offset=28/44) 同値書込み)
- per-light write: `LIGHT_CENTER/SIZE/FALLOFF` setter → PointLightV `center/size` (offset=0/12) + PointLightF/SpotLightF `falloff` (offset=20/40) 同時 dirty、PerDraw 降格候補 (= L0-4 結果反映)
- flush: 3 UBO 個別 (= pointLightV/F = point light program、spotLightF = spot light program、PointLightV declared-but-unused は spotLightF 同 binding=5 reuse = η-28-C type 3)
- V/F pair: PointLightV + PointLightF = 同 point light program 内 同時 bind、PointLightV + SpotLightF = cross-stage 共有 (= declared-but-unused、host bind は本 UBO 1 instance、frag 側別 UBO 経由で size 参照)
- multi-spot (= `pipeline.cpp:11625-11628`) は §3.5.16 PerDrawUBO_MultiLight に分岐、本 group では multi-spot 経路は light cvar 書込のみ担当

**group 全体統合**:
- 3 sub-cluster は 3 異 cvar/state listener で独立、ただし全て「1 cvar → N UBO 同時 dirty」共通 pattern
- 各 sub-cluster cross-UBO 同期 logic は 1 setter call 集約 (= Core 分散原則整合、各 UBO write は将来 thread 分割候補)

##### (5) 工程 task
- trace 順内位置: L4 group 2 件目 (= §3.5.1 r20 SSS 完了後、AYAstorm 視覚表現章機能維持 risk 大)
- group 内 並列性: 3 sub-cluster 並列着手可 (= 異 cvar/data source、独立 protocol)
- group 間 並列性: §3.5.3 (shadow) / §3.5.4 (box) / §3.5.13 (pathfinding) と並列可、§3.5.7 sky/cloud + §3.5.10 post-process との交差は要 verify (= AtmoExtra 9 member 共有 + chroma_str/res_scale DOF data source)
- 推定工数: **L** (= 1 日 +、7 UBO 多 setter + 3 sub-cluster 独立 protocol + cross-stage declared-but-unused + per-light dirty cadence + 2 program permutation 切替 + AYAstorm 章機能維持 verify)

##### (6) A 確定条件
- mUseUBO ON + 7 shader 活性化 + 全 setter 通電 (= 3 sub-cluster cross-UBO sync)
- AYA live verify:
  - sub-cluster (a): **AYAstorm r14+ visual realism (= 大気感/SSS 顔肌) + r14/r16 cvar 効果が既存と同一** (visual regression ゼロ §5.4)
  - sub-cluster (b): **DoF chroma aberration (= AYAstorm r30 P4 BD 改善) 効果が既存と同一** (HAS_DOF_CHROMA on/off 両 path、edge-aware shift 同強度)
  - sub-cluster (c): **point/spot/multi-spot light 描画 + sun_wash/global_light_strength 効果が既存と同一** (per-light volume 描画整合 + cvar 変化反映)
- Vulkan validation 0 件 (= 7 UBO 配線 + cross-UBO sync logic + declared-but-unused cross-stage validation + binding=0 衝突解決 verify)
- 各 sub-cluster cvar 変化時連動 dirty verify
- AtmoExtra 部分 write logic verify (= `aya_visual_realism_enabled` offset=24 write が他 9 member dirty 無効化なし)
- PostDeferredF `chroma_str` + `res_scale` 部分 write 整合 (= §3.5.10 group との交差 verify)
- SkinSSS multi-pass `aya_blur_dir` 値設定確定 (= horizontal/vertical pass 別、PerDraw 降格候補)
- light cadence 確定 (= PerProgram 維持 or PerDraw 降格 AYA 判断、L0-4 結果反映)
- PointLightV declared-but-unused SPIR-V validation 0 warning (= η-28-C type 3 範式維持確認)
- **verify 単位 = group verify** (= 7 UBO 揃って初めて整合 visual、中間状態は暫定 default 値 (= visual_realism=0 / chroma_str=0 / light cvar=既存値) で破綻回避)

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: 3 sub-cluster 独立 cvar listener = 各 sub-cluster 別 thread 担当可能 → ✅、各 UBO write 1 setter 集約で分散粒度維持
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内、SkinSSS / light cadence 再分類は L0-4 結果依存 → ✅
- **原則 4 (OpenGL を殺さない)**: 7 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能 → ✅
- **visual regression ゼロ**: 3 sub-cluster 全 path 描画同一 → AYA live verify、AYAstorm 視覚表現章 (r14/r16/r20) + r30 BD 改善 (chroma_str) + light cvar 機能維持必須 (= memory `project_ayastorm_visual_realism_chapter` + `project_ayastorm_r30_bd_improvement_phase` 整合)

---

#### §3.5.3 L4-3: shadow_target_width 3 UBO triple-write group (= shadow target resize 同期)

**AYA literal 命名 mapping**: READINESS §3.3 (= 3 UBO triple-write、AYA 単純配列で C-12〜C-14 相当)

**位置付け**: L4 group 3 件目 = setter 集約 (= pipeline.cpp 7 site)、3 UBO 全 1 member only (= `shadow_target_width`)、最も単純な triple-write group、独立性高 (= AYA 視覚表現章機能と直接交差なし、shadow 系単独)

**group 概要**:
- 3 UBO 重複格納 (= 全 1 member only):
  - **PerProgramUBO_ShadowAlphaMaskV** (set=2 binding=6、256B、4 member = 1 active + 3 pad、`shadow_target_width` offset=0) — `shadowAlphaMaskV.glsl:85` singleton site (= non-PBR shadow alpha mask)
  - **PbrShadowAlphaMaskVParamUBO_Legacy** (set=3 binding=21、256B、1 member、`shadow_target_width` offset=0) — `pbrShadowAlphaMaskV.glsl:89` singleton site (= PBR shadow alpha mask)
  - **AvatarAlphaShadowVParamUBO_Legacy** (set=3 binding=22、256B、1 member、`shadow_target_width` offset=0) — `avatarAlphaShadowV.glsl:59` singleton site (= avatar shadow alpha)
- data source: `LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH` (= `llshadermgr.h:195` enum + `llshadermgr.cpp:1677` reserved)
- setter site: **`pipeline.cpp:8562/8570/8584/8592` (4 site 連続) + `:12596/12611/12642` (3 site 別経路)** = 7 setter site 全特定済 (READINESS §4.3)、`LLGLSLShader::sCurBoundShaderPtr->uniform1f(LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH, (float)target_width)`
- 3 program 別 UBO instance、各 program bind 時に該当 UBO 1 件に write (= RELATIONS.md §5.1「shadow target resize → 3 UBO triple-write」trigger)
- 共通 trigger: shadow target resize (= window resize 連動 / shadow buffer regen)

##### (1) 前提条件
- L0-1 (= dispatch logic、3 program 識別で該当 UBO のみ write)
- L0-4 (= cadence 妥当性、PerProgram は 7 setter site 全て `sCurBoundShaderPtr` 経由ゆえ妥当)
- L1b-1 (= FrameViewProj per-shader 拡大完了、shadow view-projection 経路 pattern 利用)
- L3-19 (= ShadowUtilParamUBO_Legacy 9 setter site + shadow render 全 program 共有 pattern 確立)

##### (2) 不明事項
- 3 UBO 同時 write vs program 識別で 1 UBO のみ write の設計選択 (= shadow pass 内 3 program 切替時の冗長性回避) **[要 AYA 判断]**
- 7 setter site の trigger 経路詳細 (= `pipeline.cpp:8562-8592` 4 site と `:12596-12642` 3 site の分離理由、sun/spot/cube shadow 分岐) **[要追加調査]**
- `target_width` 値の source (= `LLPipeline` 内 shadow target 取得経路、cvar `RenderShadowTargetWidth` 由来 or `LLPipeline::mSunShadowMaps[i]->getWidth()` 由来) **[要追加調査]**
- PerProgramUBO_ShadowAlphaMaskV tail pad 12 B の将来 member 追加意図 (= shadowAlphaMaskV 単独で 4 member、PBR/Avatar 版は 1 member only と非対称) **[要 verify]**
- PerProgramUBO_ShadowCubeV (= set=2 binding=14、§3.5.4 group 所属) との同期 (= shadow cube も target_width 影響あり、別 trigger か) **[要 verify / 要 §3.5.4 整合]**
- shadowAlphaMaskV/pbrShadowAlphaMaskV/avatarAlphaShadowV 3 shader での同 `shadow_target_width` 使用箇所 (= `target_pos_x = 0.5 * (shadow_target_width - 1.0) * pos.x` 共通 logic、3 shader 同 use) **[要 verify]**

##### (3) 調査手法
- **D1 setter Grep**: `DEFERRED_SHADOW_TARGET_WIDTH` 全 setter site (= 既 7 site 特定済、他経路 grep verify)
- **D2 既存実装読解**: `pipeline.cpp:8562/12596` 周辺の shadow rendering flow (= sun shadow / spot shadow / cube shadow / avatar shadow 分岐)
- **D3 cadence verify**: shadow target resize trigger (= window resize listener / shadow buffer regen / cvar 変更)
- **D4 突合**: 3 UBO 同 offset=0 size=4 整合性 (= 全 1 member only、float 同 layout) + PerProgramUBO_ShadowCubeV (§3.5.4) との target_width 関係

##### (4) 設計 task (= 4 経路、group 単位)
- **register**: 3 UBO 個別 register (= PerProgram cadence triple-buffer)、program 識別で shadowAlphaMaskV → PerProgramUBO_ShadowAlphaMaskV / pbrShadowAlphaMaskV → PbrShadowAlphaMaskVParamUBO_Legacy / avatarAlphaShadowV → AvatarAlphaShadowVParamUBO_Legacy wire
- **write**: 7 `DEFERRED_SHADOW_TARGET_WIDTH` setter call → host C++ で program ID 識別 → 該当 program の所属 UBO に `writeProgramUbo` (= `sCurBoundShaderPtr` 経由ゆえ既存経路 1:1 redirect)
- **flush**: 3 UBO 個別 (= 該当 program bind 単位、shadow pass dispatch 毎)
- **shader 接続**: 既存 LL_VULKAN_GLSL block 活性化 (= 3 shader 改変ゼロ、原則 4 維持)
- **cross-UBO 同期 protocol**: shadow target resize 1 event → 3 UBO 全 dirty (= 次 shadow pass dispatch 時 3 program 切替で順次 7 setter site で UBO write、RELATIONS.md §5.1 trigger 実装)

##### (5) 工程 task
- trace 順内位置: L4 group 3 件目 (= §3.5.1/§3.5.2 後、setter 完全特定済で確実、AYA 視覚機能交差なし)
- group 内 並列性: 3 UBO 個別 register/write 並列可、ただし 7 setter site 共通ゆえ write 経路集約必須
- group 間 並列性: §3.5.4 box_center/box_size (= shadow cube 関連、要 verify) / §3.5.1-§3.5.16 残全件と並列可
- 推定工数: **S-M** (= 半日、3 UBO 全 1 member only + setter 完全特定済 + visual verify が shadow visual 同一性で容易、cadence 単純)

##### (6) A 確定条件
- mUseUBO ON + 3 shader 活性化 + 7 setter 通電 (= 3 UBO triple-write)
- AYA live verify: **shadow alpha mask 描画 (= sun shadow 4 cascade + spot shadow + avatar alpha shadow) が既存と同一** (visual regression ゼロ §5.4、shadow edge / fade 同一性確認)
- Vulkan validation 0 件 (= 3 UBO 同 set=2+set=3 配線 + 3 program 識別 dispatch validation)
- 3 UBO 同 `shadow_target_width` 値同期 verify (= shadow target resize 連動 dirty 反映)
- 7 setter site 全件 redirect 確認 (= `pipeline.cpp:8562/8570/8584/8592/12596/12611/12642` 全 UBO 経由)
- shadow pass 内 3 program 切替時 program 識別 dispatch 正常動作 (= 誤 UBO write による shadow artifact 発生なし)
- ShadowCubeV (§3.5.4) との target_width 関係確定 (= 別 group 別 data source 確認)
- **verify 単位 = group verify** (= 3 UBO 揃って初めて整合 visual、中間状態は暫定 default 値 (= shadow_target_width=既存 OpenGL 値) で破綻回避)

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: 3 program 個別 = render thread 単独 → ✅、7 setter site 集約 1 redirect logic で Core 化粒度維持
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内、cadence 単純 (= PerProgram 妥当) → ✅
- **原則 4 (OpenGL を殺さない)**: 3 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能 → ✅
- **visual regression ゼロ**: shadow alpha mask 描画同一 → AYA live verify、shadow 系単独 group ゆえ AYAstorm 視覚機能交差なし

---

#### §3.5.4 L4-4: box_center/box_size 2 UBO program 識別 dispatch group (= occlusion + shadow cube)

**AYA literal 命名 mapping**: READINESS §3.4 (= cross-UBO 2 UBO program 識別 dispatch、AYA 単純配列で C-15〜C-16 相当)

**位置付け**: L4 group 4 件目 = 同 `BOX_CENTER`/`BOX_SIZE` enum 由来 2 UBO、別 program 別用途 (= occlusion query / shadow cube)、host C++ 側 program 識別必須 (= 誤 bind で undefined behavior)、setter site 全 grep 未取得 (= L4 group 中 setter 不明度高)

**group 概要**:
- 2 UBO 同 layout (= 全 4 member = 2 active vec3 + 2 pad):
  - **OcclusionCubeVParamUBO_Legacy** (set=3 binding=50、256B、`box_center` offset=0 + `box_size` offset=16) — `occlusionCubeV.glsl:54` singleton site (= interface/occlusion query 用 bounding cube 描画)
  - **PerProgramUBO_ShadowCubeV** (set=2 binding=14、256B、同 offset) — `shadowCubeV.glsl:57` singleton site (= shadow cube map dispatch、box-shaped occluder)
- data source: `LLShaderMgr::BOX_CENTER` + `BOX_SIZE` enum (= `llshadermgr.h:157-158` + `llshadermgr.cpp:1634-1635` reserved)、setter call site 全件 grep 未取得 (= `pipeline.cpp` に直接 setter 不在、`LLSpatialGroup`/`LLDrawable`/`LLViewerOctree`/`llselectmgr.cpp` 候補)
- 用途差: occlusion = occlusion query bounding cube 描画 (= `vec3 p = position*box_size+box_center` で頂点 transform)、shadow = shadow cube map 中 box-shaped occluder 描画 (= 同 logic、cadence は同 per-program)
- program 識別 dispatch: `occlusionCubeV` program → OcclusionCubeVParamUBO_Legacy (set=3)、`shadowCubeV` program → PerProgramUBO_ShadowCubeV (set=2) 別 set 別 binding ゆえ pipeline layout 上分離

##### (1) 前提条件
- L0-1 (= dispatch logic、特に同 enum で別 program 別 UBO の正しい dispatch)
- L0-4 (= cadence 妥当性、occlusion cube は per-draw 寄り = 1 program 内 多 cube 描画ゆえ PerProgram cadence 妥当性 question)
- L1b-1 (= FrameViewProj per-shader 拡大完了、shadow view-projection 経路)
- §3.5.3 (= shadow_target_width との交差 = ShadowCubeV と shadow target 関連 verify)

##### (2) 不明事項
- `BOX_CENTER` / `BOX_SIZE` 直接 setter call site 全件 (= `pipeline.cpp` に直接 setter 不在、`LLSpatialGroup::doOcclusion` / `LLDrawable` / `LLViewerOctree` / `llselectmgr.cpp` 候補) **[要追加調査]**
- occlusion cube 実 cadence (= per-draw 寄り = 1 program 内多数 cube 描画、metadata cadence_tag=1 PerProgram と乖離) **[要 L0-4 結果反映 / 要 AYA 判断 = PerDraw 降格検討]**
- shadow cube box data owner (= `LLPipeline::generateSunShadow` cube shadow 経路 / `LLDrawable` shadow box / `LLViewerOctree` occlusion box) **[要追加調査]**
- 2 UBO box data source 共有性 (= 同 spatial group bounding box か、別 owner か、共有なら 1 setter で 2 UBO dirty 候補) **[要 verify]**
- PerProgramUBO_PointLightV (= set=2 binding=5、§3.5.2 group 所属) との shape 類似の意図 (= `vec3 + float` 同 layout、誤 bind 防止 design pattern) **[要 verify / 要 §3.5.2 整合]**
- §3.5.3 shadow_target_width との shadow cube 関連 (= ShadowCubeV も shadow render なので target_width 影響あり、別 trigger 別 cadence) **[要 verify / 要 §3.5.3 整合]**
- occlusionCubeV.glsl/shadowCubeV.glsl 2 shader での同 `box_center`/`box_size` 使用箇所 (= 同 `position*box_size+box_center` 頂点 transform logic、共通 implementation) **[要 verify]**

##### (3) 調査手法
- **D1 setter Grep**: `BOX_CENTER` / `BOX_SIZE` 全 setter site 拡大 grep (= `indra/newview` + `indra/llrender` + spatial culling 関連 `LLSpatialGroup::doOcclusion` 経路)
- **D2 既存実装読解**: `LLPipeline` shadow cube generation 経路 + `LLSpatialGroup` occlusion query dispatcher 構造
- **D3 cadence verify**: occlusion cube per-draw 多数描画 vs PerProgram cadence stale risk + shadow cube box dirty trigger
- **D4 突合**: 2 UBO 同 layout (offset=0/16、size=12/12、pad=4/4) 整合性 + box data source 共有性

##### (4) 設計 task (= 4 経路、group 単位)
- **register**: 2 UBO 個別 register、program 識別で occlusionCubeV → OcclusionCubeVParamUBO_Legacy / shadowCubeV → PerProgramUBO_ShadowCubeV wire
- **write**: `BOX_CENTER` / `BOX_SIZE` setter call → host C++ で program ID 識別 → 該当 UBO に `writeProgramUbo` (= 別 set 別 binding ゆえ pipeline layout 上独立、program 識別誤りで cross-UBO 不正 write risk)
- **flush**: 2 UBO 個別 (= 該当 program bind 単位)
- **shader 接続**: 既存 LL_VULKAN_GLSL block 活性化 (= 2 shader 改変ゼロ、原則 4 維持)
- **program 識別 dispatch protocol**: L0-1 dispatch logic で同 enum 由来 setter を program ID で振り分け、誤 dispatch 防止 (= unit test / validation layer 経由)
- **cross-UBO data 共有 (条件付き)**: 2 UBO box data 共有なら 1 setter で 2 UBO dirty 候補、別 owner なら独立 trigger (= verify 結果次第)

##### (5) 工程 task
- trace 順内位置: L4 group 4 件目 (= §3.5.3 shadow group 直後、shadow cube 関連で group 間整合 verify)
- group 内 並列性: 2 UBO 個別 register/write 並列可、ただし setter 共通 enum ゆえ dispatch logic 集約
- group 間 並列性: §3.5.5 GLTF (= 異 data source) と並列可、§3.5.3 shadow + §3.5.2 PointLightV との整合 verify 要
- 推定工数: **M** (= 半日 +、setter site 全 grep 未取得 + 2 UBO box owner 特定 + cadence 再評価 + program 識別 dispatch 設計)

##### (6) A 確定条件
- mUseUBO ON + 2 shader 活性化 + setter 通電 (= program 識別 dispatch)
- AYA live verify:
  - occlusion: **occlusion culling 描画 (= bounding cube visualization、debug-like)** が既存と同一 (visual regression ゼロ §5.4)
  - shadow: **shadow cube map (= sun shadow / spot shadow box-shaped occluder)** が既存と同一 (= shadow edge / depth 同一性)
- Vulkan validation 0 件 (= 2 UBO 同 layout 別 set 別 binding 配線 + program 識別 dispatch validation + cross-binding 不正 bind 0)
- 2 UBO 各 box data 値同期 verify (= 共有 case = 同 box owner、独立 case = 別 owner trigger)
- occlusion cube cadence 判定確定 (= PerProgram 維持 or PerDraw 降格、L0-4 結果反映)
- shadow cube box owner 確定 (= setter site 特定後 data source 明示)
- §3.5.3 shadow_target_width との関係確定 (= ShadowCubeV も shadow target 影響あり、別 UBO 別 cadence で独立 trigger)
- §3.5.2 PointLightV との誤 bind 防止 verify (= `vec3 + float` 同 layout、別用途 binding 区別)
- **verify 単位 = group verify** (= 2 UBO 揃って初めて整合 visual、中間状態は暫定 default 値 (= box_center=0/box_size=既存値) で破綻回避)

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: 2 program 個別 = render thread 単独 → ✅
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内、occlusion cube cadence 再分類は L0-4 結果依存 → ✅
- **原則 4 (OpenGL を殺さない)**: 2 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能 → ✅
- **visual regression ゼロ**: occlusion + shadow cube 描画同一 → AYA live verify、occlusion は debug visualization ゆえ visual 影響軽微、shadow cube は影描画の core ゆえ厳密 verify 必須

---

#### §3.5.5 L4-5: GLTF texture transform 3 UBO program 識別 dispatch group (= material 切替連動)

**AYA literal 命名 mapping**: READINESS §3.5 (= 4 UBO/path 整理、AYA 単純配列で C-17〜C-19 相当、+ pbrmetallicroughnessV.glsl bare local path 含む)

**位置付け**: L4 group 5 件目 = GLTF KHR_texture_transform extension 由来、3 UBO + 1 bare local path、material 切替連動、**MaterialUBO は shell + write 経路通電済 (= Phase 1.A/1.C 完了)** = 本 group の中で唯一通電済 UBO、cadence mismatch 重大 (= material per-draw 切替 vs PerProgram cadence)

**group 概要**:
- 3 UBO + 1 bare local:
  - **PbrOpaqueVParamUBO_Legacy** (set=3 binding=53、256B、2 member = `texture_normal_transform[2]` + `texture_metallic_roughness_transform[2]` = 各 vec4×2 stride=16) — `pbropaqueV.glsl:88` singleton site (= PBR opaque V program)
  - **PerProgramUBO_PbrAlphaV** (set=2 binding=11、256B、同 2 member 同 layout) — `pbralphaV.glsl:98` singleton site (= PBR alpha V program)
  - **MaterialUBO** (set=1 binding=0、256B、10 member full canonical = `texture_matrix0` mat4 + `texture_base_color_transform[2]` + `texture_emissive_transform[2]` + `color` + `emissiveColor` + `metallicFactor` + `roughnessFactor` + pad) — `pbropaqueF.glsl:44` + 3 PBR-extended site (= pbropaqueV/pbralphaV/class2/pbralphaF)、**shell + write 通電済**
  - **bare local path**: `class1/gltf/pbrmetallicroughnessV.glsl:85-86` — UBO 不経由、`gltf_material_data` UBO (= Asset_GLTFMaterials、§3.5.14 group) 経由 derive
- data source: `LLShaderMgr::TEXTURE_NORMAL_TRANSFORM` + `TEXTURE_METALLIC_ROUGHNESS_TRANSFORM` enum (= `llshadermgr.h:59-60` + `llshadermgr.cpp:1521-1522` reserved)、setter 全特定済 = **`llfetchedgltfmaterial.cpp:136-140`** `shader->uniform4fv(...TRANSFORM, 2, ...)` 2 setter site
- MaterialUBO 内 `texture_base_color_transform[2]` + `texture_emissive_transform[2]` = 同 GLTF transform 系列だが別 enum (= `TEXTURE_BASE_COLOR_TRANSFORM`/`TEXTURE_EMISSIVE_TRANSFORM`)、本 group の `texture_normal_transform`/`texture_metallic_roughness_transform` とは別 member、ただし同 GLTF material 由来ゆえ同 trigger
- 3 UBO 同 GLTF material data 由来 cross-write (= RELATIONS.md §5.1「material 切替 → MaterialUBO + Asset_GLTFMaterials + PbrOpaqueV + PbrAlphaV」trigger)

##### (1) 前提条件
- L0-1 (= dispatch logic、set=1 binding=0 排他 (MaterialUBO ↔ MaterialUBO_Legacy = §3.5.1 group) + PbrOpaqueV/PbrAlphaV program 識別)
- L0-3 (= per-shader UBO block 拡大方式、MaterialUBO は 4 PBR-extended (10-member) + 45+ base (6-member layout-compat view) で 49+ file 対象 = upstream merge conflict 高 risk)
- L0-4 (= cadence 妥当性、3 UBO 全 GLTF material per-draw 切替 vs PerProgram cadence、PerDraw 降格候補)
- L1b-1 (= FrameViewProj per-shader 拡大完了、material V/F pair 経路)
- §3.5.1 (= MaterialUBO_Legacy と排他切替 + `aya_sss_skin_flag` の SSS group との交差)
- §3.5.14 (= Asset_GLTFMaterials/Nodes と GLTF asset 連動)

##### (2) 不明事項
- GLTF KHR_texture_transform encoding 詳細 (= `vec4[2]` の中身 = scale.xy/offset.xy/rotation packing、`llfetchedgltfmaterial.cpp:136-140` `normal_packed`/`metallic_roughness_packed` 32B memory layout vs UBO `vec4[2]` 整合) **[要 verify]**
- 3 UBO 実 cadence vs PerProgram (= material per-draw 切替で PerProgram cadence では 1 program 内 multi-material 描画時 stale risk) **[要 L0-4 結果反映 / 要 AYA 判断 = PerDraw 降格検討]**
- MaterialUBO 内 `texture_base_color_transform` / `texture_emissive_transform` setter site (= `LLDrawPoolPBR*` 内 draw-time setter call、grep 未取得) **[要追加調査]**
- MaterialUBO 内 `metallicFactor` / `roughnessFactor` / `emissiveColor` / `color` setter site (= `LLShaderMgr::METALLIC_FACTOR`/`ROUGHNESS_FACTOR`/`EMISSIVE_COLOR`/`DIFFUSE_COLOR` draw-time setter call) **[要追加調査]**
- MaterialUBO base 6-member 内訳 (= 45+ shader 宣言の 6 member 特定、推定 texture_matrix0 + texture_base_color_transform + texture_emissive_transform + color + emissiveColor + ?) **[要 verify]**
- MaterialUBO layout-compat 慣用 (= 10-member full buffer に 6-member view、trailing 4 member 未参照) の Vulkan validation layer 挙動 **[要 Phase 2 cold launch verify]**
- MaterialUBO 用 per-shader block 拡大対象 file 全列挙 (= 現 4 file 宣言済、残 45+ file の状況) **[要追加調査]**
- pbrmetallicroughnessV.glsl bare local path (= `gltf_material_data` UBO 経由 derive) と本 3 UBO 経路の使い分け確立 **[要 verify]**
- §3.5.1 set=1 binding=0 排他切替 (= MaterialUBO 10-member ↔ MaterialUBO_Legacy 6-member、program 単位排他選択) の具体 logic **[要 §3.5.1 整合]**

##### (3) 調査手法
- **D1 setter Grep**: `TEXTURE_NORMAL_TRANSFORM` / `TEXTURE_METALLIC_ROUGHNESS_TRANSFORM` / `TEXTURE_BASE_COLOR_TRANSFORM` / `TEXTURE_EMISSIVE_TRANSFORM` / `METALLIC_FACTOR` / `ROUGHNESS_FACTOR` / `EMISSIVE_COLOR` / `DIFFUSE_COLOR` 全 uniform setter site (= LLDrawPoolPBR / LLFetchedGLTFMaterial / draw-time 経路)
- **D2 既存実装読解**: `LLFetchedGLTFMaterial::bind()` material bind 経路 + `LLDrawPoolPBR*::render` PBR draw flow + GLTF KHR_texture_transform spec 整合
- **D3 cadence verify**: material 切替頻度 per-draw 実測 + PerProgram cadence stale data risk
- **D4 突合**: 3 UBO 同 layout (offset=0/32、size=32/32、vec4[2] stride=16) 整合 + MaterialUBO 内 GLTF transform member offset (offset=64/96) との関係 + bare local path data flow

##### (4) 設計 task (= 4 経路、group 単位)
- **register**: 3 UBO 個別 register、program 識別で pbropaqueV → PbrOpaqueVParamUBO_Legacy / pbralphaV → PerProgramUBO_PbrAlphaV / PBR-extended program → MaterialUBO wire (= MaterialUBO は既通電、追加配線最小)
- **write**: 2 `TEXTURE_NORMAL_TRANSFORM` / `TEXTURE_METALLIC_ROUGHNESS_TRANSFORM` setter call (`llfetchedgltfmaterial.cpp:136-140`) → host C++ で program ID 識別 → 該当 UBO に `writeProgramUbo` (= MaterialUBO は別 offset (= base color/emissive)、PbrOpaque/PbrAlpha は normal/metallic-roughness)
- **flush**: 3 UBO 個別 (= 該当 program bind 単位、cmdbuf 経路で triple-buffer)
- **shader 接続**: 3 file 既存 LL_VULKAN_GLSL block 活性化 + MaterialUBO 45+ base shader 拡大 (= L0-3 per-shader 拡大対象、layout-compat 6-member view、原則 4 維持 = `#ifdef LL_VULKAN_GLSL` gate で OpenGL 100% 維持)
- **cross-UBO 同期 protocol**: material 切替 1 event → 3 UBO 全 dirty (= RELATIONS.md §5.1 trigger、ただし MaterialUBO は base color/emissive offset、PbrOpaque/PbrAlpha は normal/metallic-roughness offset で別 member 部分 write)
- **MaterialUBO 部分 write 設計**: 同 UBO 内 GLTF transform 4 member (= base_color/emissive/normal/metallic-roughness 全) のうち base_color/emissive のみ本 group で扱う、normal/metallic-roughness は PbrOpaque/PbrAlpha 経由 (= 別 UBO 別 program)
- **set=1 binding=0 排他 logic**: §3.5.1 group との整合 (= MaterialUBO 10-member full = PBR-extended 4 program / MaterialUBO_Legacy 6-member base = 残 45+ program、program 単位選択排他)

##### (5) 工程 task
- trace 順内位置: L4 group 5 件目 (= §3.5.4 後、MaterialUBO 通電済で着手容易性高、ただし per-shader 拡大 45+ file が L0-3 依存)
- group 内 並列性: 3 UBO 個別 register/write 並列可、ただし setter 集約 (= `llfetchedgltfmaterial.cpp:136-140` 2 setter site)
- group 間 並列性: §3.5.1 (MaterialUBO 排他) + §3.5.14 (Asset_GLTFMaterials/Nodes 連動) との整合 verify 要、他 group と並列可
- 推定工数: **L** (= 1 日 +、3 UBO + 8 setter site (= GLTF transform 4 + PBR factor 4) + MaterialUBO 45+ base shader 拡大 + layout-compat 慣用 verify + cadence 再評価)

##### (6) A 確定条件
- mUseUBO ON + 3 shader + 45+ MaterialUBO base shader 活性化 + setter 通電
- AYA live verify: **PBR 描画 (= opaque + alpha 両 path) + GLTF material texture transform (= normal/metallic-roughness/base_color/emissive UV scale/offset/rotation) + PBR factor (= metallic/roughness/emissive/diffuse color) が既存と同一** (visual regression ゼロ §5.4)
- Vulkan validation 0 件 (= 3 UBO 配線 + layout-compat 6-member view 合法性 + set=1 排他 dispatch validation + 45+ shader 拡大 SPIR-V validation)
- 3 UBO 各 transform 値同期 verify (= material 切替時連動 dirty + program 切替時値継承)
- MaterialUBO 部分 write logic verify (= base_color/emissive offset と normal/metallic-roughness offset の独立性)
- §3.5.1 set=1 排他切替動作 verify (= MaterialUBO ↔ Legacy 不正混在なし)
- §3.5.14 Asset_GLTFMaterials との data source 共有性確定 (= bare local path との関係)
- 3 UBO cadence 判定確定 (= PerProgram 維持 or PerDraw 降格、L0-4 結果反映)
- MaterialUBO 用 per-shader 拡大対象 45+ file 全特定 + 拡大完了
- **verify 単位 = group verify** (= 3 UBO 揃って初めて整合 visual、中間状態は暫定 default 値 (= transform=identity/factor=既存値) で破綻回避 = material 無装飾描画継続)

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: 3 program 個別 = render thread 単独 → ✅、2 setter call 集約 1 redirect logic で Core 化粒度維持
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内、3 UBO cadence 再分類は L0-4 結果依存、MaterialUBO 45+ shader 拡大は L0-3 結果依存 → ✅
- **原則 4 (OpenGL を殺さない)**: 3 shader `#else` block uniform 個別宣言維持 + MaterialUBO 45+ base shader 拡大は `#ifdef LL_VULKAN_GLSL` gate で OpenGL 100% 維持 → ✅
- **visual regression ゼロ**: PBR 描画同一 → AYA live verify、PBR rendering core ゆえ厳密 verify 必須

---

#### §3.5.6 L4-6: water 系 5 UBO 連動 dirty group (= LLEnvironment LLSettingsWater 由来)

**AYA literal 命名 mapping**: READINESS §3.6 (= water 系 4-5 UBO 連動 dirty、AYA 単純配列で C-20〜C-24 相当)

**位置付け**: L4 group 6 件目 = water rendering 全 5 UBO 同 LLEnvironment LLSettingsWater data source、rename 範式 (= η-6 §3.3 nameless block member 衝突回避)、cadence mismatch 重大 (= time/eyeVec/lightDir per-frame 変化 vs PerProgram cadence)、water settings 切替 + camera move + day cycle 3 trigger 連動

**group 概要**:
- 5 UBO:
  - **WaterFogUBO_Legacy** (set=3 binding=9、256B、5 member、`waterFogColor`/`waterFogDensity`/`waterFogKS` + 2 pad) — `waterFogF.glsl:48` singleton site
  - **WaterVParamUBO_Legacy** (set=3 binding=60、256B、6 member、`waveDir1`/`waveDir2`/`time`/`eyeVec`/`waterHeight`/`lightDir`) — `waterV.glsl:61` + `waterF.glsl:108` V/F multi-site identical
  - **UnderWaterFParamUBO_Legacy** (set=3 binding=39、256B、14 member 最大、rename 4 member = `lightDir_underwater_legacy`/`eyeVec_underwater_legacy`/`waterFogColor_underwater_legacy`/`waterFogKS_underwater_legacy` + `fogCol`/`lightExp`/`specular`/`refScale`/`fbScale`/`znear`/`zfar`/`kd`/`waterFogColorLinear`/`screenRes`) — `underWaterF.glsl:63` singleton site、**FrameLights consume guard wrap** (= `#ifndef FRAME_LIGHTS_DEFINED`)
  - **PerProgramUBO_WaterF** (set=2 binding=23、256B、8 member、`specular`/`blend_factor`/`normScale`/`blurMultiplier`/`refScale`/`kd`(dead)/`fresnelScale`/`fresnelOffset`) — `waterF.glsl:119` singleton site
  - **PerProgramUBO_WaterHazeV** (set=2 binding=15、256B、4 member = 1 active + 3 pad、`above_water` int) — `waterHazeV.glsl:86` + `waterHazeF.glsl` V+F shared host 1 bind
- data source: LLEnvironment LLSettingsWater + LLDrawPoolWater + LLViewerCamera (= per-frame state) + framebuffer state
- rename 4 member (= η-6 §3.3 範式) = WaterFog/WaterV と UnderWaterF 間で同 host data source duplicate write 解消の rename
- `above_water` (WaterHazeV) = SimpleColorFParamUBO_Legacy.waterSign (= §3.4.4 L3 group 所属) と data 共有候補 [要 verify]
- 連動 trigger:
  - water settings 切替 → 4 UBO (Fog/V/UnderWater/WaterF) 連動 dirty
  - camera move (= per-frame) → V (time/eyeVec) + UnderWater (eyeVec/screenRes/znear/zfar) cadence stale risk
  - day cycle → V (lightDir) + UnderWater (lightDir_legacy) 連動 dirty

##### (1) 前提条件
- L0-1 (= dispatch logic、5 program 識別)
- L0-2 (= LLStaticHashedString redirect、rename 4 member の cross-UBO 同期)
- L0-3 (= per-shader 拡大、`atmosphericsFuncs.glsl` 等 windlight 連動)
- L0-4 (= cadence 妥当性、per-frame 変化 member 多数 = time/eyeVec/lightDir/screenRes/znear/zfar、PerDraw/PerFrame 降格候補)
- L1b-1 (= FrameViewProj per-shader 拡大完了、water V/F program で view+proj 経路)
- L1b-2 (= FrameLights per-shader 拡大完了、underWaterF FrameLights guard 整合)
- §3.4.4 L2-4 (= DeferredUtilParamUBO_Legacy `waterSign` per-program vs per-draw cadence 判断、本 group `above_water` 同種)
- §3.4.7/9 L3 group (= SimpleColorFParamUBO_Legacy / SnapshotFrameFParamUBO_Legacy 等、waterSign cadence 整合)

##### (2) 不明事項
- 5 UBO 各 member の setter call site (= `LLDrawPoolWater::renderWater` + `LLEnvironment` water settings + `LLViewerCamera` 経路、grep 未取得) **[要追加調査]**
- WaterFog ↔ UnderWaterF の rename 4 member duplicate write 最適化 (= 同 host data source から 2 UBO 同時 update 経路、duplicate write 性能 risk) **[要 verify / 要 AYA 判断]**
- WaterV time per-frame stale risk (= PerProgram cadence で water animation time の per-frame 更新が反映されるか) **[要 L0-4 結果反映 / 要 PerFrame 降格検討]**
- WaterV eyeVec per-frame stale risk (= camera move 反映) **[要 L0-4 結果反映]**
- UnderWaterF eyeVec_underwater_legacy / screenRes / znear / zfar per-frame stale risk **[要 L0-4 結果反映]**
- WaterHazeV `above_water` 判定 logic (= camera Z vs water plane Z 判定、`LLPipeline`/`LLViewerCamera` 経路) **[要追加調査]**
- WaterHazeV V+F shared host bind の VkShaderStageFlags (= VERTEX | FRAGMENT 両指定) **[要 verify]**
- WaterHazeV above_water vs SimpleColorFParamUBO_Legacy.waterSign / DeferredUtilParamUBO_Legacy.waterSign data 共有候補 **[要 verify / 要 §3.4 L3 group 整合]**
- WaterF `kd` declared-but-unused 維持 (= layout 不変契約、host setter no-op) **[要 AYA 判断 = 維持必須前提]**
- WaterV/F multi-site identical (= waterV.glsl:61 + waterF.glsl:108 byte-for-byte 一致) byte-level verify + blueprint 改変時の同期 gate **[要 verify]**
- water cadence 再評価結果に応じた group 全体 PerProgram → PerFrame/PerDraw 降格選択 **[要 L0-4 結果反映 / 要 AYA 判断]**

##### (3) 調査手法
- **D1 setter Grep**: 5 UBO 全 member setter site (= rename 前後の名前で grep、WaterFog `waterFogColor`/`waterFogDensity`/`waterFogKS` / WaterV `waveDir1`/`waveDir2`/`time`/`eyeVec`/`waterHeight`/`lightDir` / UnderWaterF 14 member / WaterF 8 member / WaterHazeV `above_water`)
- **D2 既存実装読解**: `LLDrawPoolWater::renderWater` + `LLEnvironment::getCurrentWater` + `LLPipeline::renderWaterHaze` 経路 + `LLViewerCamera::getOrigin/getNear/getFar` + day cycle blend
- **D3 cadence verify**: per-frame 変化 member (time/eyeVec/lightDir/screenRes/znear/zfar/above_water) PerProgram cadence stale data risk 実測
- **D4 突合**: rename 4 member 同 host data source 整合 (= WaterFog `waterFogColor` ↔ UnderWaterF `waterFogColor_underwater_legacy` value 同期) + WaterV/F multi-site byte 一致 + WaterHazeV V+F shared 整合

##### (4) 設計 task (= 4 経路、group 単位)
- **register**: 5 UBO 個別 register (= PerProgram cadence triple-buffer、L0-4 結果次第で PerFrame/PerDraw 降格)、program 識別で water 系 program (= waterFogF/waterV/waterF/underWaterF/waterHazeV/F) に該当 UBO wire
- **write**: 5 UBO 個別 setter redirect、rename 4 member は 1 host setter で 2 UBO 同時 dirty (= WaterFog + UnderWaterF 同時 write)
- **flush**: 5 UBO 個別 (= 該当 program bind 単位、cmdbuf 経路)
- **shader 接続**: 5 file 既存 LL_VULKAN_GLSL block 活性化 (= 改変ゼロ、原則 4 維持)
- **cross-UBO 同期 protocol**:
  - water settings 切替 1 event → 4 UBO (Fog/V/UnderWater/WaterF) 連動 dirty
  - camera move 1 event → V (time/eyeVec) + UnderWater (eyeVec_legacy/screenRes/znear/zfar) 連動 dirty (cadence 降格依存)
  - day cycle 1 event → V (lightDir) + UnderWater (lightDir_legacy) 連動 dirty
  - above_water 切替 1 event → WaterHazeV + (要 verify) SimpleColorF/DeferredUtil waterSign 連動 dirty
- **rename 4 member duplicate write 最適化**: 1 host setter で 2 UBO 同時 forwardToUboUpload、cross-UBO data 同期 protocol で write overhead 抑制
- **V+F shared host bind (WaterHazeV)**: VkShaderStageFlags = VERTEX | FRAGMENT 両指定で 1 bind に統合

##### (5) 工程 task
- trace 順内位置: L4 group 6 件目 (= §3.5.5 後、water rendering 単独 group ゆえ AYAstorm 視覚機能交差軽微 = AtmoExtra/SkinSSS 経由なし)
- group 内 並列性: 5 UBO 個別 register/write 並列可、ただし rename 4 member 同期 logic 集約
- group 間 並列性: §3.5.7 sky/cloud (= LLEnvironment 由来 sky settings 連動候補)、§3.4.4 L2-4 DeferredUtil waterSign との整合 verify 要
- 推定工数: **L** (= 1 日 +、5 UBO + 30+ setter site + rename duplicate write 最適化 + cadence 再評価 + V/F multi-site verify + V+F shared bind)

##### (6) A 確定条件
- mUseUBO ON + 5 shader 活性化 + setter 通電 (= cross-UBO sync)
- AYA live verify: **water rendering 全 path 描画 (= waterFog / water V (= 水面波) / water F (= 水面色) / underWater (= 水中描画) / waterHaze (= 水中靄) ) が既存と同一** (visual regression ゼロ §5.4、water settings 切替 + camera 水上/水中切替 + day cycle 連動 verify)
- Vulkan validation 0 件 (= 5 UBO 配線 + V+F shared bind + V/F multi-site SPIR-V 整合 + rename 4 member 同期 validation)
- 5 UBO 各 member 値同期 verify (= water settings + camera + day cycle 各 trigger 連動)
- rename 4 member duplicate write 動作 verify (= WaterFog `waterFogColor` ↔ UnderWaterF `waterFogColor_underwater_legacy` 同値、cvar 変化反映)
- WaterV/F multi-site byte-level 同期 verify (= 将来 blueprint 改変時の自動同期 gate)
- WaterHazeV V+F shared bind 動作 verify (= V/F 両 stage で 1 UBO access)
- water cadence 判定確定 (= PerProgram 維持 or PerFrame/PerDraw 降格 AYA 判断、L0-4 結果反映)
- above_water vs waterSign 関係確定 (= §3.4.4 L2-4 + §3.4.4 L3-4 / L3-5 group 整合)
- **verify 単位 = group verify** (= 5 UBO 揃って初めて整合 visual、中間状態は暫定 default 値 (= water_settings=既存 / above_water=0 / time=0) で破綻回避)

##### (7) 4 原則 gate
- **原則 1 (Core 分散)**: 5 program 個別 = render thread 単独 → ✅、rename 4 member 同期 logic 集約で Core 化粒度維持
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内、water cadence 再分類は L0-4 結果依存 (PerFrame/PerDraw 降格選択) → ✅
- **原則 4 (OpenGL を殺さない)**: 5 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能 → ✅
- **visual regression ゼロ**: water rendering 全 path 描画同一 → AYA live verify、water 系単独 group ゆえ AYAstorm 視覚機能交差軽微、ただし AYA r12.1 FSParcelStreamQuality + r13 OBB occlusion (memory) との関連は別軸 (= 本 group 影響範囲外)

---

#### §3.5.7 L4-7: sky/cloud/atmospheric 10 UBO group (= LLSettingsSky 由来 + day cycle + AYA r14/r16/r18 cvar)

**AYA literal 命名 mapping**: READINESS §3.7 (= sky/cloud/atmospheric 群連動 dirty、25-34 = 10 UBO、AYA 単純配列で C-25〜C-34 相当)

**位置付け**: L4 group 7 件目 = 本 phase 最大 group (= 10 UBO)、AYAstorm 視覚表現章直結 (= AYA r14 volumetric atmosphere / r16 aerial perspective / r18 volumetric clouds + r20 SSS skin との交差) + LLSettingsSky 由来 sky preset 切替 + day cycle 連続変化 (= per-frame trigger) + camPosLocal per-frame 共有 + binding=0/1/2 衝突 3 site (= Asset_GLTFNodes / Asset_GLTFMaterials / Skin_GLTFJoints との descriptor set 内 binding 共有 = L0-1 dispatch protocol 経路) + cloudsV/cloudsF 複製併存 (= η-14 path G-β) + sub-cluster 内 PerFrame/PerProgram cadence 混在 (= L0-4 再評価依存度 highest)

**group 概要** (= 5 sub-cluster 10 UBO、cross-UBO 同期 protocol 4 trigger 由来):

**sub-cluster (a) FrameAtmosphere_Lighting 1 UBO** (= set=0 PerFrame、本 group 唯一の PerFrame):
- **FrameAtmosphere_Lighting** (set=0 binding=2、256B、20 member、blue_horizon offset=48 + sunlight_color/moonlight_color/ambient_color/blue_density/glow vec3 ×5 + haze_density/density_multiplier/distance_multiplier/max_y/sky_sunlight_scale/sky_ambient_scale/sky_hdr_scale/scene_light_strength float ×8 + classic_mode/cube_snapshot int ×2 + minimum_alpha/max_cof float ×2 + _pad_atm0/_pad_atm1 ×2) — `class1/lighting/lightAlphaMaskF.glsl:37-45` + `class1/lighting/lightAlphaMaskNonIndexedF.glsl:37-45` + 4 sample sites (= atmosphericsF / atmosphericsHelpersV / atmosphericsFuncs / atmosphericsHelpersF) + sky shader (= skyV/skyF) 拡大対象、shell 通電済 + write 経路本格化済 (= Phase 1.A PA-8 + 1.C PC-7γ-1、blue_horizon setter `llsettingsvo.cpp:1057` literal `shader->uniform3fv(LLShaderMgr::BLUE_HORIZON, blue_horizon.mV);`)
- data source: AYAstorm 視覚表現章 + LLSettingsSky + post-process pass:
  - sunlight_color / moonlight_color / ambient_color / blue_horizon / blue_density / glow / haze_density / density_multiplier / distance_multiplier / max_y = `LLSettingsSky` getter 経由 (= `llsettingssky.cpp:74-105` SETTING_BLUE_HORIZON 等 literal)
  - scene_light_strength = `LLSettingsSky::getSceneLightStrength()` (= verify 要)
  - sky_sunlight_scale / sky_ambient_scale / sky_hdr_scale = sky shader 用 scale factor (= verify 要、AYAstorm 視覚表現章 cvar 候補)
  - classic_mode / cube_snapshot = mode flag (= reflection probe regenerate 中の特殊処理 gate、verify 要)
  - minimum_alpha / max_cof = post-process pass param (= postDeferred 経路由来、PerProgram 候補だが frame 内 stable ゆえ per-frame 配置採用、AYA option 起源)

**sub-cluster (b) AtmoExtra 残 9 member 部分 write** (= §3.5.2 sub-cluster (a) で `aya_visual_realism_enabled` offset=24 既起案、本 group では他 9 member trigger):
- **AtmoExtraUBO_Legacy** (set=3 binding=0、256B、10 member、本 group 対象 = `lightnorm` offset=0 / `haze_horizon` offset=12 / `cloud_shadow` offset=16 / `sun_moon_glow_factor` offset=20 / `aya_r14_volumetric_atmosphere_enabled` offset=28 / `aya_r14_strength` offset=32 / `aya_r16_aerial_perspective_enabled` offset=36 / `aya_r16_strength` offset=40 / `_pad_atmo_extra_legacy_0` offset=44) — 4 shader site (= `class1/windlight/atmosphericsFuncs.glsl:85-101` 起源 + `class1/deferred/skyV.glsl:112` + `class1/deferred/skinSSSF.glsl` + `class1/deferred/cloudsV.glsl`)
- data source:
  - lightnorm (vec3 sun direction) = `llsettingsvo.cpp:869/875/1349` literal `shader->uniform3fv(LLViewerShaderMgr::LIGHTNORM, light_direction)` + `lldrawpoolwater.cpp:298` 4 writer site
  - haze_horizon = `llsettingsvo.cpp:844` literal `draw_real(shader, getHazeHorizon(), LLShaderMgr::HAZE_HORIZON)`
  - cloud_shadow = `llsettingsvo.cpp:849` literal `draw_real(shader, getCloudShadow(), LLShaderMgr::CLOUD_SHADOW)`
  - sun_moon_glow_factor = `llsettingsvo.cpp:1071` literal `shader->uniform1f(LLShaderMgr::SUN_MOON_GLOW_FACTOR, getSunMoonGlowFactor())`
  - aya_r14_volumetric_atmosphere_enabled / aya_r14_strength = AYAstorm r14 章 cvar (= memory `project_ayastorm_r14_pivot_to_light` + `project_ayastorm_visual_realism_chapter`、setter 不明 **[要追加調査]**)
  - aya_r16_aerial_perspective_enabled / aya_r16_strength = AYAstorm r16 章 cvar (= setter 不明 **[要追加調査]**)
- **§3.5.2 group との交差**: 同一 UBO 内、`aya_visual_realism_enabled` offset=24 は §3.5.2 sub-cluster (a) trigger (= AYA r14+ visual_realism cvar 変化) で SkinSSS と cross-write、本 group では sky preset / day cycle / lightnorm / AYA r14/r16 cvar trigger で 9 member 部分 write → 同 UBO 内 member 別 dirty 粒度設計必須 (= §3.5.1 MaterialUBO_Legacy 部分 write 設計と同型)

**sub-cluster (c) Sky V/F pair 2 UBO** (= LLSettingsSky 由来 V/F 2 UBO、binding=1/2 衝突):
- **SkyVParamUBO_Legacy** (set=3 binding=1、256B、2 member = `camPosLocal` vec3 offset=0 + `_pad_sky_v_legacy_0` vec3 offset=16) — `class1/deferred/skyV.glsl:83` singleton site (= blueprint literal extract source)
- **SkyFParamUBO_Legacy** (set=3 binding=2、256B、4 member = `hdri_split_screen` / `moisture_level` / `droplet_radius` / `ice_level` float ×4 offset=0/4/8/12) — `class1/deferred/skyF.glsl:117` singleton site
- data source:
  - camPosLocal = `LLViewerCamera::getOrigin()` 経由 local space 変換 (= verify 要、per-frame 変化 = cadence mismatch 重大 [要 L0-4 結果反映 / 要 AYA 判断 = PerFrame 降格候補])
  - hdri_split_screen = HDRI debug split-screen toggle (= LLSettingsSky / debug setting 由来、verify 要)
  - moisture_level / droplet_radius / ice_level = WindLight cloud microphysics params (= LLSettingsSky / LLEnvironment 由来、verify 要、4 member setter 全件 **[要追加調査]**)
- binding 衝突 3 site の 2 件:
  - **SkyV binding=1** ↔ **Asset_GLTFMaterials binding=1** (cadence=3 PerAsset) = L0-1 dispatch protocol 経路 (= program 識別で descriptor set 内容差替、verify 要)
  - **SkyF binding=2** ↔ **Skin_GLTFJoints binding=2** (cadence=3 PerSkin) = 同 L0-1 dispatch 経路、skyF program は GLTF skin を必要としないため衝突なし推定 (= verify 要)

**sub-cluster (d) Clouds V/F pair 2 UBO + 複製併存** (= LLSettingsSky 由来 V/F 2 UBO、camPosLocal/cloud_scale 共有):
- **CloudsVParamUBO_Legacy** (set=3 binding=3、256B、4 member = `camPosLocal` vec3 offset=0 + `cloud_scale` float offset=12 + `cloud_color` vec3 offset=16 + `_pad_clouds_v_legacy_0` float offset=28) — `class1/deferred/cloudsV.glsl:106` 起源 + `class1/deferred/cloudsF.glsl:79` 複製併存 (= η-14 path G-β、`CLOUDS_V_PARAM_UBO_LEGACY_DEFINED` guard で cloud_scale 解決経路、`cloudsF.glsl:71-83`)
- **CloudsFParamUBO_Legacy** (set=3 binding=4、256B、8 member = `cloud_pos_density1` vec3 offset=0 + `blend_factor` float offset=12 + `cloud_pos_density2` vec3 offset=16 + `cloud_variance` float offset=28 + `aya_r18_cloud_volumetric_enabled` int offset=32 + `aya_r18_strength` float offset=36 + `_pad_clouds_f_legacy_0` offset=40 + `_pad_clouds_f_legacy_1` offset=44) — `class1/deferred/cloudsF.glsl:61` singleton site
- data source:
  - camPosLocal (V) = sub-cluster (c) SkyV と同 source (= LLViewerCamera 経由、cross-UBO 同 data source、verify 要 D4 突合)
  - cloud_scale (V) = WL cloud scale (= LLSettingsSky cloud preset 由来、writer **[要追加調査]**)
  - cloud_color (V) = WL cloud color (= LLSettingsSky cloud preset 由来、writer **[要追加調査]**)
  - cloud_pos_density1 / cloud_pos_density2 (F) = WL cloud density (= `llinventory/llsettingssky.cpp:84` literal `SETTING_CLOUD_POS_DENSITY1("cloud_pos_density1")` + `llshadermgr.cpp:1622` reserved + `llshadermgr.h:149` CLOUD_POS_DENSITY1 enum、writer **[要追加調査]**)
  - blend_factor (F) = WL cloud blend factor (= sub-cluster (e) Stars/SunDisc blend_factor と同名異 data source 候補、verify 要)
  - cloud_variance (F) = WL cloud variance (= verify 要)
  - aya_r18_cloud_volumetric_enabled / aya_r18_strength = AYAstorm r18 章 cvar (= memory `project_ayastorm_visual_realism_chapter` r18 = volumetric clouds、writer **[要追加調査]**)
- **複製併存 risk**: cloudsF.glsl:79 で CloudsVParamUBO_Legacy (set=3 binding=3) を fragment 側に guard 付き複製、Vulkan 仕様上 1 pipeline 内で同 set=N binding=M を vertex/fragment 両 stage で参照可能 (= descriptor set bind は pipeline 単位、stage visibility は pipeline layout で制御)、layout (member 構成) 両 stage 完全一致必須 (= blueprint コメント `verified identical` 担保)、Vulkan validation 0 件確認要

**sub-cluster (e) Stars F/V + SunDisc + Moon 4 UBO** (= day cycle blend_factor/time 共有、`lldrawpoolwlsky` 由来):
- **StarsFParamUBO_Legacy** (set=3 binding=42、256B、3 member = `blend_factor` float offset=0 + `custom_alpha` float offset=4 + `time` float offset=8) — `class1/deferred/starsF.glsl:57` singleton site
- **StarsVParamUBO_Legacy** (set=3 binding=45、256B、1 member = `stars_v_time` float offset=0) — `class1/deferred/starsV.glsl:67` singleton site (= F の `time` の rename 版、shader 側 nameless block member 衝突回避、underWaterF/waterFog rename pattern と同形 [要 verify])
- **SunDiscFParamUBO_Legacy** (set=3 binding=43、256B、1 member = `blend_factor` float offset=0) — `class1/deferred/sunDiscF.glsl:48` singleton site
- **MoonFParamUBO_Legacy** (set=3 binding=44、256B、1 member = `moon_brightness` float offset=0) — `class1/deferred/moonF.glsl:67` singleton site (= setter 特定済 = `lldrawpoolwlsky.cpp:453-455` literal `moon_shader->uniform1f(LLShaderMgr::MOON_BRIGHTNESS, moon_brightness);` + `llsettingsvo.cpp:853` literal `draw_real(shader, getMoonBrightness(), LLShaderMgr::MOON_BRIGHTNESS);`)
- data source:
  - blend_factor (Stars F + SunDisc F、同名 4B float) = day/night blend 計算 (= LLEnvironment day cycle / WindLight blend、同一 source 候補、verify 要 D4 突合)
  - custom_alpha (Stars F) = stars custom alpha (= debug settings / LLEnvironment 由来、setter **[要追加調査]**)
  - time (Stars F) / stars_v_time (Stars V) = per-frame time accumulator (= twinkle/position animation、cadence mismatch 重大、stars_v_time は F `time` の rename = 同 data source 候補、verify 要 D4 突合)
  - moon_brightness (Moon F) = `LLSettingsSky::mMoonBrightness` (= `llinventory/llsettingssky.cpp:1201` literal `mMoonBrightness = (F32)settings[SETTING_MOON_BRIGHTNESS].asReal();`、setter `setMoonBrightness(F32)` `llsettingssky.cpp:2152`)
- **cross-UBO 同 data source**: `blend_factor` member は Stars F + SunDisc F + CloudsF で同名 (= 4 UBO 共有候補)、day/night blend 計算経路が同一なら 1 setter call で 4 UBO 同時 dirty (= RELATIONS.md §5.1 trigger、verify 要)

##### (1) 前提条件

- L0-1 (= dispatch logic、特に set=3 binding=0/1/2 衝突 3 site = AtmoExtra ↔ Asset_GLTFNodes / SkyV ↔ Asset_GLTFMaterials / SkyF ↔ Skin_GLTFJoints、program 識別で 10 UBO 個別 wire + cloudsF.glsl:79 複製併存 SPIR-V validation)
- L0-2 (= LLStaticHashedString UBO redirect 経路、setter redirect 形式利用)
- L0-3 (= per-shader UBO block 拡大、特に atmosphericsFuncs.glsl snippet shader (= `llviewershadermgr.cpp:854/957` literal で windlight consumer 全 program に link) は L0-3 拡大対象、FrameAtmosphere_Lighting per-shader 拡大 (= 現確認 6 file、残 sky/atmospheric/lighting shader) と同期)
- L0-4 (= cadence 妥当性、本 group cadence mismatch 重大 = SkyV.camPosLocal / CloudsV.camPosLocal / CloudsF.camPosLocal 重複 / StarsF.time / StarsV.stars_v_time / SunDiscF.blend_factor / StarsF.blend_factor は per-frame 性質を PerProgram で運ぶ stale data risk、PerFrame 降格候補多数、結果反映待ち)
- L1a-1 (= ClipFParamUBO_Legacy LLStaticHashedString redirect pilot 完了、同 pattern 利用)
- L1b-1 (= FrameViewProj per-shader UBO block 拡大完了、FrameAtmosphere_Lighting と同 per-shader 拡大経路)
- L1b-2 (= FrameLights、sun_dir/moon_dir/light array、本 group lightnorm/blend_factor day cycle 連動 source 共有、cross UBO source verify)
- L3-20 (= GlobalFParamUBO_Legacy mirror_flag/clipSign、本 group FrameAtmosphere_Lighting.cube_snapshot reflection probe regenerate 中 mode flag と関連)
- §3.5.1 group 完了 (= AYA r20 SSS skin、`skinSSSF.glsl` AtmoExtra 内同時 consume 経由交差、SkinSSS との data source 別系統だが同 shader file 内同時 consume 確認要)
- §3.5.2 group 完了 (= AtmoExtra `aya_visual_realism_enabled` offset=24 既起案、本 group 残 9 member 部分 write の前提として cross-UBO 同期 logic 確立、AYA r14/r16 cvar setter 経路は本 group で追加調査)

##### (2) 不明事項

**setter 経路不明**:
- FrameAtmosphere_Lighting 20 member の setter call site (= blue_horizon `llsettingsvo.cpp:1057` 確認済、他 19 member 全件 **[要追加調査]**、`LLShaderMgr::SUNLIGHT_COLOR` / `MOONLIGHT_COLOR` / `AMBIENT_COLOR` / `BLUE_DENSITY` / `HAZE_DENSITY` / `DENSITY_MULTIPLIER` / `DISTANCE_MULTIPLIER` / `MAX_Y` / `GLOW` / `SCENE_LIGHT_STRENGTH` 経路推定)
- FrameAtmosphere_Lighting sky_sunlight_scale / sky_ambient_scale / sky_hdr_scale / classic_mode / cube_snapshot / minimum_alpha / max_cof setter 経路 **[要追加調査]**
- AtmoExtra AYA r14/r16 cvar setter 経路 (= `aya_r14_volumetric_atmosphere_enabled` / `aya_r14_strength` / `aya_r16_aerial_perspective_enabled` / `aya_r16_strength`、AYAstorm 章別実装 grep 要) **[要追加調査]**
- SkyF 4 member setter 経路 (= hdri_split_screen / moisture_level / droplet_radius / ice_level) **[要追加調査]**
- CloudsV cloud_scale / cloud_color writer 経路 (= 推定 `llsettingsvo.cpp` 経由 `uniform1f`/`uniform3fv` で reserved name `cloud_scale`/`cloud_color`) **[要追加調査]**
- CloudsF cloud_pos_density1/2 / blend_factor / cloud_variance writer 経路 (= reserved name 登録済、writer site 全件 **[要追加調査]**)
- CloudsF AYA r18 cvar setter 経路 (= `aya_r18_cloud_volumetric_enabled` / `aya_r18_strength`、AYAstorm r18 章実装) **[要追加調査]**
- Stars F blend_factor / custom_alpha / time setter 経路 **[要追加調査]**
- Stars V stars_v_time setter 経路 (= F の `time` の rename、OpenGL setter は `uniform1f("time", ...)` か `uniform1f("stars_v_time", ...)` か **[要追加調査]**)
- SunDisc F blend_factor setter 経路 **[要追加調査]**
- camPosLocal writer 経路 (= SkyV + CloudsV + CloudsF 複製 + MaterialUBO_Legacy + materialF.glsl 共有、推定 `pipeline.cpp` per-frame 経路、全 5 use site 同一 data source 確認要) **[要追加調査 D4 突合]**

**cadence 妥当性 (= L0-4 結果反映待ち)**:
- SkyV.camPosLocal / CloudsV.camPosLocal / CloudsF.camPosLocal = per-frame 変化 (= camera move 毎)、PerProgram cadence で同 program 内 frame 間 stale data 表示 risk **[要 L0-4 結果反映 / 要 AYA 判断 = PerFrame 降格検討]**
- StarsF.time / StarsV.stars_v_time = per-frame 変化 (= twinkle animation)、PerProgram cadence stale risk **[要 L0-4 結果反映 / 要 AYA 判断 = PerFrame 降格検討]**
- StarsF.blend_factor / SunDiscF.blend_factor = day cycle 連続変化 (= per-frame)、PerProgram cadence stale risk **[要 L0-4 結果反映 / 要 AYA 判断 = PerFrame 降格検討]**
- FrameAtmosphere_Lighting.minimum_alpha / max_cof = post-process pass param、frame 内 stable ゆえ PerFrame 配置採用 (Phase 1.A AYA option)、verify 要 (= setter trigger 経路) **[要 verify]**

**cross-UBO data source 共有 verify**:
- camPosLocal 5 use site (= SkyV / CloudsV / CloudsF (複製) / MaterialUBO_Legacy / materialF.glsl) の同一 data source verify (= grep `camPosLocal` 7 件、全件 LLViewerCamera::getOrigin() local 変換経路同一か) **[要 verify D4 突合]**
- blend_factor 3 use site (= StarsF / SunDiscF / CloudsF) の同一 data source verify (= day/night blend 計算 vs cloud blend factor、別系統候補) **[要 verify D4 突合]**
- time / stars_v_time = 同 source rename か別 data source か (= StarsF `time` と StarsV `stars_v_time` の OpenGL setter 名前 verify) **[要 verify D4 突合]**

**binding 衝突 3 site**:
- AtmoExtra (set=3 binding=0、cadence=1 PerProgram) ↔ Asset_GLTFNodes (set=3 binding=0、cadence=3 PerAsset) = pipeline layout 上分離経路 (= subset 値違いで分離か別経路か) **[要 verify L0-1 dispatch protocol]**
- SkyV (set=3 binding=1、cadence=1 PerProgram) ↔ Asset_GLTFMaterials (set=3 binding=1、cadence=3 PerAsset) = 同上 **[要 verify L0-1]**
- SkyF (set=3 binding=2、cadence=1 PerProgram) ↔ Skin_GLTFJoints (set=3 binding=2、cadence=3 PerSkin) = 同上 (= skyF program は GLTF skin を必要としない推定、verify 要) **[要 verify L0-1]**

**複製併存 risk**:
- cloudsV.glsl + cloudsF.glsl が CloudsVParamUBO_Legacy set=3 binding=3 両 stage 参照、Vulkan validation 0 件確認 **[要 verify]**
- atmosphericsFuncs.glsl snippet shader (= windlight consumer 全 program に link) の per-shader UBO block 拡大対象範囲 (= AtmoExtra + FrameAtmosphere_Lighting 同時宣言、影響 program 数 verify 要) **[要追加調査]**

**bare uniform 残存**:
- 10 shader `#else` block bare uniform setter が OpenGL 経路で host C++ 側に残存しているか **[要追加調査]**

**AYAstorm 視覚機能交差**:
- §3.5.1 (= r20 SSS skin) との交差 = AtmoExtra が skinSSSF.glsl 内同時 consume 経由、本 group AtmoExtra 残 9 member trigger 時の SSS pipeline 影響 verify 要 **[要 verify]**
- §3.5.2 sub-cluster (a) (= visual_realism cross-write) との交差 = 同 AtmoExtra UBO 内 member 別 dirty、本 group が `aya_visual_realism_enabled` を読まないが skinSSSF.glsl が読む ↔ 本 group trigger で skinSSSF.glsl 内 stale data risk 検証 **[要 verify]**

##### (3) 調査手法

- **D1 setter Grep**: 全 reserved uniform name (= `BLUE_HORIZON` / `SUNLIGHT_COLOR` / `MOONLIGHT_COLOR` / `AMBIENT_COLOR` / `BLUE_DENSITY` / `HAZE_DENSITY` / `DENSITY_MULTIPLIER` / `DISTANCE_MULTIPLIER` / `MAX_Y` / `GLOW` / `SCENE_LIGHT_STRENGTH` / `LIGHTNORM` / `HAZE_HORIZON` / `CLOUD_SHADOW` / `SUN_MOON_GLOW_FACTOR` / `CLOUD_POS_DENSITY1` / `MOON_BRIGHTNESS`) setter site 全件 (= `indra/newview/llsettingsvo.cpp` + `indra/newview/lldrawpoolwlsky.cpp` + `indra/newview/pipeline.cpp` + `indra/newview/lldrawpoolwater.cpp` 主要、推定 50+ site)
- **D1 setter Grep AYA cvar**: `aya_r14_volumetric_atmosphere` / `aya_r14_strength` / `aya_r16_aerial_perspective` / `aya_r16_strength` / `aya_r18_cloud_volumetric` / `aya_r18_strength` cvar 経路 (= AYAstorm 章別実装、`indra/newview` + `LLCachedControl` + settings.xml AYA* prefix grep)
- **D2 既存実装読解**: AYAstorm r14/r16/r18 視覚表現章実装 (= memory `project_ayastorm_r14_pivot_to_light` + `project_ayastorm_visual_realism_chapter`)、章別実装 hook 位置特定
- **D3 cadence verify**: camPosLocal 5 use site / time 2 use site / blend_factor 3 use site の PerProgram cadence stale risk 実測 (= 同 program 内 multi-frame 描画で stale data 発生有無)
- **D4 突合**:
  - camPosLocal 5 use site 同 data source verify (= LLViewerCamera::getOrigin() local 変換経路、SkyV/CloudsV/CloudsF(複製)/MaterialUBO_Legacy/materialF.glsl 全件 同一)
  - blend_factor 3 use site 同 data source verify (= day/night blend 計算 vs cloud blend factor、別系統候補確認)
  - time/stars_v_time data source 共有 verify (= 同 per-frame time accumulator か別 source か)
  - AtmoExtra 9 member 部分 write logic (= §3.5.2 group `aya_visual_realism_enabled` offset=24 と member 別 dirty 粒度設計、本 group 9 member trigger と非干渉性)

##### (4) 設計 task (= 4 経路、group 単位)

- **register**: 10 UBO 個別 register、program 識別で per-program cadence triple-buffer + FrameAtmosphere_Lighting は PerFrame frame 単位 register (= 既 shell 通電済 + write 経路本格化済)
- **write**: 4 trigger 経路で N UBO 同時 dirty:
  - **trigger 1 sky preset 切替 (= LLSettingsSky 切替、LLEnvironment::onSettingsChanged)**: FrameAtmosphere_Lighting 全 20 member + AtmoExtra 9 member (lightnorm/haze_horizon/cloud_shadow/sun_moon_glow_factor) + SkyF 4 member + CloudsV cloud_scale/cloud_color + CloudsF cloud_pos_density1/2/blend_factor/cloud_variance + Moon moon_brightness = 多 UBO cross dirty (= 推定 7+ UBO)
  - **trigger 2 day cycle update (= per-frame、blend_factor/time 連続変化)**: StarsF blend_factor/time + StarsV stars_v_time + SunDiscF blend_factor + CloudsF blend_factor + AtmoExtra lightnorm/sun_moon_glow_factor (= 6 UBO、PerFrame cadence 候補に降格すれば cross UBO 同期不要、L0-4 結果反映待ち)
  - **trigger 3 camera move (= per-frame、camPosLocal 共有)**: SkyV + CloudsV + CloudsF (複製) + MaterialUBO_Legacy + materialF.glsl 5 use site cross dirty (= 全 use site 同一 source verify 前提、PerFrame cadence 降格候補)
  - **trigger 4 AYA r14/r16/r18 cvar 変化**: AtmoExtra aya_r14_*/aya_r16_* + CloudsF aya_r18_* member dirty (= cvar setter site 特定後 1 setter call → 該当 member 部分 write)
- **flush**: 各 UBO 個別 (= 該当 program bind 単位 + FrameAtmosphere_Lighting frame 開始時 set=0 全 4 UBO 同時 bind 経路、cmdbuf 経路で triple-buffer 経由 `vkCmdBindDescriptorSets`)
- **shader 接続**: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= FrameAtmosphere_Lighting `lightAlphaMaskF.glsl:37-45` + `lightAlphaMaskNonIndexedF.glsl:37-45` + 4 sample sites、AtmoExtra 4 shader、SkyV `skyV.glsl:83`、SkyF `skyF.glsl:117`、CloudsV `cloudsV.glsl:106` + `cloudsF.glsl:79` 複製、CloudsF `cloudsF.glsl:61`、StarsF `starsF.glsl:57`、StarsV `starsV.glsl:67`、SunDiscF `sunDiscF.glsl:48`、MoonF `moonF.glsl:67`、改変ゼロ、原則 4 維持)
- **cross-UBO 同期 protocol**: 4 trigger event で group 内該当 UBO 全 dirty (= host 側 dispatch logic で 4 trigger setter を受けて該当 program 系全 dirty bit を立てる、RELATIONS.md §5.1 trigger 実装、特に sky preset 切替 = 7+ UBO 一括 dirty が最大規模)
- **AtmoExtra 部分 write 設計**: AtmoExtra UBO 内 `aya_visual_realism_enabled` offset=24 (= 1 member) は §3.5.2 group trigger で write、本 group では他 9 member (= lightnorm offset=0 / haze_horizon offset=12 / cloud_shadow offset=16 / sun_moon_glow_factor offset=20 / aya_r14_* offset=28-32 / aya_r16_* offset=36-40 / pad offset=44) を別 trigger で部分 write → 同 UBO 内 member 別 dirty 粒度設計必須 (= §3.5.1 MaterialUBO_Legacy 部分 write 設計と同型)
- **FrameAtmosphere_Lighting 部分 write 設計**: 20 member を 4 trigger で部分 write (= sky preset 切替で 14 member / day cycle で lightnorm 等 / post-process pass で minimum_alpha/max_cof / mode flag classic_mode/cube_snapshot で reflection probe regenerate 時)、同 UBO 内 member 別 dirty 粒度設計必須
- **複製併存 cloudsV/cloudsF.glsl:79**: CloudsVParamUBO_Legacy set=3 binding=3 を vertex/fragment 両 stage で参照、blueprint コメント `verified identical` 担保 + pipeline layout で両 stage visibility 有効化 + Vulkan validation 0 件確認

##### (5) 工程 task

- trace 順内位置: L4 group 7 件目 = 最大 group (= 10 UBO)、AYA 既存機能 risk 大 (= AYA r14/r16/r18 + r20 SSS 交差) + binding 衝突 3 site + 複製併存 1 site + cadence mismatch 6 候補 + PerFrame/PerProgram 混在 (= L0-4 再評価依存度 highest) → §3.5.2 + §3.5.6 完了後着手推奨
- group 内 並列性:
  - sub-cluster (a) FrameAtmosphere_Lighting = 単独 PerFrame、独立着手可
  - sub-cluster (b) AtmoExtra 9 member = §3.5.2 sub-cluster (a) 完了前提 (= member 別 dirty 粒度設計が §3.5.2 で確立)
  - sub-cluster (c) Sky V/F pair = binding=1/2 衝突 L0-1 dispatch 確立前提
  - sub-cluster (d) Clouds V/F + 複製 = cloudsF.glsl:79 SPIR-V validation + camPosLocal cross UBO 整合前提
  - sub-cluster (e) Stars + SunDisc + Moon = day cycle blend_factor/time cross UBO 整合 + MoonF setter 特定済ゆえ最先着手可
- group 間 並列性: §3.5.3 (shadow_target_width) / §3.5.4 (occlusion+shadow cube) / §3.5.5 (GLTF transform) / §3.5.8 (velocity) / §3.5.10 (post-process)〜§3.5.16 と並列可、ただし §3.5.1 (r20 SSS skin) + §3.5.2 (visual_realism cross-write) との SkinSSS/AtmoExtra 経由交差は前提解消必須
- 推定工数: **L** (= 1 日 +、本 group 最大、setter site 50+ 全件特定 + 10 UBO 同期 protocol + 5 sub-cluster cross UBO 整合 + 4 trigger 全件 verify + cadence 再評価 + AYA r14/r16/r18 cvar 経路特定 + binding 衝突 3 site verify + 複製併存 SPIR-V validation + 10 path visual verify)

##### (6) A 確定条件

- mUseUBO ON + 10 shader 活性化 + setter 通電 (= 10 UBO 全件、4 trigger 経路全件)
- AYA live verify (= 全 path visual regression ゼロ §5.4):
  - **sky preset 全件** (= LLSettingsSky 全 preset 切替で 7+ UBO 同期描画整合、windlight 旧 preset + EEP day cycle 両 path)
  - **day cycle 連続変化** (= sunrise / midday / sunset / midnight 4 段階 + 中間連続変化、blend_factor/time/lightnorm/sun_moon_glow_factor 経路で stars/sun/moon/sky/cloud 描画 stale 無し)
  - **AYA r14 volumetric atmosphere cvar ON/OFF** (= AYAstorm 視覚表現章 r14 機能維持、`aya_r14_volumetric_atmosphere_enabled` + strength 連動)
  - **AYA r16 aerial perspective cvar ON/OFF** (= AYAstorm r16 機能維持、`aya_r16_aerial_perspective_enabled` + strength)
  - **AYA r18 volumetric clouds cvar ON/OFF** (= AYAstorm r18 機能維持、`aya_r18_cloud_volumetric_enabled` + strength)
  - **AYA r20 SSS skin との交差** (= skinSSSF.glsl 内 AtmoExtra 同時 consume で SSS 描画維持、§3.5.1 group verify と並走)
  - **camera move 連続変化** (= camPosLocal 5 use site 同期、sky/cloud 描画追随確認)
  - **reflection probe regenerate 時** (= classic_mode/cube_snapshot mode flag 連動、reflection probe 再生成中の sky/cloud 描画整合)
- Vulkan validation 0 件 (= 10 UBO + binding 衝突 3 site descriptor set 内分離 + cloudsV/cloudsF.glsl:79 複製併存 SPIR-V validation + AtmoExtra 9 member 部分 write + FrameAtmosphere 20 member 部分 write)
- cross-UBO 同期 verify (= 4 trigger 全件で N UBO 全 dirty + 値同期、特に sky preset 切替 7+ UBO 一括 dirty 連動性 + day cycle per-frame blend_factor 6 UBO 連続変化 + camPosLocal 5 use site 同 data source + AYA cvar 変化 4 member 即時反映)
- bare uniform 残存ゼロ確認 (= 10 shader `#else` block bare uniform setter が host C++ 側に残存しない)
- cadence 再評価結果反映 (= L0-4 結果反映、PerProgram → PerFrame 降格候補 6 件 = SkyV/CloudsV/CloudsF camPosLocal + StarsF time + StarsV stars_v_time + StarsF/SunDiscF blend_factor、AYA 判断)
- AtmoExtra 部分 write logic verify (= 本 group 9 member write が §3.5.2 sub-cluster (a) `aya_visual_realism_enabled` を無効化しない、別 group trigger との独立性確認)
- FrameAtmosphere_Lighting 部分 write logic verify (= 20 member を 4 trigger で部分 write、相互 dirty 干渉ゼロ)
- **verify 単位 = group verify** (= 10 UBO 揃って初めて整合 visual、中間状態 (= 1 UBO のみ通電) は暫定 default 値 (sky/cloud 黒抜け or 単色描画継続) で破綻回避、AYA literal「現状の見た目とほぼ変わらない描画」継承)

##### (7) 4 原則 gate

- **原則 1 (Core 分散)**: 10 program 個別 = render thread 単独 → ✅、cross-UBO 同期 logic は 4 trigger setter call に集約 (= sky preset / day cycle / camera move / AYA cvar)、各 trigger 内で 10 UBO write 並列化候補 = 将来 Core 化で各 UBO 別 thread で write 可能、設計原則 (2) Core 分散実現整合 (memory `project_ayastorm_r41_design_principles`)
- **原則 2 (3 OS 共通)**: ✅ (= Vulkan core spec 1.3 範囲内、binding 衝突解決 + 複製併存は MoltenVK Argument Buffer Tier 2 制約整合)
- **原則 3 (Phase 2/3)**: Phase 2 内、L0-4 cadence 再分類は PerProgram → PerFrame 降格選択 6 候補多数 → ✅
- **原則 4 (OpenGL を殺さない)**: 10 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能、複製併存 cloudsF.glsl:79 も `#else` block で bare uniform 並走 → ✅
- **visual regression ゼロ**: sky/cloud/atmospheric/stars/sun/moon 描画同一 + AYAstorm r14/r16/r18/r20 視覚表現章機能維持 → AYA live verify (= sky preset 全件 + day cycle 連続 + AYA cvar ON/OFF + camera move)、AYAstorm 視覚表現章機能維持必須 (= memory `project_ayastorm_visual_realism_chapter` + `project_ayastorm_r14_pivot_to_light` 整合、AYAstorm 視覚機能交差最大 group)

---

#### §3.5.8 L4-8: velocity / motion blur 6 UBO group (= curr/prev cadence pair + last_object_matrix per-draw mismatch)

**AYA literal 命名 mapping**: READINESS §3.8 (= velocity/motion blur pair UBO 35-40 = 6 UBO、AYA 単純配列で C-35〜C-40 相当、handoff §3.1 = 5 UBO 計上は VelocityV/VelocityAlphaV pair 統合カウント、本起案は 6 UBO 個別、READINESS 一致)

**位置付け**: L4 group 8 件目 = motion blur / TAA velocity buffer 経路直結、curr/prev pair 同期 protocol + first-frame fallback logic (= lightning-streak velocity 回避) + 全 UBO 中最大 size (= PerDrawUBO_ObjectSkin 10752 B) + set=2 binding=0 共有 4 UBO (= AvatarSkin/AvatarVelocity/ObjectSkin/SkinnedVelocity) + PerProgram cadence mismatch 重大 2 件 (= VelocityV / VelocityAlphaV、`last_object_matrix` per-draw 性質を PerProgram で運ぶ → PerDraw 降格必須候補) + data duplication risk (= ObjectSkin.lastMatrixPalette ↔ SkinnedVelocity.lastMatrixPalette_skinned_velocity 同 data) + ring buffer 容量制約 (= ObjectSkin 10752 + SkinnedVelocity 5376 = 16128 B/draw 大型、`maxUniformBufferRange` 16384 B 範囲内だが境界近接)

**group 概要** (= 3 sub-cluster 6 UBO、cross-UBO 同期 protocol 4 trigger 由来):

**sub-cluster (a) avatar curr/prev pair 2 UBO** (= avatar bone palette frame swap、lightning-streak fallback 必須):
- **PerDrawUBO_AvatarSkin** (set=2 binding=0、768B、std140 720B、matrixPalette[45] vec4×45 stride=16、curr frame avatar bone matrix palette) — `class1/avatar/avatarSkinV.glsl:45` singleton site、setter 3 site `lldrawpool.cpp:701/737/775` literal `LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX, count, false, (GLfloat*)&(mpc.mGLMp[0]));` (= 3 overload `LLRenderPass::uploadMatrixPalette`)
- **PerDrawUBO_AvatarVelocity** (set=2 binding=0、768B、std140 720B、lastMatrixPalette[45] vec4×45 stride=16、prev frame avatar bone matrix palette) — `class1/deferred/avatarVelocityV.glsl:53` singleton site、setter `lldrawpool.cpp:1021` literal `uniformMatrix3x4fv(LLShaderMgr::AVATAR_LAST_MATRIX, count, false, (GLfloat*)&(src[0]));`
- data source:
  - AvatarSkin = `LLVOAvatar::MatrixPaletteCache::mGLMp` (= `lldrawpool.cpp:704` literal `(GLfloat*)&(mpc.mGLMp[0])`、`std::vector<F32>` 720 B)
  - AvatarVelocity = `LLVOAvatar::MatrixPaletteCache::mLastGLMp` (= 前 frame の bone matrix snapshot、frame 跨ぎ curr frame → next frame `mLastGLMp`)
- **first-frame fallback logic** (= `lldrawpool.cpp:1015` literal `const std::vector<F32>& src = mpc.mLastGLMp.empty() ? mpc.mGLMp : mpc.mLastGLMp;`、`lldrawpool.cpp:1009-1015` comment 「Upload mGLMp instead so last_pose == curr_pose → velocity = 0, the correct 'no motion captured yet' answer」):
  - **必須遵守**: `mLastGLMp.empty()` 時に `mGLMp` (= curr) を入れる、無視すると前回 rig の bone が漏れて motion blur で lightning-streak velocity 視覚 bug 発生
  - **`motionBlurF.glsl:145` comment 警告**「avatar lastMatrixPalette uninitialized → NaN, per-frame matrix」= upload skip すると undefined behavior
- dedup logic (= 既存 OpenGL 経路、`lldrawpool.cpp:723` literal `if (avatar == lastAvatar && skinInfo->mHash == lastMeshId)` skip)、ring buffer 経路でも維持必須 (= 同 hash 再 upload 回避で frame 当 bandwidth 削減)
- reserved 登録: AvatarSkin `llshadermgr.cpp:1734` "matrixPalette" + `llshadermgr.h:246` AVATAR_MATRIX enum / AvatarVelocity `llshadermgr.cpp:1878` "lastMatrixPalette" + `llshadermgr.h:397` AVATAR_LAST_MATRIX enum (= `llshadermgr.h:386` literal 「Imported from BlackDragon Viewer 995a1354d8」)

**sub-cluster (b) object curr/prev pair + 抽出版 2 UBO** (= attachment object bone palette、data duplication 解消候補):
- **PerDrawUBO_ObjectSkin** (set=2 binding=0、10752B std140 10560B、**全 UBO 中最大 size**、2 member = `matrixPalette[110]` mat3x4 stride=48 offset=0 size=5280 + `lastMatrixPalette[110]` mat3x4 stride=48 offset=5280 size=5280) — `class1/avatar/objectSkinV.glsl:43` singleton site、setter 不明 [要追加調査]
- **PerDrawUBO_SkinnedVelocity** (set=2 binding=0、5376B std140 5280B、1 member = `lastMatrixPalette_skinned_velocity[110]` mat3x4 stride=48 offset=0、prev frame のみ抽出版 = ObjectSkin.lastMatrixPalette 同 data) — `class1/deferred/skinnedVelocityV.glsl:81` + `class1/deferred/skinnedVelocityAlphaV.glsl` 2 file consume (= blueprint コメント `verified identical across 2 sample sites`)、setter 不明 [要追加調査]
- data source:
  - ObjectSkin.matrixPalette = `LLMeshSkinInfo` 経由 bone matrix + `LLVOAvatar::mRiggedJointMatrix` curr frame palette (= 推定、verify 要)
  - ObjectSkin.lastMatrixPalette = prev frame palette (= velocity computation 用)
  - SkinnedVelocity.lastMatrixPalette_skinned_velocity = **ObjectSkin.lastMatrixPalette と同 data**、別 UBO export 形式 (= data duplication)
- MAX_JOINTS_PER_MESH_OBJECT = 110 (= `indra/llcharacter/lljoint.h:48` `LL_MAX_JOINTS_PER_MESH_OBJECT` literal、`llviewershadermgr.cpp:870/3353/3383` 全 shader inject)
- **data duplication 罠**: ObjectSkin.lastMatrixPalette (5280B) + SkinnedVelocity.lastMatrixPalette_skinned_velocity (5280B) = 同 data を 2 UBO 別 binding 配置で host 側 2 回書込 = 5280B × 2 = 10560 B/draw 浪費 risk。統合 candidate = SkinnedVelocity shader が ObjectSkin UBO 直接 consume (= shader 改修必要、別案 [要 AYA 判断])
- **ring buffer 容量 risk**: ObjectSkin 10752 B = `maxUniformBufferRange` Vulkan minimum guarantee 16384 B (= 16 KiB) 範囲内だが境界近接、rigged draw 多発 scene で `sDrawUboRingBufferMgr` 容量 verify 必須 [要 verify]
- **cadence 再分類検討**: PerDraw → PerSkin / PerProgram 移行 (= Skin_GLTFJoints (set=3 binding=2 cadence=4 PerSkin 16384B) 同等戦略候補) [要 AYA 判断]

**sub-cluster (c) per-program velocity matrix pair 2 UBO** (= cadence mismatch 重大、PerDraw 移行必須候補):
- **VelocityVParamUBO_Legacy** (set=3 binding=55、256B std140 64B、1 member = `last_object_matrix` mat4 offset=0) — `class1/deferred/velocityV.glsl:54` singleton site、setter 不明 [要追加調査]、cadence PerProgram (= ubo_metadata.inl:116 cadence_tag=1)
- **PerProgramUBO_VelocityAlphaV** (set=2 binding=19、256B std140 64B、1 member = `last_object_matrix` mat4 offset=0) — `class1/deferred/velocityAlphaV.glsl:60` singleton site、setter 4 site (= `lldrawpool.cpp:845/934` + `lldrawpooltree.cpp:202` + `lldrawpoolterrain.cpp:248` literal `LLGLSLShader::sCurBoundShaderPtr->uniformMatrix4fv(LLShaderMgr::LAST_OBJECT_MATRIX, 1, GL_FALSE, (GLfloat*)last_mat->mMatrix);`)、cadence PerProgram (= ubo_metadata.inl:90 cadence_tag=1)
- data source: `LLMatrix4a` previous frame object model matrix (= `lldrawpool.cpp:807` literal comment `via the LAST_OBJECT_MATRIX uniform (set by Step 3 enum), draw, then store`)
- reserved 登録: `llshadermgr.cpp:1876` "last_object_matrix" + `llshadermgr.h:395` LAST_OBJECT_MATRIX enum literal 確認
- **cadence mismatch 重大 risk**: `last_object_matrix` は **per-draw 性質** (= 同 program 内 N object 描画で N 値変化)、PerProgram cadence で運ぶと最後の object の matrix のみ反映 = **motion blur 全 object 同 last_matrix 使用 = 誤描画 / motion blur 退化** (= 重大 risk、verify 要 D3)、解決策 = PerProgram → PerDraw 移行 (= ubo_metadata.inl cadence_tag=1 → cadence_tag=2 PerDraw 変更 + set/binding 再配置 set=2 binding=0 共有 6 UBO 追加 or 別 binding) **[要 AYA 判断 = PerDraw 降格必須候補]** [要 L0-4 結果反映]
- 4 setter site cross-pool 共有 (= drawpool / tree / terrain)、Vulkan 化後は 4 site 全て UBO write 経由必須
- **velocityV.glsl vs velocityAlphaV.glsl 別 UBO 別 binding**:
  - VelocityV → set=3 binding=55 (Legacy 帯)、PerProgram
  - VelocityAlphaV → set=2 binding=19 (PerDraw + PerProgram 混在帯)、PerProgram
  - = 同 data `last_object_matrix` を別 2 UBO で持つ duplicate (= 別 program 別 path、velocity vs velocityAlpha 分離) [要 verify D4 突合]
- shader comment literal `velocityAlphaV.glsl:54-55`: `last_object_matrix mat4 を UBO 化。velocityAlphaF は plain uniform 無し、skinnedVelocityAlphaV は last_object_matrix 不使用 (skin matrix 経由)、本 V 単独 attach`

##### (1) 前提条件

- L0-1 (= dispatch logic、特に set=2 binding=0 共有 6 UBO の 4 件 (= AvatarSkin/AvatarVelocity/ObjectSkin/SkinnedVelocity) program 識別、program 毎に異なる UBO レイアウトで write、誤った layout で write すると skinning 崩壊)
- L0-2 (= LLStaticHashedString UBO redirect 経路、setter redirect 形式利用)
- L0-4 (= cadence 妥当性、特に VelocityV / VelocityAlphaV PerProgram → PerDraw 降格 = cadence mismatch 解消必須、結果反映待ち、本 group は L0-4 依存度最大 group の 1 つ)
- L1a-1 (= ClipFParamUBO_Legacy LLStaticHashedString redirect pilot 完了、同 pattern 利用)
- L1b-1 (= FrameViewProj 配信、velocity shader は view+proj matrix 同時 consume、特に velocityAlphaV `vec4 last_pos = projection_matrix * last_modelview_matrix * last_object_matrix * vec4(position.xyz, 1.0)` 経路)
- L3-20 (= GlobalFParamUBO_Legacy mirror_flag/clipSign、cube_snapshot mode flag 経由連動候補)
- L4-1 / L4-2 / L4-5 完了 (= skin matrix 関連 UBO 設計の前提として r20 SSS + visual_realism + GLTF transform 確立、本 group は skin matrix curr/prev 純粋系)
- Phase 1.B PC-6β (= `sDrawUboRingBufferMgr` ring buffer infrastructure 配線済) + Phase 1.E PC-N-15a (= per-thread `mDrawUboRingBuffer` 配線済、thread 競合解消)

##### (2) 不明事項

**setter 経路不明**:
- ObjectSkin matrixPalette / lastMatrixPalette setter (= 推定 `LLVOAvatar::computeJointMatrices` 等候補) **[要追加調査]**
- SkinnedVelocity lastMatrixPalette_skinned_velocity setter (= velocity shader 用、ObjectSkin と同 host 経路 2 UBO 同時書込み候補) **[要追加調査]**
- VelocityV last_object_matrix setter (= `uniformMatrix4fv("last_object_matrix", ...)` grep verify 要) **[要追加調査]**

**cadence 妥当性 (= L0-4 結果反映待ち)**:
- VelocityV cadence PerProgram → PerDraw 降格必須 (= per-object per-frame 変化を PerProgram で運ぶ stale data) **[要 L0-4 結果反映 / 要 AYA 判断]**
- VelocityAlphaV cadence PerProgram → PerDraw 降格必須 (= 同上、4 setter site cross-pool 共有) **[要 L0-4 結果反映 / 要 AYA 判断]**
- ObjectSkin / SkinnedVelocity cadence PerDraw → PerSkin 昇格検討 (= Skin_GLTFJoints (cadence=4 PerSkin 16384B) 同等戦略、bone palette は skin 単位 stable + frame 単位更新ゆえ PerSkin 適合候補) **[要 AYA 判断]**

**data duplication**:
- ObjectSkin.lastMatrixPalette (5280B) ↔ SkinnedVelocity.lastMatrixPalette_skinned_velocity (5280B) 同 data **[要 AYA 判断 = 統合 vs 維持]**
- VelocityV.last_object_matrix ↔ VelocityAlphaV.last_object_matrix 同 data 別 UBO **[要 verify D4 突合 + 要 AYA 判断]**

**ring buffer 容量**:
- ObjectSkin 10752 B = `maxUniformBufferRange` Vulkan minimum 16384 B 範囲内だが境界近接 **[要 verify]**
- rigged draw 多発 scene (= AYAstorm 視覚機能交差 = avatar 多数 + 装着物多数) で ring buffer 容量実測 **[要 verify]**

**dispatch logic**:
- set=2 binding=0 共有 6 UBO の program 識別 logic (= avatar mesh program → AvatarSkin/AvatarVelocity、object mesh program → ObjectSkin/SkinnedVelocity、clip plane program → ClipPlane、light program → LightParams 等) **[要 verify L0-1]**
- avatar mesh と attachment object mesh の bind 順序 (= AvatarSkin + AvatarVelocity 同 draw call で 2 UBO 同時 bind は不可、別 draw call 推定) **[要 verify]**

**first-frame fallback ring buffer 経路**:
- `mLastGLMp.empty() ? mGLMp : mLastGLMp` 分岐を ring buffer write 側でどう移植するか (= ring buffer slot 取得後 memcpy 前に分岐) **[要 verify]**

**dedup logic ring buffer 整合性**:
- `(avatar, mHash)` dedup で write skip するか、常に新 slot 取得か (= bandwidth 削減 vs slot 不足) **[要 AYA 判断]**

**bone count packing**:
- AvatarSkin `vec4[45]` 解釈 (= avatarSkinV.glsl:59-61 `i+0/i+15/i+30` index pattern → 15 bone × 3 row layout、`LL_CHARACTER_MAX_ANIMATED_JOINTS` との整合) **[要 verify]**
- ObjectSkin/SkinnedVelocity `mat3x4[110]` packing (= column_major 3 vec4、host 側 mat4 → mat3x4 投影変換、bottom row drop) **[要 verify]**

**bare uniform 残存**:
- 6 shader `#else` block bare uniform setter 全件 (= AvatarSkin matrixPalette / AvatarVelocity lastMatrixPalette / ObjectSkin matrixPalette+lastMatrixPalette / SkinnedVelocity lastMatrixPalette_skinned_velocity / VelocityV last_object_matrix / VelocityAlphaV last_object_matrix) host C++ 側残存 **[要追加調査]**

##### (3) 調査手法

- **D1 setter Grep**: 全 reserved uniform name (= `AVATAR_MATRIX` / `AVATAR_LAST_MATRIX` / `LAST_OBJECT_MATRIX`) + bare member name (= `matrixPalette` / `lastMatrixPalette` / `lastMatrixPalette_skinned_velocity` / `last_object_matrix`) setter site 全件 (= `indra/newview/lldrawpool*.cpp` + `pipeline.cpp` + `lldrawpoolavatar.cpp` 主要、推定 10+ site)
- **D2 既存実装読解**: motion blur pipeline (= `motionBlurF.glsl` + velocity buffer 経路 = TAA / deferred motion blur) 既存実装 + BlackDragon AVATAR_LAST_MATRIX import 経緯 (= `llshadermgr.h:386` literal 「Imported from BlackDragon Viewer 995a1354d8」)
- **D3 cadence verify**: VelocityV / VelocityAlphaV cadence mismatch 実測 (= 同 program 内 N object 描画で last_object_matrix 値変化と PerProgram cadence stale risk)
- **D4 突合**:
  - ObjectSkin.lastMatrixPalette ↔ SkinnedVelocity.lastMatrixPalette_skinned_velocity 同 data verify (= 1 host cache から 2 UBO 同時書込み or 統合可能性)
  - VelocityV.last_object_matrix ↔ VelocityAlphaV.last_object_matrix 同 data verify (= LLDrawable previous frame matrix tracking 共通)
  - AvatarSkin.matrixPalette ↔ ObjectSkin.matrixPalette (= avatar bone vs object bone、別 LLVOAvatar bone vs LLMeshSkinInfo skin、別系統 verify)

##### (4) 設計 task (= 4 経路、group 単位)

- **register**: 6 UBO 個別 register、set=2 binding=0 共有 4 UBO は program 識別で wrap (= avatar mesh → AvatarSkin/AvatarVelocity / object mesh → ObjectSkin/SkinnedVelocity / clip plane → ClipPlane / light → LightParams = §3.5.15 group)、set=2 binding=19 = VelocityAlphaV、set=3 binding=55 = VelocityV
- **write**: 4 trigger 経路で N UBO 同時 dirty:
  - **trigger 1 bone animation frame swap (= per-frame、avatar curr/prev pair)**: AvatarSkin (curr) + AvatarVelocity (prev) 同時 dirty、first-frame fallback `mLastGLMp.empty() ? mGLMp : mLastGLMp` 適用、dedup `(avatar, mHash)` 適用
  - **trigger 2 bone animation frame swap (= per-frame、object curr/prev pair)**: ObjectSkin (curr+prev 統合) + SkinnedVelocity (prev 抽出版) 同時 dirty、data duplication 解消候補 (= 1 host cache → 2 UBO 同時書込み or 統合)
  - **trigger 3 per-draw object 切替 (= per-draw、last_object_matrix per-draw 性質)**: VelocityV + VelocityAlphaV 同時 dirty (= 同 data 別 UBO、cadence mismatch 解消必須 = PerDraw 降格 + set=2 binding=0 or 別 binding 再配置)
  - **trigger 4 motion blur enable 時**: 6 UBO velocity 系全体 dirty 必須 (= TAA / deferred motion blur pipeline 起動時)
- **flush**: 各 UBO 個別 (= 該当 program/draw 単位、cmdbuf 経路で `sDrawUboRingBufferMgr` ring buffer 経由 + dynamic offset bind `vkCmdBindDescriptorSets` set=2 binding=0)、VelocityV は set=3 binding=55 個別 bind
- **shader 接続**: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= 6 shader、AvatarSkin `avatarSkinV.glsl:45` / AvatarVelocity `avatarVelocityV.glsl:53` / ObjectSkin `objectSkinV.glsl:43` / SkinnedVelocity `skinnedVelocityV.glsl:81` + `skinnedVelocityAlphaV.glsl` 2 file / VelocityV `velocityV.glsl:54` / VelocityAlphaV `velocityAlphaV.glsl:60`、改変ゼロ、原則 4 維持)
- **cross-UBO 同期 protocol**: 4 trigger event で group 内該当 UBO 全 dirty (= host 側 dispatch logic で 4 trigger setter を受けて該当 program 系全 dirty bit を立てる、RELATIONS.md §5.1 trigger 実装)
- **first-frame fallback ring buffer 経路**: `mLastGLMp.empty()` 分岐を ring buffer slot 取得後 memcpy 前に適用、lightning-streak velocity 回避担保
- **data duplication 解消設計**: ObjectSkin.lastMatrixPalette / SkinnedVelocity.lastMatrixPalette_skinned_velocity 統合候補 = (i) 維持 (= 2 UBO 別 binding で host 側 2 回書込) / (ii) SkinnedVelocity shader が ObjectSkin UBO 直接 consume (= shader 改修必要、原則 4 整合) / (iii) 別案 **[要 AYA 判断]**
- **cadence 再分類設計**: VelocityV / VelocityAlphaV PerProgram → PerDraw 降格 (= ubo_metadata.inl cadence_tag=1 → cadence_tag=2、set/binding 再配置、ring buffer 経路移行) **[要 AYA 判断 必須]**

##### (5) 工程 task

- trace 順内位置: L4 group 8 件目 = §3.5.7 に次ぐ大型 group (= 6 UBO + 全 UBO 中最大 size + cadence mismatch 重大 + data duplication + ring buffer 容量境界)、§3.5.7 完了後着手推奨
- group 内 並列性:
  - sub-cluster (a) avatar curr/prev = AvatarSkin + AvatarVelocity 並列開発候補 (= 同 size 同 layout 同 cadence)、first-frame fallback logic 共有
  - sub-cluster (b) object curr/prev = ObjectSkin + SkinnedVelocity 並列開発候補、data duplication 解消設計確定後
  - sub-cluster (c) per-program velocity = VelocityV + VelocityAlphaV 並列開発候補、cadence 再分類確定後
- group 間 並列性: §3.5.9 (reflection probe/IBL) / §3.5.10 (post-process) / §3.5.11 (glow) / §3.5.12 (SMAA) / §3.5.13 (pathfinding) / §3.5.14 (GLTF asset) / §3.5.15 (set=2 binding=0 共有残) / §3.5.16 (MultiLight) と並列可、ただし §3.5.15 group ClipPlane/LightParams (= 同 set=2 binding=0 共有 6 UBO の残 2 件) と L0-1 dispatch 経路共有
- 推定工数: **L** (= 1 日 +、6 UBO + cadence 再分類設計 + data duplication 解消設計 + first-frame fallback ring buffer 移植 + ring buffer 容量 verify + 6 path visual verify + lightning-streak velocity 回避 verify + AVATAR_MATRIX/AVATAR_LAST_MATRIX/LAST_OBJECT_MATRIX setter 10+ site UBO 化)

##### (6) A 確定条件

- mUseUBO ON + 6 shader 活性化 + setter 通電 (= 6 UBO 全件、4 trigger 経路全件)
- AYA live verify (= 全 path visual regression ゼロ §5.4):
  - **avatar bone animation** (= avatar walk/run/sit/emote、curr/prev 同期描画、AvatarSkin + AvatarVelocity 連動)
  - **attachment rigged mesh** (= avatar 装着 rigged object、ObjectSkin + SkinnedVelocity 連動、110 joint mesh)
  - **motion blur enable** (= velocity buffer 完全性、lightning-streak velocity ゼロ、avatar/object 連続 motion 描画整合)
  - **first-frame display** (= 新 avatar/mesh 出現時の初 frame、`mLastGLMp.empty()` fallback で velocity=0 描画、lightning-streak 回避)
  - **per-draw object 切替** (= 同 program 内 N object 描画で last_object_matrix per-object 正確、motion blur 退化ゼロ、cadence 再分類後 verify)
  - **dedup logic 効果** (= 同 hash skin 再 upload skip、ring buffer 容量効率)
- Vulkan validation 0 件 (= 6 UBO + set=2 binding=0 共有 dispatch + PerProgram → PerDraw 降格 SPIR-V validation + ring buffer dynamic offset alignment)
- cross-UBO 同期 verify (= 4 trigger 全件で N UBO 全 dirty + 値同期、特に bone animation frame swap で AvatarSkin/AvatarVelocity + ObjectSkin/SkinnedVelocity 4 UBO 連動性 + per-draw object 切替で VelocityV/VelocityAlphaV 2 UBO 連動性)
- first-frame fallback ring buffer 経路実装 verify (= `mLastGLMp.empty()` 分岐維持、lightning-streak velocity 回避担保)
- ring buffer 容量 verify (= ObjectSkin 10752 + SkinnedVelocity 5376 + AvatarSkin/Velocity 768×2 + VelocityAlphaV 256 = 17920 B/draw 大型 rigged scene 実測、`sDrawUboRingBufferMgr` 容量設計確認)
- data duplication 解消設計確定 (= ObjectSkin/SkinnedVelocity 統合 vs 維持、VelocityV/VelocityAlphaV 同 data 別 UBO 維持の根拠)
- cadence 再分類結果反映 (= L0-4 結果、VelocityV/VelocityAlphaV PerProgram → PerDraw 必須降格 + ubo_metadata.inl cadence_tag=1 → cadence_tag=2 変更、AYA 判断 + roadmap §5.1 cadence 別 update site 整合)
- bare uniform 残存ゼロ確認 (= 6 shader `#else` block bare uniform setter が host C++ 側に残存しない)
- bone count packing verify (= AvatarSkin vec4[45] 15 bone × 3 row layout + ObjectSkin/SkinnedVelocity mat3x4[110] stride=48 packing 正確)
- **verify 単位 = group verify** (= 6 UBO 揃って初めて整合 visual、中間状態 (= 1 UBO のみ通電) は暫定 default 値で破綻回避 = static avatar 描画継続 + motion blur 無効、AYA literal「現状の見た目とほぼ変わらない描画」継承)

##### (7) 4 原則 gate

- **原則 1 (Core 分散)**: 6 program 個別 = render thread 単独 → ✅、cross-UBO 同期 logic は 4 trigger setter call に集約 (= bone frame swap × 2 + per-draw object 切替 + motion blur enable)、各 trigger 内で N UBO write 並列化候補 = 将来 Core 化で各 UBO 別 thread で write 可能、ring buffer per-thread 化済 (= PC-N-15a `mDrawUboRingBuffer`)、設計原則 (2) Core 分散実現整合 (memory `project_ayastorm_r41_design_principles`)
- **原則 2 (3 OS 共通)**: ✅ (= Vulkan core spec 1.3 範囲内、`maxUniformBufferRange` minimum 16384 B 範囲内、UBO_DYNAMIC + dynamic offset bind は MoltenVK Argument Buffer Tier 2 整合)
- **原則 3 (Phase 2/3)**: Phase 2 内、L0-4 cadence 再分類 PerProgram → PerDraw 降格 2 件必須 (= VelocityV/VelocityAlphaV) + PerDraw → PerSkin 昇格検討 2 件 (= ObjectSkin/SkinnedVelocity、Skin_GLTFJoints 戦略整合候補)、ubo_metadata.inl cadence_tag 変更 → ✅
- **原則 4 (OpenGL を殺さない)**: 6 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能、first-frame fallback logic は OpenGL 経路で BlackDragon import 由来 (= `llshadermgr.h:386` literal)、UBO 経路でも同 logic 移植維持 → ✅
- **visual regression ゼロ**: avatar/object skinning 描画同一 + motion blur 退化ゼロ + lightning-streak velocity 回避 + first-frame fallback 維持 → AYA live verify (= avatar bone animation + attachment rigged mesh + motion blur enable + first-frame display + per-draw object 切替 + dedup logic)、本 group は motion blur pipeline 直結 + skinning pipeline 直結ゆえ AYAstorm 視覚機能交差大 (= memory `project_skin_hash_collision_bom_body` + `project_sl_alpha_mesh_rigged_nonrigged_link` + `reference_attachment_rendering_routing` 整合)、`motionBlurF.glsl:145` comment 警告「uninitialized → NaN」回避担保

---

#### §3.5.9 L4-9: reflection probe / IBL pipeline 5 UBO group (= LLReflectionMapManager 由来、SINGLETON + PerProgram + mip chain loop)

**AYA literal 命名 mapping**: READINESS §3.9 (= reflection probe/IBL pipeline 連動 41-45 = 5 UBO、AYA 単純配列で C-41〜C-45 相当)

**位置付け**: L4 group 9 件目 = LLReflectionMapManager 由来 reflection probe IBL pipeline 経路、Phase 1.C PC-2 shell 通電済 (= Global_ReflectionProbes 唯一 SINGLETON cadence)、shell 通電済 + write 経路本格化済 (= IrradianceGen + Gaussian 2 UBO、Phase 1.C PC-7γ-1)、cadence 混在 (= SINGLETON 1 + PerProgram 4) + IBL mip chain loop で per-pass dirty pattern (= PerProgram cadence では複数回 dirty pattern、PerDraw 化候補) + probe count loop で sourceIdx 共有 (= IrradianceGen + RadianceGen 同 `sSourceIdx` static)

**group 概要** (= 3 sub-cluster 5 UBO、cross-UBO 同期 protocol 3 trigger 由来):

**sub-cluster (a) Global_ReflectionProbes 単独 SINGLETON 1 UBO** (= set=0 PerFrame 帯外、process-wide):
- **Global_ReflectionProbes** (set=0 binding=3、256B、std140 16B、1 member = `_shell_placeholder` vec4 offset=0) — shell 通電済 (= Phase 1.C PC-2、`bringupTestUBO()` 経由 zero dummy buffer + 5 cadence 全経路 `vkCmdBindDescriptorSets` 通電完了、shader consume 未開始)、cadence_tag=5 SINGLETON = `llglslshader.cpp:99` literal `constexpr U32 kCadenceSingleton = 5u; // Global_ReflectionProbes (flushSingletonUbos 別経路)`、`flushSingletonUbos` 経路
- data source (= **不明 / verify 要**):
  - `LLReflectionMapManager` 経由 reflection probe global state (= `gPipeline.mReflectionMapManager` 等候補)
  - 実 member 候補 = reflection probe count / probe positions array / probe influence radii array / probe HDR exposure / probe cubemap atlas slot (= 推定、全 [要追加調査])
- bringupTestUBO 経路: `llglslshader.cpp:2273` literal、contract assertion = `block_hash == 0xabdfdb31u` + `block_size == ubo::Global_ReflectionProbes_SIZE` literal `llglslshader.cpp:2287-2288`
- layout 不可触契約 (= roadmap §4.2)、Phase 2 で `_shell_placeholder` を実 reflection probe data 群に置換しても set/binding/size 不変、shader 接続必須 (= 既存 OpenGL `#else` block uniform 個別宣言を UBO member access 書換)

**sub-cluster (b) ReflectionProbeUBO_Legacy 単独 PerProgram 1 UBO** (= set=3 binding=17、reflectionProbeF.glsl singleton consume):
- **ReflectionProbeUBO_Legacy** (set=3 binding=17、256B、std140 16B = **最小 size UBO**、2 member = `max_probe_lod` float offset=0 + `transparent_surface` bool offset=4) — `class3/deferred/reflectionProbeF.glsl:80` singleton site、setter 不明 [要追加調査]
- data source (= 不明 / verify 要):
  - `LLReflectionMapManager` 経由現在 reflection probe state (= max probe LOD + 透過面判定)
  - `LLPipeline::renderDeferredLighting` 経路の per-pass parameter
- **bool member std140 risk**: `bool transparent_surface` は std140 で 4 B (uint 相当)、VkBool32 (= 4 B 0/1) 整合 verify 要 [要 verify]
- **Global_ReflectionProbes との関係**: 同 `LLReflectionMapManager` 由来候補だが cadence/set 異 (= SINGLETON vs PerProgram、process-wide vs program 単位)、統合可能性 + 別 UBO 維持理由 verify 要 [要 verify L0-1 統合判断]

**sub-cluster (c) IBL mip pipeline 3 UBO** (= reflection probe regenerate pass + mip chain loop + probe count loop):
- **RadianceGenFParamUBO_Legacy** (set=3 binding=51、256B、std140 32B、8 member = `sourceIdx` int offset=0 + `u_width` int offset=4 + `mipLevel` float offset=8 + `max_probe_lod` float offset=12 + `probe_strength` float offset=16 + pad ×3 offset=20-28) — `class1/interface/radianceGenF.glsl:42` singleton site、setter `llreflectionmapmanager.cpp:930` literal `gRadianceGenProgram.uniform1i(sSourceIdx, sourceIdx);` (= `sSourceIdx` static LLStaticHashedString 共有)
- **IrradianceGenFParamUBO_Legacy** (set=3 binding=52、256B、std140 16B、4 member = `sourceIdx` int offset=0 + `max_probe_lod` float offset=4 + pad ×2 offset=8-12) — `class2/interface/irradianceGenF.glsl:42` singleton site、setter `llreflectionmapmanager.cpp:977` literal `gIrradianceGenProgram.uniform1i(sSourceIdx, sourceIdx);` (= `sSourceIdx` 共有)、shell 通電済 + write 経路本格化済 (= Phase 1.A PA-8 + 1.C PC-7γ-1)
- **GaussianFParamUBO_Legacy** (set=3 binding=58、256B、std140 16B、2 member = `resScale` float offset=0 + `direction` vec2 offset=8 = 4B pad 自動挿入) — `class1/interface/gaussianF.glsl:46` singleton site、`gGaussianProgram` 経路 (= `llviewershadermgr.cpp:86` literal + `:3977` "Reflection Mip Shader"、用途 = separable gaussian blur for reflection mip)、setter 不明 (= reserved 宣言済 `llglslshader.cpp:1082/1088` literal `"direction"` / `"resScale"`、実 setter call grep hit せず) [要追加調査]、shell 通電済 + write 経路本格化済
- data source: 全 3 UBO `LLReflectionMapManager::doProbeUpdate` reflection probe regenerate 経路 (= `llreflectionmapmanager.cpp:810-814/920-977` literal、3 program (= gIrradianceGenProgram / gRadianceGenProgram / gGaussianProgram) 連動 + probe count loop で sSourceIdx 共有 + mip chain loop で per-pass dirty)
- `llheroprobemanager.cpp:456` literal `gHeroRadianceGenProgram.uniform1i(sSourceIdx, sourceIdx);` (= hero probe 別 program、本 group 直接対象外だが同 IBL 経路、verify 要)
- consume detail: `class2/interface/irradianceGenF.glsl:215` literal `lod = clamp(lod, 0, max_probe_lod);` + `:217` literal `vec4 lambertian = textureLod(reflectionProbes, vec4(H, sourceIdx), lod);` + `class1/interface/radianceGenF.glsl` separable mip filtering pass
- **mip chain loop dirty pattern**: RadianceGen + Gaussian は mip level 毎に dispatch (= 各段で `mipLevel` / `direction` 値変化)、PerProgram cadence では同 program 連続 dispatch で N 回 dirty pattern → cadence mismatch 候補 (= PerDraw cadence 化検討) **[要 L0-4 結果反映 / 要 AYA 判断]**
- **probe count loop dirty pattern**: IrradianceGen + RadianceGen は probe 毎に `sourceIdx` 更新 (= N probe × write + flush + dispatch)、既存 OpenGL の uniform 更新と同 frequency、perf inversion なし想定 [要 verify]

##### (1) 前提条件

- L0-1 (= dispatch logic、特に set=3 binding=17/51/52/58 衝突なし独立 binding、Global_ReflectionProbes は set=0 binding=3 独立)
- L0-2 (= LLStaticHashedString UBO redirect 経路、IrradianceGen + RadianceGen が `sSourceIdx` 共有で同 redirect pattern 利用)
- L0-4 (= cadence 妥当性、特に RadianceGen + Gaussian mip chain loop での per-pass dirty pattern が PerProgram cadence で正常動作するか、PerDraw cadence 化必須候補)
- L1a-1 (= ClipFParamUBO_Legacy LLStaticHashedString redirect pilot 完了)
- L1b 完了 (= set=0 同居 Frame UBO 4 件、Global_ReflectionProbes と同 set=0 frame start bind 経路共有)
- Phase 1.C PC-2 (= Global_ReflectionProbes shell 通電済、`bringupTestUBO()` 経路通電 + 5 cadence 全経路通電完了)
- Phase 1.C PC-7γ-1 (= IrradianceGen + Gaussian shell 通電済 + write 経路本格化済)

##### (2) 不明事項

**setter 経路不明**:
- Global_ReflectionProbes 実 reflection probe data 群 setter (= LLReflectionMapManager 内 31 setter のうち該当 site 全件) **[要追加調査]**
- ReflectionProbeUBO_Legacy `max_probe_lod` / `transparent_surface` setter (= `uniform1f(max_probe_lod)` + `uniform1i(transparent_surface)`、grep verify 要) **[要追加調査]**
- RadianceGen `u_width` / `mipLevel` / `max_probe_lod` / `probe_strength` setter (= sSourceIdx は特定済、他 4 member 不明) **[要追加調査]**
- IrradianceGen `max_probe_lod` setter (= reserved 宣言 `llshadermgr.cpp:1834` literal 確認、実 setter call 別 site 想定) **[要追加調査]**
- Gaussian `resScale` / `direction` setter (= reserved 宣言済、実 setter call grep hit せず、`LLReflectionMapManager` 内 reflection mip generation 経路想定) **[要追加調査]**
- `gGaussianProgram` bind callsite (= reflection probe regenerate path 特定要) **[要追加調査]**

**cadence 妥当性 (= L0-4 結果反映待ち)**:
- RadianceGen + Gaussian mip chain loop での per-pass dirty pattern (= 1 program 連続 dispatch で N 回 dirty)、PerProgram cadence で正常動作するか、PerDraw cadence 化必須候補 **[要 L0-4 結果反映 / 要 AYA 判断]**
- IrradianceGen + RadianceGen probe count loop での `sourceIdx` 更新 (= N probe × write + flush + dispatch)、既存 OpenGL の uniform 更新と同 frequency、perf inversion なし想定 [要 verify]

**cross-UBO data source 共有 verify**:
- Global_ReflectionProbes (SINGLETON) ↔ ReflectionProbeUBO_Legacy (PerProgram) 同 `LLReflectionMapManager` 由来、統合可能性 + 別 UBO 維持理由 (= cadence 違い + consume pattern 違い) **[要 verify D4 突合 + 要 AYA 判断]**
- IrradianceGen.sourceIdx ↔ RadianceGen.sourceIdx 同 `sSourceIdx` 共有 verify (= 同 LLStaticHashedString static instance) **[要 verify D4 突合]**
- ReflectionProbeUBO_Legacy.max_probe_lod ↔ RadianceGen.max_probe_lod ↔ IrradianceGen.max_probe_lod 同名 3 UBO 共有 (= 同 LOD clamp、verify 要 D4 突合) **[要 verify D4 突合]**

**Global_ReflectionProbes 実 member 構造**:
- shell `_shell_placeholder` を Phase 2 で実 reflection probe data 群に置換、layout 不可触契約 (= set/binding/size 不変)、実 member 候補全件 **[要追加調査]**

**reflection update fence throttle (RF)**:
- chapter 07 §12 持越項目、Phase 0 計測結果参照要、dirty 判定 trigger = reflection probe update event (= camera move / probe regenerate / scene change) **[要追加調査]**

**hero probe 別経路**:
- `llheroprobemanager.cpp:456` literal `gHeroRadianceGenProgram.uniform1i(sSourceIdx, sourceIdx);` (= hero probe 別 program、本 group 直接対象外だが同 IBL 経路) [要 verify、別 UBO 化必要か共有可能か]

**bare uniform 残存**:
- 5 shader `#else` block bare uniform setter が OpenGL 経路で host C++ 側に残存しているか [要追加調査]

##### (3) 調査手法

- **D1 setter Grep**: `LLReflectionMapManager` 内 reflection probe regenerate 経路全件 (= `llreflectionmapmanager.cpp:810-814/920-977` 周辺) + `gIrradianceGenProgram` / `gRadianceGenProgram` / `gGaussianProgram` bind callsite + 5 UBO 各 member 名 (= `_shell_placeholder` placeholder / `max_probe_lod` / `transparent_surface` / `sourceIdx` / `u_width` / `mipLevel` / `probe_strength` / `resScale` / `direction`) setter site 全件
- **D2 既存実装読解**: `LLReflectionMapManager::doProbeUpdate` 経路読解 (= probe regenerate trigger + mip chain loop + probe count loop + IBL pipeline 3 program 連動)、`indra/newview/llreflectionmapmanager.h` + `indra/newview/llheroprobemanager.cpp` 構造把握
- **D3 cadence verify**: mip chain loop + probe count loop での PerProgram cadence stale risk 実測 (= 同 program 連続 dispatch で `mipLevel` / `direction` / `sourceIdx` 値変化に追従するか)
- **D4 突合**:
  - Global_ReflectionProbes ↔ ReflectionProbeUBO_Legacy data source 共有 verify
  - IrradianceGen.sourceIdx ↔ RadianceGen.sourceIdx static 共有 verify
  - max_probe_lod 3 UBO 共有 verify

##### (4) 設計 task (= 4 経路、group 単位)

- **register**: 5 UBO 個別 register、Global_ReflectionProbes は SINGLETON 経路 (= `flushSingletonUbos`) + 他 4 UBO は PerProgram triple-buffer + program 識別で wrap (= reflectionProbeF program / radianceGenF program / irradianceGenF program / gaussianF program)
- **write**: 3 trigger 経路で N UBO 同時 dirty:
  - **trigger 1 reflection probe regenerate (= probe regenerate event、scene change / probe update)**: Global_ReflectionProbes + ReflectionProbeUBO_Legacy + IrradianceGen + RadianceGen + Gaussian 5 UBO 全 dirty + IBL pipeline 3 program 連続 dispatch
  - **trigger 2 mip chain loop (= per-mip dispatch)**: RadianceGen + Gaussian で `mipLevel` / `direction` per-pass update (= 1 program 連続 dispatch、PerProgram cadence では多回 dirty pattern → PerDraw cadence 化候補)
  - **trigger 3 probe count loop (= per-probe dispatch)**: IrradianceGen + RadianceGen で `sourceIdx` per-probe update (= N probe × write + flush + dispatch、sSourceIdx static 共有)
- **flush**: Global_ReflectionProbes = `flushSingletonUbos` 経路 / 他 4 UBO = `flushProgramUbos` per-program 経路 (= cmdbuf 経路で triple-buffer 経由 `vkCmdBindDescriptorSets`)
- **shader 接続**: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= Global_ReflectionProbes は shader 接続必須 = `class3/deferred/reflectionProbeF.glsl` 等候補で UBO member access 追加、ReflectionProbeUBO_Legacy `reflectionProbeF.glsl:80` / RadianceGen `radianceGenF.glsl:42` / IrradianceGen `irradianceGenF.glsl:42` / Gaussian `gaussianF.glsl:46`、改変ゼロ原則 4 維持、ただし Global_ReflectionProbes は shell → 実 data 置換時 `#else` block uniform 個別宣言を UBO member access 書換必要)
- **cross-UBO 同期 protocol**: 3 trigger event で group 内該当 UBO 全 dirty (= reflection probe regenerate trigger = 5 UBO 一括 / mip chain loop = RadianceGen+Gaussian 連動 / probe count loop = IrradianceGen+RadianceGen sSourceIdx 共有)
- **mip chain loop cadence 再設計**: RadianceGen + Gaussian PerProgram → PerDraw 降格 (= ubo_metadata.inl cadence_tag=1 → cadence_tag=2、ring buffer 経路移行) **[要 AYA 判断 = mip loop dirty pattern 解消必須候補]**
- **probe count loop cadence 検討**: IrradianceGen + RadianceGen probe count loop の sourceIdx 更新は既 PerProgram cadence で perf inversion なし想定 (= 既存 OpenGL setter と同 frequency)、PerDraw 移行不要、ただし perf 実測 verify
- **Global_ReflectionProbes 統合検討**: ReflectionProbeUBO_Legacy と統合 vs 維持 = data source 共有なら 1 UBO 統合候補、ただし cadence 違い (SINGLETON vs PerProgram) で別 UBO 設計理由有り得る **[要 AYA 判断]**

##### (5) 工程 task

- trace 順内位置: L4 group 9 件目 = §3.5.7/§3.5.8 大型 group 後、shell 通電済 3 UBO (= Global_ReflectionProbes / IrradianceGen / Gaussian) ゆえ通電本格化作業優位、reflection probe pipeline 直結
- group 内 並列性:
  - sub-cluster (a) Global_ReflectionProbes = SINGLETON cadence、shell 通電済ゆえ実 member 置換 + shader 接続着手可
  - sub-cluster (b) ReflectionProbeUBO_Legacy = PerProgram、reflectionProbeF.glsl singleton consume、independent
  - sub-cluster (c) IBL mip pipeline 3 UBO = mip chain loop cadence 再設計確定後、IrradianceGen 先着手可 (= shell 通電済 + setter 特定済)、RadianceGen + Gaussian は cadence 再分類後着手
- group 間 並列性: §3.5.10 (post-process) / §3.5.11 (glow) / §3.5.12 (SMAA) / §3.5.13 (pathfinding) / §3.5.14 (GLTF asset) / §3.5.15 (set=2 binding=0 共有残) / §3.5.16 (MultiLight) と並列可
- 推定工数: **M-L** (= 半日 +〜1 日、5 UBO + cadence 再設計 (mip chain loop) + Global_ReflectionProbes 実 member 特定 + 5 path visual verify + LLReflectionMapManager 内 setter 30+ site 特定 + 統合判断)

##### (6) A 確定条件

- mUseUBO ON + 5 shader 活性化 + setter 通電 (= 5 UBO 全件、3 trigger 経路全件)
- AYA live verify (= 全 path visual regression ゼロ §5.4):
  - **PBR material 反射描画** (= reflection probe IBL 効果、metallic/glossy 表面で probe cubemap 反射整合)
  - **reflection probe regenerate** (= probe scene change で 5 UBO 一括 dirty + IBL pipeline 3 program 連続 dispatch 整合、probe regenerate 描画一致)
  - **mip chain loop 整合** (= radianceGen / gaussian の mip level 毎 dispatch 値正確、reflection mip chain 全段品質維持)
  - **probe count loop 整合** (= IrradianceGen + RadianceGen の sSourceIdx 各 probe 正確、N probe IBL 効果整合)
  - **transparent_surface 効果** (= opaque/blend pass 判定整合、ReflectionProbeUBO_Legacy.transparent_surface 反映)
  - **max_probe_lod clamp** (= IBL fetch LOD clamp 整合、reflection 詳細度維持)
- Vulkan validation 0 件 (= Global_ReflectionProbes SINGLETON 経路 + 4 PerProgram UBO + mip chain loop per-pass dirty + sSourceIdx 共有 + bool member std140 4B 整合)
- cross-UBO 同期 verify (= 3 trigger 全件で N UBO 全 dirty + 値同期、reflection probe regenerate で 5 UBO 一括 + mip chain loop で per-pass cadence 整合 + probe count loop で sSourceIdx 共有)
- cadence 再設計結果反映 (= L0-4 結果、RadianceGen + Gaussian mip chain loop PerProgram → PerDraw 降格判断、AYA 判断 + ubo_metadata.inl cadence_tag 変更整合)
- Global_ReflectionProbes 実 member 確定 (= shell `_shell_placeholder` → 実 reflection probe data 群置換、layout 不可触契約 256 B / set=0 / binding=3 不変、shader 接続 (= `class3/deferred/reflectionProbeF.glsl` 等候補) UBO member access 追加)
- bare uniform 残存ゼロ確認 (= 5 shader `#else` block bare uniform setter が host C++ 側に残存しない)
- bool member std140 4B 整合 verify (= ReflectionProbeUBO_Legacy.transparent_surface VkBool32 整合)
- reflection update fence throttle (RF) 配線 (= chapter 07 §12 持越項目、Phase 0 計測結果反映、dirty 判定 trigger 経路実装)
- **verify 単位 = group verify** (= 5 UBO 揃って初めて整合 visual、中間状態は暫定 default 値で破綻回避 = reflection 黒抜け or default cubemap 描画継続、AYA literal「現状の見た目とほぼ変わらない描画」継承)

##### (7) 4 原則 gate

- **原則 1 (Core 分散)**: 5 program 個別 = render thread 単独 → ✅、reflection probe regenerate 経路は render thread 内逐次 (= probe regenerate 1 frame 内 完結)、将来 Core 化候補 = LLReflectionMapManager 内 probe regenerate を worker thread で実行 + UBO write は per-thread `LLUboRingBuffer` 経路 (= memory `project_ayastorm_r41_design_principles` C1-C6 整合、設計原則 (2) Core 分散実現整合)
- **原則 2 (3 OS 共通)**: ✅ (= Vulkan core spec 1.3 範囲内、SINGLETON cadence は MoltenVK 整合、bool member std140 4B は VkBool32 統一)
- **原則 3 (Phase 2/3)**: Phase 2 内、L0-4 cadence 再分類 (= RadianceGen + Gaussian mip chain loop PerProgram → PerDraw 降格判断) → ✅
- **原則 4 (OpenGL を殺さない)**: 5 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能 → ✅、ただし Global_ReflectionProbes は shell → 実 data 置換時 `#else` block 改変必要 (= 既存 OpenGL 経路 uniform 個別宣言を UBO member access 書換)、shader compile 差分 risk + upstream patch conflict risk (= 設計原則 (1) Upstream 取り込みやすさ維持と緊張) **[要 verify]**
- **visual regression ゼロ**: PBR material 反射描画 + reflection probe regenerate + mip chain loop + probe count loop + transparent_surface 効果 + max_probe_lod clamp 全件描画同一 → AYA live verify、本 group は PBR material + reflection probe pipeline 直結ゆえ AYAstorm 視覚機能交差 (= 反射描画 = AYAstorm 視覚表現章 reflection 品質維持)、reflection update fence throttle (RF) 整合担保

---

#### §3.5.10 L4-10: post-process pipeline 6 UBO group (= luminance / exposure / tonemap / gamma / color grading / vignette chain、AYAstorm r30 Cinematic Control 13 cvar 直結)

**AYA literal 命名 mapping**: READINESS §3.10 (= post-process pipeline 連動 46-51 = 6 UBO、AYA 単純配列で C-46〜C-51 相当)

**位置付け**: L4 group 10 件目 = post-process chain 直結、AYAstorm r30 Cinematic Control 13 cvar 直結 (= memory `project_r30_cinematic_control_tuning_deferred` + `project_ayastorm_r30_cinematic_chapter` + `project_ayastorm_visual_realism_chapter`)、AYA 既存機能 risk 大 (= AYAstorm 視覚表現章 + r30 Cinematic 機能維持必須)、shell 通電済 + write 経路本格化済 2 UBO (= Luminance + Exposure、Phase 1.A PA-8 + 1.C PC-7γ-1)、setter 特定済 2 UBO + 不明 4 UBO、cadence PerProgram + auto-exposure per-frame perceived (= Exposure noiseVec ll_frand setter ゆえ frame 内 1 回 GPU upload で済む)、4 sub-cluster (= auto-exposure / tonemap / display correction / vignette)、post-process chain sequential dispatch (= luminance → exposure → tonemap → gamma correct → color grading → vignette)

**group 概要** (= 4 sub-cluster 6 UBO、cross-UBO 同期 protocol 4 trigger 由来):

**sub-cluster (a) auto-exposure chain 2 UBO** (= Luminance → Exposure、`pipeline.cpp` 経路 shell 通電済 + write 経路通電済):
- **LuminanceFParamUBO_Legacy** (set=3 binding=26、256B std140 4B、1 member = `diffuse_luminance_scale` float offset=0、**最小 member UBO**) — `class1/deferred/luminanceF.glsl:60` singleton site、**shell 通電済 + write 経路本格化済** (= Phase 1.A PA-8 + 1.C PC-7γ-1)、setter `pipeline.cpp:8754` literal `gLuminanceProgram.uniform1f(diffuse_luminance_scale_s, diffuse_luminance_scale);` (= `pipeline.cpp:8753` `static LLStaticHashedString diffuse_luminance_scale_s("diffuse_luminance_scale");` 宣言済)、data source `RenderDiffuseLuminanceScale` cvar (= `pipeline.cpp:8731` literal `static LLCachedControl<F32> diffuse_luminance_scale(gSavedSettings, "RenderDiffuseLuminanceScale", 1.0f);`)
- **ExposureFParamUBO_Legacy** (set=3 binding=25、256B std140 48B、4 member = `dt` float offset=0 + `noiseVec` vec2 offset=8 + `dynamic_exposure_params` vec4 offset=16 + `dynamic_exposure_params2` vec4 offset=32) — `class1/deferred/exposureF.glsl:49` singleton site、**shell 通電済 + write 経路本格化済** (= Phase 1.A PA-8 + 1.C PC-7γ-1)、setter 4 site (= `pipeline.cpp:8815` `dt` LLStaticHashedString 宣言 + `:8863` literal `shader->uniform2f(noiseVec, ll_frand() * 2.0f - 1.0f, ll_frand() * 2.0f - 1.0f);` + `:8864` literal `shader->uniform4f(dynamic_exposure_params, dynamic_exposure_coefficient, exp_min, exp_max, dynamic_exposure_speed_error);` + `:8865` literal `shader->uniform4f(dynamic_exposure_params2, sky->getHDROffset(should_auto_adjust()), exp_min, exp_max, dynamic_exposure_speed_target);`)、`dt` 実 setter call site **[要追加調査]**、`dynamic_exposure_enabled` (= `pipeline.cpp:8819` literal 宣言、本 UBO member 不在 = shader 内別 uniform 経路 vs gating-only) **[要 verify]**
- data source: `LLPipeline::renderPostProcess` post-deferred exposure pass (= `pipeline.cpp:8791-8865` + `:8729-8762` literal、`gExposureProgram` / `gExposureProgramNoFade` / `gLuminanceProgram` 経路)
- **cross-UBO 同期 protocol**: auto-exposure update trigger (= per-frame) で 2 UBO 連動 dirty、`noiseVec` `ll_frand()` per-frame 変化ゆえ setter 呼出 frequency = per-frame、ただし frame 内 exposure pass 1 回ゆえ writeProgramUbo → flushProgramUbos 経由 frame 内 1 回 GPU upload で済む (= perf inversion なし)
- consume: `class1/deferred/luminanceF.glsl` (= scene luminance 計算 + diffuse channel scaling、`pipeline.cpp:8734/8740/8746` literal で DEFERRED_DIFFUSE / DEFERRED_EMISSIVE / NORMAL_MAP texture sampler 同時 consume) + `class1/deferred/exposureF.glsl` (= dynamic exposure HDR 計算)

**sub-cluster (b) tonemap 1 UBO** (= Cinematic mode 関連、setter 不明):
- **TonemapUBO_Legacy** (set=3 binding=10、256B std140 16B、4 member = `exposure` float offset=0 + `tonemap_mix` float offset=4 + `tonemap_type` int offset=8 + `_pad_tonemap_0` float offset=12) — `class1/deferred/tonemapUtilF.glsl:146` singleton site、setter 不明 [要追加調査]、cadence PerProgram (= ubo_metadata.inl:114 cadence_tag=1)
- data source (= 不明 / verify 要):
  - `exposure` = HDR exposure (= ExposureFParamUBO_Legacy 計算結果由来 / LLEnvironment auto-exposure、sub-cluster (a) と data source 共有可能性 [要 verify D4 突合])
  - `tonemap_mix` = tonemap mix factor (= debug settings `RenderTonemapMix` / Cinematic mode 由来、verify 要、AYAstorm r30 Cinematic Control 13 cvar 関連候補)
  - `tonemap_type` = tonemap operator enum int (= ACES / Reinhard / Linear 等、debug settings `RenderTonemapType` 由来)
- AYAstorm 視覚表現章 / r30 Cinematic mode tonemap 系 cvar 関連 (= memory `project_ayastorm_visual_realism_chapter` + `project_ayastorm_r30_cinematic_chapter` + `project_r30_cinematic_control_tuning_deferred`)、本 UBO は tonemap util 共通 base、AYAstorm 独自拡張は別 UBO 候補 [要 verify]
- cadence 再評価候補: `exposure` per-frame 変化、PerProgram cadence で同 program 内 frame 間 stale data risk (= sub-cluster (a) Exposure 同形 risk、ただし frame 内 tonemap pass 1 回ゆえ実質的 perf inversion なし想定) [要 verify D3]

**sub-cluster (c) display correction 2 UBO 共有** (= GammaCorrect 2 shader 共有 + ColorGrading AYAstorm r30 Cinematic Control 13 cvar 直結):
- **PerProgramUBO_GammaCorrect** (set=2 binding=2、256B std140 16B、4 member = `gamma` float offset=0 + `_pad_gc0/1/2` float ×3 offset=4-12) — 2 file consume (= **`class1/deferred/postDeferredGammaCorrect.glsl:45` + `class1/deferred/postDeferredTonemap.glsl`**、blueprint コメント `verified identical across 2 sample sites`)、setter 不明 [要追加調査]、data source `RenderDeferredDisplayGamma` cvar (= 推定、`LLShaderMgr::DISPLAY_GAMMA` 等候補)
- **PerProgramUBO_ColorGrading** (set=2 binding=4、256B std140 32B、8 member = `color_saturation` float + `color_contrast` float + `color_temperature` float + `color_brightness` float + `color_grading_lut_intensity` float + `color_grading_lut_enabled` int + `_pad_cg0/1` float ×2、offset=0/4/8/12/16/20/24/28) — `class1/deferred/postDeferredTonemap.glsl:74` singleton site、setter 不明 [要追加調査]
- **AYAstorm Cinematic Control 13 cvar 直結 (= 本 group 最重要、AYA 既存機能 risk 大)**:
  - memory `project_r30_cinematic_control_tuning_deferred` (= r30 Phase 6 で 13 cvar BD live 配線完了、default/range/UX チューニングは後日)
  - 6 active member (= saturation / contrast / temperature / brightness / lut_intensity / lut_enabled) と 13 cvar の対応関係 verify 要 [要 verify D4 突合]
  - AYAstorm 視覚表現章 + r30 Cinematic Control 機能維持必須 (= memory `project_ayastorm_visual_realism_chapter` + `project_ayastorm_r30_cinematic_chapter` + `project_ayastorm_r30_bd_improvement_phase`)
- consume detail:
  - postDeferredGammaCorrect.glsl = SRGB 出力路 gamma correction (= primary site)
  - postDeferredTonemap.glsl = tonemap + color grading + gamma 統合 pass (= 2nd site for GammaCorrect、singleton site for ColorGrading)
- **2 shader UBO 共有 risk (GammaCorrect)**: postDeferredGammaCorrect + postDeferredTonemap 両者で同 UBO consume、host 側 1 UBO instance を 2 program bind 共有可能 (= PerProgram cadence で data 共通ゆえ 1 alloc 2 bind 可能性)、別案 = 各 program 別 UBO instance (= cadence=1 PerProgram 厳密解釈) [要 verify L0-1 dispatch]
- **set=2 binding=2 vs binding=4**:
  - GammaCorrect set=2 binding=2 (= set=2 binding=0 共有 6 UBO の binding=2 ≠ MultiLight binding=1、独立 binding)
  - ColorGrading set=2 binding=4 (= 独立 binding)
  - 衝突なし、ただし set=2 PerDraw 帯 + PerProgram 混在帯ゆえ dispatch 経路注意 [要 verify L0-1]

**sub-cluster (d) vignette 1 UBO** (= exoVignette、Exodus/BD derivative):
- **VignetteParamUBO_Legacy** (set=3 binding=46、256B std140 16B、2 member = `vignette` vec3 offset=0 + `_pad_vignette_legacy_0` float offset=12) — `class1/post/exoVignetteF.glsl:42` singleton site、setter 不明 [要追加調査]
- data source: AYAstorm r14+ 視覚表現章 / Cinematic mode vignette 関連 cvar (= 推定、verify 要)、`vignette` vec3 3 component 内訳不明 (= intensity / radius / softness 等推定、shader 内 access pattern verify 要 [要追加調査])
- **exoVignette 由来 risk**: shader 名 `exo` prefix = Exodus / BlackDragon derivative branch 経由実装の可能性、設計原則 (1) Upstream OpenGL 取り込みやすさ維持 (= memory `project_ayastorm_r41_design_principles`) と緊張、上流 vignette 実装と Vulkan path 整合性 verify 要 [要 verify]
- consume: post-process chain 後段 (= tonemap → gamma → color grading → vignette、final display 直前 pass)

##### (1) 前提条件

- L0-1 (= dispatch logic、特に set=2 binding=2 (GammaCorrect) + set=2 binding=4 (ColorGrading) 独立 binding、ただし set=2 PerDraw 帯 + PerProgram 混在帯ゆえ program 識別経路 verify)
- L0-2 (= LLStaticHashedString UBO redirect 経路、Luminance.diffuse_luminance_scale_s が name-based setter で UBO redirect 透過するか verify 必須、PC-7γ-1 で 31 setter 経路 index + name 両対応想定だが name 版経路の確認要 [要 verify])
- L0-3 (= per-shader UBO block 拡大、特に GammaCorrect 2 shader 共有 + postDeferredTonemap 内 GammaCorrect + ColorGrading 同時 consume の同 file 内多 UBO 宣言経路)
- L0-4 (= cadence 妥当性、Exposure noiseVec ll_frand per-frame 変化 + TonemapUBO exposure auto-exposure per-frame 変化 が PerProgram cadence で正常動作するか、frame 内 1 回 GPU upload で perf inversion なし想定だが verify 要)
- L1a-1 (= ClipFParamUBO_Legacy LLStaticHashedString redirect pilot 完了、Luminance.diffuse_luminance_scale_s 同 pattern 利用)
- L1b 完了 (= FrameViewProj + FrameLights + FrameAtmosphere_Lighting、全 post-process shader で同時 consume 推定、verify 要)
- Phase 1.A PA-8 + 1.C PC-7γ-1 (= Luminance + Exposure shell 通電済 + write 経路本格化済)
- §3.5.9 完了 (= reflection probe / IBL pipeline、tonemap pre-step、verify 要 cross-UBO data source)

##### (2) 不明事項

**setter 経路不明 (= 4 UBO)**:
- TonemapUBO `exposure` / `tonemap_mix` / `tonemap_type` setter (= `uniform1f`/`uniform1i` 3 件、Cinematic mode 経路) **[要追加調査]**
- GammaCorrect `gamma` setter (= postDeferredGammaCorrect + postDeferredTonemap 2 program 共有経路、`LLShaderMgr::DISPLAY_GAMMA` 等候補) **[要追加調査]**
- ColorGrading 6 member setter (= postDeferredTonemap pass、AYAstorm r30 Cinematic Control 13 cvar 経路) **[要追加調査]**
- Vignette `vignette` setter (= `uniform3fv("vignette", ...)` 経路) **[要追加調査]**

**Exposure 既存 setter 詳細**:
- `dt` 実 setter call site (= `pipeline.cpp:8815` LLStaticHashedString 宣言済、実 setter call grep 要) **[要追加調査]**
- `dynamic_exposure_enabled` (= `pipeline.cpp:8819` literal 宣言、本 UBO member 不在 = shader 内別 uniform 経路 vs gating-only) **[要 verify]**

**cadence 妥当性 (= L0-4 結果反映待ち)**:
- Exposure noiseVec ll_frand per-frame 変化 + TonemapUBO exposure auto-exposure per-frame 変化 が PerProgram cadence で frame 内 1 回 GPU upload で済むか perf 実測 [要 verify D3]
- ColorGrading + GammaCorrect Cinematic Control cvar 変化頻度 (= per-frame / per-cvar 変化、PerProgram cadence で問題なし想定) [要 verify]

**AYAstorm Cinematic Control 13 cvar 対応**:
- memory `project_r30_cinematic_control_tuning_deferred` r30 Phase 6 で 13 cvar BD live 配線完了
- 本 group ColorGrading 6 member (= saturation / contrast / temperature / brightness / lut_intensity / lut_enabled) と 13 cvar の対応関係 verify 要 [要 verify D4 突合]
- 残 7 cvar の格納先 (= TonemapUBO / GammaCorrect / 別 UBO 候補) [要 verify D4 突合]

**cross-UBO data source 共有 verify**:
- TonemapUBO.exposure ↔ ExposureFParamUBO_Legacy.dynamic_exposure_params (= HDR exposure 計算結果同 source か別 source か) [要 verify D4 突合]
- GammaCorrect.gamma ↔ TonemapUBO.tonemap_mix (= display correction 同 cvar 経路か別経路か) [要 verify D4 突合]
- ColorGrading.color_grading_lut_enabled int 値域 (= boolean 0/1 か他 enum か) [要 verify]
- Vignette vec3 3 component 内訳 (= intensity / radius / softness 等) [要追加調査]

**GammaCorrect 2 shader UBO 共有**:
- postDeferredGammaCorrect + postDeferredTonemap で同 UBO instance 共有 vs 別 instance (= PerProgram cadence 厳密解釈) [要 verify L0-1 dispatch]

**exoVignette upstream**:
- `exo` prefix = Exodus / BlackDragon derivative 由来か AYAstorm 独自か [要 verify]
- 上流 vignette 実装と Vulkan path 整合性 [要 verify]

**bare uniform 残存**:
- 6 shader `#else` block bare uniform setter が OpenGL 経路で host C++ 側に残存しているか [要追加調査]

##### (3) 調査手法

- **D1 setter Grep**: 全 member 名 (= `diffuse_luminance_scale` / `dt` / `noiseVec` / `dynamic_exposure_params` / `dynamic_exposure_params2` / `exposure` / `tonemap_mix` / `tonemap_type` / `gamma` / `color_saturation` / `color_contrast` / `color_temperature` / `color_brightness` / `color_grading_lut_intensity` / `color_grading_lut_enabled` / `vignette`) setter site 全件 (= `indra/newview/pipeline.cpp` + `indra/newview/llpipeline.cpp` + `LLPipeline::renderTonemap` / `renderGammaCorrect` / `applyColorGrading` 経路、推定 20+ site)
- **D1 setter Grep AYA cvar**: AYAstorm r30 Cinematic Control 13 cvar (= memory `project_r30_cinematic_control_tuning_deferred` 参照) + AYAstorm vignette 関連 cvar + `RenderTonemap*` / `RenderDeferredDisplayGamma` 経路
- **D2 既存実装読解**: `LLPipeline::renderPostProcess` chain (= luminance → exposure → tonemap → gamma → color grading → vignette、各 program bind 順序 + setter call 順序)、AYAstorm r30 Cinematic Control 章実装 hook 位置特定 (= memory `project_ayastorm_r30_cinematic_chapter` + `project_r30_cinematic_control_tuning_deferred`)
- **D3 cadence verify**: Exposure noiseVec / TonemapUBO exposure per-frame 変化 vs PerProgram cadence の frame 内 1 回 GPU upload perf 実測
- **D4 突合**:
  - TonemapUBO.exposure ↔ Exposure.dynamic_exposure_params 同 source verify
  - ColorGrading 6 member ↔ AYAstorm r30 Cinematic Control 13 cvar 対応 mapping verify
  - GammaCorrect.gamma ↔ TonemapUBO.tonemap_mix display correction source verify
  - Vignette vec3 3 component 内訳 shader access pattern verify

##### (4) 設計 task (= 4 経路、group 単位)

- **register**: 6 UBO 個別 register、auto-exposure chain 2 UBO (= Luminance + Exposure) は shell 通電済 + write 経路本格化済ゆえ既経路活用、tonemap + display correction + vignette 4 UBO は新規 register 配線、GammaCorrect は 2 shader 共有 instance vs 別 instance dispatch 確定
- **write**: 4 trigger 経路で N UBO 同時 dirty:
  - **trigger 1 auto-exposure update (= per-frame、luminance + exposure chain)**: Luminance (diffuse_luminance_scale) + Exposure (dt + noiseVec + dynamic_exposure_params + dynamic_exposure_params2) 連動 dirty、`pipeline.cpp:8754` Luminance setter + `pipeline.cpp:8863-8865` Exposure setter chain 既存
  - **trigger 2 AYAstorm r30 Cinematic Control 13 cvar 変化**: ColorGrading 6 member + TonemapUBO 3 member + GammaCorrect gamma + Vignette 3 member 連動 dirty (= 13 cvar 配信先 group 内 UBO 全件、setter call 経路は per-cvar、PerProgram cadence で frame 内 stable)
  - **trigger 3 LLEnvironment auto-exposure / sky preset 切替**: TonemapUBO exposure + Exposure dynamic_exposure_params 連動 dirty (= sky-derived HDR offset)
  - **trigger 4 post-process chain dispatch (= per-frame sequential)**: 6 program 順次 bind = luminance → exposure → tonemap → gamma correct → color grading → vignette、各 program bind 時 PerProgram cadence flush
- **flush**: 各 UBO 個別 PerProgram cadence = `flushProgramUbos` (= cmdbuf 経路で triple-buffer 経由 `vkCmdBindDescriptorSets`)、GammaCorrect 2 shader 共有なら 1 instance 2 bind / 別 instance なら 2 setter
- **shader 接続**: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= 6 shader、Luminance `luminanceF.glsl:60` / Exposure `exposureF.glsl:49` / Tonemap `tonemapUtilF.glsl:146` / GammaCorrect `postDeferredGammaCorrect.glsl:45` + `postDeferredTonemap.glsl` / ColorGrading `postDeferredTonemap.glsl:74` / Vignette `exoVignetteF.glsl:42`、改変ゼロ、原則 4 維持)
- **cross-UBO 同期 protocol**: 4 trigger event で group 内該当 UBO 全 dirty (= auto-exposure trigger = 2 UBO + AYAstorm Cinematic Control trigger = 4 UBO + LLEnvironment trigger = 2 UBO + post-process chain dispatch = 6 program sequential)
- **GammaCorrect 2 shader 共有設計**: 1 UBO instance を postDeferredGammaCorrect + postDeferredTonemap で共有 vs 各 program 別 instance (= PerProgram cadence 厳密解釈) **[要 AYA 判断]**
- **AYAstorm r30 Cinematic Control 13 cvar mapping**: ColorGrading 6 member + 残 7 cvar 配信先 (= TonemapUBO / GammaCorrect / 別 UBO) 確定設計 **[要 verify + 要 AYA 判断]**
- **LLStaticHashedString UBO redirect 経路**: Luminance.diffuse_luminance_scale_s name-based setter が UBO redirect 透過確認 (= L0-2 経路 verify)

##### (5) 工程 task

- trace 順内位置: L4 group 10 件目 = §3.5.7-9 完了後、shell 通電済 2 UBO (= Luminance + Exposure) ゆえ通電本格化作業優位、post-process chain pipeline 直結、AYAstorm r30 Cinematic Control 機能維持必須
- group 内 並列性:
  - sub-cluster (a) auto-exposure chain = Luminance + Exposure 並列開発候補 (= 同 pipeline 連動)、shell 通電済ゆえ最先着手可
  - sub-cluster (b) Tonemap = setter 不明、Cinematic Control cvar mapping 確定後着手
  - sub-cluster (c) display correction 2 UBO = GammaCorrect 2 shader 共有 dispatch 確定 + ColorGrading 13 cvar mapping 確定後並列着手
  - sub-cluster (d) Vignette = 独立、setter 特定後着手
- group 間 並列性: §3.5.11 (glow chain) / §3.5.12 (SMAA) / §3.5.13 (pathfinding) / §3.5.14 (GLTF asset) / §3.5.15 (set=2 binding=0 共有残) / §3.5.16 (MultiLight) と並列可、§3.5.9 (reflection probe / IBL) と cross-UBO data source 共有候補 (= TonemapUBO.exposure ↔ Exposure / sub-cluster (b) tonemap consume 経路で reflection cubemap 由来)
- 推定工数: **L** (= 1 日 +、6 UBO + AYAstorm r30 Cinematic Control 13 cvar mapping + GammaCorrect 2 shader 共有 dispatch + 6 path visual verify + AYA 既存機能維持 verify + setter 4 UBO 不明分特定 + post-process chain 6 program sequential dispatch verify)

##### (6) A 確定条件

- mUseUBO ON + 6 shader 活性化 + setter 通電 (= 6 UBO 全件、4 trigger 経路全件)
- AYA live verify (= 全 path visual regression ゼロ §5.4):
  - **auto-exposure 動作** (= 明暗 scene 切替で exposure 自動追従、Luminance + Exposure chain 連動、noiseVec dithering 動作)
  - **AYAstorm r30 Cinematic Control 13 cvar 全件動作維持** (= ColorGrading saturation/contrast/temperature/brightness/LUT + TonemapUBO + GammaCorrect + Vignette 連動、live cvar flip 反映、AYAstorm 視覚表現章 + r30 Cinematic mode 機能維持必須、memory `project_r30_cinematic_control_tuning_deferred` 整合)
  - **tonemap 効果** (= ACES / Reinhard / Linear tonemap_type 切替、tonemap_mix 連続変化、exposure 追従)
  - **gamma correction** (= SRGB display 出力路、`RenderDeferredDisplayGamma` cvar 反映、postDeferredGammaCorrect + postDeferredTonemap 2 path 同期)
  - **color grading** (= saturation/contrast/temperature/brightness 連続変化、LUT enable 切替、LUT intensity 連続変化)
  - **vignette 効果** (= exoVignette vec3 連続変化、final display 直前 pass、AYAstorm 視覚表現章機能維持)
  - **post-process chain sequential dispatch** (= 6 program 順次 bind 整合、frame 内 1 回 GPU upload perf 維持)
- Vulkan validation 0 件 (= 6 UBO + set=2 binding=2/4 独立 + GammaCorrect 2 shader 共有 instance 整合 + Exposure/Luminance name-based UBO redirect)
- cross-UBO 同期 verify (= 4 trigger 全件で N UBO 全 dirty + 値同期、特に AYAstorm Cinematic Control 13 cvar 変化で 4 UBO 連動性 + auto-exposure per-frame で 2 UBO 連動性)
- AYAstorm Cinematic Control 13 cvar mapping 確定 (= 6 ColorGrading member + 残 7 cvar 配信先確定、memory `project_r30_cinematic_control_tuning_deferred` r30 Phase 6 配線継承)
- LLStaticHashedString UBO redirect 経路 verify (= L0-2 経路、Luminance.diffuse_luminance_scale_s name-based setter 透過確認)
- GammaCorrect 2 shader 共有 dispatch 確定 (= 1 instance vs 別 instance、AYA 判断 + PerProgram cadence 厳密解釈 vs data 共通最適化)
- exoVignette upstream 起源確認 (= Exodus / BlackDragon derivative vs AYAstorm 独自、設計原則 (1) Upstream 取り込みやすさ整合判定)
- Vignette vec3 3 component 内訳確定 (= shader access pattern verify、intensity / radius / softness 等の具体内訳)
- bare uniform 残存ゼロ確認 (= 6 shader `#else` block bare uniform setter が host C++ 側に残存しない)
- cadence 再評価結果反映 (= L0-4 結果、Exposure/TonemapUBO per-frame perceived の PerProgram cadence frame 内 1 回 GPU upload perf 維持確認)
- **verify 単位 = group verify** (= 6 UBO 揃って初めて整合 visual、中間状態は暫定 default 値で破綻回避 = post-process chain 部分動作 + Cinematic Control 一部 cvar 反映、AYA literal「現状の見た目とほぼ変わらない描画」継承)

##### (7) 4 原則 gate

- **原則 1 (Core 分散)**: 6 program 個別 = render thread 単独 → ✅、post-process chain sequential dispatch は render thread 内逐次 (= 1 frame 完結)、将来 Core 化候補 = post-process chain を worker thread で実行 + UBO write は per-thread `LLUboRingBuffer` 経路 (= memory `project_ayastorm_r41_design_principles` C1-C6 整合)、cross-UBO 同期 logic は 4 trigger setter call に集約 (= 1 setter call → N UBO write 並列化候補)、設計原則 (2) Core 分散実現整合
- **原則 2 (3 OS 共通)**: ✅ (= Vulkan core spec 1.3 範囲内、set=2 binding=2/4 独立 + GammaCorrect 2 shader 共有は pipeline 単位 stage visibility、MoltenVK Argument Buffer Tier 2 整合)
- **原則 3 (Phase 2/3)**: Phase 2 内、cadence PerProgram 維持 (= auto-exposure per-frame perceived は frame 内 1 回 GPU upload で済む、PerDraw 降格不要)、L0-4 cadence 再分類は cross-UBO data source 共有判定中心 → ✅
- **原則 4 (OpenGL を殺さない)**: 6 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能、exoVignette は upstream derivative 候補ゆえ shader 改変なし維持必須 → ✅
- **visual regression ゼロ**: auto-exposure + tonemap + gamma + color grading + vignette + AYAstorm r30 Cinematic Control 13 cvar 全件描画同一 → AYA live verify、本 group は post-process chain 最終出力直結ゆえ AYAstorm 視覚機能交差最大 (= AYAstorm 視覚表現章 + r30 Cinematic Control 直結、memory `project_ayastorm_visual_realism_chapter` + `project_ayastorm_r30_cinematic_chapter` + `project_r30_cinematic_control_tuning_deferred` + `project_ayastorm_r30_bd_improvement_phase` 整合)、AYA 既存機能 risk 大 group ゆえ実装着手前に 13 cvar mapping 確定必須

---

#### §3.5.11 L4-11: glow chain sequential pipeline 4 UBO group (= extract → blur (V/F) → combine、全 4 UBO shell + write 通電済)

**AYA literal 命名 mapping**: READINESS §3.11 (= glow chain sequential pipeline 52-55 = 4 UBO、AYA 単純配列で C-52〜C-55 相当)

**位置付け**: L4 group 11 件目 = glow chain post-process pipeline 経路、**全 4 UBO shell 通電済 + write 経路本格化済** (= Phase 1.A PA-8 + 1.C PC-7γ-1、本 group 最先進)、setter 全件特定済 (= `pipeline.cpp:9061-9068/9116/9134/9138/9589-9597/10706-10715` literal、計 16 setter site)、sequential dispatch 4 pass (= extract → blur V/F → combine)、horizontal/vertical 2 pass dispatch (= GlowV.glowDelta 切替で 2 回 GPU upload)、color grading 後段 (= greyscale / sepia / posterize、`pipeline.cpp` 9589/9595 active/disable + 10706/10713 別 path)、AYA 既存機能交差 = AYAstorm 視覚表現章 r14+ glow 経路 (= memory `project_ayastorm_visual_realism_chapter`、ただし glow chain 自体は LL upstream 標準、AYAstorm 独自拡張は別 UBO 候補)

**group 概要** (= 3 sub-cluster 4 UBO、cross-UBO 同期 protocol 4 trigger 由来):

**sub-cluster (a) glow extract 1 UBO** (= HDR scene → glow source 抽出):
- **GlowExtractFParamUBO_Legacy** (set=3 binding=20、256B std140 48B、5 member = `lumWeights` vec3 offset=0 + `minLuminance` float offset=12 + `warmthWeights` vec3 offset=16 + `maxExtractAlpha` float offset=28 + `warmthAmount` float offset=32) — `class1/effects/glowExtractF.glsl:71` singleton site、shell 通電済 + write 経路本格化済、setter 5 site:
  - `pipeline.cpp:9061` literal `gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_MIN_LUMINANCE, RenderGlowMinLuminance);`
  - `pipeline.cpp:9063` literal `gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_MAX_EXTRACT_ALPHA, maxAlpha);`
  - `pipeline.cpp:9064-9065` literal `gGlowExtractProgram.uniform3f(LLShaderMgr::GLOW_LUM_WEIGHTS, lumWeights.mV[0], lumWeights.mV[1], lumWeights.mV[2]);`
  - `pipeline.cpp:9066-9067` literal `gGlowExtractProgram.uniform3f(LLShaderMgr::GLOW_WARMTH_WEIGHTS, warmthWeights.mV[0], warmthWeights.mV[1], warmthWeights.mV[2]);`
  - `pipeline.cpp:9068` literal `gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_WARMTH_AMOUNT, warmthAmount);`
- data source: `RenderGlowLumWeights` cvar + `RenderGlowWarmthWeights` cvar + `RenderGlowWarmthAmount` cvar + `RenderGlowMinLuminance` cvar + `maxAlpha` local 変数 (= `pipeline.cpp:9063` derive 元 verify 要 [要追加調査])
- reserved 登録: `llshadermgr.cpp:1638-1642` literal + `llshadermgr.h:160-164` enum 全件確認、`llpostprocess.cpp:39` `sLumWeights` 別 path 参照 (= 旧 post-process 経路の残骸 vs active path verify 要 [要 verify])
- **vec3 + float tail packing**: `vec3 lumWeights; float minLuminance;` (= offset 0-12 + 12-16) + 同 pattern `warmthWeights / maxExtractAlpha` + 末尾 `warmthAmount`、host C++ で LLVector3 mV[3] 直接 memcpy 時 align 注意要 [要 verify]
- consume: HDR scene から glow source 抽出 (= luminance weight + warmth tint + min/max threshold)

**sub-cluster (b) glow blur V/F pair 2 UBO** (= horizontal/vertical 2 pass dispatch、V/F 連動 dirty):
- **GlowVParamUBO_Legacy** (set=3 binding=19、256B std140 8B、1 member = `glowDelta` vec2 offset=0、**248B dead space**) — `class1/effects/glowV.glsl:54` singleton site、shell 通電済 + write 経路本格化済、setter 2 site (= horizontal/vertical):
  - `pipeline.cpp:9134` literal `gGlowProgram.uniform2f(LLShaderMgr::GLOW_DELTA, delta, 0);` (= horizontal pass `(delta, 0)`)
  - `pipeline.cpp:9138` literal `gGlowProgram.uniform2f(LLShaderMgr::GLOW_DELTA, 0, delta);` (= vertical pass `(0, delta)`)
- **GlowFParamUBO_Legacy** (set=3 binding=18、256B std140 4B、1 member = `glowStrength` float offset=0、**最小 member UBO**、252B dead space) — `class1/effects/glowF.glsl:39` singleton site、shell 通電済 + write 経路本格化済、setter `pipeline.cpp:9116` literal `gGlowProgram.uniform1f(LLShaderMgr::GLOW_STRENGTH, strength);`
- data source:
  - GlowV.glowDelta = horizontal `(delta, 0)` / vertical `(0, delta)` 切替、`delta` derive 元 = 推定 `RenderGlowSize` cvar 周辺 (= verify 要 [要追加調査])
  - GlowF.glowStrength = `strength` local 変数、derive 元 = 推定 `RenderGlowStrength` cvar (= verify 要 [要追加調査])
- reserved 登録: GlowV `llshadermgr.cpp:1644` + `llshadermgr.h:166` GLOW_DELTA literal / GlowF `llshadermgr.cpp:1643` + `llshadermgr.h:165` GLOW_STRENGTH literal
- **horizontal/vertical 2 pass dispatch protocol**:
  - 同 `gGlowProgram` 内で 2 回 setter 連続呼出 (= horizontal pass dispatch → vertical pass dispatch)
  - GlowV.glowDelta: 2 回 setter 切替 (= horizontal `(delta, 0)` → flush → dispatch → vertical `(0, delta)` → flush → dispatch、2 回 GPU upload 必須)
  - GlowF.glowStrength: 1 回設定で 2 pass 共通 (= 同 frame 内変化なし、horizontal/vertical 同値)
  - = GlowV は 2 pass で別値ゆえ flush 必須、GlowF は 1 値で 2 pass 共有
- **V/F pair 連動 dirty**: 同 blur pass で 2 UBO 同時 consume (= vertex glowV + fragment glowF)、連動 dirty trigger 経路 verify 要 [要 verify]
- consume: glow blur pass (= vertex shader glowDelta で sampling offset 計算、fragment shader glowStrength で blur intensity scale)

**sub-cluster (c) glow combine 1 UBO** (= color grading post-process 後段):
- **GlowCombineFParamUBO_Legacy** (set=3 binding=49、256B std140 16B、4 member = `greyscale_str` float offset=0 + `sepia_str` float offset=4 + `num_colors` float offset=8 + `_pad_glowcombine_f_legacy_0` float offset=12) — `class1/interface/glowcombineF.glsl:44` singleton site、shell 通電済 + write 経路本格化済、setter 8 site (= 2 path 重複、active/disable):
  - `pipeline.cpp:9589/10706` literal `uniform1f(DEFERRED_GREYSCALE_STRENGTH, RenderGreyscaleStrength)` (= active path)
  - `pipeline.cpp:9595/10713` literal `uniform1f(DEFERRED_GREYSCALE_STRENGTH, 0.0f)` (= disable path)
  - `pipeline.cpp:9590/10707` literal `uniform1f(DEFERRED_SEPIA_STRENGTH, RenderSepiaStrength)` + `:9596/10714` `0.0f` disable
  - `pipeline.cpp:9591/10708` literal `uniform1f(DEFERRED_NUM_COLORS, (GLfloat)RenderNumColors)` + `:9597/10715` `1.0f` disable
- data source: `RenderGreyscaleStrength` cvar + `RenderSepiaStrength` cvar + `RenderNumColors` cvar
- reserved 登録: `llshadermgr.cpp:1896-1898` literal `"sepia_str"` / `"greyscale_str"` / `"num_colors"` + `llshadermgr.h:417-419` DEFERRED_SEPIA_STRENGTH / DEFERRED_GREYSCALE_STRENGTH / DEFERRED_NUM_COLORS enum literal
- **enum comment vs reserved string 不一致** (= `llshadermgr.h:417,418` enum comment は `sepia_strength` / `greyscale_strength` だが reserved string + blueprint member 名は `sepia_str` / `greyscale_str`、shader UBO member 名と一致、enum comment が古い表記、本 UBO scope 外修正候補)
- **setter 4 path 重複**:
  - `pipeline.cpp:9589-9597` (= gGlowCombineProgram 専属、active + disable path)
  - `pipeline.cpp:10706-10715` (= shader 引数版、別 path、specific shader **[要追加調査]**)
  - 両 path とも PER_PROGRAM case 経由で writeProgramUbo に集約、shader 切替で dirty 重複なし (= `sProgramUboDirty` key = shader × block_hash)
- consume: glow combine post-process (= color grading = greyscale + sepia tint + posterize、glow chain 最終段)

##### (1) 前提条件

- L0-1 (= dispatch logic、特に set=3 binding=18/19/20/49 衝突なし独立 binding)
- L0-2 (= LLStaticHashedString UBO redirect 経路、`llpostprocess.cpp:39` `sLumWeights` 別 path 参照 verify、本 UBO redirect で集約されるか確認)
- L0-3 (= per-shader UBO block 拡大、blur pass V/F pair 連動 dirty 経路、horizontal/vertical 2 dispatch 間 flush timing 経路)
- L0-4 (= cadence 妥当性、cadence PerProgram 維持 = glow chain 全件 frame 内 1 回連続 dispatch、stale risk なし、horizontal/vertical 2 pass は GlowV 2 回 GPU upload 必須だが per-pass dirty で完結)
- L1a-1 (= ClipFParamUBO_Legacy LLStaticHashedString redirect pilot 完了)
- L1b 完了 (= FrameViewProj.screen_res 経由 GlowExtract DEFERRED_SCREEN_RES 同 shader 内同時 consume)
- Phase 1.A PA-8 + 1.C PC-7γ-1 (= 全 4 UBO shell 通電済 + write 経路本格化済、本 group 最先進)
- §3.5.10 完了 (= post-process chain pipeline、glow chain pre/post-step、cross-UBO data source 共有候補 verify)

##### (2) 不明事項

**setter derive 元不明**:
- GlowExtract `maxAlpha` local 変数の derive 元 (= `pipeline.cpp:9063` literal) **[要追加調査]**
- GlowV `delta` local 変数の derive 元 (= `pipeline.cpp:9134` literal、推定 `RenderGlowSize` cvar) **[要追加調査]**
- GlowF `strength` local 変数の derive 元 (= `pipeline.cpp:9116` literal、推定 `RenderGlowStrength` cvar) **[要追加調査]**
- GlowCombine `pipeline.cpp:10706-10715` shader 引数版の specific shader (= `gGlowCombineProgram` 以外で本 UBO consume する shader 特定要) **[要追加調査]**

**LLStaticHashedString 別 path verify**:
- `llpostprocess.cpp:39` `sLumWeights` 別 path 参照 (= 旧 post-process 経路の残骸 vs active path) **[要 verify]**

**horizontal/vertical 2 pass dispatch timing**:
- GlowV.glowDelta 2 回 setter 切替 (= horizontal → flush → dispatch → vertical → flush → dispatch) の flush timing 経路実装 verify (= dirty store 連続上書き回避、2 回 GPU upload 必須) **[要 verify]**
- V/F pair 連動 dirty trigger 経路 (= 同 blur pass で 2 UBO 同時 consume、独立 dirty vs 連動 dirty 設計) **[要 verify]**

**std140 packing 動作確認**:
- GlowExtract vec3 + float tail packing (= `vec3 lumWeights; float minLuminance;` 等、host C++ で LLVector3 mV[3] 直接 memcpy 時 stride 違反なし) **[要 verify]**

**dead space 活用検討**:
- GlowV 248B dead space + GlowF 252B dead space + GlowCombine 240B dead space (= Phase 2 で glow chain 関連 param 追加候補、layout 不変前提) **[要 AYA 判断]**

**set 3 bind 単位**:
- program 切替時 set 3 全 ~58 UBO 同時 bind か binding 単位 rebind か (= 全 group 共通課題、本 group 4 UBO 全 set=3 帯) **[要 verify]**

**bare uniform 残存**:
- 4 shader `#else` block bare uniform setter が OpenGL 経路で host C++ 側に残存しているか [要追加調査]

##### (3) 調査手法

- **D1 setter Grep**: 全 reserved uniform name (= `GLOW_MIN_LUMINANCE` / `GLOW_MAX_EXTRACT_ALPHA` / `GLOW_LUM_WEIGHTS` / `GLOW_WARMTH_WEIGHTS` / `GLOW_WARMTH_AMOUNT` / `GLOW_DELTA` / `GLOW_STRENGTH` / `DEFERRED_GREYSCALE_STRENGTH` / `DEFERRED_SEPIA_STRENGTH` / `DEFERRED_NUM_COLORS`) setter site 全件確認 (= 既 16 site 特定済、追加 site verify) + local 変数 (`maxAlpha` / `delta` / `strength`) derive 元 grep
- **D1 setter Grep AYA cvar**: `RenderGlow*` (= LumWeights / WarmthWeights / WarmthAmount / MinLuminance / Size / Strength) + `RenderGreyscaleStrength` / `RenderSepiaStrength` / `RenderNumColors` 経路 + `llpostprocess.cpp:39` `sLumWeights` 別 path 経路
- **D2 既存実装読解**: `LLPipeline::renderPostProcess` glow chain (= `pipeline.cpp:9049-9099` extract + `:9100-9140` blur + `:9583-9597 / :10706-10715` combine、各 pass program bind 順序 + setter call 順序)、horizontal/vertical 2 dispatch flush timing 経路 (= shader × block_hash key dirty store + flush 経路)
- **D3 cadence verify**: horizontal/vertical 2 pass dispatch で GlowV 2 回 GPU upload 必須確認 (= dirty store 連続上書きでなく flush 必須、`sProgramUboDirty` key 共有性 verify)
- **D4 突合**:
  - V/F pair (= GlowV + GlowF) 同 blur pass で連動 dirty trigger 共有 verify
  - GlowCombine 4 path setter (= active / disable × 2 path) 全 PER_PROGRAM case 集約 verify
  - GlowExtract std140 vec3 + float tail packing 動作 verify

##### (4) 設計 task (= 4 経路、group 単位)

- **register**: 4 UBO 個別 register、全 shell 通電済 + write 経路本格化済ゆえ既経路活用、追加 register 配線不要 (= 既 Phase 1.A PA-8 + 1.C PC-7γ-1 経路継承)
- **write**: 4 trigger 経路で N UBO 順次 dirty:
  - **trigger 1 glow extract pass (= post-process chain 第 1 段)**: GlowExtract 単独 dirty (= 5 setter call sequential、`pipeline.cpp:9061-9068`)
  - **trigger 2 glow blur horizontal pass (= 第 2 段 horizontal)**: GlowV `(delta, 0)` + GlowF `glowStrength` 連動 dirty + flush + dispatch
  - **trigger 3 glow blur vertical pass (= 第 3 段 vertical)**: GlowV `(0, delta)` 切替 dirty + flush + dispatch (GlowF は同値再 set 不要)
  - **trigger 4 glow combine pass (= 第 4 段)**: GlowCombine 単独 dirty (= 3 setter call、active/disable path 切替)
- **flush**: 各 UBO 個別 PerProgram cadence = `flushProgramUbos` (= cmdbuf 経路で triple-buffer 経由 `vkCmdBindDescriptorSets`)、特に GlowV は horizontal/vertical 2 pass 間 flush 必須 (= 2 回 GPU upload)
- **shader 接続**: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= 4 shader、GlowExtract `glowExtractF.glsl:71` / GlowV `glowV.glsl:54` / GlowF `glowF.glsl:39` / GlowCombine `glowcombineF.glsl:44`、改変ゼロ、原則 4 維持)
- **cross-UBO 同期 protocol**: 4 trigger event で group 内該当 UBO sequential dirty (= post-process chain sequential dispatch、各 trigger 独立 program bind ゆえ shader × block_hash key で独立)
- **horizontal/vertical 2 dispatch flush timing 設計**: GlowV.glowDelta 2 回切替で 2 回 GPU upload 必須、`sProgramUboDirty` 連続上書き回避、horizontal dispatch → flush → vertical write → flush → dispatch 順序確立
- **V/F pair 連動 dirty 設計**: 同 blur pass で GlowV + GlowF 同時 consume、独立 dirty trigger vs 連動 dirty (= cross-UBO 同期 logic、blur pass 内 2 UBO 同時 dirty bit) **[要 verify]**
- **enum comment 修正**: `llshadermgr.h:417,418` enum comment 古い表記 (`sepia_strength` / `greyscale_strength`) → 修正候補だが本 UBO scope 外 (= 別 chore commit) **[要 AYA 判断]**

##### (5) 工程 task

- trace 順内位置: L4 group 11 件目 = §3.5.7-10 完了後、全 4 UBO shell 通電済 + write 経路本格化済 + setter 全件特定済ゆえ **本 group 最先進 = 通電本格化作業最短**、§3.5.10 post-process chain pipeline 完了後 dispatch sequential 整合確認
- group 内 並列性:
  - sub-cluster (a) GlowExtract = 独立着手可、5 setter 全件特定済ゆえ即着手可
  - sub-cluster (b) GlowV + GlowF = V/F pair 並列開発候補 (= 同 blur pass)、horizontal/vertical 2 dispatch flush timing 確立後着手
  - sub-cluster (c) GlowCombine = 独立着手可、8 setter site 全件特定済
- group 間 並列性: §3.5.12 (SMAA) / §3.5.13 (pathfinding) / §3.5.14 (GLTF asset) / §3.5.15 (set=2 binding=0 共有残) / §3.5.16 (MultiLight) と並列可、§3.5.10 (post-process) と post-process chain sequential dispatch 整合
- 推定工数: **M** (= 半日、4 UBO 全件 shell + write 通電済ゆえ通電本格化 + horizontal/vertical 2 dispatch flush timing 確立 + V/F pair 連動 dirty verify + cold launch 検証 + 4 path visual verify)

##### (6) A 確定条件

- mUseUBO ON + 4 shader 活性化 + setter 通電 (= 4 UBO 全件、4 trigger 経路全件)
- AYA live verify (= 全 path visual regression ゼロ §5.4):
  - **glow extract 動作** (= HDR scene から glow source 抽出、luminance weight + warmth tint + min/max threshold 正確、`RenderGlow*` cvar 反映)
  - **glow blur horizontal/vertical 2 pass 整合** (= GlowV.glowDelta horizontal `(delta, 0)` → vertical `(0, delta)` 切替で blur 縦横正確、GlowF.glowStrength 共有値で 2 pass 整合)
  - **V/F pair 連動 dirty** (= 同 blur pass で 2 UBO 同時 consume、blur pass 内 stale data なし)
  - **glow combine 動作** (= greyscale + sepia + posterize 連動、active/disable path 切替正確、`RenderGreyscaleStrength` / `RenderSepiaStrength` / `RenderNumColors` cvar 反映)
  - **sequential dispatch 整合** (= extract → blur (horizontal V/F) → blur (vertical V/F) → combine 4 pass 順次 dispatch、frame 内 1 回完結)
- Vulkan validation 0 件 (= 4 UBO + horizontal/vertical 2 pass 2 回 GPU upload + V/F pair 同 binding 衝突なし + std140 vec3+float tail packing)
- cross-UBO 同期 verify (= 4 trigger 全件で N UBO sequential dirty + 値同期、特に V/F pair 連動 dirty + horizontal/vertical 2 dispatch flush timing)
- LLStaticHashedString 別 path verify (= `llpostprocess.cpp:39` `sLumWeights` 旧 path vs active path 判定)
- setter derive 元確定 (= GlowExtract `maxAlpha` / GlowV `delta` / GlowF `strength` local 変数 derive 元)
- GlowCombine `pipeline.cpp:10706-10715` shader 引数版 specific shader 特定
- horizontal/vertical 2 dispatch flush timing 確立 (= GlowV 2 回 GPU upload 必須、`sProgramUboDirty` 連続上書き回避)
- std140 vec3 + float tail packing 動作確認 (= GlowExtract host C++ LLVector3 mV[3] 直接 memcpy 時 stride 違反なし)
- bare uniform 残存ゼロ確認 (= 4 shader `#else` block bare uniform setter が host C++ 側に残存しない)
- cadence PerProgram 維持確認 (= glow chain 全件 frame 内 1 回連続 dispatch、stale risk なし)
- **verify 単位 = group verify** (= 4 UBO 揃って初めて整合 visual、中間状態は暫定 default 値で破綻回避 = glow 効果無効化、AYA literal「現状の見た目とほぼ変わらない描画」継承)

##### (7) 4 原則 gate

- **原則 1 (Core 分散)**: 4 program 個別 = render thread 単独 → ✅、glow chain sequential dispatch は render thread 内逐次 (= 1 frame 完結)、将来 Core 化候補 = glow chain を worker thread で実行 + UBO write は per-thread `LLUboRingBuffer` 経路、cross-UBO 同期 logic は 4 trigger setter call に集約 (= sequential dispatch 4 pass、V/F pair 連動 dirty は同 blur pass 内 集約)、設計原則 (2) Core 分散実現整合
- **原則 2 (3 OS 共通)**: ✅ (= Vulkan core spec 1.3 範囲内、horizontal/vertical 2 pass dispatch + V/F pair 同 binding 共有なし binding 独立 + std140 vec3+float tail packing は MoltenVK 整合)
- **原則 3 (Phase 2/3)**: Phase 2 内、cadence PerProgram 維持 (= glow chain frame 内 1 回連続 dispatch、stale risk なし)、L0-4 cadence 再分類対象なし → ✅
- **原則 4 (OpenGL を殺さない)**: 4 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能、glow chain は LL upstream 標準ゆえ shader 改変なし維持 → ✅
- **visual regression ゼロ**: glow extract + blur (horizontal/vertical) + combine 描画同一 + greyscale/sepia/posterize 効果維持 → AYA live verify、本 group は LL upstream 標準 glow chain ゆえ AYAstorm 視覚機能交差軽微、ただし AYAstorm r14+ 視覚表現章 glow 経路と連動候補 verify (= memory `project_ayastorm_visual_realism_chapter`)、enum comment 古い表記の修正は本 UBO scope 外別 chore

---

#### §3.5.12 L4-12: SMAA pass chain 2 UBO group (= edge detection → blend weights → neighborhood blending、shared include SMAA.glsl)

**AYA literal 命名 mapping**: READINESS §3.12 (= SMAA pass chain pipeline 56-57 = 2 UBO、AYA 単純配列で C-56〜C-57 相当)

**位置付け**: L4 group 12 件目 = SMAA (Subpixel Morphological Antialiasing) pass chain 経路、SMAA.glsl shared include で 3 pass 共通使用、screen resize 連動 + temporal SMAA frame swap 連動、setter 全件不明 [要追加調査]、AYAstorm 視覚機能交差 = AA pipeline (= upstream LL 標準、AYAstorm 独自拡張 verify 要)

**group 概要** (= 2 sub-cluster 2 UBO):

**sub-cluster (a) RT metrics 1 UBO** (= SMAA.glsl shared include 経由 pass chain 全 pass 共通):
- **SMAAParamUBO_Legacy** (set=3 binding=14、256B std140 16B、1 member = `SMAA_RT_METRICS` vec4 offset=0 = `(1/screen_w, 1/screen_h, screen_w, screen_h)`) — `class1/deferred/SMAA.glsl:41` shared include site (= edge detection + blend weights + neighborhood blending 3 pass 共通 consume、verify 要)、setter 不明 [要追加調査]
- data source: screen resolution (= `gViewerWindow` size or render target size、verify 要)、process-wide で resize 時のみ実 update
- **cadence 妥当性 question**: 全 SMAA program で同値共有 + resize 時のみ update = cadence=PerFrame or SINGLETON 候補だが現状 PerProgram (= ubo_metadata.inl:100 literal)、Phase 2 設計で cadence 再検討余地有り得る、ただし shell layout 不変契約遵守 [要 L0-4 結果反映 / 要 AYA 判断]
- screen res 由来 UBO 群 (= ScreenSpaceReflPostFParamUBO_Legacy / CASParamUBO_Legacy 等) との同時 dirty 連動候補 [要 verify D4 突合]

**sub-cluster (b) blend weights 1 UBO** (= temporal SMAA subsample index、blend weights pass 専用):
- **SMAABlendWeightsFParamUBO_Legacy** (set=3 binding=62、256B std140 16B、1 member = `subsampleIndices` vec4 offset=0、SMAA 2x/4x subsample blending 用 index) — `class1/deferred/SMAABlendWeightsF.glsl:67` singleton site、setter 不明 [要追加調査]
- data source: SMAA temporal 用 subsample index (= 2x SMAA で 2 frame jitter、4x で 4 frame、verify 要)、temporal SMAA enable 経路 (= AYAstorm 側 temporal SMAA on/off cvar 候補) [要追加調査]
- **temporal SMAA dirty pattern risk**: SMAA 2x/4x で subsample index が frame 毎更新、PerProgram cadence で 1 frame 内 dirty 1 回対応可能だが temporal 経路 verify 必要 [要 verify D3]

##### (1) 前提条件

- L0-1 (= dispatch logic、衝突なし binding=14/62 独立)
- L0-3 (= per-shader UBO block 拡大、SMAA.glsl shared include で 3 pass 共通 consume 経路 verify)
- L0-4 (= cadence 妥当性、SMAAParam cadence PerFrame/SINGLETON 候補 + SMAABlendWeights temporal SMAA per-frame dirty pattern)
- L1b 完了 (= FrameViewProj.screen_res 同 source 由来候補、cross-UBO data source 共有 verify)
- §3.5.10 完了 (= post-process chain、SMAA は AA pass の独立 pipeline)

##### (2) 不明事項

**setter 経路不明**:
- SMAAParam SMAA_RT_METRICS setter (= `uniform4fv(SMAA_RT_METRICS, vec4(1/w, 1/h, w, h))` 経路、viewer resize callback) **[要追加調査]**
- SMAABlendWeights subsampleIndices setter (= `uniform4fv(subsampleIndices, ...)` 経路、temporal SMAA pass) **[要追加調査]**

**cadence 妥当性**:
- SMAAParam PerProgram → PerFrame/SINGLETON 降格検討 (= process-wide resize 時のみ update、全 SMAA program 同値共有、PerProgram 73 件中 1 件として割当て続けるか別 cadence 化) **[要 L0-4 結果反映 / 要 AYA 判断]**
- SMAABlendWeights temporal SMAA per-frame dirty pattern (= PerProgram cadence で frame 内 1 値、stale risk なし想定) **[要 verify]**

**SMAA pass chain 構成**:
- SMAA.glsl shared include で edge detection + blend weights + neighborhood blending 3 pass 全 program で同 UBO consume 確認 [要 verify]
- SMAA pass chain 全 UBO 一覧 (= 各 pass の UBO 配置、本 group 2 UBO のみか他 UBO 候補) [要 verify]

**temporal SMAA**:
- temporal SMAA enable 経路 (= AYAstorm temporal SMAA cvar 候補) [要追加調査]
- subsampleIndices owner (= SMAA mode 1x/2x/4x + temporal frame index、verify 要) [要 verify]

**screen res 由来 cross-UBO 連動**:
- viewer resize 時 SMAAParam + 他 screen res 由来 UBO (= ScreenSpaceReflPostF / CASParam 等) 同時 dirty 連動 [要 verify D4 突合]

**bare uniform 残存**:
- 2 shader `#else` block bare uniform setter が OpenGL 経路で host C++ 側に残存しているか [要追加調査]

##### (3) 調査手法

- **D1 setter Grep**: `SMAA_RT_METRICS` / `subsampleIndices` setter site 全件 (= `indra/newview/pipeline.cpp` + SMAA post-process 経路、推定 < 5 site)
- **D2 既存実装読解**: `LLPipeline::renderPostProcess` SMAA pass chain (= edge → blend weights → neighborhood blending sequential dispatch + SMAA.glsl shared include 構造)
- **D3 cadence verify**: SMAAParam resize 時のみ update vs PerProgram 73 件中 1 件 cadence 整合 + SMAABlendWeights temporal per-frame dirty pattern
- **D4 突合**:
  - SMAAParam ↔ FrameViewProj.screen_res 同 source verify
  - SMAAParam ↔ ScreenSpaceReflPostF/CASParam screen res 由来 cross-UBO 同期 verify
  - SMAA pass chain 3 pass 全 program で SMAAParam 同 binding consume verify

##### (4) 設計 task (= 4 経路、group 単位)

- **register**: 2 UBO 個別 register、SMAAParam は SMAA.glsl shared include 経由 SMAA pass chain 3 program 全件で wrap、SMAABlendWeights は blend weights pass 専用
- **write**: 2 trigger 経路で N UBO 同時 dirty:
  - **trigger 1 viewer resize (= screen res 変化、SMAAParam 単独 dirty)**: SMAA pass chain 3 program 全件で SMAA_RT_METRICS 同 dirty、screen res 由来他 UBO (= ScreenSpaceReflPostF/CASParam) と連動候補
  - **trigger 2 temporal SMAA frame swap (= SMAABlendWeights per-frame dirty)**: SMAABlendWeights subsampleIndices 更新、SMAA 2x/4x temporal jitter
- **flush**: 各 UBO 個別 PerProgram cadence = `flushProgramUbos`、SMAAParam は SMAA pass chain 全 program で共有 instance (= cadence 適切なら 1 instance 多 bind) [要 AYA 判断]
- **shader 接続**: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= 2 shader、SMAAParam `SMAA.glsl:41` shared include / SMAABlendWeights `SMAABlendWeightsF.glsl:67`、改変ゼロ、原則 4 維持)
- **cross-UBO 同期 protocol**: viewer resize trigger で SMAAParam + screen res 由来 cross-UBO 全 dirty + temporal SMAA trigger で SMAABlendWeights 単独 dirty
- **cadence 再分類検討**: SMAAParam PerProgram → PerFrame/SINGLETON 降格判断 **[要 AYA 判断]**、ただし shell layout 不変契約遵守

##### (5) 工程 task

- trace 順内位置: L4 group 12 件目 = §3.5.11 完了後、SMAA pass chain 独立、small group ゆえ快速着手可
- group 内 並列性: 2 UBO 並列開発候補 (= 同 SMAA pipeline)
- group 間 並列性: §3.5.13 (pathfinding) / §3.5.14 (GLTF asset) / §3.5.15 (set=2 binding=0 共有残) / §3.5.16 (MultiLight) と並列可
- 推定工数: **S-M** (= 数時間〜半日、2 UBO + cadence 再分類検討 + setter 特定 + SMAA pass chain shared include verify + temporal SMAA verify + 2 path visual verify)

##### (6) A 確定条件

- mUseUBO ON + 2 shader 活性化 + setter 通電 (= 2 UBO 全件、2 trigger 経路)
- AYA live verify (= 全 path visual regression ゼロ §5.4):
  - **SMAA edge detection 動作** (= AA エッジ抽出、SMAA_RT_METRICS screen res 反映)
  - **SMAA blend weights 動作** (= subpixel weighting、subsampleIndices 反映)
  - **SMAA neighborhood blending 動作** (= 最終 AA 合成、SMAA pass chain 3 pass sequential 整合)
  - **viewer resize 追従** (= screen res 変化で SMAA_RT_METRICS 更新、AA 効果維持)
  - **temporal SMAA 動作** (= 2x/4x subsample jitter、subsampleIndices frame 切替反映)
- Vulkan validation 0 件 (= 2 UBO + 衝突なし + std140 vec4 packing)
- cross-UBO 同期 verify (= viewer resize trigger で SMAAParam + screen res 由来 UBO 連動 dirty + temporal SMAA trigger で SMAABlendWeights per-frame dirty)
- cadence 再分類結果反映 (= L0-4 結果、SMAAParam PerProgram 維持 vs PerFrame/SINGLETON 降格判断)
- SMAA pass chain shared include verify (= SMAA.glsl shared include で 3 pass 全 program に SMAAParam 同 binding consume 確認)
- bare uniform 残存ゼロ確認
- **verify 単位 = group verify** (= 2 UBO 揃って初めて整合 visual、中間状態は SMAA 効果無効化 = AA なし描画継続、AYA literal「現状の見た目とほぼ変わらない描画」継承)

##### (7) 4 原則 gate

- **原則 1 (Core 分散)**: 3 program 個別 (= edge / blend weights / neighborhood) = render thread 単独 → ✅、SMAA.glsl shared include で UBO bind 共有、cross-UBO 同期 logic は 2 trigger setter call に集約、設計原則 (2) Core 分散実現整合
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内、cadence PerProgram 維持 vs PerFrame/SINGLETON 降格 L0-4 結果依存 → ✅
- **原則 4 (OpenGL を殺さない)**: 2 shader `#else` block uniform 個別宣言維持 → ✅
- **visual regression ゼロ**: SMAA 3 pass + temporal jitter + viewer resize 追従 描画同一 → AYA live verify、本 group は LL upstream 標準 SMAA ゆえ AYAstorm 視覚機能交差軽微、ただし AA 効果は AYAstorm 撮影撮影描画章 (= r30+ Cinematic) 連動候補 verify

---

#### §3.5.13 L4-13: pathfinding visualization 2 UBO group (= pathfindingV + pathfindingNoNormalV、debug 用途)

**AYA literal 命名 mapping**: READINESS §3.13 (= pathfinding debug pair 58-59 = 2 UBO、AYA 単純配列で C-58〜C-59 相当)

**位置付け**: L4 group 13 件目 = pathfinding visualization (= debug 描画) pair、debug menu enable 時のみ active、優先度低、tint/alpha_scale 共有 + ambiance 有無で 2 UBO に分離設計、本 group 最小規模、AYAstorm 視覚機能交差なし (= LL upstream debug 機能)

**group 概要** (= 1 sub-cluster 2 UBO、program 識別 dispatch):

- **PathfindingVParamUBO_Legacy** (set=3 binding=47、256B std140 16B、4 member = `tint` float offset=0 + `ambiance` float offset=4 + `alpha_scale` float offset=8 + `_pad_pathfinding_v_legacy_0` float offset=12) — `class1/interface/pathfindingV.glsl:70` singleton site (= with normal lighting + ambiance、`pathfindingV.glsl:94` literal `lit = clamp(lit, ambiance, 1.0);` + `:96` literal `vertex_color = vec4(diffuse_color.rgb * tint * lit, diffuse_color.a*alpha_scale);`)、setter 不明 [要追加調査]、reserved `ambiance` = `llglslshader.cpp:1078` reserved list 共通登録
- **PathfindingNoNormalVParamUBO_Legacy** (set=3 binding=48、256B std140 16B、4 member = `tint` float offset=0 + `alpha_scale` float offset=4 + pad ×2 offset=8-12) — `class1/interface/pathfindingNoNormalV.glsl:65` singleton site (= no normal lighting、`pathfindingNoNormalV.glsl:80` literal `vertex_color = vec4(diffuse_color.rgb * tint, diffuse_color.a*alpha_scale);`)、setter 不明 [要追加調査]
- data source: pathfinding visualization (= `LLPathfindingNavMesh` / `LLPathfindingPathTool` 等候補、verify 要)、navmesh / walkability layer 別 tint/alpha
- **2 UBO 分離設計**: ambiance 有無で 2 UBO 別配置 (= with-lighting program → ambiance 必要、no-lighting program → ambiance 不要)、tint/alpha_scale 共有 = 同 data source 由来 (= setter call site 共通可能性大 [要 verify D4 突合])
- **`ambiance` 名衝突 risk**: 他用途 `proj_ambiance` (`llshadermgr.cpp:1559`) / `reflection_probe_ambiance` (`llshadermgr.cpp:1833`) と reserved uniform handling 整合性、pathfinding 専用 `ambiance` (= prefix なし) との衝突回避 [要 verify]

##### (1) 前提条件

- L0-1 (= dispatch logic、program 識別で 2 UBO 別 wire、tint/alpha_scale 名衝突回避 + ambiance 名衝突回避)
- L0-2 (= LLStaticHashedString UBO redirect 経路、共通 reserved `ambiance` 名 dispatch)
- §3.5.10 / §3.5.11 完了 (= post-process chain、本 group は独立 debug pipeline)

##### (2) 不明事項

**setter 経路不明 (= 2 UBO 共通)**:
- tint / ambiance / alpha_scale setter (= `uniform1f` 経路、pathfinding visualization dispatcher) **[要追加調査]**
- pathfinding visualization dispatcher (= `LLPathfindingPathTool` / `LLFloaterPathfindingConsole` 等) **[要追加調査]**

**debug menu trigger**:
- pathfinding 描画 enable cvar / menu item 経路 (= debug 用途、通常 frame 不使用) [要追加調査]

**ambiance 名衝突 verify**:
- `proj_ambiance` / `reflection_probe_ambiance` 等 reserved との衝突回避経路 (= host C++ 側 reserved uniform handling) [要 verify]

**2 UBO data source 共有**:
- tint/alpha_scale 共有 setter (= 同 setter call で 2 UBO 同時 dirty vs 別経路) [要 verify D4 突合]

**bare uniform 残存**:
- 2 shader `#else` block bare uniform setter が残存しているか [要追加調査]

##### (3) 調査手法

- **D1 setter Grep**: tint / ambiance / alpha_scale setter site 全件 (= pathfinding visualization 経路、`indra/newview/llpathfinding*.cpp` + `LLFloaterPathfindingConsole` 周辺、推定 < 5 site)
- **D2 既存実装読解**: `LLPathfindingPathTool` / `LLPathfindingNavMesh` debug visualization 経路読解、debug menu trigger 経路
- **D4 突合**: tint/alpha_scale 2 UBO 共有 verify (= 1 setter call で 2 UBO 同時 dirty)

##### (4) 設計 task (= 4 経路、group 単位)

- **register**: 2 UBO 個別 register、program 識別で wrap (= pathfindingV program → PathfindingV / pathfindingNoNormalV program → PathfindingNoNormal)
- **write**: 2 trigger 経路:
  - **trigger 1 pathfinding debug state 切替**: tint/alpha_scale 同 data source → 2 UBO 同時 dirty (= 1 setter call で host 側 dispatch logic で 2 program 系全 dirty bit) + with-lighting program のみ ambiance dirty
  - **trigger 2 navmesh layer 切替**: layer 別 tint/alpha_scale 連動 dirty
- **flush**: 各 UBO 個別 PerProgram cadence = `flushProgramUbos`
- **shader 接続**: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= 2 shader、PathfindingV `pathfindingV.glsl:70` / PathfindingNoNormal `pathfindingNoNormalV.glsl:65`、改変ゼロ、原則 4 維持)
- **cross-UBO 同期 protocol**: tint/alpha_scale 1 setter call → 2 UBO 同時 dirty (= 同 data source 共有部分) + ambiance setter → PathfindingV 単独 dirty (= ambiance 専用 member)

##### (5) 工程 task

- trace 順内位置: L4 group 13 件目 = §3.5.12 完了後、debug 用途ゆえ優先度低、small group 快速着手可
- group 内 並列性: 2 UBO 並列開発候補 (= 兄弟 UBO、setter 共通)
- group 間 並列性: §3.5.14 / §3.5.15 / §3.5.16 と並列可
- 推定工数: **S** (= 数時間、2 UBO + debug 用途優先度低 + 2 path visual verify)

##### (6) A 確定条件

- mUseUBO ON + 2 shader 活性化 + setter 通電
- AYA live verify: pathfinding debug menu enable で navmesh visualization 動作 (= tint + ambiance + alpha_scale 反映、with/no-lighting 2 path 整合)
- Vulkan validation 0 件
- cross-UBO 同期 verify (= tint/alpha_scale 共有 + ambiance 専用)
- ambiance 名衝突回避 verify (= proj_ambiance / reflection_probe_ambiance との reserved uniform handling 整合)
- bare uniform 残存ゼロ確認
- **verify 単位 = group verify** (= 2 UBO 揃って整合 visual、debug 用途ゆえ中間状態でも pathfinding debug 一部動作)

##### (7) 4 原則 gate

- **原則 1 (Core 分散)**: 2 program 個別 → ✅、debug 用途ゆえ frame 内 1 回未満 dispatch、Core 化粒度緩
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内、cadence PerProgram 維持 → ✅
- **原則 4 (OpenGL を殺さない)**: 2 shader `#else` block 維持 → ✅
- **visual regression ゼロ**: pathfinding debug 描画同一 → AYA live verify、本 group は LL upstream 標準 debug ゆえ AYAstorm 視覚機能交差なし

---

#### §3.5.14 L4-14: GLTF asset 2 UBO group (= Asset_GLTFMaterials + Asset_GLTFNodes、PerAsset cadence pilot 通電済、Phase 3 R4 メインターゲット)

**AYA literal 命名 mapping**: READINESS §3.14 (= GLTF asset pair 60-61 = 2 UBO、AYA 単純配列で C-60〜C-61 相当)

**位置付け**: L4 group 14 件目 = GLTF asset pipeline 直結、**PerAsset cadence cluster 唯一 2 UBO** (= ubo_metadata.inl cadence_tag=3 全件、`llglslshader.cpp:97` literal「現 codegen 0 件、PC-7γ-2 defensive 通電 / PC-7γ-3 本格化」)、**pilot 段階通電済** (= Phase 1.C PC-7γ-3 で per-asset cadence 経路 + GLTF host write 置換配線 + Phase 1.D/E で real PBR shader 連動進行)、**Phase 3 R4 メインターゲット** (= memory `project_r41_phase2_4_principles` 原則 3 = `UB_GLTF_MATERIALS per-asset 本実装 + 実 PBR shader 接続 (R4) + PerDrawUBO_LightParams 実内容 (R5) bundle`)、**全 UBO 中最大単一 size** (= 16384 B = Vulkan 1.3 min UBO size、ObjectSkin 10752B を超え Skin_GLTFJoints 16384B と並ぶ)、**binding=0/1 衝突 3 site のうち 2 site** (= §3.5.7 sub-cluster (b)/(c) で起案済 AtmoExtra binding=0 / SkyV binding=1、L0-1 dispatch protocol 経路)、triple-buffer (= `sAssetUboSetV3a × FRAMES_IN_FLIGHT=3`、`llvkloader.cpp:905` literal)

**group 概要** (= 1 sub-cluster 2 UBO、cross-UBO 同期 protocol PerAsset 単一 trigger):

- **Asset_GLTFNodes** (set=3 binding=0、16384B std140 16384B、1 member = `gltf_nodes` vec4[1024] offset=0 stride=16) — `class1/gltf/pbrmetallicroughnessV.glsl:335-338` singleton site、**MAX_NODES_PER_GLTF_OBJECT = `gGLManager.mMaxUniformBlockSize/48` = Vulkan 1.3 min 16384 B → 341 nodes** (= blueprint コメント記載)、data source = `LL::GLTF::Asset` node transforms array、既存 OpenGL writer `gltf/asset.cpp` updateNodeData 推定 [要追加調査]
- **Asset_GLTFMaterials** (set=3 binding=1、16384B std140 16384B、1 member = `gltf_material_data` vec4[1024] offset=0 stride=16) — `class1/gltf/pbrmetallicroughnessV.glsl:66-82` (= packing layout 起源) + `pbrmetallicroughnessF.glsl:38-42` 2 file consume、**MAX_UBO_VEC4S = `gGLManager.mMaxUniformBlockSize/16` = 1024 vec4** (= Vulkan 1.3 min)、data source = `LL::GLTF::Asset` materials array、既存 OpenGL writer `gltf/asset.cpp` updateMaterialData 推定 [要追加調査]
- shader manager UB enum: `llglslshader.h:170/171` literal `UB_GLTF_NODES` / `UB_GLTF_MATERIALS` + `llglslshader.cpp:1962/1963` block 名 string 登録
- set 配線 const: `llvkloader.cpp:862` literal `V3A_ASSET_SET_BINDINGS = 3 // set=3: Asset_GLTFNodes + Asset_GLTFMaterials + Skin_GLTFJoints (X2-B sampler 除外)`
- forwardToUboUpload PerAsset case: `llglslshader.cpp:2172-2195` literal `LL::GLTF::Asset* asset = LLVKLoader::getCurrentAsset()` → `LLVKLoader::writeAssetUbo(asset, loc.block_hash, loc.offset, data, size)` (= block_hash 経由 generic 経路)
- **upper bound at register vs runtime size at write 規約** (= G5-A1、blueprint コメント記載):
  - blueprint 宣言は std140 array 上限 1024 vec4 (= 16384 B) で固定 (= register 時 upper bound)
  - 実 buffer 確保は runtime size を写し込む (= write 時 runtime size)
  - viewer `#define MAX_UBO_VEC4S = gGLManager.mMaxUniformBlockSize/16` runtime 上限経路
- **Vulkan vs OpenGL upper bound 差分 risk**:
  - Asset_GLTFMaterials: Vulkan 1024 vec4 / OpenGL 4096 vec4 (= 4x 差) → material count > 1024 で truncate/split risk [要 verify]
  - Asset_GLTFNodes: Vulkan 341 nodes / OpenGL 1365 nodes (= 4x 差) → node count > 341 で truncate/split risk [要 verify]
- **cross-UBO data source 共有**: 同 `LL::GLTF::Asset` 由来 (= 同 asset 切替時 2 UBO 同時 dirty 推定、verify 要 [要 verify])
- **dispatch logic**: binding=0/1 衝突解消 = `bindV3aStatic` / `bindV3aRigged` 経路で set=3 帯一括 bind (= `llvkloader.cpp:2168/2172/2174` literal、rigged draw 経路で set=2 skip → set=3 swap)、§3.5.7 sub-cluster (b)/(c) で起案済の PerProgram cadence UBO (= AtmoExtra/SkyV) と program 識別で descriptor set 内容差替 [要 verify L0-1]

##### (1) 前提条件

- L0-1 (= dispatch logic、特に set=3 binding=0/1 衝突 = AtmoExtra (PerProgram) ↔ Asset_GLTFNodes (PerAsset) / SkyV (PerProgram) ↔ Asset_GLTFMaterials (PerAsset)、cadence_tag 別経路で binding 番号空間分離、§3.5.7 group dispatch protocol 整合)
- L0-2 (= LLStaticHashedString UBO redirect 経路、generic writeAssetUbo 経路は block_hash 経由ゆえ name-based 経路と整合 verify)
- L0-4 (= cadence 妥当性、PerAsset cadence pilot 通電済、本格化作業のみ)
- Phase 1.A PA-8 (= blueprint 起案 + codegen Asset_* block 追加)
- Phase 1.C PC-7γ-2 (= defensive 配線、cadence_tag=3 block 0 件時)
- Phase 1.C PC-7γ-3 (= GLTF host write 置換 + codegen Asset_* block 追加 + lifecycle hook 配線、hot path 通電、`llglslshader.cpp:2178-2180` literal)
- Phase 1.D / 1.E (= real PBR shader connection 進行中、Phase 1.E complete marker handoff doc 参照)
- §3.5.7 sub-cluster (b)/(c) 完了 (= AtmoExtra binding=0 / SkyV binding=1 衝突解消 dispatch protocol 確立)

##### (2) 不明事項

**setter 経路特定**:
- Asset_GLTFNodes 既存 OpenGL writer `gltf/asset.cpp` updateNodeData 具体 call site **[要追加調査]**
- Asset_GLTFMaterials 既存 OpenGL writer `gltf/asset.cpp` updateMaterialData 具体 call site **[要追加調査]**

**packing layout 詳細**:
- Asset_GLTFNodes packing 仕様 (= mat4 単位 packed transform / node_id 解決経路、48B align ゆえ vec3 = 3 vec4) **[要追加調査]**
- Asset_GLTFMaterials packing 仕様 (= `pbrmetallicroughnessV.glsl:66-82` 詳細、vec4 単位 packed material 構造、material_id index 解決経路) **[要追加調査]**

**runtime size 経路**:
- `writeAssetUbo` 内で runtime size の伝達経路 (= upper bound register vs runtime size write の動作) **[要 verify]**

**upper bound 超過 asset 処理**:
- node count > 341 asset の truncate / split 処理方針 **[要 AYA 判断]**
- material count > 1024 asset の truncate / split 処理方針 **[要 AYA 判断]**

**通電 commit 特定**:
- PC-7γ-3 完了 commit hash (= pilot 通電 commit、handoff doc chain 参照要) **[要追加調査]**
- Phase 1.E complete persistence 確認 (= real PBR shader Vulkan path 実効化 status) **[要 verify]**

**cross-UBO dirty 連動**:
- asset 切替時に 2 UBO 同時 dirty trigger 経路 verify (= 1 setter call で 2 UBO 同期 dirty vs 別経路) **[要 verify D4 突合]**

**node animation update cadence**:
- node transform per-frame update (= animation 更新時 per-asset cadence 内 frame 内複数回 write 可能性) **[要 verify D3]**

**race condition**:
- asset 切替 timing と `sCurrentAsset` accessor の race condition 有無 **[要 verify]**

**Skin_GLTFJoints との関係**:
- 同 set=3 帯 binding=2 Skin_GLTFJoints (cadence=PerSkin)、本 group 2 UBO とは別 lifecycle (= asset 切替時 skin 単独切替なら本 group 不変、推定 verify 要) **[要 verify]**

**Vulkan vs OpenGL upper bound 差分 影響**:
- Vulkan 1.3 min 16384 B (= 1024 vec4 / 341 nodes) vs OpenGL min 65536 B (= 4096 vec4 / 1365 nodes) 4x 差で実世界 asset truncate risk **[要 verify]**

**bare uniform 残存**:
- pbrmetallicroughnessV.glsl + pbrmetallicroughnessF.glsl `#else` block bare uniform setter 残存有無 [要 verify]

##### (3) 調査手法

- **D1 setter Grep**: `gltf/asset.cpp` 内 updateMaterialData / updateNodeData 候補 call site + `writeAssetUbo` block_hash 経路 + `getCurrentAsset()` accessor 経路 (= `llglslshader.cpp:2172-2195` literal)
- **D2 既存実装読解**: `LL::GLTF::Asset` 構造 (= `indra/newview/gltf/asset.h` 等)、node transforms array + materials array packing logic、asset lifecycle hook (= load/unload trigger)、PBR shader pbrmetallicroughness V/F packing layout 詳細
- **D3 cadence verify**: PerAsset cadence frame 内 multi-write (= node animation per-frame update) vs PerAsset 1 回 dirty pattern
- **D4 突合**:
  - Asset_GLTFNodes ↔ Asset_GLTFMaterials 同 asset 由来 dirty 連動 verify
  - upper bound at register vs runtime size at write 整合性 verify
  - binding=0/1 衝突 dispatch protocol verify (= AtmoExtra/SkyV PerProgram cadence と Asset PerAsset cadence の descriptor set 内容差替)

##### (4) 設計 task (= 4 経路、group 単位)

- **register**: 2 UBO 個別 register、PerAsset cadence ゆえ block_hash 経由 generic 経路 (= `writeAssetUbo` 経由)、upper bound 16384 B at register
- **write**: 2 trigger 経路:
  - **trigger 1 asset 切替 (= asset load / draw 切替)**: 2 UBO 同時 dirty (= 同 `LL::GLTF::Asset` 由来、`sCurrentAsset` accessor 更新で getCurrentAsset() 結果変化)
  - **trigger 2 node animation update (= per-frame node transform 変化)**: Asset_GLTFNodes 単独 dirty (= frame 内 multi-write 可能性、PerAsset cadence で frame 内連続 write → triple-buffer flush)
- **flush**: 各 UBO 個別 PerAsset cadence = `sAssetUboSetV3a` triple-buffer 経路 (= `llvkloader.cpp:5729-5840` literal)、cmdbuf bind 経路で `vkCmdBindDescriptorSets` set=3 帯一括
- **shader 接続**: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= `pbrmetallicroughnessV.glsl:66-82` Materials packing + `:335-338` Nodes + `pbrmetallicroughnessF.glsl:38-42` Materials F 側、改変ゼロ、原則 4 維持、ただし AYAstorm 改変ゆえ upstream merge conflict risk)
- **cross-UBO 同期 protocol**: asset 切替 trigger で 2 UBO 同時 dirty (= 同 LL::GLTF::Asset 由来、1 asset 切替で host 側 dispatch logic で 2 UBO 全 dirty bit)
- **PerAsset cadence flush 経路 verify**: `sCurrentAsset` accessor 経路が PBR draw 直前で set される確認 + triple-buffer 経路で frame in flight 単位 update
- **set=3 swap 確認**: rigged draw 経路で set=2 skip → set=3 swap (= `llvkloader.cpp:2174` literal) と非衝突
- **upper bound 超過 asset 処理設計**: truncate vs split vs warning + degrade 方針確定 **[要 AYA 判断]**
- **binding 衝突 dispatch protocol**: AtmoExtra/SkyV PerProgram cadence ↔ Asset_GLTFNodes/Materials PerAsset cadence、descriptor set 内容差替経路 (= rigged draw 経路で set=2 skip → set=3 swap 整合)

##### (5) 工程 task

- trace 順内位置: L4 group 14 件目 = §3.5.13 完了後、pilot 通電済ゆえ通電本格化作業優位、**Phase 3 R4 メインターゲット ゆえ Phase 2 内で前提整備完成必須**
- group 内 並列性: 2 UBO 並列開発候補 (= 同 `LL::GLTF::Asset` 由来、同 cadence、同 dispatch 経路)、ただし Asset_GLTFNodes 先着手 (= node count 上限 341 確認 + animation update 経路)、Asset_GLTFMaterials 後続 (= material packing 詳細 + PBR shader 連動)
- group 間 並列性: §3.5.15 (set=2 binding=0 共有残) / §3.5.16 (MultiLight) と並列可
- 推定工数: **M** (= 半日、2 UBO + pilot 通電済ゆえ通電本格化 + setter 特定 (= updateMaterialData/updateNodeData) + packing layout 詳細 + binding 衝突 dispatch verify + upper bound 超過 asset 処理 + 2 path visual verify + Phase 3 R4 前提整備)

##### (6) A 確定条件

- mUseUBO ON + 2 shader 活性化 + setter 通電 (= 2 UBO 全件、2 trigger 経路全件)
- AYA live verify (= 全 path visual regression ゼロ §5.4):
  - **GLTF asset PBR material 描画** (= metallic/glossy 表面、material array 経由 PBR shader 反射整合、Asset_GLTFMaterials 全 material consume)
  - **GLTF node transform 描画** (= node hierarchy 階層変換、Asset_GLTFNodes node transforms 反映、animation 連続変化)
  - **asset 切替** (= 2 UBO 同時 dirty + dispatch、asset load/unload 整合)
  - **node animation update** (= per-frame node transform 変化、triple-buffer 経路 frame in flight 単位 update)
  - **upper bound 超過 asset 処理** (= truncate or split or warning + degrade、material count > 1024 / node count > 341 asset で挙動確認)
  - **binding 衝突 dispatch** (= AtmoExtra/SkyV PerProgram ↔ Asset PerAsset 同 binding 共有、descriptor set 内容差替整合、§3.5.7 group verify と並走)
- Vulkan validation 0 件 (= 2 UBO + 16384 B max UBO size + PerAsset cadence triple-buffer + binding 衝突 dispatch + std140 vec4[1024] array)
- cross-UBO 同期 verify (= asset 切替で 2 UBO 同時 dirty + 値同期)
- PerAsset cadence flush 経路 verify (= `sCurrentAsset` accessor 経路 + triple-buffer 経路)
- upper bound at register / runtime size at write 規約整合 (= G5-A1、blueprint コメント記載)
- PBR shader Vulkan path 実効化 (= Phase 1.E complete persistence 確認、Phase 3 R4 前提整備)
- bare uniform 残存ゼロ確認 (= 2 shader `#else` block bare uniform setter が host C++ 側に残存しない、ただし pbrmetallicroughness は LL upstream ゆえ shader 改変は upstream merge conflict risk)
- **verify 単位 = group verify** (= 2 UBO 揃って初めて整合 visual、中間状態は暫定 default 値で破綻回避 = PBR material default + node identity transform、AYA literal「現状の見た目とほぼ変わらない描画」継承)

##### (7) 4 原則 gate

- **原則 1 (Core 分散)**: 2 UBO 同 PBR program = render thread 単独 → ✅、PerAsset cadence は将来 Core 化候補 = asset 切替時 worker thread で writeAssetUbo + per-thread `LLUboRingBuffer` 経路、cross-UBO 同期 logic は 1 setter call (= asset 切替) に集約、設計原則 (2) Core 分散実現整合
- **原則 2 (3 OS 共通)**: ✅ (= Vulkan core spec 1.3 範囲内、16384 B max UBO size MoltenVK 整合、std140 vec4[1024] array は MoltenVK Argument Buffer Tier 2 整合)
- **原則 3 (Phase 2/3)**: **Phase 3 R4 メインターゲット直結**、Phase 2 で前提整備完成必須 (= pilot 通電本格化 + setter 特定 + binding 衝突 dispatch + PBR shader Vulkan path 実効化)、cadence PerAsset 維持 → ✅
- **原則 4 (OpenGL を殺さない)**: 2 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能、ただし pbrmetallicroughness V/F は AYAstorm LL_VULKAN_GLSL block 追加で改変済 (= upstream merge conflict risk)、Vulkan path 経路は dual-path 出荷 → ✅
- **visual regression ゼロ**: GLTF asset PBR material + node transform 描画同一 + asset 切替 + animation 連続変化 → AYA live verify、本 group は GLTF PBR pipeline 直結ゆえ AYAstorm 視覚機能交差大 (= AYAstorm 視覚表現章 PBR material 反射品質維持、§3.5.9 reflection probe / IBL pipeline 連動)、Phase 3 R4 で本実装、Phase 2 で pilot 通電維持必須

---

#### §3.5.15 L4-15: set=2 binding=0 共有残 2 UBO group (= ClipPlane + LightParams、name-based dispatch、LightParams pilot zero IS real data 通電済)

**AYA literal 命名 mapping**: READINESS §3.15 (= PerDrawUBO set=2 binding=0 共有 6 UBO の残 = ClipPlane + LightParams、62-63 = 2 UBO、AYA 単純配列で C-62〜C-63 相当)

**位置付け**: L4 group 15 件目 = set=2 binding=0 共有 6 UBO の残 2 件 (= AvatarSkin/AvatarVelocity/ObjectSkin/SkinnedVelocity 4 件は §3.5.8 起案済)、**name-based dispatch precedent 集約 group** (= MaterialUBO/MaterialUBO_Legacy 同位、program 識別で 6 UBO 名 dispatch)、LightParams は **pilot zero IS real data 通電済** (= Phase 1.E PC-N-13、`AYAGltfRealLightParamsEnabled` cvar gate)、**Phase 3 R5 メインターゲット** = LightParams 実内容 (= memory `project_r41_phase2_4_principles` 原則 3 R5)、AYAstorm 視覚機能交差 (= spot light per-draw = AYAstorm light cvar 関連 §3.5.2 sub-cluster (c) と連動候補)

**group 概要** (= 1 sub-cluster 2 UBO、set=2 binding=0 name-based dispatch):

- **PerDrawUBO_ClipPlane** (set=2 binding=0、256B std140 16B、1 member = `clipPlane` vec4 offset=0、視錐台クリップ平面係数) — 5 file consume (= `class1/deferred/pbropaqueF.glsl:180` (= blueprint source) + `class1/gltf/pbrmetallicroughnessF.glsl` + `class3/deferred/softenLightF.glsl` + `class3/deferred/reflectionProbeF.glsl` + `class1/deferred/globalF.glsl`、blueprint コメント `verified identical across 5 sample sites`)、setter 不明 (= `LLShaderMgr::CLIP_PLANE` uniform handle 経由候補、`LLPipeline::beginRenderDeferred` 等、grep verify 要) [要追加調査]、data source 上流 = `LLPipeline::mTransformedClip` / `LLViewerCamera` 候補 [要追加調査]、状態 = **untouched** (= host C++ `writeDrawUbo(PerDrawUBO_ClipPlane, ...)` 呼出 0 件)
- **PerDrawUBO_LightParams** (set=2 binding=0、256B std140 16B、2 member = `spot_light_color` vec3 offset=0 + `spot_light_size` float offset=12、tight pack 1 vec4 slot) — 10 file consume (= primary site `class1/deferred/deferredUtil.glsl:175` + 9 file = `class3/deferred/multiPointLightF / reflectionProbeF / spotLightF / pointLightF / softenLightF` + `class1/deferred/globalF / pbropaqueF / starsV` + `class1/gltf/pbrmetallicroughnessF`)、**alias 定義** (= `deferredUtil.glsl:180-181` literal `#define color spot_light_color` + `#define size spot_light_size` で deferredUtil 後段 attach shader (= shadowUtil 等) backward-compat 維持、scope-limit alias)、**pilot zero IS real data 通電済** (= Phase 1.E PC-N-13 (a)、`writeDrawUbo` 256B zero buffer 2 site = `llvkloader.cpp:6470-6475` PC-N-1 (c) recordPlaceholderPoolDraw + `:6612-6617` PC-N-13 (a) recordGltfAssetDraw + `:6870-6880` 付近 PC-N-2 sky_smoke draw)、`AYAGltfRealLightParamsEnabled` cvar gate (= settings.xml Boolean default=0 Persist=1)、first-fire LL_INFOS marker (= `llvkloader.cpp:6622-6638` literal、cvar=true 時「zero IS real data」semantic 通電)、Phase 1.F+ で real value 置換予定 (= PC-N-13.1 持越、cross-platform spec §6)
- **「zero IS real data」semantic** (= LightParams pilot 通電 architectural truth、PC-N-13 採用 (N13-1) C): sGltfStubAssetPipeline 流用 sky_smoke shader (= sSkySmokeVertModule / sSkySmokeFragModule) は PerDrawUBO_LightParams 非 consume → host write 256B zero buffer が descriptor set layout 充足 architectural truth (= `llvkloader.cpp:5843-5845` 既明示)、shader 側 GPU error なし、Phase 1.F+ 実 PBR shader 接続時 semantic 解釈変化 (= 「zero IS placeholder data」へ移行、real value 置換必須)
- **set=2 binding=0 name-based dispatch 6 UBO 集約**:
  - 本 group: ClipPlane + LightParams (= 2 UBO)
  - §3.5.8 group: AvatarSkin + AvatarVelocity + ObjectSkin + SkinnedVelocity (= 4 UBO)
  - = 計 6 UBO で binding=0 共有、program 毎に 1 名のみ active、host wiring で program 識別 + 該当 UBO 名 dispatch (= MaterialUBO/MaterialUBO_Legacy 同位 precedent、誤った UBO 名 dispatch 時 = layout 不一致 validation 違反 + GPU error risk)
- **cross-UBO 同 binding 衝突**: §3.5.16 PerDrawUBO_MultiLight は set=2 binding=1 別 binding、本 group とは独立

##### (1) 前提条件

- L0-1 (= dispatch logic、set=2 binding=0 共有 6 UBO name-based dispatch logic 確立、本 group 2 UBO + §3.5.8 group 4 UBO 全件で program 識別 + UBO 名 dispatch)
- L0-2 (= LLStaticHashedString UBO redirect 経路)
- L0-4 (= cadence 妥当性、PerDraw cadence 維持)
- L1a-1 (= ClipFParamUBO_Legacy LLStaticHashedString redirect pilot 完了、ただし本 UBO は別 cadence PerDraw + 別 set=2 帯)
- Phase 1.B PC-6β (= `sDrawUboRingBufferMgr` ring buffer infrastructure 配線済)
- Phase 1.E PC-N-13 (= LightParams pilot zero IS real data 通電完了、`AYAGltfRealLightParamsEnabled` cvar gate)
- §3.5.8 group 完了 (= AvatarSkin/AvatarVelocity/ObjectSkin/SkinnedVelocity 4 UBO name-based dispatch 経路確立)

##### (2) 不明事項

**ClipPlane 関連**:
- setter 不明 (= `LLShaderMgr::CLIP_PLANE` uniform handle 経由 grep verify、`LLPipeline::beginRenderDeferred` 等候補) **[要追加調査]**
- data source 上流 (= `LLPipeline::mTransformedClip` / `LLViewerCamera` 等、grep verify) **[要追加調査]**
- shell 通電 commit 特定 (= bringupTestUBO 経由 generic 通電 commit hash、Phase 1.C handoff doc 参照要) **[要追加調査]**
- 5 file consume 統一確認 (= blueprint コメント `verified identical across 5 sample sites` 再 verify) **[要 verify]**
- dirty 連動 = viewport / camera 変更時 Frame_ViewProj (set=0 binding=0) と連動可能性 **[要 verify]**

**LightParams 関連**:
- 本実装化時 real value source = `spot_light_color` host data source (= `LLShaderMgr::LIGHT_DIFFUSE` 等候補) + `spot_light_size` (= `LIGHT_DEFERRED_ATTENUATION.w` 等候補) **[要追加調査]**
- per-light per-draw 書込み legacy 経路 (= 既存 OpenGL `LLPipeline::renderDeferredLighting` 等候補) **[要追加調査]**
- PC-N-13 通電 commit hash (= handoff doc 内 commit 引用要、`cd253cb754` PC-N-13 design-lock の次の実装 commit) **[要追加調査]**
- Phase 1.F+ data 内容置換実装 plan 詳細 (= PC-N-13.1 等の具体 step、cross-platform spec §6 持越項目) **[要 verify]**
- AYAstorm 視覚機能交差 = §3.5.2 sub-cluster (c) light cvar 3 UBO (= PointLightF/SpotLightF/PointLightV) との data source 共有可能性 **[要 verify D4 突合]**

**name-based dispatch 詳細**:
- 6 UBO 同 set=2 binding=0 host wiring 実装詳細 (= Phase 1.B 設計済、実装は Phase 2 本実装化時) **[要 verify L0-1]**

**bare uniform 残存**:
- ClipPlane 5 shader + LightParams 10 shader `#else` block bare uniform setter 残存有無 [要 verify]

**LightParams alias scope**:
- `#define color spot_light_color` 影響 scope (= deferredUtil 後段 attach 全 shader、`deferredUtil.glsl:777-779` 既明示) **[要 verify]**

##### (3) 調査手法

- **D1 setter Grep**: `LLShaderMgr::CLIP_PLANE` / `LLShaderMgr::LIGHT_DIFFUSE` / `LIGHT_DEFERRED_ATTENUATION` setter site 全件 (= `indra/newview/pipeline.cpp` + `LLPipeline::beginRenderDeferred` / `renderDeferredLighting` 周辺)
- **D2 既存実装読解**: PC-N-13 handoff doc (= `handoff/phase1/e/handoff-phase1-e-pc-n-13-complete.md`) + cross-platform spec §6 PC-N-13.1 持越項目 + deferredUtil.glsl alias 影響 scope
- **D4 突合**:
  - ClipPlane ↔ FrameViewProj 連動 dirty verify (= viewport/camera 変化時)
  - LightParams ↔ §3.5.2 sub-cluster (c) light cvar 3 UBO data source 共有 verify
  - 6 UBO name-based dispatch 6 program 全 mapping 統合 (= §3.5.8 4 UBO + 本 group 2 UBO)

##### (4) 設計 task (= 4 経路、group 単位)

- **register**: 2 UBO 個別 register、name-based dispatch で program 識別 (= ClipPlane consume 5 shader / LightParams consume 10 shader / §3.5.8 group AvatarSkin/AvatarVelocity/ObjectSkin/SkinnedVelocity consume 4 group)、各 program で 1 名のみ binding=0 active
- **write**: 2 trigger 経路:
  - **trigger 1 ClipPlane viewport/camera 変化**: ClipPlane per-draw 同値 broadcast (= per-frame 1 回計算 → per-draw 同値 ring buffer 書込み、dirty 判定不要、毎 draw 書込み許容)
  - **trigger 2 LightParams per-light per-draw 変化**: LightParams 各 light/draw で新値 (= zero IS real data semantic 現状、Phase 1.F+ real value 置換)
- **flush**: 各 UBO PerDraw cadence = ring buffer (= `sDrawUboRingBufferMgr`) allocate + memcpy + dynamic_offset、cmdbuf bind 経路で `vkCmdBindDescriptorSets` set=2 帯 4 binding 同時 bind (= `bindV3aStatic` / `bindV3aRigged`、`V3A_DRAW_SET_BINDINGS=4` 個 dynamic_offset 渡し)
- **shader 接続**: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= ClipPlane 5 shader + LightParams 10 shader、改変ゼロ、原則 4 維持、ただし LightParams alias `#define color spot_light_color` で deferredUtil 後段 attach 全 shader backward-compat 維持)
- **cross-UBO 同期 protocol**: ClipPlane と LightParams は独立 data source ゆえ cross-UBO 同期 trigger なし、ただし viewport/camera 変化 (ClipPlane) と FrameViewProj 連動 + light list 変化 (LightParams) と FrameLights + PerDrawUBO_MultiLight 連動候補
- **name-based dispatch 6 UBO 集約設計**: §3.5.8 4 UBO + 本 group 2 UBO の 6 UBO program mapping 統合 (= 全 binding=0 共有 program で誤 dispatch なし、layout 不一致 validation 違反回避)
- **LightParams Phase 1.F+ real value 置換**: Phase 3 R5 メインターゲット、`AYAGltfRealLightParamsEnabled` cvar gate 撤去 (= PC-N-15c 撤去 precedent、AYAGltfRealDrawEnabled / AYAGltfMultiSkinEnabled 同位)

##### (5) 工程 task

- trace 順内位置: L4 group 15 件目 = §3.5.14 完了後、set=2 binding=0 共有 6 UBO の残 2 件 + LightParams pilot 通電済ゆえ通電本格化作業優位、**Phase 3 R5 メインターゲット ゆえ Phase 2 内で前提整備完成必須**
- group 内 並列性: 2 UBO 並列開発候補、ClipPlane は untouched ゆえ writeDrawUbo 配線新設、LightParams は pilot 通電済ゆえ real value 置換のみ
- group 間 並列性: §3.5.16 (MultiLight、set=2 binding=1) と並列可
- 推定工数: **M** (= 半日、2 UBO + name-based dispatch 6 UBO 集約整合 + ClipPlane writeDrawUbo 配線新設 + LightParams Phase 1.F+ real value 置換準備 + 2 path visual verify + Phase 3 R5 前提整備)

##### (6) A 確定条件

- mUseUBO ON + 2 UBO 全件 setter 通電 (= ClipPlane 新規配線 + LightParams Phase 1.F+ real value 置換)
- AYA live verify (= 全 path visual regression ゼロ §5.4):
  - **ClipPlane 動作** (= 視錐台 clip 描画整合、5 shader consume 全件、viewport/camera 変化追従)
  - **LightParams 動作** (= spot light per-draw 描画、10 shader consume 全件、Phase 1.F+ real value 反映、AYAstorm light cvar 連動 (§3.5.2 sub-cluster (c)))
  - **name-based dispatch 6 UBO** (= §3.5.8 4 UBO + 本 group 2 UBO 全 program で正しい UBO 名 dispatch、layout 不一致 validation 違反ゼロ)
  - **set=2 binding=0 共有切替** (= 同 draw 内 program 切替で binding=0 共有 UBO 名切替正確)
- Vulkan validation 0 件 (= 2 UBO + name-based dispatch 6 UBO + std140 vec4 / vec3+float packing)
- name-based dispatch 6 UBO 集約 verify (= §3.5.8 4 UBO + 本 group 2 UBO program mapping 統合)
- ClipPlane setter 経路特定 + writeDrawUbo 配線
- LightParams Phase 1.F+ real value 置換準備 (= source 特定 + `AYAGltfRealLightParamsEnabled` cvar gate 移行設計)
- LightParams alias scope verify (= `#define color spot_light_color` deferredUtil 後段 attach 全 shader 影響)
- bare uniform 残存ゼロ確認 (= ClipPlane 5 shader + LightParams 10 shader `#else` block)
- AYAstorm 視覚機能交差 verify (= LightParams ↔ §3.5.2 sub-cluster (c) light cvar 3 UBO data source 共有)
- **verify 単位 = group verify** (= 2 UBO 揃って初めて整合 visual、中間状態は暫定 default 値で破綻回避 = LightParams zero IS real data semantic + ClipPlane default、AYA literal「現状の見た目とほぼ変わらない描画」継承)

##### (7) 4 原則 gate

- **原則 1 (Core 分散)**: 6 UBO 同 set=2 binding=0 共有 dispatch logic = render thread 単独 → ✅、ring buffer per-thread 化済 (= PC-N-15a `mDrawUboRingBuffer`)、将来 Core 化候補 = worker thread で writeDrawUbo + per-thread ring buffer 経路、設計原則 (2) Core 分散実現整合
- **原則 2 (3 OS 共通)**: ✅ (= Vulkan core spec 1.3 範囲内、name-based dispatch + UBO_DYNAMIC + dynamic offset bind は MoltenVK 整合)
- **原則 3 (Phase 2/3)**: **Phase 3 R5 メインターゲット直結 (LightParams real value 置換)**、Phase 2 で前提整備完成必須 → ✅
- **原則 4 (OpenGL を殺さない)**: ClipPlane 5 shader + LightParams 10 shader `#else` block uniform 個別宣言維持、OpenGL 経路で bare uniform setter 並走可能、LightParams alias `#define color spot_light_color` で deferredUtil 後段 backward-compat 維持 → ✅
- **visual regression ゼロ**: ClipPlane + LightParams 描画同一 + AYAstorm light cvar 連動 + name-based dispatch 6 UBO 整合 → AYA live verify、本 group は deferred lighting + reflection probe + clip plane 直結ゆえ AYAstorm 視覚機能交差大 (= §3.5.2 sub-cluster (c) light cvar 3 UBO 連動 + §3.5.9 reflection probe IBL pipeline 連動)、Phase 3 R5 で本実装、Phase 2 で pilot 通電維持必須、PC-N-13 「zero IS real data」semantic Phase 1.F+ 「zero IS placeholder data」へ移行設計

---

#### §3.5.16 L4-16: PerDrawUBO_MultiLight 1 UBO group (= multiPointLightF LIGHT_COUNT permutation、set=2 binding=1 独立、`gDeferredMultiLightProgram[i]` 16 件)

**AYA literal 命名 mapping**: READINESS §3.16 (= PerDrawUBO_MultiLight 独立 1 UBO、AYA 単純配列で C-64 相当)

**位置付け**: L4 group 16 件目 (= 本 phase 最終 group)、set=2 binding=1 独立 (= 他 PerDraw UBO の binding=0 共有とは独立)、LIGHT_COUNT permutation 16 件 (= `gDeferredMultiLightProgram[i]`、`llviewershadermgr.cpp:1756` literal)、multiPointLightF.glsl singleton consume、untouched、deferred lighting pipeline 直結、AYAstorm light cvar §3.5.2 sub-cluster (c) + §3.5.15 LightParams 連動候補

**group 概要** (= 1 UBO):

- **PerDrawUBO_MultiLight** (set=2 binding=1、768B std140 528B、6 member = `light` vec4[16] offset=0 size=256 stride=16 + `light_col` vec4[16] offset=256 size=256 stride=16 + `far_z` float offset=512 + `global_light_strength` float offset=516 + `_pad_ml0/1` float ×2 offset=520-524) — `class3/deferred/multiPointLightF.glsl:76` singleton site、setter 不明 [要追加調査]、状態 = **untouched** (= host C++ `writeDrawUbo(PerDrawUBO_MultiLight, ...)` 呼出 0 件)
- data source (= verify 要):
  - `light` + `light_col` = `LLPipeline::mNearbyLights` 経由 LIGHT_POSITION + LIGHT_DIFFUSE array (= 推定、grep verify 要 [要追加調査])
  - `far_z` = camera far plane (= verify 要 [要追加調査])
  - `global_light_strength` = `RenderGlobalLightStrength` cvar (= §3.5.2 sub-cluster (c) light cvar と同 source 候補 [要 verify D4 突合])
- **LIGHT_COUNT permutation**: viewer 側 `addPermutation` で 1..16 inject (= `llviewershadermgr.cpp:1756` literal `gDeferredMultiLightProgram[i]` 16 件 permutation)、blueprint は最大 variant LIGHT_COUNT=16 を canonical 採用
- **host alloc 戦略**: full 16 entry alloc + smaller LIGHT_COUNT shader 派生は trailing entry 未参照 view (= Vulkan std140 layout-compat 慣用、shader 改修ゼロ、set=1 MaterialUBO option I precedent)
- **size 中規模 risk**: 768B per-draw buffer × per-draw cadence = ring buffer 圧迫可能性、scene 内 multiPoint draw 多発時 verify 要 [要 verify]
- **LIGHT_COUNT < 16 時 trailing entry GPU read 未発火**: shader uniform array access `light[i]` で `i < LIGHT_COUNT` guard 要 verify [要 verify]
- **cross-UBO 連動**: §3.5.15 LightParams (= 同 deferred lighting pipeline) + §3.5.2 sub-cluster (c) PointLightF/SpotLightF/PointLightV (= AYAstorm light cvar) + FrameLights (= sun_dir/light array) と data source 共有候補 [要 verify D4 突合]

##### (1) 前提条件

- L0-1 (= dispatch logic、set=2 binding=1 独立 binding、衝突なし)
- L0-2 (= LLStaticHashedString UBO redirect 経路)
- L0-4 (= cadence 妥当性、PerDraw cadence 維持)
- Phase 1.B PC-6β + PC-N-15a (= ring buffer infrastructure)
- §3.5.2 sub-cluster (c) 完了 (= light cvar PointLightF/SpotLightF/PointLightV、global_light_strength 同 source candidate verify)
- §3.5.15 完了 (= LightParams + ClipPlane、同 deferred lighting pipeline)

##### (2) 不明事項

- setter 不明 = `gDeferredMultiLightProgram[i]` 経路 LIGHT_POSITION + LIGHT_DIFFUSE array uniform 書込 host C++ site **[要追加調査]**
- data source 上流 = `LLPipeline::mNearbyLights` 構造 + LIGHT_POSITION/LIGHT_DIFFUSE 設計 **[要追加調査]**
- `far_z` data source = camera far plane / scene far depth **[要追加調査]**
- `global_light_strength` data source = `RenderGlobalLightStrength` cvar (= §3.5.2 sub-cluster (c) PointLightF/SpotLightF と同 source 候補) **[要 verify D4 突合]**
- shell 通電 commit 特定 (= bringupTestUBO 経由 generic 通電) **[要追加調査]**
- LIGHT_COUNT < 16 時 trailing entry GPU read 未発火 verify (= shader guard `i < LIGHT_COUNT`) **[要 verify]**
- AYAstorm light cvar (= §3.5.2 sub-cluster (c)) との cross-UBO data source 共有 **[要 verify D4 突合]**
- bare uniform 残存 = `class3/deferred/multiPointLightF.glsl` `#else` block uniform 個別宣言 [要 verify]

##### (3) 調査手法

- **D1 setter Grep**: `LIGHT_POSITION` / `LIGHT_DIFFUSE` array uniform setter site + `gDeferredMultiLightProgram` 経路 + `mNearbyLights` data source
- **D2 既存実装読解**: `LLPipeline::renderDeferredLighting` 経路 + `llviewershadermgr.cpp:1756` permutation 構造
- **D4 突合**:
  - global_light_strength ↔ §3.5.2 sub-cluster (c) light cvar 3 UBO data source 共有
  - light/light_col ↔ FrameLights + PerDrawUBO_LightParams data source 共有

##### (4) 設計 task (= 4 経路、group 単位)

- **register**: 1 UBO 個別 register、`gDeferredMultiLightProgram[i]` 16 permutation 全件で同 register
- **write**: light list 変化 trigger で writeDrawUbo (= `LLVKLoader::writeDrawUbo(ubo::block_hash::PerDrawUBO_MultiLight, 0u, mlight_data, 528, dynamic_offset)`、528 B std140 size、768 B padded buffer 内 trailing 240 B 未使用)
- **flush**: PerDraw cadence ring buffer (= `sDrawUboRingBufferMgr` + PC-N-15a per-thread)、cmdbuf bind 経路で `vkCmdBindDescriptorSets` set=2 帯 4 binding 同時 bind
- **shader 接続**: 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 (= `multiPointLightF.glsl:76` singleton、改変ゼロ、原則 4 維持)
- **LIGHT_COUNT permutation 対応**: full 16 entry alloc + LIGHT_COUNT < 16 時 trailing entry zero fill (= shader 改修ゼロ pattern、set=1 MaterialUBO option I precedent)
- **cross-UBO 同期**: global_light_strength を §3.5.2 sub-cluster (c) light cvar 3 UBO と同期 dirty trigger (= 1 setter call → 4 UBO 同時 dirty if same source)

##### (5) 工程 task

- trace 順内位置: L4 group 16 件目 = 本 phase 最終 group、§3.5.15 完了後、§3.5.2 sub-cluster (c) light cvar 連動 verify
- group 内 並列性: 1 UBO 単独
- group 間 並列性: 他 group 全件と並列可
- 推定工数: **S-M** (= 数時間〜半日、1 UBO + LIGHT_COUNT permutation 16 件対応 + setter 特定 + cross-UBO data source 共有 verify + 1 path visual verify)

##### (6) A 確定条件

- mUseUBO ON + 1 shader 活性化 + setter 通電
- AYA live verify (= 全 path visual regression ゼロ §5.4):
  - **multiPointLight 描画** (= LIGHT_COUNT 1-16 各 permutation で deferred lighting 整合)
  - **light list 変化追従** (= camera move / light add/remove)
  - **trailing entry zero fill** (= LIGHT_COUNT < 16 時 trailing array entry 未参照確認)
  - **AYAstorm light cvar 連動** (= global_light_strength 同 source 経由 §3.5.2 sub-cluster (c) と同期)
- Vulkan validation 0 件 (= 1 UBO + binding=1 独立 + std140 vec4[16] array stride=16)
- LIGHT_COUNT permutation 16 件全 verify (= shader guard `i < LIGHT_COUNT` 動作確認)
- cross-UBO data source 共有 verify
- bare uniform 残存ゼロ確認
- **verify 単位 = group verify** (= 1 UBO 単独ゆえ即 verify、AYA literal「現状の見た目とほぼ変わらない描画」継承)

##### (7) 4 原則 gate

- **原則 1 (Core 分散)**: 1 program 16 permutation = render thread 単独 → ✅、ring buffer per-thread 化済、将来 Core 化候補、設計原則 (2) Core 分散実現整合
- **原則 2 (3 OS 共通)**: ✅
- **原則 3 (Phase 2/3)**: Phase 2 内、cadence PerDraw 維持 → ✅
- **原則 4 (OpenGL を殺さない)**: 1 shader `#else` block 維持、LIGHT_COUNT permutation shader 改修ゼロ → ✅
- **visual regression ゼロ**: multiPointLight 描画同一 + LIGHT_COUNT permutation 16 件整合 + AYAstorm light cvar 連動 → AYA live verify、本 group は deferred lighting pipeline 直結ゆえ AYAstorm 視覚機能交差 (= §3.5.2 sub-cluster (c) light cvar + §3.5.15 LightParams 連動)

---

#### §3.5.17 L4 サマリ + 横断観点

##### §3.5.17.1 L4 16 group (= 62 UBO) サマリ

| # | section | UBO 数 | group 概要 | 工数 | shell/write 通電 | AYAstorm 視覚機能交差 |
|---|---|---|---|---|---|---|
| 1 | §3.5.1 | 3 | aya_sss_skin_flag triple-write (r20 SSS) | M | 一部 | 大 (r20) |
| 2 | §3.5.2 | 7 | aya_visual_realism + chroma_str + light cvar 3 sub-cluster | L | 一部 | 大 (r14-30) |
| 3 | §3.5.3 | 3 | shadow_target_width triple-write | M | 一部 | 中 (shadow) |
| 4 | §3.5.4 | 2 | box_center/box_size program 識別 dispatch | M | - | 中 (shadow cube) |
| 5 | §3.5.5 | 3 | GLTF texture transform program 識別 dispatch | M | 一部 | 大 (PBR) |
| 6 | §3.5.6 | 5 | water 系連動 dirty | M | 一部 | 中 (water) |
| 7 | §3.5.7 | 10 | sky/cloud/atmospheric (最大 group) | L | 一部 | 大 (r14/r16/r18/r20) |
| 8 | §3.5.8 | 6 | velocity curr/prev pair + cadence mismatch | L | - | 中 (motion blur) |
| 9 | §3.5.9 | 5 | reflection probe/IBL (SINGLETON + mip chain) | M-L | 3/5 | 大 (PBR 反射) |
| 10 | §3.5.10 | 6 | post-process chain (Cinematic Control 13 cvar) | L | 2/6 | 大 (r30 Cinematic) |
| 11 | §3.5.11 | 4 | glow chain sequential (extract→blur→combine) | M | **4/4** | 中 |
| 12 | §3.5.12 | 2 | SMAA pass chain | S-M | - | 中 (AA) |
| 13 | §3.5.13 | 2 | pathfinding debug pair | S | - | なし (debug) |
| 14 | §3.5.14 | 2 | GLTF asset (Phase 3 R4 メインターゲット) | M | pilot 通電済 | 大 (PBR) |
| 15 | §3.5.15 | 2 | set=2 binding=0 共有残 (Phase 3 R5 メインターゲット) | M | LightParams pilot | 大 (deferred lighting) |
| 16 | §3.5.16 | 1 | PerDrawUBO_MultiLight (LIGHT_COUNT permutation) | S-M | - | 中 (multi light) |

合計 = 62 UBO (= 3+7+3+2+3+5+10+6+5+6+4+2+2+2+2+1 = 63、ただし AtmoExtra は §3.5.2 + §3.5.7 cross-group ゆえ実件数は 62 + cross 1 = 62 件 L4 = READINESS C 件数一致 ✅)

##### §3.5.17.2 L4 完了後の unblocking 関係

- §3.5.7 sky/cloud → §3.5.14 GLTF asset binding 衝突 3 site (= AtmoExtra/SkyV/SkyF ↔ Asset_GLTFNodes/Materials/Skin_GLTFJoints) dispatch protocol 確立必須
- §3.5.8 velocity + §3.5.15 set=2 binding=0 共有 → 6 UBO name-based dispatch 集約整合 (= AvatarSkin/AvatarVelocity/ObjectSkin/SkinnedVelocity + ClipPlane/LightParams)
- §3.5.10 post-process + §3.5.11 glow + §3.5.12 SMAA → post-process pipeline sequential dispatch 整合
- §3.5.9 reflection probe + §3.5.14 GLTF asset + §3.5.15 LightParams → PBR pipeline 統合 (= Phase 3 R4 + R5)
- §3.5.2 sub-cluster (c) light cvar + §3.5.15 LightParams + §3.5.16 MultiLight → light system 統合 (= AYAstorm light cvar 連動)
- §3.5.1 r20 SSS + §3.5.2 sub-cluster (a) visual_realism + §3.5.7 AtmoExtra 残 9 → AtmoExtra UBO 部分 write 統合 (= 同 UBO 内 member 別 dirty 粒度)
- §3.5.3 + §3.5.4 → shadow pipeline 統合 (= shadow_target_width + box_center/box_size)
- §3.5.5 + §3.5.14 → GLTF texture transform 統合 (= MaterialUBO + Asset_GLTFMaterials)
- §3.5.6 + §3.5.7 → water + sky preset 連動 (= LLEnvironment 由来)

##### §3.5.17.3 L4 全件共通注記

- **cadence 再分類候補多数** (= L0-4 結果依存): §3.5.7 camPosLocal/time/blend_factor (= 6 UBO PerProgram → PerFrame 降格候補) + §3.5.8 VelocityV/VelocityAlphaV (= PerProgram → PerDraw 必須降格) + §3.5.8 ObjectSkin/SkinnedVelocity (= PerDraw → PerSkin 昇格検討) + §3.5.9 RadianceGen/Gaussian (= PerProgram → PerDraw mip chain loop 降格候補) + §3.5.12 SMAAParam (= PerProgram → PerFrame/SINGLETON 降格候補) [要 AYA 判断 = AYA 判断 cadence 再分類 protocol]
- **binding 衝突 3 site** (= §3.5.7 sub-cluster (b)/(c) AtmoExtra binding=0 / SkyV binding=1 / SkyF binding=2 ↔ §3.5.14 Asset_GLTFNodes/Materials + Skin_GLTFJoints) = L0-1 dispatch protocol 経路、PerProgram cadence + PerAsset cadence 別経路で binding 番号空間分離
- **set=2 binding=0 共有 6 UBO** (= §3.5.8 4 UBO + §3.5.15 2 UBO) = name-based dispatch precedent 集約、program 識別で正しい UBO 名 dispatch
- **複製併存 1 site** (= §3.5.7 sub-cluster (d) cloudsV/cloudsF.glsl:79、η-14 path G-β) = vertex/fragment 両 stage 同 set=3 binding=3 参照、Vulkan 仕様整合
- **data duplication 解消候補** (= §3.5.8 sub-cluster (b) ObjectSkin.lastMatrixPalette ↔ SkinnedVelocity.lastMatrixPalette_skinned_velocity 同 data、§3.5.8 sub-cluster (c) VelocityV.last_object_matrix ↔ VelocityAlphaV.last_object_matrix 同 data) [要 AYA 判断]
- **first-frame fallback logic 必須遵守** (= §3.5.8 sub-cluster (a) AvatarVelocity `mLastGLMp.empty() ? mGLMp : mLastGLMp`、lightning-streak velocity 回避担保)
- **ring buffer 容量 risk** (= §3.5.8 sub-cluster (b) ObjectSkin 10752B = 全 UBO 中最大 + SkinnedVelocity 5376B、Vulkan min 16384B 範囲内だが境界近接) [要 verify]
- **maxUniformBufferRange 制約** (= §3.5.14 Asset_GLTFNodes/Materials 16384B = Vulkan 1.3 min 上限、material count > 1024 / node count > 341 で truncate/split risk) [要 AYA 判断]
- **upper bound at register / runtime size at write 規約 G5-A1** (= §3.5.14 PerAsset UBO 全件適用)
- **shell + write 経路通電済 UBO 集約**:
  - §3.5.11 glow chain 4 UBO 全件 (= 本 phase 最先進)
  - §3.5.9 IrradianceGen + Gaussian (= 2 UBO)
  - §3.5.10 Luminance + Exposure (= 2 UBO)
  - §3.5.14 Asset_GLTFNodes + Materials (= pilot 通電済 PC-7γ-3)
  - §3.5.15 LightParams (= pilot zero IS real data 通電済 PC-N-13)
  - §3.5.7 FrameAtmosphere_Lighting (= shell 通電済 + write 経路本格化済 PC-7γ-1)
  - §3.5.9 Global_ReflectionProbes (= shell 通電済 PC-2 SINGLETON)
- **Phase 3 R4/R5 メインターゲット**: §3.5.14 GLTF asset (R4) + §3.5.15 LightParams (R5 bundle = PerDrawUBO_LightParams 実内容)
- **AYAstorm 既存機能 risk 大 group** (= 視覚機能交差大、機能維持必須): §3.5.1 r20 SSS + §3.5.2 visual_realism + r30 BD light cvar + §3.5.5 GLTF + §3.5.7 sky/cloud + §3.5.9 reflection IBL + §3.5.10 Cinematic Control 13 cvar + §3.5.14 GLTF asset + §3.5.15 LightParams、本 group 全件で AYA live verify 必須 (= memory `project_ayastorm_visual_realism_chapter` + `project_ayastorm_r30_cinematic_chapter` + `project_r30_cinematic_control_tuning_deferred` + `project_ayastorm_r14_pivot_to_light` 整合)

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

## §4. 4 原則 gate 整合 + visual regression ゼロ + violation 検知 protocol

**起源**: AYA literal 確定 2026-06-06 (= memory `project_r41_phase2_4_principles`、本 design session 内 record)。Phase 2 以降の全 Phase + r42 milestone まで継続遵守。

**位置付け**: 本 §4 = WORK_ORDER 全 94 UBO の **sub-work (7) 4 原則 gate 評価軸 normative 定義**。L0 横断 protocol 4 件 + L1〜L5 全 94 UBO 各項目で sub-work (7) を本 §4 の 4 原則 + 視覚 regression ゼロ + violation 検知 protocol に照らして評価。

### §4.1 原則 1: Core プロセス分散を意識した設計を維持 (= C1-C6 設計制約)

**根拠**: r41 Vulkan 化の根幹方針 (= memory `project_ayastorm_r41_design_principles` (2) Core 分散) の Phase 2 具体化。Phase 1.E PC-N-15a/b で確立した worker thread baseline (= `LLUboRingBuffer` per-thread + per-thread `VkCommandPool` + secondary cmdbuf + `LL::WorkQueue`) を Phase 2..K で意図せず破壊しない、r42 milestone での worker thread default ON 化を後付け可能な設計を維持。

**6 件 gate**:

| gate | 内容 | 検知点 |
|---|---|---|
| **C1** | UBO 書込は thread-local accessor 経由のみ (= `sCurrentAsset` / `sCurrentSkin` / `sCurrentPrimitive` / `sCurrentNodeAssetMatrix`)、global state 直書き禁止 | sub-work (4) write 経路設計 |
| **C2** | dirty flag + ring buffer は per-thread `LLUboRingBuffer` instance 内、shared mutable state 回避 | sub-work (4) register/flush 経路設計 |
| **C3** | UBO flush は secondary cmdbuf 内のみ (= `PcN14WorkerContext::mSecondaryCmdBuf` 経由)、primary cmdbuf 直書き禁止 | sub-work (4) flush 経路設計 |
| **C4** | descriptor set bind は per-thread cmdbuf 内のみ | sub-work (4) flush 経路設計 |
| **C5** | shared 書込は mutex 保護必須 (= `sPcn13MultiAssetSeenMutex` pattern)、可能な限り per-thread instance 化優先 | sub-work (4) write 経路設計 + (2) 不明事項 |
| **C6** | call site API (= `recordGltfAssetDraw` / `recordAvatarPlaceholderDraw` / `writeDrawUbo` / `writeSkinUbo` / `bindV3aStatic` / `bindV3aRigged` signature) 不変維持 | sub-work (4) write 経路設計 (= host C++ 既存 setter call site 温存) |

**sub-work (7) 評価**: 各 UBO で C1-C6 各 gate を **✅ / [要追加調査] / [要 AYA 判断] / violation** で逐次判定。violation 検知時は §4.7 protocol 適用。

### §4.2 原則 2: 3 OS が同じ処理で動く実装 (= OS-1〜OS-10 gate)

**根拠**: 3 OS (Linux / Windows / macOS) 同一 source compile + 同一動作前提 (= memory `project_ayastorm_three_platforms`)。OS 別分岐 code 追加禁止 (= MoltenVK 固有 hack / Windows driver 固有 workaround 等)。

**10 件 gate**:

| gate | 内容 | 検知点 |
|---|---|---|
| **OS-1** | Vulkan core spec 1.3 範囲内のみ使用、独自 extension 追加禁止 | sub-work (4) shader 接続 + register 経路設計 |
| **OS-2** | descriptor set 数 5 維持 (= Phase 1.A 確立)、新 set 帯追加禁止 (= MoltenVK Argument Buffer Tier 2 制約整合) | sub-work (4) register 経路設計 + (2) 不明事項 |
| **OS-3** | std140 padding 厳守 (= chapter 08 §6.4) + offset 二重保証 (= SPIR-V reflection + 独自 calculator 照合) | sub-work (4) register 経路設計 + (3) D4 layout 突合 |
| **OS-4** | buffer alignment は `minUniformBufferOffsetAlignment` query 結果使用、hard-code 256 等禁止 | sub-work (4) register 経路設計 |
| **OS-5** | shader 改変ゼロ維持 (= 既存 PBR shader 流用、新 shader 追加禁止) | sub-work (4) shader 接続 経路設計 |
| **OS-6** | `vkCmdUpdateBuffer` size 65536 B 厳守、超過は staging buffer 経由 | sub-work (4) flush 経路設計 |
| **OS-7** | Linux validation layer warnings 0 件 (= Phase X.C Exit 必須) | sub-work (6) A 確定条件 cold launch validation |
| **OS-8** | threading は `std::thread` + `std::mutex` + `std::atomic` + `thread_local` のみ、lock-free 自前実装禁止 | sub-work (4) write/flush 経路設計 |
| **OS-9** | OS 固有 path / dlopen / driver-specific code 不混入 | sub-work (4) 全経路 + (3) 調査手法 |
| **OS-10** | PC-N-15a 確立 infra (= `LLUboRingBuffer` + per-thread `VkCommandPool` + secondary cmdbuf) の API 不変、既 3 OS 想定動作を破壊しない | sub-work (4) 全経路 設計 |

**sub-work (7) 評価**: 各 UBO で OS-1〜OS-10 各 gate を逐次判定。validation warnings 検知時は §4.7 protocol 適用。

### §4.3 原則 3: Phase 2 と 3 の作業範囲を明確にして工程を予定 (= Template A R3-R6 所属確定 + O3-2 r42 移管)

**根拠**: roadmap §5 詳細化 + handoff Phase 1.D/1.E pilot 先回り着手の本実装化を Phase 番号で明示分離。

**Phase 範囲 (= R3-R6 Template A 順序)**:

| Phase | R# | scope | 対象 UBO |
|---|---|---|---|
| **Phase 2** | R3 | UB_REFLECTION_PROBES 単独本実装 | `Global_ReflectionProbes` (= shell zero dummy → 実 reflection data) |
| **Phase 3** | R4 + R5 bundle | UB_GLTF_MATERIALS per-asset 本実装 + 実 PBR shader 接続 + PerDrawUBO_LightParams 実内容 | `Asset_GLTFMaterials` + `PerDrawUBO_LightParams` (= zero IS real data → 実 light) |
| **Phase 4** | - | UB_GLTF_NODES per-asset 本実装 | `Asset_GLTFNodes` |
| **Phase 5** | R6 | UB_GLTF_JOINTS per-skin 本実装 + avatar Vulkan draw 通電 + sPlaceholderSkin 撤去 | `Skin_GLTFJoints` (= placeholder → 実 bone matrix) |
| **Phase 6..K** | - | 残 UBO (= bare uniform 集約 + 最頻出 per-draw) | L1〜L5 残 94 UBO 順次 |

**Phase 範囲 gate**:

| gate | 内容 | 検知点 |
|---|---|---|
| **R-1** | (Q2) A = 1 UBO 厳守維持、cluster 例外なし (= 1 Phase 2-3 UBO 同 batch 禁止) | sub-work (5) 工程 task |
| **R-2** | pilot 通電済 (Skin/Asset/LightParams) は Template A 順序に従って Phase 3/4/5 で本実装化 | sub-work (1) 前提条件 + (5) 工程 task |
| **R-3** | sub-step 命名 = handoff sub-letter (= Phase X.A / X.B / X.C)、PC-N-* 体系は Phase 1.C/1.D/1.E で役目終了 | sub-work (5) 工程 task |
| **R-4** | O3-2 採用 = OpenGL 撤廃は r42 milestone 後半 sub-phase に移管 (= 旧 Phase K+4 → r42-8) | sub-work (5) 工程 task + 原則 4 (§4.4) cross-reference |

**sub-work (7) 評価**: 各 UBO で「所属 Phase 確定 / Phase 範囲 violation 無し」を確認。Template A 順序逸脱提案は §4.7 protocol 適用。

### §4.4 原則 4: OpenGL を殺さない (= dual-path 出荷 + `mUseUBO` runtime flag)

**根拠**: r41 milestone 内で OpenGL path 撤廃しない (= O3-2 採用)。同 binary 内 A/B 比較 + 同 environment 計測 + iteration cycle fallback + ユーザー fallback を r41 release 後も維持。

**重要 design 決定** (= memory `project_r41_phase1b_vulkan_host_gate`):
- **host C++ 側 redirect 層**: `mUseUBO` runtime flag のみで gate (= default false で OpenGL path 維持)
- **`LL_VULKAN_GLSL` macro 不使用** (C++ context): `LL_VULKAN_GLSL` は GLSL preprocessor 専用 (= `llglslshader.cpp:1159` / `llshadermgr.cpp:543` で shader source concat 時のみ)。C++ で使うと dead code 化
- **GLSL shader 側**: `#ifdef LL_VULKAN_GLSL` は引き続き有効 (= shader source 内の Vulkan-only block gate)

**5 件 gate**:

| gate | 内容 | 検知点 |
|---|---|---|
| **O-1** | r41 milestone Phase 2..K 内で OpenGL path 撤廃しない | sub-work (4) 全経路 設計 + (5) 工程 task |
| **O-2** | r41 release は dual-path 出荷 (= `mUseUBO` runtime flag default OFF、OpenGL path 経路 fallback 提供) | sub-work (4) write/flush 経路設計 |
| **O-3** | host C++ redirect 層 = `mUseUBO` runtime gate のみ、`#ifdef LL_VULKAN_GLSL` C++ 側不使用 | sub-work (4) write 経路設計 (= LLGLSLShader 内 setter 全件) |
| **O-4** | GLSL shader 側 `#ifdef LL_VULKAN_GLSL` block 維持 (= 既存 OpenGL path uniform 宣言 + Vulkan UBO block の dual 並走) | sub-work (4) shader 接続 経路設計 |
| **O-5** | OpenGL 撤廃は r42 milestone 後半 sub-phase 移管 (= r41 milestone scope 外) | sub-work (5) 工程 task |

**sub-work (7) 評価**: 各 UBO で O-1〜O-5 各 gate を逐次判定。`mUseUBO=false` 経路で既存 visual 同一保証が崩れる提案は §4.7 protocol 適用 + §4.5 visual regression ゼロ違反として連動判定。

### §4.5 visual regression ゼロ (= AYA literal 2026-06-06 追加条件、§5.4 policy 参照)

**根拠**: AYA literal 2026-06-06「**現状の見た目とほぼ変わらない描画が望まれる**」。4 原則 (= Core 分散 / 3 OS 共通 / Phase 範囲 / OpenGL を殺さない) に加え、**visual regression ゼロ を Phase 2 全工程の必須条件** として全 94 UBO に追加適用。

**4 件 gate** (= §5.4 policy 参照):

| gate | 内容 | 検知点 |
|---|---|---|
| **V-1** | sub-work (6) A 確定条件 全件に「**visual regression ゼロ (= 現状の見た目とほぼ変わらない描画)**」を含む | sub-work (6) A 確定条件 |
| **V-2** | L1a / L1b / L2 / L3 / L5 (= 個別 verify / 一括 verify) = 各 UBO 通電後 AYA live verify で確認 | sub-work (6) A 確定条件 |
| **V-3** | L4 (= group verify) = group 全件通電後 AYA live verify で確認 (= group 完成前の部分通電 visual 不整合は許容、ただし出荷時 visual regression ゼロ) | sub-work (6) A 確定条件 + L4 group サマリ |
| **V-4** | visual regression 検知時 = 該当 UBO の (4) 設計 task 見直し、(7) 4 原則 gate (= 特に原則 4 OpenGL を殺さない dual-path 維持) 再検証 | §4.7 violation 検知 protocol 連動 |

**例外**: 既存 bug fix (= AYA 明示承認)、性能改善で visual 副作用が許容範囲 (= AYA 明示判断) のみ例外。

**含意 (= L0 / L1-L5 横断)**:
- L0-2 LLStaticHashedString redirect = **dual-write 経路必須** (= GL path 既存 uniform 設定維持で visual 同一保証)
- L0-3 per-shader UBO block 拡大 = **`LL_VULKAN_GLSL` gate 必須** (= GL path 既存 uniform 宣言維持で visual 同一保証)
- L0-4 cadence 再評価 = **stale 許容判断は visual regression risk を含めて判定** (= per-program stale で visual 副作用ゼロの厳格確認)
- L4 C group 中間状態 = **group 完成前の部分通電 visual 不整合は許容、ただし出荷時 visual regression ゼロ**

### §4.6 各項目評価 protocol (= sub-work (7) 全 94 項目逐次 check)

**適用範囲**: L0 横断 protocol 4 件 + L1 (3) + L1b (2) + L2 (4) + L3 (20) + L4 (62) + L5 (3) = 全 94 UBO + L0 4 protocol、合計 98 件 sub-work (7) 評価。

**評価軸 5 軸** (= §4.1-§4.5):

| 軸 | 評価対象 gate 数 | 評価方式 |
|---|---|---|
| 原則 1 Core 分散 | C1-C6 = 6 件 | ✅ / [要追加調査] / [要 AYA 判断] / violation |
| 原則 2 3 OS 共通 | OS-1〜OS-10 = 10 件 | ✅ / [要追加調査] / [要 AYA 判断] / violation |
| 原則 3 Phase 範囲 | R-1〜R-4 = 4 件 | ✅ / [要追加調査] / [要 AYA 判断] / violation |
| 原則 4 OpenGL 殺さない | O-1〜O-5 = 5 件 | ✅ / [要追加調査] / [要 AYA 判断] / violation |
| 視覚 regression ゼロ | V-1〜V-4 = 4 件 | ✅ / [要追加調査] / [要 AYA 判断] / violation |

**合計 29 gate / 94 UBO = 2726 件 + L0 4 件 = 2842 件 評価 cell**。本 §3.1-§3.6 各 UBO sub-work (7) で既に gate 評価記載済 (= L0 4 件 + L1a 3 + L1b 2 + L2 4 + L3 20 + L4 62 + L5 3 = 94 件 起案完了)。

**評価記載 protocol**:
- 各 UBO sub-work (7) に **「原則 1 (= C1〜C6)」「原則 2 (= OS-1〜OS-10)」「原則 3 (= R-1〜R-4)」「原則 4 (= O-1〜O-5)」「視覚 regression ゼロ (= V-1〜V-4)」** 5 行記載
- 各行で該当 gate 番号と ✅ / マーク + 簡潔注記
- violation 検知時は §4.7 protocol 連動

**完成 verify**: C-8 完了時に全 98 件 sub-work (7) 5 行記載確認 (= grep `^##### \(7\) 4 原則 gate` で 98 件 hit 確認)。

### §4.7 violation 検知時の対応 protocol

**3 stage 対応**:

| stage | trigger | 対応 |
|---|---|---|
| **stage 1: 提案撤回** | sub-work 起案中に 4 原則 + 視覚 regression ゼロ 1 件以上 violation 検知 | 該当 sub-work (4) 設計 task 即時撤回、(2) 不明事項に violation 内容記載、別案検討 |
| **stage 2: 設計再考** | 別案検討で 4 原則整合解 が見つからない | 該当 UBO 設計を Layer 1 段上から再考 (= L1-L5 所属 Layer 見直し、cadence 再分類検討、L0 protocol 修正検討) |
| **stage 3: AYA literal 確認** | 設計再考でも 4 原則整合解 が見つからない場合の最終判断 | AYA literal 確認 (= 例外承認 / 設計大幅変更 / Phase 移管 / Phase 範囲再定義)、AYA literal record |

**stage 別記載 marker**:
- **[要追加調査]** = 不明事項発生、調査後再評価可能
- **[要 AYA 判断]** = AYA literal 判断要、stage 3 即時 escalation
- **violation** = 4 原則 / 視覚 regression ゼロ いずれか確定 violation、stage 1 即時撤回

**AYA literal 確認 trigger 例** (= L0 §2.5 review + L4 group 内既出含む):
1. set=3 binding 衝突 3 site 解消方針 (= L0-1 protocol-C、AYA 判断要)
2. LLStaticHashedString mapping table 構築方式 (= L0-2 protocol-B、AYA 判断要)
3. shader build pipeline preprocessor inject 方式 (= L0-3 protocol-B、AYA 判断要)
4. cadence 再分類 strategy (= L0-4 protocol-B、AYA 判断要)
5. upstream merge conflict 自動検出 strategy (= L0-3 protocol-D、AYA 判断要)
6. sliced UBO 化が Phase 3 移管対象か Phase 2 内か (= L0-4 (7) 原則 3、AYA 判断要)
7. L4 group cross-reference 設計判断 (= §3.5.17 L4 サマリ + 各 group sub-work (4))

**violation 記録**: 全 violation 検知 record は WORK_ORDER.md sub-work (7) + 該当 UBO file §12 に同期記載 (= single source of truth = WORK_ORDER.md §3.N、§12 = 個別 UBO ナビゲーション)。

**起案規律遵守 reference**:
- memory `feedback_admit_unknown` (= 推論禁止、不明明示)
- memory `feedback_no_scope_shrink` (= AYA literal scope 厳守、4 原則 violation 提案禁止)
- memory `feedback_self_bug_no_defer_option` (= 自作 violation の「先送り/disable」を提案として並べない)

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
