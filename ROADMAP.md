# Roadmap

## Phase 0: Analysis & Planning [COMPLETE]
- [x] Clone and explore repository
- [x] Identify all source files and dependencies
- [x] Recover missing source from ZIP archives
- [x] Identify modernization opportunities
- [x] Finalize implementation plan

## Phase 1: Project Setup [NOT STARTED]
- [ ] Create CMakeLists.txt with SDL2 integration
- [ ] Extract M6502.H from SOUND.ZIP into source tree
- [ ] Create .geminiignore
- [ ] Verify CMake configures without errors

## Phase 2: 6502 CPU Core [NOT STARTED]
- [ ] Download/integrate fake6502.c
- [ ] Write m6502_shim.cpp bridging fake6502 to M6502.H API
- [ ] Verify CPU core compiles standalone

## Phase 3: NES APU [NOT STARTED]
- [ ] Integrate Blargg's Nes_Snd_Emu
- [ ] Adapt NESSOUND.CPP to use Blargg's interface
- [ ] All 5 channels: square x2, triangle, noise, DMC

## Phase 4: ASM to C [NOT STARTED]
- [ ] Implement draw_tile_asm in C
- [ ] Implement draw_sprite_asm / draw_sprite_behind_asm in C
- [ ] Implement drawimager2 (RLE blit) in C
- [ ] Implement drawing primitives in C

## Phase 5: SDL2 Backend [NOT STARTED]
- [ ] Video: SDL_Window + 8-bit surface + palette
- [ ] Audio: SDL audio callback to APU
- [ ] Input: keyboard + mouse via SDL events
- [ ] Timer: SDL_GetTicks game loop
- [ ] Surface class for offscreen rendering

## Phase 6: MSVC Compatibility [NOT STARTED]
- [ ] Fix Borland/Watcom-specific headers and pragmas
- [ ] Fix C++ standard compliance issues
- [ ] Resolve type conflicts with Windows headers

## Phase 7: First Build & Testing [NOT STARTED]
- [ ] Achieve zero compile errors
- [ ] Window opens
- [ ] Load and display a NES ROM
- [ ] Keyboard input works
- [ ] Sound output works

## Stretch Goals
- [ ] Dear ImGui overlay for modern ROM browser
- [ ] Additional mapper support
- [ ] Save state improvements
- [ ] Fullscreen / windowed toggle
