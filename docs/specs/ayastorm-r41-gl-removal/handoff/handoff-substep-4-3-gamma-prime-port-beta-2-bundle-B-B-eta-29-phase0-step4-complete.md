# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 0 Step 4 complete

**作成**: 2026-06-03
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase0-step2-complete.md` (= η-29 Phase 0 Step 2 = LL_INFOS hook 配線実装 完了 / Step 3 = AYA Linux 実機計測 session は Claude 出番なし phase で handoff doc 不在)
**branch**: `feature/ayastorm-r41-gl-removal`
**最新 commit (本 handoff 時点)**: `c3e64095fe` (= Phase 0 Step 4 (d) = 06a-prep 副産物 2 件 spec 訂正、`indra/` 改変ゼロ厳守継続)

---

## §0 state 一行 summary

η-29 **Phase 0 Step 4 = AYA 実機計測 log 解析 + §5 反映 + 副産物 2 件 spec 訂正** 完了 (= 2 commit = `6cbfbb28fa` §5.5 観察 7 sub-section fill + `c3e64095fe` 副産物 2 件 + cascading count 統一、`indra/` 改変ゼロ厳守継続、Phase 0 計測 phase 結果反映完了) → 次は **Step 5 = 計測 hook 除去 phase** (= `indra/` 再解禁、`c27733ae79` 機械的 revert で `llglslshader.cpp` + `00-Common.cmake` から hook 痕跡を全削除、flag OFF build PASS 確認、commit、`06a-prep §2.8` protocol 準拠)。Claude 即着手対象 = Step 5。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。最初の session 入りでは **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read (offset/limit) する。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-29-phase0-step4-complete.md` | 全文 | 本 handoff (= 現 state + Step 5 hook 除去 phase 着手地点 + Step 4 成果 summary) |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md` | §2.8 (検証完了後の除去 protocol、5 step) / §5.5 (= 本 session で fill した 7 sub-section、Phase 1.A 入口資料) | Step 5 削除手順 spec + Step 4 観察結果 review |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §14.3 (Stage 1 self-check 9 項目、Phase 0 exit / Phase 1.A entry 境界) | Phase 0 完了判定 + 次 phase entry condition 確認 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp` | 上部 helper block (`#ifdef AYASTORM_UBO_CADENCE_HOOK` ~ `#endif` 直後の macro 定義、line 番号は HEAD 時点で grep 再確認) / 31 setter body 先頭 `AYA_UBO_HOOK_IDX` / `_HASH` 行 / `c27733ae79` 全 diff = 除去対象 |
| `indra/cmake/00-Common.cmake` | 末尾 `option(AYASTORM_UBO_CADENCE_HOOK ...)` block (= 9 line) / `c27733ae79` 全 diff = 除去対象 |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md` | §2.8 (除去 protocol 5 step) / §5.5.5 (= dead 121 件、`aya_*` 3 件 chapter 10 残課題追記候補) / §5.5.6 (= hashed-path 40 件、計測対象 inventory 358 件確定) / §5.5.7 (= matrix 系 4-5 件 per-program → per-draw 補正必要、chapter 05 §7.3 補正は chapter 06b 起案直前) |
| `docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md` | §1.5 Q26-MUL / Q27-CONFL / 新規 candidate (= §5.5.5 「`aya_*` 3 件 dead」 + §5.5.7 「matrix 系 cadence 補正」を chapter 10 残課題に追記するか判断、AYA 判断対象) |

---

## §2 Phase 0 Step 4 = 完了成果 (本 session で済んだこと)

### §2.1 commit log

```
c3e64095fe docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 0 Step 4 (d) = 06a-prep 副産物 2 件 spec 訂正 = §2.3.1 #include "llappviewer.h" → extern U32 gFrameCount; (= layering 違反) + §2.2.2 「13 method」 → 「14 method」 typo + cascading count 統一 (§2.2 / §2.3.2 / §2.8 で「30 setter/method」→「31」、5 箇所一括補正) / git diff 1 file +12/-9 / indra/ 改変ゼロ厳守継続
6cbfbb28fa docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 0 Step 4 (b)+(c) = AYA 実機計測 log 解析結果 + 06a-prep §5.5 観察項目 7 sub-section 反映 = §5.5.1 計測条件 (2 run 分割 + 各 run 前 cache clear、AYA 指示で §5.1.2 単発案から逸脱) / §5.5.2 全体統計 (s2 922 frame 16.2M event 230 uniform 145 shader / s3 740 frame 19.7M event 231 uniform 135 shader / union 237 / 定常域 rolling window 平滑化で s2 100-915 / s3 100-735) / §5.5.3 cadence band 観察 vs §0.2 推定 delta / §5.5.4 scene-scaling (s3/s2 比 ≥ 2.5x = minimum_alpha 2.85x / shadow_target_width 4.60x / env_intensity 4.74x / emissive_brightness 4.38x / sun_up_factor 3.40x) / §5.5.5 reserved 318 中 dead 121 件 (= terrain detail_* 20 + post-effect ~15 + atmospheric 2 + aya_* 独自 3 件要追跡 + その他 ~80) / §5.5.6 hashed-path 40 件 (= 計測対象 inventory 358 件確定) / §5.5.7 per-program ↔ per-draw 境界 verify (matrix 系 4-5 件補正必要) / git diff 1 file +109 / indra/ 改変ゼロ厳守継続
```

### §2.2 成果 (= literal scope 1 file editing、2 commit に分離)

| # | commit | 編集対象 | 内容 | 結果 |
|---|---|---|---|---|
| 1 | `6cbfbb28fa` | `06a-prep-phase0-measurement.md` §5.5 新設 | 7 sub-section literal 全件 fill (= §5.5.1 計測条件 / §5.5.2 全体統計 / §5.5.3 cadence band 観察 vs 推定 delta / §5.5.4 scene-scaling / §5.5.5 dead 121 件 / §5.5.6 hashed-path 40 件 / §5.5.7 per-program ↔ per-draw 境界 verify) | +109 line |
| 2 | `c3e64095fe` | 同上 §2.2 / §2.2.2 / §2.3.1 / §2.3.2 / §2.3.3 / §2.7 / §2.8 / §7 | 副産物 2 件 (= §2.3.1 #include → extern + §2.2.2 13→14) + cascading count 統一 (= §2.2 / §2.3.2 ×2 / §2.8 で「30 setter/method」→「31」、計 5 箇所) | +12/-9 line |

### §2.3 解析手法 (= 次 session で確認したい場合の参照)

| # | 用途 | script | input | output |
|---|---|---|---|---|
| 1 | log → per-frame summary 抽出 | `/tmp/aya_step4_analysis/extract_cadence.py` | `/tmp/aya_scenario_{2,3}.log` (3.1 / 3.6 GB) | `s{2,3}_per_frame_lines.tsv` / `s{2,3}_shader_uniform_agg.tsv` |
| 2 | 定常域 frame 境界識別 | `identify_rez_boundary.py` | per_frame_lines.tsv | rolling mean / std (30 frame window) |
| 3 | 定常域内 aggregate | `aggregate_steady_state.py` | log + frame range | `s{2,3}_steady_uniform_rate.tsv` / `_shader_rate.tsv` / `_top_setters.tsv` |
| 4 | cadence band 分類 | `classify_cadence.py` | 上記 uniform_rate.tsv (s2+s3) | `cadence_classification.tsv` (237 uniform × 10 column) |

定常域認定根拠:
- s2 Cocobolo Island: frame 922 中 定常 = 100-915 = 816 frame、UBO_CADENCE event 16.2M
- s3 Roleplay Heaven: frame 740 中 定常 = 100-735 = 636 frame、UBO_CADENCE event 19.7M

### §2.4 観察 7 件 highlight (= Phase 1.A 入口判断材料)

| § | 観察 | Phase 1.A 影響 |
|---|---|---|
| §5.5.3 | per-asset 推定 45 → 観察 18 = sampler 系 hook 対象外注記、§0.2 推定値は実 cadence + sampler 系合算で 45 件、cadence 結論には別軸計測必要 | chapter 06b 起案時 sampler band を別表で扱う前提整備済 |
| §5.5.5 | dead 121 件 (= 38%) 内訳 = terrain detail_* 20 / post-effect ~15 / atmospheric 2 / **`aya_*` 独自 3 件 (aya_alpha_plate / aya_alpha_plate_enabled / aya_sss_skin_flag)** / その他 ~80 | **`aya_*` 3 件は AYAstorm 独自 path 未踏 or hash 経由配線、chapter 10 残課題追記候補** (= AYA 判断対象) |
| §5.5.6 | hashed-path 40 件 = §0.2「不明 16 件」推定 → 実観察 40 件で吸収、計測対象 inventory = reserved 318 + hashed-path 40 = **358 件確定** | chapter 06b 起案時の cadence inventory 母数確定 |
| §5.5.7 | matrix 系 (`modelview_matrix` / `inv_modelview` / `modelview_projection_matrix`) 4-5 件は per-program 推定 → 観察 437-688 cpf で **per-draw 確定補正必要** | chapter 05 §7.3 補正反映は chapter 06b 起案直前で実施 |

---

## §3 次着手 = Phase 0 **Step 5** = 計測 hook 除去 phase

### §3.1 scope (= literal、`indra/` 再解禁 phase)

`06a-prep §2.8` 検証完了後の除去 protocol に従い、**`c27733ae79` の機械的 revert** で hook 痕跡を全削除する。

| # | 削除対象 | 削除内容 | 参照 |
|---|---|---|---|
| 1 | `indra/llrender/llglslshader.cpp` 上部 | helper 2 個 (`ayaUboHookOnSetterByIndex` / `ByHashed`) + macro 2 個 (`AYA_UBO_HOOK_IDX` / `_HASH`) + `extern U32 gFrameCount;` + anonymous namespace + `#ifdef AYASTORM_UBO_CADENCE_HOOK` / `#else` / `#endif` block 全行 | §2.8 step 2 |
| 2 | 同 `llglslshader.cpp` 31 setter body 先頭 | `AYA_UBO_HOOK_IDX("...")` / `AYA_UBO_HOOK_HASH("...")` 1 行 × 31 method 全削除 | 同 |
| 3 | `indra/cmake/00-Common.cmake` 末尾 | `option(AYASTORM_UBO_CADENCE_HOOK ...)` + `if(AYASTORM_UBO_CADENCE_HOOK) add_compile_definitions(...) endif()` + 説明 comment block 全 9 行 | 同 |
| 4 | 残存 grep | `AYASTORM_UBO_CADENCE_HOOK` / `AYA_UBO_HOOK` / `g_aya_ubo_hook` / `UBO_CADENCE` の残存 0 件確認 | §2.8 step 3 |
| 5 | flag OFF build | autobuild `ReleaseFS_open` default build PASS 確認 (= flag は cmake から既に消えているため明示不要) | §2.8 step 4 |

### §3.2 推奨 revert 手順 (= `git revert` でなく機械的 `git checkout` ベース)

```bash
# 1. c27733ae79 の影響範囲確認
git show c27733ae79 --stat

# 2. 2 file を c27733ae79 直前の state に戻す (= 8579b09318 = Step 1 完了 handoff commit)
git checkout 8579b09318 -- indra/llrender/llglslshader.cpp indra/cmake/00-Common.cmake

# 3. 残存 grep self-verify
grep -rn "AYASTORM_UBO_CADENCE_HOOK\|AYA_UBO_HOOK\|UBO_CADENCE\|g_aya_ubo_hook" indra/ docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md
# 期待: indra/ 配下 0 件 / docs/ 内は §2.x spec 記述 + §5.5 観察記録 で複数存在 (= 残してよい)

# 4. flag OFF build (= 通常 build) PASS 確認
# AYA に build run 依頼 (= memory feedback_build 準拠で Claude 実行可だが、Step 5 は AYA 環境差確認も兼ねるため AYA build が望ましい)

# 5. commit
```

### §3.3 Step 5 commit message 雛形

```
revert(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 Phase 0 Step 5 計測 hook 除去 = c27733ae79 (Step 2 LL_INFOS hook 配線実装) を機械的 revert、indra/llrender/llglslshader.cpp + indra/cmake/00-Common.cmake から hook 痕跡全削除 / 06a-prep §2.8 検証完了後の除去 protocol 5 step 準拠 (= helper/macro/extern/setter body macro/CMake option 全削除、残存 grep 0 件、flag OFF build PASS) / Phase 0 計測 phase 完全完了 (= Step 1 pre-hook static / Step 2 hook 配線 / Step 3 AYA 計測 / Step 4 解析+§5 反映+副産物訂正 / Step 5 hook 除去 = 5 step 全完走) / 次 = Phase 1.A entry = (E')(F)(Q26-MUL)(Q27-CONFL) AYA 判断 + chapter 06b 起案
```

### §3.4 規律 (= Step 5 着手時 self-check)

1. **literal scope 厳守**: Step 5 = `indra/` 2 file revert + commit のみ、`docs/` 改変なし (= §5.5 観察記録は Phase 1.A 資料として保存)
2. **§5.5 内容は revert しない**: spec doc の §5.5 観察結果は Phase 0 計測の成果物、Phase 1.A 入口で chapter 06b 起案資料として使うため **削除しない**
3. **副産物 2 件訂正 (`c3e64095fe`) は revert しない**: §2.3.1 / §2.2.2 / §2.3.2 / §2.3.3 / §2.7 / §2.8 / §7 (P3) の訂正は spec 永続改善、Step 2 hook 実装と独立した品質改善
4. **build 検証は flag OFF のみ**: flag ON build は計測 build 専用、Step 5 で flag 自体が消えるため再 verify 不要
5. **`indra/` 改変は最小**: revert は機械的 (= `git checkout 8579b09318 -- <2 files>`)、新規 edit は行わない
6. **`/tmp/aya_step4_analysis/` 産出物は branch に含めない**: Python script + tsv 出力は AYA local 環境のみ、commit 対象外 (= `.gitignore tests/` 準拠の哲学、解析産物は spec doc §5.5 に統合済)

---

## §4 self-verify (= 本 handoff 起こした時点の整合性)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) 前 handoff (`...phase0-step2-complete.md`) からの遷移整合 | 「次 = Step 3 AYA 計測 → Step 4 解析+§5+副産物」(前 §3.1/§3.2) → 本 handoff 「Step 3 完了 (= AYA 計測 log 採取済) + Step 4 完了 (= 2 commit) → Step 5 hook 除去 移行」 起点と一致 | ✅ |
| (2) commit `6cbfbb28fa` / `c3e64095fe` 確認 | `git log --oneline -3` で本 handoff 起こす前の HEAD と一致 (前 = `ec95bc79ad` = Step 2 完了 handoff、本 session = `6cbfbb28fa` + `c3e64095fe` 2 連) | ✅ |
| (3) `indra/` 改変ゼロ厳守継続 | `git show 6cbfbb28fa c3e64095fe --stat` = `docs/specs/.../06a-prep-phase0-measurement.md` 1 file のみ × 2 commit、`indra/` 波及ゼロ | ✅ |
| (4) `feedback_no_scope_shrink` 準拠 | Step 4 (d) 副産物訂正で cascading count typo を 5 箇所一括補正 (= 指示 2 件のみでなく整合性連動 3 箇所追加)、scope 縮小なし | ✅ |
| (5) `feedback_design_phase_no_code_write` 準拠 | Phase 0 計測 doc 化 phase = `indra/` 改変 Step 2 のみ解禁、Step 4 は §5 / 副産物 spec doc のみ | ✅ |
| (6) `feedback_handoff_minimal_pre_req_read` 準拠 | 本 §1.1 = 3 件のみ列挙、§1.2 = pinpoint Read reference 分離 | ✅ |
| (7) `feedback_no_claude_coauthor` 準拠 | `6cbfbb28fa` / `c3e64095fe` 両 commit message に `Co-Authored-By: Claude` 不在 | ✅ |
| (8) `feedback_self_verify_before_handoff` 準拠 | §5.5 fill 前に「7 sub-section 結論」を AYA に summary 提示 → AYA「OK」確認 → commit 順守、handoff 前の数学整合 (= 318 reserved - 121 dead - 197 在 reserved = 0 / union 237 = 197 在 reserved + 40 hashed-path / 計測対象 358 = 318 + 40) self-verify PASS | ✅ |

---

## §5 引き継ぎ済の memory (= 次 session も活きる)

- `feedback_handoff_minimal_pre_req_read` (= 本 handoff §1 構造の根拠)
- `feedback_design_phase_no_code_write` (= Step 5 hook 除去 = `indra/` 再解禁、revert のみで新規 edit なし)
- `feedback_no_scope_shrink` (= Step 4 (d) cascading count 5 箇所一括補正、副産物指示 2 件のみで止めない根拠)
- `feedback_self_verify_before_handoff` (= Step 4 §5.5 fill 前の AYA summary 確認)
- `feedback_no_claude_coauthor` (= 全 commit 共著行禁止)
- `project_ayastorm_r41_vulkan_migration` (= r41 章 active pointer)
- `project_ayastorm_r41_design_principles` (= upstream 取込容易性 + core 並列化容易性の 2 大設計原則、Phase 1.A 入口 cadence 5 種 update site 設計の前提)

---

## §6 次 session 着手 1 line

「Step 4 commit `6cbfbb28fa` (§5.5 観察 7 sub-section fill) + `c3e64095fe` (副産物 2 件 + cascading count 統一) 完了、AYA 実機計測 log 解析 + spec 反映 phase 完了。次 = Step 5 = 計測 hook 除去 phase = `c27733ae79` 機械的 revert (= `git checkout 8579b09318 -- indra/llrender/llglslshader.cpp indra/cmake/00-Common.cmake`) + 残存 grep 0 件確認 + flag OFF build PASS 確認 + commit、`06a-prep §2.8` 5 step protocol 準拠、`docs/` 改変なし、Phase 0 計測 phase 5 step 全完走で Phase 1.A entry ready」
