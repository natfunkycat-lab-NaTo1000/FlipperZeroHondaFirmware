/**
 * SubGHz application entry point – C++ rewrite.
 *
 * All string_t / M*LIB usage replaced with FuriString* (see furi/core/string.h).
 */

/* Abandon hope, all ye who enter here. */

#include "subghz/types.h"
#include "subghz_i.h"
#include <lib/toolbox/path.h>

// ──────────────────────────────────────────────────────────────────────────────
// ViewDispatcher event callbacks
// ──────────────────────────────────────────────────────────────────────────────

static bool subghz_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    SubGhz* subghz = static_cast<SubGhz*>(context);
    return scene_manager_handle_custom_event(subghz->scene_manager, event);
}

static bool subghz_back_event_callback(void* context) {
    furi_assert(context);
    SubGhz* subghz = static_cast<SubGhz*>(context);
    return scene_manager_handle_back_event(subghz->scene_manager);
}

static void subghz_tick_event_callback(void* context) {
    furi_assert(context);
    SubGhz* subghz = static_cast<SubGhz*>(context);
    scene_manager_handle_tick_event(subghz->scene_manager);
}

// ──────────────────────────────────────────────────────────────────────────────
// RPC callback
// ──────────────────────────────────────────────────────────────────────────────

static void subghz_rpc_command_callback(RpcAppSystemEvent event, void* context) {
    furi_assert(context);
    SubGhz* subghz = static_cast<SubGhz*>(context);
    furi_assert(subghz->rpc_ctx);

    if(event == RpcAppEventSessionClose) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSceneRpcSessionClose);
        rpc_system_app_set_callback(subghz->rpc_ctx, nullptr, nullptr);
        subghz->rpc_ctx = nullptr;
    } else if(event == RpcAppEventAppExit) {
        view_dispatcher_send_custom_event(subghz->view_dispatcher, SubGhzCustomEventSceneExit);
    } else if(event == RpcAppEventLoadFile) {
        view_dispatcher_send_custom_event(subghz->view_dispatcher, SubGhzCustomEventSceneRpcLoad);
    } else if(event == RpcAppEventButtonPress) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSceneRpcButtonPress);
    } else if(event == RpcAppEventButtonRelease) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSceneRpcButtonRelease);
    } else {
        rpc_system_app_confirm(subghz->rpc_ctx, event, false);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// LED helpers
// ──────────────────────────────────────────────────────────────────────────────

void subghz_blink_start(SubGhz* instance) {
    furi_assert(instance);
    notification_message(instance->notifications, &sequence_blink_start_magenta);
}

void subghz_blink_stop(SubGhz* instance) {
    furi_assert(instance);
    notification_message(instance->notifications, &sequence_blink_stop);
}

// ──────────────────────────────────────────────────────────────────────────────
// Allocation / deallocation
// ──────────────────────────────────────────────────────────────────────────────

static SubGhz* subghz_alloc() {
    SubGhz* subghz = static_cast<SubGhz*>(malloc(sizeof(SubGhz)));

    subghz->file_path     = furi_string_alloc();
    subghz->file_path_tmp = furi_string_alloc();

    // GUI
    subghz->gui = static_cast<Gui*>(furi_record_open(RECORD_GUI));

    // ViewDispatcher
    subghz->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(subghz->view_dispatcher);
    subghz->scene_manager = scene_manager_alloc(&subghz_scene_handlers, subghz);
    view_dispatcher_set_event_callback_context(subghz->view_dispatcher, subghz);
    view_dispatcher_set_custom_event_callback(
        subghz->view_dispatcher, subghz_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        subghz->view_dispatcher, subghz_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        subghz->view_dispatcher, subghz_tick_event_callback, 100);

    // Notifications
    subghz->notifications =
        static_cast<NotificationApp*>(furi_record_open(RECORD_NOTIFICATION));

    // SubMenu
    subghz->submenu = submenu_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher, SubGhzViewIdMenu, submenu_get_view(subghz->submenu));

    // Receiver
    subghz->subghz_receiver = subghz_view_receiver_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdReceiver,
        subghz_view_receiver_get_view(subghz->subghz_receiver));

    // Popup
    subghz->popup = popup_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher, SubGhzViewIdPopup, popup_get_view(subghz->popup));

    // Text Input
    subghz->text_input = text_input_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdTextInput,
        text_input_get_view(subghz->text_input));

    // Custom Widget
    subghz->widget = widget_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher, SubGhzViewIdWidget, widget_get_view(subghz->widget));

    // Dialog
    subghz->dialogs = static_cast<DialogsApp*>(furi_record_open(RECORD_DIALOGS));

    // Transmitter
    subghz->subghz_transmitter = subghz_view_transmitter_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdTransmitter,
        subghz_view_transmitter_get_view(subghz->subghz_transmitter));

    // Variable Item List
    subghz->variable_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdVariableItemList,
        variable_item_list_get_view(subghz->variable_item_list));

    // Frequency Analyzer
    subghz->subghz_frequency_analyzer = subghz_frequency_analyzer_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdFrequencyAnalyzer,
        subghz_frequency_analyzer_get_view(subghz->subghz_frequency_analyzer));

    // Read RAW
    subghz->subghz_read_raw = subghz_read_raw_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdReadRAW,
        subghz_read_raw_get_view(subghz->subghz_read_raw));

    // Carrier Test
    subghz->subghz_test_carrier = subghz_test_carrier_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdTestCarrier,
        subghz_test_carrier_get_view(subghz->subghz_test_carrier));

    // Packet Test
    subghz->subghz_test_packet = subghz_test_packet_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdTestPacket,
        subghz_test_packet_get_view(subghz->subghz_test_packet));

    // Static send
    subghz->subghz_test_static = subghz_test_static_alloc();
    view_dispatcher_add_view(
        subghz->view_dispatcher,
        SubGhzViewIdStatic,
        subghz_test_static_get_view(subghz->subghz_test_static));

    // Settings
    subghz->setting = subghz_setting_alloc();
    subghz_setting_load(subghz->setting, EXT_PATH("subghz/assets/setting_user"));

    // Worker / protocol / history / keyboard
    subghz->lock = SubGhzLockOff;
    subghz->txrx = static_cast<SubGhzTxRx*>(malloc(sizeof(SubGhzTxRx)));
    subghz->txrx->preset = static_cast<SubGhzPresetDefinition*>(
        malloc(sizeof(SubGhzPresetDefinition)));
    subghz->txrx->preset->name = furi_string_alloc();
    subghz_preset_init(
        subghz,
        "AM650",
        subghz_setting_get_default_frequency(subghz->setting),
        nullptr,
        0);

    subghz->txrx->txrx_state          = SubGhzTxRxStateSleep;
    subghz->txrx->hopper_state         = SubGhzHopperStateOFF;
    subghz->txrx->rx_key_state         = SubGhzRxKeyStateIDLE;
    subghz->txrx->history              = subghz_history_alloc();
    subghz->txrx->worker               = subghz_worker_alloc();
    subghz->txrx->fff_data             = flipper_format_string_alloc();
    subghz->txrx->environment          = subghz_environment_alloc();

    subghz_environment_set_came_atomo_rainbow_table_file_name(
        subghz->txrx->environment, EXT_PATH("subghz/assets/came_atomo"));
    subghz_environment_set_nice_flor_s_rainbow_table_file_name(
        subghz->txrx->environment, EXT_PATH("subghz/assets/nice_flor_s"));
    subghz->txrx->receiver =
        subghz_receiver_alloc_init(subghz->txrx->environment);
    subghz_receiver_set_filter(subghz->txrx->receiver, SubGhzProtocolFlag_Decodable);

    subghz_worker_set_overrun_callback(
        subghz->txrx->worker,
        reinterpret_cast<SubGhzWorkerOverrunCallback>(subghz_receiver_reset));
    subghz_worker_set_pair_callback(
        subghz->txrx->worker,
        reinterpret_cast<SubGhzWorkerPairCallback>(subghz_receiver_decode));
    subghz_worker_set_context(subghz->txrx->worker, subghz->txrx->receiver);

    subghz->error_str = furi_string_alloc();

    return subghz;
}

static void subghz_free(SubGhz* subghz) {
    furi_assert(subghz);

    if(subghz->rpc_ctx) {
        rpc_system_app_set_callback(subghz->rpc_ctx, nullptr, nullptr);
        rpc_system_app_send_exited(subghz->rpc_ctx);
        subghz_blink_stop(subghz);
        subghz->rpc_ctx = nullptr;
    }

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdTestPacket);
    subghz_test_packet_free(subghz->subghz_test_packet);

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdTestCarrier);
    subghz_test_carrier_free(subghz->subghz_test_carrier);

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdStatic);
    subghz_test_static_free(subghz->subghz_test_static);

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdReceiver);
    subghz_view_receiver_free(subghz->subghz_receiver);

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdTextInput);
    text_input_free(subghz->text_input);

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdWidget);
    widget_free(subghz->widget);

    furi_record_close(RECORD_DIALOGS);

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdTransmitter);
    subghz_view_transmitter_free(subghz->subghz_transmitter);

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdVariableItemList);
    variable_item_list_free(subghz->variable_item_list);

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdFrequencyAnalyzer);
    subghz_frequency_analyzer_free(subghz->subghz_frequency_analyzer);

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdReadRAW);
    subghz_read_raw_free(subghz->subghz_read_raw);

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdMenu);
    submenu_free(subghz->submenu);

    view_dispatcher_remove_view(subghz->view_dispatcher, SubGhzViewIdPopup);
    popup_free(subghz->popup);

    scene_manager_free(subghz->scene_manager);
    view_dispatcher_free(subghz->view_dispatcher);

    furi_record_close(RECORD_GUI);
    subghz->gui = nullptr;

    subghz_setting_free(subghz->setting);

    subghz_receiver_free(subghz->txrx->receiver);
    subghz_environment_free(subghz->txrx->environment);
    subghz_worker_free(subghz->txrx->worker);
    flipper_format_free(subghz->txrx->fff_data);
    subghz_history_free(subghz->txrx->history);

    furi_string_free(subghz->txrx->preset->name);
    free(subghz->txrx->preset);
    free(subghz->txrx);

    furi_string_free(subghz->error_str);

    furi_record_close(RECORD_NOTIFICATION);
    subghz->notifications = nullptr;

    furi_string_free(subghz->file_path);
    furi_string_free(subghz->file_path_tmp);

    free(subghz);
}

// ──────────────────────────────────────────────────────────────────────────────
// Application entry point
// ──────────────────────────────────────────────────────────────────────────────

extern "C" int32_t subghz_app(void* p) {
    SubGhz* subghz = subghz_alloc();

    if(!furi_hal_region_is_provisioned()) {
        subghz_dialog_message_show_only_rx(subghz);
        subghz_free(subghz);
        return 1;
    }

    // Load database
    bool load_database = subghz_environment_load_keystore(
        subghz->txrx->environment, EXT_PATH("subghz/assets/keeloq_mfcodes"));
    subghz_environment_load_keystore(
        subghz->txrx->environment, EXT_PATH("subghz/assets/keeloq_mfcodes_user"));

    if(p && strlen(static_cast<const char*>(p))) {
        const char* arg = static_cast<const char*>(p);
        uint32_t    rpc_ctx = 0;
        if(sscanf(arg, "RPC %lX", &rpc_ctx) == 1) {
            subghz->rpc_ctx = reinterpret_cast<void*>(rpc_ctx);
            rpc_system_app_set_callback(subghz->rpc_ctx, subghz_rpc_command_callback, subghz);
            rpc_system_app_send_started(subghz->rpc_ctx);
            view_dispatcher_attach_to_gui(
                subghz->view_dispatcher, subghz->gui, ViewDispatcherTypeDesktop);
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneRpc);
        } else {
            view_dispatcher_attach_to_gui(
                subghz->view_dispatcher, subghz->gui, ViewDispatcherTypeFullscreen);
            if(subghz_key_load(subghz, arg, true)) {
                furi_string_set_str(subghz->file_path, arg);
                if(!strcmp(subghz->txrx->decoder_result->protocol->name, "RAW")) {
                    subghz->txrx->rx_key_state = SubGhzRxKeyStateRAWLoad;
                    scene_manager_next_scene(subghz->scene_manager, SubGhzSceneReadRAW);
                } else {
                    scene_manager_next_scene(subghz->scene_manager, SubGhzSceneTransmitter);
                }
            } else {
                scene_manager_stop(subghz->scene_manager);
                view_dispatcher_stop(subghz->view_dispatcher);
            }
        }
    } else {
        view_dispatcher_attach_to_gui(
            subghz->view_dispatcher, subghz->gui, ViewDispatcherTypeFullscreen);
        furi_string_set_str(subghz->file_path, SUBGHZ_APP_FOLDER);
        if(load_database) {
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneStart);
        } else {
            scene_manager_set_scene_state(
                subghz->scene_manager,
                SubGhzSceneShowError,
                SubGhzCustomEventManagerSet);
            furi_string_set_str(
                subghz->error_str,
                "No SD card or\ndatabase found.\nSome app function\nmay be reduced.");
            scene_manager_next_scene(subghz->scene_manager, SubGhzSceneShowError);
        }
    }

    furi_hal_power_suppress_charge_enter();
    view_dispatcher_run(subghz->view_dispatcher);
    furi_hal_power_suppress_charge_exit();

    subghz_free(subghz);
    return 0;
}
