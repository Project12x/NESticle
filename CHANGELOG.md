# Changelog

All notable changes to this project will be documented in this file.
Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added
- **SDL3 backend**: Vendored SDL 3.4.10 and moved the active platform backend to `platform_sdl3.cpp`.
- **SDL3 audio stream path**: Replaced SDL2 audio device callback setup with an SDL3 audio stream callback fed by the existing emulator ring buffer.
- **Release package target**: Added `package_release`, which stages the SDL3 runtime layout and creates a deterministic release zip with license and provenance notes.
- **Manual verification checklist**: Added `docs/verification.md` for release smoke checks across startup, packaging, ROM loading, input, audio, settings, and save states.
- **Forward roadmap**: Added preservation-bounded compatibility, mapper, timing/PPU, audio/input verification, and release-candidate phases after the completed SDL3 improvement work.
- **Startup asset validation**: Added clear preflight checks for `gui.vol` and `NESticle.pal` before the original initialization path runs.
- **Developer diagnostics flag**: Added `--dev-log` / `--diagnostics` for off-by-default SDL3 audio, timing, and gamepad diagnostics.
- **Release docs**: Release packages now include manual verification and save-state compatibility docs.
- **ROM directory memory**: The retro ROM dialog now remembers the last visited ROM directory and lists recent successful ROM loads.

### Changed
- **Build output**: New SDL3 build tree convention is `build-sdl3/`, with `SDL3.dll` copied beside `NESticle.exe`.
- **Gamepad APIs**: Ported SDL GameController usage to SDL3 Gamepad APIs while keeping the same NES button behavior and remapping UI.
- **SDL3 shutdown path**: Centralized SDL cleanup, restored the Windows timer period on exit, and fail cleanly if the primary texture cannot be created.
- **ROM load failures**: ROM files are now staged in temporary allocations and validated before replacing the current running ROM state.
- **Config durability**: Config loading now tolerates short, old, oversized, or partially invalid files, and saves through a temp file before replacing the old config.
- **Timing/audio counters**: SDL3 audio underrun/overrun and skipped host-frame catch-up ticks are counted without changing the emulator timing constants.
- **Save-state validation**: Save states now include a small magic/version/size/mapper header and are validated before emulator state is restored.
- **Frame pacing after stalls**: Host catch-up after long pauses is now bounded without changing NESticle's timing constants.
- **Gamepad reconnect handling**: SDL3 gamepads now close cleanly when disconnected and try to reopen by instance id before falling back to slot.
- **File failure messages**: Battery saves, palette writes, ROM header writes, and snapshots now report create/read/write failures instead of silently continuing.
- **Quiet default diagnostics**: Startup, GUI, input, ROM header, mapper, CPU reset, and mixer-mode details now print only with `--dev-log` / `--diagnostics`; failure messages still print normally.
- **ROM dialog path handling**: The retro loader now strips extensions only from the filename portion, shows clipped long entries with `...`, and keeps command-line run/restore flows from advancing after a failed ROM load.
- **Gamepad deadzone tuning**: SDL3 gamepad inputs now persist a per-player deadzone value in the retro input dialog, and `--dev-log` reports duplicate NES button mappings.
- **GUI focus/click hardening**: Edit boxes, list boxes, and window body clicks now claim focus consistently, and destroyed modal/captured widgets clear stale mouse pointers.
- **Dump file failures**: RAM and ROM dump menu actions now report create/write failures instead of silently claiming success.
- **Image/font file hardening**: Bitmap and font helpers now fail cleanly on missing, unreadable, or unwritable files instead of dereferencing null or ignoring write failures.
- **Palette load hardening**: Short or unreadable palette files no longer partially overwrite the built-in fallback palette.
- **Save-state validation tests**: Added a small CMake regression harness for state header validation and safe `.sta` filename construction.
- **Config format validation tests**: Added CMake coverage for config version and file-size gates before loading settings blobs.

## [0.5.0] - 2026-03-17

### Added
- **Gamepad button remapping**: New "Redefine buttons..." dialog lets users remap NES B/A/Select/Start to any SDL GameController button. Accessible from the input device dialog when a gamepad is selected.
- **`padmap` struct**: Stores per-player gamepad button assignments, persisted in `inputdevicesettings`.
- **NES hardware nonlinear mixer**: Implements the actual NES DAC formulas (`pulse_out = 95.88/(8128/p+100)`, `tnd_out = 159.79/(1/(t/8227+n/12241+d/22638)+100)`) for hardware-accurate channel mixing. Togglable via `nessound::nonlinear_mixing`.
- **DC blocking high-pass filter**: Removes DC offset from the nonlinear mixer's unipolar [0,1] output, preserving only the AC audio signal (~7 Hz cutoff).
- **Mixer mode diagnostic**: Startup log confirms which mixer mode is active.

### Fixed
- **Input dialog buttons**: "Redefine keys..." and "Save settings" buttons no longer disappear or spawn outside the dialog window when changing input device. Buttons are created once and updated via `settext()`/`moveto()` instead of delete/recreate.
- **Gamepad button polling**: Switched from keyboard-triggered `keyhit()` to frame-based polling in `drawdata()` with `SDL_GameControllerUpdate()`.
- **Uninitialized `raw_vol`**: All `neschannel` state (`vol`, `raw_vol`, `count`, `K`, `freq`) now zero-initialized in `reset()`, preventing garbage noise output.

## [0.4.2] - 2026-03-17

### Fixed
- **Load button positioning**: Moved from title bar to bottom of dialog (absolute y-coord bug in GUIonebuttonbox/GUItwobuttonbox)
- **Filename text clipping**: Listbox text truncated to fit within scrollbar bounds using `font->getwidth()`
- **NES Timing crash**: Guarded `GUIedit::draw` drawrect against negative width when prompt text exceeds edit control width at 640x480 fonts
- **NES Timing layout**: Widened dialog from 140x80 to 250x80, edit controls from xw=30 to xw=180
- **About dialog layout**: Widened from 290x65 to 500x90, text pushed to x=140 to clear buddy portrait, proper line spacing
- **Maximize button icon**: Now uses `guivol.wmmark` image resource
- **Escape from fullscreen**: Added keyhit handler to GUImaximizebox for Escape key restore
- **ROM loader width**: Increased content from 140px to 280px

## [0.4.1] - 2026-03-09

### Fixed
- **GUIVOL struct alignment**: Added missing `sfont` and 12 other fields to match original gui.vol entry order; all icons now display correctly
- **GUI constructor coordinates**: Fixed 5 widget constructors using relative coordinates where absolute were required
- **Button bar overlap**: GUIonebuttonbox/GUItwobuttonbox now extend box height by 18px for the bottom button bar
- **Popup menu crash**: Fixed use-after-free in GUIpopupmenu::domenuitem()

## [0.4.0] - 2026-03-09

### Added
- **True Fullscreen**: ALT+ENTER toggles between 640x480 windowed GUI and 256x224 native NES fullscreen
- **SDL Logical Scaling**: Aspect-ratio-correct scaling and letterboxing
- **Command-line ROM loading**: Fixed path handling for files with spaces

## [0.3.1] - 2026-03-08

### Fixed
- **SMB1 Freeze**: Fixed emulation freeze (Sprite 0 Hit CPU starvation)
- **Clock Synchronization**: Accurate cycle count synthesis from fake6502 to NESticle PPU
- **Clear-On-Read**: $2002 VBlank flag properly cleared on read

## [0.3.0] - 2026-03-07

### Fixed
- **Sprite Flipping**: Repaired 8-bit signed integer casts for flipx/flipy
- **Palette Bleeding**: Masked attrib byte with `& 3`
- **Noise Channel timing**: LFSR duty/volume fixes
- **Input System**: Restored standard keyboard scanning via SDL2

### Added
- Animated cursor rendering via ARGB framebuffer overlay

### Discovered
- Complete missing source code recovered from SOUND.ZIP archive
