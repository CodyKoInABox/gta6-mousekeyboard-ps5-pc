cmake_minimum_required(VERSION 3.23)
project(PS5KeyboardBridge VERSION 0.1.0 LANGUAGES C CXX)

if(NOT WIN32)
  message(FATAL_ERROR "PS5 Keyboard Bridge targets Windows 10/11 x64 only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static CRT: one self-contained exe, no VC++ redistributable needed on the user's PC.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# /Brepro: the PE timestamp comes from a content hash, so identical sources give identical binaries.
if(MSVC)
  add_link_options($<$<CONFIG:Release>:/Brepro>)
endif()

option(PSKB_BUILD_TESTS "Build the offline test suite" ON)

# Remote Play protocol core (chiaki-ng, LAN only). See third_party/README.md.
add_subdirectory(third_party EXCLUDE_FROM_ALL)

# Portable logic: pad model, mapping, mouse-to-stick maths, pairing storage, link state.
add_library(pskb_core STATIC
  src/core/pad_state.cpp
  src/core/input_codes.cpp
  src/core/mouse_to_stick.cpp
  src/core/bindings.cpp
  src/core/pad_engine.cpp
  src/core/account_id.cpp
  src/core/psn_login.cpp
  src/core/pairing.cpp
  src/core/secure_store.cpp
  src/core/settings.cpp
  src/core/link_manager.cpp
  src/core/raw_keys.cpp
  src/core/capture_session.cpp
  src/core/input_gate.cpp
  src/core/process_match.cpp
  src/core/app_model.cpp
  src/core/remote_play_clients.cpp
  src/core/windows_support.cpp
)
target_include_directories(pskb_core PUBLIC src)
target_compile_options(pskb_core PRIVATE /W4 /utf-8 /permissive-)
target_compile_definitions(pskb_core PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(pskb_core PUBLIC crypt32)

# Remote Play backend built on chiaki-ng.
add_library(pskb_backend STATIC
  src/backend/chiaki_backend.cpp
  src/backend/chiaki_discovery.cpp
  src/backend/psn_signin.cpp
)
target_include_directories(pskb_backend PUBLIC src)
target_compile_options(pskb_backend PRIVATE /W4 /utf-8 /permissive-)
target_compile_definitions(pskb_backend PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(pskb_backend PUBLIC pskb_core chiaki_core)

# Win32 input: Raw Input observation, foreground gate, window tracking.
add_library(pskb_platform STATIC
  src/platform/input_bridge.cpp
  src/platform/veyra_locator.cpp
  src/platform/process_list.cpp
  src/platform/remote_play_guard.cpp
  src/platform/network_status.cpp
  src/platform/windows_version.cpp
)
target_include_directories(pskb_platform PUBLIC src)
target_compile_options(pskb_platform PRIVATE /W4 /utf-8 /permissive-)
target_compile_definitions(pskb_platform PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(pskb_platform PUBLIC pskb_core iphlpapi ws2_32)

add_executable(pskb_cli src/cli/main.cpp)
target_compile_options(pskb_cli PRIVATE /W4 /utf-8 /permissive-)
target_compile_definitions(pskb_cli PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(pskb_cli PRIVATE pskb_core pskb_backend pskb_platform)
set_target_properties(pskb_cli PROPERTIES OUTPUT_NAME "ps5kb-cli")

# The windowed app: setup wizard, main screen, tray, and settings. Pt-BR, dark theme, native Win32.
add_executable(pskb_app WIN32
  src/app/main.cpp
  src/app/ui.cpp
  src/app/saved_pairing.cpp
  src/app/app_controller.cpp
  src/app/main_window.cpp
  src/app/setup_wizard.cpp
  src/app/settings_window.cpp
  src/app/bindings_window.cpp
  src/app/self_test.cpp
)
target_compile_options(pskb_app PRIVATE /W4 /utf-8 /permissive-)
target_compile_definitions(pskb_app PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(pskb_app PRIVATE pskb_core pskb_backend pskb_platform comdlg32 shell32 user32 gdi32)
set_target_properties(pskb_app PROPERTIES OUTPUT_NAME "PS5KeyboardBridge")

if(PSKB_BUILD_TESTS)
  enable_testing()
  add_executable(pskb_tests
    tests/test_main.cpp
    tests/test_pad_state.cpp
    tests/test_mouse_to_stick.cpp
    tests/test_bindings.cpp
    tests/test_pad_engine.cpp
    tests/test_account_id.cpp
    tests/test_psn_login.cpp
    tests/test_pairing.cpp
    tests/test_secure_store.cpp
    tests/test_settings.cpp
    tests/test_link_manager.cpp
    tests/test_chiaki_mapping.cpp
    tests/test_raw_keys.cpp
    tests/test_process_match.cpp
    tests/test_capture_session.cpp
    tests/test_input_gate.cpp
    tests/test_app_model.cpp
    tests/test_input_bridge.cpp
    tests/test_remote_play_clients.cpp
    tests/test_windows_support.cpp
    tests/test_aim_sensitivity.cpp
    tests/test_bindings_edit.cpp
    tests/test_settings_audit.cpp
  )
  target_compile_options(pskb_tests PRIVATE /W4 /utf-8 /permissive-)
  target_compile_definitions(pskb_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(pskb_tests PRIVATE pskb_core pskb_backend pskb_platform)
  add_test(NAME pskb_tests COMMAND pskb_tests)
endif()
