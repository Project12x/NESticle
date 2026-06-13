# SDL3 Improvement Program Design

## Purpose

Move this NESticle restoration from "modern Windows port" to "carefully improved restoration" without changing the 1990s NESticle personality. The nostalgia contract is explicit: preserve emulator quirks, visual presentation, retro GUI behavior, default control feel, and the existing 8-bit indexed rendering model unless a change prevents crashes, data loss, or host-platform failure.

## Approved Direction

SDL3 becomes the new backend immediately on branch `codex/sdl3-improvement-phases`. This is not a dual-backend experiment. The branch may break temporarily during migration, but each implementation phase must end with a buildable and manually testable emulator before moving to the next phase.

## Non-Goals

- Do not rewrite the emulator core.
- Do not modernize the retro GUI style.
- Do not "fix" historical emulation quirks for accuracy unless the existing port is crashing or corrupting state.
- Do not add bundled ROMs, BIOS files, or claims of authorship over the original NESticle core.
- Do not reformat or broad-clean original uppercase source files.

## Primary References

- Project docs inspected: `README.md`, `STATE.md`, `ROADMAP.md`, `ARCHITECTURE.md`, `HOWTO.md`, `CHANGELOG.md`.
- SDL3 documentation inspected through Context7:
  - `/libsdl-org/sdlwiki`, `SDL3/README-migration.md`
  - `/libsdl-org/sdlwiki`, `SDL3/SDL_AudioStreamCallback.md`
- Reuse mode for SDL3 migration design: docs-guided pattern-only. Before implementation code is written, inspect the pinned SDL3 release package or source used by the branch, record its version, license, and relevant example/source paths in the implementation notes.

## Architecture

The modern platform layer remains the boundary between host OS/SDL and original NESticle behavior. Replace the SDL2 implementation with an SDL3 implementation in the lowercase platform layer while keeping original uppercase emulator files as stable as possible.

The SDL3 backend must continue to provide:

- process startup and shutdown
- 8-bit framebuffer ownership
- palette upload and ARGB conversion
- window/fullscreen transitions
- keyboard, mouse, and gamepad input
- frame pacing
- audio output
- resource loading and runtime asset diagnostics

Original emulator responsibilities remain in their current files: `MAIN.CPP`, `CPU.CPP`, `NES.CPP`, `NESVIDEO.CPP`, `NESSOUND.CPP`, `MMC.CPP`, `ROM.CPP`, and GUI classes. Changes to uppercase files are allowed only when the data structure or function already lives there and cannot be hardened safely from a shim.

## Phase Design

### Phase 10: SDL3 Backend Migration

Replace SDL2 usage with SDL3 as the default backend. Rename the platform file or create a new SDL3 platform file and remove SDL2 from the default build. Update CMake to consume the vendored SDL3 package and copy `SDL3.dll` into the output directory.

Behavior contract:

- same startup path into `initgame()`
- same calls to `gametimer()` and `updatescreen()`
- same palettized framebuffer dimensions and conversion behavior
- same nearest-neighbor visual character
- same default keyboard mapping
- same ALT+ENTER fullscreen command

Known SDL3 migration risks:

- audio moves from SDL2 device callbacks to SDL3 audio streams
- `SDL_GameController` APIs become SDL3 gamepad APIs
- event constants and some init/shutdown APIs are renamed or adjusted
- CMake target/linkage changes from SDL2 library paths to SDL3 package or vendored target

### Phase 11: Reliability Hardening

Fix host-port failures without changing emulation output. Targets include startup asset checks, bad path handling, shutdown order, null pointer guards, GUI focus/click lifetime issues, fullscreen edge cases, file create/read/write errors, and predictable failure messages.

The output contract is that a valid ROM should render and sound the same after each fix. A fix is acceptable when it changes "crash or undefined behavior" into "message and continue/exit cleanly."

### Phase 12: Timing And Audio Stability

Port audio to SDL3 streams while preserving the current NESticle APU/mixer behavior. Add underrun/overrun counters, device loss handling, pause/resume handling, and stderr diagnostics behind a developer flag.

Frame pacing should be clarified and bounded so window dragging, resize, focus changes, and long host stalls do not spiral into unstable catch-up behavior. The emulator's existing timing constants remain the compatibility baseline.

### Phase 13: Input And Gamepad Durability

Move gamepad support to SDL3 gamepad APIs. Add hotplug handling, clearer remapping persistence, multiple-controller discovery, deadzone tuning, and conflict reporting. Preserve NES button semantics and the retro redefine dialogs.

The input layer should avoid losing user mappings when devices disconnect, reconnect, or appear in a different enumeration order.

### Phase 14: ROM And File UX Polish

Keep the retro file dialog, but make it more forgiving: safer path joins, remembered last directory, recent ROMs, long filename clipping, and clearer messages for failed ROM/header/save/palette/snapshot operations.

Path improvements must not force a modern native file picker or change the visual identity of the UI.

### Phase 15: Settings Durability

Version and validate the config file. Loading should tolerate missing, short, old, oversized, or corrupt configs by falling back to defaults while preserving fields that can be safely read. Saving should be atomic: write a temp file, flush/close it, then replace the old config.

Input mappings are high-value user data and should not be reset silently unless the stored data is invalid.

### Phase 16: Save-State Hardening

Add a small save-state header with magic, version, payload size, mapper id, and compatibility flags. Loading should validate before mutating emulator state. Incompatible state files should produce a clear message and leave the running emulator unchanged.

This phase must not change what emulator state means; it only makes state files safer to read and write.

### Phase 17: Packaging And Verification

Add deterministic package output. A release package should contain:

- `NESticle.exe`
- `SDL3.dll`
- `gui.vol`
- `NESticle.pal`
- `README.md`
- license and notice files for bundled third-party components
- manual verification checklist

Add `docs/verification.md` with a practical checklist for known-good ROM classes, input, audio, fullscreen, config, save/load state, and failure cases. No ROMs are committed or bundled.

### Phase 18: Developer Diagnostics

Add off-by-default diagnostics to stderr or a developer log file. Suggested signals:

- startup paths and asset load status
- SDL3 video/audio/gamepad device details
- mapper number, mirroring, trainer, battery flag
- frame pacing counters
- audio underrun/overrun counters
- input hotplug/remap events
- config load/save outcome
- save-state compatibility failures

Diagnostics must not add visible in-emulator overlays by default.

## Data Flow

SDL3 startup initializes platform services, loads config, calls `initgame()`, and enters the main loop. The loop polls SDL3 events, maps host input into existing NESticle input structures, advances `gametimer()` on the existing cadence, renders through `updatescreen()`, converts the indexed buffer for SDL3 texture presentation, and feeds audio through SDL3 audio streams using samples produced by `nessound::update()`.

ROM and config operations remain driven by existing menu commands. Hardened file code should validate read/write results and report failures through existing message plumbing plus optional diagnostics.

## Error Handling

The preferred failure behavior is:

1. keep the emulator running when the failed operation is optional
2. leave existing state unchanged when a load operation fails
3. show a retro UI message when the user initiated the operation
4. write detailed cause to diagnostics when enabled
5. exit cleanly only when required runtime assets or platform initialization are unavailable

## Verification Strategy

There is no automated test suite. Each phase must include at least:

- Release build with MSVC/CMake
- Launch from project root
- Launch from packaged release directory
- Missing `gui.vol` failure check
- Missing palette failure check
- Load a known mapper-0 ROM
- Exercise keyboard input
- Exercise gamepad detection/remap when hardware is available
- Toggle fullscreen in and out
- Confirm audio starts, continues, pauses/resumes, and shuts down cleanly
- Save/load config
- Save/load state with a valid state file
- Attempt to load a deliberately invalid state file

## Documentation Updates

Keep `ROADMAP.md` as the phase tracker. Update `ARCHITECTURE.md` when SDL3 changes the platform architecture, `HOWTO.md` when build/package/run commands change, and `CHANGELOG.md` when user-visible changes land.

## Approval Status

The user approved SDL3 as the immediate new backend strategy and requested this work on a branch. This design is the scope guard for subsequent implementation plans.
