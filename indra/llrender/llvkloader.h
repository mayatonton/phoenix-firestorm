/**
* @file llvkloader.h
* @brief AYAstorm r41 Vulkan loader + instance lifecycle (volk-based)
*
* $LicenseInfo:firstyear=2026&license=viewerlgpl$
* AYAstorm Viewer Source Code
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* $/LicenseInfo$
*/

#ifndef LL_LLVKLOADER_H
#define LL_LLVKLOADER_H

#include "volk.h"

#include <vector>

// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6δ:
// 5 cadence flush 関数 (flushProgramUbos / flushAssetUbos / flushSkinUbos) 用 forward decl。
// LLGLSLShader と LL::GLTF::{Asset, Skin} 実体は llrender / newview の重い header に
// 含まれるため、本 header では opaque pointer 受けに留め、include 連鎖を回避する。
class LLGLSLShader;
namespace LL
{
namespace GLTF
{
    class Asset;
    class Skin;
}
}

namespace LLVKLoader
{
    bool initVulkan();
    void shutdownVulkan();

    bool isVulkanInitialized();
    bool isValidationEnabled();

    bool beginFrame();
    bool endFrame();
    VkCommandBuffer getCurrentCommandBuffer();

    // r41 sub-step 3.1b PSO foundation (sub-doc 03 §1.5 / §3.1 sub-step 3.1)
    VkDevice         getDevice();
    VkPipelineCache  getPipelineCache();

    // Standard pipeline layout helper. Caller owns returned handle (destroy with vkDestroyPipelineLayout).
    // Pass nullptr / 0 for descriptor sets or push constants to omit.
    VkPipelineLayout createStandardPipelineLayout(
        const VkDescriptorSetLayout* descriptor_set_layouts,
        U32                          descriptor_set_layout_count,
        const VkPushConstantRange*   push_constant_ranges,
        U32                          push_constant_range_count);

    // Compile a graphics pipeline using the persistent VkPipelineCache.
    // ci.layout / ci.renderPass / ci.pStages etc. must be filled by caller.
    bool compileGraphicsPipeline(const VkGraphicsPipelineCreateInfo& ci, VkPipeline& out_pipeline);

    // r41 sub-step 3.4-δ-1 (sub-doc 03 §3.1.4): 12 pool 共用 placeholder draw helper
    // (旧名 recordSkySmokeDraw、3.2 sky-smoke 由来を 12 pool 共用へ unification)。
    // PSO bind (sSkySmokePipeline) + set=0 PerFrame descriptor + set=1 PerMaterial descriptor +
    // push constant 64 B identity modelview / VERTEX_BIT + vkCmdDraw(3, 1, 0, 0)。
    // 各 pool の recordPoolDraws hook body から呼出、in-frame 前提 (caller 側 guard)。
    void recordPlaceholderPoolDraw(VkCommandBuffer cmd_buf);

    // r41 sub-step 3.4-δ-4 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.4 部分内包):
    // avatar pool 専用 placeholder draw helper (bone matrix SSBO 並走基本配線)。
    // PSO bind (sAvatarBonePipeline、shader は sSkySmoke 共用 = 段階 3 placeholder 用途) +
    // set=0 PerFrame descriptor + set=1 PerMaterial descriptor +
    // set=2 AvatarBone push descriptor (vkCmdPushDescriptorSetKHR、STORAGE_BUFFER、
    // 110 mat4 identity HOST_VISIBLE+MAPPED) +
    // push constant 64 B identity modelview / VERTEX_BIT + vkCmdDraw(3, 1, 0, 0)。
    // VK_KHR_push_descriptor 未対応 / Vulkan 未初期化 / PSO 未作成時は
    // recordPlaceholderPoolDraw() へ graceful fallback (一般 12 pool 共用挙動)。
    // lldrawpoolavatar.cpp::recordPoolDraws() hook body から呼出、in-frame 前提。
    void recordAvatarPlaceholderDraw(VkCommandBuffer cmd_buf);

    // ------------------------------------------------------------------
    // r41 sub-step 3.3-β-1: per-frame matrix UBO layout (二段構え)
    // sub-doc 03 §3.1.1 / sub-doc 05 §3.5 (AYA 確定 2026-05-29)
    //
    // 二段構え:
    //   push constant : modelview_matrix (mat4 = 64 B、VERTEX_BIT、GL 流儀継承)
    //   UBO binding 0 : PerFrameMatrixUBO (3 mat4 = 192 B)
    //   UBO binding 1 : TextureMatrixUBO  (4 mat4 = 256 B)
    //
    // MVP / normal_matrix / inverse_modelview は vertex shader 内で
    // `projection_matrix × modelview_matrix` 等から算出 (3.3-B 範疇)。
    // ------------------------------------------------------------------

    // binding 0 (std140): projection 系 3 mat4
    struct PerFrameMatrixUBO
    {
        float projection_matrix[16];
        float inverse_projection_matrix[16];
        float identity_matrix[16];
    };
    static_assert(sizeof(PerFrameMatrixUBO) == 192,
                  "PerFrameMatrixUBO size mismatch (std140 expects 192 B)");

    // binding 1 (std140): MM_TEXTURE0..3 (texture × 4)
    struct TextureMatrixUBO
    {
        float texture_matrix[4][16];
    };
    static_assert(sizeof(TextureMatrixUBO) == 256,
                  "TextureMatrixUBO size mismatch (std140 expects 256 B)");

    // set=0 descriptor set layout (binding 0 = PerFrameMatrixUBO,
    // binding 1 = TextureMatrixUBO, stage = VERTEX | FRAGMENT).
    // Owned by LLVKLoader; do not destroy. Returns VK_NULL_HANDLE before
    // sub-step 3.3-β-2 wires the layout up.
    VkDescriptorSetLayout getPerFrameDescriptorSetLayout();

    // r41 sub-step 3.3-δ-1: frame in flight index counter (sub-doc 03 §3.1.1 δ)。
    // beginFrame() 呼出毎に (sFrameIndex + 1) % FRAMES_IN_FLIGHT で進む。δ-2 の
    // vkCmdPushConstants + vkCmdBindDescriptorSets でも同 index を共有する。
    U32 getCurrentFrameIndex();

    // r41 sub-step 3.3-δ-1: per-frame matrix UBO write helper (sub-doc 03 §3.1.1 δ)。
    // sPerFrameUboMapped[getCurrentFrameIndex()] の対応 offset へ memcpy。
    // Vulkan 未初期化 / 未 map 時は no-op (GL path 単独動作環境で safe)。
    void writeCurrentPerFrameMatrixUBO(const PerFrameMatrixUBO& data);
    void writeCurrentTextureMatrixUBO(const TextureMatrixUBO& data);

    // r41 sub-step 3.3-δ-2: modelview push constant helper (sub-doc 03 §3.1.1 δ)。
    // in-frame (beginFrame...endFrame 間) のみ vkCmdPushConstants 投入、それ以外 no-op。
    // 現状 sPlaceholderLayout 使用 (beginFrame で sPlaceholderPipeline bind 済、layout は γ で
    // 二段構え準拠化 = push constant range 0..64 B / VERTEX_BIT)。実 draw call 経路への
    // PSO bind / descriptor set bind / push 投入統合は 3.3 後続 sub-step。
    void pushCurrentModelviewMatrix(const float modelview_matrix[16]);

    // ------------------------------------------------------------------
    // r41 sub-step 3.3-C-β-1: dynamic rendering attachment + begin/end helper
    // sub-doc 03 §3.1.2 (AYA 確定 2026-05-29、案 A 承認)
    //
    // LLRenderTarget::bindTarget() / flush() の Vulkan path 並走で使用。
    // 3.3-C は API surface 並走化のみで、VkImage / VkImageView 実体作成 +
    // 実 attachment 提供は領域 7 sub-step 7.5 移管 (sub-doc 07 §1.2.3 + §3.1
    // sub-step 7.5)。3.3-C-β-2 以降の transit smoke では placeholder image view
    // (nullable) を渡す。実 attachment 提供は領域 7 sub-step 7.5 で本配線。
    // ------------------------------------------------------------------

    // color × ≤4 + depth × 1 の attachment 1 件記述 (VkRenderingAttachmentInfoKHR の wrap)。
    // β-2 transit smoke では image_view = VK_NULL_HANDLE 可 (helper 側で skip 判定)。
    struct DynamicRenderingAttachment
    {
        VkImageView         image_view;     // β-2 transit smoke は VK_NULL_HANDLE 可、sub-step 7.5 で実 view 提供
        VkImageLayout       image_layout;   // 通例 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL / VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
        VkAttachmentLoadOp  load_op;        // VK_ATTACHMENT_LOAD_OP_CLEAR / LOAD / DONT_CARE
        VkAttachmentStoreOp store_op;       // VK_ATTACHMENT_STORE_OP_STORE / DONT_CARE
        VkClearValue        clear_value;    // load_op == CLEAR の時のみ参照
    };

    // dynamic rendering 開始 / 終了 helper (δ-2 case P gating pattern 継承)。
    // in-frame (beginFrame...endFrame 間) かつ Vulkan 初期化済 + sCommandBuffer 有効時のみ
    // vkCmdBeginRenderingKHR / vkCmdEndRenderingKHR を発行、それ以外 no-op (GL path
    // 単独動作環境で safe)。color_attachments == nullptr / color_count == 0 + depth_attachment == nullptr
    // の場合は何も発行しない (β-2 transit smoke 時の placeholder 受入)。
    // color_attachments[].image_view または depth_attachment->image_view が VK_NULL_HANDLE の場合も
    // helper 側で個別 skip (β-2 transit smoke 時の placeholder 受入)。
    void beginDynamicRendering(U32                               width,
                               U32                               height,
                               const DynamicRenderingAttachment* color_attachments,
                               U32                               color_count,
                               const DynamicRenderingAttachment* depth_attachment);
    void endDynamicRendering();

    // ------------------------------------------------------------------
    // r41 sub-step 3.3-B-β-1: SPIR-V shader module load helper
    // sub-doc 03 §3.1.3 (AYA 確定 2026-05-30、1 shader exemplar pre-flight)
    //
    // 既存 GL compile chain (llshadermgr.cpp:909-955 = glCreateShader →
    // glShaderSource → glCompileShader) に対する SPIR-V 並走分岐の最小 entry point。
    // β-2 で実装 body 配信 (build 時 pre-compile した .spv binary を
    // vkCreateShaderModule() 経由で VkShaderModule 化、領域 6 sub-step 6.1 一括化
    // までの 1 shader exemplar pre-flight 配置)。
    //
    // 戻り値: 成功時 VkShaderModule、失敗時 VK_NULL_HANDLE
    //         (Vulkan 未初期化 / spv_code == nullptr / code_size_bytes == 0 /
    //          code_size_bytes % 4 != 0 を含む)。
    // 所有: caller (vkDestroyShaderModule で破棄)。PSO compile 後は安全に破棄可
    //       (VkPipeline は内部で SPIR-V を取り込む)。
    //
    // spv_code        : SPIR-V binary (SPIR-V spec で U32 alignment 保証)。
    // code_size_bytes : spv_code が指すデータの byte 数 (4 の倍数必須)。
    // ------------------------------------------------------------------
    VkShaderModule loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes);

    // ------------------------------------------------------------------
    // r41 sub-step 4.3-γ'-port-α-5 (sub-doc 06 §3.1 case ② runtime path):
    // production SPIR-V sink。std::vector<unsigned int> blob を VkShaderModule 化。
    // LLShaderMgr Vulkan path (llshadermgr.cpp:908 直前 hook) + SPIR-V cache layer
    // 案 C (~/.ayastorm_x64/cache/shader_cache/<mShaderHash>_{vert,frag}.spv) の
    // memory 載った blob 共通 sink。loadSpirvShaderModule (primitive 配列+size sink)
    // を内部委譲。
    //
    // 戻り値: VK_NULL_HANDLE = 失敗 (spirv.empty() / loadSpirvShaderModule 失敗)。
    // 所有: caller (vkDestroyShaderModule で破棄)。
    //
    // 3.3-B exemplar (試作レール) の loadSpirvShaderModuleFromFile() とは sink 分離、
    // sub-doc 03 §3.1.3 役割再定義注記に従い両者並存。
    // ------------------------------------------------------------------
    VkShaderModule loadSpirvShaderModuleFromMemory(const std::vector<unsigned int>& spirv);

    // ------------------------------------------------------------------
    // r41 sub-step 3.4-β-2: LLImageGL → VkImage + VkImageView lifecycle 並走化
    // sub-doc 03 §3.1.4 (AYA 確定 2026-05-31、案 1 (image lifecycle 先行) 採用)
    //
    // β-2 scope:
    //   - format conversion table (LL GL internalformat → VkFormat) 公開
    //   - placeholder 1×1 white texture transit smoke を llvkloader 内部で
    //     vmaCreateImage + vkCreateImageView + (β-2-3) staging upload +
    //     2 段 layout transition + 破棄まで一連動作実証
    //   - LLImageGL 側は generateTextures に Vulkan path mirror marker のみ
    //     (per-LLImageGL VkImage 配線は領域 7 sub-step 7.5 持越し)
    //
    // 設計境界:
    //   - VmaAllocation は llvkloader.cpp 1 TU 限定 (β-1 設計継承、vk_mem_alloc.h
    //     を header へ持込まない)、本 sub-step で公開するのは VkFormat 変換 API のみ
    //   - VkImageResource 集約 struct は file-local 留置 (placeholder smoke 専用)、
    //     per-LLImageGL 抱合せ型の header 露出は 7.5 で抱合せ要件確定後に検討
    // ------------------------------------------------------------------

    // LLImageGL の mFormatInternal (GL internalformat、e.g. GL_RGBA8 / GL_DEPTH24_STENCIL8 /
    // GL_RGBA16F 等) を入力に VkFormat へ集約変換。llvkloader は GL header から独立する
    // ため、OpenGL spec 確定値を U32 hex literal で受信 (LLImageGL 側は llgltypes.h
    // 経由で LLGLenum = U32 として運用、本 API は LLGLenum 互換 U32 受け)。
    //
    // 戻り値: 対応する VkFormat、未対応 enum / 0 入力時は VK_FORMAT_UNDEFINED
    //         (caller 側で fallback / assert 判断)。
    // 対応 enum: 20 entry (8/16/32-bit normalized + float + depth/stencil + sRGB +
    //            packed HDR、sub-doc 07 §1.2.1 set=1 想定 7 PBR slot + LLImageGL
    //            主要 internalformat 網羅)。
    VkFormat llGlEnumToVkFormat(U32 ll_gl_intformat);

    // ------------------------------------------------------------------
    // r41 sub-step 4.3-β': LLVertexBuffer Vulkan 化 (charter §7.5 boundary refine)
    // sub-doc 04 §3.4 (AYA 確定 2026-05-31、案 D hybrid 採用)
    //
    // β' scope (placement のみ):
    //   - VkBuffer + VmaAllocation の lifecycle helper (create/destroy)
    //   - HOST_VISIBLE + MAPPED 想定 (GL VBO/IBO mapped region 経路 parallel)
    //   - bind helper 配置 (β'-3、caller fire は 4.3-ε' 範囲)
    //
    // 設計境界:
    //   - VmaAllocation handle は void* opaque で公開 (vk_mem_alloc.h header 持込み回避、
    //     既存 llvkloader.h:192 / sub-step 3.4-β-2 設計継承)。impl 側で
    //     reinterpret_cast<VmaAllocation> 経由で取扱い。
    //   - out_mapped は HOST_VISIBLE+MAPPED 確保時のみ非 nullptr、device-local 経路
    //     (将来拡張) では nullptr。
    //   - 失敗時 caller-owned out_buffer / out_allocation は VK_NULL_HANDLE / nullptr のまま
    //     (caller 側追加破棄不要)。
    // ------------------------------------------------------------------

    // VERTEX_BUFFER_BIT + HOST_VISIBLE + MAPPED で確保。size_bytes は LLVertexBuffer::mSize
    // 想定 (>0、0 入力は失敗)。Vulkan 未初期化 / VMA 未確保時も false を返し caller-owned
    // ハンドル群は VK_NULL_HANDLE のまま (caller 側 GL fallback 想定)。
    bool createVertexBufferVk(U32     size_bytes,
                              VkBuffer& out_buffer,
                              void*&    out_allocation,
                              void**    out_mapped);

    // INDEX_BUFFER_BIT + HOST_VISIBLE + MAPPED で確保。size_bytes は LLVertexBuffer::mIndicesSize 想定。
    bool createIndexBufferVk (U32     size_bytes,
                              VkBuffer& out_buffer,
                              void*&    out_allocation,
                              void**    out_mapped);

    // create*BufferVk で取得した buffer / allocation を破棄。VK_NULL_HANDLE / nullptr は no-op。
    // Vulkan 未初期化時も no-op (caller 側未確保を前提に対称呼出)。
    void destroyBufferVk     (VkBuffer  buffer,
                              void*     allocation);

    // ------------------------------------------------------------------
    // r41 sub-step 4.3-β'-3: vkCmdBindVertexBuffers / vkCmdBindIndexBuffer 配置 (1:1 wrap)。
    // β' 段階 = caller 配置のみ (実 fire は 4.3-ε' per-pool draw 配線時)。
    // cmd_buf == VK_NULL_HANDLE / buffer == VK_NULL_HANDLE 時は no-op (caller 側 in-frame /
    // GL fallback 経路と対称運用)。binding 番号は 0 固定 (LLVertexBuffer interleaved 1 binding
    // pattern、4.3-ε' で per-pool layout 確定時に多段化判断)。
    // ------------------------------------------------------------------
    void bindVertexBufferVk(VkCommandBuffer cmd_buf,
                            VkBuffer        buffer,
                            VkDeviceSize    offset);

    void bindIndexBufferVk (VkCommandBuffer cmd_buf,
                            VkBuffer        buffer,
                            VkDeviceSize    offset,
                            VkIndexType     index_type);

    // ------------------------------------------------------------------
    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6δ:
    // 5 cadence (per-frame / per-program / per-draw / per-asset / per-skin)
    // flush 関数。本 PC-6δ scope は LLUboRingBuffer allocate() / beginFrame()
    // 経路を通電させ、各 cadence の駆動位置から空 dummy 書込 (256 B / memset 0) を
    // 発射して ring buffer wrap + grow + side-table mapped ポインタ参照を実証する。
    //
    // 設計根拠 (canonical naming = "per-program"):
    //   docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md
    //     §2.2 5 cadence 分類 / §4.1 flush 駆動関数 名前 / §4.3 駆動位置 /
    //     §5.3 mUseUBO runtime gate と dirty propagation
    //
    // 駆動位置 (P1 採用 2026-06-04 = design 06b canonical):
    //   flushFrameUbos    : LLPipeline::renderGeomDeferred() 入口
    //                       (= design 06b §4.3 renderGeom() 系の主経路、AYA Q3a)
    //   flushProgramUbos  : LLGLSLShader::bind() 入口
    //   flushDrawUbos     : 主要 pool render entry の canary 配線
    //                       (= AYA Q3b、PC-6ε で残 pool 全配線)
    //   flushAssetUbos    : gltfscenemanager.cpp 内 nodes/materials UBO bind 直前
    //   flushSkinUbos     : gltfscenemanager.cpp 内 joints UBO bind 直前
    //
    // MUSEUBO-A 整合: 本 PC-6δ は OpenGL 描画 path に対して常に no-op。
    //   - sDrawUboRingBufferMgr 未初期化 (= GL 単独動作 / Vulkan 未起動) 時は即時 return
    //   - mUseUBO runtime gate は redirect 層 (PC-7+) で参照、本 PC-6δ flush は
    //     ring buffer 上に空 256 B を流すだけで描画 state を一切変更しない
    //
    // 引数 (LLGLSLShader / Asset / Skin) は将来 PC-6ε で per-program / per-asset /
    // per-skin dirty map lookup の key として使用、本 PC-6δ では受信のみ
    // (空書込で hash / id 参照しない = (unused) ガード)。
    // ------------------------------------------------------------------
    void flushFrameUbos();
    void flushProgramUbos(LLGLSLShader* shader);
    void flushDrawUbos();
    void flushAssetUbos(LL::GLTF::Asset* asset);
    void flushSkinUbos(LL::GLTF::Skin* skin);

    // ------------------------------------------------------------------
    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6ε-1:
    // singleton cadence (= 第 6 cadence) flush 関数。5 cadence (per-frame /
    // per-program / per-draw / per-asset / per-skin) + singleton = 6 cadence
    // 体系成立。
    //
    // 設計根拠 (= design 02 §3 + design 06c §2.2):
    //   - design 02 §3 で `Global_` prefix = singleton cadence と明示分類
    //     (= 5 cadence prefix `Frame_` / `Program_` / `Draw_` / `Asset_` / `Skin_`
    //     + singleton prefix `Global_`)。
    //   - design 06c §2.2 で `Global_ReflectionProbes` (= PC-1 で codegen 確定済
    //     test UBO) = singleton 配置の代表例。
    //   - design 06a §3.3 `CadenceTag` enum 値域に singleton 含む
    //     (= cadence_tag=5 = SINGLETON、codegen main.py:63 CADENCE_SINGLETON=5)。
    //
    // 駆動位置 (PC-6ε-1):
    //   bringupTestUBO() (llglslshader.cpp:2032) で PC-1 contract assertion 後に
    //   呼出 (= PC-2 forwardToUboUpload 経路 → PC-6ε-1 で本格 cadence 経路置換)。
    //
    // MUSEUBO-A 整合: 本関数も sDrawUboRingBufferMgr 未初期化時 = no-op
    // (= flushDummyUboWrite helper entry guard で 5 cadence と同形保証)。
    // GATE-B 整合: mUseUBO runtime gate 未依存 (= PC-6α..δ 同形)。
    // ------------------------------------------------------------------
    void flushSingletonUbos();
}

#endif // LL_LLVKLOADER_H
