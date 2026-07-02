# -*- cmake -*-

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
