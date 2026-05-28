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
}

#endif // LL_LLVKLOADER_H
