# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-1 完了

**作成日**: 2026-06-04
**前 session commit**:
- `7fe58b7428` (= 副次 (a) `_PREFIX_TO_CADENCE` 拡張 + suffix table 追加、cadence default fallback 累計 85 件解消)
- `0ba743463c` (= PB-1 LLGLSLShader cache 構造 3 member 追加 + GATE-B 確定)

**本 session 物理出力** (= 全て **未 commit**、AYA さん明示指示後 batch commit):
- 本 handoff doc 新規 (= PB-1 complete = host C++ redirect 層 第 1 sub-step 終端)

**次 session 着手**: **AYA 判断確定済** (= 2026-06-04 AYA 判断 4 件 (entry handoff §3.6) + GATE-B (本 handoff §3.1) 全採用済) → **PB-2 着手** = `mapUniforms()` Vulkan path integer index 経路拡張、AYA 確認不要、Claude 自走可。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-1 complete state** (= 直前 commit `0ba743463c`):
- **副次 (a) `_PREFIX_TO_CADENCE` 拡張 完了** (= commit `7fe58b7428`) = main.py L65-72 に `PerDrawUBO_` → 2 / `PerProgramUBO_` → 1 / `_SUFFIX_TO_CADENCE` 新表 `UBO_Legacy` → 1 追加 + unittest 3 件追加 = 累計 90 UBO cadence 分布 `0:3 + 1:80 + 2:7` 物理確認
- **PB-1 `mUniformUBOLoc` cache 構造実装 完了** (= commit `0ba743463c`) = `indra/llrender/llglslshader.h` L36-41 + L404-428 = `#include "ubo/ubo_perfect_hash.inl"` 追加 + 3 member 追加 (`std::vector<ubo::UniformLocation> mUniformUBOLoc` / `std::unordered_map<U64, ubo::UniformLocation> mUniformUBOLocByHash` / `bool mUseUBO = false`) + full viewer build PASS (= `[100%] Built target llpackage` + tar.xz package)
- **GATE-B 確定** (= 本 handoff §3.1 で新規確定、AYA 「GATE-B で」literal 受領 2026-06-04) = C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate。Phase 1.B 全 sub-step (PB-2〜PB-7) で適用継続
- **GATE-B memory 保存** = `project_r41_phase1b_vulkan_host_gate.md` 新規 + `MEMORY.md` index 1 line 追加
- **残 sub-step**: PB-2 → PB-3 → PB-4.1〜.17 → PB-5.1〜.13 → PB-6 → PB-7 → PB-N strict 線形
- **AYA 判断確定済** (= 本 handoff 起案後 AYA 確認待ち項目 0 件) = entry handoff §3.6 の 4 件 + 本 session で確定した GATE-B = 全 default 採用済、Claude 自走継続可

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-1-complete.md`) | 全文 | 副次 (a) + PB-1 完了 state + GATE-B 確定根拠 + 残 sub-step (PB-2〜PB-N) 順序 + PB-2 着手第 1 step |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §4 全文 (= `mapUniforms()` Vulkan path 拡張 + R3 設計 hash 計算 shader link 時 1 回限り + LLStaticHashedString 補助 path + §4.3.1 S1-A/B/C/D 4 案 + §4.4 整合 check) | PB-2/PB-3 chapter source-of-truth (= mapUniforms() 末尾に Vulkan path block 追加の code shape) |
| 3 | `indra/llrender/llglslshader.cpp:1700-1900` | `mapUniforms()` 既存実装 + 周辺 (= active uniform 列挙 loop + mapUniform(GLint) helper) | PB-2 着手前 mapUniforms 既存実装把握 = どこに `if (mUseUBO)` block を挿入するか確定 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-entry.md` | entry handoff §3.6 AYA 判断 4 件確定表 (= S1-C / FWD-1 / MUSEUBO-A / §2.3 default) + §2.3 PB-1〜PB-N 構成表 |
| `indra/llrender/llglslshader.h:404-428` | 本 session 追加 3 member (= `mUniformUBOLoc` + `mUniformUBOLocByHash` + `mUseUBO`) + L36-41 codegen include 確認 |
| `build-linux-x86_64/codegen/ubo/ubo_perfect_hash.inl` | `struct ubo::UniformLocation` 定義 L13-18 + `lookup_runtime(name)` API (= PB-2 で使用予定) |
| `build-linux-x86_64/codegen/ubo/ubo_metadata.inl` | 90 block metadata (= cadence_tag 分布 0:3 + 1:80 + 2:7、副次 (a) 反映後) |
| `build-linux-x86_64/codegen/ubo/ubo_index.inl` | aggregated include hub (= PB-2/PB-3 で使用する block 解決 API hub) |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md:§5` | 17 method 一覧 + path 分岐 code shape (= PB-4 着手時) |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-update-site-and-dirty-flag.md` | `forwardToUboUpload()` interface 確認 (= PB-6 shell 実装時、本実装は Phase 1.C 譲り) |
| `scripts/ubo_codegen/main.py:65-78` | `_PREFIX_TO_CADENCE` + `_SUFFIX_TO_CADENCE` (= 副次 (a) 拡張後の表) + `_derive_cadence` 関数 |

---

## §2 残 sub-step (PB-2〜PB-N) strict 線形構成 (= entry handoff §2.3 から継承)

| PB-X | 内容 | 該当 chapter | 物理改変 file | 出力契約 | 状態 |
|---|---|---|---|---|---|
| ~~PB-1~~ | ~~`mUniformUBOLoc` cache 構造実装~~ | 06a §3 | `indra/llrender/llglslshader.h` | header + full build PASS | **✅ 完了** (commit `0ba743463c`) |
| **PB-2** | `mapUniforms()` Vulkan path integer index 経路拡張 = `mapUniforms()` 末尾 (= L1700+) に `if (mUseUBO) { mUniformUBOLoc.resize(mUniform.size()); for (i: mReservedUniforms) { lookup_runtime(name) → mUniformUBOLoc[i] }}` block 追加。GATE-B 整合で `#ifdef LL_VULKAN_GLSL` 不使用、`if (mUseUBO)` runtime gate のみ | 06a §4.1 / §4.2 | `indra/llrender/llglslshader.cpp:1700-` | build PASS + `mUseUBO=false` default ゆえ Vulkan path 走らず既存挙動不変 |
| **PB-3** | LLStaticHashedString 経路 cache 構築 (= S1-C 採用確定) = build-time list `g_static_hashed_uniform_names[]` を新規追加 (= chapter 05 集約表確定時の list を build-time 静的配列化) + shader link 時に iterate して `lookup_runtime(name)` → `mUniformUBOLocByHash[hash] = loc` 登録 | 06a §4.3 / §4.3.1 (S1-C) | `mapUniforms()` 末尾 PB-2 block 直後 + 新規 `g_static_hashed_uniform_names[]` (= 場所判断要、`llglslshader.cpp` ファイル内 anonymous namespace か、別 file か) | build PASS + 既存挙動不変 |
| **PB-4** | 17 method (integer index 経路) Vulkan path 分岐追加 = 各 method 末尾 `glUniform*` 直前に `if (mUseUBO) { auto& loc = mUniformUBOLoc[index]; if (loc.cadence_tag != CADENCE_SAMPLER) { forwardToUboUpload(loc, data, size); return; }}` 分岐挿入。1 method 1 PB-4.X sub-step で逐次実装 (= `feedback_ubo_migration_one_at_a_time` 準拠、17 setter 一括禁止) | 06a §5.2 / §5.3 / §5.6 | `indra/llrender/llglslshader.cpp` 17 箇所 | 17 method 全 compile PASS + `mUseUBO=false` 経路 unchanged |
| **PB-5** | 13 method (LLStaticHashedString 経路) Vulkan path 分岐追加 = 各 method の `const LLStaticHashedString&` overload 版に `mUniformUBOLocByHash.find(hash)` lookup → `forwardToUboUpload()` 分岐追加。1 method 1 PB-5.X sub-step で逐次実装 | 06a §5.5 | `indra/llrender/llglslshader.cpp` 該当 LLStaticHashedString overload 13 箇所 | 13 method 全 compile PASS + `mUseUBO=false` 経路 unchanged |
| **PB-6** | `forwardToUboUpload()` shell 実装 (= FWD-1 採用確定) = `void LLGLSLShader::forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size) {}` 空関数 | 06a §5.2 (= 「(= 06b で実装)」literal) + 06b 譲り | `indra/llrender/llglslshader.cpp` 末尾 + `llglslshader.h` method 宣言追加 | shell 関数 link PASS + PB-4/PB-5 path 内 call 解決 |
| **PB-7** | 整合 check 仕込み = `mapUniforms()` 末尾 PB-2/PB-3 拡張部直後に debug build 用 `llassert(mUniformUBOLoc.size() == mUniform.size())` + cadence_tag check + `cadence_tag == CADENCE_SAMPLER` の uniform は OpenGL path 強制 check | 06a §4.4 | `mapUniforms()` 末尾 | debug build PASS + assert 動作確認 (= release build では `#ifndef LL_RELEASE_FOR_DOWNLOAD` 等で除去判断) |
| **PB-N (= PB-8)** | Phase 1.B Exit Criteria 検証 = full build + install + cache clear + viewer launch + bind 不変動作確認 (= Phase 1.A complete §2.3 の「経路非到達 3 観点 verify」を Phase 1.B 後 state で再走) + log で Phase 1.B 起因 fail 0 件確認 + AYA さん起動目視「変化していないと思う」確認 + Phase 1.B 全終了 handoff doc 起案 | 09 §4.2 | (検証 phase = 物理 code 改変なし) | viewer 起動 + login + region entry PASS + log 内 keyword 0 件 fail + AYA 確認後 commit |

**注**: PB-2 着手前に `mapUniforms()` 既存実装 (= `llglslshader.cpp:1700-1900`) を必ず pinpoint Read = どの位置に Vulkan path block を挿入するか + 既存の `mUniform.size()` / `mReservedUniforms` iterate pattern との整合確認。

---

## §3 本 session で新規確定した設計判断

### §3.1 GATE-B 確定 (= C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate)

**設計問**: 06a §3.2 (L117-124) + §5.3 (L333) literal で C++ host code に `#ifdef LL_VULKAN_GLSL` compile-time gate を指定。しかし `LL_VULKAN_GLSL` は **GLSL preprocessor 専用** macro (= `llshadermgr.cpp:543` + `llglslshader.cpp:1159` で `concatenated.append("#define LL_VULKAN_GLSL 1\n")` は SPIR-V source 生成 path 内のみ、C++ context では未定義)。literal 実装すると `mUniformUBOLoc` / `mUniformUBOLocByHash` / `mUseUBO` + PB-4/PB-5 全 setter の `if (mUseUBO)` 分岐が preprocessor 段で消滅 = 完全 dead code 化。

**3 案** (= 本 session で AYA に提示):

| 案 | 内容 | 採否 |
|---|---|---|
| GATE-A | 06a literal 通り `#ifdef LL_VULKAN_GLSL` 維持 → C++ 側でも `LL_VULKAN_GLSL` を define する手段追加 (= CMake `target_compile_definitions` で llrender に gate 付き注入 + viewer config option) | 退け |
| **GATE-B (= AYA 承認、Claude 推奨)** | 06a literal を破棄、既存 r41 pattern (= `mStageSources` / `mVulkanAttachedVertexUtilities` 等、gate なし + runtime flag のみ) に揃える | **採用** |
| GATE-C | 別案 = 新規 C++ macro `LL_VULKAN_GLSL_HOST` を別途定義して spec の意図を C++ 側で再現 | 退け |

**採用根拠**:
- (i) `LL_VULKAN_GLSL` は GLSL 専用 macro、C++ では未定義 (= grep 5 種で確認 = `define LL_VULKAN_GLSL` / `target_compile_definitions` / `add_compile_definitions` 全 0 件 except SPIR-V source concat)
- (ii) literal 適用すると 3 member + 30 setter `if (mUseUBO)` 分岐全 preprocessor 段で消滅 = dead code 化
- (iii) 既存 r41 C++ Vulkan member (`mStageSources` h:386 / `mVulkanAttachedVertexUtilities` h:394 / `mVulkanAttachedFragmentUtilities` h:395) は compile-time gate 無 + runtime は `LLVKLoader::isVulkanInitialized()` + member flag pattern で揃っている
- (iv) MUSEUBO-A 確定 (= `mUseUBO=false` default) と組合せれば dead code 化と同等の path 非到達効果 + Phase 1.B Exit Criteria literal「mUseUBO=false default で OpenGL path 経路維持」と整合 + build system 改変ゼロで scope 最小

**AYA 指示 literal** (= 2026-06-04): 「GATE-B で」 → GATE-B 確定。

**memory 保存**: `project_r41_phase1b_vulkan_host_gate.md` 新規 + `MEMORY.md` index 1 line 追加 = Phase 1.B 全 sub-step (PB-2〜PB-7) で再適用。spec 06a §3.2 / §5.3 literal `#ifdef LL_VULKAN_GLSL` は **C++ 側のみ非適用**、GLSL shader 側は引き続き有効 (= dual-context macro = GLSL では injected define 経由活性、C++ では未定義のまま)。

### §3.2 PB-1 着手中 include path 修正 1 件

**設計問**: 初回 `#include "codegen/ubo/ubo_perfect_hash.inl"` を llglslshader.h に追加して build → llrender 全 12 source compile fail = `codegen/ubo/ubo_perfect_hash.inl: そのようなファイルやディレクトリはありません`。

**根本原因**: `AYA_UBO_CODEGEN_INCLUDE_DIR` = `${CMAKE_BINARY_DIR}/codegen` (= 既に `codegen/` まで含む)。出力 dir = `${CMAKE_BINARY_DIR}/codegen/ubo/`。`#include "codegen/ubo/..."` だと search path 起点 `${CMAKE_BINARY_DIR}/codegen` から `codegen/ubo/...` を探しに行き `${CMAKE_BINARY_DIR}/codegen/codegen/ubo/...` を見て fail。

**fix**: spec 08 §99 literal「shader 側 setter から `#include "ubo/ubo_index.inl"` 形」整合で `"ubo/ubo_perfect_hash.inl"` に修正 → llrender build PASS + full viewer build PASS。

**注 (= 次 session 注意)**: PB-2/PB-3 で codegen artifact を追加 include する場合 (= `ubo_index.inl` / `ubo_host_loader.inl` 等) も同 pattern `"ubo/<file>.inl"` で書く。Phase 1.A の sanity_check.cpp は /tmp/ 独立 cmake project ゆえ include 形が異なる ("codegen/ubo/..." 形)、本物 build 系では "ubo/..." 形が正。

---

## §4 self-verify (= 9 観点、本 handoff 起案時点)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) 副次 (a) commit 確認 | `git log --oneline | head -3` で `7fe58b7428` (= `_PREFIX_TO_CADENCE` 拡張) 物理存在 + unittest 130/130 PASS baseline + cadence 分布 `0:3 + 1:80 + 2:7` ubo_metadata.inl で物理確認 | ✅ |
| (2) PB-1 commit 確認 | `git log --oneline | head -1` で `0ba743463c` (= LLGLSLShader cache 構造 3 member 追加) 物理存在 + `indra/llrender/llglslshader.h` L36-41 + L404-428 物理確認 + `git diff --stat` で 1 file changed + 34 insertions | ✅ |
| (3) llrender 単体 build PASS | `make -j$(nproc) llrender` で 12 source compile + `libllrender.a` link OK | ✅ |
| (4) full viewer build PASS | `make -j$(nproc)` で `[100%] Built target llpackage` + tar.xz package `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz` 生成 + downstream 23 file llglslshader.h include consumer 全 satisfy (= newview/llsettingsvo + lldrawpoolwlsky + llvieweroctree + llenvironment.h + lldrawpool 他 + llappearance/lltexlayer.h) | ✅ |
| (5) GATE-B 確定 + memory 保存 | `project_r41_phase1b_vulkan_host_gate.md` 新規 + `MEMORY.md` index 1 line 追加 + AYA 「GATE-B で」literal 受領 | ✅ |
| (6) include path 修正 | 初回 `"codegen/ubo/..."` build fail → 自疑い → spec 08 §99 literal 確認 → `"ubo/..."` 修正 → build PASS (= `feedback_doubt_self_first` 適用) | ✅ |
| (7) Phase 1.B Exit Criteria literal 充足準備 | mUseUBO=false default 維持 (= MUSEUBO-A) + GATE-B (= compile-time gate 無、runtime flag のみ) で OpenGL path 既存挙動 100% 維持、Phase 1.B 後 viewer launch 時の bind 不変動作の基礎完了 | ✅ |
| (8) 残 sub-step strict 線形 | PB-2 → PB-3 (= S1-C) → PB-4.1〜.17 → PB-5.1〜.13 → PB-6 (= FWD-1) → PB-7 → PB-N (= MUSEUBO-A) + 1 method 1 sub-step (= `feedback_ubo_migration_one_at_a_time` 準拠) | ✅ |
| (9) git working tree 状態 | `git status` で本 handoff doc のみ新規、indra/ 改変なし + scripts/ 改変なし + Co-Authored-By: Claude 行不在 | ✅ |

---

## §5 引き継ぎ済 memory (= 次 session で active、entry handoff から継承 + 1 件追加)

- `project_r41_phase1b_vulkan_host_gate.md` **(= 本 session 追加)** — Phase 1.B 全 sub-step (PB-2〜PB-7) で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守、subset 分岐実装で済まさない
- `feedback_self_verify_before_handoff` — PB-N 検証時に AYA 起動目視前に Claude 「3 経路非到達 verify」を Phase 1.B 後 state で再走
- `feedback_no_claude_coauthor` — 本 handoff doc 含め全 commit 共著行不在
- `feedback_one_step_at_a_time` — PB-2 → PB-3 → … → PB-N strict 線形、各 PB-X 単独 commit
- `feedback_ubo_migration_one_at_a_time` — PB-4 内部 17 method / PB-5 内部 13 method 一括禁止、1 method 1 sub-step
- `feedback_doubt_self_first` — PB-X 中の不可解な build error / link fail は AYA に投げる前に Claude が root cause 特定 (= 本 session の include path 修正で適用済)
- `feedback_admit_unknown` — Vulkan path 動作確認等の判断は推測で進めず source-of-truth (= 06a chapter) literal 再確認
- `feedback_proactive_handoff` — Phase 1.B 全終了時に Phase 1.C entry handoff doc 起案
- `feedback_no_auto_commit` — 本 handoff doc + 各 PB-X commit は AYA 明示指示後 commit
- `feedback_remove_verification_logs` — PB-X 実装中の `LL_INFOS` hook は commit 前必ず除去、debug `llassert` は仕様内ゆえ残す
- `feedback_build_only_verified` — Phase 1.B Exit Criteria literal 充足 (= viewer launch + 経路非到達 verify) 後 commit
- `feedback_tests_dir_never_commit` — scripts/ubo_codegen/tests/ は commit 不可
- `feedback_build` — Phase 1.B PB-N full build フローは Claude 全権実行 (= `project_build_procedure` 参照)
- `project_build_procedure` — PB-N で参照 (= configure → build → install → cache clear フル実行)
- `project_ayastorm_r41_vulkan_migration` — Phase 1.B PB-1 完了 = 次 PB-2 着手状態
- `project_ayastorm_r41_design_principles` — 原則 1「upstream 取込やすさ維持」 = GATE-B 採用根拠 (= 既存 r41 pattern 整合性)
- `project_ayastorm_three_platforms` — Phase 1.B Exit Criteria literal 充足は Linux first-class baseline で十分
- `feedback_use_agents_proactively` — PB-X 内部の 3 file 以上確認 / 経路 trace は Agent 投入候補
- `feedback_explanation_lead_with_conclusion` — AYA 判断項提示時に結論先

---

## §6 次 session 着手 1 line

**「前 session で Phase 1.B 副次 (a) `_PREFIX_TO_CADENCE` 拡張 (commit `7fe58b7428`) + PB-1 LLGLSLShader cache 構造 3 member 追加 + GATE-B 確定 + memory 保存 + full viewer build PASS (commit `0ba743463c`) を完了 + PB-1 完了 handoff doc 起案。本 session = **AYA 判断不要 自走** = **PB-2 着手** = `mapUniforms()` Vulkan path integer index 経路拡張 (= `indra/llrender/llglslshader.cpp:1700-` `mapUniforms()` 末尾に `if (mUseUBO) { mUniformUBOLoc.resize(mUniform.size()); for (i: mReservedUniforms) { lookup_runtime(name) → mUniformUBOLoc[i] }}` block 追加、GATE-B 整合で `#ifdef LL_VULKAN_GLSL` 不使用、`if (mUseUBO)` runtime gate のみ、06a §4.1/§4.2 literal) → build PASS 確認 → 独立 commit → 以降 PB-3 (= S1-C build-time list) → PB-4.1〜.17 (17 method integer index) → PB-5.1〜.13 (13 method hashed string) → PB-6 (= FWD-1 完全 stub) → PB-7 (= 整合 check) → PB-N (= MUSEUBO-A `mUseUBO=false` default で full build + viewer launch + bind 不変 verify) と strict 線形進行。」**
