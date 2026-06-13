# SDL3 3.4.10 Provenance

- Upstream repo: https://github.com/libsdl-org/SDL
- Release tag: release-3.4.10
- Commit: 8e37db5
- License: zlib
- Source archive: https://github.com/libsdl-org/SDL/releases/download/release-3.4.10/SDL3-3.4.10.zip
- SHA-256: be8dafef3ecc956e736cfd1c242c10737f3ad589a866f25ba437b7f2f6d5ced2
- Reuse mode: dependency
- Files inspected:
  - LICENSE.txt
  - CMakeLists.txt
  - docs/README-cmake.md
  - docs/README-migration.md
- Local integration notes:
  - Built as a vendored CMake subproject.
  - Linked through SDL3::SDL3.
  - Runtime DLL copied from SDL3::SDL3-shared when available.
