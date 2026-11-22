#pragma once

#include <string_view>

namespace pskb {

// True when the executable's file name matches the pattern, ignoring case and a ".exe"
// extension. Full paths are accepted; only the file name is compared. "Veyra" matches
// "C:\Program Files\Veyra\veyra.exe". An empty pattern never matches.
bool ProcessNameMatches(std::wstring_view imagePath, std::wstring_view pattern) noexcept;

}  // namespace pskb
