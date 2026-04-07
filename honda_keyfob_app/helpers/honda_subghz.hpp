#pragma once

#include "../honda_keyfob_app.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// Honda SubGHz helper – manages the CC1101 transceiver for Honda replays.
//
// All functions are free functions (no class state) to keep the API simple and
// compatible with the Flipper Zero C SDK.
// ──────────────────────────────────────────────────────────────────────────────

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialise the SubGHz environment used by the Honda app.
 *
 * Allocates a SubGhzEnvironment and ensures the CC1101 starts in sleep mode.
 *
 * @param  app  Pointer to the application state struct.
 */
void honda_subghz_init(HondaKeyFobApp* app);

/**
 * @brief  Release all SubGHz resources held by the Honda app.
 *
 * @param  app  Pointer to the application state struct.
 */
void honda_subghz_deinit(HondaKeyFobApp* app);

/**
 * @brief  Transmit the RAW signal stored in a .sub file on the SD card.
 *
 * The function:
 *  1. Opens the .sub file from @p sub_file_path.
 *  2. Reads the frequency and the inline Honda custom CC1101 preset.
 *  3. Loads the preset into the CC1101 hardware via furi_hal_subghz_*.
 *  4. Starts async TX and waits for completion.
 *  5. Returns the CC1101 to idle/sleep.
 *
 * @param  app           Application state (provides SubGHz environment).
 * @param  sub_file_path Absolute path to the .sub capture file.
 * @return true on success, false on any error (file missing, bad preset, etc.).
 */
bool honda_subghz_transmit_file(HondaKeyFobApp* app, const char* sub_file_path);

#ifdef __cplusplus
}
#endif
