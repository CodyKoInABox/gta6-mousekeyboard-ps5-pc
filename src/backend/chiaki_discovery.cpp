#include "backend/chiaki_discovery.h"

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#include <chrono>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>

#include "chiaki/discoveryservice.h"
#include "chiaki/log.h"
#include "chiaki/common.h"

#pragma comment(lib, "iphlpapi.lib")

namespace pskb {
namespace {

constexpr std::size_t kMaxRecentLines = 200;

std::mutex& LogMutex() {
    static std::mutex m;
    return m;
}

std::deque<std::string>& LogLines() {
    static std::deque<std::string> lines;
    return lines;
}

void LogSink(ChiakiLogLevel level, const char* msg, void* /*user*/) {
    const char tag = chiaki_log_level_char(level);
    std::lock_guard lock(LogMutex());
    auto& lines = LogLines();
    lines.push_back(std::string(1, tag) + " " + (msg ? msg : ""));
    if (lines.size() > kMaxRecentLines) lines.pop_front();
}

struct ProbeContext {
    std::mutex mutex;
    std::vector<ConsoleInfo> found;
};

void DiscoveryCallback(ChiakiDiscoveryHost* hosts, size_t count, void* user) {
    auto* ctx = static_cast<ProbeContext*>(user);
    std::lock_guard lock(ctx->mutex);
    ctx->found.clear();
    for (size_t i = 0; i < count; ++i) {
        const ChiakiDiscoveryHost& h = hosts[i];
        ConsoleInfo c;
        c.host = h.host_addr ? h.host_addr : "";
        c.name = h.host_name ? h.host_name : "";
        c.id = h.host_id ? h.host_id : "";
        c.firmware = h.system_version ? h.system_version : "";
        c.isPs5 = chiaki_discovery_host_is_ps5(const_cast<ChiakiDiscoveryHost*>(&h));
        c.ready = h.state == CHIAKI_DISCOVERY_HOST_STATE_READY;
        ctx->found.push_back(std::move(c));
    }
}

// Directed broadcast address of every up IPv4 adapter (for example 192.168.15.255).
std::vector<sockaddr_storage> DirectedBroadcasts() {
    std::vector<sockaddr_storage> out;
    ULONG size = 16 * 1024;
    std::vector<std::uint8_t> buffer(size);
    auto* list = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG rc = GetAdaptersAddresses(AF_INET, flags, nullptr, list, &size);
    if (rc == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(size);
        list = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        rc = GetAdaptersAddresses(AF_INET, flags, nullptr, list, &size);
    }
    if (rc != NO_ERROR) return out;

    for (auto* a = list; a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        for (auto* u = a->FirstUnicastAddress; u; u = u->Next) {
            if (u->Address.lpSockaddr->sa_family != AF_INET) continue;
            const UINT8 prefix = u->OnLinkPrefixLength;
            if (prefix == 0 || prefix >= 31) continue;
            sockaddr_in in = *reinterpret_cast<sockaddr_in*>(u->Address.lpSockaddr);
            const std::uint32_t ip = ntohl(in.sin_addr.s_addr);
            const std::uint32_t mask = ~((1u << (32 - prefix)) - 1u);
            in.sin_addr.s_addr = htonl(ip | ~mask);
            in.sin_port = 0;
            sockaddr_storage ss{};
            std::memcpy(&ss, &in, sizeof(in));
            out.push_back(ss);
        }
    }
    return out;
}

}  // namespace

namespace chiaki_runtime {

chiaki_log_t* Log() {
    static chiaki_log_t log{};
    static const bool initialised = [] {
        chiaki_log_init(&log, CHIAKI_LOG_ERROR | CHIAKI_LOG_WARNING | CHIAKI_LOG_INFO, &LogSink, nullptr);
        chiaki_lib_init();
        return true;
    }();
    (void)initialised;
    return &log;
}

std::vector<std::string> RecentLog() {
    std::lock_guard lock(LogMutex());
    return {LogLines().begin(), LogLines().end()};
}

}  // namespace chiaki_runtime

std::vector<ConsoleInfo> ProbeConsoles(const std::string& host, unsigned int timeoutMs,
                                       const std::function<bool(const std::vector<ConsoleInfo>&)>& done) {
    ProbeContext ctx;

    sockaddr_storage send{};
    auto* sin = reinterpret_cast<sockaddr_in*>(&send);
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = INADDR_BROADCAST;

    std::vector<sockaddr_storage> broadcasts;
    std::string hostCopy = host;

    ChiakiDiscoveryServiceOptions options{};
    options.hosts_max = 16;
    options.host_drop_pings = 3;
    options.ping_ms = 500;
    options.ping_initial_ms = 0;
    options.send_addr = &send;
    options.send_addr_size = sizeof(sockaddr_in);
    options.cb = &DiscoveryCallback;
    options.cb_user = &ctx;
    if (!host.empty()) {
        options.send_host = hostCopy.data();
    } else {
        broadcasts = DirectedBroadcasts();
        options.broadcast_addrs = broadcasts.empty() ? nullptr : broadcasts.data();
        options.broadcast_num = broadcasts.size();
    }

    ChiakiDiscoveryService service{};
    if (chiaki_discovery_service_init(&service, &options, chiaki_runtime::Log()) != CHIAKI_ERR_SUCCESS)
        return {};

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    std::vector<ConsoleInfo> snapshot;
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard lock(ctx.mutex);
            snapshot = ctx.found;
        }
        if (done && done(snapshot)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    chiaki_discovery_service_fini(&service);

    std::lock_guard lock(ctx.mutex);
    return ctx.found;
}

bool WakeConsole(const std::string& host, std::uint64_t credential) {
    if (host.empty()) return false;
    std::string hostCopy = host;
    return chiaki_discovery_wakeup(chiaki_runtime::Log(), nullptr, hostCopy.c_str(), credential, true) ==
           CHIAKI_ERR_SUCCESS;
}

}  // namespace pskb
