# Save-State Format

Save states use the ROM base name plus `.sta`. They are restoration-safe snapshots of the existing NESticle runtime state; they do not change gameplay semantics.

## Header

New save states start with:

- magic: `NSTA`
- version: `1`
- payload size: `sizeof(nesstate)` for this build
- mapper id: current iNES mapper nibble
- flags: reserved, currently `0`

The loader validates the header before changing emulator state. Missing, old, truncated, wrong-version, wrong-size, or wrong-mapper state files are rejected cleanly and the running emulator is left unchanged.

## Payload

The payload captures:

- CPU low RAM and battery RAM window
- 6502 PC, AF, X, Y, and stack registers
- sprite memory
- PPU pattern tables, name tables, palettes, mirroring, and last screen context
- selected `$2000` and `$4000` port mirrors
- APU enable bits
- mapper-private state buffer

The format is build-specific. It is intended for hardening against crashes and corrupt files, not for cross-version interchange.
