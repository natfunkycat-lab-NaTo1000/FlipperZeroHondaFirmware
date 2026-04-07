#pragma once

#include <m-string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extract filename without extension from path.
 * 
 * @param path path string
 * @param filename output filename string. Must be initialized before.
 */
void path_extract_filename_no_ext(const char* path, string_t filename);

/**
 * @brief Extract filename string from path.
 * 
 * @param path path string
 * @param filename output filename string. Must be initialized before.
 * @param trim_ext true - get filename without extension
 */
void path_extract_filename(string_t path, string_t filename, bool trim_ext);

/**
 * @brief Extract file extension from path.
 * 
 * @param path path string
 * @param ext output extension string
 * @param ext_len_max maximum extension string length
 */
void path_extract_extension(string_t path, char* ext, size_t ext_len_max);

/**
 * @brief Extract last path component
 * 
 * @param path path string
 * @param filename output string. Must be initialized before.
 */
void path_extract_basename(const char* path, string_t basename);

/**
 * @brief Extract path, except for last component
 * 
 * @param path path string
 * @param filename output string. Must be initialized before.
 */
void path_extract_dirname(const char* path, string_t dirname);

/**
 * @brief Appends new component to path, adding path delimiter
 * 
 * @param path path string
 * @param suffix path part to apply
 */
void path_append(string_t path, const char* suffix);

/**
 * @brief Appends new component to path, adding path delimiter
 * 
 * @param path first path part
 * @param suffix second path part
 * @param out_path output string to combine parts into. Must be initialized
 */
void path_concat(const char* path, const char* suffix, string_t out_path);

/**
 * @brief Check that path contains only ascii characters
 * 
 * @param path 
 * @return true 
 * @return false 
 */
bool path_contains_only_ascii(const char* path);

#ifdef __cplusplus
}

// ──────────────────────────────────────────────────────────────────────────────
// C++ convenience overloads – accept FuriString* instead of string_t
// ──────────────────────────────────────────────────────────────────────────────

#include <furi/core/string.h>

inline void path_extract_filename(FuriString* path, FuriString* filename, bool trim_ext) {
    string_t tmp_path, tmp_filename;
    string_init_set_str(tmp_path, furi_string_get_cstr(path));
    string_init(tmp_filename);
    path_extract_filename(tmp_path, tmp_filename, trim_ext);
    furi_string_set_str(filename, string_get_cstr(tmp_filename));
    string_clear(tmp_path);
    string_clear(tmp_filename);
}

inline void path_extract_dirname(const char* path, FuriString* dirname) {
    string_t tmp;
    string_init(tmp);
    path_extract_dirname(path, tmp);
    furi_string_set_str(dirname, string_get_cstr(tmp));
    string_clear(tmp);
}

inline void path_extract_dirname(FuriString* path, FuriString* dirname) {
    path_extract_dirname(furi_string_get_cstr(path), dirname);
}

#endif // __cplusplus
