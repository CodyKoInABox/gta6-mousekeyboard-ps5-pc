// ps5kb-cli: command-line tool for setup checks and hardware tests of the Remote Play core.
// The GUI is built on the same pieces; this exists so each step can be verified on its own.

#include <windows.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "backend/chiaki_backend.h"
#include "backend/chiaki_discovery.h"
#include "core/account_id.h"
#include "core/app_paths.h"
#include "core/bindings.h"
#include "core/link_manager.h"
#include "core/pairing.h"
#include "core/secure_store.h"

namespace {

using namespace pskb;

std::string Arg(const std::vector<std::string>& args, const std::string& name) {
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == name) return args[i + 1];
    }
    return {};
}

bool HasFlag(const std::vector<std::string>& args, const std::string& name) {
    for (const auto& a : args) {
        if (a == name) return true;
    }
    return false;
}

std::optional<PairingInfo> LoadPairing() {
    const auto blob = secure_store::LoadFile(PairingFilePath());
    if (!blob) return std::nullopt;
    return PairingInfo::Deserialize(*blob);
}

void PrintUsage() {
    std::printf(
        "ps5kb-cli <command>\n"
        "  status                          show saved pairing (no secrets)\n"
        "  discover [--host IP] [--seconds N]  find PS5 consoles on the LAN (no pairing)\n"
        "  pair --host IP [--account ID] [--pin 12345678]  one-time pairing\n"
        "  connect [--seconds N] [--test-buttons]  input-only session with saved pairing\n"
        "  forget                          delete the saved pairing\n"
        "  bindings                        print the default GTA 6 mapping\n"
        "  log                             print recent protocol log lines\n");
}

int CmdStatus() {
    const auto pairing = LoadPairing();
    if (!pairing) {
        std::printf("Nenhum pareamento salvo.\n");
        return 1;
    }
    std::printf("Pareado com: %s\nÚltimo IP conhecido: %s\nArquivo: %ls\n", pairing->name.c_str(),
                pairing->host.c_str(), PairingFilePath().c_str());
    return 0;
}

int CmdDiscover(const std::vector<std::string>& args) {
    const std::string host = Arg(args, "--host");
    unsigned seconds = 5;
    if (const auto s = Arg(args, "--seconds"); !s.empty()) seconds = static_cast<unsigned>(std::stoul(s));
    const auto found = ProbeConsoles(host, seconds * 1000, nullptr);
    if (found.empty()) {
        std::printf("Nenhum console respondeu.\n");
        return 1;
    }
    for (const auto& c : found) {
        std::printf("%-16s %-8s %-8s %s  fw=%s\n", c.host.c_str(), c.isPs5 ? "PS5" : "outro",
                    c.ready ? "pronto" : "repouso", c.name.c_str(), c.firmware.c_str());
    }
    return 0;
}

int CmdPair(const std::vector<std::string>& args) {
    const std::string host = Arg(args, "--host");
    std::string accountText = Arg(args, "--account");
    std::string pin = Arg(args, "--pin");
    if (accountText.empty()) {
        std::printf("ID da conta PSN (base64 de 12 caracteres ou número): ");
        std::getline(std::cin, accountText);
    }
    if (pin.empty()) {
        std::printf("Código de 8 dígitos do PS5: ");
        std::getline(std::cin, pin);
    }
    const auto account = ParseAccountId(accountText);
    if (!account) {
        std::printf("ID da conta inválido.\n");
        return 2;
    }

    const auto result = chiaki_backend::RegisterConsole(host, *account, pin);
    std::printf("%s\n", result.message.c_str());
    if (!result.ok) return 1;

    if (!secure_store::SaveFile(PairingFilePath(), result.pairing.Serialize())) {
        std::printf("Não foi possível salvar o pareamento com proteção do Windows (DPAPI).\n");
        return 1;
    }
    std::printf("Pareamento salvo com proteção DPAPI em %ls\n", PairingFilePath().c_str());
    return 0;
}

int CmdConnect(const std::vector<std::string>& args) {
    const auto pairing = LoadPairing();
    if (!pairing) {
        std::printf("Nenhum pareamento salvo. Execute 'pair' primeiro.\n");
        return 2;
    }
    unsigned seconds = 20;
    if (const auto s = Arg(args, "--seconds"); !s.empty()) seconds = static_cast<unsigned>(std::stoul(s));
    const bool testButtons = HasFlag(args, "--test-buttons");

    ChiakiBackend backend;
    LinkManager link(backend, [] { return LoadPairing(); });
    link.SetStatusListener([](const LinkStatus& s) {
        std::printf("[estado] %s (%s) %s\n", LinkStateName(s.state), LinkErrorName(s.error), s.message.c_str());
    });
    link.Connect();

    const auto start = std::chrono::steady_clock::now();
    const auto end = start + std::chrono::seconds(seconds);
    bool wasConnected = false;
    while (std::chrono::steady_clock::now() < end) {
        const auto status = link.Status();
        if (status.state == LinkState::Failed) break;
        if (status.state == LinkState::Connected && !wasConnected) {
            wasConnected = true;
        }
        if (status.state == LinkState::Connected) {
            const auto t = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            PadState frame;
            if (testButtons) {
                const int phase = static_cast<int>(t * 2.0) % 4;
                const std::uint32_t pressed[] = {PadCross, PadCircle, PadSquare, PadTriangle};
                frame.buttons = pressed[phase];
                const double angle = t * 2.0;
                frame.lx = StickFromUnit(static_cast<float>(std::cos(angle)) * 0.8f);
                frame.ly = StickFromUnit(static_cast<float>(std::sin(angle)) * 0.8f);
            }
            link.SubmitPad(frame);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }

    link.Disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::printf("Sessão encerrada. Conectou: %s\n", wasConnected ? "sim" : "não");
    return wasConnected ? 0 : 1;
}

int CmdForget() {
    std::error_code ec;
    const bool removed = std::filesystem::remove(PairingFilePath(), ec);
    std::printf(removed ? "Pareamento removido.\n" : "Nenhum pareamento para remover.\n");
    return 0;
}

int CmdBindings() {
    std::printf("%s", SerializeBindings(GtaProfile()).c_str());
    return 0;
}

int CmdLog() {
    for (const auto& line : chiaki_runtime::RecentLog()) std::printf("%s\n", line.c_str());
    return 0;
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        const int len = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(static_cast<size_t>(len > 0 ? len - 1 : 0), '\0');
        if (len > 1) WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, utf8.data(), len, nullptr, nullptr);
        args.push_back(utf8);
    }
    SetConsoleOutputCP(CP_UTF8);
    if (args.empty()) {
        PrintUsage();
        return 2;
    }
    const std::string cmd = args[0];
    const std::vector<std::string> rest(args.begin() + 1, args.end());
    if (cmd == "status") return CmdStatus();
    if (cmd == "discover") return CmdDiscover(rest);
    if (cmd == "pair") return CmdPair(rest);
    if (cmd == "connect") return CmdConnect(rest);
    if (cmd == "forget") return CmdForget();
    if (cmd == "bindings") return CmdBindings();
    if (cmd == "log") return CmdLog();
    PrintUsage();
    return 2;
}
