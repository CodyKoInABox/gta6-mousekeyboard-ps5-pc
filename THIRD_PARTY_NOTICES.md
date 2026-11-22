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

## Source availability

The complete corresponding source of this project, including the vendored components above,
is in this repository. Build instructions are in `README.md`.

## Not included

- Sony PlayStation software, firmware, or assets.
- Veyra NRVideo binaries or code. Veyra is a separate application the user installs.
- keyboard-ds5 code (reviewed, not reused; see `docs/RESEARCH.md`).
