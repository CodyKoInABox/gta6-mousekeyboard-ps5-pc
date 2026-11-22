#pragma once

#include <windows.h>

#include <string>
#include <string_view>

namespace pskb::ui {

inline constexpr COLORREF kBackground = RGB(0x12, 0x14, 0x18);
inline constexpr COLORREF kPanel = RGB(0x1E, 0x21, 0x28);
inline constexpr COLORREF kInput = RGB(0x2A, 0x2E, 0x38);
inline constexpr COLORREF kText = RGB(0xF2, 0xF4, 0xF8);
inline constexpr COLORREF kMuted = RGB(0x9A, 0xA3, 0xB2);
inline constexpr COLORREF kAccent = RGB(0x2F, 0x7D, 0xE8);
inline constexpr COLORREF kGood = RGB(0x3D, 0xC2, 0x7A);
inline constexpr COLORREF kWarn = RGB(0xF5, 0xB0, 0x42);
inline constexpr COLORREF kBad = RGB(0xFF, 0x6B, 0x6B);

std::wstring ToWide(std::string_view utf8);
std::string ToUtf8(std::wstring_view wide);
std::string GetText(HWND control);
void SetText(HWND control, std::string_view utf8);

HFONT Font(int pixelHeight, int weight = FW_NORMAL);
HBRUSH Brush(COLORREF color);

// Controls. Ids are caller-chosen; labels and buttons take their colours from SetColor/MarkPrimary.
HWND Label(HWND parent, std::string_view text, int id, int x, int y, int w, int h, HFONT font = nullptr,
           COLORREF color = kText);
HWND Button(HWND parent, std::string_view text, int id, int x, int y, int w, int h, bool primary = false);
HWND Edit(HWND parent, int id, int x, int y, int w, int h, int maxLength = 0);
HWND Check(HWND parent, std::string_view text, int id, int x, int y, int w, int h, bool checked);
HWND ComboBox(HWND parent, int id, int x, int y, int w, int h);
HWND ListBox(HWND parent, int id, int x, int y, int w, int h);

void SetColor(HWND control, COLORREF color);
void MarkPrimary(HWND button);

// Forward from WM_CTLCOLOR*. Returns true and sets result when the message was handled.
bool HandleControlColor(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result);
// Forward from WM_DRAWITEM. Returns true when the item was an owner-drawn button.
bool DrawButton(LPARAM lParam);

// Creates the dark background brush used as a window class background.
HBRUSH WindowBackground();

}  // namespace pskb::ui
