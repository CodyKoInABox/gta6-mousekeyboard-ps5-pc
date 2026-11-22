#include "app/saved_pairing.h"

#include <filesystem>
#include <system_error>

#include "core/app_paths.h"
#include "core/secure_store.h"

namespace pskb {

std::optional<PairingInfo> LoadSavedPairing() {
    const auto blob = secure_store::LoadFile(PairingFilePath());
    if (!blob) return std::nullopt;
    return PairingInfo::Deserialize(*blob);
}

bool SaveSavedPairing(const PairingInfo& pairing) {
    return secure_store::SaveFile(PairingFilePath(), pairing.Serialize());
}

bool DeleteSavedPairing() {
    std::error_code ec;
    return std::filesystem::remove(PairingFilePath(), ec);
}

}  // namespace pskb
