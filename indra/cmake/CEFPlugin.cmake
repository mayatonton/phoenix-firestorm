# -*- cmake -*-
include(Linking)
include(Prebuilt)

include_guard()
add_library( ll::cef INTERFACE IMPORTED )

# <AYAstorm> select dullahan installable: upstream (default) vs t-noami fork w/ audio callback
# Both installables extract to the same include/cef and lib/libdullahan paths, so
# switching variants must uninstall the previously-installed one to avoid an
# autobuild file-conflict error.  Sentinel below also resets so use_prebuilt_binary
# re-runs install for the newly selected variant.
if (LL_DULLAHAN_AUDIO_CALLBACK)
    set(_dullahan_pkg dullahan_aya_audio)
    set(_dullahan_other dullahan)
else ()
    set(_dullahan_pkg dullahan)
    set(_dullahan_other dullahan_aya_audio)
endif ()
execute_process(COMMAND "${AUTOBUILD_EXECUTABLE}" uninstall
                --install-dir=${AUTOBUILD_INSTALL_DIR}
                ${_dullahan_other}
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                OUTPUT_QUIET ERROR_QUIET
                RESULT_VARIABLE _dullahan_uninstall_unused)
file(REMOVE "${PREBUILD_TRACKING_DIR}/${_dullahan_pkg}_installed")
file(REMOVE "${PREBUILD_TRACKING_DIR}/${_dullahan_other}_installed")
use_prebuilt_binary(${_dullahan_pkg})
# </AYAstorm>
target_include_directories( ll::cef SYSTEM INTERFACE  ${LIBS_PREBUILT_DIR}/include/cef)

if (WINDOWS)
    target_link_libraries( ll::cef INTERFACE
        libcef.lib
        libcef_dll_wrapper.lib
        dullahan.lib
    )
elseif (DARWIN)
    FIND_LIBRARY(APPKIT_LIBRARY AppKit)
    if (NOT APPKIT_LIBRARY)
        message(FATAL_ERROR "AppKit not found")
    endif()

    set(CEF_LIBRARY "'${ARCH_PREBUILT_DIRS_RELEASE}/Chromium\ Embedded\ Framework.framework'")
    if (NOT CEF_LIBRARY)
        message(FATAL_ERROR "CEF not found")
    endif()

    target_link_libraries( ll::cef INTERFACE
        ${ARCH_PREBUILT_DIRS_RELEASE}/libcef_dll_wrapper.a
        ${ARCH_PREBUILT_DIRS_RELEASE}/libdullahan.a
        ${APPKIT_LIBRARY}
       )

elseif (LINUX)
    target_link_libraries( ll::cef INTERFACE
        dullahan
        cef
        cef_dll_wrapper.a
    )
endif (WINDOWS)
