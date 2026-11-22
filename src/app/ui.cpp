#include "app/ui.h"

#include <iterator>
#include <map>
#include <utility>

namespace pskb::ui {
namespace {

constexpr wchar_t kColorProp[] = L"pskb.color";
constexpr wchar_t kPrimaryProp[] = L"pskb.primary";

HINSTANCE Instance() { return GetModuleHandleW(nullptr); }

COLORREF ColorOf(HWND control, COLORREF fallback) {
    const HANDLE stored = GetPropW(control, kColorProp);
    if (!stored) return fallback;
    return static_cast<COLORREF>(reinterpret_cast<ULONG_PTR>(stored) - 1);
}

void ApplyFont(HWND control, HFONT font) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

}  // namespace

std::wstring ToWide(std::string_view utf8) {
    if (utf8.empty()) return {};
    const int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring out(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), length);
    return out;
}

std::string ToUtf8(std::wstring_view wide) {
    if (wide.empty()) return {};
    const int length = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0,
                                           nullptr, nullptr);
    std::string out(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), out.data(), length, nullptr,
                        nullptr);
    return out;
}

std::string GetText(HWND control) {
    const int length = GetWindowTextLengthW(control);
    if (length <= 0) return {};
    std::wstring text(static_cast<std::size_t>(length), L'\0');
    GetWindowTextW(control, text.data(), length + 1);
    return ToUtf8(text);
}

void SetText(HWND control, std::string_view utf8) { SetWindowTextW(control, ToWide(utf8).c_str()); }

HFONT Font(int pixelHeight, int weight) {
    static std::map<std::pair<int, int>, HFONT> cache;
    const auto key = std::make_pair(pixelHeight, weight);
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    HFONT font = CreateFontW(-pixelHeight, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    cache.emplace(key, font);
    return font;
}

HBRUSH Brush(COLORREF color) {
    static std::map<COLORREF, HBRUSH> cache;
    auto it = cache.find(color);
    if (it != cache.end()) return it->second;
    HBRUSH brush = CreateSolidBrush(color);
    cache.emplace(color, brush);
    return brush;
}

HWND Label(HWND parent, std::string_view text, int id, int x, int y, int w, int h, HFONT font, COLORREF color) {
    HWND control = CreateWindowExW(0, L"STATIC", ToWide(text).c_str(), WS_CHILD | WS_VISIBLE, x, y, w, h, parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), Instance(), nullptr);
    ApplyFont(control, font ? font : Font(16));
    SetColor(control, color);
    return control;
}

HWND Button(HWND parent, std::string_view text, int id, int x, int y, int w, int h, bool primary) {
    HWND control = CreateWindowExW(0, L"BUTTON", ToWide(text).c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                   x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), Instance(),
                                   nullptr);
    ApplyFont(control, Font(primary ? 26 : 18, FW_SEMIBOLD));
    if (primary) MarkPrimary(control);
    return control;
}

HWND Edit(HWND parent, int id, int x, int y, int w, int h, int maxLength) {
    HWND control = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, x, y, w, h, parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), Instance(), nullptr);
    ApplyFont(control, Font(18));
    if (maxLength > 0) SendMessageW(control, EM_SETLIMITTEXT, static_cast<WPARAM>(maxLength), 0);
    return control;
}

HWND Check(HWND parent, std::string_view text, int id, int x, int y, int w, int h, bool checked) {
    HWND control = CreateWindowExW(0, L"BUTTON", ToWide(text).c_str(),
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, x, y, w, h, parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), Instance(), nullptr);
    ApplyFont(control, Font(18));
    SendMessageW(control, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    return control;
}

HWND ComboBox(HWND parent, int id, int x, int y, int w, int h) {
    HWND control = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                                   x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), Instance(),
                                   nullptr);
    ApplyFont(control, Font(18));
    return control;
}

HWND ListBox(HWND parent, int id, int x, int y, int w, int h) {
    HWND control = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                                   x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), Instance(),
                                   nullptr);
    ApplyFont(control, Font(18));
    return control;
}

void SetColor(HWND control, COLORREF color) {
    SetPropW(control, kColorProp, reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(color) + 1));
    InvalidateRect(control, nullptr, TRUE);
}

void MarkPrimary(HWND button) {
    SetPropW(button, kPrimaryProp, reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(1)));
}

bool HandleControlColor(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) {
    const HDC dc = reinterpret_cast<HDC>(wParam);
    const HWND control = reinterpret_cast<HWND>(lParam);
    switch (message) {
        case WM_CTLCOLORSTATIC:
            SetTextColor(dc, ColorOf(control, kText));
            SetBkMode(dc, TRANSPARENT);
            *result = reinterpret_cast<LRESULT>(Brush(kBackground));
            return true;
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            SetTextColor(dc, kText);
            SetBkColor(dc, kInput);
            *result = reinterpret_cast<LRESULT>(Brush(kInput));
            return true;
        case WM_CTLCOLORBTN:
            SetTextColor(dc, kText);
            SetBkMode(dc, TRANSPARENT);
            *result = reinterpret_cast<LRESULT>(Brush(kBackground));
            return true;
        default:
            return false;
    }
}

bool DrawButton(LPARAM lParam) {
    auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
    if (item->CtlType != ODT_BUTTON) return false;

    const bool primary = GetPropW(item->hwndItem, kPrimaryProp) != nullptr;
    const bool disabled = (item->itemState & ODS_DISABLED) != 0;
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;

    COLORREF fill = primary ? kAccent : kPanel;
    if (pressed) fill = primary ? RGB(0x23, 0x63, 0xB8) : kInput;
    if (disabled) fill = primary ? RGB(0x2A, 0x3A, 0x52) : kBackground;
    const COLORREF textColor = disabled ? kMuted : kText;

    RECT rect = item->rcItem;
    FillRect(item->hDC, &rect, Brush(kBackground));

    const int savedDc = SaveDC(item->hDC);
    SelectObject(item->hDC, Brush(fill));
    SelectObject(item->hDC, GetStockObject(NULL_PEN));
    RoundRect(item->hDC, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
    RestoreDC(item->hDC, savedDc);

    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, textColor);
    const HFONT font = reinterpret_cast<HFONT>(SendMessageW(item->hwndItem, WM_GETFONT, 0, 0));
    const HFONT previous = font ? static_cast<HFONT>(SelectObject(item->hDC, font)) : nullptr;

    wchar_t text[256] = {};
    GetWindowTextW(item->hwndItem, text, static_cast<int>(std::size(text)));
    DrawTextW(item->hDC, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if (previous) SelectObject(item->hDC, previous);
    if (item->itemState & ODS_FOCUS) {
        RECT focus = rect;
        InflateRect(&focus, -4, -4);
        DrawFocusRect(item->hDC, &focus);
    }
    return true;
}

HBRUSH WindowBackground() { return Brush(kBackground); }

}  // namespace pskb::ui
