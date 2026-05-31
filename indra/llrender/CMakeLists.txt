# -*- cmake -*-

project(llrender)

include(00-Common)
include(OpenGL)
include(Vulkan)
include(Glslang)
include(AyaShaderCompile)
include(FreeType)
include(LLCommon)
include(LLImage)
include(LLWindow)

set(llrender_SOURCE_FILES
    llcubemap.cpp
    llcubemaparray.cpp
    llfontbitmapcache.cpp
    llfontfreetype.cpp
    llfontfreetypesvg.cpp
    llfontgl.cpp
    llfontvertexbuffer.cpp
    llfontregistry.cpp
    llgl.cpp
    llglslshader.cpp
    llgltexture.cpp
    llimagegl.cpp
    llpostprocess.cpp
    llrender.cpp
    llrender2dutils.cpp
    llrendernavprim.cpp
    llrendersphere.cpp
    llrendertarget.cpp
    llshadermgr.cpp
    lltexture.cpp
    lltexturemanagerbridge.cpp
    lluiimage.cpp
    llvertexbuffer.cpp
    llglcommonfunc.cpp
    llvkloader.cpp
    volk.c
    )
    
set(llrender_HEADER_FILES
    CMakeLists.txt

    llcubemap.h
    llcubemaparray.h
    llfontgl.h
    llfontvertexbuffer.h
    llfontfreetype.h
    llfontfreetypesvg.h
    llfontbitmapcache.h
    llfontregistry.h
    llgl.h
    llglheaders.h
    llglslshader.h
    llglstates.h
    llgltexture.h
    llgltypes.h
    llimagegl.h
    llpostprocess.h
    llrender.h
    llrender2dutils.h
    llrendernavprim.h
    llrendersphere.h
    llshadermgr.h
    lltexture.h
    lltexturemanagerbridge.h
    lluiimage.h
    lluiimage.inl
    llvertexbuffer.h
    llglcommonfunc.h
    llvkloader.h
    volk.h
    )

list(APPEND llrender_SOURCE_FILES ${llrender_HEADER_FILES})

if (BUILD_HEADLESS)
  add_library (llrenderheadless
    ${llrender_SOURCE_FILES}
    )

  set_property(TARGET llrenderheadless
    PROPERTY COMPILE_DEFINITIONS LL_MESA=1 LL_MESA_HEADLESS=1
      )

  target_link_libraries(llrenderheadless
          llcommon
          llimage
          llmath
          llrender
          llxml
          llfilesystem
          )
endif (BUILD_HEADLESS)

add_library (llrender ${llrender_SOURCE_FILES})
target_include_directories( llrender  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

# r41 sub-step 3.4-β-1 (sub-doc 03 §3.1.4 / sub-doc 07 §3.1 sub-step 7.1 内包):
# VMA (Vulkan Memory Allocator) v3.3.0 vendor 配置、header-only / MIT license。
# 利用は llvkloader.cpp 1 translation unit に限定 (#define VMA_IMPLEMENTATION 経由)。
target_include_directories(llrender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

# r41 sub-step 3.3-B-γ (sub-doc 03 §3.1.3):
# glslangValidator が build host に無い場合は SPIR-V file load が runtime で失敗するため、
# llvkloader.cpp の embedded byte array fallback を有効化。Linux build (glslangValidator 検出)
# では未定義 → embedded byte array は dead code として binary から除外。
if (NOT AYA_GLSLANG_VALIDATOR)
    target_compile_definitions(llrender PRIVATE AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK)
endif()

set_property(TARGET llrender PROPERTY POSITION_INDEPENDENT_CODE ON)

# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
target_link_libraries(llrender
        llcommon
        llimage
        llmath
        llfilesystem
        llxml
        llwindow
        ll::freetype
        OpenGL::GL
        OpenGL::GLU
        ll::vulkan
        ll::glslang
        )

