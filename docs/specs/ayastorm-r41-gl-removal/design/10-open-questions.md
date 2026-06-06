# chapter 10: open-questions (= 設計 chapter 群 最終 chapter)

**位置付け**: 設計 chapter 群 (01-10) の **最終 chapter**。各 chapter で「chapter 10 持越」とされた未確定事項を集約 + AYA 判断仰ぎ候補を default 採用案付きで提示 + 実装 phase 入口で消化される項目を listing。本 chapter 完了 = design-phase 完了マーク、次 phase = implementation-phase 入口 (= η-29 Phase 0 計測) に移行可能 state 到達。

---

## §0 本 chapter の役割と扱い方

- 各 chapter の **「未決」のみ集約** (= 既起案完了確定事項は再収集しない)
- AYA 判断仰ぎ候補は **default 採用案 + 判断ポイント** を明示、AYA 確定後に該当 chapter §N を default → 確定形に書き換え (= 各 chapter の update 規律で reflect)
- **実装 phase 入口で消化される項目**は本 chapter 内に独立節として listing するが、判断は持越 (= 実装 phase 入口の grep / 計測 / build で解消)
- **live 表** (= migration 進行で update する各 chapter §N) は本 chapter ではなく該当 chapter 内に保持、本 chapter は pointer 化のみ
- 本 chapter 自体も live doc として AYA 判断確定 / Phase 完了で都度反映

---

## §1 AYA 判断仰ぎ候補 (= 主要 open question 集約)

### §1.0 29 件分類 index (= 全 AYA 判断仰ぎ候補 一覧、2026-06-03 Phase 0 Step 1 由来 Q26-MUL + Q27-CONFL 追加で 25 → 27 件、Phase 0 Step 1 enumerate (= ST-1) 由来 Q28-FFDUP 追加で 27 → 28 件、ST-7 sub-task 4 batch chapter 04 §6.4.7 由来 (Q-NTTP) 追加で 28 → 29 件、内 13 件判断済 = 2026-06-03 η-30 PA-1 entry 直前 gap remediation で (B2) 追加判断済)

**集約方法**: 設計 chapter 群 (01-10) 第二次査読 + Phase 2d-β prep audit (= 2026-06-03 batch) で抽出された AYA 判断仰ぎ候補を **(Q) ID + 該当 §1.x + 状態** で index 化。本 §1.0 は §1.1-§1.6 各表の summary、各項目の **default 採用案 + 判断ポイント** は該当 §1.x 表本体で確認。

| # | (Q) ID | 項目 | 該当 §1.x | 状態 |
|---|---|---|---|---|
| 1 | (V1') | set=1 80 binding split (40/40) | §1.1 | 未判断 |
| 2 | (V3') | set=1 layout 共通性 (全 program 共通 vs 別最適) | §1.1 | 未判断 |
| 3 | (S3') | sampler 49 個 descriptor set 配置 (set=3 同居 vs 分離) | §1.1 | 未判断 |
| 4 | (W) | `sProgramUboPool` maxSets (6 vs 1200) | §1.1 | 未判断 |
| 5 | (A1) | std140 offset 計算 (Codegen 独自 vs SPIR-V reflection vs 二重) | §1.2 | ✅ 判断済 (default 採用 = 二重保証 = `scripts/ubo_codegen/std140.py` 独自 calculator + spirv-cross cross-check、Phase 1.A 実装で物理確定、2026-06-06 audit 訂正) |
| 6 | (P) | GLSL parse 手段 (mini-parser vs glslang library) | §1.2 | ✅ 判断済 (default 採用 = `scripts/ubo_codegen/glsl_parser.py` mini-parser + `glslang_preproc.py` -E 前処理、Phase 1.A 実装で物理確定、2026-06-06 audit 訂正) |
| 7 | (G/B3) | perfect hash generator (Python frozen-table vs gperf vs CHD) | §1.2 | ✅ 判断済 (default 採用 = CHD 算法、`scripts/ubo_codegen/perfect_hash.py`、Phase 1.A 実装で物理確定、2026-06-06 audit 訂正) |
| 8 | (B1) | Codegen 実装言語 (Python vs C++ vs CMake script) | §1.2 | ✅ 判断済 (default 採用 = Python、`scripts/ubo_codegen/main.py:55 PYTHON_MIN = (3, 8)`、Phase 1.A 実装で物理確定、2026-06-06 audit 訂正) |
| 9 | (B2) | glslang 統合方式 (autobuild vs system pkg vs 自前) | §1.2 | ✅ 判断済 (B2b system pkg, 2026-06-03 η-30 PA-1 entry 直前 gap remediation = 実装で先行 commit 済 (`indra/cmake/Glslang.cmake`) + spirv-cross 同 pattern 拡張) |
| 10 | (B4) | 増分 build cache (mtime vs hash vs ccache) | §1.2 | ✅ 判断済 (default 採用 = hash + mtime hybrid、`scripts/ubo_codegen/build_cache.py:check_cache`、Phase 1.A 実装で物理確定、2026-06-06 audit 訂正) |
| 11 | (B5) | Codegen 実行 trigger (CMake DEPENDS vs 手動 target) | §1.2 | ✅ 判断済 (default 採用 = CMake DEPENDS + 手動 target 両方、`indra/cmake/AyaUboCodegen.cmake:83-86 CONFIGURE_DEPENDS` + `:100-106 AYA_UBO_CODEGEN_OUTPUTS add_custom_command` + 別途 `codegen_ubo_force` 手動 target、Phase 1.A 実装で物理確定、2026-06-06 audit 訂正) |
| 12 | (Q1) | 第 1 UBO migration template (Template A/B/C) | §1.3 | ✅ 判断済 (A, 2026-06-03 ST-5 batch) |
| 13 | (Q2) | Phase 当たり UBO 数 (1 厳守 vs cluster 許可) | §1.3 | ✅ 判断済 (A, 2026-06-03 ST-5 batch) |
| 14 | (Q3) | OpenGL path 並走期間 (全 Phase vs 中間撤廃 vs 段階撤廃) | §1.3 | ✅ 判断済 (A, 2026-06-03 ST-7 batch) |
| 15 | (Q4) | 3 OS 確証 Phase 順序 (Linux 先行 vs 並走 vs 順次) | §1.3 | ✅ 判断済 (C, 2026-06-03 ST-5 batch) |
| 16 | (Q5) | Phase 0 計測の Phase 番号化 (η-29 独立 vs Phase 1.0 vs 並走) | §1.3 | ✅ 判断済 (A, 2026-06-03 ST-7 batch = 既物理確定の形式 ✅ 化) |
| 17 | (K) | dirty 判定粒度 (member 単位 vs UBO 単位 vs cadence 単位) | §1.4 | 未判断 |
| 18 | (M) | descriptor set 4 帯 ↔ cadence 5 分類 配置 (1:1 vs 拡張) | §1.4 | 未判断 |
| 19 | (N) | `mUseUBO` initial 設定 (shader 単位 phase 移行 vs 全 ON) | §1.4 | 未判断 |
| 20 | (O) | `UB_*` 4 binding 拡張 (既存維持 + 新規追加 vs 全体再構成) | §1.4 | 未判断 |
| 21 | (F) | `MaterialUBO` vs `MaterialUBO_Legacy` 処遇 (統合 vs 別名分離 vs 廃止) | §1.5 | 未判断 |
| 22 | (Q22-NUM) | UBO blueprint 数 84 vs 85 整合 + set=2 25 vs 26 整合 | §1.6 | ✅ 判断済 (A', 2026-06-03) |
| 23 | (Q23-K) | chapter 09 §5.2 Template 内 Phase 2/3/4 具体数値の placeholder 性質明示 | §1.6 | ✅ 判断済 (A, 2026-06-03) |
| 24 | (Q24-S1) | chapter 06a §4.3.1 代替案 S1-A/B/C/D 採用 ((S1-存在) 解消済 + (S1-代替) 採用確定) | §1.6 | ✅ 判断済 (A, 2026-06-03) |
| 25 | (Q25-21CNT) | 21 件 → 25 件 double-count 検証 + 18 件 batch 統合反映方針 | §1.6 | ✅ 判断済 (B, 2026-06-03) |
| 26 | (Q26-MUL) | `MaterialUBO_Legacy` 構造改修方針 (= rename + 同 program set=1 binding=0 二重宣言解消) | §1.5 | ✅ 判断済 (A1+B1, 2026-06-03 ST-3 batch) |
| 27 | (Q27-CONFL) | (E') V/F 同 program 内 `set=2, binding=0` 共存 risk 解消方針 (= 5 UBO 同 binding ↔ V/F stage 跨ぎ link conflict 候補) | §1.5 | ✅ 判断済 (A1+B2+C1, 2026-06-03 ST-3 batch) |
| 28 | (Q28-FFDUP) | explicit F 群 (= pbropaqueF/pbrmetallicroughnessF/softenLightF/reflectionProbeF) + globalF 同 PerDrawUBO_ClipPlane F+F 重複宣言集約方針 (= GL spec で identical 宣言 link OK、code quality 観点で集約候補) | §1.5 | 未判断 (Phase 0 Step 1 ST-1 enumerate 由来 新規、Phase 1.A 中盤判断可 = default 提案 = 追記する確定) |
| 29 | (Q-NTTP) | R1 compile-time literal path 採否 (= C++20 NTTP 採用 vs C++17 維持) (= chapter 04 §6.4.7 R1 path 採否判定 = AYAstorm 既存 build C++17 default との trade-off) | §1.3 | ✅ 判断済 (A, 2026-06-03 ST-7 sub-task 8 batch = R1 不採用 / C++17 維持 = default 採用継続) |

**count 内訳** (2026-06-06 audit 訂正後): §1.1 (4) + §1.2 (7) + §1.3 (6) + §1.4 (4) + §1.5 (4) + §1.6 (4) = **29 件** (内 **19 件判断済** = §1.6 4 件 + §1.3 (Q1)(Q2)(Q3)(Q4)(Q5)(Q-NTTP) 6 件 + §1.5 (Q26-MUL)(Q27-CONFL) 2 件 + §1.2 **(A1)(P)(G/B3)(B1)(B2)(B4)(B5) 7 件 (= 2026-06-06 audit 訂正で Phase 1.A 実装 default 採用形を物理確定 status に反映、`scripts/ubo_codegen/` で実体充足)**、残 **10 件 未判断** = §1.1 4 + §1.4 4 + §1.5 (F)(Q28-FFDUP) 2)。

**判断済 4 件の反映先 cross-ref (= §1.6 batch、2026-06-03 Wave A-G)**: (Q22-NUM) → inventory §3.3.1 + 06c §3/§8 + 04 §5.3 + 01 §4.2 (= 計 13 箇所 `85 GLSL blueprint` rewrite 済) / (Q23-K) → 09 §5.2 冒頭注記 / (Q24-S1) → 06a §4.3 / §4.3.1 / §6.2 / §10 + 06a-prep §6 (S1-存在) / 本 chapter §4 live 表 / (Q25-21CNT) → 本 §1.0 表 + 各 chapter 反映 batch (= Wave A-G)。

**判断済 5 件の反映先 cross-ref (= 2026-06-03 ST-3/ST-5 batch 追加)**: (Q1) → 09 §11.1 / §5.2 / §14.4 / 本 §1.3 verdict マーク / (Q2) → 09 §11.2 / §14.4 / 本 §1.3 verdict マーク / memory `feedback_ubo_migration_one_at_a_time` 連動 / (Q4) → 09 §11.4 / §6.1 / §14.4 / 本 §1.3 verdict マーク / (Q26-MUL) → 02 §3.2 命名規則 `MaterialUBO_Class3_Legacy` 追加 / 05 §5 / §7.3 集約表 F2 確定マーク / `class3/deferred/materialV.glsl` 新規 file 起案 (= Phase 1.A 入口実装 task) / 本 §1.5 verdict マーク / (Q27-CONFL) → V 側 5 file (avatarSkinV / objectSkinV / skinnedVelocityV / skinnedVelocityAlphaV / avatarVelocityV) の `layout(set=2, binding=1/2/3/4)` 書換 (= Phase 1.A 入口実装 task) + 06c §3 接合表 / 04 §5 `ubo_metadata.inl` 出力契約 / 本 §1.5 verdict マーク。

**判断済 2 件の反映先 cross-ref (= 2026-06-03 ST-7 batch 追加)**: (Q3) → 09 §11.3 default → 確定形書換 / §14.4 ST-7 verdict マーク / §7.1 / §2.1 Phase 全体マップへの確定反映は Phase 1.A 中盤 (= dual-path 並走運用安定動作確認後) で実施 / 本 §1.3 verdict マーク / (Q5) → 09 §11.5 default → 確定形書換 / §14.4 ST-7 verdict マーク / 既物理確定 (= sub-step 命名 η-29 active) の形式 ✅ 化 / 本 §1.3 verdict マーク。

**未判断 1 件の新規登録 cross-ref (= 2026-06-03 ST-7 sub-task 4 batch)**: (Q-NTTP) → chapter 04 §6.4.7 既起案 default 「R1 不採用 (R3 で十分)」を chapter 09 §11.6 (= 本 batch で新設) + 本 chapter §1.0 row 29 + §1.3 表 (Q-NTTP) 行 (= 本 batch で追加) に集約 index 化、AYA 判断本体は §14.5 row 3-14 (Phase 1.A 入口) 保留継続、default A 採用継続で Phase 1.A 着手可。本登録は §14.5 row 3-10 (= chapter 10 持越項目登録済) + 遡及 §14.2 row 0-9 (= chapter 10 open questions 集約) の (NTTP) gap 解消連動 (= ST-7 sub-task 4 verify 時に検出、本 batch で remediation 実施)。

**判断済 1 件追加の反映先 cross-ref (= 2026-06-03 ST-7 sub-task 8 batch)**: (Q-NTTP) → 09 §11.6 default → 確定形書換 (= A 確定 / R1 不採用 / C++17 維持) / 09 §14.4 末尾 ST-7 sub-task 8 完了 paragraph / 09 §14.5 row 3-14 inline ✅ mark / 09 §14.8 Stage 3 entry verdict 達成 mark / 04 §6.4.7 default → 確定形書換 / 本 chapter §1.0 row 29 状態 column 「未判断」→「✅ 判断済 (A, ST-7 sub-task 8 batch)」 / 本 §1.0 count 内訳 11 → 12 判断済 / 18 → 17 未判断 / 本 §1.3 表 (Q-NTTP) 行 verdict マーク / 本 §1.3 末尾 ST-7 sub-task 8 batch verdict paragraph。本判断確定で **Stage 3 14/14 ✅ 全完走 = 設計 phase 完了 = `feedback_design_phase_no_code_write` 完全解禁条件達成 = Phase 1.A 実装 entry へ移行可能 state**。

**判断済 1 件追加の反映先 cross-ref (= 2026-06-03 η-30 PA-1 entry 直前 gap remediation)**: (B2) → 09 §14.4 末尾 post-completion correction paragraph 新設 (= ST-7 sub-task 6 verdict 訂正 + (B2) verdict 確定 + PA-1 真 scope 訂正の連動 update batch 記録) / 09 §14.4 ST-7 sub-task 6 paragraph 末尾追記 (= post-completion correction note) / 09 §14.5 row 3-11 verdict 訂正 (= 設計 phase 内充足 ✅ は維持 + 失敗時対応欄 update = spirv-cross のみ取込) / 08 §5.4.1.5 format version pin paragraph 全書換 (= glslang B2b 確定 + spirv-cross 同 pattern 拡張 + Python find_package(Python3) 既存取込済 + version drift 抑制 mechanism = §11.5.1 cache key environment block runtime 取得 + r42-α/β + Phase K+4 で Win/Mac bundle + 最終 version pin policy 確定推奨) / handoff PA-1 entry doc §0 + §3 PA-1 cell + §3 (Q-NTTP) paragraph + §4 (B2) 行 + §5 規律 11/12 + §6 row 10 + §7 memory + §8 着手 1 line / 本 §1.0 row 9 状態 「未判断」→「✅ 判断済 (B2b)」 / 本 §1.0 count 12 → 13 判断済 / 17 → 16 未判断 / 本 §1.2 (B2) 行 default 「B2a」→「✅ B2b 確定」。**本判断確定経路** = `feedback_doubt_self_first` 適用 (= autobuild.xml 片側 grep 検証 gap 検出 = ST-7 sub-task 6 verdict は `indra/cmake/Glslang.cmake` + `indra/cmake/Python.cmake` 既存取込を見落としていた) → Agent Explore 横断 verify (= `indra/cmake/` + `scripts/` medium thoroughness) → AYA「A」応答 (= spirv-cross のみ追加 = `SpirvCross.cmake` 起案 + Glslang.cmake と同 pattern = Linux first-class baseline)。**本確定で**: 未判断 1 件 → 判断済 1 件 (= B2) = Stage 3 14/14 ✅ 維持 (= 設計 phase 完了 verdict 不変) + chapter 10 持越 1 件減 (= 16 件 未判断) + handoff doc §5 規律 11/12 に教訓 literal 反映 (= `feedback_doubt_self_first` 強化適用 + 両側検証規律 = autobuild manifest 単独 grep でなく `*.cmake` / `scripts/` / build config 横断で実装側既存確認)。

**未判断 16 件の解消順序**: §1.1 (chapter 07 4 件) + §1.2 (chapter 08 6 件 = (A1)/(P)/(G/B3)/(B1)/(B4)/(B5)) + §1.4 (06b/06c 4 件) は **後続 AYA 判断 batch session** で集約消化、§1.5 (Q28-FFDUP) 1 件は Phase 1.A 中盤判断可 (= default 提案: 追記する 確定 = entry のみ作成、judgement 後ろ倒し)、(F) 1 件は Q26-MUL 確定で実質消化済だが本表 status 未 update。§1.3 (Q-NTTP) は **2026-06-03 ST-7 sub-task 8 batch で A 確定 = 判断済 = 本「未判断」枠から外れた** (= count 18 → 17 件)。§1.2 (B2) は **2026-06-03 η-30 PA-1 entry 直前 gap remediation で B2b system pkg 確定 = 判断済 = 本「未判断」枠から外れた** (= count 17 → 16 件、実装で先行 commit 済の verdict 追認形)。

---

### §1.1 chapter 07 §12 chapter 10 送り 4 件 (set / pool / sampler / layout)

| (Q) | 項目 | default 採用案 | 判断ポイント | 出典 |
|---|---|---|---|---|
| (V1') | set=1 80 binding split | **40/40 split (= per-program × triple-buffering × double-buffering を 2 set に分散)** | 1 set に 80 binding 詰めると device `maxDescriptorSetUniformBuffers` 限界 (= 多くの GPU で 72 / 84) に抵触可能性、split で安全側 / PSO layout 数倍化と trade | 07 §3.2 |
| (V3') | set=1 layout 共通性 | **V3a 全 program 共通 layout (= 全 80 binding を全 program で同 layout 宣言、未使用 binding に dummy 投入)** | V3a は PSO compatibility 最大 / dummy 投入分の memory cost、V3b は program 別最適 layout / PSO layout 数 80 倍化 | 07 §4.2 |
| (S3') | sampler 49 個 descriptor set 配置 | **set=3 per-asset 同居 (= asset binding 群と sampler 49 個を同 set 内で混在)** | per-asset との同居で set 切替頻度最小化、ただし set=3 binding 数膨張、別 set 分離案 (S1/S2/別案) は 07 §8.2 で 3 案併記済 | 07 §5.2 |
| (W) | `sProgramUboPool` maxSets | **maxSets = 6 (= active program × triple-buffer × double-buffer)** | active program 1 個前提の最小値、shader 数 200 × 3 × 2 = 1200 案は安全だが pool 容量肥大化 | 07 §6.2 |

**AYA 判断後の反映先**: chapter 07 §3.2 / §4.2 / §5.2 / §6.2 を default → 確定形に書換え、06c §3 接合表 / §8 sampler 配置 / chapter 04 §5.1 `ubo_metadata.inl` 出力契約も連動 update。

**注 (RF 配置)**: chapter 07 §12 持越のうち **(RF) reflection update fence throttle** は **AYA 判断不要** (= 客観計測 only) のため本 §1.1 表 4 件には含めず、§2.2 (= 実装 phase 入口消化) 単独配置。chapter 07 §12 由来は本 §1.1 (= AYA 判断仰ぎ 4 件) + §2.2 (= 計測消化 1 件) で **計 5 件** に分解されている (= 性質別の意図的 split、二重参照ではない)。

### §1.2 chapter 08 §17 chapter 10 送り 7 件 (Codegen / build pipeline)

| (Q) | 項目 | default 採用案 | 判断ポイント | 出典 |
|---|---|---|---|---|
| (A1) | std140 offset 計算 | **A1a 二重保証 (= Codegen 独自 calculator + SPIR-V reflection 両方算出、不一致で build error)** | 二重保証で std140 違反検出力最大、ただし build 時間増 / Codegen 保守工数増 | 08 §17 |
| (P) | GLSL parse 手段 | **P3 mini-parser + glslang -E 併用 (= mini-parser で UBO block / member 抽出、glslang -E で preprocessor 展開)** | 独自 mini-parser で軽量 / glslang library 全引込 (P-other) は build 時間増 | 08 §17 |
| (G/B3) | perfect hash generator | **G2/B3b Python frozen-table (= Codegen Python で frozen-table 出力、C++ 側は header 読込のみ)** | Python 完結で外部依存 (gperf / frozen library) ゼロ、ただし衝突 0 検証 logic を独自実装 | 08 §17 |
| (B1) | Codegen 実装言語 | **B1a Python 3.8+** | 既存 viewer-tools / autobuild が Python なので一致、C++ standalone は実装工数増 / CMake script は機能不足 | 08 §17 |
| (B2) | glslang 統合 | **✅ B2b system pkg 確定** (= 実装で先行 commit 済 = `indra/cmake/Glslang.cmake` で `find_package(glslang CONFIG REQUIRED)` + `glslang-15.1.0/` vendored + Ubuntu 24.04 `apt install glslang-dev` (15.1.0-2) 経路、Linux first-class baseline (r41 charter §1)、Win/Mac 3 OS bundle は r42-α/β 着手時 (charter §7.5)、spirv-cross 同 pattern 拡張 = PA-1 で `SpirvCross.cmake` 起案) (2026-06-03 η-30 PA-1 entry 直前 gap remediation) | autobuild vendoring (B2a) は 3 OS 整合と Win/Mac bundle 工数高、system pkg (B2b) は Linux first-class で開発容易 = 実装で先行採用済、Win/Mac は r42-α/β 時に再判断 (= 必要なら autobuild_package 起こす)。自前実装は工数膨大で不採用 | 08 §17 + chapter 09 §14.4 末尾 post-completion correction paragraph |
| (B4) | 増分 build cache | **B4a hash + mtime 併用 (= mtime で fast path、hash で false positive 排除)** | 純 mtime only は IDE 編集で false rebuild、純 hash only は I/O cost、ccache は配線重 | 08 §17 |
| (B5) | Codegen 実行 trigger | **B5a CMake DEPENDS 自動 + 手動 target 併設** | 自動 trigger で開発体験良好 + 手動 target で debug / re-codegen 容易、全 build 時 trigger は build 時間増 / 手動 only は trigger 漏れ | 08 §17 |

**AYA 判断後の反映先**: chapter 08 §1-§16 の各節 default を確定形に書換え、chapter 04 §3 / §5 / §7 Codegen 機構と整合確認。

### §1.3 chapter 09 §11 (Q1)-(Q5) (Phase Roadmap)

| (Q) | 項目 | default 採用案 | 判断ポイント | 出典 |
|---|---|---|---|---|
| (Q1) | 第 1 UBO migration template | **A 最小リスク優先 (= singleton 系 `UB_REFLECTION_PROBES` 単体から開始、最後に最頻出 per-draw)** | A は経路 1 個ずつ通電で REJECT 影響範囲最小 / B 最頻出優先 / C cadence 系統別 batch、Phase 0 結果次第で具体順位再確定 | 09 §5.2 / §11.1 |
| (Q2) | Phase 当たり UBO 数 | **A 1 UBO 厳守** (memory `feedback_ubo_migration_one_at_a_time` 直接準拠) | A は cold launch 検証で REJECT 切り分け最大 / B cluster 許可は同一 owner UBO 群 (`UB_GLTF_NODES` / `UB_GLTF_MATERIALS` / `UB_GLTF_JOINTS`) を例外許可 | 09 §11.2 |
| (Q3) | OpenGL path 並走期間 | **A 全 UBO 移行完了まで並走 (= Phase K+4 で初撤廃)** | A は REJECT 時 baseline 確保 / B 中間 Phase 撤廃で並走 cost 削減 / C 段階撤廃で両者折衷 | 09 §7.1 / §11.3 |
| (Q4) | 3 OS 確証 Phase 順序 | **C Linux 完了後 Win/Mac 並走** | C は baseline 確定 + Mac 委任両立 / A Linux first 順次は時系列長 / B 3 OS 並走は REJECT 切り分け困難、Mac 不所持制約 (`feedback_mac_only_fixes_accept_as_is`) と整合 | 09 §6.1 / §11.4 |
| (Q5) | Phase 0 計測の Phase 番号化 | **A 独立 Phase η-29 として明示** | A は design-phase / implementation-phase 分離原則と整合 / B Phase 1.0 格納は Phase 1 入口連続実施 / C Phase 1 並走は計測結果が Phase 1.B 入力に必要なため時系列矛盾 | 09 §3 / §11.5 |
| (Q-NTTP) | R1 compile-time literal path 採否 (= C++20 NTTP 採用 vs C++17 維持) | **✅ A 確定 (= R1 不採用 / R3 name-based dispatch のみ、C++17 維持)** (2026-06-03 ST-7 sub-task 8 batch) | A は AYAstorm 既存 build C++17 default 整合 + R1 効果限定 (= compile-time vs runtime 1 indirection) / B R1 採用 + 全 module C++20 切替は 3 OS toolchain 確認 + dependent module re-validation cost / C 部分採用は機構複雑度増 + 効果 limited、R1 は Phase K+4 以降 polish 候補保留可 | 09 §11.6 / 04 §6.4.7 |

**AYA 判断後の反映先**: chapter 09 §2.1 Phase 全体マップ + §5.2 / §6.1 / §7.1 default → 確定形に書換え、K 確定値 (= §3.1) と per-Phase 担当者 (= §3.2) も連動確定。

**2026-06-03 ST-5 batch verdict** (= 「全 default 採用」AYA 応答): (Q1) = **A 確定** (= 最小リスク UBO 優先、`UB_REFLECTION_PROBES` 単体から開始、最後に最頻出 per-draw、Phase 0 結果次第で具体順位再確定) / (Q2) = **A 確定** (= 1 UBO 厳守、memory `feedback_ubo_migration_one_at_a_time` 直接準拠) / (Q4) = **C 確定** (= Linux 完了後 Win/Mac 並走、`feedback_mac_only_fixes_accept_as_is` と整合)。(Q3)(Q5) は default 採用継続、Phase 1.A 中盤まで後ろ倒し可 (= handoff §3.5 規律 7)。反映先: 09 §11.1 / §11.2 / §11.4 / §14.4 verdict マーク + 09 §2.1 / §5.2 / §6.1 / §7.1 default → 確定形書換 + K 確定値 (§3.1) と per-Phase 担当者 (§3.2) 連動確定。

**2026-06-03 ST-7 batch verdict** (= 「推奨で」AYA 応答 = ST-5 batch 「全 default 採用」継承): (Q3) = **A 確定** (= 全 UBO 移行完了まで GL ↔ Vulkan dual-path 並走 = Phase K+4 で初めて OpenGL path 撤廃、REJECT 時 baseline 確保最大、memory `feedback_build_only_verified` 整合、B/C 案は Phase K+3 進行中に再評価可 = 後ろ倒し option 保持) / (Q5) = **A 確定** (= 独立 Phase η-29 として明示 = sub-step 命名 `4.3-γ'-port-β-2-bundle-B-B?-η-29` で物理現実が既 active = 既物理確定の形式 ✅ 化、09 §14.4 で既「default 確定 = 既反映済」と記載、B/C は sub-step rename cost / 時系列矛盾で技術的不成立)。反映先: 09 §11.3 / §11.5 default → 確定形書換 (= 本 batch で完了) + 09 §14.4 ST-7 verdict マーク (= Stage 2 5 件全件 ✅ 完了 = 完全達成宣言) + 本 §1.3 verdict マーク (= 本 paragraph) + 本 chapter §1.0 状態 column update (= (Q3) ✅ + (Q5) ✅) + count 内訳 update (= 9 → 11 件判断済 / 19 → 17 件未判断)。Stage 3 残 12 項目消化 phase = handoff §3.1 sub-task 1 完了、次 sub-task 2 = 3-4 + 3-5 chapter 04+08 反映済確認 batch (= Claude 自走 verify、AYA 判断不要)。

**2026-06-03 ST-7 sub-task 4 batch (Q-NTTP) 新規登録** (= §14.5 row 3-9 / 3-10 verify 連動で chapter 04 §6.4.7 由来の (NTTP) 未登録 gap を `feedback_doubt_self_first` 適用で検出 → remediation 実施): chapter 09 §11.6 (Q-NTTP) 新設 (= A/B/C 3 案 + default A 提案 + 確定タイミング Phase 1.A 入口判定可 明示) + 本 chapter §1.0 row 29 追加 (= 28 → 29 件、count 内訳 §1.3 5 → 6 件 / 未判断 17 → 18 件) + 本 §1.3 表に (Q-NTTP) 行追加 (= default A R1 不採用 / C++17 維持、判断ポイント trade-off 明示、出典 09 §11.6 + 04 §6.4.7)。判断本体は §14.5 row 3-14 (Phase 1.A 入口) 保留継続、default A 採用継続で Phase 1.A 着手可 = 着手 ready state に影響なし。本登録で §14.5 row 3-10 (= chapter 10 持越項目登録済) 14/14 ✅ 充足 + 遡及 §14.2 row 0-9 (= chapter 10 open questions 集約) (NTTP) gap 解消連動。

**2026-06-03 ST-7 sub-task 8 batch verdict** (= AYA「A」応答 = default 採用継続): (Q-NTTP) = **A 確定** (= R1 compile-time literal path 不採用 / R3 name-based dispatch + perfect hash (CHD) + frozen-table 経路で十分高速 / AYAstorm 既存 build standard C++17 維持 / C++20 切替 cost = 3 OS toolchain 確認 + dependent module re-validation + autobuild manifest 変更 回避)。R1 は **task 完了後の polish 候補** (= Phase K+4 以降 optimization phase 候補) として保留可、Phase 1.A handoff doc §3 PA-0 (= C++20 切替 task) は不要 = PA-1 から即着手可。反映先: 09 §11.6 default → 確定形書換 + 09 §14.4 末尾 ST-7 sub-task 8 完了 paragraph + 09 §14.5 row 3-14 inline ✅ mark + 09 §14.8 Stage 3 entry verdict 達成 mark + 04 §6.4.7 default → 確定形書換 + 本 chapter §1.0 row 29 状態 ✅ + count 内訳 12 件判断済 / 17 件未判断 + 本 §1.3 (Q-NTTP) 行 verdict マーク + 本 §1.3 末尾 paragraph (= 本 paragraph)。**本 batch で Stage 3 14/14 ✅ 全完走 = 設計 phase 完了 = `feedback_design_phase_no_code_write` 完全解禁条件達成 = Phase 1.A 実装 entry へ移行可能 state**。次 session 着手地点 = Phase 1.A handoff doc PA-1 (= autobuild manifest pin)。

### §1.4 chapter 06b §8 / 06c §10 chapter 10 持越 (dirty / descriptor 配置 / mUseUBO)

| (Q) | 項目 | default 採用案 | 判断ポイント | 出典 |
|---|---|---|---|---|
| (K) | dirty 判定粒度 | **K2 UBO 単位** (= UBO 全体を 1 dirty bit で管理) | K2 は logic 単純 + flush 単位と整合 / K1 member 単位は upload dedup 強力だが logic 複雑 / K3 cadence 単位は粒度粗すぎ | 06b §3.4 / §8 |
| (M) | descriptor set 4 帯 ↔ cadence 5 分類 配置 | **M1 1:1 配置** (= per-frame=set=0 / per-program=set=1 / per-asset=set=2 / per-skin / per-draw=set=3) | M1 は配置単純 + bind 頻度最小化 / M2 cadence 別 set 拡張は Vulkan max set 数 4 制約抵触 | 06c §2.1 / §10 |
| (N) | `mUseUBO` initial 設定 | **N2 shader 単位 phase migration** (= migration 中 shader のみ ON、未 migration shader は OFF で OpenGL path 継続) | N2 は段階移行 + 1 UBO ずつ migration と整合 / N1 全 ON は cold launch 検証粒度喪失 | 06c §6.2 / §10 |
| (O) | UB_* 4 binding 拡張 | **O1 既存維持 + 新規追加** (= 既存 4 binding を変更せず、新規 cadence に新 binding 追加) | O1 は upstream 取り込みやすさ最大 / O2 全体再構成は upstream divergence 拡大 | 06c §3.4 / §10 |

**AYA 判断後の反映先**: 06b §3.2.3 / §5.2 を K 確定形に、06c §2 / §3 / §6 / §3.4 を M/N/O 確定形に書換え。

### §1.5 chapter 05 §10 統廃合再評価 + Phase 0 Step 1 由来構造改修 (= 3 件)

| (Q) | 項目 | default 採用案 | 判断ポイント | 出典 |
|---|---|---|---|---|
| (F) | `MaterialUBO` vs `MaterialUBO_Legacy` 処遇 | **F2 別名分離 (= 2026-06-03 Phase 0 Step 1 で member 完全別物確認 → F1 統合反証、両者実 attach 確認 → F3 廃止反証、F2 第一候補 narrowing 確定)** | Phase 0 Step 1 で member diff + attach 表確定 (= 06a-prep §4.6 / §4.6.2 / §4.6.3 / §4.6.5)、F2 確定後の **具体 rename 名 + 構造改修** は (Q26-MUL) に分離 | 05 §10 + 06a-prep §4.6 |
| (Q26-MUL) | `MaterialUBO_Legacy` 構造改修方針 (= (F)=F2 確定後の sub-question) | **MUL-A1 + MUL-B1 組合せ default** (= rename `MaterialUBO_Class3_Legacy` 等 specific 名 + class3 専用 V shader 追加で MaterialUBO 不宣言、または MUL-B2 binding ずらしで `set=1, binding=1` 等別 slot) | (A) rename 名選定: `MaterialUBO_Class3_Legacy` / `MaterialUBO_BlinnLegacy` / `MaterialUBO_BB` 等の specific 名 (= chapter 02 §3.2 命名規則と整合) / (B) 同 program set=1 binding=0 二重宣言解消手段: B1 class3 用 V shader 別途用意 (= MaterialUBO 不宣言、F=class3/materialF.glsl と組合せ) / B2 binding ずらし (= MaterialUBO_Legacy → `set=1, binding=1`) / B3 set 帯分離 (= set=4 等の予備帯活用) / B4 集約 (= MaterialUBO + Legacy member 全件を 1 UBO に合体、program 別 permutation で member 選択) | 06a-prep §4.6.6 (F)-1 |
| (Q27-CONFL) | (E') V/F 同 program 内 `set=2, binding=0` 共存 risk 解消方針 (= 5 UBO 同 binding ↔ V/F stage 跨ぎ link conflict 候補) | **CONFL-A1 + CONFL-B2 組合せ default** (= まず Phase 1.A 入口で V/F 共通 shaders list 経由 attach 全 program enumerate + V+F 同 binding 共存 program listing、共存程度に応じて B2 binding ずらし or B3 set 帯分離) | (A) enumerate 範囲: A1 `LLViewerShaderMgr::loadBasicShaders/loadShadersDeferred/loadShadersObject/loadShadersAvatar/loadShadersEnvironment/loadShadersInterface/loadShadersWindLight/loadShadersWater` 全 program / A2 部分 (= 高 risk program のみ) / (B) 解消手段: B1 V/F shader 分割 (= `avatarSkinV` を含む program で ClipPlane F を抜く) / B2 binding ずらし (= V 側 4 UBO を `set=2, binding=1/2/3/4` 等に分離 + F 側 ClipPlane を `binding=0` 維持) / B3 set 帯分離 (= V 側 skin/velocity UBO を `set=4` 等の予備帯へ移動) / B4 V/F 一括 UBO 集約 (= 同 program 内全 set=2 binding を 1 UBO に統合) / (C) 解消判定 phase: C1 Phase 1.A 入口で全件解消 (= LL_VULKAN_GLSL 有効化前に dormant 顕在化阻止) / C2 各 UBO migration phase 単位で逐次解消 | 06a-prep §3.5.6 (E')-1/(E')-2/(E')-3/(E')-4 |
| (Q28-FFDUP) | explicit F 群 + globalF 同 PerDrawUBO_ClipPlane F+F 重複宣言の集約方針 (= ST-1 enumerate 結果由来 = `globalF.glsl` は `attachShaderFeatures` で全 program 無条件 attach + 他 4 F file `pbropaqueF.glsl` / `pbrmetallicroughnessF.glsl` / `softenLightF.glsl` / `reflectionProbeF.glsl` も `PerDrawUBO_ClipPlane` を同 `set=2, binding=0, std140` で宣言、`gDeferredPBROpaqueProgram` / `gGLTFPBRMetallicRoughnessProgram` / `gDeferredSoftenProgram` で F+F 重複) | **FFDUP-A1 + FFDUP-B2 組合せ default** (= globalF 1 箇所集約案、判断 phase は Phase 1.A 中盤 = 他 UBO migration 進捗で構造判断容易化後) | (A) 集約手段: A1 globalF を共通 include header 化 (= 各 F file 先頭で `#include "deferred/globalF.glsl"` 経由参照に統一、他 4 file の `PerDrawUBO_ClipPlane` 宣言削除) / A2 共通 PerDrawUBO_ClipPlane を別 file (= `clipPlaneCommon.glsl`) に切出して全 F file から include / A3 現状維持 (= identical 宣言は GL spec 上 link OK、F+F 重複は warning only として受容) / (B) 判断 phase: B1 Phase 1.A 入口で全件 (= Q27-CONFL B2 採用と同時) / B2 Phase 1.A 中盤 (= 他 UBO migration 進捗で構造判断容易化後) | ST-1 (E')-1 enumerate 結果 + 06a-prep §3.5.1 5 F file listing |

**(F) AYA 判断後の反映先**: chapter 05 §5 / §7.3 集約表で F2 確定マーク、本 chapter §4 live 表 (F) 解消、Q26-MUL に sub-question 切出。

**(Q26-MUL) AYA 判断後の反映先**: chapter 02 §3.2 命名規則表に `MaterialUBO_*Legacy` 確定名追加、chapter 05 §5 / §7.3 集約表に確定 binding/set 帯反映、`class3/deferred/materialF.glsl` の UBO 宣言行 + 必要なら `class3/deferred/materialV.glsl` 新規 file 作成、inventory §3.2 共存記述補正、chapter 06c §3 接合表に set/binding 確定反映。

**(Q27-CONFL) AYA 判断後の反映先**: chapter 06c §3 接合表に確定 binding/set 帯反映、`avatarSkinV.glsl` / `objectSkinV.glsl` / `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` / `avatarVelocityV.glsl` (V 側 4 UBO 宣言 file) + `globalF.glsl` / `reflectionProbeF.glsl` / `softenLightF.glsl` / `pbrmetallicroughnessF.glsl` / `pbropaqueF.glsl` (F 側 ClipPlane 宣言 5 file) の `layout(set=N, binding=M)` 書換え、chapter 04 §5 `ubo_metadata.inl` 出力契約に確定 binding 反映、Phase 1.A 入口 handoff doc (= η-30) に enumerate 結果 attach。

**(Q28-FFDUP) AYA 判断後の反映先 (= 判断 deferred、entry のみ作成済)**: Phase 1.A 中盤 (= Q27-CONFL B2 採用後 V 側 binding ずらし完了状態) で globalF 共通 include 化 / 別 file 切出し / 現状維持の 3 案 trade-off 再評価、確定後に 5 F file の `PerDrawUBO_ClipPlane` 宣言整理 + include 構造書換、chapter 06c §3 接合表 + 04 §5 出力契約に反映。

**2026-06-03 ST-3 batch verdict** (= 「全 default 採用」AYA 応答): (Q26-MUL) = **A1 + B1 確定** (= rename `MaterialUBO_Class3_Legacy` + class3 専用 V shader `class3/deferred/materialV.glsl` 新規追加で MaterialUBO 不宣言) / (Q27-CONFL) = **A1 + B2 + C1 確定** (= A1 enumerate ST-1 で完了済 (= `attachShaderFeatures` 経由 globalF 全 program 無条件 attach + V 側 skin/velocity feature flag attach 確定)、B2 V 側 4 UBO `PerDrawUBO_AvatarSkin` / `PerDrawUBO_ObjectSkin` / `PerDrawUBO_SkinnedVelocity` / `PerDrawUBO_AvatarVelocity` を `set=2, binding=1/2/3/4` 等にずらし + F 側 `PerDrawUBO_ClipPlane` を `binding=0` 維持、C1 Phase 1.A 入口で全件解消) / (Q28-FFDUP) = **新規追記、判断 Phase 1.A 中盤まで持越** (= default 提案 = 追記する 確定、entry 作成済)。反映先: 上記 (Q26-MUL) / (Q27-CONFL) / (Q28-FFDUP) 反映先表 + 本 chapter §1.0 index 状態 update + Q28 row 新規追加 (= 本 §1.5)。

### §1.6 Phase 2d-β prep audit 起因 新規 4 件 (2026-06-03 AYA 判断済 batch)

第二次査読 §8.1 18 件 + audit §3.1 追加 2 件 = 計 20 件修正推奨の整合性確認過程で抽出された **AYA 判断仰ぎ 4 件** を本 §1.6 に追加登録。本 §1.6 4 件は **全て 2026-06-03 AYA 判断済** = 各 chapter 反映完了状態 (= Wave A-G 反映 batch で消化)、本表は judgement 履歴 + 反映先 cross-ref 保存用。

| (Q) | 項目 | AYA 判断 | 反映先 | 出典 |
|---|---|---|---|---|
| (Q22-NUM) | UBO blueprint 数 84 → 85 整合 + set=2 25 → 26 整合 | **A' (= literal 全 14+6 箇所 rewrite 全件波及)** | inventory §3.3.1 (3 箇所) / 06c §3/§4.1/§8/§10 (7 箇所) / 04 §5.3 (2 箇所) / 01 §4.2 (2 箇所) / 09 §5.1 (1 箇所) = 計 15+ 箇所 | inventory §3.3.1 set=2 個数再集計 (24 → 25 → 26) |
| (Q23-K) | chapter 09 §5.2 Template A/B/C 内 「Phase 2」「Phase 3」「Phase 4」具体数値の placeholder 性質 | **A (= §5.2 冒頭注記で placeholder 例示明示)** | 09 §5.2 冒頭注記追加 (= 「(Q1)(Q2) 確定後の phase 振分 例示、K 確定で置換される予定」明示) | 09 §11 K 確定条件 vs §5.2 具体数値の整合矛盾 |
| (Q24-S1) | chapter 06a §4.3.1 代替案 S1-A/B/C/D 採用 | **A (= (S1) を (S1-存在) + (S1-代替) 2 軸分割、(S1-存在) 解消済マーク + (S1-代替) S1-C default 採用確定)** | 06a §4.3 但し書き / §4.3.1 / §6.2 / §10 持越表 (= 6 箇所) + 06a-prep §6 (S1-存在) 解消マーク + 本 chapter §4 live 表 (S1-存在)(S1-代替) split | 06a §4.3.1 API 不存在確認済だが代替案未確定 (= 2 軸混在) |
| (Q25-21CNT) | 21 件 → 25 件 double-count 検証 + 18 件 batch 統合反映方針 | **B (= 18 件 batch 統合反映 + 25 件最初から再集計、本 §1.0 表で全件確認)** | 本 chapter §1.0 25 件分類 index + 各 chapter Wave A-G 反映 batch (= chapter 02/05/06a/09/10/handoff/audit 修正) | 第二次査読 §8.1 18 件 + audit §3.1 追加 2 件 = 修正推奨数の最終集計確認 |

**反映完了確認**: 本 §1.6 4 件 = 設計 chapter 群 全件 audit + 修正 batch 完了済 = Phase 2d-β-revise 本体 (= chapter 04 + 08 prototype 深化, Deliverable A/B/C) 着手前提条件 (= AYA 判断 4 件完了 + 設計 chapter 群 修正推奨 20 件 反映完了) の **前者 4 件 完了マーク**。後者 20 件 反映完了は Wave A-G batch 完了報告で総括。

**注 (本 §1.6 の位置付け)**: 本 §1.6 4 件は **judgement 履歴 archive** = 既消化済 = §1.1-§1.5 (= 未判断 21 件) と性質が異なる。implementation-phase 入口 (= η-29 Phase 0 計測 session) では §1.1-§1.5 のみ AYA 判断 batch session 対象、§1.6 は再判断不要 (= reference のみ)。

### §1.7 Phase 2.α 案 X 確定起因 新規 6 件 (2026-06-06 record)

**位置付け**: Phase 2.L0 sub-session 5 step 2-batch-0-a で発覚した二重 source 構造同期断裂を根治するため起案された Phase 2.α (= 独立 sub-phase) で **案 Y → 案 Z → 案 Z' → 案 X 連続 5 落ち** 経て確定した案 X 由来の新規 question 6 件。closed 3 件 + open 3 件。詳細 = handoff `phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9。

**位置付け 2**: 本 §1.7 6 件は §1.6 4 件と同質 (= judgement 履歴 archive)、closed 3 件は確定判断 record、open 3 件は二重 source 同期 protocol formal化に伴う将来課題。

| (Q) | 項目 | 判断 (= 案 X 確定) | 反映先 |
|---|---|---|---|
| (Q-α1) | blueprint dir 廃止可否 | **closed = 保護指示** (= AYA 指示 #5「discard しない」literal 真意 = codegen 入力 source of truth 保護、case Y 撤回根拠、commit `df38b7c994` + `f95182ded5`) | 04 §2.2 literal 訂正 + 09 §2.1.2 + handoff §D.9.3 / §D.9.4 |
| (Q-α2) | codegen 入力 source の確定方向 | **closed = blueprint dir 単独** (= 別 GLSL 並列 build process、actual class*/ + cinematic_bd/ は runtime compile target、案 Z+Z' 撤回根拠、blueprint dir 単独走行で 94 UBO emit + parse error 0 件 evidence) | 04 §2.2 literal 訂正 + handoff §D.9.2 / §D.9.4 |
| (Q-α3) | 二重 source 構造同期 protocol | **closed = formal化** (= main.py `_verify_block_match` 拡張で blueprint + actual 対称的整合 verify build-time check、case Z 起案契機の二重 source 構造同期断裂を構造的に解消、phase F で実装) | 04 §4.4 維持 + handoff §D.9.6 #3 + α-3 entry §3.6 phase F |
| (Q-α4) | `_verify_block_match` 拡張時の **誤検出許容範囲** | **open** = (a) cinematic_bd 上書き path で同 layout 必須 (= 既 α-2 commit `b66ec99f72` 対応済)、(b) blueprint と actual の binding 値乖離は abort (= 二重 source 整合違反)、(c) member 順序入替も abort or warning か未確定 = phase F 実装時に protocol 確定 | phase F 実装時 |
| (Q-α5) | UBO 新規追加時の **追加先順序** = blueprint dir 先 vs actual 先 | **open** = 設計 doc 内 protocol 未確定、両方順序 OK で build-time `_verify_block_match` 整合 verify で abort detection が default 候補、case X README §5 編集規律で「順序逆も可」literal 既記載だが将来 sub-step 起案時に確認要 | phase F or 別 sub-phase |
| (Q-α6) | Phase 2 全 94 UBO migration 進行中 blueprint dir と actual の **対称的書換 protocol** | **open** = sub-session 5 7 UBO (= phase E で blueprint 側同期書換予定) 以降の 73 UBO 改修時の同期 protocol、actual 改修先 (= class*/ + cinematic_bd/ 改修先) と blueprint 改修先の **手順順序** + sub-step level commit 分割方針 = WORK_ORDER.md 更新時に protocol formal化候補 | WORK_ORDER.md 改修時 |

**反映完了確認** (= 2026-06-06 時点): closed 3 件 = phase A-D で全件反映完了 (= revert + blueprint README + handoff §D.9 + 04 §2.2 literal 訂正 + 09 §2.1.2)。open 3 件 = phase F (= main.py `_verify_block_match` 拡張) + WORK_ORDER.md 改修時に protocol formal化予定。

---

## §2 実装 phase 入口で消化される項目 (= 判断は持越、listing のみ)

本節は **設計 chapter 群完了後の implementation-phase 入口** (= η-29 Phase 0 計測 session) で grep / 計測 / build を通じて解消される項目。本 chapter 10 では判断しない、listing のみ。

### §2.1 chapter 06a-prep §7 (P1)-(P4) (Phase 0 入口 grep 消化)

**消化方法の記法**: 各項目は **(a) 実行 command + (b) 対象 dir + (c) 判定閾値 + (d) pass-fail criteria** を imperative で明示。

| (P) | 項目 | 消化方法 |
|---|---|---|
| (P1) | `LL_INFOS("UBO_CADENCE")` class 名衝突有無 | (a) Grep `LL_INFOS\("UBO_CADENCE"\)` (b) `indra/` 配下全 `.cpp` / `.h` / `.glsl` (c) ヒット件数 ≥ 1 で衝突判定 (d) PASS = 0 件 / FAIL → 衝突 class を別 log tag (`UBO_CADENCE_NEW` 等) に rename して再 grep PASS 確認 |
| (P2) | CMake patch 配置先 (`00-Common.cmake` vs `LLRender.cmake` 等) | (a) Grep `AYASTORM_UBO_CADENCE_HOOK\|AYASTORM_.*_HOOK` (b) `indra/cmake/*.cmake` (c) 既存 AYASTORM_* flag のヒット位置 listing (d) 既存 flag が `00-Common.cmake` 集中なら同位置に追加 / 別 cmake 分散なら最頻出位置を採用、判断結果は本表に追記して chapter 06a-prep §7 (P2) に反映 |
| (P3) | frame counter 公開方式 (`extern` / helper / 既存流用) | (a) (P4) 結果を入力に判定 (b) `indra/newview/llappviewer.{h,cpp}` (c) 流用可否 boolean (d) 流用可 → 既存 symbol を `#include` で参照のみ (extern / helper 追加不要) / 流用不可 → `llappviewer.h` に `extern U64 gAyaFrameCount;` 宣言 + `.cpp` で `LLAppViewer::idle()` 末尾 increment 追加 |
| (P4) | 既存 frame counter 流用可能性 (`gFrameCount` 等) | (a) Grep `gFrameCount\|gFrameCounter\|mFrameCount\|sFrameCount` (b) `indra/newview/llappviewer.{h,cpp}` + `indra/llcommon/llapp.{h,cpp}` + `indra/llrender/` (c) 定義 1 件以上 AND 単調増加 logic 確認 (d) PASS (定義 + monotonic) → (P3) で流用宣言 / FAIL (未定義 OR rollover あり) → (P3) で新規追加宣言 |

### §2.2 chapter 07 §12 (RF) reflection update fence throttle

**配置**: chapter 07 §12 持越項目だが **AYA 判断不要 (= 客観計測 only)** のため §1.1 表 (AYA 判断仰ぎ 4 件) には含めず、本 §2.2 単独配置。本 chapter §1.1 注 (RF 配置) と対応。

| 項目 | 消化方法 |
|---|---|
| (RF) reflection update fence throttle (> 100 回/分で warn) | (a) 実装 phase で `LLReflectionMapManager::updateProbeFace()` 入口に `LL_INFOS("RF_FENCE")` hook 追加 (= wall-clock 1 分 window の call count 記録、循環 buffer 60 slot で 1 秒粒度) (b) 計測対象は AYAstorm baseline build (= H1b と同 build flag `-DAYASTORM_UBO_CADENCE_HOOK=ON`) (c) 判定閾値 = 1 分 window 内 call count ≥ 100 (d) PASS (≤ 99) → throttle 不要マーク / FAIL (≥ 100) → `LL_WARNS("RF_FENCE")` 1 回出力 + cvar `AYAReflectionFenceWarned=true` で以降抑制、warn 検知時は chapter 07 §12 (RF) を「throttle 実装要」に解消マーク変更 |

### §2.3 chapter 08 §17 実装 phase 評価項目

| 項目 | 消化方法 |
|---|---|
| (P-future) 「unused mask」判定基準 (GLSL 宣言の有無 only vs member 参照解析) | (a) default = 「GLSL UBO block 内 member **宣言有無** only」(= 安全側 dummy 投入) を採用 (b) 実装 phase 入口で `indra/newview/app_settings/shaders/class*/` 配下全 `.glsl` の `layout(std140)` block を grep + member 参照解析 (= `glsl_unused_member` linter pass) を試走 (c) 判定閾値 = false positive 率 ≤ 1% (= 「未宣言だが実 GPU で参照ある」事案件数 / 全 UBO 数) (d) PASS (≤ 1%) → member 参照解析昇格、chapter 08 §17 (P-future) 「member 解析採用」確定 / FAIL (> 1%) → default 維持、(P-future) 「宣言 only 維持」確定 |
| (cache-grow) cache file (= `codegen_state.json`) サイズ上限 / 古 entry GC | (a) 実装 phase で `build-{linux,darwin,windows}*/codegen_state.json` (= Codegen 出力位置、§4 (P3) 配置確定後 fix) のサイズを毎 build 後測定 (b) 対象 = AYAstorm 3 OS build artifact (c) 判定閾値 = サイズ ≥ 10 MB (= 推定 GLSL 1500 file × 8 byte hash + member meta 想定上限) (d) PASS (< 10 MB) → GC 不要 / FAIL (≥ 10 MB) → chapter 09 §10.1 sub-task pool に「mtime ≥ 30 days entry GC + post-GC size shrink check」task 投入 (= 持越紐付け確定済) |

### §2.4 chapter 06b §8 (default 確定済 + chapter 07 / Phase 進行中で消化)

| 項目 | 消化先 | default 採用案 |
|---|---|---|
| (L) per-draw Vulkan 最適化 (L1 ring buffer / L2 dynamic offset / L3 sub-allocation) | chapter 07 / 06c | **L1 + L2 組合せ** (06b §5.3) |
| (M) thread-safe 化方式 (mutex / atomic / lock-free) | chapter 07 | **現 phase atomic** (06b §5.4.3) |
| (U1) per-frame triple-buffering buffer 個数 | chapter 07 | **3 (triple-buffering 標準)** (06b §4.3) |
| (U2) per-asset / per-skin dirty 判定方式 | chapter 07 | **既存 owner 変化検知 + Vulkan 側 dirty bit 両立** (06b §5.4) |
| (U3) flush timing で per-program ↔ per-draw 境界 | (H1b) hook 計測後再評価 | **bind 直後 upload (per-program) / draw 直前 upload (per-draw) で分離** (06b §4.1) |
| (U4) `mValue` cache 適用外 5 method の Vulkan dirty 判定 | chapter 07 / Phase 進行中 | **stage 1 bypass + stage 3 dirty bit のみで dedup** (06b §8) |

### §2.5 chapter 06c §10 (V1)(V2)(V3)(S3) のうち Phase 0 計測連動

| 項目 | 消化先 | default 採用案 |
|---|---|---|
| (V1) set=1 80 binding device limit 懸念 | chapter 07 | **default 80 binding 1 set、device limit 検知時に split 検討** → **(V1') で 40/40 split** (= chapter 07 §3.2 で V1 → V1' に派生、本 chapter §1.1 で集約) |
| (V2) inventory §3.3.1 同一 binding 複数 UBO 名疑い (= ClipPlane / SkinnedVelocity 等 5 個) | 実装 phase 入口 (= `06a-prep` §3 (E')) | **A/B/C 案いずれか、Phase 0 計測待ち** |

### §2.6 chapter 06a §9 (T1) glUniform4iv 内部 glUniform1iv bug 疑い

| 項目 | 消化先 |
|---|---|
| (T1) `glUniform4iv` setter 内部で `glUniform1iv` を呼んでいる bug 疑い (inventory §4.3 line 2330) | **本 migration scope 外、別軸 bug fix track で扱う** |

### §2.7 Phase 0 計測結果由来 追記候補 (= 2026-06-03 ST-4 batch、§5.5.5 dead candidate + §5.5.7 matrix per-draw 補正 / 8 件)

2026-06-03 ST-4 batch (= 「全 default 採用」AYA 応答) で chapter 10 §2 (= 実装 phase 入口で消化) に **8 件追記確定**。本 §2.7 は判断不要 listing (= grep / 計測 / 補正実施で消化)、source は 06a-prep §5.5.5 (dead candidate 121 件中 3 件) + §5.5.7 (matrix 系 per-program → per-draw 補正 4-5 件) + §5.5.5 terrain `detail_*` 20 件。

| # | 候補 ID | 内容 | 消化方法 | 反映先 | 状態 (2026-06-03 ST-6 前段 grep) |
|---|---|---|---|---|---|
| 1 | (R-AYA1) | `aya_alpha_plate` reserved 318 件中 observed 0 | grep `aya_alpha_plate` で hash 経由配線 / dead path / shader 種別確認 | Phase 1.A 入口 grep、結果次第で inventory §7 残課題 (= dead 削除候補) / `LLStaticHashedString` 経由配線記録 | **dead 確定** (= reserved push (llshadermgr.cpp:1902) + enum `AYA_ALPHA_PLATE` 宣言 (llshadermgr.h:423) のみ、`AYA_ALPHA_PLATE` 参照 0 件 / shader 使用 0 件 / `LLStaticHashedString` 経由 0 件 = full dead path、inventory §7 残課題 dead 削除候補) |
| 2 | (R-AYA2) | `aya_alpha_plate_enabled` 同上 (= AYAstorm 独自 setting path 由来 reserved 登録) | 同上 | 同上 | **dead 確定** (= reserved push (llshadermgr.cpp:1903) + enum `AYA_ALPHA_PLATE_ENABLED` 宣言 (llshadermgr.h:424) のみ、参照 0 件 / shader 使用 0 件 / hashed 経由 0 件 = full dead path、(R-AYA1) と同等 dead 削除候補) |
| 3 | (R-AYA3) | `aya_sss_skin_flag` 同上 (= reserved 登録 + MaterialUBO_Legacy member 名と同名 = §4.6.2 で確認、二重配線疑い) | 同上 + MaterialUBO_Legacy member 経路と reserved 経路の両方挙動確認 | 同上 + Q26-MUL 構造改修と整合確認 | **alive 確定** (= reserved push (llshadermgr.cpp:1611) + enum `AYA_SSS_SKIN_FLAG` 宣言 (llshadermgr.h:138) + setter 4 site (lldrawpool.cpp:1093 / lldrawpoolavatar.cpp:906/950 / lldrawpoolmaterials.cpp:157 で `getUniformLocation(LLShaderMgr::AYA_SSS_SKIN_FLAG)`) + shader 3 file 使用 (class1/deferred/avatarF.glsl / class1/deferred/pbropaqueF.glsl / class3/deferred/materialF.glsl)、各 shader で `#ifdef LL_VULKAN_GLSL` guard により OpenGL `uniform float aya_sss_skin_flag` ↔ Vulkan UBO member (= `AvatarFParamUBO_Legacy` (set=3, binding=54) 等 per-shader 別 Legacy UBO) の **dual-path 既移植済**、§2.7 source の「MaterialUBO_Legacy member 名と同名 = 二重配線疑い」は実体 = path 切替 `#ifdef` で意図的 dual declaration = 二重ではない、Q26-MUL `MaterialUBO_Class3_Legacy` 改名 sweep 対象に materialF.glsl の member 名揃え確認のみ、削除/統合 task は不発生) |
| 4 | (R-MAT1) | `modelview_matrix` per-program 推定 → 観察 437/688 cpf, 93/97 shader = per-draw 確定 | chapter 05 §7.3 集約表で cadence 列 per-program → per-draw 上書 | chapter 05 §7.3 (= chapter 06b 起案直前で反映)、06a §0.2 cadence 推定表整合 | **完了** (= 観察値 chapter 05 §7.3.4 + 本表 内容列に literal 転記済、per-draw 確定) |
| 5 | (R-MAT2) | `inv_modelview` 同上 | 同上 | 同上 | 同上 |
| 6 | (R-MAT3) | `modelview_projection_matrix` per-program 推定 → 観察 375/619 cpf, 37/46 shader = per-draw 確定 | 同上 | 同上 | 同上 |
| 7 | (R-MAT4) | matrix 系 group 4 件目 = `normal_matrix` (= indra/ 172 occurrences、shader 内 normal transform per-draw 性質と物理整合)、ST-6 前段 (b) grep verify で前 commit candidate `modelview_projection_inverse` は indra/ 0 件 = 訂正撤回、group 5 件目 candidate も A 案採用で撤回 (= matrix 系 per-draw 確定総件数 4 件) | 06a-prep §5.5.7 group-level 観察値 (= group rate > 100 → per-draw 補正) を group 4 件全件適用、`normal_matrix` 個別 cpf 再計測なし (= 「再計測しない」規律準拠) | chapter 05 §7.3.4 + 06a §0.2 連動 | **完了** (= group 4 件 = `normal_matrix` 1 件のみ確定、§5.5.7 group-level 適用、`modelview_projection_inverse` 撤回) |
| 8 | (R-TERR) | terrain `detail_*` 20 件 (= terrain 6 ch 別 PBR sampler 群、2 scene 共 dead candidate) | AYA test 2 scene = terrain 不在のため別 scene (= PBR terrain region) で再計測必要 | **別 phase task 化候補** (= Phase 1.A 中盤以降、Cocobolo Island / Roleplay Heaven 以外の PBR terrain scene で計測 re-run)、現 phase での消化不要 | 別 phase task (= 本 entry 範囲外) |

**§2.7 反映規律**: (R-AYA1)-(R-AYA3) と (R-MAT1)-(R-MAT4) は **chapter 06b 起案前提整備** (= cadence 5 種 update site 設計で per-draw / per-program 帯分類が正確であること) に直接連動 = ST-6 起案直前に消化。(R-TERR) は別 phase task 化、ST-6 起案には影響なし。

**2026-06-03 ST-6 前段 (a) R-AYA1/2/3 grep 完了** (= 本 §2.7 状態列に反映): (R-AYA1) = **dead 確定** / (R-AYA2) = **dead 確定** / (R-AYA3) = **alive 確定** (= dual-path 既移植済、Q26-MUL 改名 sweep で member 名揃え確認のみ、削除/統合 task 不発生)。**判定第 3 カテゴリ「alive」追加経緯**: handoff §3.4 規律 2 は「dead か hashed か」マーク前提だったが、R-AYA3 grep 実体 = reserved enum 経由 + setter 4 site + shader 3 file + Vulkan UBO 移植済 = 「alive (= 通常配線 + 移植済)」が第 3 カテゴリで、`feedback_doubt_self_first` / `feedback_admit_unknown` 準拠で実体優先記述。chapter 06b cadence 5 種 update site 設計に与える影響 = (R-AYA1)(R-AYA2) は cadence 議論対象外 (= 死) / (R-AYA3) は per-draw cadence (= avatar / object material draw 単位の SSS skin marker、observed 0 件は AYA test 2 scene の SSS 非該当 condition 由来で「実は alive」が物理一貫、計測上 dead に見えた現象は scene 条件由来 dead candidate = 06a-prep §5.5.5 false-positive 1 件として認識)。

**2026-06-03 ST-6 前段 (b) R-MAT1-4 cadence column 反映完了** (= 本 §2.7 状態列 + chapter 05 §7.3.4 に反映): R-MAT1/2/3 (= named 3 件 = `modelview_matrix` / `inv_modelview` / `modelview_projection_matrix`) は 7735505d03 commit (= ST-4 batch) で §5.5.7 literal 観察値 (437-688 cpf, 93/97 + 37/46 shader) 転記済、本 (b) で状態列「完了」マーク追加。R-MAT4 = **`normal_matrix` 1 件で確定** (= A 案採用、前 commit candidate `modelview_projection_inverse` は ST-6 前段 (b) grep verify で indra/ 0 件 = 訂正撤回、group 5 件目 candidate も A 案採用で撤回)。matrix 系 per-draw 確定総件数 = **4 件** (= R-MAT1/2/3/4)、§5.5.7 group rate > 100 → per-draw 補正 group-level 観察値を 4 件全件適用、`normal_matrix` 個別 cpf 再計測なし (= 「再計測しない」規律準拠)。chapter 06b cadence 5 種 update site 設計に与える影響 = matrix 系 4 件全て **per-draw cadence (= set=3 帯) の update site 設計対象**、per-program (= set=1 帯) update site から除外 (= chapter 06b §N で「matrix 系 4 件は per-draw として配線」を起案前提条件とする)。

---

## §3 chapter 09 §12 持越項目 (= AYA 判断後に確定される項目)

### §3.1 K の確定値

- 現時点: 「5-20 程度」とのみ記載 (chapter 09 §11 / §12)
- 確定条件: (Q1) 第 1 UBO migration template + (Q2) Phase 当たり UBO 数 確定後
- 反映先: chapter 09 §2.1 Phase 全体マップ表で K = 具体値に書換え

### §3.2 per-Phase 担当者役割分担

- 現時点: AYAstorm release flow (memory `feedback_release_flow`) で AYA 主導が原則
- 確定条件: AYA / @t-noami 役割分担を AYA 判断で確定
- default: AYA 主導 (Linux 全 Phase + Win) / @t-noami は Mac 担当 (= Mac 不所持制約 `feedback_mac_only_fixes_accept_as_is`)、Claude は commit まで
- 反映先: chapter 09 §6.1 / §6.2 3 OS 確証 Phase

### §3.3 (Q1) Template 確定後の具体 UBO 順

- 現時点: chapter 09 §11.1 で 3 template (A/B/C) 提示、具体 UBO 順は Phase 0 計測待ち
- 確定条件: (Q1) Template 確定 + Phase 0 計測結果 (cadence 別 update 頻度 + per-frame / per-program / per-draw 境界)
- 反映先: chapter 09 §5.2 template → 具体 UBO 順 (Phase 2 / Phase 3 / ... の per-Phase scope)

---

## §4 chapter 04 / 05 / 06a / 06b / 06c 内 live 表 (= 本 chapter で持たず該当 chapter 内に保持)

各 chapter §N の live 表は migration 進行で update する性質、本 chapter には集約しない (= 該当 chapter 内が source of truth)。本節は pointer のみ。

| chapter | live 表 | 役割 |
|---|---|---|
| chapter 04 §10 | (D) shader 内動的 uniform 名 (array flatten 等) の存在確認 | chapter 06 起案時 grep で消化済 (= 06a §5 path 分岐内に dummy 名解決 logic 配線済) |
| chapter 05 §7.3 | bare uniform → UBO 集約対応表 | migration 進行で 候補 → 確定 → 移行済 マーク |
| chapter 05 §10 (H1)(H2)(H3) | bare uniform 集合 enumerate / 集約表 owner / conflict 判断ループ | chapter 06 起案時 grep + LL_INFOS hook で部分消化、live 表で残継続 |
| chapter 06a §9 (H1b)(Q1)(Q2)(R1)(S1-存在)(S1-代替) | cache 構造 / `mUseUBO` flag / `forwardToUboUpload` interface / `LLStaticHashedString` 67 個 UBO 化対象有無 / API 存在確認 (= (S1-存在) 解消済) / §4.3.1 代替案 S1-A/B/C/D 採用確定 (= (S1-代替) = Q24-S1 AYA 判断仰ぎ未消化) | chapter 06b / 06c / 07 / Phase 0 で消化進行中 ((S1-代替) は 18 件修正 batch で chapter 10 §1 新規 Q24-S1 登録予定) |
| chapter 06b §8 (K)(L)(M)(U1)(U2)(U3)(U4) | dirty 粒度 / per-draw 最適化 / thread-safe / triple-buffer / dirty 判定 / flush timing / mValue 適用外 5 method | (K) は本 chapter §1.4 で集約、その他は §2.4 listing 済 |

---

## §5 inventory §7 残課題接続 (= 設計 chapter 群でのカバー状況)

`ayastorm-r41-ubo-current-state-inventory.md` §7 の 9 項目について、設計 chapter 群でのカバー状況を確認。未カバー項目は §6 で「設計 chapter 未カバー」として明示。

| # | inventory §7 項目 | カバー状況 |
|---|---|---|
| 1 | set=2 同一 binding 複数 UBO 名疑い (§3.3.1) | **06c §10 (V2) + 06a-prep §3 (E')(Q27-CONFL)** = Phase 0 Step 1 で C/B 案反証 + A 案純粋形不成立 + V/F 共存 risk 顕在化 → §1.5 (Q27-CONFL) で構造改修 |
| 2 | `LLGLSLShader::UB_*` enum 全件 | **✓ 解消 (2026-06-03)** (inventory §4.1 / 06c §3.4) |
| 3 | set=3 Legacy の正確な個数 | **✓ 解消 (2026-06-03)** (inventory §3.4 / 05 §3) |
| 4 | `FrameViewProj` / `FrameLights` / `FrameAtmosphere_Lighting` member 一覧と他 UBO/bare uniform との重複 | **05 §3 + 06a-prep §3 (E')** = Phase 0 計測でカバー |
| 5 | `MaterialUBO` と `MaterialUBO_Legacy` の関係 | **05 §5 / §10 (F)(Q26-MUL)** = 本 chapter §1.5 でカバー (2026-06-03 Phase 0 Step 1 で (F)=F2 narrowing 確定 + (Q26-MUL) sub-question 新規分離) |
| 6 | `LLGLSLShader::uniform*fv()` redirect 痕跡 | **✓ 解消 (2026-06-03)** (inventory §4.3) |
| 7 | upstream Firestorm との UBO blueprint 差分 | **設計 chapter 未カバー** (= §6 で明示) |
| 8 | OpenGL path 棚卸し外 SSBO / image binding 有無 | **設計 chapter 未カバー** (= §6 で明示) |
| 9 | 典型 scene 実 instance 数 (N = rezzed GLTF asset / M = rigged GLTF Skin) | **06a-prep §3 (H1b) hook** = Phase 0 計測でカバー |

---

## §6 設計 chapter 未カバー項目 (= 実装 phase / 別 track で消化)

inventory §7 残課題のうち、設計 chapter 群 (01-10) でカバーされていない 2 項目:

### §6.1 inventory §7 #7 upstream Firestorm との UBO blueprint 差分

- **現状**: 未調査
- **必要性**: 原則 1 (= upstream 取り込みやすさ維持) と直結、divergence 量が大きいと merge cost 増
- **消化方法**: upstream Firestorm HEAD と本 branch の GLSL UBO 宣言 diff、AYAstorm 独自追加 UBO listing
- **消化先**: 実装 phase 入口 (= η-29 Phase 0 計測 session) で grep + diff 実施、結果を chapter 05 §3 / inventory §3 に追記
- **AYA 判断不要** = 客観的事実調査、結果反映のみ

### §6.2 inventory §7 #8 OpenGL path 棚卸し外 SSBO / image binding 有無

- **現状**: 未調査
- **必要性**: SSBO / image binding が存在すれば UBO migration scope 外として明示必要、scope 漏れリスク
- **消化方法**: `glBindBufferBase` の non-`GL_UNIFORM_BUFFER` target 全件 grep、`glBindImageTexture` 全件 grep
- **消化先**: 実装 phase 入口 (= η-29 Phase 0 計測 session) で grep 実施、結果を inventory §1 / §2 に追記
- **AYA 判断不要** = 客観的事実調査、結果反映のみ

---

## §7 chapter 10 完了 → implementation-phase 入口

### §7.1 design-phase 完了マーク

- 設計 chapter 群 **01-10 全件起案完了** = design-phase 完了マーク
- 01-overview §4 進捗表で chapter 10 を ✅ 起案済 に update
- chapter 10 = AYA 判断仰ぎ候補集約 + listing chapter のため、AYA 判断完了で全 chapter §N default → 確定形書換えが連鎖発動

### §7.2 implementation-phase 入口 = η-29 Phase 0 計測 session

- 入口 handoff doc 起案 (= 本 chapter 10 完了 → AYA commit 指示後の次々 session)
- 入口手順書 = `06a-prep-phase0-measurement.md` (= 計測 spec / 解析 spec / 反映 flow)
- 入口 pre-flight check (chapter 09 handoff §5 で確定済):
  - chapter 06a-prep §7 (P1)-(P4) 4 件 grep 消化
  - AYAstorm build flow 確認 (memory `project_build_procedure`)
  - log path 確認 (memory `reference_log_path`)
  - 計測 build flag `-DAYASTORM_UBO_CADENCE_HOOK=ON` 配置先確定

### §7.3 η-29 Phase 0 完了 → Phase 1.A 入口

- (H1b) hook 計測完了 → (Q1) Template 確定の入力データ揃う
- (E') 同一 binding 複数 UBO 名疑い解消 → Phase 0 Step 1 で C++ name attach 0 件 + 全 5 UBO `set=2 binding=0 std140` 確定済 + V/F 同 program 共存 risk 顕在化 → (V2) 確定は (Q27-CONFL) 構造改修方針確定後
- (F) MaterialUBO 比較 → **Phase 0 Step 1 で F1/F3 反証 + F2 narrowing 確定済**、(Q26-MUL) 構造改修方針が次の判断対象
- (RF) reflection update fence throttle 判定 → 07 §12 (RF) 解消
- Phase 1.A = Codegen + redirect 層整備 (= chapter 08 §17 7 件 + chapter 09 §10.1 持越紐付け) + (Q26-MUL) 構造改修反映

---

## §8 本 chapter update 規律

- 本 chapter は **live doc**、AYA 判断確定 / Phase 完了で都度反映
- AYA 判断確定で §1.1 / §1.2 / §1.3 / §1.4 / §1.5 表の default → 確定形マーク、各 chapter §N も連鎖 update
- 実装 phase 入口で §2 各項目 (= (P1)-(P4) / (RF) / (V2) 等) 消化 → 該当節を「解消済」マーク
- inventory §7 #7 / #8 (= §6.1 / §6.2) は実装 phase 入口で調査 → 解消マーク + inventory 本体更新
- chapter 10 完了 / 各 chapter 確定形書換え完了 で 01-overview §4 進捗表を update
- 実装 phase 進行で Phase 完了毎に §7.3 以降の Phase 入口 handoff doc を起案、本 chapter §3 (K の確定値 / 担当者 / 具体 UBO 順) を順次確定形に書換え

### §8.1 spot check 持越 task (= 次 session で消化)

- **task**: §1.1 〜 §1.5 全 21 件 AYA 判断仰ぎ候補の **「AYA 判断後の反映先」 §N が実 chapter §N で実在するか** を pinpoint Read で全件確認
- **判定基準**: 各 reflect §N について該当 chapter §N の本文を Read → default 採用案の reflect 先として意味的に整合するか check、ズレあれば §N 修正
- **対象範囲**:
  - §1.1 (4 件) → chapter 07 §3.2 / §4.2 / §5.2 / §6.2 + 06c §3 / §8 + chapter 04 §5.1
  - §1.2 (7 件) → chapter 08 §1-§16 各節 (= (A1)/(P)/(G/B3)/(B1)/(B2)/(B4)/(B5) ごとの owner §N) + chapter 04 §3 / §5 / §7
  - §1.3 (5 件) → chapter 09 §2.1 / §5.2 / §6.1 / §7.1 / §3.1 / §3.2
  - §1.4 (4 件) → 06b §3.2.3 / §5.2 + 06c §2 / §3 / §6 / §3.4
  - §1.5 (1 件) → chapter 05 §5 / §7.3 + 06a-prep §3
- **trigger**: 本 chapter 10 完了 AYA commit 後、AYA 判断確定 session 前に handoff doc 起案 + 次 session で消化
- **失敗時 fallback**: ズレ ≥ 1 件発見時は本 §8.1 task 結果に基づき該当 §1.X 表の reflect 文を修正、AYA 判断仰ぎ前に整合確保

---

**= 本 chapter で設計 chapter 群 (01-10) 全 chapter の chapter 10 持越項目 (= AYA 判断仰ぎ候補 + 実装 phase 消化候補 + chapter 09 §12 持越 + live 表 pointer + inventory §7 残課題接続) が集約された**。AYA 判断完了で各 chapter §N の default → 確定形書換えが連鎖発動、design-phase 完了マーク → implementation-phase 入口 (= η-29 Phase 0 計測) に移行可能 state 到達。
