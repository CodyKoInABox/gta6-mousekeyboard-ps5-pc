#pragma once

#include <optional>

#include "core/pairing.h"

namespace pskb {

// The one saved PS5 pairing, DPAPI-protected. Reads are cheap enough for the link worker to call.
std::optional<PairingInfo> LoadSavedPairing();
bool SaveSavedPairing(const PairingInfo& pairing);
bool DeleteSavedPairing();

}  // namespace pskb
