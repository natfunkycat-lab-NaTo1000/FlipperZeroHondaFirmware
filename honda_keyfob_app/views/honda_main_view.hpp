#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <gui/view.h>
#include <gui/elements.h>
#include <furi.h>

#ifdef __cplusplus
}
#endif

#include <cstdint>

// ──────────────────────────────────────────────────────────────────────────────
// Events emitted by the main menu view
// ──────────────────────────────────────────────────────────────────────────────

enum class HondaMenuEvent : uint32_t {
    Lock   = 0,
    Unlock = 1,
    About  = 2,
};

// ──────────────────────────────────────────────────────────────────────────────
// HondaMainView – the primary interactive view for the Honda Key Fob app.
//
// Renders a simple lock / unlock / about menu and dispatches custom events
// back to the ViewDispatcher.
// ──────────────────────────────────────────────────────────────────────────────

struct HondaMainView {
    View*     view;
    uint8_t   selected_item;  // 0 = Lock, 1 = Unlock, 2 = About
};

// Lifecycle ------------------------------------------------------------------

HondaMainView* honda_main_view_alloc();
void           honda_main_view_free(HondaMainView* instance);

// Accessors ------------------------------------------------------------------

View* honda_main_view_get_view(HondaMainView* instance);
