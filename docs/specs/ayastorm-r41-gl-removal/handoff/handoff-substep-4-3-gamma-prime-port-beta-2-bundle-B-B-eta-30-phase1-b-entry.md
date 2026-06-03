# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B entry

**作成日**: 2026-06-04
**前 session commit**: `a6ba39b4ae` (= Phase 1.A 全終了 marker)
**本 session 物理出力** (= 全て **未 commit**、AYA さん明示指示後 batch commit):
- 本 handoff doc 新規 (= Phase 1.B entry = host C++ redirect 層着手の起点)

**次 session 着手**: **AYA 判断確定済** (= 2026-06-04 AYA 判断 4 件 + 副次 task (a) 全 default 採用確定、§3.6 参照) → **副次 (a) 着手 → PB-1 着手** の strict 線形で進行。AYA 確認不要、Claude 自走可。

---

## §0 state 一行 summary

η-30 **Phase 1.B entry state** (= Phase 1.A 全終了 = 直前 commit `a6ba39b4ae`):
- **Phase 1.A 全終了 marker 確定済** = Exit Criteria (i)+(ii)+(iii) 全充足 = codegen pipeline 配線 + 85 UBO blueprint 起案 + 実 indra/ build PASS + viewer launch + bind 不変 (= 経路非到達 3 観点 verify) + 画面描画 100% OpenGL 確証 + indra/cmake/AyaUboCodegen.cmake 4 line path 補正同梱
- **Phase 1.B scope** (= 09 §4.1 から literal 継承) = redirect 層実装 = **30 setter method (= integer index 17 + LLStaticHashedString 13、06a §5.5 cell)** 内部に Vulkan path 分岐 + name → offset 解決 dispatch + cache 構造 `mUniformUBOLoc` (= `LLGLSLShader` member 拡張) + shader link 時 pre-cache フロー (= `mapUniforms()` 拡張) + 整合 check (= debug build runtime assert)
- **Phase 1.B Exit Criteria** (= 09 §4.2 から literal 継承) = **30 setter Vulkan path 分岐の call site から見て transparent** = 既存 program 1 個動作 unchanged (= `mUseUBO=false` default で OpenGL path 経路維持 = 既存挙動不変)。1 setter call 1 path 決定論的、build flag で全 path 確認可能
- **本 chapter source-of-truth** = `06a-cache-structure-and-setter-redirect.md` (= 06a-prep-phase0-measurement.md は Phase 0 計測 spec で別 chapter、混同注意)
- **設計判断 AYA 確認待ち 4 件**: §3 で抽出 (= Q-PB-S1 LLStaticHashedString 経路 cache 構築 4 案 / Q-PB-FWD `forwardToUboUpload()` shape 3 案 / Q-PB-MUSEUBO Phase 1.B 中 mUseUBO=true 動作確認 要否 / Q-PB-ORDER PB-1〜PB-N strict 線形順序)
- **副次 task 候補 3 件** (= Phase 1.A 累計 finding): (a) `_PREFIX_TO_CADENCE` 拡張 (= main.py 3-5 line + unittest 2-3 件、PA-7.6 同流 trivial、Phase 1.B entry 直前 or 直後 独立 commit) / (b) spirv-cross system install (= `sudo apt install spirv-cross` で Linux baseline cross-check 有効化、AYA setup 余力時) / (c) CMakeCache.txt invalidate 手順 (= 都度発生時 `cmake -U <var>` 適用、永続記録不要)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-entry.md`) | 全文 | Phase 1.B scope + PB-1〜PB-N 構成 + AYA 判断 4 件 + 副次 task 3 件 + Exit Criteria + Phase 1.A 全終了 state 継承 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §3 (= cache 構造 `mUniformUBOLoc` + `UniformLocation` + `cadence_tag` enum + 解放規律) + §4 (= `mapUniforms()` Vulkan path 拡張 + R3 設計 hash 計算 shader link 時 1 回限り + LLStaticHashedString 補助 path + §4.3.1 S1-A/B/C/D 4 案 + §4.4 整合 check) + §5 (= 17 method 一覧 + path 分岐 code shape + 規律 + mUseUBO flag + integer index ↔ LLStaticHashedString 経路分岐 + sampler 49 個 OpenGL path 強制) | Phase 1.B 実装 chapter (source of truth)、30 setter 内部の Vulkan path 分岐 + perfect hash table dispatch 詳細 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §4 (= Phase 1.A/1.B/1.C sub-Phase 構成 + Exit Criteria + 1.C/2 境界明示 + shell ↔ 本実装 layout 互換性 + §4.3 紐付け持越項目) | Phase 1.B 全体像 + Phase 1.C 持越項目把握 (= sAssetUboPool / ring buffer / PSC は Phase 1.C 行き、Phase 1.B scope 外) |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-complete.md` | Phase 1.A 全終了 marker + 副次 finding 3 件 (cadence default fallback / CMakeCache persistence / spirv-cross PATH) + §3.4 Phase 1.B 主要 sub-task 候補 |
| `indra/llrender/llglslshader.h` | `LLGLSLShader` class 定義 (= 既存 `mUniform` vector + cache member 配置位置確認、PB-1 で `mUniformUBOLoc` + `mUniformUBOLocByHash` + `mUseUBO` 追加) |
| `indra/llrender/llglslshader.cpp:1554,1704,1842` | 既存 `mapUniform(GLint)` (= line 1554) + `mapUniforms()` (= line 1704) + active uniform 列挙 loop (= line 1842-1849)、PB-2 で Vulkan path 拡張 |
| `indra/llrender/llglslshader.cpp:2141-2557` | 既存 17 method (= `uniform1i` 〜 `uniformMatrix4fv`、06a §5.1 line 番号一覧)、PB-4 で各 method 内 Vulkan path 分岐追加 |
| `indra/llrender/llglslshader.cpp` (LLStaticHashedString overload) | 各 method の `const LLStaticHashedString&` overload 版 13 method、PB-5 で同 pattern 適用 |
| `${CMAKE_BINARY_DIR}/codegen/ubo/ubo_*.inl` (= build 後生成) | `ubo_perfect_hash.inl` (= CHD lookup) + `ubo_metadata.inl` (= block → size/set/binding) + `ubo_host_loader.inl` = Phase 1.B redirect 層が runtime consume する artifact、`ubo::lookup_runtime(name)` API |
| `scripts/ubo_codegen/main.py:65-72` | `_PREFIX_TO_CADENCE` 表 (= 副次 task (a)、Phase 1.B entry 直前 or 直後 副次 commit) |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-update-site-and-dirty-flag.md` | `forwardToUboUpload()` 実装本体 (= Phase 1.C 譲り、Phase 1.B では shell のみ)、PB-6 stub 書く時に interface 確認用 |
| `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` | Q24-S1 (= LLStaticHashedString 経路 S1-A/B/C/D 案、本 handoff §3 Q-PB-S1 と同) |

---

## §2 Phase 1.B scope と PB-1〜PB-N 構成

### §2.1 Phase 1.B scope (= 09 §4.1 から literal 継承)

**redirect 層実装** = 30 setter method (= 06a §5.5 integer index 17 + LLStaticHashedString 13) 内部に Vulkan path 分岐 + name → offset 解決 dispatch + cache 構造 `mUniformUBOLoc`。

該当 chapter = **06a 全章** (= `06a-cache-structure-and-setter-redirect.md`、source of truth)。

### §2.2 Phase 1.B Exit Criteria (= 09 §4.2 から literal 継承)

**30 setter Vulkan path 分岐の call site から見て transparent** = 既存 program 1 個の動作 unchanged。

補足:
- 30 setter 全てで Vulkan path 分岐 working (= compile 通過 + code path 到達可能)、OpenGL path 既存挙動 unchanged
- 1 setter call 1 path 決定論的、build flag (= `#ifdef LL_VULKAN_GLSL` + `mUseUBO` runtime flag) で全 path 確認可能
- 09 §4.2 注「Phase 1 完了時点では既存 program 動作 unchanged = Vulkan path 分岐 ON でも OpenGL path 経路を選ぶ default 動作」literal 充足 (= `mUseUBO=false` default で既存挙動 100% 維持、Vulkan path 動作確認は Phase 1.C で test UBO 1 個 shell + descriptor bind 通電と同時に実施が design phase 整合)

### §2.3 PB-1〜PB-N strict 線形構成 (= 06a §3-§5 から逆算、AYA 判断 Q-PB-ORDER で順序確定)

**default 提案順序** (= chapter 06a §3 → §4 → §5 → §4.4 の依存順):

| PB-X | 内容 | 該当 chapter | 物理改変 file | 出力契約 |
|---|---|---|---|---|
| **PB-1** | `mUniformUBOLoc` cache 構造実装 = `LLGLSLShader` class header 拡張 = `std::vector<ubo::UniformLocation> mUniformUBOLoc` + `std::unordered_map<U64, ubo::UniformLocation> mUniformUBOLocByHash` + `bool mUseUBO = false` member 追加 (= 全 `#ifdef LL_VULKAN_GLSL` gate) | 06a §3.2 / §3.3 / §3.4 | `indra/llrender/llglslshader.h` (+ 必要なら `#include "codegen/ubo/ubo_index.inl"` 等 forward 宣言) | header build PASS (= 既存 .cpp で `mUniformUBOLoc` symbol 未参照ゆえ link 不変) |
| **PB-2** | `mapUniforms()` Vulkan path 拡張 = integer index 経路 cache 構築 = `if (mUseUBO) { mUniformUBOLoc.resize(reserved.size()); for (i: reserved) { lookup_runtime(name) → mUniformUBOLoc[i] }}` | 06a §4.1 / §4.2 | `indra/llrender/llglslshader.cpp:1704-` (= `mapUniforms()` 既存実装の末尾に `#ifdef LL_VULKAN_GLSL` block 追加) | build PASS + `mUseUBO=false` default ゆえ Vulkan path 走らず既存挙動不変 |
| **PB-3** | LLStaticHashedString 経路 cache 構築 = `mUniformUBOLocByHash` 構築 (= **Q-PB-S1 確定後**、S1-A/B/C/D いずれかの方式で実装) | 06a §4.3 / §4.3.1 | 同上 `mapUniforms()` 末尾 + S1 案次第で `llstatichashedstring.h/.cpp` 拡張 (= S1-A の場合) or chapter 05 集約表静的配列 (= S1-C の場合) | build PASS + 既存挙動不変 |
| **PB-4** | 17 method (integer index 経路) Vulkan path 分岐追加 = `uniform1i` / `uniform1f` / `fastUniform1f` / `uniform2f` / `uniform3f` / `uniform4f` / `uniform1iv` / `uniform4iv` / `uniform1fv` / `uniform2fv` / `uniform3fv` / `uniform4fv` / `uniform4uiv` / `uniformMatrix2fv` / `uniformMatrix3fv` / `uniformMatrix3x4fv` / `uniformMatrix4fv` の各 method 末尾 `glUniform*` 直前に `#ifdef LL_VULKAN_GLSL if (mUseUBO) { … forwardToUboUpload(loc, data, size); return; }` 分岐挿入。1 method 1 PB-4.X sub-step で逐次実装 (= memory `feedback_ubo_migration_one_at_a_time` 準拠、17 setter 一括禁止) | 06a §5.2 / §5.3 / §5.6 | `indra/llrender/llglslshader.cpp:2141-2557` 17 箇所 | 17 method 全部で compile PASS + `mUseUBO=false` 経路 unchanged |
| **PB-5** | 13 method (LLStaticHashedString 経路) Vulkan path 分岐追加 = 各 method の `const LLStaticHashedString&` overload 版に `mUniformUBOLocByHash.find(hash)` lookup → `forwardToUboUpload()` 分岐追加。1 method 1 PB-5.X sub-step で逐次実装 | 06a §5.5 | `indra/llrender/llglslshader.cpp` 該当 LLStaticHashedString overload 13 箇所 | 13 method 全部で compile PASS + `mUseUBO=false` 経路 unchanged |
| **PB-6** | `forwardToUboUpload()` shell 実装 = `void LLGLSLShader::forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size)` shell 関数定義 (= **Q-PB-FWD 確定後**、完全 stub / LL_INFOS log のみ / Phase 1.C 前倒し の 3 案いずれかで実装) | 06a §5.2 (= 「(= 06b で実装)」と明記、Phase 1.B は interface 呼出位置のみ確定) + 06b 譲り | `indra/llrender/llglslshader.cpp` 末尾 (= 06b 譲りでも shell declaration は Phase 1.B 必要) + `llglslshader.h` method 宣言追加 | shell 関数 link PASS + PB-4/PB-5 の path 内 call で symbol 解決 |
| **PB-7** | 整合 check 仕込み = `mapUniforms()` 末尾に debug build 用 `llassert(mUniformUBOLoc.size() == mUniform.size())` + `llassert(各 mUniformUBOLoc[i].cadence_tag != CADENCE_INVALID で対応する mUniform[i] != -1)` + cadence_tag == CADENCE_SAMPLER の uniform は OpenGL path 強制 check | 06a §4.4 | 同上 `mapUniforms()` 末尾 (= PB-2/PB-3 拡張部の直後) | debug build PASS + assert 動作確認 (= release build では `#ifdef LL_RELEASE_FOR_DOWNLOAD` 等で除去) |
| **PB-N (= PB-8)** | Phase 1.B Exit Criteria 検証 = 実 indra/ full build PASS + install + cache clear + viewer launch + bind 不変動作確認 (= Phase 1.A complete §2.3 の経路非到達 3 観点 verify を Phase 1.B 後 state で再走) + log で Phase 1.B 起因 fail 0 件確認 + AYA さん起動目視「変化していないと思う」確認 + Phase 1.B 全終了 handoff doc 起案 | 09 §4.2 | (検証 phase = 物理 code 改変なし) | viewer 起動 + login + region entry PASS + log 内 `aya_ubo` / `mUniformUBOLoc` keyword 0 件 fail + AYA 確認後 commit |

**注**: PB-1〜PB-7 は 06a chapter §3-§5 の依存順で並列性なし strict 線形、PB-4/PB-5 内部は 1 method 1 sub-step で逐次 (= memory `feedback_ubo_migration_one_at_a_time` 準拠、17+13=30 method 一括 build 禁止)。各 PB-X 完了時に build PASS 確認 (= compile 通過の確認、`mUseUBO=false` default で既存挙動 unchanged) を入れる。

### §2.4 引き継ぐべき protocol (= Phase 1.B 中の自己統治)

| protocol | 内容 |
|---|---|
| (PB-P-1) 各 method 内 path 分岐 pattern 1 対 1 展開 | 06a §5.3 規律表通り、`mProgramObject` check → `mValue` cache check 維持 → `#ifdef LL_VULKAN_GLSL` compile-time gate → `if (mUseUBO)` runtime gate → `mUniformUBOLoc[index]` 直引き + `llassert` → `cadence_tag` skip 判定 → `forwardToUboUpload()` → OpenGL path 維持。pattern 一旦確定後 16 method 機械的 copy + 引数型差異のみ修正 (= PB-4 内部 sub-step 並走可能性、ただし 1 commit 1 method 原則維持) |
| (PB-P-2) build PASS 確認は各 method 追加後 | 17+13=30 method 一括追加禁止、各 PB-4.X / PB-5.X sub-step で `indra/llrender/` 部分 build 確認 (= autobuild の `--target llrender` 等で時間短縮可、ただし最終検証 PB-N は full build 必須) |
| (PB-P-3) `feedback_self_verify_before_handoff` | PB-N 検証時に AYA 起動目視前に Claude 側で「3 経路非到達 verify」を Phase 1.B 状態で再走 (= Phase 1.A complete §2.3 と同形式)、AYA に確認お願いする前に self-verify PASS |
| (PB-P-4) `feedback_remove_verification_logs` | PB-X 実装中の検証用 `LL_INFOS` hook は commit 前に必ず除去、debug `llassert` は仕様内ゆえ残す |

### §2.5 Phase 1.A 累計 protocol (= 前 handoff §2.6 から継承、本 Phase は blueprint 拡張ではない host C++ 拡張ゆえ未発動 default)

| protocol | 本 Phase 発動可能性 |
|---|---|
| P-1 divergence 検出 (= grep 列挙 + Agent 並列 + 直接 Read 二重 verify) | 未発動 default、ただし PB-3 LLStaticHashedString 経路で 67 個 uniform 名集計時に S1-C 案採用なら再発動可能性あり |
| P-2 binding 一意性 metadata 確認 | 未発動 (= Phase 1.B は blueprint 拡張なし、metadata は Phase 1.A 結果を consume) |
| P-3 cadence_tag 推定 (= `_PREFIX_TO_CADENCE` 確認) | 副次 task (a) 着手時のみ発動 (= main.py 3-5 line + unittest 2-3 件追加) |
| P-4 hash collision check | 未発動 (= 同上) |
| P-5 macro literal 置換 | 未発動 (= 同上) |

---

## §3 AYA 判断項抽出 (= 4 件、Phase 1.B PB-1 着手前確認)

### §3.1 Q-PB-S1: LLStaticHashedString 経路 cache 構築方式 4 案

**設計問**: 06a §4.3 で記載の `LLStaticHashedString::getGlobalRegistry()` は実装不在 (= 06a-prep §2.2.2 (S1-存在) 解消マーク済) ゆえ literal code shape のままでは compile fail。06a §4.3.1 で 4 案提示、いずれかを Phase 1.B 着手前に確定要。

**4 案** (= 06a §4.3.1 から literal 継承):

| 案 | 内容 | 利点 | 欠点 |
|---|---|---|---|
| S1-A | `LLStaticHashedString::forEachInstance(callback)` helper 新設 | 1 helper 追加で 06a §4.3 code shape の構造維持、call site 1 箇所 | LLStaticHashedString class への侵襲、upstream divergence 1 件 (= r41 原則 1「upstream 取込やすさ維持」軽微違反) |
| S1-B | shader link 時 `glGetActiveUniform` 列挙結果から `mReservedUniforms` 未収の uniform を拾い、`LLStaticHashedString(name)` で hash 計算 → `mUniformUBOLocByHash` 登録 | 既存 API のみで実装可、LLStaticHashedString 側無改修 | shader link 時 GL call 増 (= Vulkan path 専用 cache、OpenGL build 影響なし) |
| **S1-C (= 06a §4.3.1 default 候補)** | LLStaticHashedString 経由 setter 67 個の名前を chapter 05 集約表確定時に build-time list 化 (= 静的配列 `g_static_hashed_uniform_names[]`)、shader link 時はその配列を iterate | runtime registry iterate 不要、build-time decidable、LLStaticHashedString class 無侵襲 | 67 個 list の保守責任が chapter 05 集約表に追加、追加忘れで silent skip |
| S1-D | 補助 path (= `mUniformUBOLocByHash`) を **廃止**、67 個全て chapter 05 集約表で `mReservedUniforms` 化 (= integer index 経路に統合) | path 分岐削減 (30 entry point → 17)、cache 構造単純化 | 67 個全て `mReservedUniforms` 増要、chapter 05 集約表 67 行追加、各 program で全 67 個が active uniform 化される負担 |

**Claude 推奨** (= 06a §4.3.1 default + r41 原則整合): **S1-C** (= build-time list 化、LLStaticHashedString class 無侵襲、原則 1 維持)。

**AYA 判断仰ぎ**: S1-A / S1-B / **S1-C** (default) / S1-D いずれを採用するか。

### §3.2 Q-PB-FWD: `forwardToUboUpload()` Phase 1.B 時点の shape 3 案

**設計問**: 06a §5.2 code shape は `forwardToUboUpload(loc, &x, sizeof(GLfloat))` を最後の出口に置くが、06a §5.2 末尾「(= 06b で実装)」と明記 = 実 buffer write は 06b 譲り。Phase 1.B では「interface 呼出位置を確定」までを scope に含むが、関数本体をどこまで埋めるかは判断項。

**3 案**:

| 案 | 内容 | 利点 | 欠点 |
|---|---|---|---|
| **FWD-1 (= 完全 stub、Claude 推奨)** | `void LLGLSLShader::forwardToUboUpload(…) { /* 06b 実装、Phase 1.B では shell */ }` 空関数 | Phase 1.B Exit Criteria literal 充足 (= compile 通過 + path 到達可能 + 既存 program 動作 unchanged = `mUseUBO=false` 維持で path 到達せず + Vulkan path 動作確認は Phase 1.C 譲り = 09 §4.2 注の design phase 整合)。実装最少 | `mUseUBO=true` で path 動作確認しても何も起きない (= 完全無動作)、PB-N の Vulkan path 分岐 working 確認は「compile 通過 + path 到達可能」までで `mUseUBO=true` 実走チェックは Phase 1.C |
| FWD-2 (= `LL_INFOS` log のみ) | `void LLGLSLShader::forwardToUboUpload(…) { LL_INFOS("UBO") << "forwardToUboUpload called name=" << ... << LL_ENDL; }` | `mUseUBO=true` で path 動作確認時に log で経路通電確認可 (= debug 用) | 検証用 log で commit 前除去必要 (= `feedback_remove_verification_logs` 適用)、PB-N で log 除去 + 再 build + 再 viewer launch のサイクルが追加 |
| FWD-3 (= Phase 1.C 06b 実装前倒し) | `forwardToUboUpload()` 内で実 UBO buffer write (= `memcpy(UBO_buffer + loc.offset, data, size)`) + dirty flag mark | Phase 1.B で実 Vulkan path 通電確認可 | 06b chapter scope の前倒し = 09 §4.1 Phase 1.B/1.C 境界違反、chapter 設計上の sub-Phase 分離 (= Phase 1.B redirect 層 / Phase 1.C update site + dirty flag) を Claude が独断で merge する scope overshoot |

**Claude 推奨**: **FWD-1** (= 完全 stub、Phase 1.B literal scope 厳守、Phase 1.C 譲り 06b 実装を Phase 1.B に前倒ししない `feedback_no_scope_shrink` の逆「scope_overshoot 禁止」適用)。

**AYA 判断仰ぎ**: **FWD-1** (default) / FWD-2 / FWD-3 いずれを採用するか。

### §3.3 Q-PB-MUSEUBO: Phase 1.B 中 `mUseUBO=true` 実走動作確認の要否

**設計問**: 09 §4.1 Phase 1.B 行 Exit 判定 literal = 「30 setter 全てで Vulkan path 分岐 working、OpenGL path 既存挙動 unchanged」。「working」を (a) compile 通過 + code path 到達可能 (= dead code として存在) で literal 充足とするか、(b) `mUseUBO=true` runtime flip で実走経路まで確認するか、解釈分岐あり。

**2 案**:

| 案 | 内容 | 利点 | 欠点 |
|---|---|---|---|
| **MUSEUBO-A (= compile 通過のみ、Claude 推奨)** | Phase 1.B 中 `mUseUBO=false` default 維持、Vulkan path 分岐 code は compile 通過 + path 到達可能 (= dead code) で Exit Criteria literal 充足とする。`mUseUBO=true` 実走確認は Phase 1.C で test UBO 1 個 shell + descriptor bind 通電と同時 | 09 §4.2 注「Phase 1 完了時点では既存 program 動作 unchanged = Vulkan path 分岐 ON でも OpenGL path 経路を選ぶ default 動作」literal 充足、`forwardToUboUpload()` shell (= Q-PB-FWD FWD-1) と整合、Phase 1.B/1.C 境界明確 | `mUseUBO=true` runtime flip での実走確認は Phase 1.C 譲り、Phase 1.B 中は path 到達可能性 (= code review + grep で確認) のみ |
| MUSEUBO-B (= `mUseUBO=true` 実走) | Phase 1.B 中に debug settings cvar `AYAUboRedirectEnabled` 等を仮設置、AYA さん起動目視で `mUseUBO=true` flip + path 到達確認 (= LL_INFOS log で `forwardToUboUpload called` 出力確認等) | Phase 1.B 中に Vulkan path 分岐の動作 literal 確認可能 | `forwardToUboUpload()` が完全 stub (= FWD-1) だと flip しても何も起きない = LL_INFOS log path (= FWD-2) 採用必要 + 検証用 log は commit 前除去 = `feedback_remove_verification_logs` 適用 + 検証 cvar も Phase 1.B Exit 時除去 = 06c chapter mUseUBO 決定方法 (= 自動判定 vs cvar) の前倒し議論を Phase 1.B に持ち込む scope overshoot 可能性 |

**Claude 推奨**: **MUSEUBO-A** (= compile 通過 + path 到達可能性で Exit Criteria literal 充足、`mUseUBO=true` 実走確認は Phase 1.C 譲り)。理由 = 09 §4.2 注の design phase 整合 + `forwardToUboUpload()` shell (= FWD-1) と整合 + chapter 06a §5.4 「決定方法 (= 自動判定 vs cvar) は 06c で詰める」literal 通り 06c 譲り維持。

**AYA 判断仰ぎ**: **MUSEUBO-A** (default) / MUSEUBO-B いずれを採用するか。

### §3.4 Q-PB-ORDER: PB-1〜PB-N strict 線形順序確定

**設計問**: §2.3 で提示した default 順序 (= PB-1 cache 構造 → PB-2 mapUniforms 整数 index 経路 → PB-3 mapUniforms LLStaticHashedString 経路 → PB-4 17 method int index 分岐 → PB-5 13 method hashed string 分岐 → PB-6 forwardToUboUpload shell → PB-7 整合 check → PB-N Exit 検証) が 06a chapter §3 → §4 → §5 → §4.4 の依存順だが、AYA 確認要。

**注**: §2.3 default 順序は 06a chapter 依存順 + memory `feedback_ubo_migration_one_at_a_time` (= 17+13=30 method 一括禁止) 整合。PB-1/PB-2/PB-3 を merge して 1 commit (= `mapUniforms()` 拡張同時に cache 構造追加) 等の最適化は scope minimal 維持の観点で却下、各 PB-X 単独 commit で 1 method 1 PB-4.X / PB-5.X sub-step も含めて strict 線形維持。

**Claude 推奨**: **§2.3 default 順序通り** (= PB-1 → PB-2 → PB-3 → PB-4 (内部 PB-4.1〜PB-4.17 sub-step) → PB-5 (内部 PB-5.1〜PB-5.13 sub-step) → PB-6 → PB-7 → PB-N)。

**AYA 判断仰ぎ**: 順序変更 / sub-step merge 等の修正提案あるか。default 採用なら確定。

### §3.5 副次 task 候補 3 件 (= Phase 1.A 累計 finding、Phase 1.B entry 直前 or 直後 判定)

| # | 内容 | weight | 着手判定 (default 提案) |
|---|---|---|---|
| **(a) `_PREFIX_TO_CADENCE` 拡張** | `scripts/ubo_codegen/main.py:65-72` に `PerDrawUBO_` → 2 = Draw / `PerProgramUBO_` → 1 = Program / `UBO_Legacy` suffix-match → 1 = Program の 3 prefix 追加 + unittest 2-3 件追加 = 累計 85 UBO の cadence_tag default fallback 解消 | main.py 3-5 line + unittest 2-3 件 = PA-7.6 同流 trivial | **Phase 1.B entry 直前 独立 commit** (= AYA 判断不要 trivial fix、PB-1 着手前に処理して再 build → cadence 正値 metadata 取得 → PB-1 着手の方が cache 構造内 `cadence_tag` 検証に役立つ) |
| (b) spirv-cross system install | `sudo apt install spirv-cross` で Linux baseline cross-check 有効化、Phase 1.A の `AYA_CODEGEN_SKIP_SPIRV_CHECK=1` fallback 解除 | system install のみ (= AYA さん setup task) | AYA setup 余力時に実施、Phase 1.B 着手 blocker でない (= Linux build cross-check なしでも Phase 1.A Exit Criteria literal 充足済) |
| (c) CMakeCache.txt invalidate 手順 | Phase 1.B/1.C/2 で cmake module 更新時の共通注意、本 handoff §0 + 前 handoff §2.5 (2) literal 記録済 | 都度発生時 `cmake -U <var>` 適用 | 永続記録不要、Phase 1.B PB-X 中に cmake module 改修が発生した時のみ参照 |

**Claude 推奨**: (a) を **Phase 1.B entry 直前 独立 commit** で処理 (= 副次 task として Phase 1.B PB-1 着手前に挟む、weight = main.py 3-5 line + unittest 2-3 件、PA-7.6 同流 trivial、AYA 判断不要)。(b)(c) は適宜対応。

**AYA 判断仰ぎ**: (a) を Phase 1.B entry 直前に処理するか、PB-1〜PB-N 完了後の独立 commit にするか、または skip するか。

---

## §3.6 AYA 判断確定 (= 2026-06-04 session 終端、本 handoff 起案後 AYA さん指示)

| # | 判断項 | 確定案 | 採用理由 |
|---|---|---|---|
| Q-PB-S1 | LLStaticHashedString 経路 cache 構築 | **S1-C** (= build-time list 化 `g_static_hashed_uniform_names[]`) | r41 原則 1「upstream 取込やすさ維持」整合 + LLStaticHashedString class 無侵襲 + 06a §4.3.1 default 候補 |
| Q-PB-FWD | `forwardToUboUpload()` Phase 1.B 時点 shape | **FWD-1** (= 完全 stub、空関数) | 06a §5.2「(= 06b で実装)」literal + 09 §4.1 Phase 1.B/1.C 境界 (= 実 buffer write は 1.C 譲り) + scope overshoot 禁止 |
| Q-PB-MUSEUBO | Phase 1.B 中 `mUseUBO=true` 実走確認の要否 | **MUSEUBO-A** (= `mUseUBO=false` default 維持、compile 通過 + path 到達可能性で Exit Criteria literal 充足) | 09 §4.2 注「Phase 1 完了時点 mUseUBO=false default で OpenGL path 経路選択」literal + FWD-1 整合 + 06a §5.4 「決定方法は 06c 譲り」literal |
| Q-PB-ORDER | PB-1〜PB-N strict 線形順序 | **§2.3 default 順序** (= PB-1 cache 構造 → PB-2 mapUniforms 整数 index → PB-3 mapUniforms hashed string → PB-4 17 method 分岐 (内部 PB-4.1〜.17) → PB-5 13 method 分岐 (内部 PB-5.1〜.13) → PB-6 forwardToUboUpload shell → PB-7 整合 check → PB-N Exit 検証) | 06a chapter §3 → §4 → §5 → §4.4 依存順 + `feedback_ubo_migration_one_at_a_time` 30 method 一括禁止 |
| 副次 (a) | `_PREFIX_TO_CADENCE` 拡張 | **Phase 1.B entry 直前 独立 commit** (= 副次 commit → PB-1 着手) | PB-1 着手前に cadence 正値 metadata 取得 → PB-1 cache 構造内 `cadence_tag` 検証に役立つ + weight = PA-7.6 同流 trivial |
| 副次 (b) | spirv-cross system install | **AYA setup 余力時** (= Phase 1.B blocker でない) | Linux baseline cross-check 有効化 = 二重保証強化、Phase 1.A Exit Criteria literal 充足は cross-check なしでも済む |
| 副次 (c) | CMakeCache.txt invalidate 手順 | **都度発生時 `cmake -U <var>` 適用** (= 永続記録不要) | Phase 1.B/1.C/2 で cmake module 更新時の共通注意、本 handoff §0 + 前 handoff §2.5 (2) literal 記録済 |

**AYA 指示 literal**: 「コンテキスト余裕なければ handoff 次セッションとして 全 default で」 → handoff 次 session 化 (= 本 commit) + 全 default 確定。

**次 session 第 1 着手 = 副次 (a) `_PREFIX_TO_CADENCE` 拡張** (= AYA 判断不要、本 confirmed default に従う Claude 自走)。
完了後 → smoke 再走で cadence 正値確認 → 独立 commit → **PB-1 cache 構造実装** (= `LLGLSLShader` header 拡張) 着手。

---

## §4 self-verify (= 9 観点、本 handoff 起案時点)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) Phase 1.A 全終了 marker 確認 | `git log --oneline -1` で `a6ba39b4ae r41: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A 全終了` 物理確認、Exit Criteria (i)(ii)(iii) 全充足 + 副次 path fix 4 line 反映完了 | ✅ |
| (2) Phase 1.B scope 明確化 | 09 §4.1 Phase 1.B 行 literal 継承 = 「30 setter Vulkan path 分岐 + name → offset 解決 dispatch + cache 構造 mUniformUBOLoc」 + 該当 chapter = 06a §3/§4/§5 確認 | ✅ |
| (3) Phase 1.B Exit Criteria literal 継承 | 09 §4.2 Phase 1.B 行 literal = 「30 setter Vulkan path 分岐の call site から見て transparent = 既存 program 1 個動作 unchanged」 + 注「Phase 1 完了時点では既存 program 動作 unchanged = mUseUBO=false default で OpenGL path 経路維持」literal 反映 | ✅ |
| (4) PB-1〜PB-N strict 線形構成 | 06a §3 → §4 → §5 → §4.4 依存順 + `feedback_ubo_migration_one_at_a_time` 30 method 一括禁止 + PB-4 内部 PB-4.1〜PB-4.17 / PB-5 内部 PB-5.1〜PB-5.13 sub-step 分割 | ✅ |
| (5) AYA 判断項 4 件抽出完了 | Q-PB-S1 (= 06a §4.3.1 S1-A/B/C/D + Claude 推奨 S1-C) + Q-PB-FWD (= 06a §5.2 末尾「06b 実装」literal + Claude 推奨 FWD-1) + Q-PB-MUSEUBO (= 09 §4.2 注「mUseUBO=false default」literal + Claude 推奨 MUSEUBO-A) + Q-PB-ORDER (= §2.3 default 順序 + Claude 推奨 default 採用) | ✅ |
| (6) 副次 task 3 件 (a)(b)(c) 判定明確化 | (a) `_PREFIX_TO_CADENCE` 拡張 = Phase 1.B entry 直前 独立 commit 推奨 / (b) spirv-cross install = AYA setup 余力時 / (c) CMakeCache invalidate = 都度対処 | ✅ |
| (7) source-of-truth file 名混同回避 | `06a-cache-structure-and-setter-redirect.md` (= Phase 1.B redirect 層 chapter) vs `06a-prep-phase0-measurement.md` (= Phase 0 計測 spec) 別 chapter ゆえ前者を本 handoff source-of-truth として明示、user prompt 「06a §3-§5」記述は前者を指すと解釈 (= 後者 §3-§5 は「同一 binding 複数 UBO 名 grep spec」「MaterialUBO vs MaterialUBO_Legacy 比較 spec」「解析 spec」で redirect 層と無関係) | ✅ |
| (8) Phase 1.C 持越項目把握 | 09 §4.3 持越項目 (= `sAssetUboPool` prealloc / ring buffer / PSC) は Phase 1.C 行き、Phase 1.B scope 外。`forwardToUboUpload()` 本実装も 06b 譲り = Phase 1.C 行き | ✅ |
| (9) git working tree 状態 | `git status` で `?? docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-entry.md` のみ新規、indra/ 改変なし + scripts/ 改変なし + Co-Authored-By: Claude 行不在 | ✅ |

---

## §5 引き継ぎ済 memory (= 次 session で active)

- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_design_phase_no_code_write` — 解禁済 (= Phase 1.A 完了、Phase 1.B 実装 phase 継続)
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守、subset 分岐実装で済まさない
- `feedback_self_verify_before_handoff` — PB-N 検証時に AYA 起動目視前に Claude 「3 経路非到達 verify」を Phase 1.B 後 state で再走
- `feedback_no_claude_coauthor` — 本 handoff doc 含め全 commit 共著行不在
- `feedback_one_step_at_a_time` — PB-1 → PB-2 → … → PB-N strict 線形、各 PB-X 単独 commit
- `feedback_ubo_migration_one_at_a_time` — PB-4 内部 17 method / PB-5 内部 13 method 一括禁止、1 method 1 sub-step
- `feedback_doubt_self_first` — PB-X 中の不可解な build error / link fail は AYA に投げる前に Claude が root cause 特定
- `feedback_admit_unknown` — Vulkan path 動作確認等の判断は推測で進めず source-of-truth (= 06a chapter) literal 再確認
- `feedback_proactive_handoff` — Phase 1.B 全終了時に Phase 1.C entry handoff doc 起案 (= 本 handoff と同形式)
- `feedback_no_auto_commit` — 本 handoff doc + 各 PB-X commit は AYA 明示指示「commit して」後 batch commit
- `feedback_remove_verification_logs` — PB-X 実装中の `LL_INFOS` hook は commit 前必ず除去、debug `llassert` は仕様内ゆえ残す
- `feedback_build_only_verified` — Phase 1.B Exit Criteria literal 充足 (= viewer launch + 経路非到達 verify) 後 commit
- `feedback_tests_dir_never_commit` — 副次 task (a) main.py unittest は `scripts/ubo_codegen/tests/` 配下、commit 不可
- `feedback_restore_debug_settings` — Phase 1.B 中 debug settings 一時変更があれば AYA に「戻す値表」提示
- `feedback_build` — Phase 1.B PB-N full build フローは Claude 全権実行 (= `project_build_procedure` 参照)
- `project_build_procedure` — PB-N で参照 (= configure → build → install → cache clear フル実行)
- `project_ayastorm_r41_vulkan_migration` — Phase 1.A 全終了 = 次 Phase 1.B entry 状態
- `project_ayastorm_r41_design_principles` — 原則 1「upstream 取込やすさ維持」が Q-PB-S1 S1-A 案 (= LLStaticHashedString class 侵襲) 退ける根拠 / 原則 2「Core プロセス分散実現容易な設計」は Phase 1.B では未発動 default (= Phase 1.C 以降で cadence 別 update site で活性化)
- `project_ayastorm_three_platforms` — Phase 1.B Exit Criteria literal 充足は Linux first-class baseline 1 platform で十分、3 OS 統一は別 phase
- `feedback_use_agents_proactively` — PB-X 内部の 3 file 以上確認 / 経路 trace は Agent 投入候補
- `feedback_explanation_lead_with_conclusion` — AYA 判断 4 件提示時に結論先 (= Claude 推奨案を明示) + 根拠表で提示

---

## §6 次 session 着手 1 line

**「前 session で Phase 1.A 全終了 (= Exit Criteria (i)(ii)(iii) 全充足 + 副次 path fix 4 line + commit 1 件 = `a6ba39b4ae`) + Phase 1.B entry handoff doc 起案完了 + AYA 判断 4 件 + 副次 (a) 全 default 確定済 (= §3.6、2026-06-04 AYA 指示「コンテキスト余裕なければ handoff 次セッションとして 全 default で」literal)。本 session = **AYA 判断不要 自走** = 副次 (a) `_PREFIX_TO_CADENCE` 拡張 (= main.py 3-5 line + unittest 2-3 件、PA-7.6 同流 trivial) → smoke 再走で cadence 正値確認 → 独立 commit → **PB-1 cache 構造実装** (= `LLGLSLShader` header 拡張 = `mUniformUBOLoc` + `mUniformUBOLocByHash` + `mUseUBO` member 追加、全 `#ifdef LL_VULKAN_GLSL` gate) → build PASS 確認 → 独立 commit → 以降 PB-2 → PB-3 (= S1-C 採用) → PB-4.1〜.17 → PB-5.1〜.13 → PB-6 (= FWD-1 完全 stub) → PB-7 → PB-N (= MUSEUBO-A `mUseUBO=false` default 維持) と strict 線形進行。」**
