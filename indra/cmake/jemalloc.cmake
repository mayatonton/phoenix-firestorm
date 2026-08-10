# -*- cmake -*-
include(Prebuilt)

if (LINUX AND NOT SYSTEMLIBS )
  set(USE_JEMALLOC ON)
endif ()

if (USE_ASAN OR USE_TSAN)
  set(USE_JEMALLOC OFF)
endif ()

if( USE_JEMALLOC )
  if (USESYSTEMLIBS)
    message( WARNING "Not implemented" )
  else (USESYSTEMLIBS)
    use_prebuilt_binary(jemalloc)
  endif (USESYSTEMLIBS)
endif()
