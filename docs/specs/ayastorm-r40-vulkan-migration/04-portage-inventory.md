# r40 sub-phase 3 work item (a): Vulkan portage 棚卸し

**status**: a-1 + a-2 + a-3 + a-4 (棚卸し総括) 全完了 → work item (a) 全完了、work item (b) Vulkan API 設計 着手前
**親 doc**: `03-sub-phase-3-vulkan-plan.md` work item (a)
**達成条件**: §6 棚卸し総括 完成 → work item (b) Vulkan API 設計 着手

---

## §1 現状棚卸し (a-1 完了)

a-1 = repo 現状の客観 numbers を集める phase。4 領域を Explore agent 並列で実施 (2026-05-28)。

### §1.1 `indra/llrender/` 棚卸し

| 項目 | 数値 |
|---|---|
| 全 file 数 | 51 (header 25 + source 26) |
| 合計 LOC | **28,168** |
| 合計 `gl*` API call | **381** |

**主要 file (LOC + GL call) — TOP 10:**

| file | LOC | gl* calls | 主要 interface |
|---|---|---|---|
| llgl.cpp | 3,027 | 81 | LLGLManager, LLGLState |
| llimagegl.cpp | 2,663 | 59 | LLImageGL, LLImageGLThread |
| llrender.cpp | 2,207 | 44 | LLRender, LLTexUnit, LLLightState |
| llglslshader.cpp | 2,089 | 41 | LLGLSLShader, LLShaderFeatures |
| llvertexbuffer.cpp | 1,942 | 40 | LLVertexBuffer |
| llrender2dutils.cpp | 1,872 | 2 | LLRender2D |
| llshadermgr.cpp | 1,689 | 32 | LLShaderMgr |
| llfontgl.cpp | 1,458 | 0 | LLFontGL, LLFontDescriptor |
| llfontfreetype.cpp | 970 | 0 | LLFontFreetype |
| llfontregistry.cpp | 830 | 0 | LLFontRegistry |
| llrendertarget.cpp | 589 | 35 | LLRenderTarget |
| llpostprocess.cpp | 454 | 11 | LLPostProcess |
| llgltexture.cpp | 400 | 0 | LLGLTexture |
| llcubemap.cpp | 343 | 3 | LLCubeMap |

**暫定分類 (a-1 段階の粗判定):**

| 分類 | file 数 | LOC | 含まれる file |
|---|---|---|---|
| 要 port | 8 | ~7,800 | llvertexbuffer / llglslshader / llfontgl / llfontregistry / llshadermgr / llrender2dutils / llcubemap / llcubemaparray |
| 要再設計 | 5 | ~10,600 | llgl / llrender / llimagegl / llrendertarget / llpostprocess |
| 不要 port | 3 | ~1,820 | llfontfreetype / llfontfreetypesvg / llfontbitmapcache |
| 判定保留 | 25 | ~5,950 | llglheaders.h 等 utility / macro 集約 |

→ 厳密な per-file verdict は a-2 で精緻化、a-1 は粗分類のみ。

### §1.2 GL header include 範囲棚卸し

| 項目 | 数値 |
|---|---|
| 全 file 数 (indra/) | 3,098 |
| GL include file 数 | **189 (6.1%)** |
| 直接 `<GL/gl.h>` include | **1** (llglheaders.h のみ) |
| 間接 (`llgl.h` 経由) | 188 |

**directory 別:**

| directory | 全 file 数 | GL include 数 | 直接 | 間接 (llgl.h) |
|---|---|---|---|---|
| indra/llrender | 51 | 28 | 2 | 17 |
| indra/newview | 1,754 | 82 | 0 | 48 |
| indra/llui | 230 | 19 | 0 | 19 |
| indra/llwindow | 40 | 5 | 0 | 5 |
| indra/llprimitive | 40 | 3 | 0 | 3 |
| indra/llappearance | 34 | 2 | 0 | 1 |
| media_plugins | 22 | 5 | 0 | 5 |

**AYAstorm/Firestorm 追加 file:**
- `fsmaniprotatejoint.cpp`: `#include "llgl.h"` 経由 (Firestorm)
- `fspanellogin.cpp`: `#include "llglheaders.h"` 直接 (Firestorm Login UI)
- `llayaudit.cpp`: GL include なし (AYAstorm r30 audit tool)

**含意**: 99% の file が `llgl.h` wrapper 経由。**`llglheaders.h` + `llgl.h` の 2 ヶ所書換で広範対応可能**、これは Vulkan abstraction 設計の最大の追い風 (memory 想定 「212 files」 はほぼ整合、実測 189)。

例外: `indra/llwindow` (5 file) + `media_plugins` (5 file) の platform-specific GL 処理は要個別調査 (WGL / GLX 系)。

### §1.3 GLSL shader 棚卸し

| 項目 | 数値 |
|---|---|
| 全 shader file 数 | **248** (想定通り) |
| vertex shader | 110 |
| fragment shader | 124 |
| include / util | 14 |
| compute shader | **0** |
| geometry shader | **0** |
| tessellation shader | **0** |

**directory 別:**

| directory | file 数 |
|---|---|
| class1/deferred | 120 (deferred rendering core) |
| class1/interface | 44 (UI) |
| class3/deferred | 16 (high-end feature) |
| class1/objects | 14 |
| class1/lighting | 9 |
| class1/windlight | 8 |
| cinematic_bd | 3 (Cinematic 専用, AYAstorm r30) |
| その他 (avatar/effects/environment/gltf/post) | 34 |

**GLSL version / extension:**
- `#version` directive 明示なし (runtime preprocessor 注入)
- 推定 GLSL 3.3 core ベース + GL_ARB_* 拡張
- Mac GL 4.1 制約あり (compute / SSBO / bindless は使用回避)
- extension 使用: GL_ARB_shader_texture_lod / GL_ARB_texture_rectangle / GL_EXT_gpu_shader4

**AYAstorm 改変 shader 抜粋 (a-3 で詳細化):**

| shader | 章 | 内容 |
|---|---|---|
| godraysF.glsl / godraysV.glsl | r15 | screen-space light shaft ray-march (BD import) |
| volumetricLightF.glsl (class3) | r30 BD import | BlackDragon volumetric lighting |
| cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl | r30 | Cinematic 専用 SSR utility |
| ayaAlphaPlateCompositeF.glsl | r30 P5 | AYAstorm 独自 alpha BLEND composite (DoF layered depth) |
| postDeferredHQDoFF.glsl / postDeferredNoDoFF.glsl | r30 | macOS guard 強化 (NaN/Inf, Apple Silicon Metal 安全化) |
| blurLightF.glsl | r30 P3.8 | SSAO-only vs all-channel blur permutation |

**SPIR-V 移行計量:**
- sampler 数: 206 (descriptor set 再設計対象)
- matrix uniform: 252 (mat4)
- textureGather / textureLod 等: 13
- buffer/SSBO 言及: 74 (推測)
- bindless / atomic / coherent: **0** (移行リスク低)

**含意**: ~85% は GLSL 3.3 core subset、cross compile (glslang) で素直に通る見込み。compute / geometry / tessellation 全部ゼロは段階移行に追い風。最大課題は **sampler 206 個の descriptor set 再設計**。

### §1.4 pipeline.cpp + AYAstorm 固有機能棚卸し

**pipeline.cpp 本体:**

| 項目 | 数値 |
|---|---|
| pipeline.cpp LOC | **14,579** |
| pipeline.h LOC | 1,375 |
| 合計 | **15,954** |

**3 大グローバル参照 (memory root cause):**

| グローバル | 参照数 | 主要 file |
|---|---|---|
| `LLPipeline::sCull` | **79** | pipeline.cpp: 72 + 他 5 file |
| `LLPipeline::sShadowRender` | **31** | pipeline.cpp: 12 + 他 11 file |
| `LLPipeline::sCurCameraID` | **66** | pipeline.cpp: 14 + 他 12 file |
| **合計** | **176** | |

**cull/stateSort 内 GL 呼出 (具体 file:line):**
- `llvieweroctree.cpp:1134` — `glGetQueryObjectuiv(mOcclusionQuery[...], GL_QUERY_RESULT_AVAILABLE, &available)`
- `llvieweroctree.cpp:1146` — `glGetQueryObjectuiv(mOcclusionQuery[...], GL_QUERY_RESULT, &query_result)`
- `pipeline.cpp:4006, 4072` — `checkOcclusion()` 呼出

**geometry mutation:**
- `llspatialpartition.cpp:377` — `new LLVertexBuffer(mVertexDataMask)` (VBO allocation)
- `llspatialpartition.cpp:378` — `allocateBuffer(vertex_count, index_count)` (GPU upload)
- `pipeline.cpp:3136-3145` — `LLPipeline::markOccluder()` (state mutation)
- `pipeline.cpp:4021, 4083` — `rebuildMesh()` 呼出

**AYAstorm 固有機能 touchpoint:**

| 機能 | 章 | 主要 touchpoint |
|---|---|---|
| **self-rigged picker** | r21.1 | pipeline.cpp:1151 `mObjectIDBuffer.allocate(resX, resY, GL_RGBA, false)` / :10757 bindTarget() / :10857 flush() / :11060-11061 shader hook |
| **visual realism** | r14+ | pipeline.cpp:5073-5121 deferred pools / :5275-5282 atmospherics / :800 + :1555 `FSRenderVignette` cvar |
| **Cinematic mode** | r30 | pipeline.cpp:3008-3009 `isCinematicMode()` / :9784-9785 DoF condition / :10153, :7102, :7179, :7394, :9515 mode 分岐 5 箇所 |

**AYAstorm 追加 cvar 描画関連 (top 4):**
- `RenderDepthOfField`: 9 箇所
- `FSRenderVignette`: 2 箇所
- `RenderSpotLightsInNondeferred`: 3 箇所
- `RenderDeferredAtmospheric`: 1 箇所
- `RenderSSAO*` / `RenderShadow*` 系: 合計 45+ 箇所

**その他 file 棚卸し補助:**

| file/dir | 数 | LOC | 備考 |
|---|---|---|---|
| `lldrawpool*.cpp` | 13 files | **7,907** | base 1,186 + alpha/terrain/bump/avatar 各 1,169-1,186 |
| `llviewershadermgr.{cpp,h}` | 2 files | **4,423** | LLViewerShaderMgr |
| `llspatialpartition.cpp` | 1 file | **4,416** | geometry rebuild + occlusion + spatial partition |
| `llvosky.cpp` + `llvowlsky.cpp` | 2 files | (est. 2-3K) | atmospherics + sky |

### §1.5 a-1 横断 summary + 新発見

**LOC 合計の更新 (memory 想定との比較):**

| 範囲 | memory 想定 | a-1 実測 | 差 |
|---|---|---|---|
| indra/llrender/ files | 16 | 51 (header 25 + source 26) | +35 (memory は source のみ計上想定?) |
| indra/llrender/ LOC | 28.2K | 28,168 | ほぼ一致 |
| indra/llrender/ GL calls | 464 | 381 | -83 (定義 / count 手法差) |
| GL header include files | 212 | 189 | -23 |
| GLSL shader files | 248 | 248 | 一致 |
| pipeline.cpp LOC | (未記載) | 14,579 (+ pipeline.h 1,375) | 新規確定 |

**a-1 で確定した portage 規模 (Vulkan 化作業の真の本丸 LOC):**

| 領域 | LOC | 性質 |
|---|---|---|
| indra/llrender/ | 28,168 | GL API 直叩き層 (要 port + 要再設計) |
| pipeline.cpp + .h | 15,954 | 描画 orchestrator + 3 大グローバル 176 参照 |
| lldrawpool*.cpp | 7,907 | 各 pool 描画 dispatcher |
| llspatialpartition.cpp | 4,416 | geometry rebuild + cull |
| llviewershadermgr.{cpp,h} | 4,423 | shader manager |
| GLSL shader | 248 file (LOC 未集計) | SPIR-V 移行対象 |
| **Vulkan 化 critical path 合計** | **~60.9K LOC + 248 shader** | (memory 想定 28.2K + 464 GL call は llrender/ 単独、pipeline.cpp 以下込みで約 2 倍規模) |

**新発見 (memory 未記載で a-1 で判明したもの):**

1. **GL header 依存は局在的**: 全 file 3,098 のうち GL 依存は 189 file (6.1%)、しかも直接 `<GL/gl.h>` は 1 ヶ所 (llglheaders.h) のみ。**99% の file は `llgl.h` wrapper 経由** → Vulkan abstraction 設計の追い風、wrapper 2 ヶ所書換戦略が現実的。
2. **GLSL は SPIR-V 移行リスク低**: compute / geometry / tessellation 全部ゼロ、bindless / atomic / coherent 全部ゼロ、~85% は GLSL 3.3 core subset。最大課題は sampler 206 個の descriptor set 再設計のみ。
3. **pipeline.cpp が真の本丸**: 15.9K LOC + 3 大グローバル 176 参照 + AYAstorm 固有機能 3 系統 (r21.1 picker / r14+ vignette / r30 Cinematic) の touchpoint 集約点。r41.5 abstraction 分離 milestone の critical path。
4. **lldrawpool*.cpp が見落とし範囲**: 13 file / 7.9K LOC、memory には未計上。Vulkan 化では各 pool の draw call emission を vulkan command buffer 化する必要、portage 範囲に追加。
5. **AYAstorm 固有機能の touchpoint は局在**: 改変 shader 5-7 件 + cvar 数十件 + pipeline.cpp 内数十箇所、独立 modular で抽出可能。Vulkan 化作業を「LL 基盤」と「AYAstorm 拡張」で分離設計しやすい。

---

## §2 各 file の 4 軸分類 verdict (a-2 完了)

a-2 = a-1 暫定分類を per-file で精緻化する phase。51 + 13 + 5 file の verdict を 4 軸で確定 (2026-05-28)。

### §2.1 `indra/llrender/` 51 file の per-file verdict

#### §2.1.1 要 port (16 file / 6,518 LOC)

GL API が Vulkan equivalent で 1:1 対応可能、interface 残置で実装可能。

| file | LOC | 根拠 | Vulkan 実装方針 |
|---|---|---|---|
| llvertexbuffer.{cpp,h} | 2,288 | VBO 管理。glBindBuffer / glBufferData / glDrawElements | VkBuffer + vkCmdBindVertexBuffers + vkCmdDraw* |
| llglslshader.{cpp,h} | 2,484 | shader compile。glCreateShader / glCompileShader | glslang SPIR-V crosscompile → VkShaderModule |
| llfontgl.{cpp,h} | 1,719 | text rendering driver。glyph rasterize → texture upload | llimagegl 依存型変更のみ、interface 残置 |
| llfontregistry.{cpp,h} | 974 | font file cache、GL dependency なし | 変更不要 (pure data structure) |
| llshadermgr.{cpp,h} | 2,221 | shader feature detect、GL 依存は glslang UBO layout check | UBO layout は Vulkan descriptor set layout に置換 |
| llrender2dutils.{cpp,h} | 2,050 | 2D primitive。matrix/color は LLRender 経由、GL call 2 件 | LLRender Vulkan 化後に自動継承 |
| llcubemap.{cpp,h} | 435 | cube map texture | VK image + sampler (VK_IMAGE_VIEW_TYPE_CUBE) |
| llcubemaparray.{cpp,h} | 297 | cube map array | 同上 (VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) |
| llfontvertexbuffer.{cpp,h} ★ | 369 | font VB abstraction、GL call なし | LLVertexBuffer Vulkan 化後に自動継承 |
| llglcommonfunc.{cpp,h} ★ | 68 | utility helper (stencil)、glStencilOp 2 call | wrapper で VkStencilOp に置換 |

★ = a-1 で判定保留から a-2 で昇格

#### §2.1.2 要再設計 (10 file / 7,937 LOC、pipeline / spatial は §2.3 に再掲)

GL state machine / FBO semantics / query async が Vulkan model と非対応、内部実装全替。

| file | LOC | 根拠 | 構造変更案 |
|---|---|---|---|
| llgl.{cpp,h} | 3,514 | GL capability + state machine + 延遅 query | VkPhysicalDevice properties + PSO + VkQueryPool |
| llrender.{cpp,h} | 2,789 | matrix stack + texture unit activate + immediate mode | push constant matrix + descriptor set texture binding |
| llimagegl.{cpp,h} | 3,034 | texture upload + GL_UNPACK_* alignment | staging buffer + vkCmdCopyBufferToImage、alignment は device limit |
| llrendertarget.{cpp,h} | 783 | FBO binding + depth texture share | VkRenderPass + VkFramebuffer、layout transition 明示 |
| llpostprocess.{cpp,h} | 721 | post-process chain (FBO ping-pong) | VkRenderPass chain + descriptor set reuse |
| llglheaders.h ★ | 1,095 | GL header aggregator (`#include <GL/gl.h>` + extension) | Vulkan header + volk loader に全置換 (本 file が wrapper の境界) |
| llglstates.h ★ | 196 | state scoped wrapper (LLGLDepthTest 等)、GL enum | VkPipeline state enum に再 map、interface 残置可 |
| llgltypes.h ★ | 39 | GL type alias (GLuint / GLenum) | Vulkan type (uint32_t / VkFormat) に置換 |

★ = a-1 で判定保留から a-2 で昇格

#### §2.1.3 不要 port (3 file / 1,701 LOC)

OpenGL 固有処理なし、FreeType / SVG / bitmap cache のみ。

| file | LOC | 根拠 |
|---|---|---|
| llfontfreetype.{cpp,h} | 1,171 | FreeType glyph cache、GL dependency なし → そのまま保持 |
| llfontfreetypesvg.{cpp,h} | 259 | SVG font (FreeType 上層)、GL 依存なし |
| llfontbitmapcache.{cpp,h} | 271 | bitmap font cache、pure data structure |

#### §2.1.4 判定保留 → 確定 (a-3 完了、17 file)

a-3 で各 file の中身確認 + GL 依存度評価 → verdict 確定。

**main 6 file の確定:**

| file | LOC | verdict | 根拠 | Vulkan 実装方針 |
|---|---|---|---|---|
| llgltexture.{cpp,h} | 614 | **要 port** | LLImageGL proxy、`mGLTexturep` 経由委譲、interface は LLTexture base で abstracted | VkImage + VkSampler、interface 残置 |
| lltexture.{cpp,h} | 118 | **要 port** | pure abstract interface、virtual base | interface 変更なし、派生側 Vulkan 化で自動継承 |
| lluiimage.{cpp,h} | 321 | **要再設計** | `gGL.getTexUnit(0)->bind()` + `gGL.color4fv()` で immediate mode 使用、LLRender2D 経由 matrix/color state 依存 | LLRender + LLRender2D Vulkan 化後に descriptor set + push constant 置換 |
| llrendernavprim.{cpp,h} | 108 | **要 port** | navmesh VB renderer、`gGL.begin(LLRender::TRIANGLES)` wrapper 経由 | LLVertexBuffer Vulkan 化後に gGL 経由 draw を `vkCmdDraw*` 自動継承 |
| llrendersphere.{cpp,h} | 181 | **要 port** | sphere geometry generator、VBO + gGL.flush() | base port で自動継承、flush → command buffer submit |
| lltexturemanagerbridge.{cpp,h} | 81 | **不要 port** | pure interface bridge (`getLocalTexture()` 仮想)、GL dependency なし | interface 残置、実装側 (newview) で対応 |

**header 集約系 11 file の一括判定:**

| 分類 | file 数 | 代表 file (LOC) | 根拠 | Vulkan 実装方針 |
|---|---|---|---|---|
| **要再設計** wrapper header | 5 | llglheaders.h (1,095) / llglstates.h (196) / llgltypes.h (39) + 2 関連 | GL header direct include + enum/type alias、wrapper 境界 | `#include <vulkan/vulkan.h>` + volk + VkFormat / VkCompare* enum 置換 (§2.1.2 で既に列挙済) |
| **要 port** interface header | 6 | llrender.h (582) / llglslshader.h (395) / llimagegl.h (371) / llvertexbuffer.h (346) / llpostprocess.h (267) / llfontgl.h (261) | state machine / matrix stack / VBO abstraction、public interface、GL call は cpp 局在 | interface 残置、対応 cpp と pair で扱う (§2.1.1 / §2.1.2 で既に {cpp,h} 表記で計上済) |

**§2.1.4 集計 (a-3 確定):**

| 確定 verdict | file 数 | LOC | 移動先 |
|---|---|---|---|
| 要 port (main 4 + interface header 6) | 10 | 1,021 + 2,222 = 3,243 | §2.1.1 / §2.1.2 既存 cpp と pair で吸収 |
| 要再設計 (main 1 lluiimage + wrapper header 5) | 6 | 321 + 1,330 = 1,651 | §2.1.2 既存 (llglheaders / llglstates / llgltypes 含) |
| 不要 port (main 1 lltexturemanagerbridge) | 1 | 81 | §2.1.3 |
| **小計** | **17** | **4,975 LOC** | |

注: a-2 §2.1.4 概算 LOC 8,995 と a-3 確定 LOC 4,975 の差 4,020 LOC は a-2 概算誤差 (header 集約系 LOC を大雑把に見積もっていた)。§6 棚卸し総括 (a-4) で 4 軸全合計確定時に再集計、charter §4 (2) の portage 規模数字反映で整合。

### §2.2 `indra/newview/lldrawpool*.cpp` 13 file (7,358 LOC)

**全て要 port** (各 pool は draw call dispatch を担当、Vulkan command buffer 化必須)。

| file | LOC | 役割 | Vulkan 実装方針 |
|---|---|---|---|
| lldrawpool.cpp | 1,186 | base pool orchestrator、state reset | render pass state setup → command buffer prepare |
| lldrawpoolalpha.cpp | 1,169 | alpha blending、transparency | VkPipelineColorBlendAttachmentState |
| lldrawpoolavatar.cpp | 1,110 | avatar mesh + skinning | bone matrix uniform → SSBO + compute |
| lldrawpoolbump.cpp | 1,122 | bump map + normal map | multi-texture → descriptor set |
| lldrawpoolmaterials.cpp | 367 | material property pool | PBR uniform → push constant / SSBO |
| lldrawpoolpbropaque.cpp | 148 | PBR opaque pool | materials pool 同様 |
| lldrawpoolsimple.cpp | 380 | simple geometry (non-deferred) | basic draw → vkCmdDraw* |
| lldrawpoolsky.cpp | 57 | sky dome | minimal state、single draw |
| lldrawpoolterrain.cpp | 1,167 | terrain quad + procedural UV (`glTexGen`) + polygon offset | **shader 側 explicit UV 計算に変更** (glTexGen 廃止)、polygon offset → `depthBiasConstantFactor` |
| lldrawpooltree.cpp | 243 | tree、alpha-test | alpha test → fragment shader discard |
| lldrawpoolwater.cpp | 358 | water surface、dual-layer texture | descriptor set reuse |
| lldrawpoolwaterexclusion.cpp | 79 | water exclusion boundary | simple quad render |
| lldrawpoolwlsky.cpp | 521 | Windlight sky dome、atmosphere | shader + uniform |

**特記**: terrain.cpp の `glTexGen` は Vulkan に対応物なし → shader 側 explicit coordinate 計算に変更必須 (要再設計寄りの分岐、ただし interface 影響は terrain pool 内に閉じる)。

### §2.3 critical path 5 file (24,793 LOC)

| file | LOC | verdict | 構造変更案 | AYAstorm 影響 |
|---|---|---|---|---|
| pipeline.cpp | 14,579 | **要再設計** | 3 大グローバル → frame context 化。cull/occlusion → GPU-driven or compute。render stage → VkRenderPass chain | **高**: r21.1 picker (mObjectIDBuffer 4 call) → render pass attachment。r30 Cinematic (isCinematicMode 6 分岐) → stage layer。r14+ vignette (FSRenderVignette) → post-process descriptor set |
| pipeline.h | 1,375 | 要再設計 | member type (GLQuery → VkQueryPool、FBO → VkFramebuffer) | r21.1 mObjectIDBuffer 型変更のみ |
| llspatialpartition.cpp | 4,416 | 要再設計 | occlusion query → VkQuery async。`glPolygonMode` → validation layer or pipeline variant。VBO は base port で自動継承 | **中**: spatial tree 独立、picker/cinematic 依存なし |
| llviewershadermgr.cpp ★ | 4,059 | **要 port** | shader loading → glslang SPIR-V invoke。feature 動的有効化 → device features + pipeline cache | **中**: GLSL → SPIR-V で shader hook (r30 Cinematic SSR / r14+ shader) integration point 変更、a-3 で cinematic shader set 抽出 |
| llviewershadermgr.h | 364 | 要 port | public interface (setShaders / updateShaderUniforms) → Vulkan equivalent | 同上 |

★ = a-1 で見落とし、a-2 で追加計上

### §2.4 a-1 暫定 → a-2 確定の変動 summary

| 分類 | a-1 file | a-1 LOC | a-2 file | a-2 LOC | 変動 |
|---|---|---|---|---|---|
| 要 port (llrender) | 8 | 7,800 | 16 | 6,518 | +8 file 昇格 (llfontvertexbuffer / llglcommonfunc + llrender2dutils 等の確定) |
| 要再設計 (llrender) | 5 | 10,600 | 10 | 7,937 | +5 file 昇格 (llglheaders / llglstates / llgltypes、pipeline / spatial は §2.3 に再掲) |
| 不要 port | 3 | 1,820 | 3 | 1,701 | LOC 微調整 |
| 判定保留 | 25 | 5,950 | 17 | 8,995 | 8 file 確定移動、17 file 継続保留 (a-3 で確定) |
| 要 port (lldrawpool 新規) | 0 | 0 | 13 | 7,358 | 新規追加 (a-1 見落とし範囲) |
| 要再設計 (critical path 新規) | 0 | 0 | 3 | 20,370 | 新規追加 (pipeline.cpp + .h + llspatialpartition) |
| 要 port (llviewershadermgr 新規) | 0 | 0 | 2 | 4,423 | 新規追加 (a-1 見落とし) |
| **Vulkan portage critical path 合計** | — | ~60.9K | — | **~57.3K + 17 file 判定保留 8.9K = ~66K LOC** | a-1 想定からさらに精緻化、shader 248 file は別途 |

## §3 要 port file の Vulkan 等価実装方針 (a-2 結果統合)

要 port の合計 = **31 file / 18.3K LOC** (llrender 16 + lldrawpool 13 + llviewershadermgr 2)。

主要方針:

- **vertex / index buffer**: `VkBuffer` + `vkCmdBindVertexBuffers` + `vkCmdDraw*` (LLVertexBuffer interface 残置、内部実装置換)
- **shader compile**: glslang で GLSL → SPIR-V crosscompile → `VkShaderModule` (LLGLSLShader / LLViewerShaderMgr interface 残置)
- **texture/sampler**: `VkImage` + `VkSampler` + descriptor set (cube map / array は `VK_IMAGE_VIEW_TYPE_CUBE[_ARRAY]`)
- **draw pool dispatch**: 各 pool の draw call emission を `vkCmdDraw*` 化、blend / alpha は `VkPipelineColorBlendAttachmentState`
- **font glyph**: FreeType rasterize はそのまま、texture upload 経路のみ Vulkan 化
- **shader manager**: feature 動的有効化を `VkPhysicalDeviceFeatures` + pipeline cache に map

interface は概ね保持可能 → caller (newview / llui 等 188 file の wrapper 経由側) は最小限の変更で済む。

## §4 要再設計 file の構造変更案 (a-2 結果統合)

要再設計の合計 = **13 file / 28.3K LOC** (llrender 10 + critical path 3)。

最重要 = pipeline.cpp + llspatialpartition.cpp。以下の構造変更:

### §4.1 pipeline.cpp + pipeline.h (15,954 LOC)

- **3 大グローバル → frame context object 集約**: `LLPipelineFrameContext` (仮称) に sCull / sShadowRender / sCurCameraID を集約、frame ごとに per-frame instance を生成、worker thread からの参照を一方向 read-only に統制
- **cull / occlusion → GPU-driven or compute shader 化検討**: CPU side の `checkOcclusion` ループを VkQuery async + compute shader culling に置換 (Vulkan 1.3 のメリット活用)
- **render stage dispatch → VkRenderPass chain**: shadow / deferred / forward / post-process の各 pass を VkRenderPass / subpass で表現、Vulkan 1.3 dynamic rendering (render pass less) 採用も検討
- **AYAstorm 拡張は render stage 層で modular factoring**:
  - r21.1 picker → render pass color attachment (mObjectIDBuffer → `VkImage` attachment)
  - r30 Cinematic → shader variant + DoF state (pipeline 層で分岐維持)
  - r14+ vignette → post-process descriptor set 内で実装

### §4.2 llspatialpartition.cpp (4,416 LOC)

- **occlusion query**: `glGetQueryObjectuiv` → `vkGetQueryPoolResults` (async fetch、frame in flight = 2-3 で latency 吸収)
- **`glPolygonMode` (wireframe debug)**: validation layer or `VkPipeline` variant で対応
- **VBO allocate / rebuildMesh**: LLVertexBuffer 要 port 完成で自動継承、追加作業最小

### §4.3 llgl.cpp + llrender.cpp + llimagegl.cpp + llrendertarget.cpp + llpostprocess.cpp (10.8K LOC)

- **state machine 廃止 → PSO (Pipeline State Object) 化**: `glEnable` / `glDisable` / `glBlendFunc` 等の immediate state を VkPipeline 事前生成に変更、state combination の cardinality を事前列挙
- **texture unit binding 廃止 → descriptor set 化**: `glActiveTexture` + `glBindTexture` を `vkCmdBindDescriptorSets` に置換
- **FBO → VkRenderPass + VkFramebuffer**: ping-pong も VkRenderPass chain で表現
- **matrix stack → push constant / uniform buffer**: immediate mode matrix stack を Vulkan push constant or per-frame uniform buffer に変更

### §4.4 llglheaders.h + llglstates.h + llgltypes.h (1.3K LOC)

- **GL header wrapper 全置換**: `<GL/gl.h>` → Vulkan header + volk loader (188 file の wrapper 依存側は変更不要、本 file が境界)
- **enum 再 map**: GL state enum → VkPipeline state enum
- **type alias 置換**: `GLuint` → `uint32_t`、`GLenum` → `VkFormat` 等

## §5 AYAstorm 固有機能の描画 stage 依存度マップ (a-3 完了)

a-3 で各機能の関連 shader 抽出 + touchpoint 詳細化 + 概算工数感 確定。

### §5.1 r21.1 self-rigged picker (touchpoint 局在度: **小**)

**関連 shader (a-3 抽出):**
- `class1/deferred/fsObjectIDV.glsl` + `fsObjectIDF.glsl` (2 file, ~200 LOC) — vertex/fragment output に LocalID/ObjectID 書込

**pipeline.cpp touchpoint:**
- :1151 `mObjectIDBuffer.allocate(resX, resY, GL_RGBA, false)` — render target allocation
- :10757 / :10857 `bindTarget()` / `flush()` — FBO bind/unbind
- :11060-11061 shader hook (LocalID write → deferred gbuffer post)

**Vulkan 化での変更:**
- `mObjectIDBuffer` type: LLRenderTarget → VkRenderPass color attachment (deferred main pass の inline attachment として統合可能、または別 pass)
- shader: GLSL → SPIR-V で output layout 変更なし (fragment output write は layout 宣言のみ)
- **概算工数感**: pipeline.cpp 4 LOC + shader 2 file + render pass attachment 1-2 日 → **軽量 modular** (フルタイム dev で ~0.5 人月、(c) で精緻化)

### §5.2 r14+ visual realism (touchpoint 局在度: **中**)

**関連 shader (a-3 抽出、5-7 件):**
- `godraysF.glsl` / `godraysV.glsl` — uniform: 6 sampler + 8 mat4
- `volumetricLightF.glsl` (class1 + class3) — uniform: 4 sampler + 5 mat4
- `blurLightF.glsl` / `blurLightV.glsl` — uniform: 3 sampler + 4 mat4
- `atmosphericsFuncs.glsl` / `atmosphericsF.glsl` (include) — uniform: ~8 sampler
- **合計 sampler ~25 + mat4 ~30** → descriptor set 再構成対象

**llvosky.cpp + llvowlsky.cpp (a-3 で追加 verdict):**
- 合計 **2,198 LOC / verdict 要 port** (sky dome rendering、shader hook 点のみ GL dependent、VBO 化済、atmospherics uniform は push constant / uniform buffer 置換)

**pipeline.cpp touchpoint:**
- :5073-5121 deferred pools 周辺 (post-process hook)
- :5275-5282 atmospherics 後 light scattering
- :800 / :1555 `FSRenderVignette` cvar (post-process condition branch)

**Vulkan 化での変更:**
- godrays / volumetricLight / blurLight: **post-process pass chain 統合** (descriptor set 1 個で sampler 25 個管理)
- cross compile (glslang): 全 shader GLSL 3.3 core subset → **SPIR-V 素直に通る見込み**
- **概算工数感**: shader 7 file SPIR-V 化 + post-process descriptor set 設計 + pipeline.cpp post-process chain 5 LOC → **中規模** (フルタイム dev で ~2-3 人月、(c) で精緻化)

### §5.3 r30 Cinematic mode (touchpoint 局在度: **中**)

**関連 shader (a-3 抽出、4 改変 file):**
- `cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl` — uniform: 2 sampler + 6 mat4 (SSR)
- `ayaAlphaPlateCompositeF.glsl` — uniform: 2-3 sampler (alpha plate composite for DoF layered depth)
- `postDeferredHQDoFF.glsl` / `postDeferredNoDoFF.glsl` — uniform: 1-2 sampler (macOS guard、DoF quality variant)

**SPIR-V 移行リスク:**
- compute shader / atomic / bindless: **全部なし** (vs/fs のみ)
- cross compile (glslang): **通る見込み**

**pipeline.cpp touchpoint (6 分岐):**
- :3008-3009 `isCinematicMode()` 判定
- :10153 volumetric lighting gate + :9784-9785 DoF condition
- :7102 / :7179 / :7394 / :9515 mode 分岐 (計 5 箇所、render stage layer selection)

**DoF state 構造:**
- 現在: `RenderDepthOfField` cvar (9 箇所参照) + `mFSDoFViewMode` + BD live cvar 13 件
- Vulkan 化案: frame context に `DoFMode enum` 集約、per-frame uniform buffer or push constant で配信 (state machine 廃止)

**概算工数感**: shader 4 file SPIR-V 化 + DoF state enum 化 + pipeline.cpp 6 分岐の frame context 統合 → **中規模** (フルタイム dev で ~2-3 人月、(c) で精緻化)

### §5.4 段階 port 戦略 (a-3 詳細化、5 段階 + AYAstorm 3 機能合成順)

#### §5.4.1 base LL port (r41) 内訳順序 (5 段階)

| 段階 | 内容 | 概算工数感 (フルタイム dev) |
|---|---|---|
| **1** | GL header wrapper 置換 (llglheaders.h + llglstates.h + llgltypes.h → Vulkan header + volk loader)。全 188 file upper layer は変更不要 (wrapper 経由) | 2-3 週間 / 0.5 人月 |
| **2** | lldrawpool 全 13 file の Vulkan command buffer 化 (interface 残置で modular)。特殊: terrain.cpp `glTexGen` → shader 側 explicit UV 計算 (1-2 日追加) | 3-4 週間 / 1 人月 |
| **3** | llgl / llrender / llimagegl / llrendertarget / llpostprocess の state machine → PSO 化 (capability detect → VkPhysicalDeviceFeatures、matrix stack → push constant、FBO → VkRenderPass + VkFramebuffer) | 4-5 週間 / 1.5 人月 |
| **4** | pipeline.cpp 3 大グローバル → `LLPipelineFrameContext` 集約 (per-frame instance)、cull/occlusion GPU-driven 検討、render stage dispatch → VkRenderPass chain。**最難関** | 3-4 週間 / 1 人月 |
| **5** | llspatialpartition / llviewershadermgr / llvertexbuffer 等の依存解決 (occlusion query → vkGetQueryPoolResults async、shader manager → device features + pipeline cache) | 2-3 週間 / 0.5 人月 |
| **r41 base port 合計** | | **13-17 週間 / 4-5 人月** |

#### §5.4.2 AYAstorm 3 機能 patch 合成順 (r42 以降)

base port 完成後の incremental patch、touchpoint 規模順 + 独立度順:

| milestone | 機能 | 内容 | 概算工数感 |
|---|---|---|---|
| **r42-α** | r21.1 self-rigged picker | mObjectIDBuffer → render pass attachment、shader 2 file SPIR-V 化、read-pick 確認テスト | 1-2 週間 / 0.5 人月 |
| **r42-β** | r30 Cinematic mode | DoF state enum 化 + frame context 統合、shader 4 file SPIR-V 化、visual quality テスト | 2-3 週間 / 1 人月 |
| **r42-γ** | r14+ visual realism | post-process pass chain 統合、shader 7 file SPIR-V 化、performance profile | 3-4 週間 / 1.5 人月 |
| **r42+ AYAstorm 合成合計** | | | **6-9 週間 / 3 人月** |

#### §5.4.3 工数見積の重要注記

**段階 port 戦略の工数見積 (base 4-5 + AYAstorm 3 = 合計 7-8 人月) は フルタイム dev / 経験者前提**。

charter §4 (3) で確定した **「6-15 人年 / AYA life plan、本職並走で 15-30 年」** とは前提条件が大きく異なる:

| 軸 | a-3 工数見積 | charter §4 (3) | 乖離理由 |
|---|---|---|---|
| 体制 | フルタイム dev 1 人 / 経験者 | AYA 本職並走 1 人 / Vulkan 初見 | 並走係数 3-5x + 学習曲線 |
| 期間 | 7-8 人月 | 6-15 人年 | 並走係数 + 不確実性 (testing / iteration / bug fix) + Doom 2016 / Blender Vulkan 参照点との整合 |
| 不確実性 | 上方 1.5x | 上方 2-3x (charter §8 plan B trigger) | 単独 fork での未踏領域、外部 dependency なし |

a-3 工数見積は **段階順序 + 概算オーダー** の確定のみが目的、絶対値の精緻化は **work item (c) 工程算定** で実施 (charter §4 (3) との整合 + 本職並走前提反映)。

### §5.5 a-3 横断 summary

**4 軸全合計 (a-1 + a-2 + a-3 統合、§6 a-4 で最終確定予定):**

| 分類 | file 数 | LOC | 内訳 |
|---|---|---|---|
| **要 port** | 41+ | ~21.5K | llrender 16 + lldrawpool 13 + llviewershadermgr 2 + a-3 main 4 (llgltexture/lltexture/llrendernavprim/llrendersphere) + llvosky/llvowlsky 2 + 判定保留 → 確定済 interface header 6 |
| **要再設計** | 13 | ~28.3K | llrender 10 (含む wrapper header 3) + critical path 3 (pipeline.cpp + .h + llspatialpartition) + a-3 main 1 (lluiimage) |
| **不要 port** | 4 | ~1.8K | FreeType / font cache / texture manager bridge |
| **Vulkan portage critical path 合計** | — | **~51.6K LOC + shader 248 file** | (header 重複計上は §6 a-4 で整理) |

**特筆事項 5 点 (a-3 で判明):**

1. **header wrapper 局在化の追い風確定**: llglheaders.h + llglstates.h + llgltypes.h の 3 ヶ所書換で 99% の上流 file 対応可能 → abstraction layer 設計が秀逸
2. **AYAstorm 機能の独立性確定**: picker (0.5 人月) + Cinematic (1 人月) + vignette (1.5 人月) = 3 人月で合成可能、base port と並列可能性あり (touchpoint 局在、1,303 LOC 未満)
3. **shader SPIR-V 移行リスク低 確定**: compute / geometry / tessellation / bindless / atomic 全部ゼロ、sampler 206 個は descriptor set 再設計で吸収 → cross compile (glslang) で ~85% 素直に通る見込み
4. **pipeline.cpp が真の critical path 確定**: 15.9K LOC + 3 大グローバル 176 参照、段階 4 が最難関 (frame context 設計 + render stage VkRenderPass chain は 1-2 月の設計議論)
5. **a-2 → a-3 数字精緻化**: 判定保留 17 file の LOC が a-2 概算 8.9K → a-3 確定 5.0K に修正、Vulkan portage critical path 合計も a-2 ~66K → a-3 ~51.6K に修正 (§6 a-4 で最終確定)

**(c) 工程算定への引継ぎ事項:**

- a-3 段階 port 戦略 5 段階 + AYAstorm 3 機能合成順を base structure として採用
- 工数絶対値は a-3 のフルタイム dev 前提を AYA 本職並走前提に変換 (並走係数 3-5x + 学習曲線 + 不確実性 2-3x)
- charter §4 (3) 6-15 人年との整合を (c) で確定

## §6 棚卸し総括 (a-4 完了 = work item (a) 全完了)

a-4 = a-1 / a-2 / a-3 結果を統合し、portage 真の規模 + 段階 port 戦略 + (b) Vulkan API 設計への引継ぎ事項を final 確定する phase。

### §6.1 portage 真の規模 final 確定

**Vulkan portage critical path 合計 LOC (重複計上整理後):**

| 領域 | LOC | 性質 | 出処 |
|---|---|---|---|
| indra/llrender/ | **28,168** | GL API 直叩き層 (判定保留 17 file 含む全 51 file) | a-1 §1.1 |
| indra/newview/pipeline.cpp + .h | **15,954** | 描画 orchestrator + 3 大グローバル 176 参照 | a-1 §1.4 |
| indra/newview/lldrawpool*.cpp | **7,907** | 各 pool 描画 dispatcher (13 file 全て要 port) | a-1 §1.4 + a-2 §2.2 |
| indra/newview/llspatialpartition.cpp | **4,416** | geometry rebuild + occlusion + spatial partition | a-1 §1.4 |
| indra/newview/llviewershadermgr.{cpp,h} | **4,423** | shader manager | a-1 §1.4 + a-2 §2.3 |
| indra/newview/llvosky.cpp + llvowlsky.cpp | **2,198** | sky dome + atmospherics (r14+ visual realism 関連) | a-3 §B.2 |
| **C++ critical path 合計** | **63,066 ≈ 63K LOC** | (header 重複計上排除済) | |
| GLSL shader (SPIR-V 移行対象) | **248 file** (LOC 別途) | class1/2/3 deferred + interface + lighting + windlight + cinematic_bd | a-1 §1.3 |

**memory / charter §4 (2) 想定との差分:**

| 範囲 | charter §4 (2) 想定 | a-4 final 確定 | 差 |
|---|---|---|---|
| indra/llrender/ files | 16 | **51** (header 25 + source 26) | +35 (header 計上分) |
| indra/llrender/ LOC | 28.2K | **28,168** | ほぼ一致 |
| indra/llrender/ GL calls | 464 | **381** | -83 (count 手法差) |
| GL header include files | 212 | **189** | -23 |
| GLSL shader files | 248 | **248** | 一致 |
| pipeline.cpp + .h LOC | (未記載) | **15,954** | 新規確定 |
| lldrawpool*.cpp LOC | (未記載) | **7,907** | 新規確定 (charter 見落とし範囲) |
| llspatialpartition.cpp LOC | (未記載) | **4,416** | 新規確定 |
| llviewershadermgr LOC | (未記載) | **4,423** | 新規確定 (charter 見落とし範囲) |
| llvosky + llvowlsky LOC | (未記載) | **2,198** | 新規確定 (r14+ 関連) |
| **C++ critical path 合計** | **~28.2K** (llrender 単独) | **63K LOC** | **約 2.2 倍に拡大** |

→ charter §4 (2) は llrender 単独で書いていたが、a-4 で真の portage critical path = **63K LOC + shader 248 file** に確定。charter §4 (2) を a-4 完了時に同期更新。

### §6.2 4 軸全合計 final table

| 分類 | file 数 | LOC | 内訳 |
|---|---|---|---|
| **要 port** | 33 | **20,749** | llrender 16 (6,518) + lldrawpool 13 (7,358) + llviewershadermgr 2 (4,423) + llvosky/llvowlsky 2 (2,198) + a-3 main 4 (1,021): llgltexture/lltexture/llrendernavprim/llrendersphere。interface header (llrender.h 等) は対応 cpp と pair で計上済 (重複計上排除) |
| **要再設計** | 14 | **41,121** | llrender 10 (7,937: llgl/llrender/llimagegl/llrendertarget/llpostprocess + wrapper header llglheaders/llglstates/llgltypes + 関連 2) + critical path 3 (pipeline.cpp 14,579 + pipeline.h 1,375 + llspatialpartition 4,416) + a-3 main 1 (lluiimage 321) |
| **不要 port** | 4 | **1,782** | llfontfreetype 1,171 + llfontfreetypesvg 259 + llfontbitmapcache 271 + lltexturemanagerbridge 81 |
| **合計** | **51** | **63,652 LOC** | (LOC 微差は header LOC 概算誤差、shader 248 file は別計上) |

注: 51 file = 棚卸し済 C++ file の合計。llrender 51 + lldrawpool 13 + critical path 5 + llvosky/llvowlsky 2 - 重複 (interface header 対応 cpp と pair で計上) = 51 が正味。

### §6.3 段階 port 戦略 final 形 (a-3 §5.4 を採用)

#### §6.3.1 base LL port (r41 = GL 除去 + Vulkan 空転) — 5 段階

1. **GL header wrapper 置換** (llglheaders.h + llglstates.h + llgltypes.h → Vulkan header + volk loader)
2. **lldrawpool 全 13 file の Vulkan command buffer 化** (terrain.cpp glTexGen → shader 側 explicit UV 計算 含む)
3. **llgl / llrender / llimagegl / llrendertarget / llpostprocess の state machine → PSO 化**
4. **pipeline.cpp 3 大グローバル → frame context 化 + render stage VkRenderPass chain** (最難関)
5. **llspatialpartition / llviewershadermgr / llvertexbuffer 等の依存解決**

#### §6.3.2 AYAstorm 3 機能 patch 合成順 (r42 以降)

- **r42-α**: r21.1 self-rigged picker (mObjectIDBuffer → render pass attachment、shader 2 file SPIR-V 化)
- **r42-β**: r30 Cinematic mode (DoF state enum 化 + frame context 統合、shader 4 file SPIR-V 化)
- **r42-γ**: r14+ visual realism (post-process pass chain 統合、shader 7 file SPIR-V 化、llvosky/llvowlsky port 含む)

#### §6.3.3 段階順序 採用の根拠 3 点

1. **wrapper 局在化** — header 3 file 置換で 188 file の wrapper 経由 file が変更不要 (a-1 §1.2 確定の 99% wrapper 経由)
2. **drawpool modular 性** — 13 file は interface 残置で並列着手可能、base port 序盤の進捗 visibility 確保
3. **pipeline.cpp 最後送り** — 3 大グローバル 176 参照の frame context 化は段階 1-3 完成後の方が refactoring tooling が揃う

### §6.4 (b) Vulkan API 設計への引継ぎ事項

work item (b) 着手時に (a) 結果から引き継ぐべき設計要件:

#### §6.4.1 必須採用の Vulkan feature / extension

- **Vulkan 1.3 default + MoltenVK 1.2 互換性**: Mac 後追い対応のため core 1.2 + 一部 1.3 extension 選択
- **volk loader**: GL header wrapper 置換時の Vulkan loader (段階 1)
- **VMA (Vulkan Memory Allocator)**: llimagegl.cpp staging buffer 化で必須 (段階 3)
- **glslang + spirv-cross**: shader SPIR-V crosscompile (248 file + AYAstorm 改変 13 file)
- **VkRenderPass + VkFramebuffer** (Vulkan 1.3 dynamic rendering 採用検討): llrendertarget / llpostprocess 再設計 (段階 3)
- **VkQueryPool (occlusion query)**: llspatialpartition (段階 5)
- **descriptor set 2-3 個** (per-frame / per-material / per-draw): shader 248 file の sampler 206 個収容

#### §6.4.2 設計検討事項 (b で詰める)

- **frame context 設計** (LLPipelineFrameContext 仮称): 3 大グローバル sCull / sShadowRender / sCurCameraID + DoFMode enum (Cinematic) + AYAstorm picker buffer の集約方式
- **PSO cardinality 事前列挙**: llgl / llrender state machine の state combination 全列挙、PSO cache 戦略
- **terrain.cpp glTexGen 廃止 shader**: shader 側 explicit UV 計算の vert / frag 設計
- **post-process pass chain**: r14+ godrays / volumetricLight / blurLight / vignette + r30 DoF を統合した VkRenderPass chain
- **r21.1 picker attachment 統合方針**: deferred main pass 内 inline attachment vs 別 pass

#### §6.4.3 設計しない / 後送り事項

- **abstraction interface 詳細**: r41.5 milestone (本線 ↔ VK repo 分離) で詰める、(b) では「分離可能な skeleton」のみ
- **Mac MoltenVK 制約詳細**: vk-RC 直前の Mac 追加 phase で詰める、(b) では「Linux 先行 + Mac 互換性 maintain 方針」のみ
- **GPU-driven culling / compute shader 化**: optional acceleration として段階 4 内で個別判断、(b) では「手動 port 可能性のみ確保」

### §6.5 charter / memory 反映候補 list (a-4 完了後の同期更新)

work item (a) 完了 + AYA review 完了後、以下を同期更新:

#### §6.5.1 `00-charter.md` 更新候補

- **§4 (2) scope 数字**: 「indra/llrender 16 files / 28.2K LOC / 464 GL calls + GL header 212 files 全置換」→ 「**C++ critical path ~63K LOC** (llrender 28K + pipeline.cpp 16K + lldrawpool 8K + llspatialpartition 4K + llviewershadermgr 4K + llvosky/llvowlsky 2K) + **GLSL shader 248 file** + GL header 依存 189 file (99% wrapper 経由)」
- **§4 (2) 補足**: wrapper 局在化 (llglheaders / llglstates / llgltypes 3 file 置換で 188 file 対応) を明示、abstraction 設計の追い風として記録
- **§6 r42+ ロードマップ**: a-3 §5.4.2 AYAstorm 3 機能合成順 (r42-α picker → r42-β Cinematic → r42-γ visual realism) を仮 line up から本 line up に格上げ
- **§4 (3) 工数**: a-3 §5.4.3 で確認した「フルタイム dev 7-8 人月 vs charter 6-15 人年 = 並走係数 3-5x + 学習曲線 + 不確実性 2-3x」を補足、(c) 工程算定で精緻化と明示

#### §6.5.2 `project_ayastorm_r40_cpu_parallel.md` 更新候補

- **§sub-phase 3 工程プラン確定事項 (2) scope**: charter §4 (2) と同期、portage 規模数字を a-4 final に置換
- **a-4 で判明した新発見**: lldrawpool 見落としや header wrapper 局在化追い風など、memory 「**達成 = 工程プラン完成**」の中身として記録

#### §6.5.3 反映 timing

- (b) Vulkan API 設計着手前に charter + memory 数字反映を推奨 (b 内で参照される数字を最新化)
- ただし (b)(c) の中で更に数字精緻化が起きる可能性 → 「a-4 final 確定値」と (b)(c) 追加発見を分けて記録、charter は反復更新可能

### §6.6 work item (a) 全完了宣言

a-1 / a-2 / a-3 / a-4 の sub-step 全完了、work item (a) Vulkan portage 棚卸し phase **完了** (2026-05-28)。

03 doc §2 work item 一覧で (a) status を **完了** に更新、次 work item (b) Vulkan API 設計 着手可能。

(b) 着手前の整理事項:
- §6.5 charter / memory 反映 (AYA judgment 仰ぐ)
- 03 doc §2 work item status table 更新 (a 完了 / b 着手)
- 必要に応じて context handoff 検討 (memory `feedback_proactive_handoff.md` 参照)

(a-2 + a-3 完了後の総括、portage 真の規模 + 段階 port 戦略 + (b) Vulkan API 設計への引継ぎ事項)

---

## 関連 doc / memory

- `03-sub-phase-3-vulkan-plan.md` — work item (a) 親 doc
- `00-charter.md` — r40 章 charter
- `01-sub-phase-1-cpu-perf.md` — sub-phase 1 で確定した 3 大グローバル root cause
- `project_ayastorm_r21_self_rigged_picker.md` — r21.1 picker memory
- `project_ayastorm_r30_cinematic_chapter.md` — r30 Cinematic memory
- `project_ayastorm_visual_realism_chapter.md` — r14+ visual realism memory
