# -*- cmake -*-

project(llaudio)

include(00-Common)
include(LLAudio)
include(FMODSTUDIO)
include(OPENAL)
include(CURL)
include(OpenSSL)
include(NGHTTP2)
include(LLCommon)
include(LLWindow)

set(llaudio_SOURCE_FILES
    llaudioengine.cpp
    lllistener.cpp
    llaudiodecodemgr.cpp
    llvorbisencode.cpp
    )

set(llaudio_HEADER_FILES
    CMakeLists.txt

    llaudioengine.h
    lllistener.h
    llaudiodecodemgr.h
    llvorbisencode.h
    llwindgen.h
    )

if (TARGET ll::fmodstudio)
    list(APPEND llaudio_SOURCE_FILES
         llaudioengine_fmodstudio.cpp
         lllistener_fmodstudio.cpp
         llstreamingaudio_fmodstudio.cpp
         llpositionalstream.cpp
         llpositionalstreamstereo.cpp
         llpositionalstreammulti.cpp
         llmultichanneldownmix.cpp
         llstereoupmix.cpp
         lllitehrtfdsp.cpp
         llfftconvolver.cpp
         llirloader.cpp
         llvenuereverbdsp.cpp
         llstream3durlresolve.cpp
         fmod_codec_ogg.cpp
         )

    list(APPEND llaudio_HEADER_FILES
         llaudioengine_fmodstudio.h
         lllistener_fmodstudio.h
         llstreamingaudio_fmodstudio.h
         llpositionalstream.h
         llpositionalstreamstereo.h
         llpositionalstreammulti.h
         llmultichanneldownmix.h
         llstereoupmix.h
         lllitehrtfdsp.h
         llfftconvolver.h
         llirloader.h
         llvenuereverbdsp.h
         llstream3durlresolve.h
         fmod_codec_ogg.h
         )

    # <AYAstorm> Dullahan audio callback path requires the t-noami/dullahan fork
    if (LL_DULLAHAN_AUDIO_CALLBACK)
        list(APPEND llaudio_SOURCE_FILES llmediaaudiostream.cpp)
        list(APPEND llaudio_HEADER_FILES llmediaaudiostream.h)
    endif ()
    # </AYAstorm>
endif ()

if (TARGET ll::openal)
  list(APPEND llaudio_SOURCE_FILES
    llaudioengine_openal.cpp
    lllistener_openal.cpp
    )
  
  list(APPEND llaudio_HEADER_FILES
    llaudioengine_openal.h
    lllistener_openal.h
    )
endif ()

list(APPEND llaudio_SOURCE_FILES ${llaudio_HEADER_FILES})

add_library (llaudio ${llaudio_SOURCE_FILES})
target_include_directories( llaudio  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries( llaudio
        llcommon
        llmath
        llmessage
        llfilesystem
        ll::vorbis
    )

if( TARGET ll::openal )
    target_link_libraries( llaudio ll::openal )
endif()
if( TARGET ll::fmodstudio )
    target_link_libraries( llaudio ll::fmodstudio )
endif()
if( TARGET ll::libcurl )
    # r11 P10: viewer-side URL pre-resolve (llstream3durlresolve.cpp).
    # libcurl pulls in openssl + nghttp2 transitively; matching how
    # llcorehttp wires them so the imported targets resolve in this
    # cmake scope (otherwise newview's link line gets `-lll::openssl`
    # / `-lll::nghttp2` as bare strings and the linker fails).
    target_link_libraries( llaudio ll::libcurl ll::openssl ll::nghttp2 )
endif()
