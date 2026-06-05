# sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 handoff: 設計 doc 分割 + chapter 01-03 起案完了

**完了日**: 2026-06-03
**位置付け**: η-28 pivot (= `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-pivot-to-ubo-design.md`) を受けて起こした **UBO 全体設計 doc 群** の chapter 01-03 までを完了した state。次セッションで chapter 04-10 を順次起案する **入口 handoff**。

---

## §0 本 session で完了した作業

### §0.1 ディレクトリ再編

- `docs/specs/ayastorm-r41-gl-removal/handoff/` 新設、既存 handoff doc **97 件全件を `git mv` で移動**
- `docs/specs/ayastorm-r41-gl-removal/design/` 新設、本 design doc 群の格納先

理由 (AYA 指示 2026-06-03):
> 「設計書は 1 つのでかいファイルより複数に別れていたほうが便利な気がするからうまくわけてほしい / 既存と新設をそれぞれ持っていれば更新もしやすい / おそらく進めていくとまた考慮がなかったとかで戻ってくるであるから資料を 1 つに固めるのは章を増やすだけでメンテが悪い / handoff doc も恐ろしい数になってるのでディレクトリを分けてほしい」

### §0.2 設計 doc 起案 (chapter 01-03)

| chapter | path | 内容 |
|---|---|---|
| 01 | `design/01-overview.md` | 全体概観 + 2 大設計原則 (upstream 取込 / Core 分散) + 用語定義 (storage lifetime / cadence / logical binding / physical instance / Codegen-UBO) + chapter 構成 + 本 session 決定済事項表 |
| 02 | `design/02-naming-convention.md` | UBO 命名規則 (`Frame*` / `Program_*` / `Draw_*` / `Material*` / `Asset_*` / `Skin_*` / `Global_*`) + Legacy suffix + C++ enum 規則 + Codegen-UBO 生成識別子 + 既存 84 UBO の rename 表 |
| 03 | `design/03-cadence-classification.md` | cadence 6 分類 (per-frame / per-program / per-draw / per-asset / per-skin / per-material) + cadence source rule (= 既存 C++ 呼出 path のスケジュール) + cadence 別 update site / thread / dirty 判定 / descriptor set 設計 + Core 分散関係 |

### §0.3 本 session で確定した設計判断 (= chapter 01 §5 と同一)

| # | 決定 | 影響 chapter |
|---|---|---|
| 1 | UBO 化アプローチは Codegen-UBO (build-time static codegen) | 04 / 06 / 08 |
| 2 | name-based call site API (`uniform4fv("color", ...)`) を温存 | 06 |
| 3 | cadence source = 既存 C++ 呼出 path のスケジュール (新規 viewer settings 追加禁止) | 03 |
| 4 | per-frame cadence は既存 frame loop (60 FPS 設定) に sync | 03 |
| 5 | 既存 84 GLSL UBO blueprint は **discard しない** (parse error 解消の蓄積を温存) | 05 |
| 6 | storage lifetime は owner class lifetime に従う (新規判断不要) | 06 |
| 7 | 設計判断は **論理 binding 軸 / 物理 instance 軸の両軸で評価** | 全 chapter |
| 8 | UBO migration は **1 UBO ずつ実装 → cold launch 検証 → 次へ** (大塊バッチ禁止) | 09 |

### §0.4 inventory doc 更新 (本 session 中)

`ayastorm-r41-ubo-current-state-inventory.md` を 6 箇所 update (unstaged):

- §1: 論理 binding 4 vs 物理 instance 1+2N+M の二軸明示
- §4.1: `UB_*` enum 表に物理 instance owner 列追加
- §5.1: frame workload を 10-100 回 / frame に再算定
- §6.6: 二軸厳密区別の rule 追加
- §7 #9: 典型 scene N/M 計測 task 追加
- §8.6: 二軸評価 rule 追加

**status**: unstaged。本 handoff doc / 設計 chapter 群と合わせて commit するかは AYA 判断。

---

## §1 次 session の作業 (= chapter 04-10 起案)

| chapter | doc 名 | scope |
|---|---|---|
| 04 | `design/04-codegen-ubo.md` | Codegen-UBO 全体機構 / GLSL → SPIR-V + C++ lookup table 生成 pipeline / build-time pre-process script 仕様 / name-based offset 解決 |
| 05 | `design/05-existing-inventory-link.md` | 既存 84 UBO blueprint の cadence 別 mapping (chapter 03 の cadence 軸で個別判定) / `_Legacy` 統廃合判断 / set=2 vs set=3 重複処遇 |
| 06 | `design/06-redirect-layer-design.md` | host C++ redirect 層 (= `LLGLSLShader::uniform*fv()` family を UBO offset 書込に redirect する mechanism) / dirty 判定 / per-cadence upload site 実装詳細 |
| 07 | `design/07-vulkan-api-state.md` | 現状 Vulkan API 実装状況棚卸し (vkQueueSubmit / swapchain / descriptor set / VMA / volk dynamic loader 等) / chapter 06 で必要な API 不足判定 |
| 08 | `design/08-build-codegen-pipeline.md` | build system 統合 (CMake / glslang 呼出 / preprocess script 配置 / 生成 header の build dir 配置 / 3 OS 互換) |
| 09 | `design/09-phase-roadmap.md` | Phase 番号体系再編 (η-28 Phase 2d-α 以降 / 1 UBO ずつ migration / cold launch 検証点) |
| 10 | `design/10-open-questions.md` | 未確定事項 (cadence source rule §3.4 例外 / inventory §7 残課題 / 次セッション持ち越し) |

### §1.1 次 session 着手前の前提読み込み (= pre-requisite)

次 session の最初に **必ず** 読む doc (順序固定):

1. `design/01-overview.md` (用語定義 + 設計原則 + 本 session 決定事項表)
2. `design/02-naming-convention.md` (命名規則)
3. `design/03-cadence-classification.md` (cadence 軸)
4. `ayastorm-r41-ubo-current-state-inventory.md` (現状棚卸し、live doc)
5. 本 handoff doc (進捗 + 次 chapter scope)

**= この 5 件で次 session の文脈は完全に reconstruct 可能**。 chapter 04+ 起案中に過去 handoff doc (97 件) を 1 件ずつ参照する必要は無い (chapter 01-03 が全集約)。

### §1.2 次 session 推奨進め方

- chapter 04 → 05 → 06 → 07 → 08 → 09 → 10 の順 (= 上から下に依存)
- 各 chapter 完成のたびに **commit (= 細かく刻む) は AYA に確認**
- 1 chapter ≈ 1 session で context を 1 サイクル分使うため、適時 `/clear` で session reset
- chapter 06 (redirect 層) は最大規模、必要なら **sub-chapter に分割** (`06a-` / `06b-`)
- chapter 09 (phase roadmap) で「最初に着手する 1 UBO」を確定 → η-28 Phase 2d-β として実装に戻る入口

---

## §2 設計議論の中で **未解消** で持ち越した item

| # | 項目 | 持ち越し先 chapter |
|---|---|---|
| A | set=2 内 binding 重複疑い (inventory §3.3.1 / `PerDrawUBO_ClipPlane` 他) | 05 (existing-inventory-link) |
| B | set=2 (新規 25 個) と set=3 (`_Legacy` 54 個) の役割重複統廃合 | 05 |
| C | `MaterialUBO` と `MaterialUBO_Legacy` の attach 排他確認 | 05 |
| D | 典型 scene での実 instance 数 N (rezzed GLTF) / M (rigged Skin) | 09 (Phase 0 計測 task) |
| E | Vulkan vkQueueSubmit / swapchain の現状実装状況 | 07 |
| F | host C++ redirect 層の dirty 判定方式 (UBO 単位 / member 単位 / cadence 単位) | 06 |
| G | per-material cadence を per-draw に統合するか独立保持か | 03 update or 05 |
| H | bare uniform → UBO 集約の **粒度設計** (member 単位の対応表) | 05 / 06 |
| I | Phase 2d-α 適用済 commit 群 (`0587c574da` 等) の処遇 (push / 保留 / revert) | 09 |
| J | upstream Firestorm との UBO blueprint 差分検証 | 09 開始前 |

---

## §3 inventory doc 内 path 参照の補正 (= 軽微な後続作業)

`ayastorm-r41-ubo-current-state-inventory.md` の Appendix A.3 で参照している handoff doc は本 session で `handoff/` 配下に移動済。path 参照 (`handoff-substep-...`) は内部 doc 間の relative ref なので、次 session 開始時に **同 doc 内の path を `handoff/handoff-substep-...` に書き換え** が必要 (= 5 分作業、chapter 04 起案前の clean-up として実施)。

同 inventory doc §A.3 以外で handoff doc を相対参照している箇所は本 session 確認範囲では未検出。

---

## §4 commit / push / session clear の判断は AYA 側

本 session 完了時点で:

- inventory doc 6 箇所 unstaged 更新あり (= §0.4)
- 新設 chapter 01-03 + 本 handoff doc が untracked
- handoff/ subdir への 97 件 `git mv` が staged

**まとめて 1 commit / 設計 doc と inventory update を分ける / 一旦 untracked のまま次 session で纏める** の判断は AYA 側で決定。Claude 側からは proactive に commit 提案しない (= memory `feedback_no_auto_commit` + 本 session で「commit とかいまもういいよ」指示)。

session clear の判断も AYA 側。次 session 開始時は §1.1 の 5 件読込から再開可能。

---

## §5 本 session の感触 (= 設計議論結果の要約)

1. **inventory が成立した瞬間に「84 UBO blueprint は dead」** が一行で説明可能になった (= host redirect 層欠落)。これは η-24 から η-28 Phase 2c までの 26 sub-step を **正しく失敗履歴として畳む** 効用がある
2. **Codegen-UBO に到達** = call site API を変えず + build-time 決定論 + 既存 84 UBO blueprint も再利用可能 = 原則 1/2 両方を満たす設計が見えた
3. **cadence source rule (= 既存 C++ 呼出 path)** が確定したため、新規 viewer settings / 動的判定が一切不要 = 設計が小さく閉じる見込み
4. **2 軸厳密区別 (logical binding ≠ physical instance)** が AYA 指摘で固まった = 今後の議論で「UBO 数」を語る時に必ず両軸表記する規律が立った
5. **設計 doc を chapter 分割した** ことで、後続 session が context size を恐れずに 1 chapter ずつ進められる構造になった

次 session で chapter 04 (Codegen-UBO 機構) に着手すれば、本設計の核心が形になる。

---

**= 本 handoff doc を次 session の入口として、chapter 04 から再開する**。
