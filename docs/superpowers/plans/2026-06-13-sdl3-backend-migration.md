# SDL3 Backend Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace SDL2 with SDL3 as NESticle's default modern Windows backend while preserving the emulator's visible behavior, retro UI, input semantics, and audio contract.

**Architecture:** Keep the platform boundary in lowercase modern code. Vendor SDL3 source under `external/`, switch CMake to build/link `SDL3::SDL3`, replace `Source/platform_sdl2.cpp` with an SDL3 backend, and port the small SDL-dependent areas in input/remap code behind `SDL3` compile guards. The uppercase emulator core remains behavior-stable.

**Tech Stack:** CMake 3.20+, MSVC 2022, SDL3 3.4.10 source release, C++17, existing NESticle C/C++ source.

---

## Scope

This plan implements only Phase 10 from `ROADMAP.md`. It does not add the later reliability, file UX, save-state, packaging, or diagnostics phases except where minimal changes are required to get the SDL3 backend building and running.

The current latest upstream SDL release checked during planning is `3.4.10` from `libsdl-org/SDL`, release tag `release-3.4.10`, short commit `8e37db5`, zlib license. Source used during implementation must be pinned to that release unless a newer SDL3 release is deliberately selected and recorded.

## File Structure

- Create: `Source/platform_sdl3.cpp`
  - SDL3 replacement for `Source/platform_sdl2.cpp`.
  - Owns process startup, window, renderer, texture, indexed framebuffer, audio stream, event loop, fullscreen, mouse, and keyboard event translation.
- Modify: `CMakeLists.txt`
  - Replace vendored SDL2 paths with vendored SDL3 source subdirectory.
  - Build/link SDL3 and copy `SDL3.dll`.
  - Build `Source/platform_sdl3.cpp` instead of `Source/platform_sdl2.cpp`.
- Modify: `Source/INPUT.CPP`
  - Replace `SDL2` game controller code path with `SDL3` gamepad code path.
  - Keep same `padmap` integer storage and NES button behavior.
- Modify: `Source/INPUTW.CPP`
  - Replace SDL2 controller button polling and names with SDL3 gamepad APIs.
  - Keep the existing retro remap dialog.
- Modify: `ARCHITECTURE.md`
  - Describe SDL3 as the platform backend.
- Modify: `HOWTO.md`
  - Update build/run notes from SDL2 to SDL3 and remove stale `CMAKE_PREFIX_PATH` guidance.
- Modify: `CHANGELOG.md`
  - Add an Unreleased entry for the SDL3 backend migration.
- Create: `external/SDL3-3.4.10/`
  - Vendored SDL3 source release, not handwritten code.
- Create: `external/SDL3-3.4.10-PROVENANCE.md`
  - Records upstream repo, tag, commit, license, download URL, checksum, files inspected, and reuse mode.

## Task 1: Vendor Pinned SDL3 Source

**Files:**
- Create: `external/SDL3-3.4.10/`
- Create: `external/SDL3-3.4.10-PROVENANCE.md`

- [ ] **Step 1: Download SDL3 3.4.10 source archive**

Run:

```powershell
Invoke-WebRequest -Uri "https://github.com/libsdl-org/SDL/releases/download/release-3.4.10/SDL3-3.4.10.zip" -OutFile "C:\tmp\SDL3-3.4.10.zip"
```

Expected: `C:\tmp\SDL3-3.4.10.zip` exists and is roughly 16 MB.

- [ ] **Step 2: Verify SHA-256**

Run:

```powershell
Get-FileHash "C:\tmp\SDL3-3.4.10.zip" -Algorithm SHA256
```

Expected hash:

```text
BE8DAFEF3ECC956E736CFD1C242C10737F3AD589A866F25BA437B7F2F6D5CED2
```

- [ ] **Step 3: Extract into `external/`**

Run:

```powershell
Expand-Archive -LiteralPath "C:\tmp\SDL3-3.4.10.zip" -DestinationPath "external" -Force
```

Expected: `external/SDL3-3.4.10/CMakeLists.txt`, `external/SDL3-3.4.10/include/SDL3/SDL.h`, and `external/SDL3-3.4.10/LICENSE.txt` exist.

- [ ] **Step 4: Inspect upstream source and license before use**

Read these files:

```powershell
Get-Content external\SDL3-3.4.10\LICENSE.txt
Get-Content external\SDL3-3.4.10\CMakeLists.txt | Select-Object -First 120
Get-Content external\SDL3-3.4.10\docs\README-cmake.md
Get-Content external\SDL3-3.4.10\docs\README-migration.md | Select-String -Pattern "SDL_AudioStream|SDL_GameController|SDL_RenderCopy|SDL_RenderSetLogicalSize" -Context 3
```

Expected: license is zlib; CMake exposes SDL3 targets; migration docs confirm the audio stream and gamepad API changes used below.

- [ ] **Step 5: Add provenance file**

Create `external/SDL3-3.4.10-PROVENANCE.md`:

```markdown
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
```

- [ ] **Step 6: Commit vendored SDL3**

Run:

```powershell
git -c safe.directory=D:/Code/nesticleS add external/SDL3-3.4.10 external/SDL3-3.4.10-PROVENANCE.md
git -c safe.directory=D:/Code/nesticleS commit -m "deps: vendor SDL3 3.4.10"
```

Expected: commit succeeds and contains only SDL3 dependency files plus provenance.

## Task 2: Switch CMake From SDL2 To SDL3

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Replace SDL2 configuration block**

Replace:

```cmake
# SDL2 paths (extracted to external/)
set(SDL2_DIR "${CMAKE_SOURCE_DIR}/external/SDL2-2.30.10")
set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include")
set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x64")
```

with:

```cmake
# SDL3 source is vendored under external/ and built as a CMake subproject.
set(SDL3_DIR "${CMAKE_SOURCE_DIR}/external/SDL3-3.4.10")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
add_subdirectory("${SDL3_DIR}" EXCLUDE_FROM_ALL)
```

- [ ] **Step 2: Replace platform source**

Replace the `MODERN_SOURCES` entry:

```cmake
    Source/platform_sdl2.cpp
```

with:

```cmake
    Source/platform_sdl3.cpp
```

- [ ] **Step 3: Replace include/link definitions**

Replace:

```cmake
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
```

with:

```cmake
target_include_directories(NESticle PRIVATE
    Source
    Source/m6502
)

target_compile_definitions(NESticle PRIVATE SDL3)

target_link_libraries(NESticle PRIVATE SDL3::SDL3)
```

- [ ] **Step 4: Replace runtime DLL copy command**

Replace:

```cmake
# Copy SDL2.dll to output directory
add_custom_command(TARGET NESticle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_LIB_DIR}/SDL2.dll"
        $<TARGET_FILE_DIR:NESticle>
)
```

with:

```cmake
# Copy SDL3.dll to output directory on Windows.
if(WIN32)
    add_custom_command(TARGET NESticle POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3-shared>
            $<TARGET_FILE_DIR:NESticle>
        VERBATIM
    )
endif()
```

- [ ] **Step 5: Configure to expose CMake errors early**

Run:

```powershell
cmake -B build-sdl3 -G "Visual Studio 17 2022"
```

Expected: configure reaches the compile phase setup. If SDL3 target naming differs, inspect `external/SDL3-3.4.10/docs/README-cmake.md` and adjust only the SDL3 target/copy expression.

- [ ] **Step 6: Commit CMake switch**

Run:

```powershell
git -c safe.directory=D:/Code/nesticleS add CMakeLists.txt
git -c safe.directory=D:/Code/nesticleS commit -m "build: switch project to SDL3"
```

Expected: commit contains only `CMakeLists.txt`.

## Task 3: Create SDL3 Platform Backend

**Files:**
- Create: `Source/platform_sdl3.cpp`
- Keep: `Source/platform_sdl2.cpp` unchanged for historical comparison, but not built.

- [ ] **Step 1: Copy SDL2 backend as migration starting point**

Run:

```powershell
Copy-Item -LiteralPath Source\platform_sdl2.cpp -Destination Source\platform_sdl3.cpp
```

Expected: `Source/platform_sdl3.cpp` exists and is identical before edits.

- [ ] **Step 2: Update header and include**

At the top of `Source/platform_sdl3.cpp`, replace:

```cpp
// platform_sdl2.cpp - SDL2 platform backend for NESticle
// Replaces WIN95/DX.CPP (DirectDraw/DirectSound) and DOS backends
// Provides: video, audio, input, timer, main loop

// WIN95 is defined globally by CMake
#include <SDL.h>
```

with:

```cpp
// platform_sdl3.cpp - SDL3 platform backend for NESticle
// Replaces WIN95/DX.CPP (DirectDraw/DirectSound) and DOS backends
// Provides: video, audio, input, timer, main loop

// WIN95 is defined globally by CMake
#include <SDL3/SDL.h>
```

- [ ] **Step 3: Replace SDL2 audio device global**

Replace:

```cpp
static SDL_AudioDeviceID sdl_audio_device = 0;
```

with:

```cpp
static SDL_AudioStream *sdl_audio_stream = nullptr;
```

- [ ] **Step 4: Replace fullscreen helpers**

Replace `toggle_sdl_fullscreen()` and `is_sdl_fullscreen()` with:

```cpp
void toggle_sdl_fullscreen() {
  if (!sdl_window)
    return;
  Uint32 flags = SDL_GetWindowFlags(sdl_window);
  if (flags & SDL_WINDOW_FULLSCREEN) {
    if (!SDL_SetWindowFullscreen(sdl_window, false))
      fprintf(stderr, "[SDL3] leave fullscreen failed: %s\n", SDL_GetError());
    changeresolution(640, 480);
    SDL_SetRenderLogicalPresentation(sdl_renderer, 640, 480,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX,
                                     SDL_SCALEMODE_NEAREST);
  } else {
    changeresolution(256, 224);
    SDL_SetRenderLogicalPresentation(sdl_renderer, 256, 224,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX,
                                     SDL_SCALEMODE_NEAREST);
    if (!SDL_SetWindowFullscreen(sdl_window, true))
      fprintf(stderr, "[SDL3] enter fullscreen failed: %s\n", SDL_GetError());
  }
}

int is_sdl_fullscreen() {
  if (!sdl_window)
    return 0;
  return (SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_FULLSCREEN) ? 1 : 0;
}
```

- [ ] **Step 5: Replace SDL2 audio callback with SDL3 audio stream callback**

Replace:

```cpp
static void sdl_audio_callback(void *userdata, Uint8 *stream, int len) {
  (void)userdata;
  short *out = (short *)stream;
  int samples = len / 2;
  for (int i = 0; i < samples; i++) {
    if (primary_read_pos != primary_write_pos) {
      out[i] = primary_buf[primary_read_pos];
      primary_read_pos = (primary_read_pos + 1) % PRIMARY_BUF_SIZE;
    } else {
      out[i] = 0;
    }
  }
}
```

with:

```cpp
static void SDLCALL sdl_audio_stream_callback(void *userdata,
                                              SDL_AudioStream *stream,
                                              int additional_amount,
                                              int total_amount) {
  (void)userdata;
  (void)total_amount;
  if (additional_amount <= 0)
    return;

  int samples = additional_amount / (int)sizeof(short);
  if (samples <= 0)
    return;

  short stack_buf[1024];
  short *out = stack_buf;
  if (samples > (int)(sizeof(stack_buf) / sizeof(stack_buf[0])))
    out = (short *)SDL_malloc(samples * sizeof(short));
  if (!out)
    return;

  for (int i = 0; i < samples; i++) {
    if (primary_read_pos != primary_write_pos) {
      out[i] = primary_buf[primary_read_pos];
      primary_read_pos = (primary_read_pos + 1) % PRIMARY_BUF_SIZE;
    } else {
      out[i] = 0;
    }
  }

  SDL_PutAudioStreamData(stream, out, samples * (int)sizeof(short));
  if (out != stack_buf)
    SDL_free(out);
}
```

- [ ] **Step 6: Update SDL initialization and window creation**

Replace the SDL init block:

```cpp
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER |
               SDL_INIT_GAMECONTROLLER) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }
```

with:

```cpp
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }
```

Replace window creation:

```cpp
  sdl_window =
      SDL_CreateWindow("NESticle", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 640 * sdl_scale, 480 * sdl_scale,
                       SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
```

with:

```cpp
  sdl_window =
      SDL_CreateWindow("NESticle", 640 * sdl_scale, 480 * sdl_scale,
                       SDL_WINDOW_RESIZABLE);
```

- [ ] **Step 7: Update cursor, renderer, hint, and logical scaling APIs**

Replace:

```cpp
  SDL_ShowCursor(SDL_DISABLE);

  sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
  if (!sdl_renderer)
    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);
  // Use nearest-neighbor for crisp pixel art scaling
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  // Let SDL handle scaling/letterboxing to any window/fullscreen size
  SDL_RenderSetLogicalSize(sdl_renderer, SCREENX, SCREENY);
```

with:

```cpp
  SDL_HideCursor();

  sdl_renderer = SDL_CreateRenderer(sdl_window, nullptr);
  if (!sdl_renderer) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
    return 1;
  }
  SDL_SetRenderLogicalPresentation(sdl_renderer, SCREENX, SCREENY,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX,
                                   SDL_SCALEMODE_NEAREST);
```

- [ ] **Step 8: Replace audio open/start block**

Replace:

```cpp
  SDL_AudioSpec want, have;
  memset(&want, 0, sizeof(want));
  want.freq = SOUNDRATE;
  want.format = AUDIO_S16;
  want.channels = 1;
  want.samples = 512;
  want.callback = sdl_audio_callback;
  sdl_audio_device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
  if (sdl_audio_device > 0) {
    soundinstalled = 1;
    SOUNDRATE = have.freq;
    SDL_PauseAudioDevice(sdl_audio_device, 0);
  }
```

with:

```cpp
  SDL_AudioSpec want;
  SDL_zero(want);
  want.freq = SOUNDRATE;
  want.format = SDL_AUDIO_S16;
  want.channels = 1;
  sdl_audio_stream =
      SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want,
                                sdl_audio_stream_callback, nullptr);
  if (sdl_audio_stream) {
    soundinstalled = 1;
    SDL_ResumeAudioStreamDevice(sdl_audio_stream);
  } else {
    fprintf(stderr, "[SDL3] audio disabled: %s\n", SDL_GetError());
  }
```

- [ ] **Step 9: Replace event constants and key fields**

In the event loop, replace:

```cpp
        case SDL_QUIT:
```

with:

```cpp
        case SDL_EVENT_QUIT:
```

Replace:

```cpp
        case SDL_KEYDOWN: {
          if (ev.key.keysym.sym == SDLK_RETURN &&
              (ev.key.keysym.mod & KMOD_ALT)) {
```

with:

```cpp
        case SDL_EVENT_KEY_DOWN: {
          SDL_Keymod mod = SDL_GetModState();
          if (ev.key.key == SDLK_RETURN && (mod & SDL_KMOD_ALT)) {
```

Replace both SDL2 scancode reads:

```cpp
          char sc = sdl_to_nesticle_scancode(ev.key.keysym.scancode);
```

with:

```cpp
          char sc = sdl_to_nesticle_scancode(ev.key.scancode);
```

Replace:

```cpp
        case SDL_KEYUP: {
```

with:

```cpp
        case SDL_EVENT_KEY_UP: {
```

Replace mouse event constants:

```cpp
        case SDL_MOUSEMOTION:
```

with:

```cpp
        case SDL_EVENT_MOUSE_MOTION:
```

and:

```cpp
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
```

with:

```cpp
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
```

- [ ] **Step 10: Replace render copy and audio cleanup**

Replace:

```cpp
        SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, nullptr);
```

with:

```cpp
        SDL_RenderTexture(sdl_renderer, sdl_texture, nullptr, nullptr);
```

Replace shutdown audio:

```cpp
  if (sdl_audio_device > 0)
    SDL_CloseAudioDevice(sdl_audio_device);
```

with:

```cpp
  if (sdl_audio_stream)
    SDL_DestroyAudioStream(sdl_audio_stream);
```

- [ ] **Step 11: Build and capture remaining SDL3 compiler errors**

Run:

```powershell
cmake --build build-sdl3 --config Release -- /m:1
```

Expected: compiler errors are limited to SDL3 API names in `Source/platform_sdl3.cpp`, `Source/INPUT.CPP`, or `Source/INPUTW.CPP`. Fix exact SDL3 names by checking vendored headers in `external/SDL3-3.4.10/include/SDL3/`.

- [ ] **Step 12: Commit platform backend**

Run:

```powershell
git -c safe.directory=D:/Code/nesticleS add Source/platform_sdl3.cpp
git -c safe.directory=D:/Code/nesticleS commit -m "feat: add SDL3 platform backend"
```

Expected: commit contains only `Source/platform_sdl3.cpp`.

## Task 4: Port Gamepad Input To SDL3

**Files:**
- Modify: `Source/INPUT.CPP`

- [ ] **Step 1: Replace SDL include guard**

Replace:

```cpp
#ifdef SDL2
#include <SDL.h>
#endif
```

with:

```cpp
#ifdef SDL3
#include <SDL3/SDL.h>
#endif
```

- [ ] **Step 2: Replace SDL2 gamepad class guard and type names**

Replace:

```cpp
#ifdef SDL2
//-------------------------------
// SDL2 GameController input
// Supports XInput and DirectInput via SDL2's unified API
//-------------------------------
```

with:

```cpp
#ifdef SDL3
//-------------------------------
// SDL3 Gamepad input
// Supports XInput and DirectInput via SDL3's unified API
//-------------------------------
```

Within `class sdlgamepad`, replace:

```cpp
  SDL_GameController *controller;
```

with:

```cpp
  SDL_Gamepad *controller;
```

- [ ] **Step 3: Replace controller open/close/query calls**

Replace:

```cpp
    if (SDL_IsGameController(sdlJoyIdx))
      controller = SDL_GameControllerOpen(sdlJoyIdx);
```

with:

```cpp
    if (SDL_IsGamepad(sdlJoyIdx))
      controller = SDL_OpenGamepad(sdlJoyIdx);
```

Replace:

```cpp
      SDL_GameControllerClose(controller);
```

with:

```cpp
      SDL_CloseGamepad(controller);
```

Replace:

```cpp
        SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
```

with:

```cpp
        SDL_GetGamepadButton(controller, SDL_GAMEPAD_BUTTON_DPAD_UP);
```

Apply the same replacement pattern:

```text
SDL_GameControllerGetButton -> SDL_GetGamepadButton
SDL_GameControllerGetAxis   -> SDL_GetGamepadAxis
SDL_CONTROLLER_BUTTON_*     -> SDL_GAMEPAD_BUTTON_*
SDL_CONTROLLER_AXIS_LEFTX   -> SDL_GAMEPAD_AXIS_LEFTX
SDL_CONTROLLER_AXIS_LEFTY   -> SDL_GAMEPAD_AXIS_LEFTY
SDL_GameControllerButton    -> SDL_GamepadButton
```

- [ ] **Step 4: Replace reconnect path**

Replace:

```cpp
      if (joystickIndex < SDL_NumJoysticks() &&
          SDL_IsGameController(joystickIndex)) {
        controller = SDL_GameControllerOpen(joystickIndex);
      }
```

with:

```cpp
      if (joystickIndex < SDL_GetNumGamepads() &&
          SDL_IsGamepad(joystickIndex)) {
        controller = SDL_OpenGamepad(joystickIndex);
      }
```

- [ ] **Step 5: Replace factory guards**

Replace both `#ifdef SDL2` factory guards with `#ifdef SDL3`.

- [ ] **Step 6: Build only far enough to validate input symbols**

Run:

```powershell
cmake --build build-sdl3 --config Release -- /m:1
```

Expected: no remaining `SDL_GameController*` or `SDL_CONTROLLER_*` errors in `Source/INPUT.CPP`.

- [ ] **Step 7: Commit input backend port**

Run:

```powershell
git -c safe.directory=D:/Code/nesticleS add Source/INPUT.CPP
git -c safe.directory=D:/Code/nesticleS commit -m "feat: port gamepad input to SDL3"
```

Expected: commit contains only `Source/INPUT.CPP`.

## Task 5: Port Gamepad Remap Dialog To SDL3

**Files:**
- Modify: `Source/INPUTW.CPP`

- [ ] **Step 1: Replace SDL include guard**

If the file includes SDL under `SDL2`, replace it with:

```cpp
#ifdef SDL3
#include <SDL3/SDL.h>
#endif
```

- [ ] **Step 2: Replace button names helper**

Replace the SDL2 controller button helper switch with SDL3 names:

```cpp
static const char *getSDLButtonName(int button) {
  switch ((SDL_GamepadButton)button) {
  case SDL_GAMEPAD_BUTTON_SOUTH: return "South";
  case SDL_GAMEPAD_BUTTON_EAST: return "East";
  case SDL_GAMEPAD_BUTTON_WEST: return "West";
  case SDL_GAMEPAD_BUTTON_NORTH: return "North";
  case SDL_GAMEPAD_BUTTON_BACK: return "Back";
  case SDL_GAMEPAD_BUTTON_GUIDE: return "Guide";
  case SDL_GAMEPAD_BUTTON_START: return "Start";
  case SDL_GAMEPAD_BUTTON_LEFT_STICK: return "LStick";
  case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return "RStick";
  case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return "LShoulder";
  case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return "RShoulder";
  case SDL_GAMEPAD_BUTTON_DPAD_UP: return "D-Up";
  case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return "D-Down";
  case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return "D-Left";
  case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return "D-Right";
  default: return "???";
  }
}
```

- [ ] **Step 3: Replace modal polling code**

Replace the SDL2 polling block inside `padedit::drawdata()`:

```cpp
      SDL_GameControllerUpdate();
      for (int ji = 0; ji < SDL_NumJoysticks(); ji++) {
        if (!SDL_IsGameController(ji)) continue;
        SDL_GameController *gc = SDL_GameControllerOpen(ji);
        if (!gc) continue;
        for (int b = SDL_CONTROLLER_BUTTON_A;
             b <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT; b++) {
          if (SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)b)) {
            buttonRef = b;
            setmodal(0);
            parent->cyclefocus(0);
            SDL_GameControllerClose(gc);
            goto done_polling;
          }
        }
        SDL_GameControllerClose(gc);
      }
```

with:

```cpp
      SDL_UpdateGamepads();
      int count = 0;
      SDL_JoystickID *gamepads = SDL_GetGamepads(&count);
      if (gamepads) {
        for (int i = 0; i < count; i++) {
          SDL_Gamepad *gp = SDL_OpenGamepad(gamepads[i]);
          if (!gp)
            continue;
          for (int b = SDL_GAMEPAD_BUTTON_SOUTH;
               b <= SDL_GAMEPAD_BUTTON_DPAD_RIGHT; b++) {
            if (SDL_GetGamepadButton(gp, (SDL_GamepadButton)b)) {
              buttonRef = b;
              setmodal(0);
              parent->cyclefocus(0);
              SDL_CloseGamepad(gp);
              SDL_free(gamepads);
              goto done_polling;
            }
          }
          SDL_CloseGamepad(gp);
        }
        SDL_free(gamepads);
      }
```

- [ ] **Step 4: Build and verify SDL3 enum names**

Run:

```powershell
cmake --build build-sdl3 --config Release -- /m:1
```

Expected: no remaining SDL2 controller symbols in `Source/INPUTW.CPP`. If the compiler reports an SDL3 enum spelling error, open `external/SDL3-3.4.10/include/SDL3/SDL_gamepad.h`, replace the misspelled enum with the header's exact symbol, and rerun this command.

- [ ] **Step 5: Commit remap dialog port**

Run:

```powershell
git -c safe.directory=D:/Code/nesticleS add Source/INPUTW.CPP
git -c safe.directory=D:/Code/nesticleS commit -m "feat: port gamepad remap dialog to SDL3"
```

Expected: commit contains only `Source/INPUTW.CPP`.

## Task 6: Resolve Final Build Errors And Preserve Behavior

**Files:**
- Modify only files already touched in Tasks 2-5 unless the compiler proves another file has an SDL2 dependency.

- [ ] **Step 1: Search for remaining SDL2 references**

Run:

```powershell
rg -n "SDL2|SDL_GameController|SDL_CONTROLLER_|SDL_RenderCopy|SDL_RenderSetLogicalSize|SDL_OpenAudioDevice|SDL_PauseAudioDevice|SDL_QUIT|SDL_KEYDOWN|SDL_KEYUP|SDL_MOUSE" Source CMakeLists.txt
```

Expected: no active SDL2 API references remain in built code. It is acceptable for `Source/platform_sdl2.cpp` to contain SDL2 references because it is no longer built.

- [ ] **Step 2: Build Release**

Run:

```powershell
cmake --build build-sdl3 --config Release -- /m:1
```

Expected: `build-sdl3/Release/NESticle.exe` and `build-sdl3/Release/SDL3.dll` exist.

- [ ] **Step 3: Build Debug**

Run:

```powershell
cmake --build build-sdl3 --config Debug -- /m:1
```

Expected: `build-sdl3/Debug/NESticle.exe` and `build-sdl3/Debug/SDL3.dll` exist.

- [ ] **Step 4: Commit final compile fixes**

Run:

```powershell
git -c safe.directory=D:/Code/nesticleS add Source/platform_sdl3.cpp Source/INPUT.CPP Source/INPUTW.CPP CMakeLists.txt
git -c safe.directory=D:/Code/nesticleS commit -m "fix: complete SDL3 backend build"
```

Expected: if no files changed since earlier commits, Git reports nothing to commit and this step is complete.

## Task 7: Manual Runtime Verification

**Files:**
- No source edits unless verification exposes a Phase 10 regression.

- [ ] **Step 1: Launch without ROM from project root**

Run:

```powershell
.\build-sdl3\Release\NESticle.exe
```

Expected:

- Window opens.
- Retro GUI is visible.
- Animated cursor is visible.
- No crash on startup.
- stderr says SDL3 audio either opened or disabled cleanly.

- [ ] **Step 2: Verify fullscreen toggle**

Manual actions:

```text
Press ALT+ENTER.
Wait one second.
Press ALT+ENTER again.
```

Expected:

- Fullscreen enters and exits.
- GUI returns to 640x480 layout after exit.
- No black screen or crash.

- [ ] **Step 3: Verify keyboard input with a mapper-0 ROM**

Run:

```powershell
.\build-sdl3\Release\NESticle.exe "D:\path\to\known-mapper0-test-rom.nes"
```

Expected:

- ROM loads.
- Arrow keys move.
- `Z`/`X` map to B/A.
- `Tab`/`Enter` map to Select/Start.

If no ROM path is available, record this verification item as skipped with the reason. Do not bundle a ROM.

- [ ] **Step 4: Verify gamepad when hardware is available**

Manual actions:

```text
Open Settings -> Redefine input.
Select a gamepad device.
Open Redefine buttons.
Assign B/A/Select/Start.
Save settings.
Restart emulator.
Open the dialog again and confirm mappings persisted.
```

Expected:

- SDL3 gamepad is detected.
- Button names display.
- Remapping works without crashing.
- Saved mapping is still present after restart.

If no gamepad is connected, record this item as skipped with the reason.

- [ ] **Step 5: Verify clean shutdown**

Manual action:

```text
Close the NESticle window.
```

Expected:

- Process exits.
- No crash dialog.
- Audio stream is destroyed cleanly.

## Task 8: Update Documentation For SDL3 Backend

**Files:**
- Modify: `ARCHITECTURE.md`
- Modify: `HOWTO.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: Update `ARCHITECTURE.md` platform references**

Replace references to SDL2 backend with SDL3 backend. The platform section should say:

```markdown
### Platform (`platform_sdl3.cpp`)
- SDL3 window, renderer, texture presentation, and fullscreen handling
- 8-bit indexed framebuffer converted to ARGB for presentation
- SDL3 audio stream feeding the existing NESticle audio ring buffer
- SDL3 keyboard, mouse, and gamepad input
- ALT+ENTER fullscreen toggle with logical presentation and nearest-neighbor scaling
- Replaces original DirectDraw/DirectSound/Win32 backend
```

- [ ] **Step 2: Update `HOWTO.md` build instructions**

Replace the SDL2 prerequisite/build text with:

````markdown
SDL3 source is vendored at `external/SDL3-3.4.10/` and built by CMake. No `CMAKE_PREFIX_PATH` is needed.

```bash
cmake -B build-sdl3 -G "Visual Studio 17 2022"
cmake --build build-sdl3 --config Release
```

Output: `build-sdl3/Release/NESticle.exe`. `SDL3.dll` is copied next to the executable by the post-build step.
````

- [ ] **Step 3: Add changelog entry**

At the top of `CHANGELOG.md`, under the intro, add:

```markdown
## [Unreleased]

### Changed
- Migrated the modern platform backend from SDL2 to SDL3 while preserving NESticle's retro UI, 8-bit framebuffer presentation, keyboard defaults, gamepad remapping behavior, fullscreen command, and existing emulator timing model.
```

- [ ] **Step 4: Commit docs**

Run:

```powershell
git -c safe.directory=D:/Code/nesticleS add ARCHITECTURE.md HOWTO.md CHANGELOG.md
git -c safe.directory=D:/Code/nesticleS commit -m "docs: document SDL3 backend migration"
```

Expected: commit contains only documentation files.

## Task 9: Final Phase 10 Verification

**Files:**
- No edits unless verification exposes a missed SDL3 migration issue.

- [ ] **Step 1: Confirm branch and status**

Run:

```powershell
git -c safe.directory=D:/Code/nesticleS branch --show-current
git -c safe.directory=D:/Code/nesticleS status --short
```

Expected:

```text
codex/sdl3-improvement-phases
```

Tracked files should be clean. Pre-existing untracked files may remain, but they must not be included in Phase 10 commits unless deliberately added by a task above.

- [ ] **Step 2: Confirm executable and DLL names**

Run:

```powershell
Get-ChildItem build-sdl3\Release\NESticle.exe,build-sdl3\Release\SDL3.dll | Select-Object Name,Length
```

Expected: both files exist and have nonzero size.

- [ ] **Step 3: Confirm no SDL2 build dependency remains**

Run:

```powershell
rg -n "SDL2-2.30.10|SDL2.lib|SDL2main.lib|SDL2.dll|target_compile_definitions\\(NESticle PRIVATE SDL2\\)" CMakeLists.txt ARCHITECTURE.md HOWTO.md README.md STATE.md ROADMAP.md
```

Expected: no matches in active build/docs. If `README.md` or `STATE.md` still mention SDL2 as historical context, update them only if the text now claims SDL2 is the current backend.

- [ ] **Step 4: Final commit if verification required small fixes**

Run:

```powershell
git -c safe.directory=D:/Code/nesticleS add CMakeLists.txt Source/platform_sdl3.cpp Source/INPUT.CPP Source/INPUTW.CPP ARCHITECTURE.md HOWTO.md CHANGELOG.md README.md STATE.md ROADMAP.md
git -c safe.directory=D:/Code/nesticleS commit -m "fix: polish SDL3 migration"
```

Expected: if no files changed since prior commits, Git reports nothing to commit and this step is complete.

## Self-Review

- Spec coverage: This plan implements Phase 10 only: vendored SDL3, CMake switch, platform backend, input/remap port, build verification, runtime smoke checks, and docs. Later roadmap phases intentionally remain separate.
- Placeholder scan: The plan uses exact commands, paths, snippets, expected outcomes, and commit messages.
- Type consistency: SDL3 names are consistently `SDL_Gamepad`, `SDL_OpenAudioDeviceStream`, `SDL_RenderTexture`, `SDL_SetRenderLogicalPresentation`, `SDL_EVENT_*`, and `SDL3::SDL3`.
