#pragma once

#include <string>
#include <vector>

namespace pskb {

// File names (e.g. "PS Remote Play.exe") of running programs that hold a Remote Play session.
std::vector<std::wstring> RunningRemotePlayClients();

}  // namespace pskb
