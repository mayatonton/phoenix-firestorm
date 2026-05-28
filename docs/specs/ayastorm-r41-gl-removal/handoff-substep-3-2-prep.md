# handoff: AYAstorm r41 sub-step 3.2 prep (WIP handoff)

**作成**: 2026-05-29
**status**: sub-step 3.2 **WIP handoff** (方針確定 + SPIR-V 設計完了、実装 code 巻き戻し、次 session で機械的再実装可)
**前 handoff**: `handoff-substep-3-1b-complete.md` (3.1b 完遂 → 3.2 着手境界、commit 8e8a846c14)
**charter**: `00-charter.md`
**sub-doc**: `03-state-machine-pso.md` §3.1 sub-step 3.2

---

## 1. なぜ WIP handoff か

前 session で sub-step 3.1b 完遂 commit (8e8a846c14) push 後、sub-step 3.2 に着手し以下を進めた:

- **方針確定 (AYA 承認済)**: sub-step 3.2 smoke-test を当初想定の **llpostprocess legacy effect uniform 移植** → **sky pool 1 draw** へ refine。llpostprocess は upstream Firestorm 由来の dead code stub (`apply()` 呼出 0 件 + effect 関数 empty body) と sub-step 3.2 着手 trace で発覚、本実装は **r42-δ basket** に移管 (charter §scope 境界 + sub-doc 03 §3.1 sub-step 3.2 marker + r42-plus-mapping §2.4 反映、commit 対象 = 本 handoff と同時)
- **SPIR-V 設計完了**: fullscreen triangle 技法で vertex buffer 不要、`gl_VertexIndex` で 3 頂点 (`(-1,-1) / (3,-1) / (-1,3)`) 生成、frag は sky blue `(0.4, 0.6, 0.9, 1.0)` 出力。glslc コンパイル成功 (vert 852B = 213 words / frag 336B = 84 words)
- **実装着手 + 巻き戻し**: `llvkloader.cpp` に SPIR-V 埋込み + `createSkySmokePipeline()` + `recordSkySmokeDraw()` を追加したが、**namespace 構造 bug** (recordSkySmokeDraw が file scope、header convention は `namespace LLVKLoader`) が残った状態で前 session summary が走った。summary 後 context 残量が読みにくく、build + install + AYA launch verify (5-10 分 + 結果対処) を今 session で完遂するリスクを取らず、**実装は `git restore` で巻き戻し、設計図を本 handoff doc に保存して機械的再実装可能な状態で引き継ぐ** 判断を採用 (AYA から「あなたの判断に任せます」)

doc 3 件 refine は実装と独立して valid なので、本 handoff doc と同時に commit して 3.2 着手境界の方針確定を残す。

---

## 2. 完了済 (本 handoff 時点で確定)

### 2.1 doc 4 件 refine (本 commit に含む 3 件 + 既 commit memory)

| file | 変更内容 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` | §1.5 placement table sub-step 3.2 row + §3.1 sub-step 3.2 marker を **sky pool 1 draw、2026-05-29 refine** に書き換え。llpostprocess 本実装 r42-δ 移管 note 追加 |
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | §scope 境界 動作不可 list に「llpostprocess legacy effects (bloom / NightVision / ColorFilter) — upstream Firestorm 由来 dead code stub、r42-δ basket 移管 2026-05-29」追加、sub-doc 03 §3.1 cross-ref |
| `docs/specs/ayastorm-r40-vulkan-migration/07-r42-plus-milestone-mapping.md` | §2.4 r42-δ work breakdown に「llpostprocess legacy effects 現代化 +0.20 PM (再算定保留)」 row 追加、totals は「(llpostprocess 0.20 別途、r42-δ 着手時に一括再算定)」 suffix |
| `~/.claude/.../memory/project_ayastorm_r41_vulkan_migration.md` | status + 段階表 + sub-doc 表 + handoff 系譜 + next action + non-scope に r42-δ llpostprocess 移管 line 追加 (3.1b complete commit 8e8a846c14 に含む) |

### 2.2 sky pool 化方針 (AYA 承認済)

sub-step 3.2 の smoke-test target を以下に refine:

- **対象**: `LLDrawPoolSky::recordPoolDraws(VkCommandBuffer)` (`indra/newview/lldrawpoolsky.cpp` L60-68)
- **追加実装**: `llvkloader.cpp` に sky pool 専用 minimal SPIR-V (vert + frag) 埋込み + `createSkySmokePipeline()` + `recordSkySmokeDraw(VkCommandBuffer)`
- **既存 placeholder PSO**: 3.1b で追加された `createPlaceholderPipeline()` + `beginFrame` 内 placeholder bind は **残置** (placeholder は frame 開始時の PSO bind smoke の役割、sky smoke は drawpool 経由の vkCmdDraw 投入 smoke の役割、独立)
- **descriptor**: 不要 (fullscreen triangle + 定数色 frag、3.2-B #11 descriptor helper は本 sub-step では未配線)

### 2.3 SPIR-V 設計 (機械的再生成可、source は `/tmp/r41_sky_smoke/`)

**`sky_smoke.vert` (GLSL → 852B SPIR-V = 213 words)**:

```glsl
#version 450

// Fullscreen triangle technique: vkCmdDraw(3, 1, 0, 0) で gl_VertexIndex 0/1/2 を入力、
// vertex 入力 binding 不要、画面全体を 1 triangle で被覆。
// (-1,-1), (3,-1), (-1,3) の 3 頂点を生成 → screen clip space を超過する 2 頂点が
// rasterizer cull で切られ、残った領域が screen 全面。
void main()
{
    vec2 pos = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
```

**`sky_smoke.frag` (GLSL → 336B SPIR-V = 84 words)**:

```glsl
#version 450

// r41 sub-step 3.2 smoke-test 用 sky color (識別しやすい sky blue)。
// sub-step 3.4 で実 sky uniform / texture sampler 配線、本 frag は smoke のみ。
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(0.4, 0.6, 0.9, 1.0);
}
```

**glslc コンパイル**:

```bash
mkdir -p /tmp/r41_sky_smoke
cd /tmp/r41_sky_smoke
# GLSL source を上記内容で書き出してから:
LD_LIBRARY_PATH=/snap/kf6-core24/36/usr/lib/x86_64-linux-gnu \
  /snap/kf6-core24/36/usr/bin/glslc -O -o sky_smoke.vert.spv sky_smoke.vert
LD_LIBRARY_PATH=/snap/kf6-core24/36/usr/lib/x86_64-linux-gnu \
  /snap/kf6-core24/36/usr/bin/glslc -O -o sky_smoke.frag.spv sky_smoke.frag
# 期待 size: vert 852B / frag 336B (size 違いは glslc version 差、再生成して words 数で確認)
```

**C++ 埋込み変換** (SPIR-V binary → `const uint32_t[]`):

```bash
# words 数: file_size / 4 (SPIR-V は 32-bit word stream)
xxd -i -c 12 sky_smoke.vert.spv  # → 213 words の unsigned char[] 出力、uint32_t で読み替え
# もしくは od -An -t x4 sky_smoke.vert.spv | sed 's/ /, 0x/g' で uint32_t hex list 化
```

### 2.4 実装設計 (`llvkloader.cpp` に追加する 4 ブロック)

**(a) anonymous namespace globals** (L48 付近、`s*` 既存 globals の隣):

```cpp
// r41 sub-step 3.2 (refine 2026-05-29): sky pool smoke-test PSO
VkShaderModule   sSkySmokeVertModule = VK_NULL_HANDLE;
VkShaderModule   sSkySmokeFragModule = VK_NULL_HANDLE;
VkPipelineLayout sSkySmokeLayout     = VK_NULL_HANDLE;
VkPipeline       sSkySmokePipeline   = VK_NULL_HANDLE;
```

**(b) anonymous namespace 内 SPIR-V embedded arrays** (`createPlaceholderPipeline` の隣):

```cpp
// r41 sub-step 3.2: sky_smoke.vert.spv (213 words)
const uint32_t kSkySmokeVertSpv[] = { /* 213 words, 再生成時 (2.3) の glslc 出力を埋込み */ };

// r41 sub-step 3.2: sky_smoke.frag.spv (84 words)
const uint32_t kSkySmokeFragSpv[] = { /* 84 words, 同上 */ };
```

**(c) anonymous namespace 内 `createSkySmokePipeline()`** (`createPlaceholderPipeline` parallel):

```cpp
bool createSkySmokePipeline()
{
    // 1. vkCreateShaderModule x 2 (vert / frag、kSkySmokeVertSpv / kSkySmokeFragSpv)
    // 2. createStandardPipelineLayout(nullptr, 0, nullptr, 0) で sSkySmokeLayout 取得
    //    (descriptor 不要 / push constant 不要)
    // 3. VkPipelineShaderStageCreateInfo stages[2] (VERTEX / FRAGMENT、entrypoint "main")
    // 4. VkPipelineVertexInputStateCreateInfo: bindingCount=0 / attributeCount=0
    // 5. VkPipelineInputAssemblyStateCreateInfo: TRIANGLE_LIST
    // 6. VkPipelineViewportStateCreateInfo: dynamic state で配線 (viewport/scissor 1 / 1)
    // 7. VkPipelineRasterizationStateCreateInfo: POLYGON_FILL / CULL_NONE / FRONT_FACE_CCW
    // 8. VkPipelineMultisampleStateCreateInfo: SAMPLE_COUNT_1_BIT
    // 9. VkPipelineDepthStencilStateCreateInfo: depthTest=FALSE / depthWrite=FALSE
    //    (sky は最遠平面なので depth 不要、後段配線時に sub-doc 03 §3.3 で再評価)
    // 10. VkPipelineColorBlendStateCreateInfo: attachment 1 / blend off / writeMask=RGBA
    // 11. VkPipelineDynamicStateCreateInfo: VK_DYNAMIC_STATE_VIEWPORT / _SCISSOR
    // 12. VkGraphicsPipelineCreateInfo: layout=sSkySmokeLayout / renderPass=sRenderPass / subpass=0
    // 13. compileGraphicsPipeline(ci, sSkySmokePipeline) で生成、PipelineCache 経由
    // 14. shader module は pipeline 生成後 destroy しても OK (Vulkan spec)、ただし shutdown まで保持
    // return true / log failure with LL_WARNS
}
```

**(d) `namespace LLVKLoader { }` 内 `recordSkySmokeDraw()`**:

```cpp
namespace LLVKLoader
{
    // ... 既存 functions ...

    void recordSkySmokeDraw(VkCommandBuffer cmd_buf)
    {
        if (cmd_buf == VK_NULL_HANDLE || sSkySmokePipeline == VK_NULL_HANDLE)
        {
            return;
        }
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sSkySmokePipeline);
        vkCmdDraw(cmd_buf, 3, 1, 0, 0);
    }
}
```

**重要**: 前 session の bug は recordSkySmokeDraw を file scope に書いて namespace LLVKLoader の convention から外れたこと。**再実装時は必ず `namespace LLVKLoader { }` 内に配置する** (header decl と整合)。

### 2.5 header 追加 (`llvkloader.h`)

`namespace LLVKLoader` 内、`compileGraphicsPipeline` の下に追加:

```cpp
    // r41 sub-step 3.2 smoke-test: sky pool 用 minimal PSO bind + vkCmdDraw 投入
    // (sub-doc 03 §3.1 sub-step 3.2、2026-05-29 refine、llpostprocess は r42-δ 移管)
    void recordSkySmokeDraw(VkCommandBuffer cmd_buf);
```

### 2.6 initVulkan / shutdownVulkan 配線 (`llvkloader.cpp`)

**initVulkan** (createPlaceholderPipeline 呼出の直後):

```cpp
if (!createSkySmokePipeline())
{
    LL_WARNS("Vulkan") << "Sky smoke PSO creation failed" << LL_ENDL;
    // sub-step 3.2 では失敗時 → 全体失敗扱いするか、placeholder のみで継続するかは AYA 確認
    // 推奨: 失敗時 false return (smoke-test の意味が無くなる)
    return false;
}
LL_INFOS("Vulkan") << "Sky smoke PSO created" << LL_ENDL;
```

**shutdownVulkan** (placeholder destroy の隣):

```cpp
if (sSkySmokePipeline != VK_NULL_HANDLE)
{
    vkDestroyPipeline(sDevice, sSkySmokePipeline, nullptr);
    sSkySmokePipeline = VK_NULL_HANDLE;
}
if (sSkySmokeLayout != VK_NULL_HANDLE)
{
    vkDestroyPipelineLayout(sDevice, sSkySmokeLayout, nullptr);
    sSkySmokeLayout = VK_NULL_HANDLE;
}
if (sSkySmokeFragModule != VK_NULL_HANDLE)
{
    vkDestroyShaderModule(sDevice, sSkySmokeFragModule, nullptr);
    sSkySmokeFragModule = VK_NULL_HANDLE;
}
if (sSkySmokeVertModule != VK_NULL_HANDLE)
{
    vkDestroyShaderModule(sDevice, sSkySmokeVertModule, nullptr);
    sSkySmokeVertModule = VK_NULL_HANDLE;
}
```

### 2.7 lldrawpoolsky.cpp hook 配線 (`indra/newview/lldrawpoolsky.cpp` L60-68)

```cpp
void LLDrawPoolSky::recordPoolDraws(VkCommandBuffer cmd_buf)
{
    static bool logged_once = false;
    if (!logged_once)
    {
        LL_INFOS("VkRecord") << "Sky pool recordPoolDraws hook fired (one-shot)" << LL_ENDL;
        logged_once = true;
    }
    // r41 sub-step 3.2 smoke-test: sky color を fullscreen triangle で出力 (vkCmdDraw 投入)
    LLVKLoader::recordSkySmokeDraw(cmd_buf);
}
```

include 追加: `#include "llvkloader.h"` (lldrawpoolsky.cpp 既存 include に応じて挿入)

---

## 3. 残作業 (次 session 着手 6 step)

| # | 内容 | 工数目安 |
|---|---|---|
| 1 | SPIR-V 再コンパイル + `xxd -i` で uint32_t hex list 生成 + .cpp 埋込み配列に貼付 | 5 分 |
| 2 | `llvkloader.cpp` に anonymous namespace globals 4 件 + SPIR-V arrays + `createSkySmokePipeline()` 追加 | 10 分 |
| 3 | `llvkloader.cpp` の `namespace LLVKLoader { }` 内に `recordSkySmokeDraw()` 追加 (file scope に書かない、前回 bug の再発防止) | 5 分 |
| 4 | `llvkloader.h` に `recordSkySmokeDraw` decl 追加 (§2.5) | 2 分 |
| 5 | `initVulkan` + `shutdownVulkan` 配線 (§2.6) | 5 分 |
| 6 | `lldrawpoolsky.cpp` recordPoolDraws hook 配線 (§2.7) + include | 5 分 |
| build | autobuild configure + build (incremental、llrender + llappearance 程度の小範囲) | 5-8 分 |
| install | rm packaged + autobuild install + cache clear (`~/ayastorm/` / `~/.ayastorm_x64/cache/shader_cache/`) | 2 分 |
| verify | AYA launch + Vulkan validation strict + log 確認 (1 frame Vulkan 描画達成 = sub-step 3.2 完遂) | 5 分 + AYA 検証 |

**着手目安**: 実装 ~35 分 + build/install ~10 分 + verify ~5 分 = ~50 分 / 1 session 内完遂可能。

---

## 4. sub-step 3.2 完遂条件 (sub-doc 03 §3.1 sub-step 3.2 marker)

- sky pool `recordPoolDraws(VkCommandBuffer)` body 内に PSO bind + `vkCmdDraw(3, 1, 0, 0)` 投入動作
- 起動時画面に **Vulkan 経由 sky color (0.4, 0.6, 0.9) = sky blue** が描画される (※GL 経由の現状画面に被って表示される、または別 viewport / 別 layer で表示される、配線は次 session で確定)
- validation strict 0 件 (placeholder PSO + sky smoke PSO 2 件並走)
- regression: 段階 1 + 段階 2 動作維持 (前 session で 8e8a846c14 commit で確定済)

---

## 5. 次 session entry point

```
状況: AYAstorm r41 sub-step 3.2 prep handoff (WIP) 受け取り、本実装着手段階。
方針: docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-2-prep.md §2 / §3 に従って sky pool 1 draw smoke-test を機械的に再実装、build + install + AYA launch verify で sub-step 3.2 完遂。
注意: recordSkySmokeDraw は file scope ではなく namespace LLVKLoader 内に配置 (前回 bug の再発防止、§2.4 (d))。
```

---

## 6. cross-refs

- 前: `handoff-substep-3-1b-complete.md` (3.1b 完遂 commit 8e8a846c14)
- 本: 本 handoff doc
- 次: `handoff-substep-3-2-complete.md` (sub-step 3.2 完遂時に作成、3.3 着手境界へ)
- sub-doc: `03-state-machine-pso.md` §1.5 / §3.1 sub-step 3.2
- charter: `00-charter.md` §scope 境界 (llpostprocess r42-δ 移管 line)
- r42 mapping: `docs/specs/ayastorm-r40-vulkan-migration/07-r42-plus-milestone-mapping.md` §2.4 (llpostprocess +0.20 PM 別途)
- memory: `project_ayastorm_r41_vulkan_migration.md` (next action / non-scope)

---

## 7. risks / caveats

- **(a) recordSkySmokeDraw 配置 bug 再発**: 前 session で file scope に書いた、header convention は `namespace LLVKLoader`。再実装時 §2.4 (d) + §2.5 を必ず確認
- **(b) sky color が現状画面に被って GL と二重表示**: r41 段階では parallel-rail (GL + Vulkan 共存) 想定、二重表示は smoke-test の前提として許容、sub-step 3.4 で render path 整理時に解消
- **(c) placeholder PSO + sky smoke PSO 並走で validation 出る可能性**: 同一 renderPass 内で 2 PSO bind が重なる場合、bind の順序は `beginFrame` placeholder → drawpool hook 内 sky smoke の順。validation error 出たら placeholder bind を `beginFrame` から外す判断 (3.1b 役割完了済なので外して OK)
- **(d) depth state**: sky smoke PSO は depthTest=FALSE / depthWrite=FALSE で書く。sub-doc 03 §3.3 で render path 配線時に再評価
- **(e) /tmp/r41_sky_smoke/ は再起動で消える**: 次 session 開始時に GLSL source 再作成必須 (§2.3 に full source あり、機械的再生成可)
