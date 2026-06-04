# Literal cross-ref audit (= Phase 1.C drift 監査 + PC-6ε..PC-N drift 予防)

**作成日**: 2026-06-04
**起案契機**: Phase 1.C PC-3..PC-6δ で 4 連続 handoff/prep doc literal drift 発生。AYA 判断 (C2 + A4) で PC-6ε 着手前に audit phase 実施。
**scope**: design chapter 02 / 03 / 06a / 06b / 06c / 07 / 09 (= 7 chapter、約 3000 line)。04 codegen + 08 build pipeline は PC-1 確定済で残 sub で touch しないため除外。

`feedback_design_phase_no_code_write` 準拠 = 本 audit は doc 化のみ、`indra/` 配下改変ゼロ。

---

## §1 drift 4 件 type 分類 + remediation 案

| # | type | 発生 sub | 発生 doc | drift literal | 実 canonical | 影響 |
|---|---|---|---|---|---|---|
| (D1) | filename drift | PC-3 | Phase 1.C prep doc line 39 + 179 | `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md §12` | `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md §12` | §12 自体は実在 + 内容 (W2/RB/PSC) 正しい、**file 名のみ stale (旧 file 名)** |
| (D2) | ID rename drift | PC-4 | Phase 1.C prep doc | `(R1)` (= ring buffer prefix の古 ID) | `(RB)` (= 設計 review 2026-06-03 §3.1 で R1→RB rename、redirect path 識別子と衝突回避) | 07 §12 row 6 で「(RB) (= 旧 (R1)、設計 review 2026-06-03 §3.1 で ID rename)」明示済 |
| (D3) | section semantic drift | PC-5 | Phase 1.C prep doc line 179 + PC-1 handoff line 150 | `chapter 07 §11 既存設計` (= PSO 作成経路の source として cite) | §11 = `chapter 04 / 06a / 06b / 06c / 08 / 09 との分担境界` table、PSO 内容なし。**真 source = §9.1 (pipeline layout) + §9.3 (PSO cache)** | 引用 section 番号自体が概念対応してない |
| (D4) | term drift | PC-6δ | PC-6γ handoff §10 + bootstrap message | `per-pass` (= 5 cadence の 1 名) | design 06b §2.2 / §4.1 canonical = `per-program` (= `flushProgramUbos(LLGLSLShader*)`) | term 自体は cadence の機能と類似だが design canonical と非整合 |

### §1.1 各 drift の remediation 3 案 (= AYA 判断仰ぎ)

| 案 | 内容 | trade-off |
|---|---|---|
| **(R-A)** | 既存 handoff / prep doc は immutable record として保存、本 audit doc で「実 file / section / ID / term の対応表」を提示 = 時点記録 + 解釈表で drift 吸収 | append-only で spec version 整合性保持、historical record として判断履歴も価値あり、ただし 新規 doc 起案時に audit doc を必ず参照する規律必要 |
| (R-B) | prep doc / handoff doc を retroactive 修正 commit | 元 source が canonical に一致、ただし時点記録破壊 + commit chain への影響 + AYAstorm release branch workflow との整合複雑 |
| (R-C) | Phase 1.C 残 sub-task (PC-6ε..PC-N) の handoff doc 起案時に limited remediation note (= 本 audit doc への link) | 中間案、ただし remediation 範囲が sub 起案者の意識に依存 |

**Claude 推奨 = (R-A)**: append-only 原則整合 + audit doc 1 件で実態提示 = 次 session で「prep doc / 旧 handoff の literal は本 audit doc §2 と照合」と規律明示で drift 回避可能。

---

## §2 design 7 chapter canonical literal cross-ref

### §2.1 chapter 02 naming-convention.md

| category | canonical literal | location |
|---|---|---|
| section anchor | §1, §2, §2.1, §2.2, §2.3, §2.3.1, §2.3.2, §2.4, §3, §3.1-§3.4, §4, §5 | (全件) |
| cadence prefix naming | `Frame*` / `Program_` / `Draw_` / `Asset_` / `Skin_` / `Global_` (singleton) | §3 系 |
| API | `LLGLSLShader::UB_*` enum / `LLShaderMgr::mReservedUniforms` / `LLStaticHashedString` / `gltf::Asset` / `gltf::Skin` | §2.3.1 / §2.3.2 |
| file path | `indra/newview/generated/ubo/` | §2.4 |

### §2.2 chapter 03 cadence-classification.md

| category | canonical literal | location |
|---|---|---|
| section anchor | §1, §2, §2.1, §2.2, §3, §3.1-§3.4, §4, §4.1-§4.4, §5, §6 | (全件) |
| 5 cadence canonical | per-frame / per-program / per-draw / per-asset / per-skin | §2 |
| API | `LLEnvironment::updateShaderUniforms()` / `LLPipeline::renderGeom()` / `LLViewerCamera::updateProjection()` / `gltf::Asset::updateNodeData()` / `gltf::Skin::updateTransforms()` | §3.2 |

### §2.3 chapter 06a cache-structure-and-setter-redirect.md

| category | canonical literal | location |
|---|---|---|
| section anchor | §0-§10 (= §0.1/§0.2/§1.1-§1.2/§3.1-§3.4/§4.1-§4.4 含む) | (全件) |
| API | `mUniformUBOLoc[index]` / `mUniformUBOLocByHash` / `mUseUBO` / `ubo::lookup_runtime()` / `ubo::g_uniform_table[]` / `forwardToUboUpload(loc, data, size)` | §3.2 / §4 / §5.2 |
| cvar | `AYAUboRedirectEnabled` (debug master switch) | §5.4 (詳細は 06c §6.3.3) |
| file path | `indra/llrender/llglslshader.cpp:1704` / `:1554` / `:1842-1849` / `:2141-2538` (17 setter) / `:2617-2844` (13 hashed setter) | (各 §) |
| 5 cadence | per-frame / per-program / per-draw / per-asset / per-skin (`CadenceTag` enum) | §3.3 |

### §2.4 chapter 06b cadence-update-site-and-dirty.md (= PC-6δ drift source)

| category | canonical literal | location |
|---|---|---|
| section anchor | §0-§9 (= §0.1/§0.2/§2.1-§2.5/§3.1-§3.4/§4.1-§4.4/§5.1-§5.4.3 含む) | (全件) |
| **5 cadence flush API canonical (= 第 4 drift 源、最重要)** | `flushFrameUbos()` / `flushProgramUbos(LLGLSLShader*)` / `flushDrawUbos()` / `flushAssetUbos(gltf::Asset*)` / `flushSkinUbos(gltf::Skin*)` | §2.1-§2.5, §4.1 |
| API | `forwardToUboUpload(loc, data, size)` / `UboInstance` struct / `dirty` flag (`std::atomic<bool>`) | §5.1 / §3.2.3 |
| cvar | `AYARingBufferSizeMB` (= 4 MB initial / 16 MB max) | §7.2 (07 §12 由来) |
| file path | `indra/llrender/llglslshader.cpp` setter family / `gltf/asset.cpp:183/232` / `gltfscenemanager.cpp:693/696/736` / `gltf/animation.cpp:411` | (各 §) |

**term drift 防止**: `per-pass` は **non-canonical** = design doc に存在しない、handoff で誤導入された term。canonical = `per-program` (= shader program 単位 = 同一 GLSL program 内全 draw 共通)。

### §2.5 chapter 06c descriptor-set-bind-wiring.md

| category | canonical literal | location |
|---|---|---|
| section anchor | §0-§11 (= §0.1/§0.2/§2.1-§2.5/§3.1-§3.4/§4.1-§4.3/§5.1-§5.3/§6.1-§6.3.3/§7.1-§7.3/§8.1-§8.3 含む) | (全件) |
| API | `LLReflectionMapManager::mUBO` / `vkCmdBindDescriptorSets()` / `vkUpdateDescriptorSets()` / `LLPhaseMigrationList::isUboReady(shader_name)` | §2.2 / §4.1 / §6.3.2 |
| cvar | `AYAUboRedirectEnabled` (master) / `AYAUboPhaseMigrationOverride` / `AYAUboShaderWhitelist` | §6.3.3 |
| singleton naming | `Global_ReflectionProbes` (= PC-1 で codegen 確定済) | §2.2 |

### §2.6 chapter 07 vulkan-api-state.md (= PC-3 / PC-5 drift source)

| category | canonical literal | location |
|---|---|---|
| section anchor | §0-§13 (= §11 = 分担境界 table / §12 = 未確定事項 / §13 = update 規律) | (全件、§11/§12/§13 含む) |
| **§12 持越項目 8 件 (= drift 源で最重要)** | (V1') / (V3') / (S3') / (W) / (W2) / **(RB) (= 旧 R1 から rename 2026-06-03)** / (PSC) / (RF) | §12 |
| cvar | `AYARingBufferSizeMB` (initial=4 MB, max=16 MB) | §7.2 |
| 設計値 (PSC) | path = `~/.ayastorm_x64/cache/pipeline_cache.bin` / 上限 = 64 MB | §9.3 + §12 row 7 |
| 設計値 (W2) | `sAssetUboPool` maxSets = N × 3 = 192 (N=64) / 起動時 1 物理 pool prealloc + grow chunk 64 | §6.1 / §6.3 + §12 row 5 |
| 設計値 (RB) | ring buffer initial=4 MB / max=16 MB + cvar `AYARingBufferSizeMB` 配信 | §7.2 + §12 row 6 |
| API | volk* / vk* / vmaCreateBuffer / vmaCreateAllocator 等 | §2.1-§2.8 |

**section semantic drift 防止表** (= PC-5 由来):

| 概念 | drift 引用 | 真 canonical section |
|---|---|---|
| PSO 作成経路 | (prep doc) 「07 §11 既存設計」 | 07 §9.1 (pipeline layout 配線) + §9.2 (PSO 構築) + §9.3 (PSO cache 戦略) |
| set 帯 4→5 化 | — | 07 §3.2 (V1') + §4 (V3') |
| sampler 49 配置 | — | 07 §5 (S3') |
| descriptor pool 容量 | — | 07 §6.1 (sAssetUboPool) + §6.2 (sProgramUboPool W) |
| ring buffer 容量 | — | 07 §7.2 + §12 row 6 |

### §2.7 chapter 09 phase-roadmap.md

| category | canonical literal | location |
|---|---|---|
| section anchor | §0-§14 (= §14.1-§14.8 含む = Phase 1.A 入口 1 step state checklist) | (全件) |
| Phase 体系 | Phase 0 (η-29 計測) / Phase 1.A (codegen) / Phase 1.B (host C++ redirect 層) / **Phase 1.C (storage + 5 cadence wire up)** / Phase 2..K (per UBO migration) / Phase K+1 Linux / K+2 Win / K+3 Mac / K+4 OpenGL 撤廃 / K+5 release | §1 / §2.1 / §3-§8 |
| (Q) 判断 5 件 + (Q-NTTP) | (Q1) A 最小リスク UBO 優先 / (Q2) A 1 UBO 厳守 / (Q3) A OpenGL 並走 / (Q4) C Linux 先行 / (Q5) A 独立 η-29 / (Q-NTTP) A R1 不採用 = C++17 維持 | §11 (全件 ST-7 batch 確定済) |
| 07 §12 由来持越解消 Phase | (V1')/(V3')/(S3')/(W) → chapter 10 / (W2)/(RB)/(PSC) → Phase 1.C / (RF) → Phase 0 | §10.1 |

---

## §3 PC-6ε..PC-N pre-cache literals (= drift 予防)

### §3.1 PC-6ε scope で必要な canonical literals

PC-6ε literal = PC-6δ handoff §10 = 「block-level test bring-up (`bringupTestUBO()`、`indra/llrender/llglslshader.cpp:2032`) を SINGLETON cadence flush 関数経由の本格置換 + per-program / per-asset / per-skin dirty map (= mUseUBO gate 配下) 配線 + per-draw cadence 残 pool 全配線」。

| 必要参照 | canonical | 章 / 節 |
|---|---|---|
| 5 cadence flush API | `flushFrameUbos()` / `flushProgramUbos(LLGLSLShader*)` / `flushDrawUbos()` / `flushAssetUbos(gltf::Asset*)` / `flushSkinUbos(gltf::Skin*)` | 06b §2.1-§2.5, §4.1 |
| SINGLETON cadence (= 第 6 cadence) | `Global_*` prefix / chapter 02 §3 + 06c §2.2 (= `Global_ReflectionProbes`) | 02 §3 / 06c §2.2 |
| mUseUBO runtime gate | `mUseUBO` flag (= GATE-B) / `forwardToUboUpload` routing 経由 | 06a §3.2 / §5.4 + 06b §5.3 |
| dirty map 配線 | `UboInstance::dirty` (`std::atomic<bool>`) / per-cadence dirty propagation | 06b §3.2.3 / §5.4 |
| per-draw 残 pool | LLDrawPoolSimple 既配線 (= PC-6δ canary) / 残 15+ pool subclass 配線 | (06b §2.3 + codebase trace) |

### §3.2 PC-6ζ scope で必要な canonical literals

PC-6ζ literal = setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突 正攻法対応。

| 必要参照 | canonical | 章 / 節 |
|---|---|---|
| SAMPLER skip 設計 | sampler 系 setter OpenGL path 強制 = 49 個 set=3 同居 (S3') | 06a §5.6 + 07 §5 |
| SINGLETON cadence_tag=5 | 5 値 cadence enum + singleton 値域 | 06a §3.3 + 02 §3 |
| 衝突 site | `indra/llrender/llglslshader.cpp:2480-2563, 3006-3079` (= 17 setter family) | (codebase) |

### §3.3 PC-7 scope で必要な canonical literals

PC-7 literal = `vkCmdBindDescriptorSets` 通電 + dynamic offset 経路で ring buffer chunk hand-off。

| 必要参照 | canonical | 章 / 節 |
|---|---|---|
| `vkCmdBindDescriptorSets` 配線 | 駆動位置 = flush 直後 bind | 06c §4.1 + 07 §2.4 |
| dynamic offset 経路 | ring buffer chunk = HOST_VISIBLE + HOST_COHERENT + MAPPED + 256 B alignment | 07 §7.2 + §7.3 |
| set 帯 5 化 | set=0/1a/1b/2/3 | 07 §3.2 (V1') + §4 (V3') |

### §3.4 PC-8 / PC-N scope

PC-8 = 3 OS build verify (Linux primary + Win/Mac 後段)。PC-N = Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点。

---

## §4 PC-6ε-1 scope 確定案 (= S1/S2/S3 + Claude 推奨)

PC-6ε は AYA 確定済 B 分割 (= PC-6ε-1 bring-up 置換 / PC-6ε-2 dirty map / PC-6ε-3 残 pool 配線)。本 audit doc は PC-6ε-1 着手前判断のみ扱う (PC-6ε-2/3 は PC-6ε-1 完了後に別判断)。

PC-6ε-1 literal = 「block-level test bring-up を SINGLETON cadence flush 関数経由の本格置換」。

### §4.1 解釈 3 案

| 案 | 内容 | 採用根拠候補 |
|---|---|---|
| **(S1)** | `flushSingletonUbos()` 新設 (= 6 cadence 目 = singleton) + `bringupTestUBO()` を `flushSingletonUbos()` 経由に置換 | design 02 §3 で `Global_` prefix = singleton として **明示分類済**、5 cadence ≠ singleton。06c §2.2 で `Global_ReflectionProbes` が singleton 例として既出 |
| (S2) | 既存 `flushFrameUbos()` を流用 = singleton は per-frame 内に併合 | bringupTestUBO 既存 comment との整合性は高いが、design 02 §3 + 06c §2.2 で singleton ≠ per-frame と分類済 = design 整合性破壊 |
| (S3) | `bringupTestUBO()` 撤去 + 内容を `flushFrameUbos()` に移植 | block-level test 機構が消滅、PC-6ε-2/3 dirty map 配線時に singleton 判定不能化 |

### §4.2 Claude 推奨 = (S1)

**前 session bootstrap 時の推奨 (S2) を訂正**。理由:
- design 02 §3 で `Global_` prefix = singleton cadence と **明示分類** = 5 cadence (`Frame_` / `Program_` / `Draw_` / `Asset_` / `Skin_`) + singleton (`Global_`) = **計 6 cadence 体系**
- 06c §2.2 で `Global_ReflectionProbes` = singleton 配置 = singleton cadence の代表例
- 06a §3.3 `CadenceTag` enum 値域 = singleton 含む (= PC-6ζ 衝突 site が cadence_tag=5 = singleton の証左)
- (S2) では singleton を per-frame に併合 = design canonical 違反 = drift 第 5 例生成リスク
- (S1) では 6 cadence flush 関数 (= `flushFrameUbos` + `flushProgramUbos` + `flushDrawUbos` + `flushAssetUbos` + `flushSkinUbos` + **`flushSingletonUbos`**) に体系化 = design 整合

### §4.3 (S1) 採用時の PC-6ε-1 sub-scope

- `indra/llrender/llvkloader.h` 編集 = `void flushSingletonUbos();` 宣言追加 (= 既存 5 件と並列)
- `indra/llrender/llvkloader.cpp` 編集 = `flushSingletonUbos()` 実装 (= `flushDummyUboWrite("flushSingletonUbos")` 委譲、PC-6δ helper pattern 継続)
- `indra/llrender/llglslshader.cpp` 編集 = `bringupTestUBO()` (`:2032`) 内処理を `LLVKLoader::flushSingletonUbos()` 経由に置換
- PC-6δ 配線済 5 cadence (per-frame/program/draw/asset/skin) は touch せず
- Exit Criteria = (i) 6 cadence 関数体系成立 + (ii) bringupTestUBO 経路で singleton flush PASS + (iii) llrender build PASS + (iv) TUT 11+10+13 PASS + (v) codegen 130/130 PASS + (vi) MUSEUBO-A 整合 (= sDrawUboRingBufferMgr 未初期化即 return) + (vii) GATE-B 整合 (= mUseUBO 未依存)

---

## §5 prevention rule (= 次 handoff doc 起案時の規律)

| 規律 | 内容 |
|---|---|
| **rule-1** | handoff doc 起案時、design chapter への section 参照 (= 「07 §12」等) は **書く前に grep `^## §N\|^### §N\.`** で実在確認、加えて section heading の **trailing 内容** (= 「07 §12 未確定事項」全文) も読む |
| **rule-2** | design chapter で ID rename (= R1 → RB 等) が判明したら、本 audit doc §1 表に追記 |
| **rule-3** | cadence / API / cvar / file path / section ID の term は **design canonical を引用元として明記** (= 「design 06b §2.2 canonical naming 採用」と handoff doc 内に必ず書く) |
| **rule-4** | 旧 file 名 (例: `07-descriptor-renderpass.md`) を見たら本 audit doc §1 (D1) と照合、`07-vulkan-api-state.md` への読替 |
| **rule-5** | 「literal stale」と diagnose する前に **両側 verify** (= drift 側引用と design canonical の両方を実 read で確認、片側 grep のみで判定しない、`feedback_doubt_self_first` + `feedback_two_sided_verify` 整合) |

---

## §6 self-verify

| # | 観点 | record |
|---|---|---|
| 1 | drift 4 件全件捕捉 + type 分類 | ✅ §1 表 4 行 (D1 filename / D2 ID rename / D3 section semantic / D4 term) |
| 2 | remediation 3 案 (R-A/B/C) + AYA 判断仰ぎ姿勢 | ✅ §1.1 表 + Claude 推奨 (R-A) 明示 |
| 3 | 7 chapter canonical literal cross-ref | ✅ §2.1-§2.7 で 7 chapter 全件 5 category 整理 |
| 4 | PC-6ε..PC-N で必要な literals pre-cache | ✅ §3 で PC-6ε / PC-6ζ / PC-7 / PC-8 / PC-N 別整理 |
| 5 | PC-6ε-1 scope 確定案 (S1/S2/S3) + Claude 推奨訂正 | ✅ §4.1-§4.3 で前 session の (S2) 推奨を (S1) に訂正 (= design canonical 整合根拠 4 件) |
| 6 | prevention rule 起案 | ✅ §5 で rule-1〜rule-5 整備 |
| 7 | feedback_design_phase_no_code_write 整合 | ✅ 本 audit doc 起案のみ、`indra/` 改変 0 |
| 8 | feedback_doubt_self_first 整合 | ✅ PC-6ε-1 scope 推奨を bootstrap 時 (S2) → 本 audit で (S1) に訂正、bootstrap message 自身を疑った |
| 9 | feedback_no_scope_shrink 整合 | ✅ A4 確定 audit scope (= 7 chapter / 約 3000 line) を完走、scope 縮小なし |

---

## §10 AYA 判断確定 + 次 session 着手 1 line

### §10.1 AYA 判断 4 件全件確定 (2026-06-04)

| # | 判断項目 | Claude 推奨 | AYA 確定 | 確定根拠 |
|---|---|---|---|---|
| 1 | **(S1/S2/S3)** PC-6ε-1 scope | (S1) = `flushSingletonUbos()` 新設 = 6 cadence 体系化 | **(S1) 確定** | AYA literal「OK」2026-06-04 受領 (= bootstrap 時 (S2) 推奨を audit 中に (S1) へ訂正後の確定) |
| 2 | **(R-A/B/C)** drift remediation | (R-A) = append-only + 本 audit doc を 1 件 reference として運用 | **(R-A) 確定** | AYA literal「引き続き作業 handoff までお願いします」2026-06-04 受領 = Claude 推奨で進行 implicit 承認 |
| 3 | **PC-6ε continue** | audit phase 終了 + PC-6δ' marker + PC-6ε-1 着手は次 session | **次 session 着手確定** | 同上 = 「handoff まで」literal = 本 session は audit + handoff で締め |
| 4 | **audit doc 配置** | `docs/specs/ayastorm-r41-gl-removal/design/literal-cross-ref-audit.md` 現状維持 | **現状配置確定** | 同上 implicit 承認 + design chapter 群と同居 = 参照規律維持 |

### §10.2 次 session 着手 1 line

PC-6ε-1 = `flushSingletonUbos()` 新設 + `bringupTestUBO()` (`indra/llrender/llglslshader.cpp:2032`) を `LLVKLoader::flushSingletonUbos()` 経由に置換 + 6 cadence 関数体系成立 + TUT 3 件 + codegen 130/130 + llrender build PASS。

§4.3 (S1) 採用時の sub-scope 通り。Exit Criteria 7 項。

### §10.3 PC-6δ' (= 本 audit interlude) complete marker

本 audit doc = `literal-cross-ref-audit.md` 起案完了 = PC-6δ' (= delta-prime) complete。Phase 1.C 進行 markers:

- PC-0 ✅ / PC-1 ✅ / PC-2 ✅ / PC-3 ✅ / PC-4 ✅ / PC-5 ✅ / PC-6α ✅ / PC-6β ✅ / PC-6γ ✅ / PC-6δ ✅ / **PC-6δ' ✅ 本 audit** / PC-6ε..PC-N ⏳ 次 session
