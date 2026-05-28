# r41 段階 1 完遂 → 段階 2 着手境界 handoff (2026-05-28)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-r41-charter-complete.md` (r41 charter 完成 → 段階 1 着手前 prep の境界)
**本 handoff の位置付け**: 段階 1 = GL header wrapper Vulkan header 並走追加 + volk loader + Vulkan instance/device 動作の完遂境界。次は段階 2 (lldrawpool Vulkan 化、13 file) 着手前 prep。

---

## 1. 段階 1 完遂 state (2026-05-28)

### 1.1 完遂宣言

- sub-step 1.1 → 1.5 全 5 sub-step 完遂、charter §2 領域 1 (0.50 PM) 達成
- 達成内容: volk + LunarG SDK 1.3.x integration / `llvkloader.{cpp,h}` 新規 / Vulkan instance + physical device 列挙 + logical device 作成 + shutdown / wrapper 2 file (`llglheaders.h` + `llgltypes.h`) Vulkan header 並走追加 / 旧 "Minimal Vulkan" stub 撤去
- 達成 marker: sub-doc `01-foundation.md` §4.1 acceptance 4 件 self-trace PASS (詳細は §2)
- branch: `feature/ayastorm-r41-gl-removal` (sub-step 1.1-1.4 commit 4 件積み済、sub-step 1.5 は本 handoff 作成 + AYA 指示後 commit 予定)

### 1.2 段階 1 内 sub-step 完遂 cadence

| sub-step | 内容 | commit | 完遂判定 |
|---|---|---|---|
| 1.1 | volk + Vulkan SDK 3rdparty 取込 + autobuild + cmake 検出 | `837bef4dbd` | configure pass + viewer build pass ✓ |
| 1.2 | `llvkloader.cpp` + `.h` 新規追加 (volk 初期化 + Vulkan instance 作成) | `cd4e9f980a` (1.3 と同 commit) | Vulkan instance 作成 log 確認 ✓ |
| 1.3 | physical device 列挙 + queue family 選択 + logical device | `cd4e9f980a` | RTX 5090 + llvmpipe 2 device 列挙 + RTX 5090 選択 + graphics queue family 取得 + logical device 作成 log 確認 ✓ |
| 1.4 | wrapper 段階 1 scope file 並走追加 (`llglheaders.h` + `llgltypes.h`) + 旧 stub 削除 | `48361e0a28` | viewer build pass + 起動 pass + Vulkan instance/device 作成/破棄 log 確認 ✓ |
| 1.5 | 段階 1 self-check + AYA review + handoff doc | (本 commit 予定) | §4.1 acceptance 4 件 self-trace PASS + 本 handoff doc 作成 |

### 1.3 status close / next active

| doc / memory | status |
|---|---|
| sub-doc `01-foundation.md` L3 | `closed 2026-05-28` (Pattern α 一括 draft + AYA review PASS、段階 1 着手準備 ready) → 段階 1 完遂で本 doc 役割完了、段階 2 着手 base は新規 sub-doc `02-portage-execution.md` |
| `handoff-stage-1-complete.md` | 本 handoff (新規作成) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (段階 1 完遂を本 handoff で marker、status field は r41 全完遂で closed) |

### 1.4 段階 1 commit / push 反映 (sub-step 1.5 分は未実施、AYA 指示後)

- 本 session の sub-step 1.5 編集 = 1 file 追加: `handoff-stage-1-complete.md` (本 doc)
- commit message draft: `docs(r41): sub-step 1.5 完了 (段階 1 self-check + handoff doc) → 段階 1 完遂宣言`
- push: AYA 手動 (`feedback_release_flow.md` 遵守)

---

## 2. §4.1 acceptance 4 件 self-trace (sub-step 1.5 中核成果)

`01-foundation.md` §4.1 段階 1 自己 acceptance の 4 criterion を実機検証 evidence で satisfy 判定。

| criterion | 判定 | evidence |
|---|---|---|
| **#1-段階 1 (wrapper 段階 1 scope file Vulkan header 並走追加)** | **PASS** | `grep -E "include.*volk\.h" indra/llrender/llglheaders.h indra/llrender/llgltypes.h` → 2 file hit (`llglheaders.h:1097` + `llgltypes.h:41`)、`grep -E "typedef.*LLVk" indra/llrender/llgltypes.h` → 8 typedef hit (`LLVkBuffer` / `LLVkImage` / `LLVkImageView` / `LLVkDeviceMemory` / `LLVkSampler` / `LLVkShaderModule` / `LLVkPipeline` / `LLVkFormat`)。GL include 残置許容 (charter §3 #1 acceptance の 0 件判定は r41 全完遂時)。`llglstates.h` は段階 3 PSO 化 scope に確定 (§3.2 で明文化、段階 1 touch せず) |
| **#3-段階 1 (Vulkan instance + device 動作)** | **PASS** | viewer 起動 log evidence (`~/.ayastorm_x64/logs/AYAstorm.log`): L80 createInstance L83 `Vulkan instance created (validation=disabled)` / L81 selectPhysicalDevice L112 `Found 2 physical device(s)` / L82-L83 L133 candidates (`NVIDIA GeForce RTX 5090` type=2 score=100 + `llvmpipe (LLVM 20.1.2, 256 bits)` type=4 score=10) / L84 L153 `Selected physical device: NVIDIA GeForce RTX 5090` / L85 L169 `Graphics queue family: 0 (queueCount=16)` / L86 L206 `Vulkan device created (graphics queue family 0)` / L2777-L2778 shutdownVulkan L260+L268 `Vulkan device destroyed` + `Vulkan instance destroyed`。validation layer error 0 件 (log grep `VK_DEBUG\|VALIDATION` 0 hit)。**known minor**: L228 `Vulkan loader version X.Y.Z` log line が log file に flush されない事象あり (string は binary + .o に embed 確認、code path 通過確実 = L83 直前で実行、channel/level は L83 と同一 LL_INFOS("Vulkan")、原因未特定)。段階 1 acceptance への影響なし (instance/device 動作 evidence 別経路で確認済)、段階 2 着手時の deferred 調査 item として §3.2 で引き継ぎ |
| **環境前提 (Linux driver 2 件動作)** | **PARTIAL (1 driver + CPU SW fallback)** | NVIDIA proprietary driver on RTX 5090 で起動/instance/device 作成/shutdown 全 PASS、Mesa llvmpipe (CPU software rasterizer) 副次検出済 (vkEnumeratePhysicalDevices で score=10 candidate 列挙確認、選択はされない正常挙動)。**testbed 制約**: AYA 環境は NVIDIA RTX 5090 単一 GPU、AMD discrete GPU 不在 → Mesa RADV (AMD) 動作確認 不可。Intel ANV は §4.1 acceptance では段階 10 polish scope で許容済。段階 1 acceptance は本 partial satisfy で合意 (Mesa RADV は段階 10 driver matrix prep で再評価) |
| **regression (本線動作維持)** | **PASS** | autobuild configure + build PASS (`ReleaseFS_open` + `--fmodstudio` + `LL_DULLAHAN_AUDIO_CALLBACK`)、viewer 起動 PASS、log 上 audio / chat / login init 系 message 出現確認 (regression 0 件報告)。`feedback_release_with_user_feedback.md` 遵守で exhaustive sustained session 検証は行わず、AYA 起動確認 PASS で sufficient |

### §4.2 不達時の対処 (charter §3 acceptance 運用方針継承)

本段階では **未達 criterion 無し** (#環境前提 は partial だが acceptance 内合意済、§4.1 base に従い段階 1 受入可)。

### §4.3 段階 1 完遂後の次 段階 (sub-doc `01-foundation.md` §4.3 反映)

- **段階 2 着手** (lldrawpool Vulkan 化 13 file)、領域 6 (shader SPIR-V 化) + 領域 7 (descriptor / render pass) と並走
- **sub-doc `02-portage-execution.md` 起草** (段階 2 着手前に prep、charter §7.4 反映)

---

## 3. 段階 2 着手前の引き継ぎ事項

### 3.1 解決未完 item (段階 2 着手時の再調査推奨)

#### 3.1.1 loader version log line 不出力 mystery (**解消 2026-05-28**)

- **症状**: `llvkloader.cpp` L228 `LL_INFOS("Vulkan") << "Vulkan loader version " << VK_VERSION_MAJOR(v) << "." << VK_VERSION_MINOR(v) << "." << VK_VERSION_PATCH(v) << LL_ENDL;` が log file に出現しない
- **真因確定**: **LLError は新規 channel tag の最初の LL_INFOS 呼び出しを swallow する**。bisect 検証 (canary-A/B/C 3 行を L228 周囲に挿入 → log 確認) で「`#Vulkan#` tag の最初の 1 行のみが消える」事象を再現確定。multi-line/single-line/`<<` chain 内容は無関係、純粋に「その channel での最初の呼び出し」が落ちる挙動。仮説 (i)(ii)(iii) は全て却下
- **fix**: `initVulkan()` 先頭に `LL_INFOS("Vulkan") << "Initializing Vulkan loader..." << LL_ENDL;` を warmup として配置、loader version を含む本命 log は全て後続に配置 → 2 回目の起動 log で確認 PASS (L82 `Vulkan loader version 1.4.319` 含む全 Vulkan log appearance)
- **段階 2+ 横展開**: 新規 LLError channel を導入する時 (Vulkan 以外も) は最初の LL_INFOS を warmup 用途として割り切る運用ルールが妥当。本 quirk は段階 2 sub-doc `02-portage-execution.md` 起草時に「LLError 新規 channel 運用 note」として継承

#### 3.1.2 Mesa RADV (AMD) 動作確認 testbed 制約

- AYA 環境に AMD discrete GPU 不在 → Mesa RADV 動作未確認、段階 10 driver matrix polish scope で再評価
- 段階 2-5 着手中は NVIDIA proprietary 単一 driver で進行可、Mesa llvmpipe (CPU SW) は副次列挙のみで実描画 path には乗らない (graphics queue priority で除外)
- 引き継ぎ事項: 段階 10 着手時に AMD GPU 持ちの testbed (CI host or 他開発者環境) で driver matrix sweep 必須

### 3.2 段階 2 scope 確認 (charter §2 領域 2 + sub-doc `01-foundation.md` §3.5 + §4.3)

- **対象**: lldrawpool 13 file の GL call → command buffer record 移行 (0.50 PM × 2 = 1.00 PM)
- **base 資料**: `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` §5.4 段階 2 (lldrawpool 13 file 内訳) + `05-vulkan-api-design.md` §4 render pass (段階 2-3 で参照) + §10 LLVKRenderer skeleton (段階 8 並走配置、段階 2 では interface 経由 call は r41.5 まで保留)
- **着手前 prep**: sub-doc `02-portage-execution.md` 起草 (Pattern α or β は charter §7.5 boundary refine 可)、13 file 順序 + per-file 着手 cadence + 段階 2 自己 acceptance 確定
- **段階 1 で touch しなかった項目** (段階 2-5 scope、`01-foundation.md` §3.5):
  - lldrawpool 13 file → 段階 2
  - llrender 主要 5 file state machine → 段階 3 (`llglstates.h` の Vulkan PSO state alias 整備もここ)
  - pipeline.cpp 3 大グローバル `sCull` / `sShadowRender` / `sCurCameraID` → 段階 4
  - llspatialpartition / llviewershadermgr / llvertexbuffer / llvosky / llvowlsky → 段階 5
  - 248 shader SPIR-V 化 → 領域 6 (段階 1 完遂後並走着手)
  - LLVKRenderer pipeline.cpp inline 実装 → 段階 4 で配置、領域 8

### 3.3 sub-step 1.4 で発覚した周辺修正の文脈共有

- `indra/newview/llviewerstats.cpp` L70-L115 の旧 "Minimal Vulkan" stub (Win `vulkan-1.dll` LoadLibraryA probe 用 ad-hoc 型定義) を撤去し、`#include "volk.h"` 1 行で代替済 (commit `48361e0a28`)
- L814-L896 の Windows vulkan-1.dll probe block は untouched (volk が提供する本物の `VkInstance` / `VkResult` / `VK_MAKE_API_VERSION` / `PFN_vk*` で symbol resolve 確認済)
- 段階 2 着手時の note: Win 用 LoadLibraryA probe は r42-α 着手時に volk 経由 (`volkInitialize()` 結果) へ refactor 予定 (sub-step 1.4 commit message 内に明示済)

### 3.4 段階 1 で確定した sub-doc `01-foundation.md` 修正履歴 (段階 2 sub-doc 起草の参考)

- §3.2 wrapper 3 file の置換方針 table を以下に refine 済 (sub-step 1.4 中の AYA review 反映):
  - `llglheaders.h` → volk 並走追加 (最末尾 `#endif` 前、GL include 残置、完全削除は段階 5)
  - `llglstates.h` → 段階 1 では touch しない (charter §2 領域 3 段階 3 PSO 化 scope に明確化)
  - `llgltypes.h` → GL type alias 残置 + Vulkan Vk* placeholder typedef 並走追記
- §3.4 sub-step 1.4 row + §4.1 #1-段階 1 acceptance row も同範式で update 済
- 段階 2 sub-doc 起草時の cadence: charter §7.4 sub-doc 構成は outline のみ確定値、起草時に同様の implementation friction で in-flight refine する pattern が成立 (本段階で実証)

---

## 4. 次 session 開始 action cadence

### 4.1 次 session 開始時の最初の action

1. **handoff 確認**: 本 handoff (`handoff-stage-1-complete.md`) を Read
2. **sub-step 1.5 commit 確認**: 本 session の編集 1 file (本 handoff doc) の commit を AYA 指示後 Claude 実行
3. **段階 1 完遂宣言**: commit 完遂後、段階 1 全 5 sub-step 完了 + §4.1 acceptance 4 件 satisfy 状態を AYA に報告 (本 handoff §1.1 + §2 を base)
4. **base 資料確認**: `00-charter.md` §2 領域 2 (段階 2) + §7.4 sub-doc 構成、`docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` §5.4 段階 2 (lldrawpool 13 file 内訳) を Read
5. **段階 2 着手前 prep の cadence 確認**: sub-doc `02-portage-execution.md` 起草を AYA さんと cadence 擦り合わせ (Pattern α/β 選択)

### 4.2 段階 2 着手前 prep の cadence 候補

- **Pattern (a) full prep 先行**: sub-doc `02-portage-execution.md` を Pattern α/β で起草完成 → AYA review PASS → 段階 2 sub-step 着手
- **Pattern (b) sub-doc 並走**: lldrawpool 13 file の 1 件目に着手しながら sub-doc 起草を並走
- **Pattern (c) retroactive sub-doc**: 段階 2 完遂時に sub-doc retroactive 起草 (段階 1 で `01-foundation.md` は prep として先行起草実績、本 pattern は r41 では非推奨)

cadence 選好は次 session 開始時に AYA さんと擦り合わせ。**推奨は Pattern (a)** (段階 1 で `01-foundation.md` 先行起草実績の継承、prep doc が着手 base を固める効果)。

### 4.3 段階 2 着手中の注意事項 (charter §6.5 + memory + feedback 反映、段階 1 から継承)

- **Linux first-class baseline 厳守**: 段階 1 と同様、NVIDIA proprietary on RTX 5090 で動作確認、Mesa RADV は段階 10 で再評価
- **parity 不要**: charter §1 thesis 維持、AYAstorm 改変 13 file shader (picker 2 / Cinematic 4 / visual realism 7) は r42-α/β/γ で port
- **acceptance satisfy は実機検証**: `feedback_build_only_verified.md` 遵守、段階 2 acceptance 推論 ban
- **仮説 2 連続外れ rule**: `feedback_admit_unknown.md` 反映、lldrawpool port trouble で log/canary/bisect 切替
- **sub-step 完遂時 self-trace + handoff**: `feedback_self_verify_before_handoff.md` + `feedback_proactive_handoff.md` 継承、段階 2 内 sub-step 境界でも適用
- **commit / push cadence**: commit は AYA 指示後 Claude 実行、push は AYA 手動

### 4.4 段階 1 deferred item の段階 2 着手時 cadence

- ~~loader version log mystery~~ (§3.1.1): **解消済 2026-05-28**、warmup 行追加で fix、段階 2 持ち越し不要
- **Mesa RADV 動作確認** (§3.1.2): 段階 2-9 進行中は据置、段階 10 driver matrix polish で改めて testbed 確保

---

## 5. 関連 doc / memory

### 関連 doc (r41 章)

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter 本体 (closed 2026-05-28 = 完成)
- `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` — sub-doc foundation (closed 2026-05-28、段階 1 着手 base、本段階完遂で役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-r41-charter-complete.md` — 前 handoff (r41 charter 完成 → 段階 1 着手前 prep の境界)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-1-complete.md` — **本 handoff** (段階 1 完遂 → 段階 2 着手の境界)
- (段階 2 着手後追加予定) `02-portage-execution.md` — lldrawpool Vulkan 化の base 資料

### 関連 doc (r40 章、段階 2 の base 資料)

- `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` — §5.4 段階 2 lldrawpool 13 file 内訳
- `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` — §4 render pass + §10 LLVKRenderer skeleton
- `docs/specs/ayastorm-r40-vulkan-migration/06-effort-estimation.md` — §3.1 領域 2 PM 1.00 工程算定

### 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone active (段階 1 完遂宣言を本 handoff で marker)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 + Linux 先行例外 (§3.1.2 Mesa RADV 据置の根拠)
- `feedback_self_verify_before_handoff.md` — sub-step 1.5 self-trace (本 handoff §2)
- `feedback_proactive_handoff.md` — chapter / 段階 境界 handoff (本 handoff で実施)
- `feedback_no_auto_commit.md` — sub-step 1.5 commit は AYA 指示後
- `feedback_release_flow.md` — push は AYA 手動
- `feedback_build_only_verified.md` — §2 acceptance satisfy は実機検証
- `feedback_admit_unknown.md` — §3.1.1 loader version log mystery は仮説 3 件未検証で fix 不可と判定、gdb trace 切替
- `feedback_self_bug_no_defer_option.md` — §3.1.1 defer は acceptance 無影響かつ調査 cadence 設定済、disable / 削除案出さず原因究明継続
- `feedback_release_with_user_feedback.md` — §2 #regression の exhaustive solo session 不要 (AYA 起動確認 PASS で sufficient)
- `feedback_use_agents_proactively.md` — 段階 2 着手中の lldrawpool 13 file trace で Agent 活用候補
