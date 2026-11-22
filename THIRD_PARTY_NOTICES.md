# Third-party notices

PS5 Keyboard Bridge is licensed under the GNU Affero General Public License v3.0
(see `LICENSE`). It includes and links the components below. Their full licence texts are
kept in `third_party/` and must stay with any copy of this project.

| Component | Use | Licence | Source | Notes |
|---|---|---|---|---|
| chiaki-ng (`lib/`) | PS5 discovery, wake, pairing, Remote Play session | AGPL-3.0-only with OpenSSL exception | <https://github.com/streetpea/chiaki-ng> (commit `43728af`) | `third_party/chiaki/COPYING`, `third_party/chiaki/LICENSES/` |
| PS5-DirectView | Reference for input-only Remote Play and the vendored chiaki-ng build | AGPL-3.0 | <https://github.com/babymonie/PS5-DirectView> (commit `c109fec`) | Source of `third_party/`; patches listed in `third_party/README.md` |
| mbed TLS 2.28.10 | Crypto used by Remote Play | Apache-2.0 | <https://github.com/Mbed-TLS/mbedtls> | `third_party/mbedtls/LICENSE` |
| nanopb runtime | Protobuf messages for Takion | zlib | <https://github.com/nanopb/nanopb> | `third_party/nanopb/LICENSE.txt` |
| Jerasure | Remote Play FEC | BSD-3-Clause | <https://github.com/streetpea/jerasure> | `third_party/jerasure/COPYING` |
| GF-Complete | Galois-field maths for Jerasure | BSD-3-Clause | <https://github.com/streetpea/gf-complete> | `third_party/gf-complete/COPYING` |

## Sony sign-in constants

`src/core/psn_login.h` contains the public Remote Play client ID and client secret that
chiaki-ng (`scripts/psn-account-id.py`, `gui/include/psnaccountid.h`) and the official Remote Play
client use for the Sony sign-in flow. They are copied from chiaki-ng, not issued to this project.
They identify the client to Sony and do not give access to any account on their own. A user's
password is only entered on Sony's page, never in this app.

## Source availability

The complete corresponding source of this project, including the vendored components above,
is in this repository. Build instructions are in `README.md`.

## Distributed installer and portable package

- The installer (`PS5KeyboardBridge-Setup-<versão>.exe`) is built with Inno Setup 6
  (<https://jrsoftware.org/isinfo.php>). The Inno Setup runtime is part of the installer and is
  free to redistribute. The Brazilian Portuguese messages come from Inno Setup's own language file.
- The application binaries link the Microsoft C++ runtime statically (`/MT`), so no separate
  Visual C++ redistributable is installed.
- Each release ships `LICENSE.txt` (AGPL-3.0), this file, and the setup guide and troubleshooting
  guide. The installer and portable zip are built from the tagged source with `scripts/build-release.ps1`.
- Source offer: the complete corresponding source for every binary is in this repository at the
  same version (`CMakeLists.txt` gives the version). Requests for the source of a release can be
  made through the repository's issues.
- `SHA256SUMS.txt` lists the SHA-256 of each published artifact.

## Not included

- Sony PlayStation software, firmware, or assets.
- Veyra NRVideo binaries or code. Veyra is a separate application the user installs.
- keyboard-ds5 code (reviewed, not reused; see `docs/RESEARCH.md`).
