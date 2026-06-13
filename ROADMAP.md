# Roadmap

## Preservation Rule

This project is a restoration-minded port, not a correctness rewrite. Future roadmap work must preserve NESticle's 1990s personality: its retro UI, sound character, palette look, timing feel, and visible emulator quirks. Do not "fix" odd behavior merely because another emulator, hardware capture, or modern accuracy test differs. Other NES emulators already cover near-perfect accuracy; this port exists to make NESticle stable and usable on modern Windows without sanding off its identity.

Allowed behavior changes are limited to crashes, memory/resource bugs, bad host integration, corrupt persistence, unsupported-file handling, or a clearly documented compatibility blocker that prevents a game from running. Sound, palette, timing, UI, and glitch signatures should be cataloged, not normalized.

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

## Phase 11: Reliability Hardening [COMPLETE]
- [x] Harden startup asset checks and failure messages
- [x] Fix shutdown order, resource leaks, and null/lifetime hazards
- [x] Improve GUI focus/click edge cases without changing visual style
- [x] Harden fullscreen transitions and bad-path handling
- [x] Validate file create/read/write failures before mutating state

## Phase 12: Timing & Audio Stability [COMPLETE]
- [x] Add SDL3 audio underrun/overrun counters
- [x] Handle audio pause/resume and device recovery
- [x] Bound host-frame catch-up behavior after stalls
- [x] Keep existing emulator timing constants as the compatibility baseline

## Phase 13: Input & Gamepad Durability [COMPLETE]
- [x] Add SDL3 gamepad hotplug handling
- [x] Improve multiple-controller discovery and assignment
- [x] Preserve remaps across reconnects and enumeration changes
- [x] Add deadzone tuning and mapping conflict diagnostics

## Phase 14: ROM & File UX Polish [COMPLETE]
- [x] Keep the retro ROM dialog while improving path handling
- [x] Remember last ROM directory and recent ROMs
- [x] Improve long filename clipping and failed-load messages
- [x] Clarify save, palette, snapshot, and battery-backed RAM failures

## Phase 15: Settings Durability [COMPLETE]
- [x] Version and validate config files
- [x] Tolerate missing, old, short, oversized, or corrupt configs
- [x] Save config atomically via temp-and-replace
- [x] Avoid silently resetting valid user input mappings

## Phase 16: Save-State Hardening [COMPLETE]
- [x] Add save-state magic, version, payload size, mapper id, and flags
- [x] Validate state files before mutating emulator state
- [x] Reject incompatible state files cleanly
- [x] Document captured state and compatibility rules

## Phase 17: Packaging & Verification [COMPLETE]
- [x] Add deterministic package output with executable, SDL3 DLL, assets, docs, and license notes
- [x] Add a manual verification checklist for ROMs, input, audio, fullscreen, config, and state files
- [x] Verify launch from both project root and packaged release directory

## Phase 18: Developer Diagnostics [COMPLETE]
- [x] Add off-by-default diagnostics for startup, assets, SDL3 devices, and config
- [x] Log mapper, mirroring, trainer, battery, and timing details
- [x] Log audio underruns/overruns and input hotplug/remap events
- [x] Keep diagnostics out of the retro UI unless explicitly enabled

## Phase 19: Compatibility Characterization [PLANNED]
- [ ] Build a known-ROM compatibility matrix without bundling ROMs
- [ ] Record mapper, mirroring, trainer, battery, region, and observed behavior for each manual test
- [ ] Classify failures as mapper, CPU, PPU, APU, input, timing, save, or host-platform issues
- [ ] Record NESticle-specific sound, palette, timing, UI, and glitch signatures as expected behavior unless they cause crashes, corruption, or a documented game-running blocker

## Phase 20: Mapper Coverage Expansion [PLANNED]
- [ ] Prioritize mapper additions from the compatibility matrix instead of popularity alone
- [ ] Inspect permissively licensed mapper implementations at pinned commits before design work
- [ ] Add mapper diagnostics and rejection messages for unsupported or malformed cartridges
- [ ] Keep new mapper code isolated to `MMC.CPP/.H` and small lowercase helper files when needed

## Phase 21: Timing And PPU Regression Workbench [PLANNED]
- [ ] Add developer-only frame, scanline, sprite-0, and IRQ trace modes
- [ ] Compare behavior against accepted NES test ROM results for diagnosis, not as a blanket mandate to erase NESticle quirks
- [ ] Document each timing change with the ROM/test that motivated it and the NESticle behavior intentionally preserved
- [ ] Avoid broad cycle-accuracy rewrites; prefer the smallest stability or game-running fix that preserves the old feel

## Phase 22: Audio And Input Verification Depth [PLANNED]
- [ ] Add repeatable audio underrun and buffer-stability checks for long sessions without changing NESticle's sound signature
- [ ] Document host audio device, sample rate, and buffer behavior in verification notes
- [ ] Expand gamepad verification across XInput, DirectInput, Bluetooth, and hotplug cases
- [ ] Keep NES controller semantics and retro remap UI unchanged

## Phase 23: Release Candidate Discipline [PLANNED]
- [ ] Create a release-candidate checklist with package hash, toolchain version, and manual smoke results
- [ ] Verify extracted-package launch on a clean machine or VM
- [ ] Keep license/provenance files current for SDL3 and fake6502
- [ ] Tag only after packaging, startup, ROM, input, audio, settings, and save-state checks are recorded
