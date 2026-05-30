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
}

#endif // LL_LLVKLOADER_H
