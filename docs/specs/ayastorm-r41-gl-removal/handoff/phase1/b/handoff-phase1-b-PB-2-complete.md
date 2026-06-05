# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-2 完了

**作成日**: 2026-06-04
**前 session commit**:
- `7032a0b1a8` (= PB-1 complete handoff doc 起案)
- `0ba743463c` (= PB-1 LLGLSLShader cache 構造 3 member 追加)
- `7fe58b7428` (= 副次 (a) `_PREFIX_TO_CADENCE` 拡張)

**本 session 物理出力** (= 全て **未 commit**、AYA さん明示指示後 batch commit):
- `indra/llrender/llglslshader.cpp` modified (= +26 line、`mapUniforms()` 内 Vulkan path integer index 経路 cache 構築 block 追加)
- 本 handoff doc 新規 (= PB-2 complete = 30 setter redirect 層 第 2 sub-step 終端 marker)

**次 session 着手**: **AYA 判断不要 自走可** (= 2026-06-04 AYA 判断 4 件 (entry handoff §3.6) + GATE-B (PB-1 complete handoff §3.1) + 本 session 確定 設計判断 1 件 (= 挿入位置 = `unbind()` 直前) 全採用済) → **PB-3 着手** = LLStaticHashedString 経路 cache 構築 (= S1-C 採用、build-time list `g_static_hashed_uniform_names[]` 新規追加 + iterate で `mUniformUBOLocByHash` 構築)。**ただし** PB-3 着手前に **chapter 05 集約表 (= LLStaticHashedString 67 個の具体名 list) の確定状況** を pinpoint Read で確認必要 (= §3.2 参照)。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-2 complete state**:
- **PB-2 `mapUniforms()` Vulkan path integer index 経路拡張 完了** (= `indra/llrender/llglslshader.cpp` modified、未 commit) = 既存 GL UBO binding loop 直後 + `unbind()` 直前 に `if (mUseUBO) { mUniformUBOLoc.resize + per index lookup_runtime → mUniformUBOLoc[i] = *loc / miss 時 cadence_tag=0xFFFFFFFFu sentinel }` block 追加、+26 line。
- **GATE-B 整合** = `#ifdef LL_VULKAN_GLSL` 不使用、`if (mUseUBO)` runtime gate 単独。
- **mUseUBO=false default 維持** (= MUSEUBO-A) ゆえ本 block 走らず、既存 OpenGL 挙動 100% 維持。
- **本 session 設計判断 1 件** (= §3.1) = **挿入位置 = `unbind()` 直前** (= bind/unbind 対称構造内、既存 GL UBO binding loop 直後)。spec 06a §4.1 literal「`mapUniforms()` 末尾」を実体化するため最末尾 `LL_DEBUGS` 直前 でなく `unbind()` 直前 を選択 (= bind 状態に依存しない CPU-side cache 構築だが、対称構造内に置く方が読み手の context 切替負担が小さい)。
- **llrender 単体 build PASS** + **full viewer build PASS** = `[100%] Built target llpackage` + tar.xz package `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz` 生成。
- **PB-3 着手前確認項 1 件** (= §3.2) = chapter 05 集約表 (= LLStaticHashedString 67 個の具体名 list、= `05a-bare-uniform-mapping.md` 切出し doc) の確定状況。S1-C は build-time list 化前提 = list の入力が必要。
- **残 sub-step**: PB-3 → PB-4.1〜.17 → PB-5.1〜.13 → PB-6 → PB-7 → PB-N strict 線形。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-2-complete.md`) | 全文 | PB-2 完了 state + 挿入位置設計判断 + PB-3 着手前確認項 + 残 sub-step (PB-3〜PB-N) 順序 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` §4.3 + §4.3.1 | 全文 (= LLStaticHashedString 経路補助 cache + S1-A/B/C/D 代替案 + S1-C default 採用根拠) | PB-3 chapter source-of-truth = build-time list 化 (S1-C) の code shape |
| 3 | **PB-3 着手前確認**: chapter 05 集約表 (= `docs/specs/ayastorm-r41-gl-removal/design/05a-bare-uniform-mapping.md` 切出し doc) 存在確認 + LLStaticHashedString 67 個の具体名 list 確定状況 (= 存在しなければ PB-3 中で grep ベース抽出 fallback) | 該当箇所 + grep 補完 | PB-3 build-time list 入力源確定 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-1-complete.md` | PB-1 完了 state + GATE-B 確定根拠 + 残 sub-step 表 |
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-entry.md` | entry handoff §3.6 AYA 判断 4 件確定表 (= S1-C / FWD-1 / MUSEUBO-A / §2.3 default) |
| `indra/llrender/llglslshader.cpp:1864-1898` | 本 session 追加 PB-2 block + 既存 GL UBO binding loop + `unbind()` 周辺の最新 state |
| `indra/llrender/llglslshader.h:404-429` | PB-1 で追加した 3 member (`mUniformUBOLoc` / `mUniformUBOLocByHash` / `mUseUBO`) |
| `build-linux-x86_64/codegen/ubo/ubo_perfect_hash.inl:887-908` | `lookup_runtime(const char* name)` API + `fnv1a_32()` (= PB-3 でも同 API 利用) |
| `indra/llrender/llshadermgr.h:498` | `mReservedUniforms = std::vector<std::string>` (= integer index 経路の入力集合、67 件 LLStaticHashedString とは別集合) |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md:§5.5` | LLStaticHashedString 13 method 一覧 (= PB-5 着手時) |
| `indra/llrender/llglslshader.cpp:2617-2844` | LLStaticHashedString 経路 13 method の現状 (= PB-5 着手時、PB-3 は cache 構築のみで setter 改変なし) |

---

## §2 残 sub-step (PB-3〜PB-N) strict 線形構成 (= entry handoff §2.3 + PB-1 complete handoff から継承)

| PB-X | 内容 | 該当 chapter | 物理改変 file | 出力契約 | 状態 |
|---|---|---|---|---|---|
| ~~PB-1~~ | ~~`mUniformUBOLoc` cache 構造実装~~ | 06a §3 | `indra/llrender/llglslshader.h` | header + full build PASS | **✅ 完了** (commit `0ba743463c`) |
| ~~PB-2~~ | ~~`mapUniforms()` Vulkan path integer index 経路 cache 構築~~ | 06a §4.1 / §4.2 | `indra/llrender/llglslshader.cpp:1864-1898` | +26 line `if (mUseUBO) { resize + lookup_runtime loop }` block | **✅ 完了** (本 session、未 commit、本 handoff 起案後 batch commit) |
| **PB-3** | **LLStaticHashedString 経路 cache 構築** (= S1-C 採用確定) = (a) `g_static_hashed_uniform_names[]` 新規 build-time 静的配列追加 (= chapter 05 集約表確定の LLStaticHashedString 67 個リスト、入力源未確定なら PB-3 中で grep 抽出) + (b) `mapUniforms()` 末尾 PB-2 block 直後 (= `unbind()` 直前 か `unbind()` 直後、§3.3 設計判断対象) に iterate block 追加 = `for (auto* name: g_static_hashed_uniform_names) { lookup_runtime(name) → mUniformUBOLocByHash[hash(name)] = *loc }` | 06a §4.3 / §4.3.1 (S1-C) | (a) 配置場所判断要 = `llglslshader.cpp` anonymous namespace か新規 file (= chapter 05 集約表 owner と同居) + (b) `mapUniforms()` 末尾 | build PASS + 既存挙動不変 (= mUseUBO=false default) |
| **PB-4** | 17 method (integer index 経路) Vulkan path 分岐追加 = 各 method 末尾 `glUniform*` 直前に `if (mUseUBO) { auto& loc = mUniformUBOLoc[index]; if (loc.cadence_tag == 0xFFFFFFFFu) return; if (loc.cadence_tag == 5 /* sampler */) return; forwardToUboUpload(loc, data, size); return; }` 分岐挿入。1 method 1 PB-4.X sub-step (= `feedback_ubo_migration_one_at_a_time` 準拠、17 setter 一括禁止) | 06a §5.2 / §5.3 / §5.6 | `indra/llrender/llglslshader.cpp` 17 箇所 (= line 2141-2538) | 17 method 全 compile PASS + mUseUBO=false 経路 unchanged |
| **PB-5** | 13 method (LLStaticHashedString 経路) Vulkan path 分岐追加 = 各 method の `const LLStaticHashedString&` overload 版に `mUniformUBOLocByHash.find(hash)` lookup → `forwardToUboUpload()` 分岐追加。1 method 1 PB-5.X sub-step | 06a §5.5 | `indra/llrender/llglslshader.cpp` 該当 LLStaticHashedString overload 13 箇所 (= line 2617-2844) | 13 method 全 compile PASS + mUseUBO=false 経路 unchanged |
| **PB-6** | `forwardToUboUpload()` shell 実装 (= FWD-1 採用確定) = `void LLGLSLShader::forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size) {}` 空関数 | 06a §5.2 (= 「(= 06b で実装)」literal) | `indra/llrender/llglslshader.cpp` 末尾 + `llglslshader.h` method 宣言追加 | shell 関数 link PASS + PB-4/PB-5 path 内 call 解決 |
| **PB-7** | 整合 check 仕込み = `mapUniforms()` 末尾 PB-2/PB-3 拡張部直後に debug build 用 `llassert(mUniformUBOLoc.size() == mUniform.size())` + cadence_tag check | 06a §4.4 | `mapUniforms()` 末尾 | debug build PASS + assert 動作確認 |
| **PB-N (= PB-8)** | Phase 1.B Exit Criteria 検証 = full build + install + cache clear + viewer launch + bind 不変動作確認 + log で Phase 1.B 起因 fail 0 件確認 + AYA さん起動目視「変化していないと思う」確認 + Phase 1.B 全終了 handoff doc 起案 | 09 §4.2 | (検証 phase = 物理 code 改変なし) | viewer 起動 + login + region entry PASS + AYA 確認後 commit |

---

## §3 本 session で新規確定した設計判断 + PB-3 着手前確認項

### §3.1 挿入位置 = `unbind()` **直前** (= 既存 GL UBO binding loop 直後)

**設計問**: spec 06a §4.1 literal「`mapUniforms()` 末尾に `if (mUseUBO)` block 追加」の「末尾」が `unbind()` 前か後か未指定。`mapUniforms()` 末尾 3 箇所候補:
- (a) 最末尾直前 = `LL_DEBUGS("ShaderUniform")` 直前 (`llglslshader.cpp:1875`)
- (b) `unbind()` 直前 (`llglslshader.cpp:1873`)
- (c) 既存 GL UBO binding loop (= `for (U32 i = 0; i < NUM_UNIFORM_BLOCKS; ++i) ... glUniformBlockBinding`) 直後 (`llglslshader.cpp:1871`)

**3 案検討**:

| 案 | 内容 | 採否 |
|---|---|---|
| (a) `LL_DEBUGS` 直前 = `unbind()` 後 | bind 状態に依存しない CPU-side cache 構築なので unbind 後でも OK | 退け |
| **(b) `unbind()` 直前 = 既存 GL UBO binding loop 直後** | bind/unbind 対称構造内、既存 GL UBO binding loop と PB-2 cache 構築 block が連続配置、読み手の context 切替 1 回で済む | **採用** |
| (c) GL UBO binding loop 直後 = 結果的に (b) と同位置 | 表記の違いだけで物理位置同一 | (b) と統合 |

**採用根拠**:
- (i) bind/unbind 対称構造の内側に PB-2 block を入れる方が、既存 GL UBO binding loop と PB-2 (= Vulkan 用 UBO loc cache 構築) の **意図的並列性** が表現できる (= 「OpenGL 用 UBO binding は GL block、Vulkan 用 UBO loc cache は次の block」)
- (ii) `lookup_runtime` は GL state を一切触らない (= 純粋 CPU side hash table lookup) ため bind 状態に依存しない、(a)/(b)/(c) どれでも機能は同じ
- (iii) 将来 PB-3 で LLStaticHashedString 経路 cache 構築 block を追加する際も、同じ「`unbind()` 直前」の位置に連続配置できる = §2 PB-3 row で `mapUniforms()` 末尾 PB-2 block 直後 と表現済
- (iv) PB-7 整合 check (= debug `llassert`) も同位置に追加すれば、PB-2/PB-3/PB-7 が 1 block 内に集中 = 後 maintenance 時の find/edit 局所化

**memory 保存不要** = PB-3/PB-7 着手時に本 handoff §3.1 参照で足りる、Phase 1.B 全 sub-step の一般原則ではない (= 個別 sub-step の位置決定)。

### §3.2 PB-3 着手前確認項 1 件 = chapter 05 集約表確定状況

**設計問**: S1-C 採用確定 (= AYA entry handoff §3.6 で確定) の前提 = LLStaticHashedString 67 個の具体名 list が build-time に確定していること。chapter 06a §0.2 (H1a) 結果で **67 unique names** と総数のみ確定、具体名 list は別 file (= `05a-bare-uniform-mapping.md` 切出し doc 予定) に持ち越しと spec 06a §9 (R1) 持越項目に明記。

**PB-3 着手前確認 step**:
1. `docs/specs/ayastorm-r41-gl-removal/design/05a-bare-uniform-mapping.md` 存在確認 (= Glob 1 回)
2. 存在する場合 = §LLStaticHashedString 集約表から 67 名抽出 → `g_static_hashed_uniform_names[]` 入力
3. **存在しない場合 (= 高確率)** = 以下のいずれか:
   - (i) `LLStaticHashedString` literal 経由の C++ caller を grep で抽出 (= `pipeline.cpp` / `lldrawpool*.cpp` / `llreflectionmapmanager.cpp` 等から `LLStaticHashedString` 引数の literal 文字列を集める)、build-time list 化
   - (ii) PB-3 を S1-B (= shader link 時 `glGetActiveUniform` 列挙結果で `mReservedUniforms` 未登録 uniform を拾う) に **path 切替判断** (= AYA 判断項に格上げ、handoff 中断)
   - (iii) PB-3 scope 縮小 = 67 個未確定のまま `mUniformUBOLocByHash` 空構造のみ用意し、PB-5 着手時の sub-step 単位で setter 個別 hash を追加 (= overall scope は同等、確定順序を後置)

**Claude 推奨** (= AYA 判断不要、自走時 default): (i) **grep 抽出 で list 化** (= S1-C 採用範囲内、build-time decidable 維持)。手順:
- `grep -rn 'LLStaticHashedString' indra/newview indra/llrender` 等で literal 文字列を抽出
- 重複除去 + sort → 67 個前後に収束するか確認
- 67 個前後で収束 = `g_static_hashed_uniform_names[]` 入力源として採用
- 大幅乖離 (= 例えば 200 個超) = AYA に S1-B 切替を提案して handoff 中断

### §3.3 PB-3 内副次設計判断 (= 着手時に決定、AYA 判断不要)

| 項目 | 候補 | Claude 推奨 |
|---|---|---|
| `g_static_hashed_uniform_names[]` 配置場所 | (a) `llglslshader.cpp` anonymous namespace / (b) 新規 file (= `static_hashed_uniform_names.inl`) / (c) chapter 05 集約表と同居 (= `05a-bare-uniform-mapping.md` 結果を Codegen 出力 .inl 化) | **(a)** (= 67 個程度なら 1 file 内で十分、Codegen 化は PB-3 scope overshoot、後 Phase で必要なら refactor) |
| PB-3 cache 構築 block 位置 | PB-2 block 直後 = `unbind()` 直前 か `unbind()` 直後 | **PB-2 block 直後 = `unbind()` 直前** (= §3.1 設計判断と整合、対称構造内) |
| LLStaticHashedString hash 計算 API | (i) `LLStaticHashedString("name").getStringHash()` literal 経由 / (ii) `fnv1a_32` を直接呼ぶ | **(i) LLStaticHashedString 経由** (= setter 側 (PB-5) で `uniform.getStringHash()` と同 API、cache 構築側と setter 側の hash 一致保証) |

---

## §4 self-verify (= 9 観点、本 handoff 起案時点)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) PB-2 物理 diff | `git diff --stat indra/llrender/llglslshader.cpp` で 1 file changed + 26 insertions、handoff doc 未起案時点 | ✅ |
| (2) PB-2 挿入位置 | `llglslshader.cpp:1864-1898` 範囲 = 既存 GL UBO binding loop 直後 + `unbind()` 直前、bind/unbind 対称構造内 | ✅ |
| (3) GATE-B 順守 | `#ifdef LL_VULKAN_GLSL` 不使用、`if (mUseUBO)` runtime gate のみ、grep で `LL_VULKAN_GLSL` 文字列出現 0 件 (本 block 内) | ✅ |
| (4) mUseUBO=false default 維持 | `mUseUBO` initial value = `false` (= llglslshader.h:429)、本 block 内で書き換えなし、既存 OpenGL build / 既存挙動 100% 維持 | ✅ |
| (5) llrender 単体 build PASS | `make -j$(nproc) llrender` = `[ 33%] Built target codegen_ubo` → `Building llglslshader.cpp.o` → `Linking libllrender.a` → `[100%] Built target llrender` | ✅ |
| (6) full viewer build PASS | `make -j$(nproc)` = `[100%] Built target llpackage` + tar.xz package `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz` 生成 | ✅ |
| (7) spec 06a §4.1 literal 準拠 | `mReservedUniforms.size()` resize + per index `lookup_runtime(c_str)` → hit:代入 / miss:`cadence_tag=0xFFFFFFFFu` sentinel (= 06a §3.3 `CADENCE_INVALID` literal 値) | ✅ |
| (8) 残 sub-step strict 線形 | PB-3 (= S1-C build-time list) → PB-4.1〜.17 → PB-5.1〜.13 → PB-6 (= FWD-1 完全 stub) → PB-7 (= 整合 check) → PB-N + 1 method 1 sub-step (= `feedback_ubo_migration_one_at_a_time` 準拠) | ✅ |
| (9) git working tree | `git status --short` で `M indra/llrender/llglslshader.cpp` のみ + 本 handoff doc 新規、scripts/ 改変なし + shader assets 改変なし + Co-Authored-By: Claude 行不在 | ✅ |

---

## §5 引き継ぎ済 memory (= 次 session で active、PB-1 complete handoff から継承、追加なし)

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step (PB-3〜PB-7) で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate (= 本 PB-2 で適用済)
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守
- `feedback_self_verify_before_handoff` — PB-N 検証時に AYA 起動目視前に Claude 「3 経路非到達 verify」を Phase 1.B 後 state で再走
- `feedback_no_claude_coauthor` — 本 handoff doc 含め全 commit 共著行不在
- `feedback_one_step_at_a_time` — PB-3 → … → PB-N strict 線形、各 PB-X 単独 commit
- `feedback_ubo_migration_one_at_a_time` — PB-4 内部 17 method / PB-5 内部 13 method 一括禁止、1 method 1 sub-step
- `feedback_doubt_self_first` — PB-X 中の不可解な build error / link fail は AYA に投げる前に Claude が root cause 特定
- `feedback_admit_unknown` — Vulkan path 動作確認等の判断は推測で進めず source-of-truth (= 06a chapter) literal 再確認
- `feedback_proactive_handoff` — Phase 1.B 全終了時に Phase 1.C entry handoff doc 起案
- `feedback_no_auto_commit` — 本 handoff doc + PB-2 commit + 各 PB-X commit は AYA 明示指示後 commit
- `feedback_remove_verification_logs` — PB-X 実装中の `LL_INFOS` hook は commit 前必ず除去、debug `llassert` は仕様内ゆえ残す
- `feedback_build_only_verified` — Phase 1.B Exit Criteria literal 充足 (= viewer launch + 経路非到達 verify) 後 commit
- `feedback_tests_dir_never_commit` — `scripts/ubo_codegen/tests/` は commit 不可
- `feedback_build` — Phase 1.B PB-N full build フローは Claude 全権実行
- `project_build_procedure` — PB-N で参照
- `project_ayastorm_r41_vulkan_migration` — Phase 1.B PB-2 完了 = 次 PB-3 着手状態
- `project_ayastorm_r41_design_principles` — 原則 1「upstream 取込やすさ維持」 = GATE-B 採用根拠
- `project_ayastorm_three_platforms` — Phase 1.B Exit Criteria literal 充足は Linux first-class baseline で十分
- `feedback_use_agents_proactively` — PB-3 着手前 grep 抽出 (§3.2 (i) 推奨経路) は Agent 投入候補
- `feedback_explanation_lead_with_conclusion` — AYA 判断項提示時に結論先

---

## §6 次 session 着手 1 line

**「前 session で Phase 1.B PB-2 `mapUniforms()` Vulkan path integer index 経路 cache 構築 block 追加 完了 (= `indra/llrender/llglslshader.cpp:1864-1898` 既存 GL UBO binding loop 直後 + `unbind()` 直前 に `if (mUseUBO) { mUniformUBOLoc.resize + per index lookup_runtime → 代入 / miss 時 cadence_tag=0xFFFFFFFFu sentinel }` 26 line 追加、GATE-B 整合で `#ifdef LL_VULKAN_GLSL` 不使用 + `if (mUseUBO)` runtime gate 単独、llrender 単体 build PASS + full viewer build PASS + tar.xz package 生成)、PB-2 完了 handoff doc 起案 + AYA 明示指示後 batch commit。本 session = **AYA 判断不要 自走** = **PB-3 着手** = LLStaticHashedString 経路 cache 構築 (= S1-C 採用、build-time list `g_static_hashed_uniform_names[]` 新規追加 + `mapUniforms()` 末尾 PB-2 block 直後 (= `unbind()` 直前) に iterate block 追加して `mUniformUBOLocByHash[hash(name)] = *loc` 登録)。**ただし** PB-3 着手前に **chapter 05 集約表 (= `05a-bare-uniform-mapping.md` 切出し doc) の存在確認 + LLStaticHashedString 67 個の具体名 list 確定状況** を pinpoint Read で確認 (= §3.2)。doc 不在の場合 default = grep 抽出 (= `LLStaticHashedString` literal C++ caller を `indra/newview` + `indra/llrender` で grep → 重複除去 + sort → 67 個前後収束確認) で build-time list 化、67 個前後で収束しない場合のみ AYA に S1-B 切替 提案 (= handoff 中断)。PB-3 → build PASS → 独立 commit → 以降 PB-4.1〜.17 (= 17 method integer index) → PB-5.1〜.13 (= 13 method hashed string) → PB-6 (= FWD-1 完全 stub) → PB-7 (= 整合 check + 1 method 1 sub-step) → PB-N (= MUSEUBO-A `mUseUBO=false` default で full build + viewer launch + bind 不変 verify) と strict 線形進行。」**
