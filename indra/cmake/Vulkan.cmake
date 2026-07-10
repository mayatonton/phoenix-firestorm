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

# AYAstorm r41 Vulkan integration module
# - System libvulkan-dev (header) is required at build time.
# - libvulkan-1.so is NOT linked: volk (in indra/llrender/) does dlopen at runtime.

include(Prebuilt)
include(FindVulkan)

if (NOT Vulkan_FOUND)
    find_package(Vulkan REQUIRED)
endif()

add_library( ll::vulkan INTERFACE IMPORTED )

target_include_directories( ll::vulkan SYSTEM INTERFACE ${Vulkan_INCLUDE_DIRS})
