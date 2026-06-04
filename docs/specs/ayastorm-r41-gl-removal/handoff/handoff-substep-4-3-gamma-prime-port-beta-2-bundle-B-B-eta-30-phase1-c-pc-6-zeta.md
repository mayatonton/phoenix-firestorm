# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-6ζ complete** handoff

**作成日**: 2026-06-05
**Phase 1.C 進行段階**: PC-6ζ complete (= setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突 正攻法対応完了)
**HEAD (commit 前)**: `d09d6f10fd` (= PC-6ε-3 = per-draw 残 pool 15 site 全配線、計 16 pool subclass wired)
**次 session 着手 (1 line)**: PC-7 = vkCmdBindDescriptorSets 通電 + dynamic offset 経路 ring buffer chunk hand-off + UboInstance member 拡充 (VkBuffer / mapped_ptr / size) + forwardToUboUpload 本格化 (= dirty=true 経路有効化)。

---

## §0 必読 3 件 (= 次 session 最小 pre-req)

1. 本 handoff doc 全文 (= PC-6ζ 実施内容 + Exit Criteria + 9 観点 self-verify + PC-7 entry conditions)
2. `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` §3.3 enum 表 + §5.6 現状実装注記 (= PC-6ζ 確定の SAMPLER=6 / SINGLETON=5 / UNKNOWN=7 値域 + 衝突解消経緯)
3. `docs/specs/ayastorm-r41-gl-removal/design/literal-cross-ref-audit.md` §3 PC-6ε..PC-N pre-cache literals + rule-1..rule-5 (= PC-7 着手前 drift 防止規律)

### pinpoint reference (= 必要時のみ読込)
- `indra/llrender/llglslshader.cpp:2030-2042` = bringupTestUBO 上の PC-6ζ 完了 marker comment (= 5→6 移動経緯 + setter 31 site 追従済の literal 記録)
- `indra/llrender/llglslshader.cpp:2059-2061` = PC-1 contract assert (= `cadence_tag == 5u` for SINGLETON、本 PC-6ζ で不変)
- `indra/llrender/llglslshader.cpp:2365-3538` = 31 setter site (= `cadence_tag == 6 /* CADENCE_SAMPLER ... */` 全件追従済)
- `scripts/ubo_codegen/main.py:58-74` = `CADENCE_SINGLETON = 5` + `_PREFIX_TO_CADENCE` (= codegen canonical 不変)

---

## §1 起案契機 + scope 確定

### §1.1 起案契機
AYA 指示 (2026-06-05) literal「PC-6ζ 着手お願いします。前 commit = d09d6f10fd (PC-6ε-3 = per-draw 残 pool 15 site 全配線)。必読 3 件 + Exit Criteria 整理 + scope ambiguity 発見時は AYA 確認」受領。

### §1.2 PC-6ζ literal scope (= AYA 指示原文)
> setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突 正攻法対応 (design 06a §5.6、llglslshader.cpp:2480-2563 + 3006-3079 17 setter family の codebase trace で正確な setter 数 + 衝突 site 確定)。

### §1.3 着手前発見 = ambiguity 2 件 + AYA 確認取得

#### (Y1) approach 選定
spec 側 (= 06a §3.3) と codegen 側 (= main.py:63 `CADENCE_SINGLETON = 5`) が同一 cadence_tag=5 を使う drift。2 案比較:

- (Y1) **sentinel 再配置** = SAMPLER 5 → 6 移動、SINGLETON=5 維持 (= codegen + PC-1 contract baked-in `cadence_tag == 5u` assert 全件不変)
- (Y2) codegen 側 SINGLETON 値変更 = main.py + perfect_hash.py + PC-1 contract assert 全件 cascade 改修

**AYA 採用 = (Y1)** (literal「OK」2026-06-05)。根拠 3 件 = (1) codegen 側 generated header (`Global_ReflectionProbes` 含む) + PC-1 contract `cadence_tag == 5u` assert (llglslshader.cpp:2061) baked-in、touchpoint 最小化 + (2) 31 setter site (= bare uniform / sampler skip 用 sentinel 比較) は spec 側でしか参照されない孤立点 + (3) 6 cadence 体系成立 (PC-6ε-1) で SINGLETON が正規 cadence、SAMPLER は UBO 化対象外 sentinel = 役割明確化。

#### (Y2) SAMPLER 移動先値
SAMPLER 6 配置 = (元 UNKNOWN=6) → UNKNOWN=7 連動移動。INVALID=0xFFFFFFFF は不変 (= sentinel max)。**AYA 採用 = 6** (= Y1 OK 包含)。

### §1.4 dead-code observation (= PC-6ζ で記録のみ、修正なし)
現状 31 setter SAMPLER skip line は実は dead code = codegen が SAMPLER cadence_tag を一切 emit しない (= sampler は集約表に登録されず CADENCE_INVALID=0xFFFFFFFF で skip 経路通過)。但し PC-7 で SINGLETON UBO setter 経路本格化時、もし SAMPLER=5 のままだと SINGLETON write を sampler と誤認して silent drop する critical bug 化 → 本 PC-6ζ の正攻法対応で恒久回避。

### §1.5 採用根拠 3 件 (= 06a §5.6 現状実装注記 literal 引用)
1. **PC-1 contract preservation** = `llassert(block->cadence_tag == 5u)` for `Global_ReflectionProbes` (= llglslshader.cpp:2061) は SINGLETON=5 で完全整合維持
2. **codegen canonical 不変** = main.py:63 `CADENCE_SINGLETON = 5` 不変、scripts/ubo_codegen 配下改変ゼロ
3. **spec 側 touchpoint 最小化** = 06a 内 SAMPLER 値参照 5 site (= §3.3 enum 表 + §4.4 (3) + §4.4.1 PB-7 (3) + §5.6 注記 + §5.6 中の (= 5) 注釈、§5.2/§5.3 は symbolic で不変) + llglslshader.cpp 31 setter site の改修のみ

---

## §2 編集詳細 (= 2 file modified、新 file 0)

### §2.1 編集 1: `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` (+15 / -7 行)
- **§3.3 enum 表更新** = `CADENCE_SINGLETON = 5` 追加 + `CADENCE_SAMPLER = 6` (5 → 6 移動) + `CADENCE_UNKNOWN = 7` (6 → 7 移動)、INVALID=0xFFFFFFFF 不変。**6 cadence 体系** 明文化段落追加 (= 5 update cadence + 1 SINGLETON = 6、PC-6ε-1 で確定、`Global_` prefix UBO = `LLVKLoader::flushSingletonUbos()` 経由 flush)
- **§4.4 (3)** = `CADENCE_SAMPLER (= 6)` 値注釈追加
- **§4.4.1 PB-7 (3)** = 同上
- **§5.6** = SAMPLER 値注釈 `(= 6、2026-06-05 PC-6ζ で 5→6 移動)` 追加 + **現状実装注記** 段落追加 (= Y1 採用根拠 + dead code observation + PC-1 contract preservation + codegen-vs-spec drift 解消経緯)
- **§5.2 (line 322 area) / §5.3 (line 351 area)** = 不変 (= symbolic `CADENCE_SAMPLER` 参照、literal 値直書きなし)

### §2.2 編集 2: `indra/llrender/llglslshader.cpp` (+50 / -42 行 = 31 setter site 置換 + 2 comment 更新)
- **31 setter site (line 2365-3538)** = `cadence_tag == 5 /* CADENCE_SAMPLER */` → `cadence_tag == 6 /* CADENCE_SAMPLER (2026-06-05 PC-6ζ で 5→6 移動、codegen SINGLETON=5 との衝突解消) */` 全件置換
- **line 1964-1966 comment** = `CADENCE_SAMPLER (= 5)` → `CADENCE_SAMPLER (= 6、2026-06-05 PC-6ζ で 5→6 移動)` + 5 行段落追加 (= 値変更根拠 = 06a §3.3 enum 表 + §5.6 現状実装注記 引用)
- **line 2034-2037 bringupTestUBO 上 comment** = 旧「PC-6ζ で正攻法対応予定」TODO 記述 → 「PC-6ζ (2026-06-05) で正攻法対応完了 = SAMPLER 値 5 → 6 移動、SINGLETON は 5 維持で PC-1 contract `cadence_tag == 5u` assert (本 file:2061) 不変。詳細は 06a §3.3 enum 表 + §5.6 現状実装注記、setter 31 site (本 file:2365-3538) 全件 `== 6` に追従済」完了 marker に書き換え
- **line 2059-2061 PC-1 contract assert** = `llassert(block->cadence_tag == 5u)` 不変 (= SINGLETON=5 で正解、改修不要)

### §2.3 改変なし (= 確認のみ)
- `scripts/ubo_codegen/main.py` (= CADENCE_SINGLETON=5 canonical 維持)
- `scripts/ubo_codegen/perfect_hash.py` (= "5:Singleton" comment 既存正確)
- `docs/specs/.../06b-cadence-update-site-and-dirty.md` line 339 (= `case CADENCE_SAMPLER:` symbolic、改修不要)
- `docs/specs/.../06c-descriptor-set-bind-wiring.md` line 448 (= `cadence_tag == CADENCE_SAMPLER` symbolic、改修不要)
- CMake 改変 0 / settings.xml 改変 0 / 新 file 0 (handoff doc 除く)

---

## §3 build verify (= 全 PASS)

| 検証項目 | 結果 |
|---|---|
| `make -j4 llrender` | libllrender.a link PASS + ERROR 0 / WARNING 0 (= PC-6ζ 改変関連) |
| `INTEGRATION_TEST_lluboringbuffer` | 11/11 YAY!! \\o/ (= PC-4 algorithm 層 regression なし) |
| `INTEGRATION_TEST_llassetubopool` | 10/10 YAY!! \\o/ (= PC-3 algorithm 層 regression なし) |
| `INTEGRATION_TEST_llpipelinecachestorage` | 13/13 YAY!! \\o/ (= PC-5 algorithm 層 regression なし) |
| `python3 -m unittest discover -s scripts/ubo_codegen/tests` | Ran 130 tests in 0.063s OK = 130/130 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-6ε-3 regression なし) |

---

## §4 PC-6ζ Exit Criteria 10 項全充足

| # | Exit Criteria | 充足 record |
|---|---|---|
| (i) | design 06a §3.3 enum 表更新 = SINGLETON=5 追加 + SAMPLER=6 (5 → 6 移動) + UNKNOWN=7 (6 → 7 移動) + INVALID 不変、cadence 体系明文化 | ✅ §2.1 編集 1 完了、6 cadence 体系段落追加 |
| (ii) | 06a §4.4 (3) / §5.2 例 code / §5.6 + 06b §339 case + 06c §448 の SAMPLER 値参照箇所整合更新 | ✅ §4.4 (3) + §4.4.1 PB-7 + §5.6 = 値注釈 `(= 6)` 追加、§5.2/§5.3/06b/06c = symbolic 参照で改修不要 |
| (iii) | `indra/llrender/llglslshader.cpp` 31 setter site SAMPLER 値 5 → 6 置換 | ✅ grep `cadence_tag == 6` = 31 件 + grep `cadence_tag == 5` = comment 2 件のみ (= PC-1 contract assert line 2061 + bringupTestUBO 上 comment line 2032 残置 OK) |
| (iv) | `mapUniforms()` PB-7 comment + bringupTestUBO 上 PC-6ζ TODO comment 整合更新 | ✅ line 1964-1966 + line 2034-2037 完了 marker 化 |
| (v) | PC-1 contract `llassert(block->cadence_tag == 5u)` 不変保証 (= SINGLETON=5 完全整合) | ✅ line 2061 改修ゼロ |
| (vi) | codegen 側 `CADENCE_SINGLETON = 5` 不変保証 (= main.py + perfect_hash.py 改修ゼロ) | ✅ scripts/ubo_codegen 配下 git diff 空 |
| (vii) | `make -j4 llrender` build PASS + warning 0 | ✅ §3 record |
| (viii) | TUT 3 件 (11/11 + 10/10 + 13/13) + codegen 130/130 全 PASS | ✅ §3 record |
| (ix) | MUSEUBO-A 整合 = `mUseUBO=false default で既存 OpenGL 描画 100% 維持 | ✅ 本 PC-6ζ は sentinel 値再配置のみ、`mUseUBO` runtime gate 不依存、setter 内分岐 31 site は `if (mUseUBO)` block 内側で本来 dead code (= sampler は INVALID 経路) ゆえ既存挙動不変 |
| (x) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 不使用、`mUseUBO` runtime flag 単独 gate | ✅ 本 PC-6ζ で新規 `#ifdef LL_VULKAN_GLSL` 追加 0、既存 `if (mUseUBO)` 内側の値変更のみ |

---

## §5 残 strict 線形

PC-6ζ (本) → **PC-7** (= vkCmdBindDescriptorSets 通電 + dynamic offset 経路 ring buffer chunk hand-off + UboInstance member 拡充 (VkBuffer / mapped_ptr / size) + forwardToUboUpload 本格化 = dirty=true 経路有効化) → **PC-8** (= 3 OS build verify、Linux primary + Win/Mac 後段) → **PC-N** (= Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点)

---

## §6 r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0 ✅ + PC-1 ✅ + PC-2 ✅ + PC-3 ✅ + PC-4 ✅ + PC-5 ✅ + PC-6α ✅ + PC-6β ✅ + PC-6γ ✅ + PC-6δ ✅ + PC-6δ' ✅ + PC-6ε-1 ✅ + PC-6ε-2 ✅ + PC-6ε-3 ✅ + **PC-6ζ ✅ 本 commit** + PC-7..PC-N ⏳ 次 session

---

## §7 self-verify 9 観点 全 ✅

1. **Exit Criteria 10 項全充足** = §4 表で literal 充足 record + grep 件数で検証
2. **enum 表 source doc 整合** = 06a §3.3 が canonical、SINGLETON=5 / SAMPLER=6 / UNKNOWN=7 / INVALID=0xFFFFFFFF + 6 cadence 体系明文化、PC-6ε-1 確定の `flushSingletonUbos()` 経路と整合
3. **(Y1) 採用根拠 record** = §1.5 で 3 件明文化 (= PC-1 contract preservation + codegen canonical 不変 + spec 側 touchpoint 最小化)、AYA literal「OK」確認 record (2026-06-05)
4. **GATE-B 整合** = `#ifdef LL_VULKAN_GLSL` 新規追加 0、setter 内 `if (mUseUBO)` 既存 block 内側の sentinel 値変更のみ
5. **MUSEUBO-A 整合** = `mUseUBO=false default で既存 OpenGL path 100% 維持、sentinel 値変更は `if (mUseUBO)` block 内の dead code (= sampler は INVALID 経路) ゆえ既存挙動不変
6. **llrender build + TUT 11/11 + 10/10 + 13/13 + codegen 130/130 全 PASS** = §3 record
7. **commit 内容** = 2 modified (= 06a md + llglslshader.cpp) + 1 new doc (本 handoff) + 新 file 0 + CMake 改変 0 + settings.xml 改変 0 + Co-Authored-By 不在
8. **feedback_no_scope_shrink 遵守** = PC-6ζ literal scope (= setter SAMPLER skip ↔ codegen SINGLETON cadence_tag=5 衝突 正攻法対応) 完全実施、31 setter site 全件置換、scope 縮小なし、dead-code observation は記録のみで scope 拡大なし
9. **feedback_doubt_self_first 遵守** = (Y1)/(Y2) 2 案比較 + SAMPLER 移動先値 確認の 2 段 AYA 確認取得 (2026-06-05 literal「OK」)、推測実装なし

---

## §8 次 session 着手 1 line (= PC-7)

**PC-7** = `vkCmdBindDescriptorSets` 通電 + dynamic offset 経路 ring buffer chunk hand-off + `UboInstance` member 拡充 (= VkBuffer / mapped_ptr / size、PC-6ε-2 で placeholder 確保済) + `forwardToUboUpload` 本格化 (= dirty=true 経路有効化、現 stub から ring buffer write + map + descriptor offset 計算へ移行)。Exit Criteria は次 session 着手前整理。

source doc = 06c §2 全体 + 07-vulkan-api-state §9.1 (pipeline layout) + §9.3 (PSO cache) + §12 (= descriptor set / cmdbuf 経路)。

---

## §9 feedback rule 遵守 record

- **feedback_proactive_handoff** = PC-7 引継 marker 本 handoff
- **feedback_handoff_minimal_pre_req_read** = 必読 3 件 + pinpoint reference 別記
- **feedback_self_verify_before_handoff** = 9 観点 self-verify 全 ✅
- **feedback_build_only_verified** = llrender build + TUT 11/11 + 10/10 + 13/13 + codegen 130/130 で literal 検証取得
- **feedback_no_scope_shrink** = §7 (8) 記録
- **feedback_doubt_self_first** = §7 (9) 記録 = 2 段 AYA 確認取得後実装
- **feedback_confirm_referent_before_acting** = (Y1)/(Y2) ambiguity 発見で停止 + AYA 確認 + literal「OK」受領後実装
- **feedback_ubo_migration_one_at_a_time** = PC-6ζ = sentinel 再配置単独実施、PC-7 (descriptor set / cmdbuf 通電) は次 session へ分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-6ζ は実装 phase (= PC-6ε-3 commit 後)、indra/ 改変 1 件 (llglslshader.cpp) = 設計 phase ではない
- **feedback_release_branch_workflow** = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** = AYA 明示 commit 指示受領後 commit (= 本 handoff 起案後 AYA 確認待ち)
- **feedback_no_claude_coauthor** = Co-Authored-By 行不在

---

(handoff 終端、本 doc 起案後 AYA 確認 → commit 指示受領後 commit)
