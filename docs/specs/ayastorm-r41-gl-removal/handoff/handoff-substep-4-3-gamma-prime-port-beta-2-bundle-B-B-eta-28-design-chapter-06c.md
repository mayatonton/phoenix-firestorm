# sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 handoff: 設計 chapter 06c 起案 + chapter 06 (06a/06a-prep/06b/06c) 全件起案完了 → AYA commit 指示待ち

**完了日**: 2026-06-03
**位置付け**: 前 handoff (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-chapter-06b.md`、chapter 06b 起案 + cadence update site / dirty / flush / `forwardToUboUpload` interface 確定完了) を受けて、**chapter 06c 起案完了** (= descriptor set 4 帯 cadence 別配置 + UB_\* 4 binding ↔ 84 blueprint 接合表 + flush 直後 bind 配線 + dynamic offset (L2) bind 側責務 + `mUseUBO` initial 設定方針 + PSO compatibility + triple-buffering rotate + sampler bridge) した state。chapter 01 §4 進捗表を 06c ✅ 起案済 に update。**chapter 06 (06a/06a-prep/06b/06c) 全件起案完了 = AYA 明示 commit 単位到達**、AYA の commit 指示待ち。次 session は AYA 指示後 commit phase 入り → chapter 07 (vulkan-api-state) 起案へ進む。

---

## §0 本 session で完了した作業

### §0.1 設計 chapter 起案

| chapter | file | 規模 | 主内容 |
|---|---|---|---|
| 06c | `design/06c-descriptor-set-bind-wiring.md` | 497 行、§0-§11 (12 section) | scope vs 非 scope / 入力契約 / descriptor set 4 帯 cadence 別配置 (= set=0 per-frame+singleton 4 binding / set=1 per-program 79 binding / set=2 per-draw 4 binding / set=3 per-asset+per-skin 3 binding、M1 採用) / UB_\* 4 binding ↔ 84 blueprint 接合表 (= GL/Vulkan binding namespace 独立、O1 採用) / flush 直後 bind 配線 (= 06b §4.1 flush 関数 5 種 → bind sequence diagram) / cadence 跨ぎ rebind 範囲 / PSO compatibility ((V3) chapter 07 持越) / dynamic offset (L2) bind 側責務 (= dynamic_offsets[] 構築 + vkCmdBindDescriptorSets uint32_t 配列 配線) / `mUseUBO` initial 設定方針 (= N2 採用 = `LLPhaseMigrationList::isUboReady()` 経由 shader 単位 phase migration、3 debug cvar `AYAUboRedirectEnabled` / `AYAUboPhaseMigrationOverride` / `AYAUboShaderWhitelist` 提案) / triple-buffering 配下 set=0 rotate (= 方式 A = 3 descriptor set rotate 採用) / Global_ReflectionProbes 例外扱い (= rotate 不要、1 物理 buffer) / sampler 49 個 chapter 07 bridge (= S3 採用 = 配置決定 chapter 07 譲り) / 04-05-06a-06b-07-09 分担境界 / 未確定 7 件 (M/N/O/V1/V2/V3/S3) / update 規律 |

### §0.2 06c 起案の波及 reflect (= 整合 update)

| update 先 | 内容 | 状態 |
|---|---|---|
| chapter 01 §4 (進捗表) | 06c 行を「未起案」→「✅ 起案済」へ update | unstaged |

(他 chapter への直接波及なし — 06c は 06a の `mUseUBO` 配置を受けて initial 設定方針を確定する側、06b の flush 関数 5 種を受けて bind 配線する側、chapter 02-05 の確定事項を消費する側、chapter 07 へ譲る側)

### §0.3 確定した設計判断 (= Claude default 推奨、AYA 判断仰ぎ対象)

| # | 論点 | Claude 推奨 default | 譲り先 |
|---|---|---|---|
| (M) | descriptor set 4 帯 ↔ cadence 5 分類 配置 | **M1 = 1:1 配置** (set=0 per-frame+singleton / set=1 per-program / set=2 per-draw / set=3 per-asset+per-skin、§2.1) | chapter 10 / AYA 判断 |
| (N) | `mUseUBO` initial 設定 | **N2 = shader 単位 phase migration** (= `LLPhaseMigrationList::isUboReady()` whitelist 経由、§6.2) | chapter 10 / AYA 判断 |
| (O) | UB_\* 4 binding 拡張方針 | **O1 = 既存 4 種維持 + 新規追加** (= GL/Vulkan binding namespace 独立、call site 改変ゼロ、§3.4) | chapter 10 / AYA 判断 |
| (V1) | set=1 が 79 binding で device `maxDescriptorSetUniformBuffers` 限界懸念 | default 79 binding 1 set、device limit 検知時に split 検討 (§2.3) | chapter 07 |
| (V3) | set=1 layout = 全 program 共通 79 binding (= dummy 埋め) vs program 別 layout | 全 program 共通 (= PSO compatibility 最大、§4.3) | chapter 07 |
| (S3) | sampler 49 個の descriptor set 配置帯 | 配置決定 chapter 07 譲り、§8.2 で S1 (= set=4 sampler 専用) / S2 (= set=1 拡張) / S3 (= chapter 07 譲り) 3 案併記 | chapter 07 |
| 方式 A | triple-buffering 配下 set=0 rotate | **A = 3 descriptor set rotate** (§7.2、update cost ゼロ + memory footprint +2 set 微小) | chapter 07 確定 |

### §0.4 解消した持越 item

| # | 項目 | 解消形 |
|---|---|---|
| (Q1) | `mUseUBO` flag initial 設定方針 | **本 chapter §6 で N2 = `LLPhaseMigrationList::isUboReady()` 経由 shader 単位 phase migration 採用**、3 debug cvar 案 (`AYAUboRedirectEnabled` / `AYAUboPhaseMigrationOverride` / `AYAUboShaderWhitelist`) 提案、whitelist 具体形は chapter 09 譲り |

### §0.5 design-phase 規律 (= 前 handoff §0.5 / §0.6 規律) 完全遵守

- `indra/` 配下改変ゼロ (= memory `feedback_design_phase_no_code_write`)
- spec doc 内の file path / line / code shape / 配線位置記述は記載 (= ルール境界、memory に明記済の通り)
- 06c §3 接合表 / §4 bind sequence / §6.3 `LLGLSLShader::link()` 末尾 hook で **既存 OpenGL upload site の file path / line を inventory § 1 / chapter 02 §2.3 / 06a §3.2 から継承して明記** (= `LLGLSLShader::link()` / `LLReflectionMapManager::mUBO` / `gltf::Asset` / `gltf::Skin` 等)

---

## §1 次 session の作業 (= AYA commit 指示後の commit phase 入り → chapter 07 起案着手)

### §1.1 次 session = AYA 指示後 commit phase + chapter 07 起案

| 項目 | 詳細 |
|---|---|
| commit phase 入口 | chapter 06 (06a/06a-prep/06b/06c) 全件起案完了 = AYA 明示 commit 単位到達、本 session 完了時点で **AYA に「06 揃いました、commit してください」報告済 → AYA commit 指示待ち** |
| commit 単位 (= AYA 明示 2026-06-03) | unstaged: chapter 01 §4 進捗表 update / chapter 06b §4 / 本 session で更新した chapter 01 §4 (06b → ✅ 起案済 ↔ 既存 + 06c → ✅ 起案済 ↔ 本 session 追加)、untracked: `design/06b-cadence-update-site-and-dirty.md` (前 session) + `design/06c-descriptor-set-bind-wiring.md` (本 session) + `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-chapter-06b.md` (前 session) + `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-chapter-06c.md` (本 session) |
| commit 後 = chapter 07 起案着手 | `vulkan-api-state.md` 起案、VMA / descriptor pool / vkCmdBindDescriptorSets / vkUpdateDescriptorSets / device limit query / PSO layout / triple-buffering 実装 / sampler 配置 (S1/S2/S3 確定) / cadence 跨ぎ rebind 範囲確定 |
| 起案完了判定 | chapter 07 単独 ~370-430 行見込み (06a/06b/06c 同等規模) |

### §1.2 chapter 07 (vulkan-api-state) scope (= 06c §10 持越 + 既存設計 chapter 07 役割)

| 項目 | 詳細 | 06c からの input |
|---|---|---|
| 現状 Vulkan API 実装棚卸し | `vkQueueSubmit` / swapchain / descriptor pool 現状 + 欠落箇所 | inventory §1 base |
| descriptor set layout 確定 (V1)(V3) | set=1 が 79 binding で device limit 検知 + 全 program 共通 layout vs program 別 layout | §2.3 (V1) / §4.3 (V3) |
| sampler 49 個配置確定 (S3) | S1 (= set=4 sampler 専用) / S2 (= set=1 拡張) / 新案 のいずれか確定 | §8.2 |
| dynamic offset (L2) 実装 | `vkCmdBindDescriptorSets()` dynamic_offsets[] uint32_t 配列構築 + 配置タイミング | §5 |
| triple-buffering 実装 | 方式 A (= 3 descriptor set rotate) の VMA / descriptor pool 配線 + Global_\* fence sync | §7 |
| PSO compatibility | pipeline layout cache + cadence 跨ぎ rebind 範囲 | §4.3 |
| device limit query | `vkGetPhysicalDeviceProperties()` で `maxDescriptorSetUniformBuffers` / `maxBoundDescriptorSets` / `maxDescriptorSetSamplers` 取得 + split 判定 | §2.3 (V1) |

### §1.3 chapter 07 起案で AYA 判断を仰ぐ可能性のある論点 (= 暫定)

| # | 論点 | 影響 |
|---|---|---|
| (V1') | device limit 不足時の set=1 split 方式 | program 別 layout 強制 / cadence 拡張 / dummy 埋め放棄判断 |
| (S1/S2/S3 final) | sampler 49 個配置の S1/S2/別案確定 | chapter 06c §3 接合表追記 + chapter 06a §5.6 sampler skip 実装範囲確定 |
| (W) | descriptor pool 容量算定 (= 86 binding × N_buf × per-asset/per-skin instance × estimate) | VMA allocator 設計 / 起動時 memory footprint |

### §1.4 次 session 着手前の前提読み込み (= pre-requisite、順序固定)

1. `design/01-overview.md` (用語定義 + 設計原則 + 確定事項 13 件、§4 進捗表で 06c ✅ 確認)
2. `design/02-naming-convention.md` (命名規則 + rename 表)
3. `design/03-cadence-classification.md` (cadence 5 分類)
4. `design/04-codegen-ubo.md` (Codegen-UBO 機構 / `UniformLocation` struct / R3 path)
5. `design/05-existing-inventory-link.md` (既存 inventory link + bare uniform 集約 framework + G1 確定)
6. `design/06a-cache-structure-and-setter-redirect.md` (cache 構造 + setter redirect + `mUseUBO` flag 配置)
7. `design/06a-prep-phase0-measurement.md` (Phase 0 計測 spec、実装 phase 入口手順書)
8. `design/06b-cadence-update-site-and-dirty.md` (cadence update site + dirty + flush + `forwardToUboUpload` interface)
9. `design/06c-descriptor-set-bind-wiring.md` (本 session 起案、descriptor set bind 配線 + UB_\* binding 4 種 vs 84 blueprint 接合)
10. `ayastorm-r41-ubo-current-state-inventory.md` (現状棚卸し、live doc、§3.3 UB_\* 4 binding 確定)
11. 本 handoff doc (本 session 完了状態 + chapter 07 scope + 持越 reset)

**= この 11 件で次 session の文脈は完全 reconstruct 可能**。

### §1.5 次 session 推奨進め方

- AYA から commit 指示が来たら **明示パス指定で stage** (= `git add -A` / `git add .` 禁止、`tests/` 厳守除外、memory `feedback_tests_dir_never_commit`)
- commit 後、AYA から push / session clear 指示があれば従う、無ければそのまま chapter 07 起案へ
- chapter 07 起案着手前に `01-overview.md` 〜 `06c` 全件 + inventory + 本 handoff の 11 件読込
- chapter 07 起案完了で chapter 07 ✅、残 chapter 08 (build-codegen-pipeline) / 09 (phase-roadmap) / 10 (open-questions)
- design phase 完了後の **実装 phase 入口で `06a-prep-phase0-measurement.md` を再読込** → Phase 0 計測 (H1b)(E')(F) 実施 → 結果を chapter 05 / 06a / 06b / 06c に反映

---

## §2 残持越 item (= 全 chapter の §10 / §8 / 本 handoff からの累積)

### §2.1 chapter 06c §10 持越 (= 本 session 新規)

| # | 項目 | 解消先 | default 採用案 |
|---|---|---|---|
| (M) | descriptor set 4 帯 ↔ cadence 5 分類 配置 | **chapter 10 / AYA 判断** | M1 (= 1:1 配置) |
| (N) | `mUseUBO` initial 設定 | **chapter 10 / AYA 判断** | N2 (= shader 単位 phase migration) |
| (O) | UB_\* 4 binding 拡張方針 | **chapter 10 / AYA 判断** | O1 (= 既存 4 種維持 + 新規追加) |
| (V1) | set=1 device `maxDescriptorSetUniformBuffers` 限界 | chapter 07 | default 79 binding 1 set、limit 検知時 split |
| (V2) | inventory §3.3.1 同一 binding 複数 UBO 名疑い | 実装 phase 入口 (= `06a-prep` §3 (E')) | A/B/C いずれか、Phase 0 計測待ち |
| (V3) | set=1 layout = 全 program 共通 vs program 別 | chapter 07 | 全 program 共通 (= PSO compatibility 最大) |
| (S3) | sampler 49 個の descriptor set 配置帯 | chapter 07 | 配置決定 chapter 07 譲り、§8.2 3 案併記 |

### §2.2 chapter 06b §8 持越 (= 前 handoff から継続)

| # | 項目 | 解消先 | default 採用案 |
|---|---|---|---|
| (K) | dirty 判定粒度 = K1 member / K2 UBO / K3 cadence 単位 | **chapter 10 / AYA 判断** | K2 (= UBO 単位) |
| (L) | per-draw Vulkan 最適化 = L1 ring buffer / L2 dynamic offset / L3 sub-allocation | **chapter 07 / 06c** | L1 + L2 組合せ |
| (M_06b) | thread-safe 化方式 (= 06b の M、本 chapter (M) と別軸) | chapter 07 | 現 phase atomic |
| (U1) | per-frame triple-buffering buffer 個数 = 2 / 3 / N | chapter 07 | 3 |
| (U2) | per-asset / per-skin dirty 判定 | chapter 07 | 既存変化検知 + Vulkan dirty bit 両立 |
| (U3) | per-program ↔ per-draw 境界 | (H1b) 後再評価 | 分離 |
| (U4) | `mValue` 適用外 5 method の Vulkan dirty 判定 | chapter 07 / Phase 進行中 | stage 1 bypass + stage 3 dedup |

### §2.3 chapter 06a §9 持越 (= 前々 handoff から継続)

| # | 項目 | 解消先 | 状態 |
|---|---|---|---|
| (H1b) | LL_INFOS hook 実装 + AYA build run + 不明 16 件 cadence 確定 | 実装 phase 入口 | spec 化済 (`06a-prep` §2)、未実施 |
| (Q1) | `mUseUBO` flag の initial 設定方針 | **本 session 06c §6 で解消** | ✅ |
| (Q2) | `forwardToUboUpload(loc, data, size)` 本体実装 | 前 session 06b §5 で解消 | ✅ |
| (R1) | LLStaticHashedString 経由 setter 67 個の核 UBO 化対象外確定根拠の chapter 05 §7.3 反映 | chapter 05 §7.3 切出し時 | 別 file 切出し対象 |
| (T1) | `mapUniforms()` 内 Vulkan path 拡張で OpenGL path との二重維持 cost | chapter 09 Phase 進行中 | 持越継続 |

### §2.4 chapter 04 §10 持越 (= 前々 handoff から継続)

| # | 項目 | 解消先 |
|---|---|---|
| (A1) | std140 offset 計算 (Codegen 独自 vs SPIR-V reflection 抽出) | chapter 08 |
| (G_codegen) | perfect hash generator (gperf / 独自 / frozen) | chapter 08 |
| (P) | parse 手段 (独自 mini-parser vs glslang reflection) | chapter 08 |

### §2.5 chapter 05 §10 持越 (= 前々 handoff から継続)

| # | 項目 | 解消先 |
|---|---|---|
| (E') | inventory §3.3.1 同一 binding 複数 UBO 名疑い | 実装 phase 入口 (= `06a-prep` §3) |
| (F) | MaterialUBO vs MaterialUBO_Legacy 処遇 | 実装 phase 入口 (= `06a-prep` §4) |
| (H2) | chapter 05 §7.3 集約表の別 file 切出し (`05a-bare-uniform-mapping.md`) | chapter 07 起案中の並行作業 |
| (H3) | 集約判定 conflict 時の AYA 判断ループ | chapter 09 Phase 進行中 case-by-case |

### §2.6 前 handoff §2 持越の本 session 消化状態

| # | 項目 | 状態 |
|---|---|---|
| **(Q1)** | `mUseUBO` flag initial 設定方針 | **✅ 本 session 06c §6 で解消 (= N2 採用)** |
| (H1b) | LL_INFOS hook 実装 | spec 化済 (`06a-prep`)、実施は実装 phase 入口 |
| (E') | binding 重複 | → 実装 phase 入口へ移管継続 |
| (F) | MaterialUBO 比較 | → 実装 phase 入口へ移管継続 |
| (R1) | LLStaticHashedString 67 個反映 | → chapter 05 §7.3 切出し時継続 |
| (T1) | `mapUniforms()` 二重維持 cost | → chapter 09 継続 |
| (A1) / (G_codegen) / (P) | std140 / perfect hash / parse 手段 | → chapter 08 継続 |
| (H2) | 集約表別 file 切出し | → chapter 07 並行継続 |
| (H3) | 集約 conflict AYA loop | → chapter 09 継続 |
| (K) / (L) / (M_06b) / (U1) / (U2) / (U3) / (U4) | 06b §8 持越 7 件 | → chapter 07 / chapter 10 / 実装 phase 継続 |

**= 前 handoff 持越のうち、本 session で 1 件 (Q1) が確定。他は次 session (= chapter 07 起案) / 実装 phase 入口 (H1b/E'/F) / 別 chapter (R1/T1/A1/G_codegen/P/H2/H3) に分配済。新規追加は 06c §10 の 7 件 (M/N/O/V1/V2/V3/S3) で、3 件 (M/N/O) は AYA 判断、4 件 (V1/V2/V3/S3) は chapter 07 / 実装 phase 譲り**。

---

## §3 unstaged / untracked 状態 (= AYA 判断対象、commit 単位 = 06 揃い到達)

### §3.1 累積状態

| 種別 | 内訳 |
|---|---|
| staged (前々々 session) | `git mv` で `handoff/` 配下に移動した 97 件 |
| unstaged (前 handoff からの累積に本 session 追加) | inventory doc / chapter 01 (§4 / §5) / chapter 02 (§2.1 / §3.4) / chapter 03 (§2 / §4.3) / 06a §7 (前々 session) + 前 session: chapter 01 §4 進捗表 (06b → ✅ 起案済) + **本 session: chapter 01 §4 進捗表 (06c → ✅ 起案済)** |
| untracked (前 handoff からの累積に本 session 追加) | design/ 配下: 01-overview.md / 02-naming-convention.md / 03-cadence-classification.md / 04-codegen-ubo.md / 05-existing-inventory-link.md / 06a-cache-structure-and-setter-redirect.md / 06a-prep-phase0-measurement.md / 06b-cadence-update-site-and-dirty.md (前 session) / **06c-descriptor-set-bind-wiring.md (本 session 新規)** / handoff/ 配下: 過去 3 件 + 前 session handoff 1 件 + **本 handoff doc 1 件 (本 session 新規)** |
| memory file (`~/.claude/projects/.../memory/`) | (前 handoff `feedback_design_phase_no_code_write.md` から本 session 追加なし) |
| tests/ | **commit 対象外** (= memory `feedback_tests_dir_never_commit` 厳守、確認問いかけも禁止、`?? tests/` をそのまま untracked 放置) |

### §3.2 commit 判断 = 06 揃い到達 (= AYA 明示指示 2026-06-03)

- AYA 指示: 「06 が揃った時点で commit する」 = chapter 06 (06a / 06a-prep / 06b / 06c) 全件完了 + chapter 01 §4 進捗表 update 揃った時点で commit
- **本 session 完了時点で 06 揃い到達** = AYA に「06 揃いました、commit してください」報告 → **AYA commit 指示待ち**
- Claude 側から proactive に commit 実行しない (memory `feedback_no_auto_commit` 既存方針継続) = AYA の commit 指示文を受けてから明示パス指定で stage + commit
- stage は **明示パス指定** で (= `git add docs/specs/ayastorm-r41-gl-removal/...` 個別列挙、`git add -A` / `git add .` 禁止)
- **`tests/` ディレクトリは commit 対象から除外** (memory `feedback_tests_dir_never_commit`、明示パス指定で stage、確認問いかけ自体禁止)
- push / session clear / (H1b) hook 実装 session タイミングも AYA 側で判断、commit 後の次 session 開始時は §1.4 の 11 件読込から再開可能

---

## §4 本 session の感触 (= 設計議論結果の要約)

1. **chapter 06 を 3 sub-chapter (06a / 06b / 06c) + prep 1 件に分割した判断が完全に正解** = 06a 479 行 + 06a-prep 546 行 + 06b 431 行 + 06c 497 行 = 合計 ~1953 行。単一 chapter で context 1 session に収めるのは不可能、3 + prep 分割で各 session に綺麗に収まった
2. **(Q1) `mUseUBO` initial 設定 = N2 採用が phase migration の信頼性を担保** = N1 (= 全 ON) は cold launch 一括 risk、N2 (= `LLPhaseMigrationList::isUboReady()` whitelist) は chapter 09 Phase 進行中の 1 UBO ずつ migration policy (= memory `feedback_ubo_migration_one_at_a_time`) と完全整合
3. **(M) 1:1 配置 (M1) は driver best practice + 06b §4.1 flush 関数 5 種と 1:1 対応** = cadence ↔ set 帯の対応が設計 stack 全体で一貫、PSO compatibility 最大、AYA 判断に出す段階で「M1 default、特殊 case 出たら M2 検討」の構図が綺麗
4. **(O) 既存 UB_\* 4 binding 温存 (O1) が原則 1 (= call site 改変ゼロ) と GL/Vulkan binding namespace 独立性で両得** = 既存 4 種は LL upstream 改修取込時の差分ゼロ、Vulkan space では set=0/2/3 帯に再配置で descriptor set 4 帯設計に整合、設計判断として摩擦ゼロ
5. **(L2) dynamic offset bind 側責務が `dynamic_offsets[] = ringBufferOffset` の 1 行で閉じた** = 06b §5.3 で確定済 (L1+L2 default) の bind 側展開が `vkCmdBindDescriptorSets()` uint32_t 配列構築だけで完結、Vulkan 教科書通り
6. **set=0 rotate 方式 A (= 3 descriptor set rotate) が triple-buffering の Vulkan 教科書解** = `vkUpdateDescriptorSets()` cost ゼロ + memory footprint +2 set 微小、Global_\* は rotate 不要で 1 物理 buffer の例外扱いも綺麗に閉じた
7. **(S3) sampler 49 個 chapter 07 譲りが本 chapter scope を守る判断** = sampler の cadence 帰属 unclear + device limit 実 query 必要 = 本 chapter 配置決定すべきでない領域、scope 切出しが設計品質維持
8. **06c §10 持越 7 件 (M/N/O/V1/V2/V3/S3)** = 全件 chapter 07 / chapter 10 / 実装 phase に分配可能な分解度に達した、本 session で持越が暴発しなかった
9. **chapter 06 揃い到達 = design phase 過半数完了** = 設計 chapter 12 件中 8 件完了 (01/02/03/04/05/06a/06a-prep/06b/06c = 9 件)、残 chapter 07/08/09/10 の 4 件 (= 33%)、design phase 全体進捗は **9/12 = 75%**
10. **AYA 明示の commit 単位 (= 06 揃い) 到達**、AYA 指示後 commit phase 入り → chapter 07 起案へ進む流れが完成

---

## §5 本 handoff doc の更新規律

- 次 session で本 doc を入口として読み込む際、§1.1 commit phase 完了状況 + chapter 07 起案完了状況を本 doc に上書き反映 or 新 handoff doc (`...-design-chapter-07.md`) に引継ぎ
- chapter 07 起案完了時は本 doc + 前 handoff (06b) + その前 (06a) を superseded mark、新 handoff doc に **chapter 07 起案完了 + descriptor set layout / sampler 配置 / device limit / triple-buffering 実装確定** state として引継ぎ
- §3 unstaged / untracked 状態は AYA の commit 判断 (= 本 session 完了直後 or 次 session) に追従して更新、commit 実行後は本 doc §3 を「commit 済 state」へ書換え
- §0.3 default 採用案 (M1 / N2 / O1 / 方式 A) は AYA 判断で確定 / 変更時に本 doc + 06c §10 を同時 update

---

**= 本 handoff doc を次 session の入口として、AYA の commit 指示を受けて commit phase 入り → chapter 06 (06a/06a-prep/06b/06c) 全件 commit → chapter 07 (vulkan-api-state) 起案 → 残 chapter 08/09/10 と段階的に design phase を完了させる流れで再開する**。
