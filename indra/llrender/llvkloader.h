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

namespace LLVKLoader
{
    bool initVulkan();
    void shutdownVulkan();

    bool isVulkanInitialized();
    bool isValidationEnabled();
}

#endif // LL_LLVKLOADER_H
