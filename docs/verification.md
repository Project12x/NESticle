# Manual Verification Checklist

Use this checklist before tagging or sharing a release package. Do not commit or bundle ROMs.

## Build And Package

- [ ] Configure: `cmake -B build-sdl3 -G "Visual Studio 17 2022"`
- [ ] Build: `cmake --build build-sdl3 --config Release`
- [ ] Package: `cmake --build build-sdl3 --config Release --target package_release`
- [ ] Confirm `releases/NESticle-sdl3-Release.zip` contains only:
  - `NESticle.exe`
  - `SDL3.dll`
  - `gui.vol`
  - `NESticle.pal`
  - `README.md`
  - `docs/verification.md`
  - `docs/save-states.md`
  - `LICENSE`
  - `SDL3-LICENSE.txt`
  - `SDL3-PROVENANCE.md`
  - `THIRD_PARTY_NOTICES.txt`

## Startup And Assets

- [ ] Launch from project root: `build-sdl3\Release\NESticle.exe`
- [ ] Launch from extracted package directory: `NESticle.exe`
- [ ] Confirm the retro GUI, cursor, menus, and file dialog are visible.
- [ ] Temporarily rename `gui.vol`, launch, and confirm failure is clean and understandable.
- [ ] Temporarily rename `NESticle.pal`, launch, and confirm palette failure is clean and understandable.

## ROM Compatibility Smoke

- [ ] Load a known mapper-0 ROM.
- [ ] Load at least one ROM using each supported mapper family when available.
- [ ] Confirm rendering starts and the retro UI remains usable.
- [ ] Confirm no bundled ROMs are present in the release zip.

## Input

- [ ] Keyboard: arrows, `Z`, `X`, `Tab`, and `Enter` map to NES controls.
- [ ] Toggle fullscreen with `ALT+ENTER`, then return to windowed mode.
- [ ] Gamepad: detect a connected SDL3 gamepad.
- [ ] Remap B/A/Select/Start in the retro dialog, save, restart, and confirm mappings persist.
- [ ] Disconnect/reconnect a gamepad and confirm the emulator does not crash.

## Audio And Timing

- [ ] Confirm audio starts without crackles during normal play.
- [ ] Pause, resume, focus another window, and return to the emulator.
- [ ] Confirm shutdown does not leave a hung process or crash dialog.

## Saves And Settings

- [ ] Save settings, restart, and confirm input mappings and window behavior persist.
- [ ] Save and load a state file for a running ROM.
- [ ] Try to load a deliberately invalid state file and confirm the running emulator remains stable.

## Notes

Record ROM names, mapper numbers, controller model, Windows version, and any stderr diagnostics in release notes or the changelog when a verification issue is found.
