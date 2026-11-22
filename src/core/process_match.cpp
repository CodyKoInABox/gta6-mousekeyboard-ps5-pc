#include "core/process_match.h"

#include <cwctype>
#include <string>

namespace pskb {
namespace {

std::wstring_view FileName(std::wstring_view path) noexcept {
    const auto slash = path.find_last_of(L"\\/");
    return slash == std::wstring_view::npos ? path : path.substr(slash + 1);
}

std::wstring_view StripExe(std::wstring_view name) noexcept {
    constexpr std::wstring_view kExe = L".exe";
    if (name.size() < kExe.size()) return name;
    const auto tail = name.substr(name.size() - kExe.size());
    for (std::size_t i = 0; i < kExe.size(); ++i) {
        if (std::towlower(tail[i]) != kExe[i]) return name;
    }
    return name.substr(0, name.size() - kExe.size());
}

bool EqualsIgnoreCase(std::wstring_view a, std::wstring_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::towlower(a[i]) != std::towlower(b[i])) return false;
    }
    return true;
}

}  // namespace

bool ProcessNameMatches(std::wstring_view imagePath, std::wstring_view pattern) noexcept {
    const auto patternName = StripExe(FileName(pattern));
    if (patternName.empty()) return false;
    return EqualsIgnoreCase(StripExe(FileName(imagePath)), patternName);
}

}  // namespace pskb
