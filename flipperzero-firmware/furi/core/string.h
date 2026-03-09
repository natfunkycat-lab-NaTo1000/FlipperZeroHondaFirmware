#pragma once

/**
 * FuriString – a heap-allocated, growable UTF-8 string.
 *
 * This header provides the FuriString type and its associated API, aligning
 * this firmware with the modern Flipper Zero SDK convention.  Internally the
 * type is backed by M*LIB's string_t so existing code that uses string_t
 * continues to build; new code should use FuriString* exclusively.
 *
 * Porting guide (old → new):
 *   string_t s;            → FuriString* s = furi_string_alloc();
 *   string_init(s);        → (handled by furi_string_alloc)
 *   string_clear(s);       → furi_string_free(s);
 *   string_set(s, other);  → furi_string_set(s, other);
 *   string_set_str(s, c);  → furi_string_set_str(s, c);
 *   string_get_cstr(s);    → furi_string_get_cstr(s);
 *   string_printf(s, …);   → furi_string_printf(s, …);
 *   string_cat(s, other);  → furi_string_cat(s, other);
 *   string_reset(s);       → furi_string_reset(s);
 *   string_cmp(a, b);      → furi_string_cmp(a, b);
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <m-string.h>

#ifdef __cplusplus
extern "C" {
#endif

// ──────────────────────────────────────────────────────────────────────────────
// Opaque type
// ──────────────────────────────────────────────────────────────────────────────

typedef struct FuriString FuriString;

// ──────────────────────────────────────────────────────────────────────────────
// Allocation
// ──────────────────────────────────────────────────────────────────────────────

/** Allocate a new, empty FuriString. */
FuriString* furi_string_alloc(void);

/** Allocate a FuriString pre-filled with @p source's content. */
FuriString* furi_string_alloc_set(const FuriString* source);

/** Allocate a FuriString pre-filled with the C string @p cstr. */
FuriString* furi_string_alloc_set_str(const char* cstr);

/** Free a FuriString allocated with any of the furi_string_alloc* functions. */
void furi_string_free(FuriString* string);

// ──────────────────────────────────────────────────────────────────────────────
// Mutation
// ──────────────────────────────────────────────────────────────────────────────

/** Replace contents with @p source. */
void furi_string_set(FuriString* string, const FuriString* source);

/** Replace contents with the C string @p cstr. */
void furi_string_set_str(FuriString* string, const char* cstr);

/** Append @p addition to the end of @p string. */
void furi_string_cat(FuriString* string, const FuriString* addition);

/** Append the C string @p cstr to the end of @p string. */
void furi_string_cat_str(FuriString* string, const char* cstr);

/**
 * Replace contents with a printf-formatted value.
 * @note Passing @p string as both the destination and a format argument
 *       results in undefined behavior.
 */
void furi_string_printf(FuriString* string, const char* format, ...)
    __attribute__((format(printf, 2, 3)));

/** Append a printf-formatted value to the end of @p string. */
void furi_string_cat_printf(FuriString* string, const char* format, ...)
    __attribute__((format(printf, 2, 3)));

/** Clear the string to an empty value (capacity is retained). */
void furi_string_reset(FuriString* string);

// ──────────────────────────────────────────────────────────────────────────────
// Query
// ──────────────────────────────────────────────────────────────────────────────

/** Return a read-only pointer to the NUL-terminated C string. */
const char* furi_string_get_cstr(const FuriString* string);

/** Return the number of bytes (not codepoints) stored in the string. */
size_t furi_string_size(const FuriString* string);

/** Return true if the string is empty. */
bool furi_string_empty(const FuriString* string);

/**
 * Lexicographic comparison.
 * @return < 0 if a < b, 0 if a == b, > 0 if a > b.
 */
int furi_string_cmp(const FuriString* a, const FuriString* b);

/** Lexicographic comparison against a C string. */
int furi_string_cmp_str(const FuriString* string, const char* cstr);

/** Return true if @p a and @p b have the same content. */
bool furi_string_equal(const FuriString* a, const FuriString* b);

/** Return true if @p string equals the C string @p cstr. */
bool furi_string_equal_str(const FuriString* string, const char* cstr);

/** Return true if @p string starts with the C string @p cstr. */
bool furi_string_start_with_str(const FuriString* string, const char* cstr);

/** Return true if @p string ends with the C string @p cstr. */
bool furi_string_end_with_str(const FuriString* string, const char* cstr);

#ifdef __cplusplus
}
#endif
