#include "platform/remote_play_guard.h"

#include <filesystem>

#include "core/remote_play_clients.h"
#include "platform/process_list.h"

namespace pskb {

std::vector<std::wstring> RunningRemotePlayClients() {
    std::vector<std::wstring> names;
    for (const std::wstring& path : RunningProcessImagePaths()) {
        if (IsRemotePlayClientExe(path)) {
            names.push_back(std::filesystem::path(path).filename().wstring());
        }
    }
    return names;
}

}  // namespace pskb
