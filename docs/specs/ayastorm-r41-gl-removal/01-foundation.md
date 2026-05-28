# AYAstorm r41 sub-doc 01-foundation — 環境前提整備 + branch 戦略 + 段階 1 file list

**status**: **closed 2026-05-28 (Pattern α 一括 draft + AYA review PASS、段階 1 着手準備 ready)**
**親 charter**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (closed 2026-05-28 = 完成)
**達成条件**: 段階 1 完遂 = Vulkan instance / device 列挙 + queue family 選択動作 + 環境前提整備済 + branch 戦略確定
**関連 charter section**: §2 領域 1 (段階 1 GL header wrapper 置換 + volk loader、0.50 PM) + §3 #1/#3 acceptance + §5.5 環境前提 + §7.4 sub-doc 構成

---

## §1 環境前提整備 plan

### §1.1 必須 3rdparty list (charter §5.5 反映)

| 3rdparty | 種別 | 役割 | 取得元 |
|---|---|---|---|
| **LunarG Vulkan SDK 1.3.x** | SDK + tools | Vulkan loader / glslang / SPIRV-Cross / validation layer 同梱 | LunarG 公式 (Linux apt repo or tarball) |
| **volk** | header-only loader | Vulkan function pointer 動的 load (loader 経由 ICD 検出) | GitHub `zeux/volk` (header-only、git clone or curl) |
| **VMA (Vulkan Memory Allocator)** | header-only allocator | per-allocation VkDeviceMemory 管理 + per-thread staging pool (charter §2 領域 7 で使用) | GitHub `GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator` (header-only) |
| **glslang** | shader compiler | GLSL → SPIR-V pre-compile (charter §2 領域 6) | LunarG SDK 同梱 (`glslangValidator`) |
| **SPIRV-Cross** | shader reflect / cross-compile | shader debug + reflection (charter §2 領域 6 補助) | LunarG SDK 同梱 |

**段階 1 範囲**: LunarG SDK + volk のみ必須、VMA / glslang / SPIRV-Cross は段階 2/3/6 着手時に integration。本 sub-doc では LunarG SDK + volk の integration のみ詳細化。

### §1.2 取得方針 (Linux 限定、本 r41 scope)

#### LunarG SDK 1.3.x

- **取得方法**: Ubuntu 24.04+ apt repo (`vulkan-sdk` package、LunarG signed) または LunarG tarball (`https://vulkan.lunarg.com/sdk/home#linux`)
- **採用**: apt repo (system-wide install、依存性管理が apt 経由で完結)
- **fallback**: tarball (CI / non-Ubuntu Linux distro 対応の余地、本 r41 では Ubuntu 限定)

#### volk

- **取得方法**: GitHub `zeux/volk` から `volk.h` + `volk.c` を直接取り込み (header-only library、submodule or git clone or curl)
- **採用**: autobuild 3rdparty fetch script で `volk` package 化 (既存 autobuild flow 整合)
- **version**: `master` 最新 (本 r41 着手時に pin 確定、charter §7.5 boundary refine 可)

### §1.3 autobuild config 反映 (段階 1 着手 1.1 で実施)

| 修正対象 | 内容 |
|---|---|
| `autobuild.xml` | `vulkan-sdk` + `volk` package 定義追加 (Linux platform = `linux64`) |
| `indra/cmake/Vulkan.cmake` (新規) | `find_package(Vulkan REQUIRED)` + `include_directories(${Vulkan_INCLUDE_DIRS})` + volk include path |
| `indra/cmake/Linking.cmake` | Vulkan link 設定 (volk 経由 dynamic load なので直接 link は不要、ただし loader 検出 fallback で `vulkan-1` link 余地あり) |
| `indra/cmake/00-Common.cmake` | Vulkan + volk 検出フラグ + compile definitions (`VK_USE_PLATFORM_XLIB_KHR` 等の WSI) |
| `indra/llrender/CMakeLists.txt` | `llvkloader.cpp` + `.h` 追加 (本 sub-doc §3.3 新規 file) |

修正詳細は段階 1 sub-step 1.1 着手時に refine、本 sub-doc では outline level で確定。

### §1.4 動作確認手順 (段階 1 完遂 marker)

| confirm | 内容 |
|---|---|
| `vulkaninfo` 動作 | Mesa RADV (AMD) + Mesa ANV (Intel) + NVIDIA proprietary 3 driver で `vulkaninfo` が physical device + queue family + extension list を出力 |
| viewer build pass | `autobuild configure -A 64 -c ReleaseOS` + `autobuild build -A 64 -c ReleaseOS` が Vulkan + volk integration 込で pass |
| viewer 起動時 volk 初期化 | LL_INFOS log で volk 初期化成功 + Vulkan instance 作成成功 (validation layer error 0 件) |
| physical device 列挙 + queue family 選択 | Mesa RADV / NVIDIA proprietary で physical device 検出 + graphics + present queue 取得成功 (LL_INFOS log で device name + queue family index 出力) |

---

## §2 branch 戦略確定

### §2.1 採用方針 (2026-05-28 AYA 推奨採用)

- **新規 `feature/ayastorm-r41-gl-removal`** fork 確定
- **fork point**: `5d1da93c7f` (r41 charter 完成 commit、現 `feature/ayastorm-r40-vulkan-migration` HEAD)
- **fork 実施**: 2026-05-28 (本 sub-doc 起草直前に `git checkout -b feature/ayastorm-r41-gl-removal` 実行済)
- **r41 着手 commit**: 本 branch に積む (段階 1-10 全 commit 含む)

### §2.2 r40 章 doc + r41 charter の本線 merge cadence (後置判断)

現時点で merge cadence は確定しない (r41 完遂は ~7 年先で実害なし、現時点判断保留)。判断軸 outline のみ記載:

| candidate | 内容 | 評価 |
|---|---|---|
| (A) r41 着手前 merge | r40 章 doc + r41 charter (2 commit `4fbca524e5..5d1da93c7f`) を `ayastorm-release` に早期 merge | 早期 share、ただし r41 進行中 doc 修正が本線にも反映され誤情報 risk あり |
| (B) r41 完遂後 merge | r41 達成宣言時に r41 doc 群 (charter + sub-doc + handoff) を一括 merge | safe、ただし ~7 年先で本線 share 遅延 |
| **(C) milestone 単位 partial merge** | r40 章 (closed 2026-05-28) は早期 merge、r41 charter (active) は完遂後 merge | **折衷案、推奨度高い** (r40 章 close で stable、r41 charter は active で修正 risk) |

判断は r41 着手中の任意 timing で AYA + Claude 擦り合わせ可。本 sub-doc では **後置** で固定、確定値ではない (charter §7.5 boundary refine 可)。

### §2.3 branch 命名規則

- `feature/ayastorm-r41-gl-removal` (本 branch、r41 全段階を統合進行)
- r41 内の **段階別 sub-branch は作らない** (`feedback_experiment_branch_single_scope.md` 類推、scope drift 回避)
- r41.5 以降 milestone は **別 branch fork** (charter §6.3 (β)、`feature/ayastorm-r41-5-vk-repo-separation` 等)
- 段階別 commit は本 branch に積む (段階 1 完遂 / 段階 2 完遂 / 等の commit message prefix で区別)

### §2.4 commit cadence (charter §6.4 反映)

- **commit**: AYA 明示指示後 Claude 実行 (`feedback_no_auto_commit.md` 遵守)
- **push**: AYA 手動 (`feedback_release_flow.md` 遵守)
- **commit prefix**: `feat(r41)` / `refactor(r41)` / `docs(r41)` 等の `r41` scope 明示
- **段階完遂 commit**: 各段階完遂時に self-trace + AYA review + handoff doc (charter §6.4 並走方針)

---

## §3 段階 1 file list (charter §2 領域 1 詳細化)

### §3.1 段階 1 scope 再掲 (charter §2 領域 1 = 0.50 PM、04 §5.4 段階 1)

- 212 GL header → volk-based 置換 + Vulkan instance / device 初期化
- 04 §3 GL header 依存 189 file の 99% wrapper 経由 = wrapper 3 file 置換で 188 file 上流対応 (a-4 で判明した abstraction 設計の最大の追い風)

### §3.2 wrapper 3 file 置換 (04 §3 + §5.4 段階 1 反映)

| file | 役割 (現状) | 置換方針 (段階 1) |
|---|---|---|
| `indra/llrender/llglheaders.h` | GL header include 中継 (`#include <GL/gl.h>` 等) | **volk 並走追加** (`#include "volk.h"` を最末尾 `#endif` 前に追加、GL include 残置)。volk は内部で `VK_NO_PROTOTYPES` 後に `vulkan.h` を内包 + 全 entry を function pointer で再宣言するため、独立 `vulkan/vulkan.h` include は **proto 衝突 risk で技術的に不要**。**GL include 完全削除は段階 5 完遂時** (wrapper 内部のみ参照状態になった時点で実施、charter §3 #1 acceptance) |
| `indra/llrender/llglstates.h` | GL state 定数 / enum 中継 (`GL_BLEND` / `GL_DEPTH_TEST` 等の alias) | **段階 1 では touch しない**。中身は `LLGLDepthTest` / `LLGLSDefault` 等の state machine RAII class で、charter §2 領域 3 「段階 3: state machine → PSO 化 llrender 主要 5 file (1.50 PM)」 scope。段階 1 で並走 alias を入れる意味ある接点が無い (GL_BLEND は API capability、`VK_BLEND_OP_*` は PSO blend state、1:1 mapping 不可)。段階 3 着手時に Vulkan PSO state alias 整備 |
| `indra/llrender/llgltypes.h` | GL types alias 中継 (`GLuint` / `GLfloat` 等) | **GL type alias 残置 + Vulkan Vk* 型 alias 並走追記** (`#include "volk.h"` + `LLVkBuffer` / `LLVkImage` / `LLVkDeviceMemory` 等の placeholder typedef)、段階 2-5 の per-file port で type 一貫性確保 |

### §3.3 volk + Vulkan loader 統合 file (新規)

| file | 役割 | 内容 outline |
|---|---|---|
| `indra/llrender/llvkloader.h` (新規) | volk + Vulkan instance / device の public interface | volk 初期化 / Vulkan instance 作成 / physical device 列挙 / queue family 選択 / global Vulkan handle accessor の public API 宣言、05 §10.1 LLVKRenderer skeleton hook と一貫した signature placeholder |
| `indra/llrender/llvkloader.cpp` (新規) | volk + Vulkan instance / device 実装 | `volkInitialize()` 呼出 / `vkCreateInstance` (validation layer enable in debug build) / `vkEnumeratePhysicalDevices` + selection logic / `vkGetPhysicalDeviceQueueFamilyProperties` + graphics + present queue 選択 / `vkCreateDevice` + queue 取得 / RAII shutdown (`vkDestroyDevice` + `vkDestroyInstance` + `volkFinalize`) |

**hook 位置 (05 §10.1 反映)**: pipeline.cpp 内 inline 実装は段階 4 着手時に追加、本段階 1 では `llvkloader.{cpp,h}` の標準 entry point のみ整備 (interface 経由 call 化は r41.5)。

### §3.4 段階 1 内 implementation 順序 (5 sub-step)

| sub-step | 内容 | 完了 marker |
|---|---|---|
| **1.1** | volk + Vulkan SDK 3rdparty 取込 + autobuild + cmake 検出 | `cmake` configure pass + viewer build pass (GL 並行残置、本段階では GL 削除しない) |
| **1.2** | `llvkloader.cpp` + `.h` 新規追加 (volk 初期化 + Vulkan instance 作成) | viewer 起動時に volk 初期化成功 + Vulkan instance 作成成功 (validation layer error 0 件) |
| **1.3** | physical device 列挙 + queue family 選択 | Mesa RADV / NVIDIA proprietary で physical device 検出 + graphics + present queue 取得成功 (LL_INFOS log 確認) |
| **1.4** | wrapper 段階 1 scope file 並走追加 (`llglheaders.h` + `llgltypes.h`、`llglstates.h` は段階 3 PSO 化時) | viewer build pass + 起動 pass (GL call は段階 2-5 で順次除去、本段階では Vulkan header / Vk* 型 placeholder typedef の並走追加のみ) |
| **1.5** | 段階 1 self-check + AYA review + handoff doc | 04 §5.4 段階 1 file 全置換 + 本 §4 completion criteria PASS |

### §3.5 段階 1 で touch しない file (段階 2-5 scope、本 §3 外)

- lldrawpool 13 file (段階 2 scope)
- llrender 主要 5 file の state machine (段階 3 scope)
- pipeline.cpp 3 大グローバル `sCull` / `sShadowRender` / `sCurCameraID` (段階 4 scope)
- llspatialpartition / llviewershadermgr / llvertexbuffer / llvosky / llvowlsky (段階 5 scope)
- 248 shader (領域 6、段階 1 完遂後に並走着手)
- LLVKRenderer pipeline.cpp inline 実装 (段階 4 で配置、領域 8)

---

## §4 段階 1 completion criteria

charter §3 acceptance criterion #1 (GL 除去) + #3 (段階 1-5 全完遂) の **段階 1 分の self-check**:

### §4.1 段階 1 自己 acceptance

| criterion | metric | test procedure |
|---|---|---|
| **#1-段階 1 (wrapper 段階 1 scope file Vulkan header 並走追加)** | `llglheaders.h` + `llgltypes.h` に volk include / LLVk* placeholder typedef が並走追加済 (GL include 残置、完全削除は段階 5 完遂時)、`llglstates.h` は段階 3 PSO 化時に着手 | `grep -E "^#include \"volk\.h\"" indra/llrender/llglheaders.h indra/llrender/llgltypes.h` が **2 file 全件 hit** + `grep -E "typedef.*LLVk" indra/llrender/llgltypes.h` が **1 件以上 hit** + GL include は **残置許容** (charter §3 #1 acceptance の 0 件判定は r41 全完遂時) |
| **#3-段階 1 (Vulkan instance + device 動作)** | Vulkan instance 作成 + physical device 列挙 + queue family 選択動作 | viewer 起動 + LL_INFOS log で instance handle + device name + queue family index 出力確認、validation layer error 0 件 |
| **環境前提 (Linux driver 2 件動作)** | Mesa RADV + NVIDIA proprietary 2 driver で sub-step 1.1-1.5 全動作 | 各 driver で起動 pass + validation error 0 件 (ANV は段階 10 polish で対応、段階 1 は 2 driver で十分) |
| **regression (本線動作維持)** | viewer build pass + 起動 pass + 1 セッション動作 (GL call は段階 2-5 で除去、段階 1 では GL 並行残置) | autobuild + 起動 + 1 セッション (~10 分) 動作確認、AYAstorm 機能 (audio + chat + login 等) regression 0 件 |

### §4.2 不達時の対処 (charter §3 acceptance 運用方針継承)

- 4 criterion のうち 1 件でも未達 = 段階 1 未達 (段階 2 着手保留)
- 未達 criterion 別に対処 (例: #3-段階 1 で NVIDIA proprietary で physical device 検出失敗 → driver 別 quirks audit → workaround → 再 sweep)
- **defer / disable 提案 ban** (`feedback_self_bug_no_defer_option.md` 遵守、fix 案のみ提示)

### §4.3 段階 1 完遂後の次 段階

- **段階 2 着手** (lldrawpool Vulkan 化 13 file)、領域 6 (shader SPIR-V 化) + 領域 7 (descriptor / render pass) と並走
- **sub-doc `02-portage-execution.md` 起草** (段階 2 着手前に prep、charter §7.4 反映)
- **handoff doc `handoff-stage-1-complete.md` 作成** (段階 1 完遂境界、charter §6.4 並走方針継承)

---

## §5 関連 doc / memory

### §5.1 直接参照 doc

| doc | 本 sub-doc での参照 section |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | §2 領域 1 + §3 #1/#3 acceptance + §5.5 環境前提 + §6.4 並走方針 + §7.4 sub-doc 構成 + §7.5 boundary |
| `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` | §3 GL header 依存 189 file 内訳 + §5.4 段階 1 file 詳細 (wrapper 3 file 置換 base) |
| `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` | §1 基盤 (Vulkan 1.3 + volk + LunarG SDK 1.3.x、本 §1.1 採用根拠) + §7 swapchain (段階 1 では着手しないが next sub-step prep) + §10.1 LLVKRenderer skeleton placeholder (本 §3.3 hook 整合) |
| `docs/specs/ayastorm-r40-vulkan-migration/06-effort-estimation.md` | §3.1 領域 1 PM 0.50 (本 §1 + §3 段階 1 scope 整合) |

### §5.2 関連 memory

| memory | 本 sub-doc での参照 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | r41 milestone active 状態 + r41 着手前 prep cadence |
| `project_ayastorm_three_platforms.md` | 3 OS 大前提 + Linux 先行例外 (本 段階 1 は Linux 限定動作確認、§4.1 #環境前提 反映) |
| `project_ayastorm_r40_cpu_parallel.md` | r40 章 close + 工程プラン 8 件確定値 (charter §4 (4) 2 phase 構成 Phase 1 進行) |

### §5.3 関連 feedback

| feedback | 本 sub-doc での参照 |
|---|---|
| `feedback_experiment_branch_single_scope.md` | §2.3 branch scope 単一性 (r41 内 sub-branch 作らない) |
| `feedback_no_auto_commit.md` | §2.4 commit は AYA 指示後 |
| `feedback_release_flow.md` | §2.4 push は AYA 手動 |
| `feedback_self_bug_no_defer_option.md` | §4.2 defer / disable 提案 ban |
| `feedback_self_verify_before_handoff.md` | §3.4 sub-step 1.5 self-check + §4.3 段階 1 完遂境界 self-trace |
| `feedback_proactive_handoff.md` | §4.3 段階 1 完遂時の handoff doc 作成 |
| `feedback_build_only_verified.md` | §4 completion criteria の satisfy は実機検証 (推論 ban) |
| `feedback_admit_unknown.md` | sub-step 内 trouble で仮説 2 連続外れたら log/canary/bisect 切替 |
| `feedback_use_agents_proactively.md` | wrapper 置換 + autobuild config 反映で複数 file 連鎖修正 = Agent 活用 |

---

## §6 起草 cadence + 完成宣言

### §6.1 本 sub-doc 起草情報

- **起草着手**: 2026-05-28
- **起草主体**: AYA + Claude
- **起草先**: `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` (本 doc)
- **起草 cadence**: **Pattern α (一括 draft)** — sub-doc は内容具体 (file list / 3rdparty plan / branch 確定) で section 数少なく、Pattern β 分割 overhead 回避 (charter §7.5 boundary refine 可)

### §6.2 完成宣言条件

本 sub-doc は **AYA review PASS で完成宣言** (2026-05-28 達成)、status field を `closed 2026-05-28 (Pattern α 一括 draft + AYA review PASS、段階 1 着手準備 ready)` に更新済。

完成宣言後の次 action:

- 段階 1 sub-step 1.1 着手 (volk + Vulkan SDK 3rdparty 取込)
- 段階 1 sub-step 1.5 完遂時に handoff doc `handoff-stage-1-complete.md` 作成
- 段階 2 着手前に sub-doc `02-portage-execution.md` 起草 (charter §7.4)

### §6.3 本 sub-doc commit 反映 (charter footer 範式継承)

本 sub-doc 完成宣言 commit は AYA 明示指示後 Claude が実施。commit message draft (AYA 指示時 refine 可):

```
docs(r41): sub-doc 01-foundation.md 完成 + branch 戦略確定 + 段階 1 prep

- 01-foundation.md 新規作成 (Pattern α 一括 draft + AYA review PASS 2026-05-28)
- §1 環境前提整備 (LunarG SDK 1.3.x + volk loader + autobuild config 反映)
- §2 branch 戦略 (新規 `feature/ayastorm-r41-gl-removal` fork 確定、merge cadence 後置)
- §3 段階 1 file list (wrapper 3 file 置換 + llvkloader.{cpp,h} 新規 + 5 sub-step 順序)
- §4 段階 1 completion criteria (wrapper 置換 / instance + device 動作 / Linux 2 driver / regression)
- §5 関連 doc/memory + §6 起草 cadence
```

push は AYA 手動 (`feedback_release_flow.md` 遵守)。
