# gta6-mousekeyboard-ps5-pc

Trying to make Keyboard and Mouse possible for GTA 6 on PS5 / PC (Veyra app).

PS5 Keyboard Bridge sends mouse and keyboard input to a PS5 over an input-only Remote Play
connection while Veyra NRVideo keeps showing the Ripsaw HDMI capture. It does not handle video.

Status: release candidate. See `docs/RESEARCH.md` for the backend decision.

## For players

- Setup guide (pt-BR): `docs/guia-configuracao.md`
- Troubleshooting (pt-BR): `docs/solucao-de-problemas.md`
- Real-hardware test checklist: `docs/checklist-hardware.md`

Download the installer or the portable zip from the release page. Each release lists SHA-256
checksums in `SHA256SUMS.txt`.

## Build (Windows 10/11 x64)

Requirements: CMake 3.23+ and Visual Studio 2022 (Desktop development with C++). Inno Setup 6 is
needed only for the installer.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
build\Release\pskb_tests.exe
```

Release artifacts (installer, portable zip, checksums, and test results) come from one script:

```
powershell -ExecutionPolicy Bypass -File scripts\build-release.ps1
```

Output goes to `dist\`. The build is reproducible: the linker runs with `/Brepro`, the zip has fixed
timestamps and sorted entries, and the script starts from a clean build directory. Building the
same commit twice gives the same bytes; `SHA256SUMS.txt` shows the hashes.

Targets:

- `PS5KeyboardBridge.exe`: the windowed app (setup wizard, main screen, tray).
- `ps5kb-cli.exe`: command-line tool (`status`, `discover`, `pair`, `connect`, `forget`, `bindings`, `log`, `veyra`).
- `pskb_tests.exe`: automated tests (no PS5 or network needed).

Vendored dependencies are in `third_party/`. Nothing is downloaded at build time.

## Licence

AGPL-3.0. See `LICENSE` and `THIRD_PARTY_NOTICES.md`.
