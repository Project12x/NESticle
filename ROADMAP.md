# Roadmap

## Phase 0: Analysis & Planning [COMPLETE]
- [x] Clone and explore repository
- [x] Identify all source files and dependencies
- [x] Recover missing source from ZIP archives
- [x] Finalize implementation plan

## Phase 1: Project Setup [COMPLETE]
- [x] Create CMakeLists.txt with SDL2 integration
- [x] Extract M6502.H from SOUND.ZIP into source tree
- [x] Create .geminiignore
- [x] Verify CMake configures without errors

## Phase 2: 6502 CPU Core [COMPLETE]
- [x] Integrate fake6502.c (public domain)
- [x] Write m6502_shim.cpp bridging fake6502 to M6502.H API
- [x] Verify CPU core compiles standalone

## Phase 3: NES APU [COMPLETE]
- [x] Restore original NESticle 5-channel APU (square x2, triangle, noise, DMC)
- [x] Fix noise channel LFSR and timing
- [x] Implement NES hardware nonlinear mixer (DAC formulas)
- [x] Add DC blocking high-pass filter

## Phase 4: ASM to C [COMPLETE]
- [x] Implement draw_tile_asm in C
- [x] Implement draw_sprite_asm / draw_sprite_behind_asm in C
- [x] Implement drawimager2 (RLE blit) in C
- [x] Implement drawing primitives in C

## Phase 5: SDL2 Backend [COMPLETE]
- [x] Video: SDL_Window + 8-bit surface + palette
- [x] Audio: SDL audio callback
- [x] Input: keyboard + mouse + SDL GameController
- [x] Timer: SDL_GetTicks game loop

## Phase 6: MSVC Compatibility [COMPLETE]
- [x] Fix Borland/Watcom-specific headers and pragmas
- [x] Fix C++ standard compliance issues
- [x] Resolve type conflicts with Windows headers

## Phase 7: Emulation Hardening [COMPLETE]
- [x] Fix sprite flipping bits
- [x] Fix APU noise channel
- [x] Fix input/keyboard mapping
- [x] Fix SMB1 Sprite 0 CPU starvation/cycle accuracy

## Phase 8: GUI Modernization [COMPLETE]
- [x] Fix widget positioning for 640x480
- [x] Fix dialog layouts (About, NES Timing, ROM loader, Input)
- [x] Fix popup menu crashes and cascading submenu behavior
- [x] ALT+ENTER fullscreen with SDL logical scaling
- [x] Fix input dialog button visibility

## Phase 9: Input & Audio Polish [COMPLETE]
- [x] Gamepad button remapping UI (padmap + redefinepad dialog)
- [x] Frame-based gamepad button polling
- [x] NES hardware nonlinear mixer
- [x] Psychoacoustic noise attenuation

## Phase 10: SDL3 Backend Migration [COMPLETE]
- [x] Replace SDL2 as the default backend with SDL3 on the improvement branch
- [x] Preserve palettized framebuffer behavior, nearest-neighbor scaling, and retro fullscreen semantics
- [x] Port SDL audio callback usage to SDL3 audio streams
- [x] Port SDL GameController usage to SDL3 gamepad APIs
- [x] Update CMake and runtime DLL copy rules for vendored SDL3

## Phase 11: Reliability Hardening [PLANNED]
- [x] Harden startup asset checks and failure messages
- [x] Fix shutdown order, resource leaks, and null/lifetime hazards
- [ ] Improve GUI focus/click edge cases without changing visual style
- [x] Harden fullscreen transitions and bad-path handling
- [x] Validate file create/read/write failures before mutating state

## Phase 12: Timing & Audio Stability [PLANNED]
- [x] Add SDL3 audio underrun/overrun counters
- [x] Handle audio pause/resume and device recovery
- [x] Bound host-frame catch-up behavior after stalls
- [x] Keep existing emulator timing constants as the compatibility baseline

## Phase 13: Input & Gamepad Durability [PLANNED]
- [x] Add SDL3 gamepad hotplug handling
- [x] Improve multiple-controller discovery and assignment
- [x] Preserve remaps across reconnects and enumeration changes
- [ ] Add deadzone tuning and mapping conflict diagnostics

## Phase 14: ROM & File UX Polish [PLANNED]
- [ ] Keep the retro ROM dialog while improving path handling
- [x] Remember last ROM directory and recent ROMs
- [ ] Improve long filename clipping and failed-load messages
- [ ] Clarify save, palette, snapshot, and battery-backed RAM failures

## Phase 15: Settings Durability [PLANNED]
- [x] Version and validate config files
- [x] Tolerate missing, old, short, oversized, or corrupt configs
- [x] Save config atomically via temp-and-replace
- [x] Avoid silently resetting valid user input mappings

## Phase 16: Save-State Hardening [PLANNED]
- [x] Add save-state magic, version, payload size, mapper id, and flags
- [x] Validate state files before mutating emulator state
- [x] Reject incompatible state files cleanly
- [x] Document captured state and compatibility rules

## Phase 17: Packaging & Verification [PLANNED]
- [x] Add deterministic package output with executable, SDL3 DLL, assets, docs, and license notes
- [x] Add a manual verification checklist for ROMs, input, audio, fullscreen, config, and state files
- [x] Verify launch from both project root and packaged release directory

## Phase 18: Developer Diagnostics [PLANNED]
- [x] Add off-by-default diagnostics for startup, assets, SDL3 devices, and config
- [ ] Log mapper, mirroring, trainer, battery, and timing details
- [x] Log audio underruns/overruns and input hotplug/remap events
- [x] Keep diagnostics out of the retro UI unless explicitly enabled
