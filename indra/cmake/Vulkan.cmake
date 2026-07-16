# -*- cmake -*-
#
# Copyright (C) 2025-2026 Ishikawa AYA (github: mayatonton, mayatonton1994@gmail.com)
#
# このファイルは Ishikawa AYA が新規に作成した独自著作物である。
# 著作権は Ishikawa AYA が保持し、パブリックドメインには置かない。
# All rights reserved by the author except as licensed below.
#
# Licensed under the GNU Lesser General Public License, version 2.1
# (same license as the Firestorm Viewer). See the LICENSE file for details.
#
# 光の国のひとたちと共にわたしはここにいる　彩
#

# AYAstorm Vulkan integration module
# - macOS uses the pinned vulkan_sdk_macos prebuilt for headers and the Loader.
# - Windows/Linux keep their existing system Vulkan SDK discovery.
# - The Loader found here is not linked: volk (in indra/llrender/) loads it at runtime.

include(Prebuilt)

if (DARWIN)
    use_prebuilt_binary(vulkan_sdk_macos)

    set(VulkanSDKMacOS_INCLUDE_DIR "${LIBS_PREBUILT_DIR}/include")
    set(VulkanSDKMacOS_LOADER_LIBRARY
        "${LIBS_PREBUILT_DIR}/lib/release/libvulkan.dylib")

    if (NOT EXISTS "${VulkanSDKMacOS_INCLUDE_DIR}/vulkan/vulkan.h" OR
        NOT EXISTS "${VulkanSDKMacOS_LOADER_LIBRARY}")
        message(FATAL_ERROR
            "vulkan_sdk_macos must provide include/vulkan/vulkan.h and "
            "lib/release/libvulkan.dylib")
    endif()

    set(Vulkan_INCLUDE_DIR "${VulkanSDKMacOS_INCLUDE_DIR}" CACHE PATH
        "Vulkan headers from vulkan_sdk_macos" FORCE)
    set(Vulkan_LIBRARY "${VulkanSDKMacOS_LOADER_LIBRARY}" CACHE FILEPATH
        "Vulkan Loader from vulkan_sdk_macos" FORCE)
endif()

include(FindVulkan)

if (NOT Vulkan_FOUND)
    find_package(Vulkan REQUIRED)
endif()

add_library( ll::vulkan INTERFACE IMPORTED )

target_include_directories( ll::vulkan SYSTEM INTERFACE ${Vulkan_INCLUDE_DIRS})
