#pragma once

namespace pskb {

// True when at least one adapter is up with a non-loopback IPv4 address that is not a
// link-local (169.254.x.x) fallback. Returns true when the adapter list cannot be read, so a
// broken query never blocks a connect attempt that might still succeed.
bool HasUsableNetwork();

}  // namespace pskb
