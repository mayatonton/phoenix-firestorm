# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-6 完了

**作成日**: 2026-06-04
**前 session commit** (= η-30 Phase 1.B 系列、新しい順):
- `d72a481e14` (= PB-3 complete handoff doc 起案)
- `127d25ecb6` (= PB-3 LLStaticHashedString 経路補助 cache 構築 +47 line)
- `8ad78b42b0` (= PB-2 complete handoff doc 起案)
- `e43d93dd25` (= PB-2 `mapUniforms()` Vulkan path integer index 経路 cache 構築 block 追加)
- `7032a0b1a8` (= PB-1 complete handoff doc 起案)
- `0ba743463c` (= PB-1 LLGLSLShader cache 構造 3 member 追加)
- `7fe58b7428` (= 副次 (a) `_PREFIX_TO_CADENCE` 拡張)

**本 session 物理出力** (= 全て **未 commit**、AYA さん明示指示後 batch commit):
- `indra/llrender/llglslshader.h` modified (= +13 line、`generatePerProgramSPIRV()` 直後 private section に `forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size)` 宣言追加 + 11 line comment header)
- `indra/llrender/llglslshader.cpp` modified (= +13 line、`mapUniforms()` と `link()` の間 = L1952-1964 に空 stub `{}` 本体実装 + 10 line comment header)
- 本 handoff doc 新規 (= PB-6 complete = host C++ redirect 層 第 4 sub-step 終端 marker、順序組替えで PB-4 直前に前倒し実施)

**次 session 着手**: **AYA 判断不要 自走可** (= 2026-06-04 AYA 判断 4 件 (entry handoff §3.6) + GATE-B (PB-1 complete handoff §3.1) + PB-2 設計判断 1 件 + PB-3 設計判断 5 件 + 順序組替え 1 件 + 本 session 確定 設計判断 2 件 (= §3.1 / §3.2) + 運用知見 1 件 (= §3.3) 全採用済) → **PB-4.1 着手** = `uniform1i(U32 index, GLint x)` (= `llglslshader.cpp:2227-` 1 番目の integer index 経路 setter) に Vulkan path 分岐 `if (mUseUBO) { ... forwardToUboUpload(loc, &x, sizeof(GLint)); return; }` 追加 (1 method 1 sub-step、`feedback_ubo_migration_one_at_a_time` 準拠)。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-6 complete state**:
- **PB-6 `forwardToUboUpload()` shell 実装 完了** (= `indra/llrender/llglslshader.h` + `indra/llrender/llglslshader.cpp` modified、未 commit) =
  - (a) header private section (`llglslshader.h:443-` `generatePerProgramSPIRV()` 直後) に `void forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size);` 宣言 + 11 line comment header (= FWD-1 採用根拠 + PB-3 §3.6 順序組替え根拠 + MUSEUBO-A 整合明示)
  - (b) cpp `mapUniforms()` 直後 = `link()` 直前 (`llglslshader.cpp:1952-1964`) に空 stub `void LLGLSLShader::forwardToUboUpload(...) {}` + 10 line comment header
- **GATE-B 整合** = `#ifdef LL_VULKAN_GLSL` 不使用、stub 空 body ゆえ runtime 走らず、gate 議論自体に到達しない。
- **mUseUBO=false default 維持** (= MUSEUBO-A) ゆえ stub 呼出元 (= PB-4/PB-5 で追加予定) が走らず、stub 到達 0 回、既存 OpenGL 挙動 100% 維持。
- **本 session 設計判断 2 件** (= §3.1 / §3.2) =
  - §3.1 visibility = `private` 採用 (= 内部 helper として `generatePerProgramSPIRV()` と同 layer に配置、外部 caller 想定なし)
  - §3.2 stub 配置場所 = `mapUniforms()` と `link()` の間 (= cache 構築 ↔ 消費の物理近接、PB-2/PB-3 cache 構築 block 直後の自然 layout)
- **本 session 運用知見 1 件** (= §3.3) = stale build artifact dir 衝突 (= `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586/`) → 物理改変無関係の packaging 失敗、rm で 1 cleanup → 完遂。次 PB-X 系列で同 conflict 発生時 1 line 対処。
- **llrender 単体 build PASS** + **full viewer build PASS** = `[100%] Built target llpackage` + tar.xz package `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz` 生成。
- **残 sub-step**: PB-4.1〜.17 (= 17 method integer index、1 method 1 sub-step) → PB-5.1〜.13 (= 13 method LLStaticHashedString、1 method 1 sub-step) → PB-7 (= 整合 check) → PB-N (= MUSEUBO-A verify) strict 線形。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-6-complete.md`) | 全文 | PB-6 完了 state + 設計判断 2 件 + 運用知見 1 件 + 残 sub-step 順序 + PB-4.1 着手詳細 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` §5.2 / §5.3 (= integer index 経路 path 分岐 code shape + 分岐配置の規律) | 当該節のみ | PB-4.1 = `uniform1i(U32 index, GLint x)` setter 内 Vulkan path 分岐 pattern 確定 |
| 3 | `indra/llrender/llglslshader.cpp:2227-2245` (= PB-4.1 対象 `uniform1i(U32 index, GLint x)` 既存実装) + `indra/llrender/llglslshader.h:443-457` (= PB-6 で追加した `forwardToUboUpload()` 宣言 + comment header) | 当該箇所のみ | PB-4.1 改変対象の既存 code 確認 + 呼出 helper の signature 確認 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-3-complete.md` | PB-3 完了 state + 順序組替え §3.6 (= PB-6 前倒し根拠) + 残 sub-step 表 |
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-2-complete.md` | PB-2 完了 state + 挿入位置設計判断 §3.1 |
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-1-complete.md` | PB-1 完了 state + GATE-B 確定根拠 §3.1 |
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-entry.md` | entry handoff §3.6 AYA 判断 4 件確定表 (= S1-C / FWD-1 / MUSEUBO-A / §2.3 default) |
| `indra/llrender/llglslshader.h:427-429` | PB-1 で追加した 3 member (`mUniformUBOLoc` / `mUniformUBOLocByHash` / `mUseUBO`) |
| `indra/llrender/llglslshader.h:443-457` | PB-6 で追加した `forwardToUboUpload()` 宣言 + comment header |
| `indra/llrender/llglslshader.cpp:1899-1923` | PB-2 で追加した `mapUniforms()` 内 integer index 経路 cache 構築 block |
| `indra/llrender/llglslshader.cpp:1925-1944` | PB-3 で追加した `mapUniforms()` 内 LLStaticHashedString 経路 iterate block |
| `indra/llrender/llglslshader.cpp:1952-1964` | PB-6 で追加した `forwardToUboUpload()` 空 stub 本体 + comment header |
| `indra/llrender/llglslshader.cpp:2227-2538` | integer index 経路 17 method 現状 (= PB-4 着手時、1 method 1 sub-step) |
| `indra/llrender/llglslshader.cpp:2643-2870` | LLStaticHashedString 経路 13 method 現状 (= PB-5 着手時) |
| `build-linux-x86_64/codegen/ubo/ubo_perfect_hash.inl:887-908` | `lookup_runtime(const char* name)` + `fnv1a_32()` API (= PB-2/PB-3 利用済、PB-4/PB-5 で直接利用なし) |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md:§5.5` | LLStaticHashedString 13 method path 分岐 code shape (= PB-5 着手時) |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md:§5.6` | sampler 系 setter OpenGL path 強制 (= PB-4 / PB-5 内、`cadence_tag == 5` skip path 根拠) |

---

## §2 残 sub-step (PB-4 → PB-5 → PB-7 → PB-N) strict 線形構成 (= PB-3 順序組替え反映済)

| PB-X | 内容 | 該当 chapter | 物理改変 file | 出力契約 | 状態 |
|---|---|---|---|---|---|
| ~~PB-1~~ | ~~`mUniformUBOLoc` cache 構造実装~~ | 06a §3 | `indra/llrender/llglslshader.h` | header + full build PASS | **✅ 完了** (commit `0ba743463c`) |
| ~~PB-2~~ | ~~`mapUniforms()` Vulkan path integer index 経路 cache 構築~~ | 06a §4.1 / §4.2 | `indra/llrender/llglslshader.cpp:1899-1923` | +26 line `if (mUseUBO) { resize + lookup_runtime loop }` block | **✅ 完了** (commit `e43d93dd25`) |
| ~~PB-3~~ | ~~LLStaticHashedString 経路 cache 構築 (= S1-C 採用)~~ | 06a §4.3 / §4.3.1 | `indra/llrender/llglslshader.cpp:993-1017` + `1925-1944` | +47 line `g_static_hashed_uniform_names[]` 80 名 + iterate block | **✅ 完了** (commit `127d25ecb6`) |
| ~~PB-6~~ | ~~`forwardToUboUpload()` shell 実装 (= FWD-1 採用、空 stub)~~ | 06a §5.2 (= 「(= 06b で実装)」literal) | `indra/llrender/llglslshader.h` (= 宣言) + `indra/llrender/llglslshader.cpp:1952-1964` (= 空 stub) | shell 関数 link PASS、後続 PB-4/PB-5 で call 解決準備完了 | **✅ 完了** (本 session、未 commit) |
| **PB-4.1** (= 次 session 着手) | **`uniform1i(U32 index, GLint x)`** (`llglslshader.cpp:2227-2245`) に Vulkan path 分岐追加 = `glUniform1i` 直前に `if (mUseUBO) { llassert(index < mUniformUBOLoc.size()); const auto& loc = mUniformUBOLoc[index]; if (loc.cadence_tag == 0xFFFFFFFFu) return; if (loc.cadence_tag == 5 /* sampler */) return; forwardToUboUpload(loc, &x, sizeof(GLint)); return; }` | 06a §5.2 / §5.3 / §5.6 | `indra/llrender/llglslshader.cpp:2227-2245` 1 method | compile PASS + mUseUBO=false 経路 unchanged | **未着手 = 次 session 着手** |
| PB-4.2〜.17 | 残 16 method (= integer index 経路) Vulkan path 分岐追加。1 method 1 sub-step strict 順守 = `uniform1f` / `fastUniform1f` / `uniform2f` / `uniform3f` / `uniform4f` / `uniform1iv` / `uniform4iv` / `uniform1fv` / `uniform2fv` / `uniform3fv` / `uniform4fv` / `uniform4uiv` / `uniform2i(const LLStaticHashedString&, ...)` (= 例外 1 件、命名は hashed だが overload 種別 = integer index 系の path 1 種、確認要) / `uniformMatrix2fv` / `uniformMatrix3fv` / `uniformMatrix3x4fv` / `uniformMatrix4fv` (= 16 method、PB-4.1 の `uniform1i` 含めて計 17 method) | 06a §5.2 / §5.3 / §5.6 | `indra/llrender/llglslshader.cpp` 16 箇所 | 各 method compile PASS + 全 method 完了後 full build PASS |
| PB-5.1〜.13 | 13 method (LLStaticHashedString 経路) Vulkan path 分岐追加 = 各 method の `const LLStaticHashedString&` overload 版に `mUniformUBOLocByHash.find(static_cast<U64>(uniform.Hash()))` lookup → `forwardToUboUpload()` 分岐追加。1 method 1 PB-5.X sub-step | 06a §5.5 | `indra/llrender/llglslshader.cpp` 該当 LLStaticHashedString overload 13 箇所 (= line 2643-2870) | 13 method 全 compile PASS + mUseUBO=false 経路 unchanged |
| PB-7 | 整合 check 仕込み = `mapUniforms()` 末尾 PB-2/PB-3 拡張部直後に debug build 用 `llassert(mUniformUBOLoc.size() == mUniform.size())` + cadence_tag check | 06a §4.4 | `mapUniforms()` 末尾 | debug build PASS + assert 動作確認 |
| PB-N (= PB-8) | Phase 1.B Exit Criteria 検証 = full build + install + cache clear + viewer launch + bind 不変動作確認 + log で Phase 1.B 起因 fail 0 件確認 + AYA さん起動目視「変化していないと思う」確認 + Phase 1.B 全終了 handoff doc 起案 | 09 §4.2 | (検証 phase = 物理 code 改変なし) | viewer 起動 + login + region entry PASS + AYA 確認後 commit |

---

## §3 本 session で新規確定した設計判断 (2 件) + 運用知見 (1 件)

### §3.1 visibility = `private` 採用 (= 内部 helper として外部 caller 想定なし)

**設計問**: `forwardToUboUpload()` を `public` / `protected` / `private` のいずれに置くか。

**判断**: **`private`** 採用 (= `llglslshader.h:435-` 既存 `private:` section、`generatePerProgramSPIRV()` と同位置 = `unloadInternal()` / `sDefaultStats` と並列)。

**根拠**:
- (i) `forwardToUboUpload()` は **内部 helper** = setter family 内部の path 分岐から呼ばれる以外の使用想定なし、外部 (= `LLGLSLShader` instance 経由の caller) からの直接呼出は設計上ありえない
- (ii) 既存の internal infrastructure helper `generatePerProgramSPIRV()` (= r41 sub-step 4.3-γ'-port-β-2 で追加) と同 layer = Vulkan/UBO 関連の private helper 群と並列配置で組織化
- (iii) `public` だと外部から `mUseUBO` を意識せず直呼出される懸念 = 06b/chapter 07 で ring buffer / thread 配線実装時に invariant 違反の risk
- (iv) `protected` は subclass (= LLGLSLShader を継承する class) を想定しないため過剰、`private` で十分
- (v) spec 06a §5.2 の code shape では visibility 未明示 = 自由度ありゆえ implementor 判断で `private` 採用が自然

**memory 保存不要** = PB-6 限定の visibility 判断、Phase 1.B 全 sub-step 一般原則ではない。本 handoff doc §3.1 で記録足りる。

### §3.2 stub 配置場所 = `mapUniforms()` 直後 = `link()` 直前 (= cache 構築 ↔ 消費の物理近接)

**設計問**: `forwardToUboUpload()` 空 stub 本体の cpp 内配置位置。候補 = (a) `mapUniforms()` 直後 = `link()` 直前 / (b) file 末尾 (= `setLabel()` 直後 / `#endif` 外) / (c) setter 群 (= `uniform*` 17+13 method) の前後 / (d) anonymous namespace 内 (= PB-3 で追加した 80 名表の近傍)。

**判断**: **(a) `mapUniforms()` 直後 = `link()` 直前** (= `llglslshader.cpp:1952-1964`)。

**根拠**:
- (i) `mapUniforms()` 内で PB-2/PB-3 が cache 構築 (= `mUniformUBOLoc` resize + `mUniformUBOLocByHash` 登録)、`forwardToUboUpload()` がその cache を消費する関数 = **cache 構築 ↔ 消費の物理近接** で読み手の mental model が直結
- (ii) (b) file 末尾は `#if LL_PROFILER_ENABLE_RENDER_DOC` block 内 = conditional compile 範囲、stub を入れると条件依存 visibility になる = 不適
- (iii) (c) setter 群の前後は将来 PB-4/PB-5 で setter が path 分岐内から `forwardToUboUpload()` を call 開始 = call site と implementation site の極端な近接で、code review 時の文脈跳躍が小さい (= setter 群最終位置の `uniformMatrix4fv` (`llglslshader.cpp:2538` 付近) 直後でも可) だが、**setter 群より cache 構築側 (= `mapUniforms()`) の方が論理的に上流ゆえ (a) を優先**
- (iv) (d) anonymous namespace 内は file scope = `LLGLSLShader::` member 関数の定義場所として不適 (= linkage 違反 / friend 必要)、退け
- (v) Phase 1.C で 06b 実装着手時 (= 空 stub → 実装本体) に同位置で書き換えれば良い = 関数移動の churn が発生しない

**memory 保存不要** = PB-6 限定の配置判断、本 handoff doc §3.2 で記録足りる。

### §3.3 運用知見: stale build artifact dir 衝突 → rm 1 cleanup で解消

**現象**: 初回 `make -C build-linux-x86_64 -j$(nproc)` 実行時、compile + link は完全 PASS したが、最終 `viewer_manifest.py` の `mv packaged/ Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586/` step で **destination directory exists, not empty** エラー → `llpackage` target 失敗。

**原因**: 前 session PB-3 commit (= `127d25ecb6`) 時点の build artifact `build-linux-x86_64/newview/Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586/` directory が remove されず残存 (= 通常 build flow では tar.xz 生成後 packaged/ に rename し直すが、何かの中断で半端な状態が残ったと推測)。

**対処**: `rm -rf build-linux-x86_64/newview/Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586{,.tar.xz}` で 2 件除去 → 再 make で `Built target llpackage` 到達。

**根拠**:
- (i) compile + link 自体は PASS (= 物理 code 改変 = PB-6 stub 13 line に起因しない)
- (ii) memory `project_build_procedure` literal「configure → build → rm → install → cache clear」中の `rm` step に該当 = 通常 flow の一部、destructive op だが build artifact 限定で安全
- (iii) `feedback_build_only_verified` 整合 = 物理 code 改変無関係の packaging 失敗を「PB-6 build NG」と誤認しないため、原因切り分けを明示記録

**memory 保存不要** = build artifact 衝突は環境固有の運用問題、Phase 1.B 全 sub-step 一般原則ではない。次 PB-X 系列で同 conflict 発生時に本 handoff §3.3 参照で 1 line 対処可能。

---

## §4 self-verify (= 9 観点、本 handoff 起案時点)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) PB-6 物理 diff | `git diff --stat indra/llrender/llglslshader.h indra/llrender/llglslshader.cpp` で 2 file changed + 26 insertions (= +13 / +13)、handoff doc 未起案時点 | ✅ |
| (2) PB-6 header 追加位置 | `llglslshader.h:443-457` = `private:` section、`generatePerProgramSPIRV()` 直後、comment header 11 line + 宣言 1 line | ✅ |
| (3) PB-6 cpp 追加位置 | `llglslshader.cpp:1952-1964` = `mapUniforms()` 直後 (= `LL_DEBUGS("ShaderUniform") << ... return res; }` の closing brace 直後) + `link()` 直前、comment header 10 line + 空 stub 本体 3 line (signature + `{` + `}`) | ✅ |
| (4) GATE-B 順守 | `#ifdef LL_VULKAN_GLSL` 不使用、stub 空 body ゆえ runtime 走らず、gate 議論自体に到達しない | ✅ |
| (5) mUseUBO=false default 維持 | `mUseUBO` initial value = `false` (= llglslshader.h:429)、本 commit で書き換えなし、stub 呼出元 (= PB-4/PB-5 未着手) ゼロゆえ stub 到達 0 回 | ✅ |
| (6) llrender 単体 build PASS | `make -C build-linux-x86_64 -j$(nproc) llrender` = `[33%] Built target codegen_ubo` → `Building llglslshader.cpp.o` → `Linking libllrender.a` → `[100%] Built target llrender` | ✅ |
| (7) full viewer build PASS | `make -C build-linux-x86_64 -j$(nproc)` = `[100%] Built target llpackage` + tar.xz package `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz` 生成 (= 初回 stale dir 衝突 → §3.3 cleanup → 2 回目で完遂) | ✅ |
| (8) spec 06a §5.2 / §5.3 literal 準拠 | `forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size)` signature 完全一致 (= spec literal `forwardToUboUpload(loc, &x, sizeof(GLfloat));` から逆引き、引数 3 種 / 型整合) | ✅ |
| (9) git working tree | `git status --short` で `M indra/llrender/llglslshader.cpp` + `M indra/llrender/llglslshader.h` のみ + 本 handoff doc 新規、scripts/ 改変なし + handoff doc 以外 doc 改変なし + Co-Authored-By: Claude 行不在 | ✅ |

---

## §5 引き継ぎ済 memory (= 次 session で active、PB-3 complete handoff から継承、追加なし)

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step (PB-4 / PB-5 / PB-7 / PB-N) で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守
- `feedback_self_verify_before_handoff` — PB-N 検証時に AYA 起動目視前に Claude 「3 経路非到達 verify」を Phase 1.B 後 state で再走
- `feedback_no_claude_coauthor` — 本 handoff doc 含め全 commit 共著行不在
- `feedback_one_step_at_a_time` — PB-4.1 → PB-4.2 → … → PB-N strict 線形、各 PB-X 単独 commit
- `feedback_ubo_migration_one_at_a_time` — PB-4 内部 17 method / PB-5 内部 13 method 一括禁止、1 method 1 sub-step
- `feedback_doubt_self_first` — PB-4.X 中に Hash() / cadence_tag 等の literal 不整合検知時、spec literal を盲信せず実装 grep で root cause 特定
- `feedback_admit_unknown` — Vulkan path 動作確認等の判断は推測で進めず source-of-truth (= 06a chapter) literal 再確認
- `feedback_proactive_handoff` — Phase 1.B 全終了時に Phase 1.C entry handoff doc 起案
- `feedback_no_auto_commit` — 本 handoff doc + PB-6 commit + 各 PB-X commit は AYA 明示指示後 commit
- `feedback_remove_verification_logs` — PB-X 実装中の `LL_INFOS` hook は commit 前必ず除去、debug `llassert` は仕様内ゆえ残す
- `feedback_build_only_verified` — Phase 1.B Exit Criteria literal 充足 (= viewer launch + 経路非到達 verify) 後 commit
- `feedback_tests_dir_never_commit` — `scripts/ubo_codegen/tests/` は commit 不可
- `feedback_build` — Phase 1.B PB-N full build フローは Claude 全権実行
- `project_build_procedure` — PB-N で参照、§3.3 stale dir cleanup も本 flow の `rm` step に該当
- `project_ayastorm_r41_vulkan_migration` — Phase 1.B PB-6 完了 = 次 PB-4.1 着手状態
- `project_ayastorm_r41_design_principles` — 原則 1「upstream 取込やすさ維持」 = GATE-B 採用根拠 + 原則 2「core プロセス分散実現」 = `forwardToUboUpload()` 中継層採用根拠
- `project_ayastorm_three_platforms` — Phase 1.B Exit Criteria literal 充足は Linux first-class baseline で十分
- `feedback_use_agents_proactively` — PB-4 着手中、複数 method 跨ぎ確認 / spec 横断 trace 等で Agent 投入候補
- `feedback_explanation_lead_with_conclusion` — AYA 判断項提示時に結論先

---

## §6 次 session 着手 1 line

**「前 session で Phase 1.B PB-6 `forwardToUboUpload()` shell 実装 完了 (= `indra/llrender/llglslshader.h` + `indra/llrender/llglslshader.cpp` modified、+26 line = (a) header `generatePerProgramSPIRV()` 直後 private section に `void forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size);` 宣言 + 11 line comment header + (b) cpp `mapUniforms()` 直後 = `link()` 直前 (`L1952-1964`) に空 stub `{}` 本体 + 10 line comment header、GATE-B 整合 (= stub 空ゆえ gate 議論不到達) + MUSEUBO-A 整合 (= 呼出元未配線、stub 到達 0 回)、llrender 単体 build PASS + full viewer build PASS = `[100%] Built target llpackage` + tar.xz package 生成、※初回 stale dir 衝突 → §3.3 rm 1 cleanup → 完遂)、PB-6 完了 handoff doc 起案 + AYA 明示指示後 batch commit。本 session = **AYA 判断不要 自走** = **PB-4.1 着手** = `uniform1i(U32 index, GLint x)` (`llglslshader.cpp:2227-2245`) の 1 method 単独に Vulkan path 分岐挿入 = `mProgramObject` check 内 `glUniform1i(mUniform[index], x);` 直前に `if (mUseUBO) { llassert(index < mUniformUBOLoc.size()); const ubo::UniformLocation& loc = mUniformUBOLoc[index]; if (loc.cadence_tag == 0xFFFFFFFFu) return; if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return; forwardToUboUpload(loc, &x, sizeof(GLint)); return; }` 分岐追加。06a §5.2 / §5.3 / §5.6 literal 準拠 + GATE-B 整合 (= `#ifdef LL_VULKAN_GLSL` 不使用) + MUSEUBO-A 維持 (= `mUseUBO=false` default で本 block 不到達)。llrender 単体 build PASS → full viewer build PASS → 独立 commit (= AYA 明示指示後) → 以降 PB-4.2〜.17 (= 16 method、1 method 1 sub-step) → PB-5.1〜.13 (= 13 method LLStaticHashedString、1 method 1 sub-step) → PB-7 (= 整合 check 仕込み) → PB-N (= MUSEUBO-A `mUseUBO=false` default で full build + viewer launch + bind 不変 verify) と strict 線形進行。」**
