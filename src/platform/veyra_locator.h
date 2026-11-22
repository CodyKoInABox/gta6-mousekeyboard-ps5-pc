#pragma once

#include <optional>
#include <string>

namespace pskb {

// Finds Veyra.exe: a running Veyra first, then a shallow search of the usual install and
// download folders. Never reads or writes Veyra's own configuration.
std::optional<std::wstring> FindVeyraExecutable();

// Brings a visible Veyra window to the front, or starts Veyra from exePath when none is open.
// Returns false when neither works.
bool LaunchOrFocusVeyra(const std::wstring& exePath);

}  // namespace pskb
