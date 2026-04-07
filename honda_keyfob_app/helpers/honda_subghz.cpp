/**
 * Honda SubGHz helper – C++ implementation.
 *
 * Loads Honda key fob .sub files from the SD card and transmits them via the
 * Flipper Zero CC1101 Sub-GHz transceiver.  Custom CC1101 preset data is read
 * inline from the .sub file through SubGhzSetting, matching the approach used
 * by the main SubGHz application.
 */

#include "honda_subghz.hpp"

// The SubGHz setting module manages preset data (including custom Honda ones)
#include "../../../flipperzero-firmware/applications/subghz/subghz_setting.h"

#include <furi.h>
#include <furi_hal.h>
#include <lib/subghz/environment.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/transmitter.h>
#include <lib/subghz/protocols/raw.h>
#include <flipper_format/flipper_format.h>
#include <storage/storage.h>

#define TAG "HondaSubGHz"

// ──────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ──────────────────────────────────────────────────────────────────────────────

/**
 * @brief  Load a Honda .sub file, configure the CC1101 with the embedded
 *         custom preset, and transmit the RAW signal.
 *
 * @param  env           SubGHz environment (for RAW transmitter).
 * @param  sub_file_path Absolute path to a Honda .sub capture file.
 * @return true on success.
 */
static bool honda_subghz_load_and_transmit(
    SubGhzEnvironment* env,
    const char*        sub_file_path) {
    furi_assert(env);
    furi_assert(sub_file_path);

    Storage*       storage  = static_cast<Storage*>(furi_record_open(RECORD_STORAGE));
    FlipperFormat* fff_file = flipper_format_file_alloc(storage);

    // A temporary SubGhzSetting instance is used solely to parse and store
    // the inline custom preset embedded in the .sub file.
    SubGhzSetting* setting = subghz_setting_alloc();

    bool        result       = false;
    uint32_t    frequency    = 0;
    FuriString* file_type    = furi_string_alloc();
    FuriString* preset_str   = furi_string_alloc();
    uint32_t    file_version = 0;

    do {
        if(!flipper_format_file_open_existing(fff_file, sub_file_path)) {
            FURI_LOG_E(TAG, "Failed to open %s", sub_file_path);
            break;
        }
        if(!flipper_format_read_header(fff_file, file_type, &file_version)) {
            FURI_LOG_E(TAG, "Missing header in %s", sub_file_path);
            break;
        }
        if(furi_string_cmp_str(file_type, "Flipper SubGhz RAW File") != 0 ||
           file_version != 1) {
            FURI_LOG_E(TAG, "Unsupported file type or version");
            break;
        }
        if(!flipper_format_read_uint32(fff_file, "Frequency", &frequency, 1)) {
            FURI_LOG_E(TAG, "Missing Frequency");
            break;
        }
        if(!furi_hal_subghz_is_frequency_valid(frequency)) {
            FURI_LOG_E(TAG, "Frequency %lu is not valid", frequency);
            break;
        }
        if(!furi_hal_region_is_frequency_allowed(frequency)) {
            FURI_LOG_E(TAG, "Frequency %lu is restricted in this region", frequency);
            break;
        }
        if(!flipper_format_read_string(fff_file, "Preset", preset_str)) {
            FURI_LOG_E(TAG, "Missing Preset");
            break;
        }

        // Load the inline custom preset so we can pass its register data to
        // the CC1101 hardware driver.
        const char* preset_name_cstr = furi_string_get_cstr(preset_str);
        if(furi_string_equal_str(preset_str, "FuriHalSubGhzPresetCustom")) {
            // The .sub file contains Custom_preset_module and Custom_preset_data
            // fields immediately after the Preset line.
            if(!subghz_setting_load_custom_preset(setting, preset_name_cstr, fff_file)) {
                FURI_LOG_E(TAG, "Failed to load custom preset from file");
                break;
            }
        }

        // Retrieve the preset register dump so we can program the CC1101.
        uint8_t* preset_data = subghz_setting_get_preset_data_by_name(
            setting, preset_name_cstr);
        if(!preset_data) {
            // Fall back to the default AM650 preset data if nothing was loaded.
            FURI_LOG_W(TAG, "Custom preset data unavailable, using Honda1 hardcoded preset");
            preset_data = const_cast<uint8_t*>(HONDA_PRESET_HONDA1);
        }

        // Build an in-memory FlipperFormat stream for the RAW transmitter.
        FlipperFormat* fff_data = flipper_format_string_alloc();
        subghz_protocol_raw_gen_fff_data(fff_data, sub_file_path);

        SubGhzTransmitter* tx =
            subghz_transmitter_alloc_init(env, SUBGHZ_PROTOCOL_RAW_NAME);
        if(!tx) {
            FURI_LOG_E(TAG, "Failed to allocate RAW transmitter");
            flipper_format_free(fff_data);
            break;
        }
        if(!subghz_transmitter_deserialize(tx, fff_data)) {
            FURI_LOG_E(TAG, "Failed to deserialize RAW data");
            subghz_transmitter_free(tx);
            flipper_format_free(fff_data);
            break;
        }

        // Program the CC1101 with the Honda custom modulation settings.
        furi_hal_subghz_reset();
        furi_hal_subghz_idle();
        furi_hal_subghz_load_custom_preset(preset_data);
        furi_hal_gpio_init(&gpio_cc1101_g0, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);
        furi_hal_gpio_write(&gpio_cc1101_g0, true);

        furi_hal_subghz_idle();
        furi_hal_subghz_set_frequency_and_path(frequency);

        if(!furi_hal_subghz_tx()) {
            FURI_LOG_E(TAG, "Failed to start TX");
            furi_hal_subghz_idle();
            subghz_transmitter_free(tx);
            flipper_format_free(fff_data);
            break;
        }

        furi_hal_subghz_start_async_tx(subghz_transmitter_yield, tx);
        while(!furi_hal_subghz_is_async_tx_complete()) {
            furi_delay_ms(10);
        }
        furi_hal_subghz_stop_async_tx();

        subghz_transmitter_stop(tx);
        subghz_transmitter_free(tx);
        flipper_format_free(fff_data);

        furi_hal_subghz_sleep();
        result = true;
    } while(false);

    furi_string_free(file_type);
    furi_string_free(preset_str);
    subghz_setting_free(setting);
    flipper_format_free(fff_file);
    furi_record_close(RECORD_STORAGE);

    return result;
}

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

void honda_subghz_init(HondaKeyFobApp* app) {
    furi_assert(app);
    app->subghz_env     = subghz_environment_alloc();
    app->tx_preset_name = furi_string_alloc();
    furi_hal_subghz_sleep();
}

void honda_subghz_deinit(HondaKeyFobApp* app) {
    furi_assert(app);
    furi_hal_subghz_sleep();
    if(app->subghz_env) {
        subghz_environment_free(app->subghz_env);
        app->subghz_env = nullptr;
    }
    if(app->tx_preset_name) {
        furi_string_free(app->tx_preset_name);
        app->tx_preset_name = nullptr;
    }
}

bool honda_subghz_transmit_file(HondaKeyFobApp* app, const char* sub_file_path) {
    furi_assert(app);
    furi_assert(sub_file_path);
    return honda_subghz_load_and_transmit(app->subghz_env, sub_file_path);
}
