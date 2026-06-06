# handoff = r41 Phase 2.L0 sub-session 3 = L0-1.C 実装着手 entry

**起案日**: 2026-06-06
**位置付け**: Phase 2.L0 sub-session 3 (= L0-1.C 実装) 着手 entry handoff。本 sub-session で **design-phase 規律解除点** = `indra/` 配下改変開始 (= memory `feedback_design_phase_no_code_write` 解除)。
**起案契機**: sub-session 2 (= L0-1.B trace) Exit AYA literal 受領「1 OK / 2 A / 3 OK」record 2026-06-06。
**起案規律**:
- memory `feedback_proactive_handoff` 適用 (= 周回境界 = 自然な session 境界、AYA 指示待たず handoff 起案)
- memory `feedback_handoff_minimal_pre_req_read` 適用 (= 最低限 3 件 + pinpoint)
- memory `feedback_ubo_migration_one_at_a_time` 適用 (= 大塊一括 reject、cold launch 検証挟む)
- memory `feedback_build_only_verified` 適用 (= 効果未確認 commit を積まない)

---

## §1. 着手目的 (= sub-session 2 結論受領内容)

### §1.1 AYA literal 受領内容 (= 2026-06-06)

| # | sub-session 2 §6 確認 | AYA literal 回答 | 含意 |
|---|---|---|---|
| 1 | L0-1 protocol-C 採用案 | **(i) 新規 binding allocation** | Legacy UBO 88 件 PerProgram cluster を実 pipeline layout の set=1a/1b (= V3A_PROGRAM_SET_A/B_BINDINGS=40+40) に再配置 |
| 2 | sub-session 2 結論パターン | **(A) sub-session 3 着手 OK** | protocol-A + D 整合 OK ゆえ host dispatch logic 新規実装不要、protocol-B 整合 NG 部分は L0-1.C 内で codegen pipeline 修正で対応 |
| 3 | trace 範囲補足 verify 3 件持越 | **OK** | SPIR-V compile 衝突実 behaviour + 残 8 件 binding allocation + ubo_metadata.inl `subset` 列使用有無 = 本 sub-session 3 内で逐次解消 |

### §1.2 sub-session 3 着手目的

**name-based dispatch logic 実装**:
- ubo_metadata.inl set/binding 値修正 = Legacy UBO 88 件の set/binding 値を実 pipeline layout 配置 (= set=1a/1b binding=0..39) に整合化
- codegen pipeline (= `scripts/ubo_codegen/perfect_hash.py`) 連動修正 = ubo_metadata.inl 出力と shader 側 layout 宣言の自動連動
- shader 側 GLSL `layout(set=N, binding=M)` 宣言整合化 = Legacy UBO 全 file の `#ifdef LL_VULKAN_GLSL` block 内 set/binding 値書換
- cold launch validation = Linux validation layer warnings 0 件 + visual regression ゼロ + AYA live verify

---

## §2. 実装 scope (= `indra/` 改変開始)

### §2.1 大塊一括 default reject (= memory `feedback_ubo_migration_one_at_a_time`)

L0-1.C 実装は **複数 step に分割、各 step 完了で cold launch 検証挟む**。1 session で完走想定せず、step 境界で必要なら handoff 切替。

### §2.2 想定 step 構成 (= 推定、AYA literal 確認 candidate)

| step | 内容 | `indra/` 改変 | cold launch |
|---|---|---|---|
| **§2.2.1 step 1** | codegen pipeline 側 set/binding 割当 logic 修正起案 + AYA literal 採用案確認 | `scripts/ubo_codegen/` 配下のみ (= `indra/` 改変ゼロ) | 不要 |
| **§2.2.2 step 2** | codegen 再実行 + ubo_metadata.inl set/binding 値更新 verify | `build-linux-x86_64/codegen/ubo/` generated file 更新 | build only (= visual regression なし) |
| **§2.2.3 step 3** | shader 側 GLSL Legacy UBO 全 file `layout(set=N, binding=M)` 宣言整合化 | `indra/newview/app_settings/shaders/` 配下 | shader compile + Linux validation 0 件 |
| **§2.2.4 step 4** | host C++ Legacy UBO register 配線追加 (= mUseUBO=true 時 dispatch 経路接続) | `indra/llrender/` 配下 | mUseUBO 既存 cvar gate、default OFF 維持 |
| **§2.2.5 step 5** | cold launch validation + visual regression ゼロ verify | - | AYA live verify (= 視覚 regression なし) |
| **§2.2.6 step 6** | commit + Phase 2.L0 sub-session 3 Exit handoff 起案 | - | - |

**1 step 1 session 想定** = 6 session 想定 (= sub-session 2 entry handoff §2.2 §4-12 cross-ref、L0-1.C scope 内 6 step に細分化)。

### §2.3 verify 3 件持越 (= sub-session 2 §6 確認 3 受領)

本 sub-session 3 内で逐次解消:
1. **SPIR-V compile 時の set=3 binding 衝突実際の挙動**: step 3 (= shader 整合化) 時に Linux validation layer warnings 確認
2. **88 件 PerProgram cluster の set=1a/1b 残 8 件追加 binding allocation 詳細**: step 1 (= codegen 起案) 時に AYA literal 確認 candidate
3. **ubo_metadata.inl `subset` 列 (= 全件 0u) の dispatch 経路使用有無**: step 1 (= codegen 起案) 時に grep verify

---

## §3. 必読 doc (= memory `feedback_handoff_minimal_pre_req_read` 適用、最低限 3 件 + pinpoint)

### §3.1 最低限 3 件

1. **本 handoff doc** = sub-session 3 着手 entry、AYA literal 受領内容反映
2. **`handoff/phase2/audit/handoff-phase2-l0-1-B-dispatch-trace.md`** = sub-session 2 出力、§5 protocol 整合判定 + §6 AYA review candidate + §8 sub-session 順序
3. **`design/ubo/WORK_ORDER.md` §2.1** = L0-1 protocol-A/B/C/D 詳細 + §2.5 AYA review check list

### §3.2 必要時 pinpoint Read 候補

- **`build-linux-x86_64/codegen/ubo/ubo_metadata.inl`** = literal 88 件 Legacy UBO set/binding 値 (= step 1 時)
- **`scripts/ubo_codegen/perfect_hash.py`** = codegen pipeline logic (= step 1/2 時)
- **`indra/llrender/llvkloader.cpp:858-868`** = V3A_*_BINDINGS literal (= step 4 時)
- **`indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl:85-101`** 等 Legacy UBO `LL_VULKAN_GLSL` block 起源 (= step 3 時)

### §3.3 memory pinpoint

- `project_r41_phase1b_vulkan_host_gate` = `mUseUBO` runtime flag、`LL_VULKAN_GLSL` C++ 不使用
- `project_r41_phase2_4_principles` = 4 原則 + Phase 2.L0 Exit 条件 + 視覚 regression ゼロ
- `project_r41_design_principles` = 2 大設計原則 (call site API 温存 + Core 分散)
- `feedback_admit_unknown` / `feedback_doubt_self_first`
- `feedback_ubo_migration_one_at_a_time` / `feedback_proactive_risk_management`
- `feedback_build_only_verified` / `feedback_self_verify_before_handoff`
- `feedback_release_flow` (= push / PR は AYA、Claude はローカル commit まで)
- `feedback_no_claude_coauthor` (= commit message Claude 共著行禁止)

---

## §4. sub-session 3 Exit 条件 (= entry handoff §5.3 + sub-session 2 doc §7 cross-ref)

| # | Exit 項目 | 判定基準 |
|---|---|---|
| 1 | codegen pipeline 修正完了 | ubo_metadata.inl Legacy UBO set/binding 値が実 pipeline layout (= set=1a/1b binding=0..39) 整合 |
| 2 | shader 側 layout 宣言整合化完了 | Legacy UBO 全 file `#ifdef LL_VULKAN_GLSL` block 内 set/binding 値整合 |
| 3 | host C++ register 配線追加 | `mUseUBO=true` 時に Legacy UBO の `sProgramUboDirty` entry 登録、`writeProgramUbo` 経路 dispatch 通電 |
| 4 | cold launch validation | Linux validation layer warnings 0 件 |
| 5 | 既存通電 UBO regression なし | pilot 通電 5 UBO の動作変化なし |
| 6 | visual regression ゼロ verify | AYA live verify (= 視覚 regression なし) |
| 7 | commit | L0-1 実装 commit (= AYA literal 指示後のみ) |
| 8 | AYA literal Exit 承認 | sub-session 4 (= L0-2.A 再精査) 着手承認 |

---

## §5. 手戻り protocol (= entry handoff §2.3 cross-ref)

- **C → B 戻り** = 実装中 cold launch reject → 整合再確認 (= sub-session 2 doc §3-5 再 trace)
- **C → A 戻り** = cold launch reject 原因が設計案破綻 → 設計大幅変更 (= sub-session 1 戻り + WORK_ORDER §2.1 部分書き直し)

手戻り発生は **失敗ではなく cycle の正常動作** (= memory `feedback_falsification_as_progress`)。

---

## §6. 起案規律 (= sub-session 3 内維持)

- **大塊一括 default reject** = step 境界で必ず cold launch 検証挟む、複数 step 1 session で完走想定しない
- **AYA literal「進められるところまで進めて」自走承認継続中でも、step 境界は守る**
- **`mUseUBO` runtime flag default OFF 維持** (= 原則 4 §4.4 O-2)
- **`#ifdef LL_VULKAN_GLSL` C++ 不使用** (= 原則 4 §4.4 O-3、memory `project_r41_phase1b_vulkan_host_gate`)
- **視覚 regression ゼロ死守** (= 原則 4 §5.4)
- **3 OS 同一実装** (= 原則 OS-1〜OS-10)
- **推奨案 OK 自走承認継続、ただし AYA literal 確認 candidate 省略しない**

---

## §7. AYA 確認 (= 初手)

**最初の AYA 確認**: 「上記 entry handoff 確認、Phase 2.L0 sub-session 3 = L0-1.C 実装 sub-session で着手 OK か?」

**step 1 着手内容** (= AYA literal「OK」受領後):
- codegen pipeline (`scripts/ubo_codegen/perfect_hash.py` 等) cold read
- Legacy UBO 88 件の set/binding 現状値 grep + 実 pipeline layout (= set=1a/1b) への割当 plan 起案
- AYA literal 確認 candidate (= 88 件中 80 件は割当可能、残 8 件追加 binding allocation strategy) 提示

---

## §A. 関連 commit + doc

| 種別 | 内容 |
|---|---|
| 関連 doc (sub-session 2 出力) | `handoff/phase2/audit/handoff-phase2-l0-1-B-dispatch-trace.md` (= 本 sub-session 3 入口) |
| 関連 doc (sub-session 1 出力) | `handoff/phase2/audit/handoff-phase2-l0-uncertainty-audit.md` (= commit `cecb55ceb1`) |
| 関連 doc (3 段階 cycle entry) | `handoff/phase2/handoff-phase2-l0-entry.md` (= commit `0bc409461d`、§5 sub-session 3 詳細 scope) |
| 関連 doc (設計) | `design/ubo/WORK_ORDER.md` §2.1 + §2.5 + §4 4 原則 gate |
| 関連 source | (sub-session 2 doc §B cross-ref) |
| 関連 memory | (本 doc §3.3 cross-ref) |
| 本 doc | sub-session 3 着手 entry handoff、commit 候補 (= sub-session 2 出力 + 本 doc セット commit、AYA literal 指示後のみ) |

---

## §B. context 引き継ぎ (= memory `feedback_proactive_handoff` 適用)

**sub-session 2 完了 = 自然な session 境界**。次 session で **/clear → 本 handoff doc + 必読 3 件 cold read → sub-session 3 step 1 着手** を推奨。

**本 session 引継ぎ事項**:
- sub-session 2 出力 doc (= `handoff-phase2-l0-1-B-dispatch-trace.md`) + 本 handoff doc (= `handoff-phase2-l0-1-C-entry.md`) の 2 件 commit (= AYA literal 指示後のみ)
- push / PR は AYA 側 (= memory `feedback_release_flow`)
- 次 session 初手 AYA 確認 = §7 内容
