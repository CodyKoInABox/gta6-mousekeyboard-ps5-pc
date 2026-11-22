#include "backend/chiaki_backend.h"

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <thread>

#include "backend/chiaki_discovery.h"
#include "chiaki/common.h"
#include "chiaki/controller.h"
#include "chiaki/regist.h"
#include "chiaki/session.h"

#include <condition_variable>
#include <mutex>

namespace pskb::chiaki_backend {

void ToControllerState(const PadState& pad, chiaki_controller_state_t& out) {
    chiaki_controller_state_set_idle(&out);
    out.buttons = pad.buttons;
    out.l2_state = pad.l2;
    out.r2_state = pad.r2;
    out.left_x = pad.lx;
    out.left_y = pad.ly;
    out.right_x = pad.rx;
    out.right_y = pad.ry;
}

LinkError ClassifyQuit(int reason) {
    switch (static_cast<ChiakiQuitReason>(reason)) {
        case CHIAKI_QUIT_REASON_STOPPED:
            return LinkError::Cancelled;
        case CHIAKI_QUIT_REASON_SESSION_REQUEST_CONNECTION_REFUSED:
        case CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_IN_USE:
        case CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_CRASH:
        case CHIAKI_QUIT_REASON_CTRL_CONNECTION_REFUSED:
        case CHIAKI_QUIT_REASON_CTRL_CONNECT_FAILED:
            return LinkError::Refused;
        case CHIAKI_QUIT_REASON_PSN_REGIST_FAILED:
            return LinkError::NotPaired;
        case CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_VERSION_MISMATCH:
        case CHIAKI_QUIT_REASON_CTRL_UNKNOWN:
        case CHIAKI_QUIT_REASON_STREAM_CONNECTION_UNKNOWN:
        case CHIAKI_QUIT_REASON_STREAM_CONNECTION_REMOTE_DISCONNECTED:
        case CHIAKI_QUIT_REASON_STREAM_CONNECTION_REMOTE_SHUTDOWN:
        case CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN:
            return LinkError::Protocol;
        case CHIAKI_QUIT_REASON_NONE:
            break;
    }
    return LinkError::Other;
}

std::string QuitMessage(int reason) {
    switch (static_cast<ChiakiQuitReason>(reason)) {
        case CHIAKI_QUIT_REASON_STOPPED:
            return "Desconectado.";
        case CHIAKI_QUIT_REASON_SESSION_REQUEST_CONNECTION_REFUSED:
            return "O PS5 recusou a conexão. Ele leva cerca de 10 segundos para aceitar uma nova sessão.";
        case CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_IN_USE:
            return "Outro aplicativo (PS Remote Play, Chiaki ou Veyra) já está conectado ao PS5. "
                   "Feche-o e tente de novo.";
        case CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_CRASH:
            return "O serviço Remote Play do PS5 falhou. Tente novamente em alguns segundos.";
        case CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_VERSION_MISMATCH:
            return "A versão do Remote Play do PS5 não é compatível com este aplicativo. "
                   "Verifique se há uma atualização do aplicativo.";
        case CHIAKI_QUIT_REASON_CTRL_CONNECTION_REFUSED:
        case CHIAKI_QUIT_REASON_CTRL_CONNECT_FAILED:
            return "Não foi possível conectar ao PS5 pela rede. Confira se o PS5 e o PC estão na mesma rede.";
        case CHIAKI_QUIT_REASON_PSN_REGIST_FAILED:
            return "O pareamento salvo não é mais válido. Pareie o PS5 de novo nas configurações.";
        case CHIAKI_QUIT_REASON_STREAM_CONNECTION_REMOTE_DISCONNECTED:
        case CHIAKI_QUIT_REASON_STREAM_CONNECTION_REMOTE_SHUTDOWN:
            return "O PS5 encerrou a sessão.";
        default:
            return "Erro de Remote Play. Tente novamente.";
    }
}

RegisterResult RegisterConsole(const std::string& host, const AccountId& account, const std::string& pin) {
    RegisterResult out;
    if (host.empty()) {
        out.message = "Informe o endereço IP do PS5.";
        return out;
    }
    if (pin.size() != 8 || !std::all_of(pin.begin(), pin.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        out.message = "O código de pareamento tem 8 dígitos, mostrado no PS5.";
        return out;
    }

    const auto probe = ProbeConsoles(host, 2500, [](const std::vector<ConsoleInfo>& v) { return !v.empty(); });
    if (probe.empty()) {
        out.message = "Nenhum PS5 respondeu em " + host + ". Confira o IP e se o PS5 está ligado.";
        return out;
    }
    const ConsoleInfo console = probe.front();
    if (!console.isPs5) {
        out.message = "Este aparelho não é um PS5.";
        return out;
    }
    if (!console.ready) {
        out.message = "O PS5 está em modo de repouso. Ligue-o completamente para parear.";
        return out;
    }

    chiaki_regist_info_t info{};
    info.target = CHIAKI_TARGET_PS5_1;
    info.host = host.c_str();
    info.broadcast = false;
    info.psn_online_id = nullptr;
    std::memcpy(info.psn_account_id, account.data(), sizeof(info.psn_account_id));
    info.pin = static_cast<std::uint32_t>(std::strtoul(pin.c_str(), nullptr, 10));
    info.console_pin = 0;
    info.holepunch_info = nullptr;
    info.rudp = nullptr;

    struct Context {
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;
        chiaki_regist_event_type_t type = CHIAKI_REGIST_EVENT_TYPE_FINISHED_FAILED;
        chiaki_registered_host_t registered{};
    } ctx;

    auto callback = [](chiaki_regist_event_t* e, void* user) {
        auto* c = static_cast<Context*>(user);
        std::lock_guard lock(c->mutex);
        c->type = e->type;
        if (e->type == CHIAKI_REGIST_EVENT_TYPE_FINISHED_SUCCESS && e->registered_host) {
            c->registered = *e->registered_host;
        }
        c->done = true;
        c->cv.notify_all();
    };

    chiaki_regist_t regist{};
    if (chiaki_regist_start(&regist, chiaki_runtime::Log(), &info, callback, &ctx) != CHIAKI_ERR_SUCCESS) {
        SecureZeroMemory(&info, sizeof(info));
        out.message = "Não foi possível iniciar o pareamento.";
        return out;
    }
    bool done = false;
    {
        std::unique_lock lock(ctx.mutex);
        done = ctx.cv.wait_for(lock, std::chrono::seconds(30), [&] { return ctx.done; });
    }
    if (!done) chiaki_regist_stop(&regist);
    chiaki_regist_fini(&regist);
    SecureZeroMemory(&info, sizeof(info));

    if (!done || ctx.type != CHIAKI_REGIST_EVENT_TYPE_FINISHED_SUCCESS) {
        SecureZeroMemory(&ctx.registered, sizeof(ctx.registered));
        out.message = "O pareamento falhou. Confira o código de 8 dígitos (ele expira em alguns minutos), "
                      "o ID da conta PSN e se o Remote Play está ativado no PS5.";
        return out;
    }

    PairingInfo& p = out.pairing;
    p.target = static_cast<std::uint32_t>(ctx.registered.target ? ctx.registered.target : CHIAKI_TARGET_PS5_1);
    std::memcpy(p.registKey.data(), ctx.registered.rp_regist_key, p.registKey.size());
    std::memcpy(p.morning.data(), ctx.registered.rp_key, p.morning.size());
    p.host = host;
    p.name = ctx.registered.server_nickname[0]
                 ? std::string(ctx.registered.server_nickname,
                               strnlen(ctx.registered.server_nickname, sizeof(ctx.registered.server_nickname)))
                 : console.name;
    p.id = console.id;
    SecureZeroMemory(&ctx.registered, sizeof(ctx.registered));

    if (!p.IsComplete()) {
        out.message = "O PS5 respondeu, mas não enviou um pareamento válido. Tente de novo.";
        return out;
    }
    out.ok = true;
    out.message = "Pareado com " + p.name + ".";
    return out;
}

}  // namespace pskb::chiaki_backend

namespace pskb {
namespace {

using namespace std::chrono_literals;

constexpr unsigned int kLocateUnicastMs = 2500;
constexpr unsigned int kLocateBroadcastMs = 4000;
constexpr unsigned int kWakeReadyMs = 45000;

struct Located {
    ConsoleInfo console;
    bool found = false;
};

Located LocateConsole(const PairingInfo& pairing, const std::atomic<bool>& cancel) {
    Located out;
    auto matchesPairing = [&](const std::vector<ConsoleInfo>& v) {
        for (const auto& c : v) {
            if (c.isPs5 && (pairing.id.empty() || c.id == pairing.id)) return true;
        }
        return false;
    };

    auto found = ProbeConsoles(pairing.host, kLocateUnicastMs, [&](const std::vector<ConsoleInfo>& v) {
        return cancel.load() || matchesPairing(v);
    });
    if (cancel.load()) return out;

    if (!matchesPairing(found)) {
        found = ProbeConsoles("", kLocateBroadcastMs, [&](const std::vector<ConsoleInfo>& v) {
            return cancel.load() || matchesPairing(v);
        });
        if (cancel.load()) return out;
    }

    for (const auto& c : found) {
        if (c.isPs5 && (pairing.id.empty() || c.id == pairing.id)) {
            out.console = c;
            out.found = true;
            return out;
        }
    }
    return out;
}

}  // namespace

ChiakiBackend::ChiakiBackend() { chiaki_runtime::Log(); }

ChiakiBackend::~ChiakiBackend() { Disconnect(); }

void ChiakiBackend::OnEvent(chiaki_event_t* event, void* user) {
    auto* self = static_cast<ChiakiBackend*>(user);
    {
        std::lock_guard lock(self->eventMutex_);
        switch (event->type) {
            case CHIAKI_EVENT_CONNECTED:
                self->events_.connected = true;
                self->connected_ = true;
                break;
            case CHIAKI_EVENT_QUIT:
                self->events_.quit = true;
                self->events_.quitReason = event->quit.reason;
                self->events_.quitRemote = event->quit.reason_str ? event->quit.reason_str : "";
                self->connected_ = false;
                break;
            case CHIAKI_EVENT_LOGIN_PIN_REQUEST:
                self->events_.loginPin = true;
                break;
            default:
                break;
        }
    }
    self->eventCv_.notify_all();
}

void ChiakiBackend::ResetEvents() {
    std::lock_guard lock(eventMutex_);
    events_ = EventState{};
    connected_ = false;
}

void ChiakiBackend::TearDown(chiaki_session_t* session) {
    if (!session) return;
    chiaki_session_stop(session);
    chiaki_session_join(session);
    chiaki_session_fini(session);
    delete session;
}

chiaki_session_t* ChiakiBackend::TakeSession() {
    std::lock_guard lock(mutex_);
    chiaki_session_t* s = session_;
    session_ = nullptr;
    connected_ = false;
    return s;
}

ConnectResult ChiakiBackend::Connect(const PairingInfo& pairing) {
    if (!pairing.IsComplete()) {
        return {false, LinkError::NotPaired, "Pareie seu PS5 primeiro."};
    }

    {
        std::lock_guard lock(eventMutex_);
        cancelRequested_ = false;
    }
    ResetEvents();

    {
        std::lock_guard lock(mutex_);
        if (session_) return {false, LinkError::Other, "Já existe uma sessão ativa com o PS5."};
    }

    const Located located = LocateConsole(pairing, cancelRequested_);
    if (cancelRequested_) return {false, LinkError::Cancelled, "Conexão cancelada."};
    if (!located.found) {
        return {false, LinkError::NotFound,
                "O PS5 não foi encontrado na rede. Confira se ele está ligado (ou em repouso, com "
                "\"Ligar o PS5 pela rede\" ativado) e no mesmo roteador que este PC."};
    }

    ConsoleInfo console = located.console;
    if (!console.ready) {
        WakeConsole(console.host, pairing.WakeCredential());
        const auto woke = ProbeConsoles(console.host, kWakeReadyMs, [&](const std::vector<ConsoleInfo>& v) {
            if (cancelRequested_) return true;
            for (const auto& c : v) {
                if (c.ready) return true;
            }
            return false;
        });
        if (cancelRequested_) return {false, LinkError::Cancelled, "Conexão cancelada."};
        bool ready = false;
        for (const auto& c : woke) {
            if (c.ready) {
                console = c;
                ready = true;
                break;
            }
        }
        if (!ready) {
            return {false, LinkError::Asleep,
                    "O PS5 não acordou do modo de repouso. Em Configurações > Economia de energia > "
                    "Recursos disponíveis no modo de repouso, ative \"Ligar o PS5 pela rede\"."};
        }
    }

    ChiakiConnectInfo info{};
    info.ps5 = true;
    info.host = console.host.c_str();
    std::memcpy(info.regist_key, pairing.registKey.data(), sizeof(info.regist_key));
    std::memcpy(info.morning, pairing.morning.data(), sizeof(info.morning));
    chiaki_connect_video_profile_preset(&info.video_profile, CHIAKI_VIDEO_RESOLUTION_PRESET_360p,
                                        CHIAKI_VIDEO_FPS_PRESET_30);
    info.video_profile.codec = CHIAKI_CODEC_H264;
    info.video_profile_auto_downgrade = true;
    info.audio_video_disabled = CHIAKI_AUDIO_VIDEO_DISABLED;
    info.enable_keyboard = false;
    info.enable_dualsense = false;
    info.auto_regist = false;
    info.holepunch_session = nullptr;
    info.rudp_sock = nullptr;
    info.packet_loss_max = 0.05;
    info.enable_idr_on_fec_failure = false;

    auto session = std::make_unique<chiaki_session_t>();
    std::memset(session.get(), 0, sizeof(chiaki_session_t));
    if (chiaki_session_init(session.get(), &info, chiaki_runtime::Log()) != CHIAKI_ERR_SUCCESS) {
        return {false, LinkError::Protocol, "Não foi possível iniciar o Remote Play."};
    }
    chiaki_session_set_event_cb(session.get(), &ChiakiBackend::OnEvent, this);
    if (chiaki_session_start(session.get()) != CHIAKI_ERR_SUCCESS) {
        chiaki_session_fini(session.get());
        return {false, LinkError::Protocol, "Não foi possível iniciar o Remote Play."};
    }

    chiaki_session_t* raw = session.release();
    {
        std::lock_guard lock(mutex_);
        session_ = raw;
    }
    return WaitForStart(raw, console.name);
}

ConnectResult ChiakiBackend::WaitForStart(chiaki_session_t* session, const std::string& name) {
    EventState snapshot;
    {
        std::unique_lock lock(eventMutex_);
        eventCv_.wait_for(lock, std::chrono::seconds(connectTimeoutSec_), [&] {
            return events_.connected || events_.quit || events_.loginPin || cancelRequested_.load();
        });
        snapshot = events_;
    }

    auto abandon = [&] { TearDown(TakeSession()); };

    if (cancelRequested_) {
        abandon();
        return {false, LinkError::Cancelled, "Conexão cancelada."};
    }
    if (snapshot.loginPin) {
        abandon();
        return {false, LinkError::LoginPasscode,
                "O usuário do PS5 tem senha de login, e este aplicativo não consegue digitá-la. "
                "Desative a senha desse usuário nas configurações do PS5 e tente de novo."};
    }
    if (snapshot.quit) {
        abandon();
        return {false, chiaki_backend::ClassifyQuit(snapshot.quitReason),
                chiaki_backend::QuitMessage(snapshot.quitReason)};
    }
    if (!snapshot.connected) {
        abandon();
        return {false, LinkError::Protocol,
                "O PS5 não confirmou a sessão a tempo. Feche outros apps de Remote Play e tente de novo."};
    }

    chiaki_controller_state_t idle{};
    chiaki_controller_state_set_idle(&idle);
    {
        std::lock_guard lock(mutex_);
        if (session_ == session) chiaki_session_set_controller_state(session, &idle);
    }
    return {true, LinkError::None, "Conectado ao " + name + " (somente controle)."};
}

void ChiakiBackend::SendPad(const PadState& pad) {
    if (!connected_) return;
    std::lock_guard lock(mutex_);
    if (!session_ || !connected_) return;
    chiaki_controller_state_t state{};
    chiaki_backend::ToControllerState(pad, state);
    chiaki_session_set_controller_state(session_, &state);
}

bool ChiakiBackend::IsConnected() const { return connected_; }

void ChiakiBackend::Disconnect() {
    {
        std::lock_guard lock(eventMutex_);
        cancelRequested_ = true;
    }
    eventCv_.notify_all();

    chiaki_session_t* session = nullptr;
    {
        std::lock_guard lock(mutex_);
        session = session_;
        if (session) {
            chiaki_controller_state_t idle{};
            chiaki_controller_state_set_idle(&idle);
            chiaki_session_set_controller_state(session, &idle);
        }
        session_ = nullptr;
        connected_ = false;
    }
    if (session) {
        std::this_thread::sleep_for(60ms);  // let the neutral frame leave before the session stops
        TearDown(session);
    }
}

void ChiakiBackend::RequestCancel() {
    {
        std::lock_guard lock(eventMutex_);
        cancelRequested_ = true;
    }
    eventCv_.notify_all();
}

}  // namespace pskb
