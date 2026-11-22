#include "core/remote_play_clients.h"

#include <array>

#include "core/process_match.h"

namespace pskb {
namespace {

constexpr std::array<std::wstring_view, 4> kClientNames{
    L"PS Remote Play",
    L"RemotePlay",
    L"chiaki",
    L"chiaki-ng",
};

}  // namespace

bool IsRemotePlayClientExe(std::wstring_view imagePath) noexcept {
    for (const std::wstring_view name : kClientNames) {
        if (ProcessNameMatches(imagePath, name)) return true;
    }
    return false;
}

}  // namespace pskb
