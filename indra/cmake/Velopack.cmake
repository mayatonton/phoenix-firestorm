# -*- cmake -*-
# Velopack installer and update framework integration
# https://velopack.io/

include_guard()

# TODO: Add Mac and Linux support
# Only available on Windows (so far)
if (WINDOWS)
    include(Prebuilt)
    use_prebuilt_binary(velopack)

    add_library(ll::velopack INTERFACE IMPORTED)

    target_include_directories(ll::velopack SYSTEM INTERFACE
        ${LIBS_PREBUILT_DIR}/include/velopack
    )

    target_link_libraries(ll::velopack INTERFACE
        ${ARCH_PREBUILT_DIRS_RELEASE}/velopack_libc.lib
    )

    # Windows system libraries required by Velopack
    target_link_libraries(ll::velopack INTERFACE
        winhttp
        ole32
        shell32
        shlwapi
        version
        userenv
        ws2_32
        bcrypt
        ntdll
    )

    target_compile_definitions(ll::velopack INTERFACE LL_VELOPACK=1)
endif()
