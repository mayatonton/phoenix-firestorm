# sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 handoff: 設計 chapter 06b 起案 + cadence update site / dirty / flush / forwardToUboUpload interface 確定

**完了日**: 2026-06-03
**位置付け**: 前 handoff (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-chapter-06a.md`、chapter 06a + 06a-prep 起案 + design-phase 規律確立完了) を受けて、**chapter 06b 起案完了** (= 5 cadence 別 update site / 二段階 dedup dirty 機構 / flush timing 論理仕様 / `forwardToUboUpload(loc, data, size)` interface 本体実装) した state。chapter 01 §4 進捗表を 06b ✅ 起案済 に update。次 session は chapter 06c (= descriptor set bind 配線) 起案へ進む。

---

## §0 本 session で完了した作業

### §0.1 設計 chapter 起案

| chapter | file | 規模 | 主内容 |
|---|---|---|---|
| 06b | `design/06b-cadence-update-site-and-dirty.md` | 431 行、§0-§9 (10 section) | scope vs 非 scope / 入力契約 / 5 cadence 別 update site (per-frame/per-program/per-draw/per-asset/per-skin × 4 軸 = 更新契機 / 配置点 / upload thread / flush 単位) / dirty 機構 (二段階 dedup = stage 1 `mValue` cache + stage 3 UBO 単位 dirty bit) / Material* per-draw 統合 (= G1 実体化) / flush timing (cadence 別 flush 関数 5 種 + upload→bind→draw 順序 + triple-buffering + memory barrier 論理要件) / `forwardToUboUpload` signature + cadence 別 routing switch + L 論点 (L1+L2 推奨) + thread 配線 / 06c/chapter 07/chapter 09/chapter 05a への bridge / 04-05-06a-06c 分担境界 / 未確定 7 件 (K/L/M/U1-U4) / update 規律 |

### §0.2 06b 起案の波及 reflect (= 整合 update)

| update 先 | 内容 | 状態 |
|---|---|---|
| chapter 01 §4 (進捗表) | 06b 行を「未起案」→「✅ 起案済」へ update | unstaged |

(他 chapter への直接波及なし — 06b は 06a の `forwardToUboUpload` 呼出位置を受けて本体実装する側、chapter 02-05 の確定事項を消費する側、06c/chapter 07 へ譲る側)

### §0.3 確定した設計判断 (= Claude default 推奨、AYA 判断仰ぎ対象)

| # | 論点 | Claude 推奨 default | 譲り先 |
|---|---|---|---|
| (K) | dirty 判定粒度 | **K2 = UBO 物理 instance 単位 dirty bit** (= `std::atomic<bool>`、§3.4) | chapter 10 / AYA 判断 |
| (L) | per-draw Vulkan 最適化 | **L1 + L2 = ring buffer + dynamic offset** 組合せ (§5.3) | chapter 07 / 06c |
| (M) | thread-safe 化方式 | 現 phase atomic (= `dirty` のみ)、将来 phase で `std::shared_mutex` 追加 | chapter 07 |
| (U1) | per-frame triple-buffering buffer 個数 | 3 (= triple) | chapter 07 |
| (U2) | per-asset / per-skin dirty 判定 | 既存 owner state 変化検知 + Vulkan 側 dirty bit 両立 (= 既存 path 改変ゼロ + upload dedup 両得) | chapter 07 |
| (U3) | per-program ↔ per-draw flush 境界 | bind 直後 upload (per-program 帯) / draw 直前 upload (per-draw 帯) 分離 | (H1b) hook 計測後再評価 |
| (U4) | `mValue` cache 適用外 5 method の Vulkan dirty 判定 | stage 1 bypass、stage 3 dirty bit のみで dedup | chapter 07 / Phase 進行中 |

### §0.4 解消した持越 item

| # | 項目 | 解消形 |
|---|---|---|
| (Q2) | `forwardToUboUpload(loc, data, size)` 本体実装 | **本 chapter §5 で signature + cadence 別 routing + thread 配線まで確定**、実 Vulkan API 接合は chapter 07 譲り |

### §0.5 design-phase 規律 (= 前 handoff §0.5 / §0.6 規律) 完全遵守

- `indra/` 配下改変ゼロ (= memory `feedback_design_phase_no_code_write`)
- spec doc 内の file path / line / code shape / 配線位置記述は記載 (= ルール境界、memory に明記済の通り)
- 06b §2.4 / §2.5 で `gltf/asset.cpp:183/232` / `gltf/animation.cpp:411` / `gltfscenemanager.cpp:693/696/736` 等の **既存 OpenGL upload site の file path / line を inventory §1.1 から継承して明記**

---

## §1 次 session の作業 (= chapter 06c 起案着手)

### §1.1 次 session = chapter 06c 起案

| 項目 | 詳細 |
|---|---|
| chapter 06c scope | descriptor set bind 配線 (= set=0/1/2/3 帯 cadence 別 rebind + UB_* 4 binding vs 84 blueprint 接合) + `mUseUBO` flag 決定方法 + dynamic offset (L2) bind 側責務 |
| 着手前提 | `01-overview.md` 〜 `06b-cadence-update-site-and-dirty.md` 全 chapter 読込済 + memory `feedback_design_phase_no_code_write` 遵守 (= `indra/` 配下改変ゼロ) |
| 規模見込み | 06a / 06b と同等 (~370-430 行) — descriptor set 4 帯 × cadence × bind タイミングの 3 軸 + UB_* 4 binding ↔ 84 blueprint 接合表 + `mUseUBO` initial 設定方針 + chapter 07 への bridge |
| 起案完了判定 | 06c 起案完了 = 設計 chapter 06 (06a/06a-prep/06b/06c) 全件完了 = **AYA 指示の commit 単位確定** (= 「06 が揃った時点で commit する」)、残 chapter 07-10 |

### §1.2 chapter 06c (descriptor-set-bind-wiring) scope (= 06a §1.4 / 06b §6 再掲 + 06b 確定事項反映)

| 項目 | 詳細 | 06b からの input |
|---|---|---|
| set=0 (per-frame 帯) | Frame* UBO の descriptor set 配置 + bind タイミング | §4.1 `flushFrameUbos()` 直後 bind / triple-buffering (= §4.3) 配下の instance idx 選択 |
| set=1 (per-program 帯) | Program_* UBO の descriptor set 配置 + shader bind 連動 | §4.1 `flushProgramUbos()` (= `LLGLSLShader::bind()` 内) 直後 bind |
| set=2 (per-draw 帯) | per-draw UBO の descriptor set + dynamic offset 戦略 (= L2 確定の実装) | §5.3 L1+L2 default 採用 / §4.1 `flushDrawUbos()` 直後 bind |
| set=3 (per-asset/per-skin 帯) | GLTF Asset/Skin owner UBO の descriptor set + 動的個数対応 | §4.1 `flushAssetUbos(asset)` / `flushSkinUbos(skin)` 直後 bind |
| UB_* 4 binding ↔ 84 blueprint 接合 | inventory §3.3 の 4 種論理 binding と 84 GLSL blueprint の最終配線表 + 06b で確定した cadence 帯 (= Frame* 3 + Program_* 79 + Asset_* 2 + Skin_* 1 + Draw_* 数件) との対応 | §6 chapter 06c 譲り |
| `mUseUBO` flag 確定 | 06a で配置だけした flag の initial 設定方針 (= cold launch 時の Vulkan path 全 ON or shader 単位 phase migration) | 06a §3 持越 (Q1) |
| sampler / opaque 系 binding | UBO 化対象外 49 sampler の descriptor set 経由 binding | chapter 07 と接続 |

### §1.3 chapter 06c 起案で AYA 判断を仰ぐ可能性のある論点 (= 暫定)

| # | 論点 | 影響 |
|---|---|---|
| (M) | descriptor set 4 帯 (set=0/1/2/3) bind 戦略 (= bind 頻度 / PSO compatibility / cadence 跨ぎ rebind 範囲) | PSO compatibility / chapter 07 直接接続 / 描画 hot path overhead |
| (N) | `mUseUBO` initial 設定 = shader 単位 phase migration vs Vulkan path 全 ON | Phase 9 (`09-phase-roadmap`) migration scope と連動 |
| (O) | UB_* 4 binding 拡張 = 既存 4 種維持 vs cadence 別 binding 拡張 (= 5 種化) | call site 改修範囲 (= 原則 1) / 既存 GLSL blueprint 改変範囲 |

### §1.4 次 session 着手前の前提読み込み (= pre-requisite、順序固定)

1. `design/01-overview.md` (用語定義 + 設計原則 + 確定事項 13 件、§4 進捗表で 06b ✅ 確認)
2. `design/02-naming-convention.md` (命名規則 + rename 表)
3. `design/03-cadence-classification.md` (cadence 5 分類)
4. `design/04-codegen-ubo.md` (Codegen-UBO 機構 / `UniformLocation` struct / R3 path)
5. `design/05-existing-inventory-link.md` (既存 inventory link + bare uniform 集約 framework + G1 確定)
6. `design/06a-cache-structure-and-setter-redirect.md` (cache 構造 + setter redirect + `mUseUBO` flag 配置)
7. `design/06a-prep-phase0-measurement.md` (Phase 0 計測 spec、実装 phase 入口手順書)
8. `design/06b-cadence-update-site-and-dirty.md` (本 session 起案、cadence update site + dirty + flush + `forwardToUboUpload` interface)
9. `ayastorm-r41-ubo-current-state-inventory.md` (現状棚卸し、live doc、§3.3 UB_* 4 binding 確定)
10. 本 handoff doc (本 session 完了状態 + 06c scope + 持越 reset)

**= この 10 件で次 session の文脈は完全 reconstruct 可能**。

### §1.5 次 session 推奨進め方

- chapter 06c 起案着手 (= descriptor set 4 帯 × cadence bind 配線 + UB_* 4 binding ↔ 84 blueprint 接合表)、`indra/` 配下改変ゼロ厳守
- 06c 起案完了で chapter 06 (06a/06a-prep/06b/06c) 完了 → **AYA に「06 揃いました、commit してください」報告**
- AYA から commit / push 指示後、別 session で chapter 07 (vulkan-api-state) 起案へ
- chapter 07 → 08 → 09 → 10 を順次別 session で起案、design phase 完了 (= 全 chapter 起案済)
- design phase 完了後の **実装 phase 入口で `06a-prep-phase0-measurement.md` を再読込** → Phase 0 計測 (H1b)(E')(F) 実施 → 結果を chapter 05 / 06a / 06b に反映

---

## §2 残持越 item (= 全 chapter の §10 / §8 / 本 handoff からの累積)

### §2.1 chapter 06b §8 持越 (= 本 session 新規)

| # | 項目 | 解消先 | default 採用案 |
|---|---|---|---|
| (K) | dirty 判定粒度 = K1 member 単位 / K2 UBO 単位 / K3 cadence 単位 | **chapter 10 / AYA 判断** | K2 (= UBO 単位) |
| (L) | per-draw Vulkan 最適化 = L1 ring buffer / L2 dynamic offset / L3 sub-allocation | **chapter 07 / 06c** | L1 + L2 組合せ |
| (M) | thread-safe 化方式 = mutex / atomic / lock-free | chapter 07 | 現 phase atomic |
| (U1) | per-frame triple-buffering buffer 個数 = 2 / 3 / N | chapter 07 | 3 |
| (U2) | per-asset / per-skin dirty 判定 = 既存変化検知継承 / Vulkan 側 dirty bit 追加 | chapter 07 | 両立 |
| (U3) | per-program ↔ per-draw 境界 = bind 直後 upload / draw 直前 upload | (H1b) 後再評価 | 分離 |
| (U4) | `mValue` 適用外 5 method の Vulkan dirty 判定 | chapter 07 / Phase 進行中 | stage 1 bypass + stage 3 dedup |

### §2.2 chapter 06a §9 持越 (= 前 handoff から継続)

| # | 項目 | 解消先 | 状態 |
|---|---|---|---|
| (H1b) | LL_INFOS hook 実装 + AYA build run + 不明 16 件 cadence 確定 | 実装 phase 入口 | spec 化済 (`06a-prep` §2)、未実施 |
| (Q1) | `mUseUBO` flag の initial 設定方針 | **chapter 06c** (= 次 session 解消対象) | 06c §1.3 (N) |
| (Q2) | `forwardToUboUpload(loc, data, size)` 本体実装 | **本 session 06b §5 で解消** | ✅ |
| (R1) | LLStaticHashedString 経由 setter 67 個の核 UBO 化対象外確定根拠の chapter 05 §7.3 反映 | chapter 05 §7.3 切出し時 | 別 file 切出し対象 |
| (T1) | `mapUniforms()` 内 Vulkan path 拡張で OpenGL path との二重維持 cost | chapter 09 Phase 進行中 | 持越継続 |

### §2.3 chapter 04 §10 持越 (= 前 handoff から継続)

| # | 項目 | 解消先 |
|---|---|---|
| (A1) | std140 offset 計算 (Codegen 独自 vs SPIR-V reflection 抽出) | chapter 08 |
| (G_codegen) | perfect hash generator (gperf / 独自 / frozen) | chapter 08 |
| (P) | parse 手段 (独自 mini-parser vs glslang reflection) | chapter 08 |

### §2.4 chapter 05 §10 持越 (= 前 handoff から継続)

| # | 項目 | 解消先 |
|---|---|---|
| (E') | inventory §3.3.1 同一 binding 複数 UBO 名疑い | 実装 phase 入口 (= `06a-prep` §3) |
| (F) | MaterialUBO vs MaterialUBO_Legacy 処遇 | 実装 phase 入口 (= `06a-prep` §4) |
| (H2) | chapter 05 §7.3 集約表の別 file 切出し (`05a-bare-uniform-mapping.md`) | 06c 起案後 or chapter 07 起案中の並行作業 |
| (H3) | 集約判定 conflict 時の AYA 判断ループ | chapter 09 Phase 進行中 case-by-case |

### §2.5 前 handoff §2 持越の本 session 消化状態

| # | 項目 | 状態 |
|---|---|---|
| **(Q2)** | `forwardToUboUpload` 本体実装 | **✅ 本 session 06b §5 で解消** |
| (H1b) | LL_INFOS hook 実装 | spec 化済 (`06a-prep`)、実施は実装 phase 入口 |
| (Q1) | `mUseUBO` flag 初期化方針 | → 06c で解消予定 |
| (E') | binding 重複 | → 実装 phase 入口へ移管継続 |
| (F) | MaterialUBO 比較 | → 実装 phase 入口へ移管継続 |
| (R1) | LLStaticHashedString 67 個反映 | → chapter 05 §7.3 切出し時継続 |
| (T1) | `mapUniforms()` 二重維持 cost | → chapter 09 継続 |
| (A1) / (G_codegen) / (P) | std140 / perfect hash / parse 手段 | → chapter 08 継続 |
| (H2) | 集約表別 file 切出し | → 06c 後 or 07 並行継続 |
| (H3) | 集約 conflict AYA loop | → chapter 09 継続 |

**= 前 handoff 持越のうち、本 session で 1 件 (Q2) が確定。他は次 session (Q1) / 実装 phase 入口 (H1b/E'/F) / 別 chapter (R1/T1/A1/G_codegen/P/H2/H3) に分配済**。

---

## §3 unstaged / untracked 状態 (= AYA 判断対象、commit 単位 = 06 揃い)

### §3.1 累積状態

| 種別 | 内訳 |
|---|---|
| staged (前々 session) | `git mv` で `handoff/` 配下に移動した 97 件 |
| unstaged (前 handoff からの累積に本 session 追加) | inventory doc / chapter 01 (§4 / §5) / chapter 02 (§2.1 / §3.4) / chapter 03 (§2 / §4.3) / 06a §7 (前 session) + **本 session: chapter 01 §4 進捗表 (06b → ✅ 起案済)** |
| untracked (前 handoff からの累積に本 session 追加) | design/ 配下: 01-overview.md / 02-naming-convention.md / 03-cadence-classification.md / 04-codegen-ubo.md / 05-existing-inventory-link.md / 06a-cache-structure-and-setter-redirect.md / 06a-prep-phase0-measurement.md / **06b-cadence-update-site-and-dirty.md (本 session 新規)** / handoff/ 配下: 過去 3 件 + **本 handoff doc 1 件 (本 session 新規)** |
| memory file (`~/.claude/projects/.../memory/`) | (前 handoff `feedback_design_phase_no_code_write.md` から本 session 追加なし) |
| tests/ | **commit 対象外** (= memory `feedback_tests_dir_never_commit` 厳守、確認問いかけも禁止、`?? tests/` をそのまま untracked 放置) |

### §3.2 commit 判断 = 06 揃い時 (= AYA 明示指示 2026-06-03)

- AYA 指示: 「06 が揃った時点で commit する」 = chapter 06 (06a / 06a-prep / 06b / 06c) 全件完了 + chapter 01 §4 進捗表 update が揃った時点で commit
- 本 session 完了時点では 06c **未起案**、commit は次 session の 06c 起案完了後に AYA 指示待ち
- Claude 側から proactive に commit 提案しない (memory `feedback_no_auto_commit` 既存方針継続)
- push / session clear / (H1b) hook 実装 session タイミングも AYA 側で判断、次 session 開始時は §1.4 の 10 件読込から再開可能
- **`tests/` ディレクトリは commit 対象から除外** (memory `feedback_tests_dir_never_commit`、明示パス指定で stage、`git add -A` / `git add .` 禁止)

---

## §4 本 session の感触 (= 設計議論結果の要約)

1. **chapter 06 を 3 sub-chapter に分割した判断が正解継続** = 06b 単独で 431 行、06a + 06b で合計 800 行台。単一 chapter で context 1 session に収めるのは確実に不可能だった
2. **(Q2) `forwardToUboUpload` 本体実装が cadence routing switch + L1+L2 推奨で綺麗に閉じた** = 5 cadence × switch case + 1 default skip = 6 branch で値域完全網羅、cadence_tag enum (= chapter 04 §5.3 確定) が銀の弾丸として機能
3. **二段階 dedup (= stage 1 `mValue` + stage 3 UBO dirty) が既存 OpenGL path 挙動の Vulkan 側完全再現を保証** = call site 改変ゼロ (= 原則 1) + GL API call 削減効果 (= 既存) + UBO upload 削減効果 (= 新規) の三得
4. **(K) dirty 粒度の K1/K2/K3 比較で K2 が論点的に最強** = K1 partial write overhead / K3 cadence 単位 upload waste の両方で K2 が勝つ、AYA 判断に出す段階で「K2 default、反証出たら K1/K3 検討」の構図が綺麗
5. **(L) per-draw L1+L2 組合せが Vulkan 描画 hot path の典型解** = ring buffer + dynamic offset は Vulkan 教科書通りの per-draw UBO 設計、独自案不要
6. **06b §8 持越 7 件 (K/L/M/U1/U2/U3/U4)** = 全件 chapter 07 / chapter 10 / 実装 phase に分配可能な分解度に達した、本 session で持越が暴発しなかった
7. **次 session 06c で 06 揃い → commit phase 到達** = AYA 明示の commit 単位 (= 06 揃い) まで残 1 chapter、design phase 全体進捗は 7/12 chapter (06a/06a-prep を 2 件として 7)

---

## §5 本 handoff doc の更新規律

- 次 session で本 doc を入口として読み込む際、§1.1 chapter 06c 起案完了状況を本 doc に上書き反映 or 新 handoff doc (`...-design-chapter-06c.md`) に引継ぎ
- chapter 06c 起案完了時は本 doc + 前 handoff (06a) + その前 (04-05) を superseded mark、新 handoff doc に **06 揃い commit phase 到達** state として引継ぎ
- §3 unstaged / untracked 状態は AYA の commit 判断 (= 06c 完了後) に追従して更新
- §0.3 default 採用案 (K2 / L1+L2 / U1=3 等) は AYA 判断で確定 / 変更時に本 doc + 06b §8 を同時 update

---

**= 本 handoff doc を次 session の入口として、chapter 06c (descriptor-set-bind-wiring) 起案 → 06 揃い (= 06a/06a-prep/06b/06c) → AYA 指示後 commit の流れで再開する**。
