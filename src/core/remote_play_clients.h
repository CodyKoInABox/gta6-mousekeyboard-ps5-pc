#pragma once

#include <string_view>

namespace pskb {

// Desktop programs that open their own Remote Play session to the PS5. The PS5 accepts one
// session at a time, so a running client makes our connection fail with "in use".
bool IsRemotePlayClientExe(std::wstring_view imagePath) noexcept;

}  // namespace pskb
