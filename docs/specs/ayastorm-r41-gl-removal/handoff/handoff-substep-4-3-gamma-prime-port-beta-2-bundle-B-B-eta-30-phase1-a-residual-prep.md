# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A **residual** prep (= candidate (X))

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `35c4be1046` = Phase 1.B **complete** marker handoff doc 起案 (= Phase 1.B host-side 全終了 verify PASS + 次 milestone 4 候補 (X)(Y)(Z)(W) record)

**本 handoff doc 目的**: **Phase 1.B complete handoff §3.3 候補 (X)「Phase 1.A 残作業 (= codegen pipeline 実 run + 生成 header テスト program 経路通電 verify)」着手 prep**。Phase 1.A complete handoff (`…-phase1-a-complete.md`) §2.5 副次 finding 3 件の **現状判定** + literal「include + bind 通電 verify」要件の **解釈分岐** AYA 判断要件起案 + 候補 sub-task 構成 record。

---

## §0 state 一行 summary

候補 (X) Phase 1.A 残作業 = **literal scope 内訳 = (a) 副次 finding 3 件 + (b) Phase 1.A Exit Criteria (iii) literal「include + bind 通電 verify」要件**。各々現状判定:

- **副次 (1)** `_PREFIX_TO_CADENCE` 拡張 (cadence default fallback 85 件解消) = **既解消済** (= commit `7fe58b7428` Phase 1.B PB-1 着手前独立 commit、main.py +11 line、unittest 130/130 PASS、`ubo_metadata.inl` cadence 分布 = 0:3 + 1:80 + 2:7 = 90 完全一致)
- **副次 (2)** CMakeCache.txt invalidate 手順 = **永続記録不要** (= 都度発生時 `cmake -U <var>` で対処、cmake module side `set(... FORCE)` 化は user override block ゆえ一般慣例で避ける)
- **副次 (3)** spirv-cross system install = **CLI 不在確認済** (= `which spirv-cross` 出力 0 + `dpkg -l | grep spirv` で library `libspirv-cross-c-shared0` + `libspirv-cross-c-shared-dev` のみ install、CLI 別 package `spirv-cross` Ubuntu repo 確認済 `apt-cache search spirv-cross` で literal 「spirv-cross - Convert SPIR-V to other shader languages (CLI tool)」)
- **(b)** Phase 1.A Exit Criteria (iii) literal「include + bind 不変動作確認」 = **Phase 1.A complete §2.3 で「経路非到達 3 観点 verify」literal 充足判定済**、ただし literal「include 実施 + bind 実施 + 動作 unchanged」厳密解釈で実通電 (= 既存 program 1 個で codegen header 実 include + 実 bind) を求めるなら別 sub-task 起案要件

**AYA 判断要件**: (b) literal 解釈分岐 = **literal scope 厳密解釈** で実通電 PA-A sub-task 着手 or **既存 §2.3 経路非到達 verify で literal 充足判定維持** (= Phase 1.A は実質完了)。前者なら PA-A (literal 通電 verify) + PA-B (spirv-cross install) の 2 sub-task、後者なら PA-B (spirv-cross install) 単独 sub-task。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | (X) Phase 1.A 残作業 prep state + AYA 判断要件 + 候補 sub-task 構成 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-complete.md` | §0 / §2.3 / §2.5 / §3.5 | Phase 1.A 全終了 state + 経路非到達 3 観点 verify literal + 副次 finding 3 件 + Phase 1.B/1.C/2 着手前 setup task 候補 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §4.2 (= line 204-224) | Phase 1.A Exit Criteria literal (= 「既存 85 UBO blueprint codegen 実行 PASS + 生成 header をテスト program (= 既存 program 1 個) で include + bind 不変動作確認」) + Phase 1 完了時点「既存 program 動作 unchanged」注 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `scripts/ubo_codegen/main.py:65-80` | `_PREFIX_TO_CADENCE` 8 entry + `_SUFFIX_TO_CADENCE` 1 entry (= 副次 (1) 解消済 state 確認) |
| `indra/cmake/AyaUboCodegen.cmake:L24-L60` | `aya_attach_ubo_codegen()` 関数定義 (= 既存 `add_dependencies` + `target_include_directories(PUBLIC)` のみ、`target_link_libraries` 不変) |
| `indra/llrender/CMakeLists.txt:10,120` | `include(AyaUboCodegen)` + `aya_attach_ubo_codegen(llrender)` 既設 wiring (= Phase 1.A wiring) |
| `${CMAKE_BINARY_DIR}/codegen/ubo/ubo_index.inl` (= build 後生成) | 既存 program で実 #include する場合の API 入口 (= PA-A literal 通電 sub-task 候補 input) |
| `apt-cache search spirv-cross` 出力 | `spirv-cross - Convert SPIR-V to other shader languages (CLI tool)` (= 副次 (3) sub-task PA-B install package 名) |

---

## §2 (X) Phase 1.A 残作業 scope 内訳

### §2.1 (a) 副次 finding 3 件の現状判定

| # | 副次 finding | 状態 | 根拠 | 残作業判定 |
|---|---|---|---|---|
| (1) | `_PREFIX_TO_CADENCE` 拡張 (= cadence default fallback 85 件解消) | ✅ 既解消済 | commit `7fe58b7428` Phase 1.B PB-1 着手前独立 commit、`scripts/ubo_codegen/main.py:65-80` `_PREFIX_TO_CADENCE` 8 entry + `_SUFFIX_TO_CADENCE` 1 entry physical 確認、unittest 130/130 PASS、`ubo_metadata.inl` cadence 分布 = 0:3 + 1:80 + 2:7 = 90 完全一致 | **残作業なし** |
| (2) | CMakeCache.txt invalidate 手順 | ❌ 永続記録不要 | Phase 1.A complete §3.5 (2) literal「都度発生時に `cmake -U <var>` 適用、永続記録不要」、`set(... FORCE)` 化は user override block ゆえ一般 cmake 慣例で避ける | **残作業なし (scope 外)** |
| (3) | spirv-cross CLI system install | ⏳ 未実施 | `which spirv-cross` 出力 0 + `dpkg -l` で library のみ install + Ubuntu repo に CLI package `spirv-cross` 存在 (`apt-cache search` 確認) | **残作業あり**、Phase 1.C / Phase 2 着手前 setup task として sub-task 化候補 |

### §2.2 (b) Phase 1.A Exit Criteria (iii) literal 解釈分岐

09 §4.2 Phase 1.A Exit Criteria literal:

> 「既存 85 UBO blueprint に対する codegen 実行 PASS + 生成 header を**テスト program (= 既存 program 1 個) で include + bind 不変動作確認**」

Phase 1.A complete §2.3 で「経路非到達 3 観点 verify PASS = bind 不変動作 literal 確証」と判定:

| 経路 | 状態 |
|---|---|
| (A) blueprint .glsl が viewer の shader loader に拾われる | ❌ 拾われない (= 名指し list 不在) |
| (B) 既存 .cpp が codegen header を #include | ❌ 誰も include していない (= grep 0 件) |
| (C) llrender library link 関係変化 | ❌ 不変 (= `target_link_libraries` 不在) |

= **経路非到達 = unchanged** で literal「動作 unchanged」充足判定。**ただし literal「include + bind 不変動作確認」を厳密に分解読みすると:**

| literal 句 | 厳密解釈 |
|---|---|
| **include** | 既存 program 1 個で codegen header (= `ubo_index.inl` 等) を **実 #include** |
| **bind** | 該当 program で UBO **実 bind** (= API 層通電) |
| **不変動作確認** | rendering result 既存と同じ (= 視覚的・log 的・per-frame stat 的全観点 unchanged) |

→ Phase 1.A complete §2.3 は「経路非到達」(= include なし、bind なし) で「不変動作確認」を literal 充足、ただし「include 実施 + bind 実施 + 動作 unchanged」厳密解釈なら未実施。

**AYA 判断要件 (b 分岐)**:

| 案 | 内容 | scope | 工数 |
|---|---|---|---|
| (b-1) | **literal 充足判定維持** = §2.3 経路非到達 verify で literal「動作 unchanged」充足 = Phase 1.A は実質完了 (= literal「include」「bind」は不到達でも「不変動作」literal 充足、Phase 1.B host-side 完了済で再検証不要) | 0 sub-task、PA-B sub-task のみ (= spirv-cross install) | 0 line indra/ + 0 line scripts/、(3) のみ |
| (b-2) | **literal 厳密通電 verify** = 既存 program 1 個 (= 例 sanity_check.cpp 経路 or `llvkloader` smoke program 等) で codegen header 実 #include + 実 bind + 動作 unchanged 確認 = PA-A sub-task 起案 | 1 sub-task PA-A、+ PA-B = 2 sub-task | TBD (= 既存 program 選定 + include 1 line + bind 1 call site、+ 動作 unchanged verify = 既存 build + viewer launch + log diff、見積 3-5 file modified) |

**Claude 推奨 (= AYA 判断仰ぐ前提): (b-1) literal 充足判定維持**。根拠 3 件:
1. Phase 1.A complete handoff §0 で「Phase 1.A 全終了 state」 + Exit Criteria (i)(ii)(iii) 全充足 marker 既起案
2. Phase 1.A complete §2.3 「経路非到達 = unchanged」literal 充足判定が 09 §4.2 「Phase 1 完了時点では既存 program 動作 unchanged (= Vulkan path 分岐 ON でも OpenGL path 経路を選ぶ default 動作)」注 と整合
3. Phase 1.B host-side 完了済で host-side 改変は全て `if (mUseUBO)` runtime gate 内 + `mUseUBO=false default` 維持 (= MUSEUBO-A) ゆえ「テスト program 経路通電 verify」は Phase 1.C test UBO shell の `vkCmdBindDescriptorSets` 通電と semantic overlap、二重実施は scope overshoot

ただし AYA さん literal scope 厳密解釈で (b-2) 採用判断もあり得る。

### §2.3 sub-task 構成案 (= AYA 判断後機械的展開)

#### §2.3.1 案 (b-1) literal 充足判定維持 (Claude 推奨)

| sub-task | scope | Exit |
|---|---|---|
| **PA-B** | spirv-cross CLI system install (= `sudo apt install spirv-cross` Linux first-class baseline) | `which spirv-cross` 出力 PASS + `AYA_CODEGEN_SKIP_SPIRV_CHECK` env unset で codegen 再走 + SPIR-V binding cross-check PASS (= 06a §3.4 inventory 更新時 verify) |
| **PA-N** | Exit Criteria 確認 + handoff complete marker | PA-B PASS + 9 観点 self-verify + handoff doc 起案 + 1 commit |

#### §2.3.2 案 (b-2) literal 厳密通電 verify

| sub-task | scope | Exit |
|---|---|---|
| **PA-A** | 既存 program 1 個選定 + codegen header 実 #include + 実 bind + 動作 unchanged 確認 | 選定 program で build PASS + viewer launch PASS + 視覚 diff 0 + log diff 0 + per-frame stat 0 deg |
| **PA-B** | (= 案 (b-1) 同様) | (= 同) |
| **PA-N** | (= 案 (b-1) 同様) | (= 同) |

---

## §3 残 sub-step strict 線形 (= 候補 (X) 着手後反映)

### §3.1 現 status (= 候補 (X) 着手前)

```
✅ Phase 1.B host-side 完了 (= commit 35c4be1046 marker)
  → ⏳ 次 milestone: (X) Phase 1.A 残作業 / (Y) Phase 1.C 着手 / (Z) AYAstorm r20 SSS verify / (W) 上流 uniform4iv bug fix
  並行可能性: (X) ⊥ (Z) ⊥ (W)、(Y) は (X) 後推奨
```

### §3.2 候補 (X) 内 strict 線形 (= 案 (b-1) 採用時)

```
(X) PA-B (spirv-cross install) → (X) PA-N (Exit + handoff) → 候補 (Y) (Z) (W) AYA 判断
```

### §3.3 候補 (X) 内 strict 線形 (= 案 (b-2) 採用時)

```
(X) PA-A (literal 通電 verify) → (X) PA-B (spirv-cross install) → (X) PA-N (Exit + handoff) → 候補 (Y) (Z) (W) AYA 判断
```

---

## §4 self-verify (= 本 handoff 起案時点、commit 前確認)

| # | 観点 | 確認方法 | 期待 |
|---|---|---|---|
| (1) (X) literal scope 内訳分解 | §2 で (a) 副次 finding 3 件 + (b) Exit Criteria (iii) literal 解釈分岐 を明文化 | ✅ §2.1 + §2.2 |
| (2) 副次 (1)(2)(3) 現状判定 | (1) commit `7fe58b7428` で解消済 + (2) 永続記録不要 scope 外 + (3) CLI 不在 install package 確認 | ✅ §2.1 table |
| (3) (b) literal 解釈分岐記録 | 案 (b-1)(b-2) 比較 table + Claude 推奨 (b-1) 根拠 3 件 | ✅ §2.2 table + 根拠 |
| (4) 候補 sub-task 構成案 | (b-1)(b-2) 各々の PA-A / PA-B / PA-N 構成 | ✅ §2.3.1 + §2.3.2 |
| (5) 残 strict 線形反映 | (b-1)(b-2) 採用後の sub-step 順 | ✅ §3.2 + §3.3 |
| (6) commit 内容 | handoff doc 1 件のみ、indra/ + scripts/ + cmake/ 改変 0 | ✅ (本 commit 段) |
| (7) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 | ✅ |
| (8) `feedback_self_bug_no_defer_option` 遵守 | 副次 (2) は「永続記録不要」literal 判定 (= 「先送り」signal でなく structural 判断)、副次 (3) は sub-task PA-B 独立起案 (= 「先送り」でなく独立 fix 案) | ✅ |
| (9) `feedback_handoff_minimal_pre_req_read` 遵守 | §1.1 必読 3 件のみ、§1.2 pinpoint Read 用 reference 別記 | ✅ |

---

## §5 引き継ぎ memory (= 次 session 着手時参照)

特に重要 (= 既存 memory から):

- `project_ayastorm_r41_vulkan_migration` (= r41 milestone active marker)
- `project_ayastorm_r41_design_principles` (= 上流取込やすさ + Core 分散実現)
- `project_r41_phase1b_vulkan_host_gate` (= GATE-B / MUSEUBO-A 確定)
- `feedback_ubo_migration_one_at_a_time` (= 1 sub-task 1 commit + cold launch 検証挟む)
- `feedback_self_verify_before_handoff` (= AYA 提示前 self-trace)
- `feedback_self_bug_no_defer_option` (= 「先送り/disable」を提案として並べない、fix or 別 sub-step 独立起案のみ)
- `feedback_doubt_self_first` (= AYA 提示情報を疑わず、自分の改変を疑う)
- `feedback_handoff_minimal_pre_req_read` (= 次 session pre-req は最小 3 件)
- `feedback_no_auto_commit` (= 本 doc commit は AYA 明示指示後)
- `feedback_no_claude_coauthor` (= Co-Authored-By 行不在)
- `feedback_no_scope_shrink` (= 候補 (X) literal scope (a)+(b) 両方扱う、(a) のみで完結させない)

---

## §6 次 session 着手 1 line

**「前 session で候補 (X) Phase 1.A 残作業 prep handoff doc 起案 + commit (= 本 doc + 1 commit)。本 session = AYA 判断 (b-1) 案 literal 充足判定維持 (= PA-B + PA-N 2 sub-task) or (b-2) 案 literal 厳密通電 verify (= PA-A + PA-B + PA-N 3 sub-task) のいずれか確定後 → 該当 sub-task 着手。必読 3 件 = (1) 本 handoff doc 全文 + (2) `…-phase1-a-complete.md` §0 / §2.3 / §2.5 / §3.5 + (3) 09 §4.2 line 204-224。副次 (1) 既解消済 + 副次 (2) 永続記録不要 scope 外 + 副次 (3) spirv-cross CLI install 残存。Phase 1.A Exit Criteria (iii) literal「include + bind 不変動作確認」は Phase 1.A complete §2.3 「経路非到達 verify」で literal 充足判定済、AYA さん literal 厳密解釈で (b-2) 採用判断もあり得る。」**
