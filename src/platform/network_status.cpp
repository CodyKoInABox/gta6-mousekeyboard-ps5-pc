#include "platform/network_status.h"

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <vector>

namespace pskb {

bool HasUsableNetwork() {
    ULONG size = 15 * 1024;
    std::vector<unsigned char> buffer;
    ULONG status = ERROR_BUFFER_OVERFLOW;
    for (int tries = 0; tries < 3 && status == ERROR_BUFFER_OVERFLOW; ++tries) {
        buffer.resize(size);
        status = GetAdaptersAddresses(AF_INET,
                                      GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                                      nullptr, reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data()), &size);
    }
    if (status != NO_ERROR) return true;

    for (auto* adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data()); adapter; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        for (auto* unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next) {
            if (!unicast->Address.lpSockaddr || unicast->Address.lpSockaddr->sa_family != AF_INET) continue;
            const auto* addr = reinterpret_cast<const sockaddr_in*>(unicast->Address.lpSockaddr);
            const std::uint32_t host = ntohl(addr->sin_addr.S_un.S_addr);
            const bool linkLocal = (host & 0xFFFF0000u) == 0xA9FE0000u;
            if (host != 0 && !linkLocal) return true;
        }
    }
    return false;
}

}  // namespace pskb
