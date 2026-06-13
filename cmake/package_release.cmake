if(NOT DEFINED NESTICLE_EXE OR NOT EXISTS "${NESTICLE_EXE}")
  message(FATAL_ERROR "NESticle executable not found: ${NESTICLE_EXE}")
endif()

if(NOT DEFINED SDL3_DLL OR NOT EXISTS "${SDL3_DLL}")
  message(FATAL_ERROR "SDL3 runtime not found: ${SDL3_DLL}")
endif()

set(required_source_files
  "${SOURCE_DIR}/gui.vol"
  "${SOURCE_DIR}/NESticle.pal"
  "${SOURCE_DIR}/README.md"
  "${SOURCE_DIR}/LICENSE"
  "${SOURCE_DIR}/external/SDL3-3.4.10/LICENSE.txt"
  "${SOURCE_DIR}/external/SDL3-3.4.10-PROVENANCE.md"
  "${SOURCE_DIR}/Source/m6502/fake6502.c"
)

foreach(required_file IN LISTS required_source_files)
  if(NOT EXISTS "${required_file}")
    message(FATAL_ERROR "Required package input is missing: ${required_file}")
  endif()
endforeach()

file(REMOVE_RECURSE "${PACKAGE_DIR}")
file(MAKE_DIRECTORY "${PACKAGE_DIR}")

file(COPY_FILE "${NESTICLE_EXE}" "${PACKAGE_DIR}/NESticle.exe" ONLY_IF_DIFFERENT)
file(COPY_FILE "${SDL3_DLL}" "${PACKAGE_DIR}/SDL3.dll" ONLY_IF_DIFFERENT)
file(COPY_FILE "${SOURCE_DIR}/gui.vol" "${PACKAGE_DIR}/gui.vol" ONLY_IF_DIFFERENT)
file(COPY_FILE "${SOURCE_DIR}/NESticle.pal" "${PACKAGE_DIR}/NESticle.pal" ONLY_IF_DIFFERENT)
file(COPY_FILE "${SOURCE_DIR}/README.md" "${PACKAGE_DIR}/README.md" ONLY_IF_DIFFERENT)
file(COPY_FILE "${SOURCE_DIR}/LICENSE" "${PACKAGE_DIR}/LICENSE" ONLY_IF_DIFFERENT)
file(COPY_FILE "${SOURCE_DIR}/external/SDL3-3.4.10/LICENSE.txt" "${PACKAGE_DIR}/SDL3-LICENSE.txt" ONLY_IF_DIFFERENT)
file(COPY_FILE "${SOURCE_DIR}/external/SDL3-3.4.10-PROVENANCE.md" "${PACKAGE_DIR}/SDL3-PROVENANCE.md" ONLY_IF_DIFFERENT)

file(READ "${SOURCE_DIR}/external/SDL3-3.4.10/LICENSE.txt" sdl3_license)
file(READ "${SOURCE_DIR}/Source/m6502/fake6502.c" fake6502_source LIMIT 1400)
file(WRITE "${PACKAGE_DIR}/THIRD_PARTY_NOTICES.txt"
"NESticle SDL3 Restoration Package

NESticle is copyright Bloodlust Software / Icer Addis.
This package is a good-faith restoration and includes no ROMs.

SDL3
----
Upstream: https://github.com/libsdl-org/SDL
Version: 3.4.10
License: zlib
Reuse mode: vendored dependency

${sdl3_license}

fake6502
--------
Author: Mike Chambers
License: public domain
Reuse mode: embedded CPU core

Relevant source notice excerpt:

${fake6502_source}
")

file(REMOVE "${PACKAGE_ZIP}")
find_program(POWERSHELL_EXE pwsh)
if(NOT POWERSHELL_EXE)
  find_program(POWERSHELL_EXE powershell)
endif()

if(POWERSHELL_EXE)
  execute_process(
    COMMAND "${POWERSHELL_EXE}" -NoProfile -ExecutionPolicy Bypass
      -File "${SOURCE_DIR}/cmake/write_deterministic_zip.ps1"
      -SourceDir "${PACKAGE_DIR}"
      -ZipPath "${PACKAGE_ZIP}"
    RESULT_VARIABLE zip_result
  )
  if(NOT zip_result EQUAL 0)
    message(FATAL_ERROR "PowerShell zip packaging failed with exit code ${zip_result}")
  endif()
else()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${PACKAGE_ZIP}" --format=zip
      "NESticle.exe"
      "SDL3.dll"
      "gui.vol"
      "NESticle.pal"
      "README.md"
      "LICENSE"
      "SDL3-LICENSE.txt"
      "SDL3-PROVENANCE.md"
      "THIRD_PARTY_NOTICES.txt"
    WORKING_DIRECTORY "${PACKAGE_DIR}"
    RESULT_VARIABLE zip_result
  )
  if(NOT zip_result EQUAL 0)
    message(FATAL_ERROR "CMake zip packaging failed with exit code ${zip_result}")
  endif()
endif()

message(STATUS "Packaged ${PACKAGE_ZIP}")
