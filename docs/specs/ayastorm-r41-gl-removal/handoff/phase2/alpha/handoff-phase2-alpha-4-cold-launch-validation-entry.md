# handoff = r41 Phase 2.α α-4 = cold launch validation entry

**起案日**: 2026-06-06
**位置付け**: Phase 2.α α-4 sub-step 着手 entry handoff = case X 確定実装 (= phase A-G 7 commit 完了) 後の **cold launch validation**。AYA 立ち会い別 session で実施想定 (= configure + build + 起動 + Vulkan validation + AYA live verify)。
**起案契機**: Phase 2.α α-3 phase G 完了 commit `d9c6e48579` (= 案 X 確定実装 全 7 phase 完走 + 102 file 改修 + test 143 件 PASS + grep 残漏れ 0 件 confirm) 受領、AYA literal「α-4 entry handoff 起案して」(= 2026-06-06) 受領反映、memory `feedback_proactive_handoff` 適用で次 session 引き継ぎ。
**起案規律**:
- memory `feedback_proactive_handoff` 適用 (= 周回境界 = α-3 完了)
- memory `feedback_handoff_minimal_pre_req_read` 適用 (= 最低限 3 件 + pinpoint)
- memory `feedback_no_dual_doc_split` 整合 (= 本 doc は pointer 役、case X 確定 source of truth は entry handoff §D.9 集約)
- memory `feedback_root_cause_no_shortcuts` §9 適用 (= AYA literal 既発令で着手承認済、preflight 質問なし)
- memory `feedback_root_cause_no_shortcuts` §11 + §12 適用 (= 自走精査網羅性 checklist 4 件 + sandbox 実証 protocol + 「採否ご判断ください」禁忌 + subagent 結果盲信禁止 + 設計 doc literal 誤り疑念)
- memory `feedback_build_only_verified` 適用 (= cold launch 検証で効果 verify)
- memory `feedback_one_step_at_a_time` 適用 (= cold launch validation は 1 step ずつ)
- memory `feedback_self_verify_before_handoff` 適用 (= AYA 動作確認前に Claude 自走 verify 全件完走)

---

## §1. α-4 着手目的 + scope

**Phase 2.α α-4** = case X 確定実装 (= phase A-G) の **cold launch validation** = configure + build + viewer 起動 + Vulkan validation log + AYA live verify (= 視覚 regression ゼロ + UI 機能維持) で **80 UBO 全件 cold launch validation 完走**、Phase 2.α 完了 + Phase 2.L0 sub-session 5 続行点 resume 承認の条件成立を確定する。

### §1.1 case X 確定後の検証対象

| 軸 | 検証内容 |
|---|---|
| codegen 入力 | blueprint dir (= `aya_r41_blueprints/`、case X 確定 source of truth) |
| codegen 出力 | `ubo_metadata.inl` + `ubo_perfect_hash.inl` + `ubo_index.inl` + `ubo_dummy_init.inl` + `ubo_host_loader.inl` + per-block layout 5+ aggregated .inl (= build/codegen/ubo/ 配下) |
| 整合性 | blueprint dir + sub-session 5 step 2-batch-0-a 7 commit 改修済 actual `class*/` + `cinematic_bd/` 14 file の二重 source 同期 verify (= main.py `_verify_block_match` + `_verify_blueprint_actual_consistency` 拡張、phase F commit `868bc38cc9`) |
| build | configure 走行 + build (= `develop.py configure` → `make -C build-linux-x86_64` 等、3 OS 共通) |
| runtime | viewer cold launch (= 起動 OK + Vulkan validation log 0 件 + 80 UBO binding 反映 confirm) |
| visual | AYA live verify (= 視覚 regression ゼロ + UI 機能維持、原則 4 §5.4 V-1) |

### §1.2 case X 確定実装 完了済 sub-step (= 検証対象 source of truth)

| commit | phase | 内容 |
|---|---|---|
| `df38b7c994` | phase A | 案 Z'+ 6 commit revert + base state 復元 |
| `f95182ded5` | phase B | blueprint dir README 全面書換 (= codegen 入力 source of truth literal) |
| `b06f860a77` | phase C | handoff §D.9 案 X 確定 source of truth + α-3 entry §3 sub-step 7 phase 構造 |
| `64eb589ee7` | phase D | 設計 doc 5 件改修 (= 04 §2.2 literal 訂正 + 09/10 案 X 反映) |
| `09ee5e8a8e` | phase E | blueprint dir 7 UBO 7 file 同期書換 (= sub-session 5 改修 actual と整合) |
| `868bc38cc9` | phase F | main.py `_verify_blueprint_actual_consistency` 追加 (= 二重 source 整合 verify formal化 + `--verify-target-paths` option + 5 test) |
| `d9c6e48579` | phase G | 波及 doc 全件改修 + 全件 grep 走査残漏れ 0 件 confirm (= 102 file +2843/-31) |

Phase 2.L0 sub-session 5 step 2-batch-0-a 7 commit (= `887ddb5341`〜`e5f57d57ff`) は **freeze 維持**、α-4 で cold launch 結果反映後 sub-session 5 続行点 resume。

---

## §2. 必読 doc (= memory `feedback_handoff_minimal_pre_req_read` 適用)

### §2.1 最低限 3 件

1. **本 handoff doc** = α-4 着手 entry pointer
2. **`handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9** = case X 確定 source of truth (= §D.9.5 6 commit revert + §D.9.6 case X 改修方針 9 件 + §D.9.7 全件波及更新範囲 + §D.9.8 既保持 commit 整合性 + §D.9.9 §11+§12 反省)
3. **`handoff/phase2/alpha/handoff-phase2-alpha-3-cmake-blueprint-readme-propagation.md` §3** = case X sub-step 7 phase 構造 + §4 Exit 条件 (= α-3 完了確認 + α-4 着手準備確認)

### §2.2 必要時 pinpoint Read 候補

- `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= blueprint dir = codegen 入力 source of truth literal、case X 確定後の位置付け確認)
- `indra/cmake/AyaUboCodegen.cmake` (= `AYA_UBO_CODEGEN_BLUEPRINT_DIR` 復元状態、build wiring)
- `scripts/ubo_codegen/main.py` (= `_verify_block_match` + `_verify_blueprint_actual_consistency` + `--verify-target-paths` option、phase F 実装)
- `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` §2.2 + §4.4 (= literal 訂正版 + 二重 source 整合 verify 設計)
- `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md` (= CMake wiring + build pipeline 設計 doc)
- `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-batch-0-a-entry.md` §C (= sub-session 5 freeze record + 続行点 resume protocol)
- `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-pre2-mapping.md` §5.1 (= 80 slot alphabetical sort literal、α-4 codegen verify 時の expected 値 source)
- `indra/llrender/llvkloader.cpp` (= V3A_*_BINDINGS + `registerProgramUbo` subset 経路 dispatch、α-4 host C++ pipeline layout 整合源)

### §2.3 memory pinpoint

- `feedback_root_cause_no_shortcuts` §11 + §12 (= 自走精査網羅性 checklist 4 件 + sandbox 実証 protocol + 「採否ご判断ください」禁忌 + subagent 結果盲信禁止 + 設計 doc literal 誤り疑念)
- `feedback_doubt_self_first` §5 (= 解決策確定前に設計 doc 精査 default)
- `feedback_design_doc_number_literal_verify` (= 数値 + 構造属性 + codegen/build pipeline 入力 source path verify)
- `feedback_proactive_risk_management` (= AYA リスク管理肩代わり防止)
- `feedback_build_only_verified` (= cold launch 検証で効果 verify)
- `feedback_self_verify_before_handoff` (= AYA 動作確認前に Claude 自走 verify 全件完走)
- `feedback_one_step_at_a_time` (= cold launch は 1 step ずつ、AYA に複数 verify 同時依頼しない)
- `feedback_no_auto_commit` (= commit は AYA literal 指示後のみ)
- `feedback_release_flow` (= push / PR は AYA 側)
- `feedback_no_claude_coauthor` (= Co-Authored-By Claude 行禁止)
- `project_r41_phase2_4_principles` (= 4 原則: Core 分散 / 3 OS 共通 / Phase 2/3 範囲 / OpenGL を殺さない)
- `project_r41_design_principles` (= 2 大設計原則: upstream OpenGL 取り込み容易 + Core 分散実現)
- `project_r41_phase1b_vulkan_host_gate` (= `mUseUBO` runtime flag default OFF 維持)
- `feedback_ubo_migration_one_at_a_time` (= 1 sub-step 1 commit、各 sub-step 完了で検証挟む)
- `feedback_handoff_minimal_pre_req_read` (= 最低限 3 件 + pinpoint)
- `project_build_procedure` (= AYAstorm build 手順 = configure → build → install → cache clear)
- `feedback_build` (= ビルドフローは Claude 一括実行 OK、3 OS では Linux primary)

---

## §3. α-4 着手順序 + 想定 sub-step

| # | 改修内容 | 改変対象 | 想定 commit | 検証 |
|---|---|---|---|---|
| 4.1 | **codegen 単独走行 verify (= Claude 自走、build phase 前段)** = blueprint dir 入力で `ubo_metadata.inl` 等 emit 成功 + 80 UBO binding 整合 confirm + main.py `--verify-target-paths` で sub-session 5 改修 7 UBO 対応 actual 4-7 file 整合 verify (= phase F 動作実機 confirm) ⇒ **✅ 完了 (= 2026-06-06、§C.1 evidence record)** | 0 (= verify のみ) | 0 commit (= verify 結果 record は §C.1 追記、本 sub-step 完了 commit に同梱) | codegen tool 単独実行 + grep `ubo_metadata.inl` binding 確認 ⇒ **PASS verdict** (§C.1) |
| 4.2 | **configure 走行 verify (= AYA 環境、Linux primary)** = AyaUboCodegen.cmake が正しく走行 + STATUS message で blueprint dir 検出 confirm + GLSL source count = 94 (= blueprint dir) 確認 ⇒ **✅ 完了 (= 2026-06-06、§C.2 evidence record)** | 0 (= configure のみ) | 0 commit (= verify 結果 record は §C.2 追記、本 sub-step 完了 commit に同梱) | `autobuild configure -A 64 -c ReleaseFS_open` 走行 + STATUS 5 件確認 ⇒ **PASS verdict** (§C.2) |
| 4.3 | **build 走行 verify (= AYA 環境、Linux primary)** = make build success + codegen_ubo target 走行成功 + build artifact (= `build-linux-x86_64/codegen/ubo/*.inl`) 生成確認 + viewer binary 生成 OK ⇒ **✅ 完了 (= 2026-06-06、§C.3 evidence record)** | 0 (= build のみ) | 0 commit (= verify 結果 record は §C.3 追記、本 sub-step 完了 commit に同梱) | `autobuild build -A 64 -c ReleaseFS_open --no-configure` 走行 + grep build log ⇒ **PASS verdict** (§C.3) |
| 4.4 | **cold launch verify (= AYA 環境、Linux primary)** = viewer 起動 OK + Vulkan validation log 0 件 + sub-session 5 改修 7 UBO の binding 反映 runtime confirm | 0 (= 起動のみ) | 0 commit | AYAstorm log 確認 (`~/.ayastorm_x64/logs/AYAstorm.log` grep "Validation" / "UBO") |
| 4.5 | **AYA live verify (= AYA live、視覚 regression check)** = AYA 立ち会いで sample scene + UI 機能の視覚 regression ゼロ confirm (= 原則 4 §5.4 V-1) | 0 (= 視覚 verify のみ) | 0 commit | AYA literal 「OK」承認 |
| 4.6 | **α-4 完了 record 起案 (= Phase 2.α 完了 doc 起案 + Phase 2.L0 sub-session 5 続行点 resume 承認 候補)** | docs/specs/ 1 file 新規 or 既存 entry handoff §C 追記 | 1 commit | doc 確認 + AYA literal 承認 |

各 sub-step 完了で AYA literal commit 指示待ち (= memory `feedback_no_auto_commit` 適用)。連結 commit は AYA literal 指示で可。

**重要 (= AYA 立ち会い別 session 想定)**:
- 4.1 (= codegen 単独 verify) は Claude 自走で本 session 内可能 (= sandbox 実証 protocol §12 適用)
- 4.2-4.5 は **AYA 立ち会い別 session** = build + 起動 + AYA live verify 必須、Claude 自走不可
- 4.6 は AYA literal 承認受領後 doc 起案 + commit

---

## §4. α-4 Exit 条件

| # | Exit 項目 | 判定基準 |
|---|---|---|
| 1 | codegen 単独走行 verify PASS (= 4.1) | blueprint dir 入力で 94 .glsl → 94 UBO emit + parse error 0 件 + `ubo_metadata.inl` 80 UBO binding 整合 confirm + `--verify-target-paths` で sub-session 5 改修 7 UBO 対応 actual 整合 verified=7 + no_match=0 (= phase F 動作 evidence) |
| 2 | configure + build 走行 verify PASS (= 4.2-4.3) | configure 走行 error 0 件 + codegen_ubo target 走行成功 + viewer binary 生成 OK + build artifact `ubo_metadata.inl` 生成確認 |
| 3 | cold launch verify PASS (= 4.4) | viewer 起動 OK + Vulkan validation log 0 件 + sub-session 5 改修 7 UBO binding 反映 runtime confirm + crash / regression 0 件 |
| 4 | AYA live verify PASS (= 4.5) | 視覚 regression ゼロ + UI 機能維持 + AYA literal「OK」承認 |
| 5 | α-4 完了 record + Phase 2.α 完了 + sub-session 5 続行点 resume 承認 (= 4.6) | doc 起案 + AYA literal 承認 |

全 5 件満たして Phase 2.α 完了、Phase 2.L0 sub-session 5 step 2-batch-0-a 7 commit (= `887ddb5341`〜`e5f57d57ff`) freeze 解除 + sub-session 5 続行点 (= cold launch + AYA live verify) resume。

---

## §5. 手戻り protocol

- **α-4 内手戻り** = sub-step (= §3 表内) reject → 該当 sub-step 内 logic 再起案
- **α-4 → α-3 戻り** = 改修不備発覚 (= 例: codegen 走行で UBO binding 不整合 / runtime Vulkan validation error / visual regression) → 該当 phase (= A-G) commit に追加修正 + α-3 sub-step 再走
- **α-4 → α-1 戻り** = 設計方針破綻 (= 例: case X 本質的構造問題発覚 = 第 6 段同じ穴落ち) → Phase 2.α entry handoff §D.10 起案 (= memory `feedback_root_cause_no_shortcuts` §11 + §12 自走精査網羅性 + sandbox 実証 protocol 強化適用)

手戻りは **失敗ではなく cycle の正常動作** (= memory `feedback_falsification_as_progress`)。

**「同じ穴」事例 record** (= 2026-06-06 5 連続落ち + 案 X 確定):
- 案 Y (= blueprint 完全廃止) AYA 指示 #5 違反で撤回
- 案 Z (= class*/ + cinematic_bd/ 入力切替) parse error 第 1 階層発覚で撤回
- 案 Z' (= 案 Z + C++ runtime emulation 層追加) parse error 第 2 階層発覚で撤回
- 案 X (= blueprint dir = codegen 入力 source of truth) **確定** = blueprint dir 単独走行で 94 UBO emit + dump file なし + parse error 0 件 evidence、設計 doc 04 §2.2 literal 誤り訂正、AYA 指示 #5 真意 = source of truth 保護指示

α-4 実施時の **再発防止 protocol** (= §11 + §12 適用):
- 自走精査網羅性 checklist 4 件 (= 設計 doc 全文逐語 + 実装 full trace + structural property + 影響範囲) を sub-step 毎に適用
- sandbox 実証 protocol = AYA 動作確認前に Claude 自走 verify 全件完走 (= 例 codegen 単独走行 + grep 整合 confirm)
- subagent 結果は raw evidence のみ信任、判定は Claude が grep + Read で確認
- 設計 doc literal 自体の誤り疑念 = case X で 04 §2.2 literal 訂正経験適用、不整合発覚時は設計 doc 側を修正する判断も permission
- 「採否ご判断ください」「OK 待ち」「進めて or やめて」literal を AYA に投げない (= destructive action のみ「明示指示後実行」protocol)

---

## §6. 起案規律 (= α-4 内維持)

- **AYA literal「α-4 entry handoff 起案して」(= 2026-06-06) で本 doc 起案承認済**、本 doc 自体は本 session 内 commit 想定
- **AYA 立ち会い別 session で α-4 着手** (= 4.2-4.5 build + 起動 + live verify) = Claude 自走不可、4.1 (= codegen 単独 verify) のみ本 session or 別 session で Claude 自走可
- **AYA 指示 literal 完全一致 verify** = design/01:146 「85 GLSL UBO blueprint は discard しない」literal 整合維持 (= 案 X 確定の核)
- **`mUseUBO` runtime flag default OFF 維持** (= 原則 4 §4.4 O-2)
- **`#ifdef LL_VULKAN_GLSL` C++ 不使用** (= 原則 4 §4.4 O-3、GLSL 内のみ使用は OK)
- **視覚 regression ゼロ死守** (= 原則 4 §5.4、α-4 AYA live verify 必須)
- **3 OS 同一実装** (= 原則 OS-1〜OS-10、cmake + shader file + codegen tool は 3 OS 共通、α-4 primary Linux でも Win/Mac の build 影響範囲 record)
- **case X 確定 source of truth 維持** = blueprint dir = codegen 入力 source of truth、actual class*/ + cinematic_bd/ = AYAstorm shader runtime compile target、二重 source 同期 protocol = main.py `_verify_block_match` + `_verify_blueprint_actual_consistency` で formal化
- **commit は AYA literal 指示後のみ** (= memory `feedback_no_auto_commit`)
- **push / PR は AYA 側** (= memory `feedback_release_flow`)
- **Co-Authored-By Claude 行禁止** (= memory `feedback_no_claude_coauthor`)
- **destructive action は明示指示後実行** (= memory §12、reset/revert/push 等)

---

## §A. 関連 commit + doc

| 種別 | 内容 |
|---|---|
| **case X 確定実装 phase A-G 7 commit** | `df38b7c994` phase A revert / `f95182ded5` phase B blueprint README / `b06f860a77` phase C handoff §D.9 / `64eb589ee7` phase D 設計 doc 5 件 / `09ee5e8a8e` phase E blueprint 同期書換 / `868bc38cc9` phase F `_verify_blueprint_actual_consistency` / `d9c6e48579` phase G 波及 doc 全件 + grep verify |
| **case X 確定前保持 commit** | `b66ec99f72` α-2 main.py multi-input + `_verify_block_match` (= phase F baseline) / `64122994c1` Phase 2.α 案 Z 確定反映 doc (= 案 Z 履歴) / `db5cbcbc36` Phase 2.α 起案 + freeze record / `310d58b556` α-3 entry handoff 起案 / `887ddb5341`〜`e5f57d57ff` sub-session 5 step 2-batch-0-a 7 commit (= freeze 維持) |
| **case Z'+ revert 済 commit** (= 案 X で全件撤回) | `9c3b3f3d72` improvement 1.5.c / `a97b3e1b6b` improvement 1.5.b / `9abae83730` improvement 4 / `bfacb1f50f` improvement 1.5.a / `246535626e` improvement 2 / `862f9cb983` improvement 1 |
| 関連 doc (本 entry の主要参照先) | `handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` **§D.9** (= 案 X 確定 source of truth) + `handoff/phase2/alpha/handoff-phase2-alpha-3-cmake-blueprint-readme-propagation.md` **§3.1-§3.7 phase A-G** + `§4 Exit 条件 6 件` (= α-3 完了確認) |
| 関連 doc (Phase 2.L0 freeze) | `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-batch-0-a-entry.md` §C (= sub-session 5 freeze record + 続行点 resume protocol) + `handoff/phase2/audit/handoff-phase2-l0-1-C-step2-pre2-mapping.md` §5.1 (= step 2-pre2 80 slot alphabetical sort literal、α-4 codegen verify 時の expected 値 source) |
| 関連 source (α-4 verify 対象) | `indra/newview/app_settings/shaders/aya_r41_blueprints/` (= 94 .glsl、case X source of truth) + `indra/cmake/AyaUboCodegen.cmake` (= BLUEPRINT_DIR 復元状態) + `scripts/ubo_codegen/main.py` (= `_verify_blueprint_actual_consistency`) + `indra/llrender/llvkloader.cpp` (= host C++ pipeline layout 整合源) |
| 関連 memory | (本 doc §2.3 cross-ref) |
| 本 doc | α-4 着手 entry handoff、commit 候補 (= AYA literal 指示後のみ) |

---

## §B. context 引き継ぎ (= memory `feedback_proactive_handoff` 適用)

**Phase 2.α α-3 phase G 完了 commit `d9c6e48579` (= 案 X 全 7 phase 完走) + AYA literal「α-4 entry handoff 起案して」(= 2026-06-06) = 自然な session 境界**。次 session で **/clear → 本 handoff doc + 必読 3 件 cold read → α-4 sub-step 4.1 (= codegen 単独走行 verify) 着手** を推奨。

**本 session 引継ぎ事項** (= α-3 完了 + α-4 起案):
- 本 handoff doc commit 候補 (= AYA literal 指示後、本 session 内 commit 想定)
- 次 session 初手 AYA 確認 = なし (= AYA literal 既発令で着手承認済、memory `feedback_root_cause_no_shortcuts` §9)
- α-4 着手手順 = 4.1 codegen 単独 verify (= Claude 自走、本 session or 別 session) → 4.2-4.5 AYA 立ち会い別 session (= configure + build + cold launch + live verify) → 4.6 α-4 完了 record (= doc 起案 + AYA literal 承認後 commit)
- Phase 2.L0 sub-session 5 step 2-batch-0-a 7 commit (= `887ddb5341`〜`e5f57d57ff`) は **freeze 維持**、Phase 2.α α-4 完了で sub-session 5 続行点 resume
- memory 7 件 = 既存 `feedback_root_cause_no_shortcuts` (= §11 + §12 拡張 5 連続落ち 記録 + 自走精査網羅性 checklist + sandbox 実証 protocol + 「採否ご判断ください」禁忌 + subagent 結果盲信禁止 + 設計 doc literal 誤り疑念) + `feedback_design_doc_number_literal_verify` (= 構造属性 + codegen/build pipeline 入力 source path verify) + `feedback_doubt_self_first` §5 (= 解決策確定前に設計 doc 精査) + `feedback_proactive_risk_management` + `feedback_self_verify_before_handoff` + `feedback_one_step_at_a_time` + `feedback_build_only_verified` は本 session 起案済、次 session 開始時 MEMORY.md index で確認

**次 session 着手 1 手目**: §2.1 必読 3 件 cold read → §3 sub-step 4.1 (= codegen 単独走行 verify) 着手、各 sub-step 完了報告で AYA literal commit 指示受領後 commit、4.2-4.5 は AYA 立ち会い別 session で実施。

**Phase 2.α α-4 完了後の継続**:
- Phase 2.L0 sub-session 5 続行点 resume (= cold launch + AYA live verify、case X 確定後の sub-session 5 7 commit 改修分の実機 verify、blueprint + actual 二重 source 整合 verify formal化済 ゆえ独立 verify 完走可能)
- Phase 2.L0 残作業 sub-session 6 以降 (= MaterialUBO 49 file 単独 + 73 UBO 残作業) は case X 確定後の二重 source 同期 protocol formal化前提で進行 (= blueprint dir 改修 + actual class*/ + cinematic_bd/ 改修 + `_verify_blueprint_actual_consistency` で整合 verify)

---

## §C. α-4 sub-step 実施 evidence record

memory `feedback_design_doc_number_literal_verify` 適用 = 数値 literal 全件記載。memory `feedback_no_dual_doc_split` 整合 = 別 record 起案せず本 entry handoff 内に集約。

### §C.1 sub-step 4.1 = codegen 単独走行 verify (= 2026-06-06 完了、PASS verdict)

**実施日**: 2026-06-06 (= 案 X 確定実装 全 7 phase A-G 完了 commit `d9c6e48579` 直後)
**実施方法**: Claude 自走 (= memory `feedback_root_cause_no_shortcuts` §12 sandbox 実証 protocol 適用、subagent 不使用、grep + Read で直接 verify)
**Verdict**: ✅ **PASS** (= §4 Exit 条件 #1 全件充足)

#### §C.1.1 実行 command

```sh
python3 scripts/ubo_codegen/main.py \
  --input indra/newview/app_settings/shaders/aya_r41_blueprints \
  --output /tmp/aya_alpha4_verify \
  --cache-file /tmp/aya_alpha4_verify/codegen_state.json \
  --project-root . \
  --glslang-bin /usr/bin/glslangValidator \
  --spirv-cross-bin /usr/bin/spirv-cross \
  --verify-target-paths \
    indra/newview/app_settings/shaders/class1/deferred/cloudsF.glsl \
    indra/newview/app_settings/shaders/class1/deferred/cloudsV.glsl \
    indra/newview/app_settings/shaders/class1/deferred/postDeferredGammaCorrect.glsl \
    indra/newview/app_settings/shaders/class1/deferred/postDeferredTonemap.glsl \
    indra/newview/app_settings/shaders/class3/deferred/pointLightV.glsl \
    indra/newview/app_settings/shaders/class3/deferred/spotLightF.glsl \
    indra/newview/app_settings/shaders/class1/deferred/postDeferredF.glsl \
    indra/newview/app_settings/shaders/class1/deferred/postDeferredHQDoFF.glsl \
    indra/newview/app_settings/shaders/class3/deferred/waterHazeF.glsl \
    indra/newview/app_settings/shaders/class3/deferred/waterHazeV.glsl \
    indra/newview/app_settings/shaders/cinematic_bd/class1/deferred/shadowUtil.glsl \
    indra/newview/app_settings/shaders/class1/deferred/shadowUtil.glsl \
    indra/newview/app_settings/shaders/class1/environment/waterV.glsl \
    indra/newview/app_settings/shaders/class3/environment/waterF.glsl
```

#### §C.1.2 evidence 4 件

**Evidence 1**: blueprint dir 入力で 94 .glsl → 94 block / 386 member emit 成功 + parse error 0 件 (= §D.9.2 evidence 再現)

```
[codegen_ubo] INFO: 94 .glsl input(s) discovered
[codegen_ubo] INFO: emitted 99 file(s) for 94 block(s) / 386 member(s) in 10973 ms
```

99 file = 94 per-block layout .inl + 5 aggregated .inl (= `ubo_metadata.inl` + `ubo_perfect_hash.inl` + `ubo_index.inl` + `ubo_dummy_init.inl` + `ubo_host_loader.inl`)。

**Evidence 2**: `ubo_metadata.inl` の sub-session 5 改修済 7 UBO binding 整合 confirm (= alphabetical sort 80 slot literal 内、全件期待値一致)

| # | UBO 名 | 期待値 (= alphabetical slot) | `ubo_metadata.inl` 確認値 | verdict |
|---|---|---|---|---|
| 1 | CloudsVParamUBO_Legacy | set=1, binding=8 | `{ "CloudsVParamUBO_Legacy", 0x7d4955feu, 256u, 1u, 8u, 0u, 1u, 4u }` | ✅ |
| 2 | PerProgramUBO_GammaCorrect | set=1, binding=39 | `{ "PerProgramUBO_GammaCorrect", 0xf34eebc8u, 256u, 1u, 39u, 0u, 1u, 4u }` | ✅ |
| 3 | PerProgramUBO_PointLightV | set=1, binding=44 | `{ "PerProgramUBO_PointLightV", 0xebfee557u, 256u, 1u, 44u, 1u, 1u, 2u }` | ✅ |
| 4 | PerProgramUBO_PostDeferredF | set=1, binding=45 | `{ "PerProgramUBO_PostDeferredF", 0x8519e0b2u, 256u, 1u, 45u, 1u, 1u, 2u }` | ✅ |
| 5 | PerProgramUBO_WaterHazeV | set=1, binding=55 | `{ "PerProgramUBO_WaterHazeV", 0x341ff24cu, 256u, 1u, 55u, 1u, 1u, 4u }` | ✅ |
| 6 | ShadowUtilParamUBO_Legacy | set=1, binding=63 | `{ "ShadowUtilParamUBO_Legacy", 0x1c7a416cu, 512u, 1u, 63u, 1u, 1u, 12u }` | ✅ |
| 7 | WaterVParamUBO_Legacy | set=1, binding=79 | `{ "WaterVParamUBO_Legacy", 0x4d192f45u, 256u, 1u, 79u, 1u, 1u, 6u }` | ✅ |

⇒ phase E commit (= `09ee5e8a8e`) blueprint dir 7 file 同期書換が `ubo_metadata.inl` に正しく反映、alphabetical sort literal (= pre2 mapping §5.1) と完全一致。

**Evidence 3**: `--verify-target-paths` phase F 動作実機 confirm

```
[codegen_ubo] INFO: 二重 source verify: discovered 14 actual shader file(s)
[codegen_ubo] INFO: 二重 source verify: verified=31 / skipped=1 (parse error) / no_match=0
```

| 軸 | 期待値 (= §3 表 4.1) | 実測値 | verdict |
|---|---|---|---|
| discovered | 14 actual file | 14 | ✅ |
| verified | ≥ 7 | 31 | ✅ (= 14 file 内に複数 UBO declaration 含まれるため 7 を超過、UBO-level 単位 count) |
| skipped | ≤ 1 | 1 | ✅ |
| no_match | 0 | 0 | ✅ |

**Evidence 4**: skipped=1 = `class3/environment/waterF.glsl` の glslang -E parse error → warning + skip = phase F 設計上の期待動作

```
[codegen_ubo] WARNING: 二重 source verify skip (parse error): indra/newview/app_settings/shaders/class3/environment/waterF.glsl — [codegen_ubo] ERROR: glslang -E failed for indra/newview/app_settings/shaders/class3/environment/waterF.glsl
```

⇒ `waterF.glsl` は AYAstorm runtime `loadShaderFile()` prepend chain (= `#version` / `addPermutation` / `AYASTORM_*` 等) 不要前提では standalone parse 不可、phase F design 04 §4.4 の「parse 失敗時 warning + skip」behavior と整合 (= **CodegenError abort せず継続**)。case X 確定の核 = blueprint dir = self-contained codegen 入力 source of truth + actual = runtime compile target 別 GLSL 並列 build process、本 evidence は両者の正当な共存を実機で confirm。

#### §C.1.3 §4 Exit 条件 #1 全件充足 verdict

| 項目 (= §4 #1) | 充足判定 |
|---|---|
| blueprint dir 入力で 94 .glsl → 94 UBO emit | ✅ (Evidence 1) |
| parse error 0 件 | ✅ (Evidence 1、blueprint 側 parse error 0) |
| `ubo_metadata.inl` 80 UBO binding 整合 confirm | ✅ (Evidence 2、改修済 7/80 全件 alphabetical sort literal 一致 = 残 73 UBO は base state set=3 binding 維持で未改修と整合) |
| `--verify-target-paths` で sub-session 5 改修 7 UBO 対応 actual 整合 | ✅ (Evidence 3、verified=31 ≥ 7 + no_match=0、Evidence 4 で skipped=1 期待動作 confirm) |

#### §C.1.4 次手

- 4.1 commit (= 本 record 同梱) は AYA literal「commit して進めて」(= 2026-06-06) 受領で本 doc 改修を commit
- 4.2 (= AYA 環境 configure 走行) は **AYA 立ち会い別 session** で実施 (= Claude 自走不可、Linux primary、`develop.py configure` 走行 + STATUS message で blueprint dir 検出 + GLSL source count 94 確認)
- 本 session は 4.1 record commit で完了、次 session 開始時 4.2 着手

### §C.2 sub-step 4.2 = configure 走行 verify (= 2026-06-06 完了、PASS verdict)

**実施日**: 2026-06-06 (= 4.1 commit `2f5f8dc961` 直後、別 session で 4.2 着手)
**実施方法**: Claude 自走 (= Linux primary、`autobuild configure` 走行 + log 解析、memory `feedback_log_reading` 適用で Claude が直接 log 読み、memory `feedback_root_cause_no_shortcuts` §12 sandbox 実証 protocol 適用)
**Verdict**: ✅ **PASS** (= §4 Exit 条件 #2 第一部 (= configure 部分) 全件充足)

#### §C.2.1 実行 command

```sh
cd ~/work_firestorm/phoenix-firestorm
source .venv/bin/activate
export AUTOBUILD_VARIABLES_FILE=$HOME/work_firestorm/fs-build-variables/variables
autobuild configure -A 64 -c ReleaseFS_open -- \
  --fmodstudio \
  -DLL_TESTS:BOOL=FALSE \
  -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE \
  --package \
  --chan AYAstorm-release \
  > /tmp/aya_alpha4_4_2_configure.log 2>&1
```

実行 log = `/tmp/aya_alpha4_4_2_configure.log` (= 58 行)。

#### §C.2.2 evidence 4 件

**Evidence 1**: AyaUboCodegen.cmake STATUS message 5 件全件出力 (= line 44-48)

```
-- AYAstorm r41 (PA-7): UBO Codegen wired = /home/ishikawa/work_firestorm/phoenix-firestorm/indra/../scripts/ubo_codegen/main.py
-- AYAstorm r41 (PA-7):   blueprint dir   = /home/ishikawa/work_firestorm/phoenix-firestorm/indra/newview/app_settings/shaders/aya_r41_blueprints
-- AYAstorm r41 (PA-7):   output dir      = /home/ishikawa/work_firestorm/phoenix-firestorm/build-linux-x86_64/codegen/ubo
-- AYAstorm r41 (PA-7):   cache file      = /home/ishikawa/work_firestorm/phoenix-firestorm/build-linux-x86_64/codegen/cache/codegen_state.json
-- AYAstorm r41 (PA-7):   blueprint count = 94 .glsl files
```

| 軸 | 期待値 (= §3 表 4.2 row) | 実測値 | verdict |
|---|---|---|---|
| AyaUboCodegen.cmake 走行 | 正常走行 (= 5 STATUS 全件出力) | 5 STATUS 全件出力 | ✅ |
| blueprint dir 検出 | `aya_r41_blueprints` literal 一致 | `.../aya_r41_blueprints` | ✅ |
| GLSL source count | 94 (= blueprint dir) | **94 .glsl files** | ✅ (= case X 確定 source of truth literal 一致) |
| output dir 設定 | `build-linux-x86_64/codegen/ubo` | 同左 | ✅ |
| cache file 設定 | `codegen_state.json` 配下 | `codegen/cache/codegen_state.json` | ✅ |

**Evidence 2**: glslangValidator 検出 confirm (= line 43、`AyaShaderCompile.cmake` 由来の find_program、`AyaUboCodegen.cmake` 内 `AYA_GLSLANG_VALIDATOR` 再利用判定で reuse path)

```
-- AYAstorm r41: glslangValidator = /usr/bin/glslangValidator
```

⇒ `AyaUboCodegen.cmake` §27-29 の reuse path 動作 confirm、`find_program(AYA_GLSLANG_VALIDATOR glslangValidator)` 結果が build phase の codegen_ubo target 走行に渡される。

**Evidence 3**: error 0 件 + warning は autobuild --id 1 件のみ (= codegen / build 無関係)

```
Warning: no --id argument or AUTOBUILD_BUILD_ID environment variable specified;
    using a value from the UTC date and time (261570900), which may not be unique
```

| 軸 | 期待値 | 実測値 | verdict |
|---|---|---|---|
| CMake Error | 0 件 | 0 件 | ✅ |
| FATAL_ERROR | 0 件 | 0 件 | ✅ |
| fatal error | 0 件 | 0 件 | ✅ |
| error: | 0 件 | 0 件 | ✅ |
| AyaUboCodegen.cmake WARNING (= glslang/spirv-cross 未検出) | 0 件 | 0 件 (= 両 tool 検出済) | ✅ |
| autobuild --id warning | 1 件 (= 期待動作、build_id 未指定で UTC date 自動生成、codegen 無関係) | 1 件 | ✅ (= 設計通り) |

**Evidence 4**: configure exit code 0 + CMake 完了 4 行 (= line 54-58)

```
-- Configuring done (2.6s)
-- Generating done (0.1s)
-- Build files have been written to: /home/ishikawa/work_firestorm/phoenix-firestorm/build-linux-x86_64
finished
EXIT=0
```

#### §C.2.3 §4 Exit 条件 #2 第一部 (= configure) 全件充足 verdict

| 項目 (= §4 #2 configure 部分) | 充足判定 |
|---|---|
| configure 走行 error 0 件 | ✅ (Evidence 3、CMake Error / FATAL_ERROR / fatal error / error: 全件 no match) |
| AyaUboCodegen.cmake 正しく走行 | ✅ (Evidence 1、STATUS 5 件全件出力 + STATUS 順序整合) |
| blueprint dir 検出 confirm | ✅ (Evidence 1、`aya_r41_blueprints` literal 一致) |
| GLSL source count = 94 | ✅ (Evidence 1、`blueprint count = 94 .glsl files` literal 一致 = case X 確定 source of truth) |
| codegen_ubo target 走行成功 | △ (= configure phase では target **定義** のみ完了、実走行は 4.3 build phase = `make codegen_ubo` で trigger) |

⇒ §4 Exit 条件 #2 は configure (= 4.2) + build (= 4.3) 両 phase 完走で完全充足、本 sub-step (= 4.2 configure) では configure 関連 4 項目全件 PASS + codegen_ubo target は **定義成功** で build phase 走行準備完了。

#### §C.2.4 副次的 verify

build dir 既存 (= 過去の build run 由来) artifact 状態 confirm:

- `build-linux-x86_64/codegen/ubo/` = 既存 file 群存在 (= 前 build run 由来、4.3 build phase で再走行時の incremental cache 検査用)
- `build-linux-x86_64/codegen/cache/codegen_state.json` = 既存 cache file 存在
- configure phase で `file(MAKE_DIRECTORY ...)` (= AyaUboCodegen.cmake line 77-78) 実行済 = output dir + cache dir 存在 confirm

#### §C.2.5 次手

- 4.2 commit (= 本 record 同梱、§3 表 4.2 row update + §C.2 追記) は AYA literal「commit して進めて」(= 2026-06-06) 受領で本 doc 改修を commit ⇒ commit `35393dc5ff` 完了
- 4.3 (= build 走行 verify) は AYA literal「commit して進めて」(= 2026-06-06) 連結指示で 4.3 着手 ⇒ §C.3 evidence record 起案
- 本 session は 4.2 record commit + 4.3 record commit 連結進行

### §C.3 sub-step 4.3 = build 走行 verify (= 2026-06-06 完了、PASS verdict)

**実施日**: 2026-06-06 (= 4.2 commit `35393dc5ff` 直後、AYA literal「commit して進めて」連結指示で 4.3 着手)
**実施方法**: Claude 自走 (= Linux primary、`autobuild build` 走行 + log 解析、memory `feedback_log_reading` 適用で Claude が直接 log 読み、memory `feedback_root_cause_no_shortcuts` §12 sandbox 実証 protocol 適用)
**Verdict**: ✅ **PASS** (= §4 Exit 条件 #2 第二部 (= build 部分) 全件充足)

#### §C.3.1 実行 command

```sh
cd ~/work_firestorm/phoenix-firestorm
source .venv/bin/activate
export AUTOBUILD_VARIABLES_FILE=$HOME/work_firestorm/fs-build-variables/variables
autobuild build -A 64 -c ReleaseFS_open --no-configure \
  > /tmp/aya_alpha4_4_3_build.log 2>&1
```

実行 log = `/tmp/aya_alpha4_4_3_build.log` (= 1360 行)。

#### §C.3.2 evidence 6 件

**Evidence 1**: codegen_ubo target 走行成功 (= line 52-73)

```
[codegen_ubo] INFO: tool=ubo_codegen version=0.1.0-PA6 python=3.12.3
[codegen_ubo] INFO: input=[PosixPath('.../aya_r41_blueprints')] output=.../build-linux-x86_64/codegen/ubo
[codegen_ubo] INFO: 94 .glsl input(s) discovered
[codegen_ubo] INFO: cache miss: codegen main script sha256 changed
[codegen_ubo] INFO: emitted 99 file(s) for 94 block(s) / 386 member(s) in 11008 ms
[codegen_ubo] INFO: cache written: .../build-linux-x86_64/codegen/cache/codegen_state.json
[ 26%] Built target codegen_ubo
```

| 軸 | 期待値 | 実測値 | verdict |
|---|---|---|---|
| codegen tool version | 0.1.0-PA6 (= phase F 適用済) | 0.1.0-PA6 | ✅ |
| input source | `aya_r41_blueprints` (= case X 確定 source of truth) | 同左 | ✅ |
| .glsl input 数 | 94 | 94 | ✅ (= case X 確定 source of truth literal 一致) |
| cache miss → 再走行 | 期待 (= main.py sha256 変更で invalidate) | cache miss → 再走行成功 | ✅ |
| emit 数 | 99 file (= 94 per-block layout + 5 aggregated) | 99 | ✅ |
| block 数 | 94 | 94 | ✅ |
| member 数 | 386 | 386 | ✅ (= 4.1 record §C.1.2 Evidence 1 と完全一致) |
| Built target codegen_ubo | 成功 | `[ 26%] Built target codegen_ubo` | ✅ |

**Evidence 2**: build artifact `build-linux-x86_64/codegen/ubo/` 配下 99 .inl file 生成確認

```
$ ls build-linux-x86_64/codegen/ubo/*.inl | wc -l
99
```

内訳:
- 5 aggregated .inl: `ubo_dummy_init.inl` + `ubo_host_loader.inl` + `ubo_index.inl` + `ubo_metadata.inl` (= 既存 verify) + `ubo_perfect_hash.inl`
- 94 per-block layout .inl: `ubo_layout_<name>.inl` (= blueprint 94 .glsl の 1:1 対応)

**Evidence 3**: `ubo_metadata.inl` 内 sub-session 5 改修済 7 UBO binding 整合 confirm (= 4.1 record §C.1.2 Evidence 2 の再現性 verify、build phase で同一 binding 出力)

| # | UBO 名 | 期待値 (= alphabetical slot) | `build-linux-x86_64/.../ubo_metadata.inl` 確認値 | verdict |
|---|---|---|---|---|
| 1 | CloudsVParamUBO_Legacy | set=1, binding=8 | line 36: `{ "CloudsVParamUBO_Legacy", 0x7d4955feu, 256u, 1u, 8u, 0u, 1u, 4u }` | ✅ |
| 2 | PerProgramUBO_GammaCorrect | set=1, binding=39 | line 78: `{ "PerProgramUBO_GammaCorrect", 0xf34eebc8u, 256u, 1u, 39u, 0u, 1u, 4u }` | ✅ |
| 3 | PerProgramUBO_PointLightV | set=1, binding=44 | line 83: `{ "PerProgramUBO_PointLightV", 0xebfee557u, 256u, 1u, 44u, 1u, 1u, 2u }` | ✅ |
| 4 | PerProgramUBO_PostDeferredF | set=1, binding=45 | line 84: `{ "PerProgramUBO_PostDeferredF", 0x8519e0b2u, 256u, 1u, 45u, 1u, 1u, 2u }` | ✅ |
| 5 | PerProgramUBO_WaterHazeV | set=1, binding=55 | line 94: `{ "PerProgramUBO_WaterHazeV", 0x341ff24cu, 256u, 1u, 55u, 1u, 1u, 4u }` | ✅ |
| 6 | ShadowUtilParamUBO_Legacy | set=1, binding=63 | line 102: `{ "ShadowUtilParamUBO_Legacy", 0x1c7a416cu, 512u, 1u, 63u, 1u, 1u, 12u }` | ✅ |
| 7 | WaterVParamUBO_Legacy | set=1, binding=79 | line 119: `{ "WaterVParamUBO_Legacy", 0x4d192f45u, 256u, 1u, 79u, 1u, 1u, 6u }` | ✅ |

⇒ /tmp 独立 verify (= 4.1) と build phase 出力 (= 4.3) で binding 値完全一致 = codegen tool deterministic 動作 confirm。

**Evidence 4**: viewer binary 生成 OK (= `ayastorm-bin` target、AYAstorm release rename 後)

```
[100%] Linking CXX executable ayastorm-bin
[100%] Built target ayastorm-bin
```

packaged/bin/ 配下:
- `do-not-directly-run-ayastorm-bin` (= viewer binary、wrapper script 経由起動 design)
- `SLPlugin` / `SLVoice` / `dullahan_host` / `chrome-sandbox` / `linux-crash-logger.bin` 等同梱

**Evidence 5**: package tar.xz 生成 (= 206 MB)

```
[100%] Generating AYAstorm-x86_64-7.2.4.261570900.tar.xz
[100%] Performing viewer_manifest copy
[100%] Built target copy_l_viewer_manifest
[100%] Built target llpackage
================ Created base package Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261570900.tar.xz
```

- channel: `Firestorm-AYAstorm-release`
- version: 7.2.4.261570900 (= revision 261570900、UTC date base autobuild id)
- tar.xz size: 206,037,388 bytes (= 206 MB)
- path: `build-linux-x86_64/newview/Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261570900.tar.xz`

**Evidence 6**: error 0 件 + build EXIT=0

```
[100%] Built target llpackage
finished
EXIT=0
```

| 軸 | 期待値 | 実測値 | verdict |
|---|---|---|---|
| error: / fatal error / FATAL_ERROR / CMake Error | 0 件 | 0 件 (= grep count 0) | ✅ |
| build EXIT code | 0 | 0 | ✅ |
| `Built target llpackage` (= 最終 target) | 成功 | `[100%] Built target llpackage` | ✅ |

#### §C.3.3 §4 Exit 条件 #2 第二部 (= build) 全件充足 verdict

| 項目 (= §4 #2 build 部分) | 充足判定 |
|---|---|
| make build success | ✅ (Evidence 6、EXIT=0 + `[100%] Built target llpackage`) |
| codegen_ubo target 走行成功 | ✅ (Evidence 1、`[ 26%] Built target codegen_ubo` + 94 .glsl → 99 file emit) |
| build artifact `ubo_metadata.inl` 生成確認 | ✅ (Evidence 2 + Evidence 3、99 .inl file + 7 UBO binding 整合 alphabetical sort literal 一致) |
| viewer binary 生成 OK | ✅ (Evidence 4、`[100%] Built target ayastorm-bin` + `do-not-directly-run-ayastorm-bin` 配置) |

⇒ §4 Exit 条件 #2 完全充足 (= configure (= 4.2 §C.2) + build (= 4.3 §C.3) 両 phase PASS)。

#### §C.3.4 副次的 verify

- codegen tool `version=0.1.0-PA6` (= phase F 適用済 version) 走行 confirm = phase F commit `868bc38cc9` `_verify_blueprint_actual_consistency` 追加が build phase で正しく load されている
- cache miss → 再走行成功 = AyaUboCodegen.cmake DEPENDS (= `CONFIGURE_DEPENDS` の `*.glsl` 全件 + `${AYA_UBO_CODEGEN_MODULES}` Python module 全件) で main.py sha256 変更を検知、§12.5.7 cache hit 時 touch path とは別の cache miss path で 11008 ms 再走行
- glslangValidator 検出済 (= configure phase §C.2 Evidence 2) で codegen 内 SPIR-V cross-check 実施

#### §C.3.5 次手

- 4.3 commit (= 本 record 同梱、§3 表 4.3 row update + §C.3 追記) は AYA literal「commit して進めて」(= 2026-06-06) 受領で本 doc 改修を commit
- 4.4 (= cold launch verify) は AYA 環境で install + 起動 + Vulkan validation log 0 件 + sub-session 5 改修 7 UBO binding 反映 runtime confirm + crash / regression 0 件 = build flow 続行 (= `rm -rf ~/ayastorm/` + `install.sh` + `rm -rf ~/.ayastorm_x64/cache/` + viewer 起動) は AYA literal 指示後実施
- memory `project_build_procedure` の build flow 残り (= `cd build-linux-x86_64/newview/packaged` + `rm -rf ~/ayastorm/` + `rm -rf ~/.local/share/applications/ayastorm-viewer.desktop` + `./install.sh` + `rm -rf ~/.ayastorm_x64/cache/`) は 4.4 cold launch 着手と同義 = AYA literal「4.4 進めて」literal or「install して起動して」literal 受領後実施
