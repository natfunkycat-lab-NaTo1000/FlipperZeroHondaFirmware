#include "string.h"

#include <furi.h>
#include <m-string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// ──────────────────────────────────────────────────────────────────────────────
// Internal layout
// ──────────────────────────────────────────────────────────────────────────────

struct FuriString {
    string_t str; /**< M*LIB string backing store */
};

// ──────────────────────────────────────────────────────────────────────────────
// Allocation
// ──────────────────────────────────────────────────────────────────────────────

FuriString* furi_string_alloc(void) {
    FuriString* s = malloc(sizeof(FuriString));
    furi_assert(s);
    string_init(s->str);
    return s;
}

FuriString* furi_string_alloc_set(const FuriString* source) {
    furi_assert(source);
    FuriString* s = malloc(sizeof(FuriString));
    furi_assert(s);
    string_init_set(s->str, source->str);
    return s;
}

FuriString* furi_string_alloc_set_str(const char* cstr) {
    furi_assert(cstr);
    FuriString* s = malloc(sizeof(FuriString));
    furi_assert(s);
    string_init_set_str(s->str, cstr);
    return s;
}

void furi_string_free(FuriString* string) {
    furi_assert(string);
    string_clear(string->str);
    free(string);
}

// ──────────────────────────────────────────────────────────────────────────────
// Mutation
// ──────────────────────────────────────────────────────────────────────────────

void furi_string_set(FuriString* string, const FuriString* source) {
    furi_assert(string);
    furi_assert(source);
    string_set(string->str, source->str);
}

void furi_string_set_str(FuriString* string, const char* cstr) {
    furi_assert(string);
    furi_assert(cstr);
    string_set_str(string->str, cstr);
}

void furi_string_cat(FuriString* string, const FuriString* addition) {
    furi_assert(string);
    furi_assert(addition);
    string_cat(string->str, addition->str);
}

void furi_string_cat_str(FuriString* string, const char* cstr) {
    furi_assert(string);
    furi_assert(cstr);
    string_cat_str(string->str, cstr);
}

void furi_string_printf(FuriString* string, const char* format, ...) {
    furi_assert(string);
    furi_assert(format);
    va_list args;
    va_start(args, format);
    string_vprintf(string->str, format, args);
    va_end(args);
}

void furi_string_cat_printf(FuriString* string, const char* format, ...) {
    furi_assert(string);
    furi_assert(format);

    // Measure required buffer size
    va_list args;
    va_start(args, format);
    int needed = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if(needed <= 0) return;

    char* buf = malloc((size_t)needed + 1);
    furi_assert(buf);
    va_start(args, format);
    vsnprintf(buf, (size_t)needed + 1, format, args);
    va_end(args);

    string_cat_str(string->str, buf);
    free(buf);
}

void furi_string_reset(FuriString* string) {
    furi_assert(string);
    string_reset(string->str);
}

void furi_string_push_char(FuriString* string, char c) {
    furi_assert(string);
    char buf[2] = {c, '\0'};
    string_cat_str(string->str, buf);
}

void furi_string_push_utf8_codepoint(FuriString* string, uint32_t codepoint) {
    furi_assert(string);
    string_push_u(string->str, (string_unicode_t)codepoint);
}

// ──────────────────────────────────────────────────────────────────────────────
// Query
// ──────────────────────────────────────────────────────────────────────────────

const char* furi_string_get_cstr(const FuriString* string) {
    furi_assert(string);
    return string_get_cstr(string->str);
}

size_t furi_string_size(const FuriString* string) {
    furi_assert(string);
    return string_size(string->str);
}

size_t furi_string_utf8_length(FuriString* string) {
    furi_assert(string);
    return string_length_u(string->str);
}

bool furi_string_empty(const FuriString* string) {
    furi_assert(string);
    return string_empty_p(string->str);
}

int furi_string_cmp(const FuriString* a, const FuriString* b) {
    furi_assert(a);
    furi_assert(b);
    return string_cmp(a->str, b->str);
}

int furi_string_cmp_str(const FuriString* string, const char* cstr) {
    furi_assert(string);
    furi_assert(cstr);
    return string_cmp_str(string->str, cstr);
}

bool furi_string_equal(const FuriString* a, const FuriString* b) {
    furi_assert(a);
    furi_assert(b);
    return string_equal_p(a->str, b->str);
}

bool furi_string_equal_str(const FuriString* string, const char* cstr) {
    furi_assert(string);
    furi_assert(cstr);
    return string_equal_str_p(string->str, cstr);
}

bool furi_string_start_with_str(const FuriString* string, const char* cstr) {
    furi_assert(string);
    furi_assert(cstr);
    return string_start_with_str_p(string->str, cstr);
}

bool furi_string_end_with_str(const FuriString* string, const char* cstr) {
    furi_assert(string);
    furi_assert(cstr);
    return string_end_with_str_p(string->str, cstr);
}
