# r41 sub-step 2.1b / 2.2 / 2.3 / 2.4 完遂 → 2.5 着手境界 handoff (2026-05-28)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-2-1a-complete.md` (sub-step 2.1a 完遂 → 2.1b 着手境界)
**本 handoff の位置付け**: 段階 2 sub-step 2.1b〜2.4 (全 pool record hook 配線、計 12 file) 連続完遂境界。次は 2.5 (段階 2 self-check: validation strict 検証 + 段階 2 完遂 handoff) 着手前 scope 確認 + 実装。

---

## 1. sub-step 2.1b / 2.2 / 2.3 / 2.4 完遂 state (2026-05-28)

### 1.1 完遂宣言

本 session で sub-step 2.1b → 2.2 → 2.3 → 2.4 を連続で satisfy。各 sub-step は file 単位 independent な hook 配線で、AYA 起動実機検証 + Vulkan init/shutdown error 0 件 + 全 pool marker 出現を毎回確認。

#### 1.1.1 sub-step 2.1b (base orchestrator bridging + 軽量 4 pool 配線)

- 採用 scope: 前 handoff §2 (1) **hook wiring only** (AYA 承認: 「OK」)
- 達成内容:
  - `indra/newview/lldrawpool.h`:
    - `VkCommandBuffer` forward decl 追加 (`typedef struct VkCommandBuffer_T* VkCommandBuffer;`)
    - base virtual `recordPoolDraws(VkCommandBuffer cmd_buf) {}` (no-op、derived override 用 bridging template)
  - `indra/newview/pipeline.h` / `pipeline.cpp`:
    - `LLPipeline::recordVulkanPools()` dispatcher 新規追加 (`LLVKLoader::getCurrentCommandBuffer()` 取得 → mPools iterate → 各 pool の `recordPoolDraws(cmd_buf)` 呼出、`VK_NULL_HANDLE` 早期 return)
    - `#include "llvkloader.h"` 追加
  - `indra/newview/llappviewer.cpp` L1781-L1783: `LLVKLoader::beginFrame()` ↔ `LLVKLoader::endFrame()` の envelope 内、`display()` 前に `gPipeline.recordVulkanPools()` 配置
  - 4 軽量 pool に `recordPoolDraws` override + 一度きり LL_INFOS("VkRecord") marker 配線:
    - `lldrawpoolsky.{h,cpp}`
    - `lldrawpoolwaterexclusion.{h,cpp}`
    - `lldrawpoolpbropaque.{h,cpp}`
    - `lldrawpoolsimple.{h,cpp}` (FullbrightAlphaMask 2 pool 含む)
- 達成 marker: AYA 起動 log で 4 pool 全 marker (`Sky pool` / `WaterExclusion pool` / `GLTFPBR pool` / `Simple pool` recordPoolDraws hook fired) 出現確認、Vulkan init L84 + shutdown L2877-L2878 正常、`VALIDATION|VK_DEBUG|VK_LAYER` error 0 件 (release build は validation disabled、API-level success のみ checkpoint)
- commit: `a69e6753b9` (`feat(r41): sub-step 2.1b 完了 (base orchestrator bridging + 軽量 4 pool record hook 配線)`)

#### 1.1.2 sub-step 2.2 (標準 5 pool record hook 配線)

- 採用方式: Agent 並列 (AYA 承認: 「了解 Agent発射してください」、`feedback_use_agents_proactively.md` 反映)
- 達成内容: 5 pool に `recordPoolDraws` override + 一度きり LL_INFOS("VkRecord") marker 配線:
  - `lldrawpoolalpha.{h,cpp}`
  - `lldrawpoolbump.{h,cpp}` (Bump 系 sub-pool 含む)
  - `lldrawpoolmaterials.{h,cpp}`
  - `lldrawpooltree.{h,cpp}`
  - `lldrawpoolwater.{h,cpp}`
- Agent 出力 cross-verify で `lldrawpoolwater.h` の override が `protected:` block 内に append された問題を Claude 側で self-catch → `public:` 内に Edit で移動 (dynamic dispatch は動くが convention 遵守、`feedback_self_verify_before_handoff.md` 反映)
- 達成 marker: AYA 起動 log で 5 pool 全 marker 出現確認、Vulkan init/shutdown 正常
- commit: `7534318ca0` (`feat(r41): sub-step 2.2 完了 (標準 5 pool: alpha / tree / bump / materials / water に record hook 配線)`)

#### 1.1.3 sub-step 2.3 (atmospherics pool wlsky 配線)

- 達成内容: `lldrawpoolwlsky.{h,cpp}` に `/*virtual*/ void recordPoolDraws(VkCommandBuffer)` override + LL_INFOS marker (header style は legacy `/*virtual*/` 表記、sky/tree precedent 踏襲)
- 達成 marker: AYA 起動 log で `WLSky pool` marker 出現確認、Vulkan init/shutdown 正常
- commit: `a03e76cb88` (`feat(r41): sub-step 2.3 完了 (atmospherics pool wlsky に record hook 配線)`)

#### 1.1.4 sub-step 2.4 (特殊対応 2 pool: terrain + avatar 配線)

- 採用方式: Agent 並列 (AYA 承認: 「お願いします」)
- 達成内容: 2 pool に override + marker 配線 (header style は r30 P2 motion blur block precedent に合わせ hybrid `/*virtual*/ ... override`):
  - `lldrawpoolterrain.{h,cpp}`
  - `lldrawpoolavatar.{h,cpp}`
- 達成 marker: AYA 起動 log で `Terrain pool` + `Avatar pool` marker 出現確認、12 pool 全 marker 累計 verify clean、Vulkan init/shutdown 正常
- commit: `61bd502445` (`feat(r41): sub-step 2.4 完了 (特殊対応 2 pool: terrain + avatar に record hook 配線)`)

### 1.2 段階 2 内 sub-step 進捗 (02-portage-execution.md §3.1 反映)

| sub-step | 内容 | commit | state |
|---|---|---|---|
| 2.1a | command 基盤 + minimal render pass + per-frame 空 record cycle | `49b9f36427` | 完遂 2026-05-28 ✓ |
| **2.1b** | base orchestrator bridging + 軽量 4 pool record 配線 | `a69e6753b9` | **完遂 2026-05-28** ✓ |
| **2.2** | 標準 5 pool (alpha + tree + bump + materials + water) | `7534318ca0` | **完遂 2026-05-28** ✓ |
| **2.3** | atmospherics pool (wlsky) | `a03e76cb88` | **完遂 2026-05-28** ✓ |
| **2.4** | 特殊対応 pool (terrain + avatar) | `61bd502445` | **完遂 2026-05-28** ✓ |
| **2.5** | 段階 2 self-check + handoff (**validation strict 検証含む**) | (次 session) | **scope 確認 + 着手 pending** |

### 1.3 status close / next active

| doc / memory | status |
|---|---|
| `handoff-substep-2-1a-complete.md` (前 handoff) | **役割完了 2026-05-28** (sub-step 2.1b scope 確定 + 着手 satisfy) |
| `handoff-substep-2-4-complete.md` (本 handoff) | 新規作成 (sub-step 2.1b〜2.4 完遂 → 2.5 着手境界) |
| sub-doc `02-portage-execution.md` | active 継続 (段階 2 全完遂までの参照) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 |

### 1.4 検証実機 evidence (`feedback_build_only_verified.md` 遵守)

- 各 sub-step build → install → cache clear → AYA 実機起動 → marker 確認 → commit の 1 cycle 完了
- 12 pool 全 marker 累計 (2.1b 4 + 2.2 5 + 2.3 1 + 2.4 2): `Sky` / `WaterExclusion` / `GLTFPBR` / `Simple` / `Alpha` / `Tree` / `Bump` / `Materials` / `Water` / `WLSky` / `Terrain` / `Avatar`
- Vulkan API-level success: 全 sub-step で init L84 instance 作成 + shutdown L2877-L2878 device + instance destroy clean、`vkCreate*` / `vkCmd*` error return 全 0

---

## 2. 2.5 着手前の scope 確認 (次 session 最優先)

### 2.1 scope (前 handoff §3.1 継承)

drafted 02-portage-execution.md §3.1 sub-step 2.5 完了 marker は:

> 段階 2 全完了 self-check + validation strict 検証 + 段階 2 完遂 handoff doc 作成

具体的に次 session でやることは以下 2 件:

1. **validation strict 検証** (release build は通常 validation disabled、一時 force-enable で 0 件確認)
2. **段階 2 完遂 handoff doc 作成** (sub-step 2.1a 〜 2.5 全完遂宣言 + 段階 3 (描画再構築) 着手前 prep)

### 2.2 validation strict 検証手順 (前 handoff §3.1 完全継承)

`llvkloader.cpp` 内の `#ifndef LL_RELEASE_FOR_DOWNLOAD` ブロック (validation layer enable 部分) を一時除去 → 強制 enable build → AYA 起動 → log grep → 復元の手順:

1. `indra/llrender/llvkloader.cpp` の `#ifndef LL_RELEASE_FOR_DOWNLOAD` ブロックを **一時除去** (validation 強制 enable)
2. `autobuild configure -c ReleaseFS_open -- --fmodstudio -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE --package --chan AYAstorm-release`
3. `autobuild build -c ReleaseFS_open`
4. `cd build-linux-x86_64/newview && rm -rf packaged && cmake --build . --target package` (install)
5. AYA 環境に install + `rm -rf ~/.ayastorm_x64/cache/`
6. AYA 起動 → **ログイン直前で落とす** (login 不要、init phase の validation 出力のみ要)
7. log grep: `grep -E 'VALIDATION|VK_DEBUG|VK_LAYER' ~/.ayastorm_x64/logs/AYAstorm.log`
8. error / warning 0 件確認 (1 件でもあれば原因特定 + fix → 再 build → 再検証)
9. `#ifndef LL_RELEASE_FOR_DOWNLOAD` ブロックを **復元** (validation force-enable 戻す)
10. 復元差分 commit (検証用 force-enable は出荷物に残さない、`feedback_remove_verification_logs.md` 反映)

### 2.3 段階 2 完遂 handoff doc 作成手順

- doc path: `docs/specs/ayastorm-r41-gl-removal/handoff-stage-2-complete.md` (新規)
- 内容:
  - 段階 2 全 sub-step (2.1a/2.1b/2.2/2.3/2.4/2.5) 完遂宣言 + commit hash 一覧
  - 段階 3 (描画再構築) 着手前 scope 確認の draft (charter §7 / sub-doc 03 (まだ未起草の場合は 03 sub-doc 起草も deliverable に含む可能性) との突き合わせ)
  - 12 pool record hook 全 wiring 完了 evidence の集約 (本 handoff §1.4)
  - validation strict 0 件 evidence の集約
  - 段階 2 完遂で sub-doc 02-portage-execution.md の status を `closed → archived` (段階 2 完遂で参照役割完了)
  - 次 session (段階 3 着手 prep) への bridge

### 2.4 次 session 最初のアクション

1. **本 handoff 確認** (`handoff-substep-2-4-complete.md`) を Read
2. **AYA の ayastorm-release 側作業完了確認** (PR review + bug 調査が終わってから 2.5 着手、本 handoff §3 参照)
3. **2.5 着手**: §2.2 手順で validation strict 検証 → §2.3 で段階 2 完遂 handoff doc 作成 → commit

---

## 3. AYA 側並走作業 (本 session 後の context switch)

AYA は本 session 後、**一時的に ayastorm-release branch に切り替えて** 以下 2 件を処理:

1. **ayastorm-release への PR 1 件 review**
2. **ayastorm-release で報告された bug 1 件 調査**

これら完了後に **本 handoff から 2.5 着手** に復帰する流れ。次 session 開始時は AYA 側作業の状況確認を最初に挟む。

---

## 4. deferred item 持ち越し

### 4.1 Mesa RADV 動作確認 (`handoff-stage-1-complete.md` §3.1.2 継承)

- AYA 環境に AMD discrete GPU 不在 → Mesa RADV 動作未確認
- 段階 2-9 進行中は据置、段階 10 driver matrix polish で改めて testbed 確保

### 4.2 段階 3 (描画再構築) sub-doc 起草

- charter §7.6+ で段階 3 の輪郭は確定済 (PSO 構築 + vkCmdDraw* 実投入 + shader port α/β/γ)
- 段階 2 完遂 handoff doc 作成と同タイミングで sub-doc 03 起草着手判断 (次 session 末尾の優先度判断、context 残量次第)

---

## 5. 次 session 開始 action cadence

### 5.1 次 session 開始時の最初のアクション

1. **handoff 確認**: 本 handoff (`handoff-substep-2-4-complete.md`) を Read
2. **AYA 並走作業 status 確認**: ayastorm-release PR review + bug 調査 完了したか質問 1 件 (完了済なら 2.5 着手 GO、未完なら待機)
3. **2.5 着手**: §2.2 validation strict 検証 手順実行
4. **段階 2 完遂 handoff doc 作成**: §2.3 手順
5. **commit + 段階 3 着手 prep**

### 5.2 段階 2 着手中の注意事項 (再掲、`handoff-stage-1-complete.md` §4.3 継承)

- **Linux first-class baseline 厳守**: NVIDIA proprietary on RTX 5090 で動作確認、Mesa RADV は段階 10 polish
- **parity 不要**: charter §1 thesis 維持、AYAstorm 改変 13 file shader (picker 2 / Cinematic 4 / visual realism 7) は r42-α/β/γ で port
- **acceptance satisfy は実機検証**: `feedback_build_only_verified.md` 遵守、推論 ban
- **仮説 2 連続外れ rule**: `feedback_admit_unknown.md` 反映、bridging code trouble で log/canary/bisect 切替
- **sub-step 完遂時 self-trace + handoff**: `feedback_self_verify_before_handoff.md` + `feedback_proactive_handoff.md` 継承
- **commit / push cadence**: commit は AYA 指示後 Claude 実行、push は AYA 手動 (`feedback_release_flow.md` 遵守)
- **検証用 force-enable は出荷物に残さない**: 2.5 validation strict 検証後の `#ifndef LL_RELEASE_FOR_DOWNLOAD` 復元忘れ厳禁 (`feedback_remove_verification_logs.md`)

---

## 6. 関連 doc / memory

### 関連 doc (r41 章)

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter 本体 (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` — sub-doc foundation (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` — sub-doc 段階 2 (closed 2026-05-28、段階 2 進行中の参照 active)
- `docs/specs/ayastorm-r41-gl-removal/handoff-r41-charter-complete.md` — r41 charter 完成 → 段階 1 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-1-complete.md` — 段階 1 完遂 → 段階 2 着手前 prep
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-2-1a-complete.md` — sub-step 2.1a 完遂 → 2.1b 着手境界 (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-2-4-complete.md` — **本 handoff** (sub-step 2.1b/2.2/2.3/2.4 完遂 → 2.5 着手境界)

### 関連 commit (本 session で積まれた branch 上 commit)

- `9dbc363974` — `docs(r41): sub-step 2.1a 完遂 → 2.1b 着手境界 handoff doc 作成`
- `a69e6753b9` — `feat(r41): sub-step 2.1b 完了 (base orchestrator bridging + 軽量 4 pool record hook 配線)`
- `7534318ca0` — `feat(r41): sub-step 2.2 完了 (標準 5 pool: alpha / tree / bump / materials / water に record hook 配線)`
- `a03e76cb88` — `feat(r41): sub-step 2.3 完了 (atmospherics pool wlsky に record hook 配線)`
- `61bd502445` — `feat(r41): sub-step 2.4 完了 (特殊対応 2 pool: terrain + avatar に record hook 配線)`

### 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone active
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 + Linux 先行例外
- `feedback_self_verify_before_handoff.md` — 各 sub-step 完遂時 self-trace 本 handoff §1.1 反映 (特に 2.2 water.h public/protected mis-placement の self-catch)
- `feedback_proactive_handoff.md` — context 圧迫境界で能動 handoff 提案 → 本 doc で実施
- `feedback_no_auto_commit.md` — 全 sub-step commit は AYA 明示指示 (「OK」「Commitしてください」) 後
- `feedback_release_flow.md` — push は AYA 手動
- `feedback_build_only_verified.md` — §1.4 達成 marker は実機検証 evidence
- `feedback_use_agents_proactively.md` — sub-step 2.2 / 2.4 で Agent 並列活用
- `feedback_remove_verification_logs.md` — §2.2 validation strict 検証後の force-enable ブロック復元厳守
- `feedback_one_step_at_a_time.md` — 各 sub-step 1 cycle 完結 (build → install → AYA 起動 verify → commit)
