# gta6-mousekeyboard-ps5-pc

Trying to make Keyboard and Mouse possible for GTA 6 on PS5 / PC (Veyra app).

PS5 Keyboard Bridge sends mouse and keyboard input to a PS5 over an input-only Remote Play
connection while Veyra NRVideo keeps showing the Ripsaw HDMI capture. It does not handle video.

Status: phase 1 (backend core). See `docs/RESEARCH.md` for the backend decision.

## Build (Windows 10/11 x64)

Requirements: CMake 3.23+ and Visual Studio 2022 (Desktop development with C++).

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
build\Release\pskb_tests.exe
```

Targets:

- `ps5kb-cli.exe`: command-line tool (`status`, `discover`, `pair`, `connect`, `forget`, `bindings`, `log`).
- `pskb_tests.exe`: automated tests (no PS5 or network needed).

Vendored dependencies are in `third_party/`. Nothing is downloaded at build time.

## Licence

AGPL-3.0. See `LICENSE` and `THIRD_PARTY_NOTICES.md`.
