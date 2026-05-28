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

    // r41 sub-step 3.2 smoke-test: sky pool 用 minimal PSO bind + vkCmdDraw 投入
    // (sub-doc 03 §3.1 sub-step 3.2、2026-05-29 refine、llpostprocess は r42-δ 移管)
    void recordSkySmokeDraw(VkCommandBuffer cmd_buf);

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
}

#endif // LL_LLVKLOADER_H
