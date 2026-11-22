#include "core/pairing.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace pskb {
namespace {

constexpr std::uint32_t kMagic = 0x31425350;  // "PSB1" in little-endian
constexpr std::uint32_t kVersion = 1;
constexpr std::size_t kMaxString = 256;

class Writer {
public:
    void Bytes(const void* data, std::size_t n) {
        const auto* p = static_cast<const std::uint8_t*>(data);
        out_.insert(out_.end(), p, p + n);
    }
    void U32(std::uint32_t v) { Bytes(&v, sizeof(v)); }
    void Str(const std::string& s) {
        const auto n = static_cast<std::uint16_t>(std::min(s.size(), kMaxString));
        Bytes(&n, sizeof(n));
        Bytes(s.data(), n);
    }
    std::vector<std::uint8_t> Take() { return std::move(out_); }

private:
    std::vector<std::uint8_t> out_;
};

class Reader {
public:
    explicit Reader(const std::vector<std::uint8_t>& in) : in_(in) {}
    bool Bytes(void* dst, std::size_t n) {
        if (n > in_.size() - pos_) return false;
        std::memcpy(dst, in_.data() + pos_, n);
        pos_ += n;
        return true;
    }
    bool U32(std::uint32_t& v) { return Bytes(&v, sizeof(v)); }
    bool Str(std::string& s) {
        std::uint16_t n = 0;
        if (!Bytes(&n, sizeof(n)) || n > kMaxString || n > in_.size() - pos_) return false;
        s.assign(reinterpret_cast<const char*>(in_.data() + pos_), n);
        pos_ += n;
        return true;
    }

private:
    const std::vector<std::uint8_t>& in_;
    std::size_t pos_ = 0;
};

}  // namespace

std::uint64_t PairingInfo::WakeCredential() const {
    const std::size_t len = strnlen(registKey.data(), registKey.size());
    if (len == 0 || len > 16) return 0;
    std::string hex(registKey.data(), len);
    return std::strtoull(hex.c_str(), nullptr, 16);
}

bool PairingInfo::IsComplete() const {
    const bool keyPresent = registKey[0] != '\0';
    const bool morningPresent = std::any_of(morning.begin(), morning.end(), [](std::uint8_t b) { return b != 0; });
    return keyPresent && morningPresent && !host.empty() && target != 0;
}

std::vector<std::uint8_t> PairingInfo::Serialize() const {
    Writer w;
    w.U32(kMagic);
    w.U32(kVersion);
    w.U32(target);
    w.Bytes(registKey.data(), registKey.size());
    w.Bytes(morning.data(), morning.size());
    w.Str(host);
    w.Str(name);
    w.Str(id);
    return w.Take();
}

std::optional<PairingInfo> PairingInfo::Deserialize(const std::vector<std::uint8_t>& blob) {
    Reader r(blob);
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    if (!r.U32(magic) || magic != kMagic) return std::nullopt;
    if (!r.U32(version) || version != kVersion) return std::nullopt;

    PairingInfo p;
    if (!r.U32(p.target) || !r.Bytes(p.registKey.data(), p.registKey.size()) ||
        !r.Bytes(p.morning.data(), p.morning.size()) || !r.Str(p.host) || !r.Str(p.name) ||
        !r.Str(p.id))
        return std::nullopt;
    if (!p.IsComplete()) return std::nullopt;
    return p;
}

}  // namespace pskb
