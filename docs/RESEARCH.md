# Backend research and decision

This records why PS5 Keyboard Bridge talks to the PS5 the way it does, what was checked,
and which limits are known. It is the reference for later phases.

## Requirement

Mouse and keyboard should drive the PS5 while Veyra NRVideo keeps showing the Ripsaw HDMI
capture on the PC. The app must provide input only and must not replace or re-implement the
video path.

## Options evaluated

| Option | Verdict | Reason |
|---|---|---|
| Veyra NRVideo controller forwarding | Rejected | Veyra forwards only SDL gamepads and sets `enable_keyboard=false`. There is no mouse/keyboard path to reuse. Using it would need a virtual gamepad driver, which the spec forbids unless proven necessary. |
| Virtual gamepad (ViGEmBus / vgamepad) | Rejected | ViGEmBus is retired, needs a kernel driver, and a virtual Xbox/DualShock pad still cannot reach the PS5 without a Remote Play client anyway. |
| keyboard-ds5 (Python) | Rejected as a base | Depends on a Python runtime and downloads a key table at runtime, which conflicts with the no-Python, no-download requirement. Its GPL/AGPL obligations also need the same review as chiaki-ng. |
| PS5-DirectView (C++) | Reused as a reference and as the vendored source of chiaki-ng | Already ships a Windows C++ input-only Remote Play path and a vendored, MSVC-buildable chiaki-ng core. |
| chiaki-ng (`lib/`) | Adopted as the protocol core | Implements discovery, wake, registration (pairing), the Remote Play session and controller-state transport. Used input-only: 360p preset, A/V discarded, `enable_dualsense=false`, `enable_keyboard=false`. |

Chosen path: a standalone input-only Remote Play session from this app to the PS5 over the LAN,
with Veyra left in HDMI capture mode. Only one Remote Play session can normally be active, so the
app detects the PS5 being busy and reports it instead of opening a second session.

## What was verified

- Discovery on the real PS5 (non-invasive UDP probe, no connection): the console answers at
  the LAN address and reports ready state and firmware. Discovery is the only live PS5 interaction
  in this phase.
- Pairing and controller input were not run against the real PS5 yet. They need the user's
  8-digit PIN and PSN account ID (see the hardware test list in the final report).

## Technical blockers and limits

- Sony's Remote Play protocol is not published. chiaki-ng reverse-engineers it. A firmware update
  can break compatibility; the vendored code is pinned to a known commit (see
  `third_party/README.md`).
- The PS5 stream must still be negotiated, so A/V packets arrive and are discarded. This costs
  some bandwidth but adds no decode work.
- PSN sign-in through Sony's browser flow needs an OAuth client ID that is not shipped with this
  project. v1 asks for the PSN account ID manually. The app never asks for a PSN password.
- If the PS5 user has a login passcode, the protocol cannot complete. The app reports this as a
  fatal, actionable error instead of retrying.
- Mouse-to-stick translation is an analog emulation. The game's own stick acceleration and
  turn-rate cap limit how fast the camera can turn. The app cannot change those.

## Licences

- chiaki-ng is AGPL-3.0-only with an OpenSSL exception. Because the combined work links it, the
  project itself is licensed AGPL-3.0 (`LICENSE`).
- Vendored dependencies and their licences are listed in `THIRD_PARTY_NOTICES.md` and
  `third_party/README.md`.
- No Sony code, PS5 firmware, or proprietary assets are included.

## Not used in this project

- No Veyra binaries or configuration are modified.
- No kernel driver, no virtual gamepad, no global hook left running unattended.
- No anti-cheat bypass, macros, rapid-fire, recoil compensation, or aim assistance.
