cmake_minimum_required(VERSION 3.20)
project(NESticle LANGUAGES C CXX)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# SDL3 source is vendored under external/ and built as a CMake subproject.
set(SDL3_DIR "${CMAKE_SOURCE_DIR}/external/SDL3-3.4.10")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(SDL_SHARED ON CACHE BOOL "Build SDL3 shared library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Build SDL3 static library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Build SDL3 tests" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL3 examples" FORCE)
add_subdirectory("${SDL3_DIR}" EXCLUDE_FROM_ALL)

# Define WIN95 globally (NESticle uses this for platform-conditional code)
add_definitions(-DWIN95)

# MSVC-specific settings for legacy 90s C++ code
if(MSVC)
    add_compile_options(
        /Zc:forScope-     # Allow old-style for-loop variable scoping
        /wd4996            # Suppress deprecation warnings
        /wd4244 /wd4018 /wd4267  # Suppress conversion warnings
        /wd4305            # Suppress truncation warnings
        /permissive        # Allow non-conforming constructs
    )
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Source files
set(NESTICLE_SOURCES
    Source/MAIN.CPP
    Source/NES.CPP
    Source/CPU.CPP
    Source/ROM.CPP
    Source/MMC.CPP
    Source/NESVIDEO.CPP
    Source/NESSOUND.CPP
    Source/NESDLG.CPP
    Source/STDDLG.CPP
    Source/R2IMG.CPP
    Source/INPUT.CPP
    Source/INPUTW.CPP
    Source/MOUSE.CPP
    Source/FONT.CPP
    Source/FILE.CPP
    Source/VOL.CPP
    Source/MESSAGE.CPP
    Source/COMMAND.CPP
    Source/CONFIG.CPP
    Source/BUILD.CPP
    Source/SLIST.CPP
    Source/PROF.CPP
    Source/SNAPSHOT.CPP
    Source/DISASM.CPP
    Source/save_state_format.cpp
)

# New/replacement source files
set(MODERN_SOURCES
    Source/m6502/m6502_shim.cpp
    Source/m6502/fake6502.c
    Source/asm_replacements.cpp
    Source/platform_sdl3.cpp
    Source/gui_impl.cpp
)

add_executable(NESticle
    ${NESTICLE_SOURCES}
    ${MODERN_SOURCES}
)

target_include_directories(NESticle PRIVATE
    Source
    Source/m6502
)

target_compile_definitions(NESticle PRIVATE SDL3)

target_link_libraries(NESticle PRIVATE SDL3::SDL3)

add_executable(save_state_format_tests
    Source/save_state_format.cpp
    tests/save_state_format_tests.cpp
)

target_include_directories(save_state_format_tests PRIVATE
    Source
)

add_test(NAME save_state_format_tests COMMAND save_state_format_tests)

# On Windows, link with WinMM for timeGetTime compatibility stubs
if(WIN32)
    target_link_libraries(NESticle PRIVATE winmm)
endif()

# Copy SDL3.dll to output directory on Windows.
if(WIN32 AND TARGET SDL3::SDL3-shared)
    add_custom_command(TARGET NESticle POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3-shared>
            $<TARGET_FILE_DIR:NESticle>
        VERBATIM
    )
endif()

set(NESTICLE_PACKAGE_DIR "${CMAKE_SOURCE_DIR}/releases/NESticle-sdl3-$<CONFIG>")
set(NESTICLE_PACKAGE_ZIP "${CMAKE_SOURCE_DIR}/releases/NESticle-sdl3-$<CONFIG>.zip")

add_custom_target(package_release
    COMMAND "${CMAKE_COMMAND}"
        -D "NESTICLE_EXE=$<TARGET_FILE:NESticle>"
        -D "SDL3_DLL=$<TARGET_FILE:SDL3::SDL3-shared>"
        -D "SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        -D "PACKAGE_DIR=${NESTICLE_PACKAGE_DIR}"
        -D "PACKAGE_ZIP=${NESTICLE_PACKAGE_ZIP}"
        -P "${CMAKE_SOURCE_DIR}/cmake/package_release.cmake"
    DEPENDS NESticle
    COMMENT "Creating deterministic NESticle SDL3 release package"
    VERBATIM
)
