# sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 handoff: 設計 chapter 07 起案 + (V1)(V3)(S3) 解消 → AYA commit 指示待ち

**完了日**: 2026-06-03
**位置付け**: 前 handoff (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-chapter-06c.md`、chapter 06c 起案 + chapter 06 (06a/06a-prep/06b/06c) 全件起案完了) を受けて、**chapter 07 起案完了** (= 現状 Vulkan API 実装棚卸し + device limit query 拡張 + set=1 layout 方式 (V3a 全 program 共通) + set=1 79 binding split (V1') + sampler 49 個 set=3 per-asset 同居 (S3') + descriptor pool 4 cadence split + 容量算定 (W) + dynamic offset ring buffer 4 MB + triple-buffering 5 cadence 全適用 + 共通 PSO layout + reflection fence sync) した state。chapter 01 §4 進捗表を 07 ✅ 起案済 に update。**AYA 明示 commit 単位到達**、AYA の commit 指示待ち。次 session は AYA 指示後 commit phase 入り → chapter 08 (build-codegen-pipeline) 起案へ進む。

---

## §0 本 session で完了した作業

### §0.1 設計 chapter 起案

| chapter | file | 規模 | 主内容 |
|---|---|---|---|
| 07 | `design/07-vulkan-api-state.md` | 571 行、§0-§13 (14 section) | scope vs 非 scope / 入力契約 / 現状 Vulkan API 実装棚卸し (= volk / VMA / `DeviceLimits` / descriptor pool / pipeline layout / sampler / `FRAMES_IN_FLIGHT=3` / SPIR-V / dynamic rendering / vertex buffer Vk helpers) / device limit query 拡張 4 field 追加 (= `maxDescriptorSetUniformBuffers` / `Dynamic` / `maxPerStageDescriptorUniformBuffers` / `minUniformBufferOffsetAlignment`) / (V1') set=1 79 → 40/39 split 採用 / (V3a) 全 program 共通 layout 採用 (= PSO compatibility 最大 + dummy buffer 微小 cost) / (S3') sampler 49 個 set=3 per-asset 同居 採用 (= cadence 整合 + `maxDescriptorSetSamplers` 96 ≥ 49 適合) / (W) descriptor pool 4 cadence split + 容量算定 (Frame/Program/Draw/Asset 別 pool) / set=1a/1b 拡張で set 帯 5 化 + bind 時 `maxBoundDescriptorSets=4` 制約死守 (= set=3 と set=2 入替 bind 方式) / dynamic offset (L2) ring buffer 4 MB 起動 / 16 MB grow / 3 chunk triple-buffering / triple-buffering 5 cadence (per-frame/per-program/per-draw/per-asset UBO/per-skin SSBO) 全適用 + `sFrameIndex` 全 cadence 共有 / 共通 `sAYAStandardLayout` PSO layout 1 種 + push constant 64 B / Global_ReflectionProbes 1 buffer + `VkFence` sync / 04-06a-06b-06c-08-09 分担境界 / 未確定 8 件 (V1'/V3'/S3'/W/W2/R1/PSC/RF) / update 規律 |

### §0.2 07 起案の波及 reflect (= 整合 update)

| update 先 | 内容 | 状態 |
|---|---|---|
| chapter 01 §4 (進捗表) | 07 行を「未起案」→「✅ 起案済」へ update | unstaged |

(他 chapter への直接 reflect は **commit phase 後 / 実装 phase 入口で段階適用**。具体的には 06a §5.6 sampler skip 注記更新、06c §2 set 帯 4 → 5 拡張表 / §3 接合表 / §8 sampler 配置確定形書換え は chapter 07 確定後の **波及 phase** で別 commit、本 session では行わない — chapter 07 単独 commit 単位の純度確保)

### §0.3 / §0.4 確定設計判断 + 解消持越 (= chapter 07 本体への pointer)

詳細は **`design/07-vulkan-api-state.md` 該当節で直接参照** (本 handoff では重複記述しない、context 節約):

| 項目 | 確定形 | chapter 07 該当節 |
|---|---|---|
| (V1) → (V1') | set=1 79 → 40/39 split | §3.2 |
| (V3) → (V3a) | 全 program 共通 layout | §4.2 |
| (S3) → (S3') | sampler 49 set=3 per-asset 同居 | §5.2 |
| (W) | 4 cadence pool split / `sProgramUboPool` maxSets=6 | §6 |
| (R1) | ring buffer 4 MB initial / 16 MB max / 3 chunk triple-buffering | §7.2 / §7.5 |
| set 帯 5 化 + bind 4 set 死守 | shader bind 時 set=0/1a/1b/2 / draw 切替で set=2↔set=3 入替 | §4.4.1 |
| triple-buffering 5 cadence 全適用 | per-frame/per-program/per-draw/per-asset UBO/per-skin SSBO | §8.3 |
| 共通 PSO layout `sAYAStandardLayout` | shader 全体 1 layout + push constant 64 B 既存温存 | §9.1 |
| reflection fence sync | Global_ReflectionProbes 1 buffer + `VkFence` | §10.1 |

= **(V1)(V3)(S3) 解消、(W)(R1) 新規確定、set 帯 5 化 + bind 4 set 制約 + triple-buffering 全適用 + 共通 PSO layout + fence sync 確定** (詳細は chapter 07 本体)。

### §0.5 design-phase 規律 (= 前 handoff §0.5 / §0.6 規律) 完全遵守

- `indra/` 配下改変ゼロ (= memory `feedback_design_phase_no_code_write`)
- spec doc 内の file path / line / code shape / 配線位置記述は記載 (= ルール境界、memory に明記済の通り)
- chapter 07 §2 棚卸しで **既存 Vulkan API 実装の file path / line を `llvkloader.cpp/h` から正確に転記** (= `llvkloader.cpp:113` `FRAMES_IN_FLIGHT=3` / `:339-353` `DeviceLimits` struct / `:362` `VmaAllocator` / `:382` `sPerMaterialDescriptorSetLayout` / `:1422-1450` per-frame pool / `:2118` `volkInitialize` / `llvkloader.h:104` `getPerFrameDescriptorSetLayout` / `:153-158` dynamic rendering 等)
- §3.1 `DeviceLimits` struct 拡張は **spec 上の C++ コード断片** として記述 (= 実 `indra/` 編集なし)
- §7.4 `vkCmdBindDescriptorSets` 呼出例 / §10.1 `writeGlobalReflectionProbes` 例 も spec 内コード断片の範囲

---

## §1 次 session の作業 (= AYA commit 指示後の commit phase 入り → chapter 08 起案着手)

### §1.1 次 session = AYA 指示後 commit phase + chapter 08 起案

| 項目 | 詳細 |
|---|---|
| commit phase 入口 | chapter 07 起案完了 = AYA 明示 commit 単位到達、本 session 完了時点で **AYA に「07 揃いました、commit してください」報告済 → AYA commit 指示待ち** |
| commit 単位 (= AYA 明示 2026-06-03 規律) | unstaged: chapter 01 §4 進捗表 update (= 本 session で「未起案」→「✅ 起案済」化、07 行のみ)、untracked: `design/07-vulkan-api-state.md` (本 session) + `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-chapter-07.md` (本 handoff doc 自身) |
| commit 後 = chapter 08 起案着手 | `08-build-codegen-pipeline.md` 起案、CMake / glslang / preprocess script / `ubo_metadata.inl` 出力 build pipeline 統合 / set=1 split 40/39 自動振分け / std140 alignment 256 B padding / 共通 PSO layout 反映 |
| 起案完了判定 | chapter 08 単独 ~370-430 行見込み (06a/06b/06c/07 同等規模、ただし 07 は 571 行 = 4 持越解消 + 容量算定 + 新規 ring/fence のため 30% 超過の前例) |

### §1.2 chapter 08 (build-codegen-pipeline) scope (= 07 §11 分担境界 / 既存設計 chapter 08 役割)

| 項目 | 詳細 | 07 からの input |
|---|---|---|
| Codegen build pipeline | glslang parse + ubo_metadata.inl 出力 + perfect hash 生成 + std140 alignment 検証 | chapter 04 §5.1 + 07 §3 (V1') split 自動化 |
| CMake 統合 | pre-build step + dependency tracking + 増分 build | chapter 04 § + 既存 cmake 構造 |
| 出力契約 | `ubo_metadata.inl` で set=0/1a/1b/2/3 + alignment 256 B padding + sampler binding 値出力 | 07 §3.1 / §4.4 / §5.4 / §7.3 |
| set=1 split 自動振分け | 79 個を名前 sort で 40/39 自動配分 (= 07 §3.2 V1') | 07 §3.2 / §4.3 |
| Codegen 出力 dummy buffer 連動 | 全 program で未使用 binding に dummy `VkBuffer` 参照を `vkUpdateDescriptorSets` 経由で program bind 時投入 | 07 §4.3 |
| build error 検出 | std140 layout 不整合 / binding 衝突 / set 帯超過 を build-time fail | chapter 04 §5.1 / 07 §3 |
| host 側 ubo_loader header 出力 | `LLGLSLShader::mUniformUBOLoc[index]` ↔ block_name 物理 instance map | chapter 04 §5.3 / 06a §3 |

### §1.3 chapter 08 起案で AYA 判断を仰ぐ可能性のある論点 (= 暫定)

| # | 論点 | 影響 |
|---|---|---|
| (B1) | Codegen 実装言語 (= Python vs C++ standalone tool vs CMake script) | build dependency 持込 / Linux/Win/Mac 3 OS 動作確証 |
| (B2) | glslang vendoring vs system pkg | autobuild stack 改修範囲 (= AYAstorm 既存 autobuild に追加 package 要否) |
| (B3) | perfect hash algorithm (= gperf / 自作 frozen-table / 既存 LL hash 利用) | build cost / hash collision rate / cache hit rate |
| (B4) | 増分 build cache strategy (= SPIR-V hash key) | build 時間 / cache disk footprint |
| (B5) | Codegen 実行 trigger (= 全 build 時 / GLSL 変更検出時 / 手動 cmake target) | build cost vs miss risk |

### §1.4 次 session 着手前の前提読み込み (= 最低限 3 件 + 必要時 pinpoint Read)

**従来 11-12 件全読み廃止** (= context 圧迫で /clear 効果消失、AYA 指摘 2026-06-03)。chapter 08 (build-codegen-pipeline) では以下の **最低限 3 件のみ全読**、他は necessity-driven pinpoint Read:

#### §1.4.1 最低限 必須 Read (chapter 08 着手時)

1. **本 handoff doc** (chapter 07 完了状態 + chapter 08 scope + (V1')(V3a)(S3')(W) 確定形 pointer + 持越 reset)
2. **`design/04-codegen-ubo.md`** (= Codegen-UBO 機構本体、build pipeline 側で 1:1 実装する出力契約)
3. **`design/07-vulkan-api-state.md`** (= 直前 chapter、§3.1 device limit 拡張 / §4 set=1 split / §5 sampler 配置 / §7.3 alignment が build 出力側で反映必須)

#### §1.4.2 必要時 pinpoint Read (= chapter 08 起案中に該当論点に触れた時のみ Read)

| 触れる論点 | Read 対象 + 該当節 pinpoint |
|---|---|
| 命名規則 (= rename 表参照) | `design/02-naming-convention.md` §3 |
| cadence 分類 (= 5 cadence 出力) | `design/03-cadence-classification.md` §2 |
| 84 UBO mapping (= 出力先 set/binding) | `design/05-existing-inventory-link.md` §3 / §4 (E3) |
| cache 構造 (= host header 出力契約) | `design/06a-cache-structure-and-setter-redirect.md` §3 / §5.6 |
| flush 関数連動 (= host 側 trigger) | `design/06b-cadence-update-site-and-dirty.md` §4.1 |
| descriptor set 接合 (= set/binding 値整合) | `design/06c-descriptor-set-bind-wiring.md` §3 (接合表) |
| inventory 現状 binding | `ayastorm-r41-ubo-current-state-inventory.md` §1 / §3.3 |
| 全 chapter 進捗 / 確定事項 13 件 | `design/01-overview.md` §4 / §5 |

#### §1.4.3 規律

- 必須 3 件で文脈不足を感じたら **その時点で pinpoint Read** (= 推測で起案しない)
- 「全部 Read = 安全」では context 圧迫で / clear 効果消失、**「最低限 + 必要時 pinpoint」が安全と context 両立**
- chapter 08 で必要 Read を実 実行したら、本 §1.4.2 を **次 handoff (= chapter 08 handoff) に反映** (= 実 Read 履歴を次回の最低限 candidate に昇格)
- memory `feedback_handoff_minimal_pre_req_read` 準拠 (= 2026-06-03 AYA 指摘確定)

### §1.5 次 session 推奨進め方

- AYA から commit 指示が来たら **明示パス指定で stage** (= `git add -A` / `git add .` 禁止、`tests/` 厳守除外、memory `feedback_tests_dir_never_commit`)
- commit 後、AYA から push / session clear 指示があれば従う、無ければそのまま chapter 08 起案へ
- chapter 08 起案中も `indra/` 配下改変ゼロを死守 (= design-phase 規律、memory `feedback_design_phase_no_code_write`)
- chapter 08 起案完了で再度 AYA に「08 揃いました」報告 → commit 指示待ち、次 chapter 09 (phase-roadmap) へ

### §1.6 chapter 07 持越 reset (= 07 § 12 → 次 chapter / 実装 phase 担当)

| # | 項目 | 担当 chapter / phase |
|---|---|---|
| (V1') | set=1 40/39 split 採用 = AYA 判断仰ぎ | chapter 10 / AYA 判断 |
| (V3') | 全 program 共通 layout (V3a) = AYA 判断仰ぎ | chapter 10 / AYA 判断 |
| (S3') | sampler 49 set=3 per-asset 同居 = AYA 判断仰ぎ | chapter 10 / AYA 判断 |
| (W) | `sProgramUboPool` maxSets=6 (= active × 3 × 2) = AYA 判断仰ぎ | chapter 10 / AYA 判断 |
| (W2) | `sAssetUboPool` 起動時 N=64 prealloc + 64 grow chunk | chapter 09 Phase Roadmap |
| (R1) | ring buffer 4 MB initial / 16 MB max / cvar 配信 | chapter 09 Phase Roadmap |
| (PSC) | PSO cache disk persist path / 容量上限 64 MB | chapter 09 Phase Roadmap |
| (RF) | reflection update fence throttle (= 100 回/分 warn) | 実装 phase 入口 + chapter 09 |
| (06a §5.6 注記反映) | sampler 強制 OpenGL path 注記 → S3' set=3 same set 反映 | commit phase 後の波及 phase |
| (06c §2 表反映) | set 帯 4 → 5 拡張 (set=1a/1b/2/3) + bind 4 set 制約反映 | commit phase 後の波及 phase |
| (06c §3 接合表反映) | sampler 49 binding 追加 + Program\_\* set=1a/1b 振分け | commit phase 後の波及 phase |
| (06c §8 sampler 配置確定) | S3 placeholder → S3' set=3 同居形へ書換え | commit phase 後の波及 phase |

---

## §2 commit unit (= 本 session の commit 単位、AYA 指示後に実行)

### §2.1 commit 単位 = chapter 07 単独

```
commit subject: docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 UBO 全体設計 chapter 07 (vulkan-api-state) 起案 + (V1)(V3)(S3) 解消

body 抜粋:
- design/07-vulkan-api-state.md 起案 (571 行、§0-§13)
- 現状 Vulkan API 実装棚卸し (volk/VMA/DeviceLimits/pool/layout/sampler/FRAMES_IN_FLIGHT/SPIR-V)
- (V1) set=1 79 binding device limit 限界 → (V1') 40/39 split 採用
- (V3) set=1 layout 全 program 共通 vs program 別 → (V3a) 全 program 共通採用 (PSO cache hit 最大)
- (S3) sampler 49 配置帯 → (S3') set=3 per-asset 同居採用 (cadence 整合 + sampler limit 適合)
- (W) descriptor pool 4 cadence split + 容量算定 (Frame/Program/Draw/Asset 別 pool)
- set 帯 5 化 (set=0/1a/1b/2/3) + bind 時 maxBoundDescriptorSets=4 制約死守 (set=2/set=3 入替 bind)
- dynamic offset (L2) ring buffer 4 MB initial / 16 MB max / 3 chunk triple-buffering
- triple-buffering 5 cadence 全適用 + sFrameIndex 全 cadence 共有
- 共通 sAYAStandardLayout PSO layout 1 種 + push constant 64 B 既存温存
- Global_ReflectionProbes 1 buffer + VkFence sync (= update 頻度 << frame 頻度)
- chapter 01 §4 進捗表 07 → ✅ 起案済 update
```

### §2.2 commit 対象ファイル (明示)

```
git add docs/specs/ayastorm-r41-gl-removal/design/01-overview.md
git add docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md
git add docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-chapter-07.md
```

**= `tests/` 厳守除外 / `git add -A` / `git add .` 禁止** (= memory `feedback_tests_dir_never_commit` / `feedback_no_auto_commit`)。

---

## §3 完了後の inventory state (= live doc reflect 不要、本 chapter 07 で 100% 設計層内消化)

`ayastorm-r41-ubo-current-state-inventory.md` への直接 reflect は **本 session 不要**。理由:
- 07 で確定したのは **設計層** (= V1'/V3a/S3'/W/ring/fence/共通 PSO layout 設計形)、inventory が映す **実装層 / 現状コード state** には差分なし
- inventory §3.3 UB_\* 4 binding 確定は 06c 時点で完結、07 で再度の変化なし
- inventory に反映するのは **実装 phase 入口 + Phase 0 計測完了** 時点、本 chapter 07 起案単独では reflect しない

= **本 session 完了状態 = design/ 配下 11 件 (01-07 全件起案完了) + handoff/ 配下 7 件 (chapter 06b/06c/07 + γ'-port-α-complete 等履歴) + inventory live doc 1 件 (差分なし)**。

---

**= 本 handoff doc を読めば、次 session で AYA commit 指示後に「明示パス stage + commit → push 待ち → chapter 08 起案着手」が完全再現可能**。
