#include "honda_main_view.hpp"

#include <gui/view.h>
#include <gui/elements.h>
#include <input/input.h>
#include <furi.h>

// ──────────────────────────────────────────────────────────────────────────────
// View model
// ──────────────────────────────────────────────────────────────────────────────

struct HondaMainViewModel {
    uint8_t selected_item;  // 0 = Lock, 1 = Unlock, 2 = About
};

// ──────────────────────────────────────────────────────────────────────────────
// Menu items
// ──────────────────────────────────────────────────────────────────────────────

static constexpr uint8_t MENU_ITEM_COUNT = 3;

static const char* const MENU_LABELS[MENU_ITEM_COUNT] = {
    "Lock Honda",
    "Unlock Honda",
    "About",
};

// ──────────────────────────────────────────────────────────────────────────────
// Draw callback
// ──────────────────────────────────────────────────────────────────────────────

static void honda_main_view_draw_cb(Canvas* canvas, void* model_ptr) {
    auto* model = static_cast<HondaMainViewModel*>(model_ptr);

    canvas_clear(canvas);

    // Title bar
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 4, AlignCenter, AlignTop, "Honda Key Fob");
    canvas_draw_line(canvas, 0, 14, 128, 14);

    // Menu items
    canvas_set_font(canvas, FontSecondary);
    for(uint8_t i = 0; i < MENU_ITEM_COUNT; i++) {
        const uint8_t y = static_cast<uint8_t>(22 + i * 14);
        if(model->selected_item == i) {
            canvas_draw_rbox(canvas, 0, y - 2, 128, 13, 3);
            canvas_invert_color(canvas);
        }
        canvas_draw_str_aligned(canvas, 64, y, AlignCenter, AlignTop, MENU_LABELS[i]);
        if(model->selected_item == i) {
            canvas_invert_color(canvas);
        }
    }

    // Bottom hint
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 61, AlignCenter, AlignBottom, "OK to select");
}

// ──────────────────────────────────────────────────────────────────────────────
// Input callback
// ──────────────────────────────────────────────────────────────────────────────

static bool honda_main_view_input_cb(InputEvent* event, void* context) {
    furi_assert(context);
    auto* instance = static_cast<HondaMainView*>(context);

    if(event->type != InputTypeShort && event->type != InputTypeRepeat) {
        return false;
    }

    bool consumed = true;

    with_view_model(
        instance->view,
        HondaMainViewModel * model,
        {
            if(event->key == InputKeyUp) {
                if(model->selected_item > 0) {
                    model->selected_item--;
                }
            } else if(event->key == InputKeyDown) {
                if(model->selected_item < MENU_ITEM_COUNT - 1) {
                    model->selected_item++;
                }
            } else if(event->key == InputKeyOk) {
                view_dispatcher_send_custom_event(
                    // Custom event value maps directly to HondaMenuEvent
                    view_get_context(instance->view),
                    static_cast<uint32_t>(model->selected_item));
            } else if(event->key == InputKeyBack) {
                consumed = false;
            } else {
                consumed = false;
            }
            instance->selected_item = model->selected_item;
        },
        true);

    return consumed;
}

// ──────────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────────

HondaMainView* honda_main_view_alloc() {
    auto* instance = new HondaMainView{};

    instance->view = view_alloc();
    view_set_context(instance->view, instance);
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(HondaMainViewModel));
    view_set_draw_callback(instance->view, honda_main_view_draw_cb);
    view_set_input_callback(instance->view, honda_main_view_input_cb);

    with_view_model(
        instance->view,
        HondaMainViewModel * model,
        { model->selected_item = 0; },
        false);

    instance->selected_item = 0;

    return instance;
}

void honda_main_view_free(HondaMainView* instance) {
    furi_assert(instance);
    view_free(instance->view);
    delete instance;
}

View* honda_main_view_get_view(HondaMainView* instance) {
    furi_assert(instance);
    return instance->view;
}
