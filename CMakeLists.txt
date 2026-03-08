cmake_minimum_required(VERSION 3.20)
project(NESticle LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# SDL2 paths (extracted to external/)
set(SDL2_DIR "${CMAKE_SOURCE_DIR}/external/SDL2-2.30.10")
set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include")
set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x64")

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
)

# New/replacement source files
set(MODERN_SOURCES
    Source/m6502/m6502_shim.cpp
    Source/m6502/fake6502.c
    Source/asm_replacements.cpp
    Source/platform_sdl2.cpp
    Source/gui_impl.cpp
)

add_executable(NESticle
    ${NESTICLE_SOURCES}
    ${MODERN_SOURCES}
)

target_include_directories(NESticle PRIVATE
    Source
    Source/m6502
    ${SDL2_INCLUDE_DIR}
)

target_compile_definitions(NESticle PRIVATE SDL2)

target_link_libraries(NESticle PRIVATE
    ${SDL2_LIB_DIR}/SDL2.lib
    ${SDL2_LIB_DIR}/SDL2main.lib
)

# On Windows, link with WinMM for timeGetTime compatibility stubs
if(WIN32)
    target_link_libraries(NESticle PRIVATE winmm)
endif()

# Copy SDL2.dll to output directory
add_custom_command(TARGET NESticle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_LIB_DIR}/SDL2.dll"
        $<TARGET_FILE_DIR:NESticle>
)
