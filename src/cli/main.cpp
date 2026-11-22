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
#include "backend/psn_signin.h"
#include "core/account_id.h"
#include "core/app_paths.h"
#include "core/bindings.h"
#include "core/link_manager.h"
#include "core/pairing.h"
#include "core/psn_login.h"
#include "core/secure_store.h"

namespace {

using namespace pskb;

std::string Trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string ReadLine(const char* prompt) {
    std::printf("%s", prompt);
    std::fflush(stdout);
    std::string line;
    std::getline(std::cin, line);
    return Trim(line);
}

std::optional<AccountId> RunSignIn() {
    std::printf(
        "\nLogin na Sony (pelo navegador)\n"
        "  1. Pressione Enter: a página de login oficial da Sony abre no seu navegador.\n"
        "  2. Entre com a mesma conta PSN que você usa neste PS5. A senha fica só no site da Sony.\n"
        "  3. Quando a página mostrar \"redirect\", copie a URL inteira da barra de endereços e cole aqui.\n\n");
    ReadLine("Pressione Enter para abrir o navegador... ");
    if (!psn_signin::OpenSignInPage()) {
        std::printf("Não consegui abrir o navegador. Abra este endereço manualmente:\n%s\n",
                    psn_login::SignInUrl().c_str());
    }

    std::string pasted = ReadLine("URL colada (redirect): ");
    const auto result = psn_signin::CompleteSignIn(pasted);
    if (!pasted.empty()) SecureZeroMemory(pasted.data(), pasted.size());
    std::printf("%s\n", result.message.c_str());
    if (!result.ok) return std::nullopt;

    std::printf("ID da conta PSN: %s\n", psn_login::AccountIdToBase64(result.account).c_str());
    return result.account;
}

int CmdPsnLogin() {
    return RunSignIn() ? 0 : 1;
}

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
        "  psn-login                       sign in to Sony in the browser and show your account ID\n"
        "  pair [--host IP] [--account ID] one-time pairing (asks for the PS5 code)\n"
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
    std::string host = Arg(args, "--host");
    if (host.empty()) {
        std::printf("Procurando o PS5 na rede...\n");
        const auto found = ProbeConsoles("", 4000, nullptr);
        std::vector<ConsoleInfo> ps5s;
        for (const auto& c : found) {
            if (c.isPs5) ps5s.push_back(c);
        }
        if (ps5s.empty()) {
            std::printf("Nenhum PS5 respondeu. Ligue o PS5 (ou deixe-o em repouso com \"Ligar pela rede\") e tente de novo.\n");
            return 1;
        }
        if (ps5s.size() > 1) {
            std::printf("Encontrei mais de um PS5. Use --host com o IP correto:\n");
            for (const auto& c : ps5s) std::printf("  %s  %s\n", c.host.c_str(), c.name.c_str());
            return 2;
        }
        host = ps5s.front().host;
        std::printf("PS5 encontrado: %s em %s\n", ps5s.front().name.c_str(), host.c_str());
    }

    std::optional<AccountId> account;
    if (const std::string accountText = Arg(args, "--account"); !accountText.empty()) {
        account = ParseAccountId(accountText);
        if (!account) {
            std::printf("ID da conta inválido. Use o número ou o código de 12 caracteres.\n");
            return 2;
        }
    } else {
        account = RunSignIn();
        if (!account) return 2;
    }

    std::printf(
        "\nAgora, no PS5: Configurações > Sistema > Remote Play > Link Device.\n"
        "A tela mostra um código de 8 dígitos que vale por alguns minutos.\n");
    const std::string pin = ReadLine("Código de 8 dígitos do PS5: ");

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
    if (cmd == "psn-login") return CmdPsnLogin();
    if (cmd == "pair") return CmdPair(rest);
    if (cmd == "connect") return CmdConnect(rest);
    if (cmd == "forget") return CmdForget();
    if (cmd == "bindings") return CmdBindings();
    if (cmd == "log") return CmdLog();
    PrintUsage();
    return 2;
}
