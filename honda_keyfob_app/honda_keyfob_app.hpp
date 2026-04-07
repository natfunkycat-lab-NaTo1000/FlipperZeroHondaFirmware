#pragma once

// C headers must be wrapped for C++ compilation
#ifdef __cplusplus
extern "C" {
#endif

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>
#include <gui/modules/widget.h>
#include <dialogs/dialogs.h>
#include <notification/notification_messages.h>
#include <lib/subghz/subghz_tx_rx_worker.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/transmitter.h>
#include <lib/subghz/environment.h>
#include <lib/subghz/protocols/raw.h>
#include <flipper_format/flipper_format.h>
#include <storage/storage.h>

#ifdef __cplusplus
}
#endif

// ──────────────────────────────────────────────────────────────────────────────
// Honda CC1101 preset configuration (Honda1 modulation @ 433.657 MHz)
// Demonstrates CVE-2022-27254 – for educational purposes only.
// ──────────────────────────────────────────────────────────────────────────────

constexpr uint32_t HONDA_FREQUENCY_PRIMARY   = 433657070UL;  // 433.657 MHz
constexpr uint32_t HONDA_FREQUENCY_SECONDARY = 434176948UL;  // 434.177 MHz

// Honda1 custom CC1101 register dump (46 bytes: reg-addr / reg-value pairs)
constexpr uint8_t HONDA_PRESET_HONDA1[] = {
    0x02, 0x0D, 0x0B, 0x06, 0x08, 0x32, 0x07, 0x04,
    0x14, 0x00, 0x13, 0x02, 0x12, 0x04, 0x11, 0x36,
    0x10, 0x69, 0x15, 0x32, 0x18, 0x18, 0x19, 0x16,
    0x1D, 0x91, 0x1C, 0x00, 0x1B, 0x07, 0x20, 0xFB,
    0x22, 0x10, 0x21, 0x56, 0x00, 0x00, 0xC0, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
constexpr size_t HONDA_PRESET_HONDA1_SIZE = sizeof(HONDA_PRESET_HONDA1);

// Honda2 custom CC1101 register dump (46 bytes) – more sensitive / noisier
constexpr uint8_t HONDA_PRESET_HONDA2[] = {
    0x02, 0x0D, 0x0B, 0x06, 0x08, 0x32, 0x07, 0x04,
    0x14, 0x00, 0x13, 0x02, 0x12, 0x07, 0x11, 0x36,
    0x10, 0xE9, 0x15, 0x32, 0x18, 0x18, 0x19, 0x16,
    0x1D, 0x92, 0x1C, 0x40, 0x1B, 0x03, 0x20, 0xFB,
    0x22, 0x10, 0x21, 0x56, 0x00, 0x00, 0xC0, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
constexpr size_t HONDA_PRESET_HONDA2_SIZE = sizeof(HONDA_PRESET_HONDA2);

// SD-card paths for captured signal files
constexpr const char* HONDA_LOCK_SUB_PATH   = EXT_PATH("subghz/honda/Lock_honda.sub");
constexpr const char* HONDA_UNLOCK_SUB_PATH = EXT_PATH("subghz/honda/Unlock_honda.sub");

// ──────────────────────────────────────────────────────────────────────────────
// View IDs
// ──────────────────────────────────────────────────────────────────────────────

enum HondaKeyFobViewId : uint32_t {
    HondaViewMenu    = 0,
    HondaViewPopup   = 1,
    HondaViewWidget  = 2,
};

// ──────────────────────────────────────────────────────────────────────────────
// Application state
// ──────────────────────────────────────────────────────────────────────────────

enum class HondaAction : uint8_t {
    None    = 0,
    Lock    = 1,
    Unlock  = 2,
};

struct HondaKeyFobApp {
    // GUI / dispatcher
    Gui*            gui;
    ViewDispatcher* view_dispatcher;
    Submenu*        submenu;
    Popup*          popup;
    Widget*         widget;
    DialogsApp*     dialogs;

    // System
    NotificationApp* notifications;

    // SubGHz
    SubGhzEnvironment*    subghz_env;
    SubGhzTransmitter*    subghz_tx;
    FuriString*           tx_preset_name;

    // State
    HondaAction pending_action;
};

// ──────────────────────────────────────────────────────────────────────────────
// C entry point (required by Flipper application loader)
// ──────────────────────────────────────────────────────────────────────────────

#ifdef __cplusplus
extern "C" {
#endif

int32_t honda_keyfob_app(void* p);

#ifdef __cplusplus
}
#endif
