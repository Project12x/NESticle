# NESticle (Modernized)

NESticle v0.2 (Bloodlust Software, 1997) ported to modern Windows via SDL3.

![NESticle running Super Mario Bros.](screenshot.png)

## Changes from Original

| Component | Original | Replacement |
|---|---|---|
| Compiler | Borland C++ / Watcom | MSVC 2022 |
| Platform | DOS / Win95 DirectDraw+DirectSound | SDL3 |
| 6502 CPU | x86 ASM core | fake6502 (public domain) |
| Rendering | x86 ASM blitters | C |
| Audio | 3-channel linear mixer | 5-channel NES DAC formulas |
| Input | Win95 keyboard | SDL3 keyboard + Gamepad |

## Controls

| Input | Action |
|---|---|
| Arrows | D-pad |
| Z / X | B / A |
| Tab / Enter | Select / Start |
| ALT+ENTER | Fullscreen |
| Gamepad | Works automatically, remappable in Settings |

## Running

Unzip and run `NESticle.exe`. Requires `gui.vol`, `NESticle.pal`, and `SDL3.dll` in the same directory (included in release).

## Current Roadmap

The SDL3 improvement branch is feature-complete for the first modernization pass: deterministic packaging, startup/shutdown hardening, settings durability, save-state validation, developer diagnostics, gamepad durability, and ROM/file UX polish.

Next work is compatibility-driven, but bounded by preservation. The goal is not to make NESticle behave like a modern accuracy emulator. Other NES emulators already do that extremely well. This port should make NESticle stable on modern Windows while keeping its 90s sound, palette look, retro UI, timing feel, and recognizable glitches.

- Build a known-ROM compatibility matrix without bundling ROMs
- Prioritize mapper additions from observed game-running failures
- Add developer-only timing/PPU traces for diagnosis, not blanket accuracy rewrites
- Deepen audio and input verification across host devices while preserving NESticle's sound signature
- Record release-candidate evidence before tagging

## Known Issues

- APU sound is the original NESticle core — not cycle-accurate
- ~8 mappers supported (MMC 0-7)
- Compatibility evidence is still manual and ROM-by-ROM
- Palette and sound differences from other emulators are not bugs by default; they may be intentional NESticle signatures

## License

NESticle is (c) Bloodlust Software / Icer Addis. This is a restoration made in good faith. If the original author objects, this repository will be taken down.

- fake6502: public domain
- SDL3: zlib license
