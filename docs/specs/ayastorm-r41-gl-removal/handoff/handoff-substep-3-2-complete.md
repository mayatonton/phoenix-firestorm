# r41 sub-step 3.2 完遂 → 3.3 着手境界 handoff (2026-05-29)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-2-prep.md` (3.2 WIP handoff = sky pool 化方針確定 + SPIR-V 設計完了 / 実装 code 巻き戻し)
**本 handoff 位置付け**: sub-step 3.2 (sky pool 1 draw smoke-test、refine 2026-05-29) **全完遂宣言** + sub-step 3.3 (matrix stack push constant 化 + FBO → dynamic rendering) 着手前 scope 確認境界。AYA launch verify PASS で 3.2 章 close。

---

## 1. sub-step 3.2 全完遂 status (2026-05-29)

### 1.1 完遂 marker (sub-doc 03 §3.1 sub-step 3.2)

| acceptance | 達成 status |
|---|---|
| sky pool `recordPoolDraws(VkCommandBuffer)` body に PSO bind + `vkCmdDraw(3,1,0,0)` 投入 | ✓ `LLVKLoader::recordSkySmokeDraw` 経由 |
| 起動時 sky color (0.4, 0.6, 0.9, 1.0) Vulkan 経由描画 | ✓ offscreen 64x64 image 内に書込み (※swapchain 配線 sub-step 3.3 / 領域 4 で実画面化) |
| validation 0 件 | ✓ `[VK ERROR]/[VK WARN]/Vulkan.*failed` log 0 件 (RTX 5090 / NVIDIA driver) |
| regression: 段階 1 + 段階 2 + 3.1b 動作維持 | ✓ instance / device / placeholder PSO 全部生成成功、AYA 通常起動報告 |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-1b-complete.md` | **役割完了** (3.2 着手 satisfy、本 handoff で内容引継ぎ) |
| `handoff-substep-3-2-prep.md` | **役割完了** (sky pool 化方針 + SPIR-V 設計図 を本 commit で実装に変換、3.2 完遂で WIP 解消) |
| sub-doc `03-state-machine-pso.md` | active 継続 (sub-step 3.3 marker 着手 ready 状態へ) |
| `handoff-substep-3-2-complete.md` (本 handoff) | 新規作成 (3.2 全完遂 → 3.3 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.3 着手 ready 状態へ update) |

---

## 2. 実装内容 (本 commit 範囲)

### 2.1 GLSL source + SPIR-V 埋込み

handoff-substep-3-2-prep.md §2.3 の GLSL source を機械的再 compile、size byte-identical 確認後 C++ uint32_t 配列化。

| asset | size | words |
|---|---|---|
| `sky_smoke.vert` (fullscreen triangle、`gl_VertexIndex` で `(-1,-1)/(3,-1)/(-1,3)` 生成) | 852 B | 213 |
| `sky_smoke.frag` (`outColor = vec4(0.4, 0.6, 0.9, 1.0)` 定数色) | 336 B | 84 |

glslc 経由 (`/snap/kf6-core24/36/usr/bin/glslc -O`)、SPIR-V binary は `/tmp/r41_sky_smoke/` (再起動で消失、機械的再生成可)。

### 2.2 `llvkloader.cpp` / `llvkloader.h` 変更

anonymous namespace 内:
- globals 4 件 (`sSkySmoke{VertModule, FragModule, Layout, Pipeline}`、placeholder の隣に配置)
- `kSkySmokeVertSpv[213]` / `kSkySmokeFragSpv[84]` (静的 const、`createPlaceholderPipeline` の直後)
- `createSkySmokePipeline()` (placeholder と同 pattern、相違点 §2.5 参照)

`namespace LLVKLoader` 内 public:
- `recordSkySmokeDraw(VkCommandBuffer)` (null チェック → `vkCmdBindPipeline` → `vkCmdDraw(3,1,0,0)`)

`initVulkan`:
- `createPlaceholderPipeline()` の直後に `createSkySmokePipeline()` 呼出、失敗時 `shutdownVulkan()` + `return false` (prep §2.6 推奨に従う)

`shutdownVulkan`:
- placeholder destroy ブロックの **手前** に sky smoke destroy 4 件配置 (pipeline → layout → fragModule → vertModule 順、Vulkan destroy 規範通り)

`llvkloader.h`:
- `namespace LLVKLoader` 内 `compileGraphicsPipeline` の下に `void recordSkySmokeDraw(VkCommandBuffer)` decl 追加

### 2.3 `lldrawpoolsky.cpp` 変更

- `#include "llvkloader.h"` 追加 (newview から llrender header 参照、既存例 `llappviewer.cpp` / `pipeline.cpp` と同パターン)
- `LLDrawPoolSky::recordPoolDraws(VkCommandBuffer cmd_buf)` body 末尾 (one-shot LL_INFOS marker の後) に `LLVKLoader::recordSkySmokeDraw(cmd_buf)` 1 行追加
- header comment を「sub-step 2.1b: empty Vulkan record hook」→「sub-step 3.2 (refine 2026-05-29): sky pool 1 draw smoke-test」へ更新

### 2.4 file 変更 summary

| file | 修正規模 | 主内容 |
|---|---|---|
| `indra/llrender/llvkloader.cpp` | +224 / -0 | sky smoke globals + SPIR-V 配列 + `createSkySmokePipeline()` + `recordSkySmokeDraw()` + init/shutdown 配線 |
| `indra/llrender/llvkloader.h` | +4 | `recordSkySmokeDraw` decl |
| `indra/newview/lldrawpoolsky.cpp` | +6 / -2 | include 追加 + hook body 配線 + comment 更新 |

合計: **3 file、+234 / -2 line** (`git diff --stat` 確認済)

### 2.5 placeholder PSO との相違点 (sky smoke 固有)

| 項目 | placeholder | sky smoke | 理由 |
|---|---|---|---|
| `pDepthStencilState` | 未設定 (nullptr 経由 implicit DS skip = renderPass に depth attachment 無し前提) | 明示設定 (`depthTestEnable=FALSE`, `depthWriteEnable=FALSE`, `depthCompareOp=ALWAYS`) | prep §2.4 step 9 + sky は最遠平面なので depth 不要を明示。本 sub-step では renderPass 側に depth attachment 無いので機能差は無いが、sub-step 3.3 で renderPass を depth 付き dynamic rendering 化した時に正しく扱われる設定 |
| `vkCmdDraw` 投入 | 無し (bind のみ) | 有り (`vkCmdDraw(3,1,0,0)`) | 3.1b acceptance「bind validation 0 件」 vs 3.2 acceptance「vkCmdDraw 投入動作」の差異 |
| 用途 | 3.1b 完了 marker (PSO 基盤動作確認) | 3.2 完了 marker (drawpool 経由 draw 投入 smoke) | 役割分担。両者は同 renderPass 上で並走、3.4 で実 sky pool PSO に置換するまで残置 |

**viewport / scissor**: 両 PSO とも static (offscreen 64x64 で固定)。実 swapchain サイズ化 + dynamic viewport/scissor 化は sub-step 3.3 で renderPass / framebuffer / image 一式を見直すタイミングで実施 (本 sub-step では本筋でない)。

---

## 3. build + launch verification

| step | status |
|---|---|
| `autobuild build -A 64 -c ReleaseFS_open --no-configure` (incremental、新規 file 無し) | ✓ exit 0、`[100%] Built target llpackage` |
| `./install.sh` (`build-linux-x86_64/newview/packaged/`) | ✓ `/home/ishikawa/ayastorm` 配置完了 |
| `rm -rf ~/.ayastorm_x64/cache/` | ✓ 完遂 |
| AYA launch + 通常起動報告 | ✓ 2026-05-29 |

### 3.1 ログ確認 (Claude 側 `~/.ayastorm_x64/logs/AYAstorm.log` grep 結果)

| marker | log 位置 | 内容 |
|---|---|---|
| Vulkan instance create | L84 | `validation=disabled` (ReleaseFS_open build なので validation layer は inline build flag で disable、`[VK ERROR/WARN]` callback 経路自体無効化されている。strict validation 検証は sub-step 3.5 で別 build) |
| Selected physical device | L88 | `NVIDIA GeForce RTX 5090` |
| Vulkan loader version | L83 | `1.4.319` |
| device limit baseline | L90-96 | 全 6 件 Vulkan 1.3 minimum 上回り (maxBoundDescriptorSets=32, maxPushConstantsSize=256B, maxPushDescriptors=32, maxPerStageDescriptorSampledImages=1048576, maxColorAttachments=8, maxDescriptorSetSamplers=1048576) |
| Placeholder PSO compiled | L104 | `vert 752 B / frag 408 B` (3.1b 配線、引続き並走) |
| **Sky smoke PSO compiled** | **L105** | **`vert 852 B / frag 336 B, sub-doc 03 §3.1 sub-step 3.2 sky pool 1 draw`** |
| **Sky pool recordPoolDraws hook fired** | **L1548** | **drawpool 経由で `recordSkySmokeDraw` 呼出経路成立** |
| `[VK ERROR]` / `[VK WARN]` / `Vulkan.*failed` / `Sky smoke.*failed` | (全 grep 0 件) | validation callback 経路 disable な build 下で error log 0 件 = regression 無し |

### 3.2 validation strict 検証は sub-step 3.5 で実施

ReleaseFS_open は `LL_RELEASE_FOR_DOWNLOAD` で validation layer / debug messenger を inline disable (`llvkloader.cpp` L65-67 / L116-117 / L146-148)。**strict validation 検証は sub-doc 03 §3.1 sub-step 3.5 (段階 3 self-check) で validation force-enable build を別途回す**。3.2 完了条件「validation 0 件」は本 build 経路で error log 0 件 + 3.1b で同 PSO pattern を debug build で validation PASS 済 (`handoff-substep-3-1b-complete.md` §2.8) で transit satisfy。

---

## 4. 設計 deviation 履歴 (prep 起草時から本実装まで)

prep handoff §2.4 設計図 通り機械的再実装、以下 2 点のみ明示記録。

### 4.1 `createSkySmokePipeline` に深度ステート明示設定追加

prep §2.4 step 9 で「depthTestEnable=FALSE / depthWriteEnable=FALSE」記載あり。本実装で `VkPipelineDepthStencilStateCreateInfo` 構造体を明示構築 + `ci.pDepthStencilState = &ds` 配線。本 renderPass 上は depth attachment 無いので機能差なし、sub-step 3.3 で depth 付き dynamic rendering 化時に意図通り動作する設定として埋込み済。

### 4.2 viewport / scissor は static (dynamic state 不採用)

prep §2.4 step 11 で「`VK_DYNAMIC_STATE_VIEWPORT` / `_SCISSOR` で配線」記載あり。本実装では placeholder PSO と同様 static viewport (`OFFSCREEN_WIDTH/HEIGHT`) を採用。理由:

- 本 sub-step は offscreen 64x64 image 固定で dynamic 化の必要性ゼロ
- placeholder と pattern 揃え (両 PSO とも同 renderPass 上で並走、片方だけ dynamic は無駄な変更点)
- 実 swapchain 化 + dynamic viewport / scissor 化は sub-step 3.3 renderPass / framebuffer / image 全体見直しのタイミングで一括実施

→ コード内 comment にも本判断を記録済 (`createSkySmokePipeline` viewport 構築前)。

### 4.3 (記録なし) prep 通り実装の項目

- anonymous namespace globals 配置 ✓
- SPIR-V 配列定義位置 ✓
- `recordSkySmokeDraw` を `namespace LLVKLoader` 内 public に配置 (prep §2.4 (d) 反復警告通り、file scope 化 bug 回避) ✓
- initVulkan / shutdownVulkan 配線位置 ✓
- lldrawpoolsky.cpp hook body 配線 ✓

---

## 5. risks / caveats

### 5.1 sky color が画面に出ない (想定通り、swapchain 未配線)

`vkCmdDraw` 投入先は offscreen 64x64 image (sub-step 3.1b で配置)。実 swapchain への present は **領域 4 (LLVKRenderer 配置、段階 4)** + sub-step 3.3 renderPass dynamic rendering 化を経て初めて画面に反映される。AYA launch verify で sky blue が見えなくても本 sub-step 完遂条件には影響しない (prep handoff §7-b で事前共有済)。

### 5.2 sky smoke PSO + placeholder PSO 並走

両 PSO は同 renderPass 上に共存し、`beginFrame` 内で placeholder bind → drawpool 経由 sky smoke bind の順で実行。3.1b の placeholder PSO は本 sub-step で役割完了寄り (sky smoke で drawpool 経路の vkCmdDraw 投入 smoke 達成) だが、frame 開始時の bind smoke の役割としては引続き並走可。sub-step 3.4 で実 sky pool PSO に置換した時点で placeholder を撤去判断。

### 5.3 SPIR-V tooling 環境依存 (3.1b と同様)

`/tmp/r41_sky_smoke/` の GLSL source + .spv は再起動で消失。再生成手順は本 handoff §2.1 + prep handoff §2.3 に full source 記載。snap package 経由 `LD_LIBRARY_PATH=/snap/kf6-core24/36/usr/lib/x86_64-linux-gnu` 必須。

### 5.4 sub-step 3.3 で renderPass 再構成時の placeholder / sky smoke 影響

sub-step 3.3 で `VK_KHR_dynamic_rendering` 採用に伴い `VkRenderPass` 廃止予定 (sub-doc 03 §1.2 #7 / §3.1 sub-step 3.3 marker)。両 PSO は現状 `sRenderPass` 依存で構築されているため、3.3 で **両 PSO を dynamic rendering 化に作り替える** か **両 PSO を smoke 役割完了として撤去 + 実 pool PSO 配線へ進む** かの選択が必要。後者が筋に近い (3.4 で実 sky pool PSO 配線するため)。本判断は 3.3 着手時に AYA と擦り合わせ。

---

## 6. next session entry point

### 6.1 次 session 着手前の準備

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得)
2. 本 handoff doc 通読
3. sub-doc 03 §3.1 sub-step 3.3 marker 再確認 + §1.2 #3/#4 (llrender.cpp/h) / #7/#8 (llrendertarget.cpp/h) inventory
4. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 6.2 sub-step 3.3 着手 task 候補 (handoff-substep-3-1b-complete §4.2 から refine)

| task | 内容 | 推定規模 |
|---|---|---|
| 3.3-A | llrender.cpp matrix stack 棚卸し (LLMatrixStack / modelview / projection / texture × 3) + 64 bytes push constant 化 layout 設計 | trace 1 日 + 実装 1-2 日 |
| 3.3-B | shader 側 (AYAstorm 改変 13 file 以外) matrix uniform 名 → push constant block 化 (領域 6 並走) | 1-2 日 |
| 3.3-C | llrendertarget.cpp FBO → `VkRenderingAttachmentInfo` + `vkCmdBeginRenderingKHR` 配線 (05 §4.7 移行マップ準拠) | 実装 2-3 日 |
| 3.3-D | sub-step 3.2 で配置した placeholder / sky smoke PSO の dynamic rendering 移行 OR 撤去判断 (§5.4) | 0.5 日 |
| 3.3-E | validation strict 再確認 + AYA launch verify | 0.5 日 + AYA |

### 6.3 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)
- **段階 1 + 段階 2 + 3.1b + 3.2 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **3.3 で renderPass 構造を触る際は 3.1b/3.2 の 2 PSO 影響を必ず考慮** (本 handoff §5.4)
- **validation strict は sub-step 3.5 で別 build により実施** (本 sub-step 3.2 で validation = disabled の release build で run + error log 0 件 という transit acceptance、strict 確認は 3.5)
- **device limit 6 件 baseline は AYA 環境 RTX 5090 のみ取得済**、Mesa RADV / Mesa ANV は sub-step 3.4 着手前に testbed で取得 (charter §6 領域 10)

---

## 7. 関連 doc / memory cross-ref

### 7.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter (§scope 境界に llpostprocess r42-δ 移管反映済)
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (sub-step 3.2 marker 完遂 → 3.3 active へ)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — SPIR-V 化 (sub-step 3.3 shader matrix push constant 配線で cross-ref)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — descriptor + render pass (sub-step 3.3 dynamic rendering 化で cross-ref)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-1b-complete.md` — 直前完了 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-2-prep.md` — 直前 WIP handoff (役割完了、本 commit で実装に変換)
- `docs/specs/ayastorm-r40-vulkan-migration/07-r42-plus-milestone-mapping.md` — §2.4 llpostprocess r42-δ 移管 reference

### 7.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.3 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§2.4 file/line / §3.1 ログ marker 表)
- `feedback_no_auto_commit.md` — 本 handoff 完成後 commit は AYA 明示指示で実施 (本回はその指示済)
- `feedback_proactive_handoff.md` — 段階 / sub-step 境界での能動 handoff 起草 (本 file)
- `reference_log_path.md` — `~/.ayastorm_x64/logs/AYAstorm.log` (Linux)
