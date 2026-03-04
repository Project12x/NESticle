# Changelog

All notable changes to this project will be documented in this file.
Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added
- Project documentation (README, STATE, ROADMAP, ARCHITECTURE, HOWTO, CHANGELOG)
- Implementation plan for modernization

### Discovered
- Complete missing source code recovered from SOUND.ZIP archive:
  - 6502 CPU core (C++ portable version + x86 ASM version)
  - Win95 DirectDraw/DirectSound platform backend
  - DOS platform backend
  - Win95 keyboard handler

### Planned
- CMake build system with SDL2
- fake6502 CPU core (public domain, replacing restrictive original)
- Blargg's nes_apu (LGPL, replacing incomplete 3-channel original)
- C replacements for x86 ASM tile/sprite blitters
- SDL2 platform backend replacing DirectDraw/DirectSound
- MSVC compatibility fixes for Borland/Watcom C++ idioms
