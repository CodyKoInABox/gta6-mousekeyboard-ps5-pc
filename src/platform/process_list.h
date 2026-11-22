#pragma once

#include <optional>
#include <string>
#include <vector>

namespace pskb {

// Full image paths of every process we can query. Processes we cannot open are skipped.
std::vector<std::wstring> RunningProcessImagePaths();

// Full image path of one process, or nullopt when it cannot be queried.
std::optional<std::wstring> ProcessImagePath(unsigned long pid);

}  // namespace pskb
