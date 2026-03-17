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
