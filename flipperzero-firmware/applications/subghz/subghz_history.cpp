/**
 * SubGHz signal history – C++ rewrite.
 *
 * Replaces M*LIB arrays and string_t with std::vector<> and FuriString* to
 * align with the modern Flipper Zero SDK conventions.
 */

#include "subghz_history.h"
#include <lib/subghz/receiver.h>
#include <lib/subghz/protocols/came.h>

#include <furi.h>

#include <vector>
#include <cstring>

#define SUBGHZ_HISTORY_MAX 50
#define TAG "SubGhzHistory"

// ──────────────────────────────────────────────────────────────────────────────
// Internal types
// ──────────────────────────────────────────────────────────────────────────────

struct SubGhzHistoryItem {
    FuriString*             item_str{nullptr};
    FlipperFormat*          flipper_string{nullptr};
    uint8_t                 type{0};
    SubGhzPresetDefinition* preset{nullptr};
};

struct SubGhzHistory {
    uint32_t                        last_update_timestamp{0};
    uint16_t                        last_index_write{0};
    uint8_t                         code_last_hash_data{0};
    FuriString*                     tmp_string{nullptr};
    std::vector<SubGhzHistoryItem>  items;
};

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

static void subghz_history_item_free(SubGhzHistoryItem& item) {
    if(item.item_str) {
        furi_string_free(item.item_str);
        item.item_str = nullptr;
    }
    if(item.preset) {
        if(item.preset->name) {
            furi_string_free(item.preset->name);
        }
        free(item.preset);
        item.preset = nullptr;
    }
    if(item.flipper_string) {
        flipper_format_free(item.flipper_string);
        item.flipper_string = nullptr;
    }
    item.type = 0;
}

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

SubGhzHistory* subghz_history_alloc(void) {
    SubGhzHistory* instance = new SubGhzHistory{};
    instance->tmp_string = furi_string_alloc();
    return instance;
}

void subghz_history_free(SubGhzHistory* instance) {
    furi_assert(instance);
    furi_string_free(instance->tmp_string);
    for(auto& item : instance->items) {
        subghz_history_item_free(item);
    }
    delete instance;
}

void subghz_history_reset(SubGhzHistory* instance) {
    furi_assert(instance);
    furi_string_reset(instance->tmp_string);
    for(auto& item : instance->items) {
        subghz_history_item_free(item);
    }
    instance->items.clear();
    instance->last_index_write   = 0;
    instance->code_last_hash_data = 0;
}

uint32_t subghz_history_get_frequency(SubGhzHistory* instance, uint16_t idx) {
    furi_assert(instance);
    return instance->items[idx].preset->frequency;
}

SubGhzPresetDefinition* subghz_history_get_preset_def(SubGhzHistory* instance, uint16_t idx) {
    furi_assert(instance);
    return instance->items[idx].preset;
}

const char* subghz_history_get_preset(SubGhzHistory* instance, uint16_t idx) {
    furi_assert(instance);
    return furi_string_get_cstr(instance->items[idx].preset->name);
}

uint16_t subghz_history_get_item(SubGhzHistory* instance) {
    furi_assert(instance);
    return instance->last_index_write;
}

uint8_t subghz_history_get_type_protocol(SubGhzHistory* instance, uint16_t idx) {
    furi_assert(instance);
    return instance->items[idx].type;
}

const char* subghz_history_get_protocol_name(SubGhzHistory* instance, uint16_t idx) {
    furi_assert(instance);
    flipper_format_rewind(instance->items[idx].flipper_string);
    if(!flipper_format_read_string(
           instance->items[idx].flipper_string, "Protocol", instance->tmp_string)) {
        FURI_LOG_E(TAG, "Missing Protocol");
        furi_string_reset(instance->tmp_string);
    }
    return furi_string_get_cstr(instance->tmp_string);
}

FlipperFormat* subghz_history_get_raw_data(SubGhzHistory* instance, uint16_t idx) {
    furi_assert(instance);
    return instance->items[idx].flipper_string;
}

bool subghz_history_get_text_space_left(SubGhzHistory* instance, FuriString* output) {
    furi_assert(instance);
    if(instance->last_index_write == SUBGHZ_HISTORY_MAX) {
        if(output != nullptr) furi_string_set_str(output, "Memory is FULL");
        return true;
    }
    if(output != nullptr) {
        furi_string_printf(
            output, "%02u/%02u", instance->last_index_write, SUBGHZ_HISTORY_MAX);
    }
    return false;
}

void subghz_history_get_text_item_menu(
    SubGhzHistory* instance,
    FuriString*    output,
    uint16_t       idx) {
    furi_assert(instance);
    furi_string_set(output, instance->items[idx].item_str);
}

bool subghz_history_add_to_history(
    SubGhzHistory*          instance,
    void*                   context,
    SubGhzPresetDefinition* preset) {
    furi_assert(instance);
    furi_assert(context);

    if(instance->last_index_write >= SUBGHZ_HISTORY_MAX) return false;

    SubGhzProtocolDecoderBase* decoder_base = static_cast<SubGhzProtocolDecoderBase*>(context);

    if((instance->code_last_hash_data ==
        subghz_protocol_decoder_base_get_hash_data(decoder_base)) &&
       ((furi_get_tick() - instance->last_update_timestamp) < 500)) {
        instance->last_update_timestamp = furi_get_tick();
        return false;
    }

    instance->code_last_hash_data = subghz_protocol_decoder_base_get_hash_data(decoder_base);
    instance->last_update_timestamp = furi_get_tick();

    SubGhzHistoryItem new_item{};
    new_item.preset = static_cast<SubGhzPresetDefinition*>(malloc(sizeof(SubGhzPresetDefinition)));
    new_item.type   = decoder_base->protocol->type;
    new_item.preset->frequency  = preset->frequency;
    new_item.preset->name       = furi_string_alloc_set(preset->name);
    new_item.preset->data       = preset->data;
    new_item.preset->data_size  = preset->data_size;
    new_item.item_str           = furi_string_alloc();
    new_item.flipper_string     = flipper_format_string_alloc();

    subghz_protocol_decoder_base_serialize(decoder_base, new_item.flipper_string, preset);

    FuriString* text = furi_string_alloc();

    do {
        if(!flipper_format_rewind(new_item.flipper_string)) {
            FURI_LOG_E(TAG, "Rewind error");
            break;
        }
        if(!flipper_format_read_string(
               new_item.flipper_string, "Protocol", instance->tmp_string)) {
            FURI_LOG_E(TAG, "Missing Protocol");
            break;
        }
        if(furi_string_equal_str(instance->tmp_string, "KeeLoq")) {
            furi_string_set_str(instance->tmp_string, "KL ");
            if(!flipper_format_read_string(new_item.flipper_string, "Manufacture", text)) {
                FURI_LOG_E(TAG, "Missing Manufacture");
                break;
            }
            furi_string_cat(instance->tmp_string, text);
        } else if(furi_string_equal_str(instance->tmp_string, "Star Line")) {
            furi_string_set_str(instance->tmp_string, "SL ");
            if(!flipper_format_read_string(new_item.flipper_string, "Manufacture", text)) {
                FURI_LOG_E(TAG, "Missing Manufacture");
                break;
            }
            furi_string_cat(instance->tmp_string, text);
        }
        if(!flipper_format_rewind(new_item.flipper_string)) {
            FURI_LOG_E(TAG, "Rewind error");
            break;
        }
        uint8_t key_data[sizeof(uint64_t)] = {0};
        if(!flipper_format_read_hex(
               new_item.flipper_string, "Key", key_data, sizeof(uint64_t))) {
            FURI_LOG_E(TAG, "Missing Key");
            break;
        }
        uint64_t data = 0;
        for(uint8_t i = 0; i < sizeof(uint64_t); i++) {
            data = (data << 8) | key_data[i];
        }
        if(!(uint32_t)(data >> 32)) {
            furi_string_printf(
                new_item.item_str,
                "%s %lX",
                furi_string_get_cstr(instance->tmp_string),
                (uint32_t)(data & 0xFFFFFFFFU));
        } else {
            furi_string_printf(
                new_item.item_str,
                "%s %lX%08lX",
                furi_string_get_cstr(instance->tmp_string),
                (uint32_t)(data >> 32),
                (uint32_t)(data & 0xFFFFFFFFU));
        }
    } while(false);

    furi_string_free(text);

    instance->items.push_back(new_item);
    instance->last_index_write++;
    return true;
}
