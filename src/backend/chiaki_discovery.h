#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct chiaki_log_t;

namespace pskb::chiaki_runtime {

// One process-wide chiaki library initialisation and log. Safe to call repeatedly.
chiaki_log_t* Log();

// Most recent protocol log lines at INFO level and above (bounded). Debug output is never
// captured, so registration and session secrets do not end up in this buffer.
std::vector<std::string> RecentLog();

}  // namespace pskb::chiaki_runtime

namespace pskb {

struct ConsoleInfo {
    std::string host;
    std::string name;
    std::string id;        // discovery host id
    std::string firmware;
    bool isPs5 = false;
    bool ready = false;    // false while in rest mode
};

// Sends discovery pings and collects replies until `done` returns true or the timeout passes.
// An empty host broadcasts on every local IPv4 subnet (LAN only, no admin rights needed).
std::vector<ConsoleInfo> ProbeConsoles(const std::string& host, unsigned int timeoutMs,
                                       const std::function<bool(const std::vector<ConsoleInfo>&)>& done = {});

// Wake-on-LAN style request to a console in rest mode. The credential is the registration key.
bool WakeConsole(const std::string& host, std::uint64_t credential);

}  // namespace pskb
