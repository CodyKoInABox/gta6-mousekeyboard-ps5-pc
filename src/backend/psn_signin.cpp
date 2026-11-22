#include "backend/psn_signin.h"

#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>

#include <cstddef>
#include <optional>
#include <string>

#include "core/psn_login.h"

#pragma comment(lib, "winhttp.lib")

namespace pskb::psn_signin {
namespace {

constexpr wchar_t kHost[] = L"auth.api.sonyentertainmentnetwork.com";
constexpr wchar_t kTokenPath[] = L"/2.0/oauth/token";
constexpr std::size_t kMaxReplyBytes = 64 * 1024;

struct Reply {
    bool transport = false;
    DWORD status = 0;
    std::string body;
};

std::wstring ToWide(std::string_view text) {
    if (text.empty()) return {};
    const int len = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring out(static_cast<std::size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), len);
    return out;
}

void Wipe(std::string& text) {
    if (!text.empty()) SecureZeroMemory(text.data(), text.size());
    text.clear();
}

bool IsTokenSafe(std::string_view token) {
    if (token.empty() || token.size() > 4096) return false;
    for (char c : token) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                        c == '-' || c == '_' || c == '.' || c == '~';
        if (!ok) return false;
    }
    return true;
}

// Plain HTTPS request to Sony's auth host. Returns the status and body, or transport == false
// when the connection itself failed. Bodies are capped so a hostile reply cannot exhaust memory.
Reply Send(const wchar_t* verb, const std::wstring& path, const std::wstring& headers, const std::string& body) {
    Reply reply;
    HINTERNET session = WinHttpOpen(L"PS5KeyboardBridge/0.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return reply;
    WinHttpSetTimeouts(session, 10000, 10000, 15000, 15000);

    HINTERNET connection = WinHttpConnect(session, kHost, INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET request = connection
                            ? WinHttpOpenRequest(connection, verb, path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                                 WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)
                            : nullptr;
    if (request) {
        const DWORD bodySize = static_cast<DWORD>(body.size());
        const bool sent = WinHttpSendRequest(request, headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(),
                                             headers.empty() ? 0 : static_cast<DWORD>(headers.size()),
                                             bodySize ? const_cast<char*>(body.data()) : nullptr, bodySize, bodySize,
                                             0) == TRUE;
        if (sent && WinHttpReceiveResponse(request, nullptr)) {
            reply.transport = true;
            DWORD statusSize = sizeof(reply.status);
            WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                                &reply.status, &statusSize, WINHTTP_NO_HEADER_INDEX);
            for (;;) {
                DWORD available = 0;
                if (!WinHttpQueryDataAvailable(request, &available) || available == 0) break;
                if (reply.body.size() + available > kMaxReplyBytes) {
                    reply.transport = false;
                    break;
                }
                std::string chunk(available, '\0');
                DWORD read = 0;
                if (!WinHttpReadData(request, chunk.data(), available, &read)) {
                    reply.transport = false;
                    break;
                }
                reply.body.append(chunk.data(), read);
                SecureZeroMemory(chunk.data(), chunk.size());
            }
        }
    }

    if (request) WinHttpCloseHandle(request);
    if (connection) WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return reply;
}

std::wstring BasicAuthHeader() {
    const std::string pair = std::string(psn_login::kClientId) + ":" + std::string(psn_login::kClientSecret);
    return L"Authorization: Basic " + ToWide(psn_login::Base64Encode(pair)) + L"\r\n";
}

SignInResult Fail(std::string message) {
    SignInResult out;
    out.message = std::move(message);
    return out;
}

}  // namespace

bool OpenSignInPage() {
    const std::wstring url = ToWide(psn_login::SignInUrl());
    const auto code = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    return code > 32;
}

SignInResult CompleteSignIn(std::string_view pastedUrl) {
    auto code = psn_login::ExtractAuthCode(pastedUrl);
    if (!code) {
        return Fail("Não encontrei o código na URL. Cole a URL completa da página que mostra \"redirect\" "
                    "(ela começa com https://remoteplay.dl.playstation.net).");
    }

    const std::wstring tokenHeaders =
        L"Content-Type: application/x-www-form-urlencoded\r\n" + BasicAuthHeader();
    std::string tokenBody = psn_login::TokenRequestBody(*code);
    Wipe(*code);
    Reply tokenReply = Send(L"POST", kTokenPath, tokenHeaders, tokenBody);
    Wipe(tokenBody);
    if (!tokenReply.transport) {
        Wipe(tokenReply.body);
        return Fail("Sem conexão com a Sony. Verifique a internet e tente de novo.");
    }
    if (tokenReply.status != 200) {
        Wipe(tokenReply.body);
        return Fail("A Sony não aceitou o login. O código expira em poucos minutos: rode o login de novo.");
    }

    auto accessToken = psn_login::JsonScalarField(tokenReply.body, "access_token");
    Wipe(tokenReply.body);
    if (!accessToken || !IsTokenSafe(*accessToken)) {
        return Fail("A resposta da Sony veio em um formato inesperado. Tente de novo.");
    }

    Reply accountReply = Send(L"GET", std::wstring(kTokenPath) + L"/" + ToWide(*accessToken), BasicAuthHeader(), {});
    Wipe(*accessToken);
    if (!accountReply.transport || accountReply.status != 200) {
        Wipe(accountReply.body);
        return Fail("Não consegui ler sua conta na Sony. Tente de novo em instantes.");
    }

    const auto userId = psn_login::JsonScalarField(accountReply.body, "user_id");
    Wipe(accountReply.body);
    const auto account = userId ? ParseAccountId(*userId) : std::nullopt;
    if (!account) {
        return Fail("A Sony não devolveu um ID de conta válido. Tente de novo.");
    }

    SignInResult out;
    out.ok = true;
    out.account = *account;
    out.message = "Conta Sony confirmada.";
    return out;
}

}  // namespace pskb::psn_signin
