/**
 * SubGHz settings – C++ rewrite.
 *
 * Replaces M*LIB arrays/lists and string_t with std::vector<> and FuriString*
 * to align with the modern Flipper Zero SDK conventions.
 */

#include "subghz_setting.h"
#include "subghz_i.h"

#include <furi.h>
#include "furi_hal_subghz_configs.h"

#include <vector>
#include <cstring>

#define TAG "SubGhzSetting"

#define SUBGHZ_SETTING_FILE_TYPE    "Flipper SubGhz Setting File"
#define SUBGHZ_SETTING_FILE_VERSION 1

#define FREQUENCY_FLAG_DEFAULT  (1UL << 31)
#define FREQUENCY_MASK          (0xFFFFFFFFUL ^ FREQUENCY_FLAG_DEFAULT)

// ──────────────────────────────────────────────────────────────────────────────
// Default frequency tables
// ──────────────────────────────────────────────────────────────────────────────

static const uint32_t subghz_frequency_list[] = {
    300000000, 303875000, 304250000, 310000000, 315000000, 318000000,
    390000000, 418000000, 433075000, 433420000,
    433920000 | FREQUENCY_FLAG_DEFAULT,
    434420000, 434775000, 438900000,
    868350000, 915000000, 925000000, 0,
};

static const uint32_t subghz_hopper_frequency_list[] = {
    310000000, 315000000, 318000000, 390000000, 433920000, 868350000, 0,
};

static const uint32_t subghz_frequency_list_region_eu_ru[] = {
    300000000, 303875000, 304250000, 310000000, 315000000, 318000000,
    390000000, 418000000, 433075000, 433420000,
    433920000 | FREQUENCY_FLAG_DEFAULT,
    434420000, 434775000, 438900000,
    868350000, 915000000, 925000000, 0,
};
static const uint32_t subghz_hopper_frequency_list_region_eu_ru[] = {
    310000000, 315000000, 318000000, 390000000, 433920000, 868350000, 0,
};

static const uint32_t subghz_frequency_list_region_us_ca_au[] = {
    300000000, 303875000, 304250000, 310000000, 315000000, 318000000,
    390000000, 418000000, 433075000, 433420000,
    433920000 | FREQUENCY_FLAG_DEFAULT,
    434420000, 434775000, 438900000,
    868350000, 915000000, 925000000, 0,
};
static const uint32_t subghz_hopper_frequency_list_region_us_ca_au[] = {
    310000000, 315000000, 318000000, 390000000, 433920000, 868350000, 0,
};

static const uint32_t subghz_frequency_list_region_jp[] = {
    300000000, 303875000, 304250000, 310000000, 315000000, 318000000,
    390000000, 418000000, 433075000, 433420000,
    433920000 | FREQUENCY_FLAG_DEFAULT,
    434420000, 434775000, 438900000,
    868350000, 915000000, 925000000, 0,
};
static const uint32_t subghz_hopper_frequency_list_region_jp[] = {
    310000000, 315000000, 318000000, 390000000, 433920000, 868350000, 0,
};

// ──────────────────────────────────────────────────────────────────────────────
// Internal types
// ──────────────────────────────────────────────────────────────────────────────

struct SubGhzSettingCustomPresetItem {
    FuriString* name{nullptr};
    uint8_t*    data{nullptr};
    size_t      data_size{0};
};

struct SubGhzSetting {
    std::vector<uint32_t>                    frequencies;
    std::vector<uint32_t>                    hopper_frequencies;
    std::vector<SubGhzSettingCustomPresetItem> presets;
};

// ──────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ──────────────────────────────────────────────────────────────────────────────

static void subghz_setting_preset_item_free(SubGhzSettingCustomPresetItem& item) {
    if(item.name) {
        furi_string_free(item.name);
        item.name = nullptr;
    }
    if(item.data) {
        free(item.data);
        item.data = nullptr;
    }
    item.data_size = 0;
}

static void subghz_setting_preset_reset(SubGhzSetting* instance) {
    for(auto& item : instance->presets) {
        subghz_setting_preset_item_free(item);
    }
    instance->presets.clear();
}

static void subghz_setting_load_default_preset(
    SubGhzSetting*    instance,
    const char*       preset_name,
    const uint8_t*    preset_data,
    const uint8_t     preset_pa_table[8]) {
    furi_assert(instance);
    furi_assert(preset_data);

    SubGhzSettingCustomPresetItem item{};
    item.name = furi_string_alloc_set_str(preset_name);

    uint32_t reg_count = 0;
    while(preset_data[reg_count]) {
        reg_count += 2;
    }
    reg_count += 2; // include the terminating 0x00/0x00 pair

    item.data_size = reg_count + 8; // registers + PA table
    item.data      = static_cast<uint8_t*>(malloc(item.data_size));
    memcpy(&item.data[0],         preset_data,     reg_count);
    memcpy(&item.data[reg_count], preset_pa_table, 8);

    instance->presets.push_back(item);
}

static void subghz_setting_load_default_region(
    SubGhzSetting*      instance,
    const uint32_t      frequencies[],
    const uint32_t      hopper_frequencies[]) {
    furi_assert(instance);

    instance->frequencies.clear();
    instance->hopper_frequencies.clear();
    subghz_setting_preset_reset(instance);

    for(; *frequencies; ++frequencies) {
        instance->frequencies.push_back(*frequencies);
    }
    for(; *hopper_frequencies; ++hopper_frequencies) {
        instance->hopper_frequencies.push_back(*hopper_frequencies);
    }

    subghz_setting_load_default_preset(
        instance, "AM270",
        reinterpret_cast<const uint8_t*>(furi_hal_subghz_preset_ook_270khz_async_regs),
        furi_hal_subghz_preset_ook_async_patable);
    subghz_setting_load_default_preset(
        instance, "AM650",
        reinterpret_cast<const uint8_t*>(furi_hal_subghz_preset_ook_650khz_async_regs),
        furi_hal_subghz_preset_ook_async_patable);
    subghz_setting_load_default_preset(
        instance, "FM238",
        reinterpret_cast<const uint8_t*>(furi_hal_subghz_preset_2fsk_dev2_38khz_async_regs),
        furi_hal_subghz_preset_2fsk_async_patable);
    subghz_setting_load_default_preset(
        instance, "FM476",
        reinterpret_cast<const uint8_t*>(furi_hal_subghz_preset_2fsk_dev47_6khz_async_regs),
        furi_hal_subghz_preset_2fsk_async_patable);
}

static void subghz_setting_load_default(SubGhzSetting* instance) {
    switch(furi_hal_version_get_hw_region()) {
    case FuriHalVersionRegionEuRu:
        subghz_setting_load_default_region(
            instance,
            subghz_frequency_list_region_eu_ru,
            subghz_hopper_frequency_list_region_eu_ru);
        break;
    case FuriHalVersionRegionUsCaAu:
        subghz_setting_load_default_region(
            instance,
            subghz_frequency_list_region_us_ca_au,
            subghz_hopper_frequency_list_region_us_ca_au);
        break;
    case FuriHalVersionRegionJp:
        subghz_setting_load_default_region(
            instance,
            subghz_frequency_list_region_jp,
            subghz_hopper_frequency_list_region_jp);
        break;
    default:
        subghz_setting_load_default_region(
            instance, subghz_frequency_list, subghz_hopper_frequency_list);
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

SubGhzSetting* subghz_setting_alloc(void) {
    return new SubGhzSetting{};
}

void subghz_setting_free(SubGhzSetting* instance) {
    furi_assert(instance);
    for(auto& item : instance->presets) {
        subghz_setting_preset_item_free(item);
    }
    delete instance;
}

void subghz_setting_load(SubGhzSetting* instance, const char* file_path) {
    furi_assert(instance);

    Storage*       storage       = static_cast<Storage*>(furi_record_open(RECORD_STORAGE));
    FlipperFormat* fff_data_file = flipper_format_file_alloc(storage);

    FuriString* temp_str = furi_string_alloc();
    uint32_t    temp_data32;
    bool        temp_bool;

    subghz_setting_load_default(instance);

    if(file_path) {
        do {
            if(!flipper_format_file_open_existing(fff_data_file, file_path)) {
                FURI_LOG_E(TAG, "Error open file %s", file_path);
                break;
            }
            if(!flipper_format_read_header(fff_data_file, temp_str, &temp_data32)) {
                FURI_LOG_E(TAG, "Missing or incorrect header");
                break;
            }
            if(furi_string_cmp_str(temp_str, SUBGHZ_SETTING_FILE_TYPE) != 0 ||
               temp_data32 != SUBGHZ_SETTING_FILE_VERSION) {
                FURI_LOG_E(TAG, "Type or version mismatch");
                break;
            }

            // Standard frequencies (optional)
            temp_bool = true;
            flipper_format_read_bool(fff_data_file, "Add_standard_frequencies", &temp_bool, 1);
            if(!temp_bool) {
                FURI_LOG_I(TAG, "Removing standard frequencies");
                instance->frequencies.clear();
                instance->hopper_frequencies.clear();
            } else {
                FURI_LOG_I(TAG, "Keeping standard frequencies");
            }

            // Load frequencies
            if(!flipper_format_rewind(fff_data_file)) {
                FURI_LOG_E(TAG, "Rewind error");
                break;
            }
            while(flipper_format_read_uint32(fff_data_file, "Frequency", &temp_data32, 1)) {
                if(furi_hal_subghz_is_frequency_valid(temp_data32)) {
                    FURI_LOG_I(TAG, "Frequency loaded %lu", temp_data32);
                    instance->frequencies.push_back(temp_data32);
                } else {
                    FURI_LOG_E(TAG, "Frequency not supported %lu", temp_data32);
                }
            }

            // Load hopper frequencies
            if(!flipper_format_rewind(fff_data_file)) {
                FURI_LOG_E(TAG, "Rewind error");
                break;
            }
            while(
                flipper_format_read_uint32(fff_data_file, "Hopper_frequency", &temp_data32, 1)) {
                if(furi_hal_subghz_is_frequency_valid(temp_data32)) {
                    FURI_LOG_I(TAG, "Hopper frequency loaded %lu", temp_data32);
                    instance->hopper_frequencies.push_back(temp_data32);
                } else {
                    FURI_LOG_E(TAG, "Hopper frequency not supported %lu", temp_data32);
                }
            }

            // Default frequency (optional)
            if(!flipper_format_rewind(fff_data_file)) {
                FURI_LOG_E(TAG, "Rewind error");
                break;
            }
            if(flipper_format_read_uint32(
                   fff_data_file, "Default_frequency", &temp_data32, 1)) {
                for(auto& freq : instance->frequencies) {
                    freq &= FREQUENCY_MASK;
                    if(freq == temp_data32) freq |= FREQUENCY_FLAG_DEFAULT;
                }
            }

            // Custom presets (optional)
            if(!flipper_format_rewind(fff_data_file)) {
                FURI_LOG_E(TAG, "Rewind error");
                break;
            }
            while(flipper_format_read_string(
                      fff_data_file, "Custom_preset_name", temp_str)) {
                FURI_LOG_I(TAG, "Custom preset loaded %s", furi_string_get_cstr(temp_str));
                subghz_setting_load_custom_preset(
                    instance, furi_string_get_cstr(temp_str), fff_data_file);
            }
        } while(false);
    }

    furi_string_free(temp_str);
    flipper_format_free(fff_data_file);
    furi_record_close(RECORD_STORAGE);

    if(instance->frequencies.empty() || instance->hopper_frequencies.empty()) {
        FURI_LOG_E(TAG, "Error loading user settings, loading defaults");
        subghz_setting_load_default(instance);
    }
}

size_t subghz_setting_get_frequency_count(SubGhzSetting* instance) {
    furi_assert(instance);
    return instance->frequencies.size();
}

size_t subghz_setting_get_hopper_frequency_count(SubGhzSetting* instance) {
    furi_assert(instance);
    return instance->hopper_frequencies.size();
}

size_t subghz_setting_get_preset_count(SubGhzSetting* instance) {
    furi_assert(instance);
    return instance->presets.size();
}

const char* subghz_setting_get_preset_name(SubGhzSetting* instance, size_t idx) {
    furi_assert(instance);
    return furi_string_get_cstr(instance->presets[idx].name);
}

int subghz_setting_get_inx_preset_by_name(SubGhzSetting* instance, const char* preset_name) {
    furi_assert(instance);
    for(size_t i = 0; i < instance->presets.size(); ++i) {
        if(furi_string_equal_str(instance->presets[i].name, preset_name)) {
            return static_cast<int>(i);
        }
    }
    furi_crash("SubGhz: No name preset.");
    return -1;
}

bool subghz_setting_load_custom_preset(
    SubGhzSetting* instance,
    const char*    preset_name,
    FlipperFormat* fff_data_file) {
    furi_assert(instance);
    furi_assert(preset_name);

    SubGhzSettingCustomPresetItem item{};
    item.name = furi_string_alloc_set_str(preset_name);

    uint32_t count = 0;
    do {
        if(!flipper_format_get_value_count(fff_data_file, "Custom_preset_data", &count)) break;
        if(!count || (count % 2)) {
            FURI_LOG_E(TAG, "Integrity error Custom_preset_data");
            break;
        }
        item.data_size = count;
        item.data      = static_cast<uint8_t*>(malloc(item.data_size));
        if(!flipper_format_read_hex(
               fff_data_file, "Custom_preset_data", item.data, item.data_size)) {
            FURI_LOG_E(TAG, "Missing Custom_preset_data");
            break;
        }
        instance->presets.push_back(item);
        return true;
    } while(false);

    subghz_setting_preset_item_free(item);
    return false;
}

bool subghz_setting_delete_custom_preset(SubGhzSetting* instance, const char* preset_name) {
    furi_assert(instance);
    furi_assert(preset_name);
    for(auto it = instance->presets.begin(); it != instance->presets.end(); ++it) {
        if(furi_string_equal_str(it->name, preset_name)) {
            subghz_setting_preset_item_free(*it);
            instance->presets.erase(it);
            return true;
        }
    }
    return false;
}

uint8_t* subghz_setting_get_preset_data(SubGhzSetting* instance, size_t idx) {
    furi_assert(instance);
    return instance->presets[idx].data;
}

size_t subghz_setting_get_preset_data_size(SubGhzSetting* instance, size_t idx) {
    furi_assert(instance);
    return instance->presets[idx].data_size;
}

uint8_t* subghz_setting_get_preset_data_by_name(
    SubGhzSetting* instance,
    const char*    preset_name) {
    furi_assert(instance);
    int idx = subghz_setting_get_inx_preset_by_name(instance, preset_name);
    return (idx >= 0) ? instance->presets[static_cast<size_t>(idx)].data : nullptr;
}

uint32_t subghz_setting_get_frequency(SubGhzSetting* instance, size_t idx) {
    furi_assert(instance);
    if(idx < instance->frequencies.size()) {
        return instance->frequencies[idx] & FREQUENCY_MASK;
    }
    return 0;
}

uint32_t subghz_setting_get_hopper_frequency(SubGhzSetting* instance, size_t idx) {
    furi_assert(instance);
    if(idx < instance->hopper_frequencies.size()) {
        return instance->hopper_frequencies[idx];
    }
    return 0;
}

uint32_t subghz_setting_get_frequency_default_index(SubGhzSetting* instance) {
    furi_assert(instance);
    for(size_t i = 0; i < instance->frequencies.size(); ++i) {
        if(instance->frequencies[i] & FREQUENCY_FLAG_DEFAULT) {
            return static_cast<uint32_t>(i);
        }
    }
    return 0;
}

uint32_t subghz_setting_get_default_frequency(SubGhzSetting* instance) {
    furi_assert(instance);
    return subghz_setting_get_frequency(
        instance, subghz_setting_get_frequency_default_index(instance));
}
