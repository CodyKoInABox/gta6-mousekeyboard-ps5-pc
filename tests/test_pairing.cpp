#include <cstring>

#include "core/pairing.h"
#include "test_framework.h"

using namespace pskb;

namespace {

PairingInfo SamplePairing() {
    PairingInfo p;
    p.target = 1100;
    std::memcpy(p.registKey.data(), "0123456789abcdef", 16);
    for (std::size_t i = 0; i < p.morning.size(); ++i) p.morning[i] = static_cast<std::uint8_t>(i + 1);
    p.host = "192.168.15.40";
    p.name = "PS5-Sala";
    p.id = "AA:BB:CC:DD:EE:FF";
    return p;
}

}  // namespace

PSKB_TEST(PairingRoundTripsThroughSerialization) {
    const PairingInfo original = SamplePairing();
    const auto blob = original.Serialize();
    const auto parsed = PairingInfo::Deserialize(blob);
    CHECK(parsed.has_value());
    CHECK_EQ(parsed->target, original.target);
    CHECK(std::memcmp(parsed->registKey.data(), original.registKey.data(), 16) == 0);
    CHECK(parsed->morning == original.morning);
    CHECK_EQ(parsed->host, original.host);
    CHECK_EQ(parsed->name, original.name);
    CHECK_EQ(parsed->id, original.id);
}

PSKB_TEST(PairingRejectsWrongMagic) {
    auto blob = SamplePairing().Serialize();
    blob[0] ^= 0xFF;
    CHECK(!PairingInfo::Deserialize(blob).has_value());
}

PSKB_TEST(PairingRejectsWrongVersion) {
    auto blob = SamplePairing().Serialize();
    blob[4] = 99;
    CHECK(!PairingInfo::Deserialize(blob).has_value());
}

PSKB_TEST(PairingRejectsTruncatedBlobs) {
    const auto blob = SamplePairing().Serialize();
    for (std::size_t cut : {std::size_t{0}, std::size_t{7}, std::size_t{20}, blob.size() - 1}) {
        std::vector<std::uint8_t> shortBlob(blob.begin(), blob.begin() + static_cast<std::ptrdiff_t>(cut));
        CHECK(!PairingInfo::Deserialize(shortBlob).has_value());
    }
}

PSKB_TEST(PairingRejectsIncompleteSecrets) {
    PairingInfo p = SamplePairing();
    p.registKey[0] = '\0';
    CHECK(!PairingInfo::Deserialize(p.Serialize()).has_value());

    p = SamplePairing();
    p.morning.fill(0);
    CHECK(!PairingInfo::Deserialize(p.Serialize()).has_value());

    p = SamplePairing();
    p.host.clear();
    CHECK(!PairingInfo::Deserialize(p.Serialize()).has_value());
}

PSKB_TEST(WakeCredentialIsRegistKeyHexAsNumber) {
    const PairingInfo p = SamplePairing();
    CHECK_EQ(p.WakeCredential(), static_cast<std::uint64_t>(0x0123456789abcdefULL));
}

PSKB_TEST(WakeCredentialIsZeroForEmptyKey) {
    PairingInfo p;
    CHECK_EQ(p.WakeCredential(), static_cast<std::uint64_t>(0));
}