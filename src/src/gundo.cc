#include "gundo.hh"

#include "game.hh"
#include "tms/backend/print.h"
#include "world.hh"
#include <cstdint>
#include <cstdlib>

#ifdef DEBUG
#include <chrono>
#endif

// TODO griffi-gh: call reset on level load
// TODO griffi-gh: support redo
// TODO griffi-gh: compress undo items?

struct undo_stack undo;

size_t undo_stack::amount() {
    return this->items.size();
}

void undo_stack::reset() {
    this->items.clear();
}

void* undo_stack::snapshot_state() {
    W->save(SAVE_TYPE_UNDO);

    void *data_copy = malloc(W->lb.size);
    memcpy(data_copy, W->lb.buf, W->lb.size);

    return data_copy;
}

void undo_stack::checkpoint(const char *reason, void *snapshot /* = nullptr */) {
#ifdef DEBUG
    auto started = std::chrono::high_resolution_clock::now();
#endif

    if (snapshot == nullptr) {
        snapshot = this->snapshot_state();
    }

    struct undo_item item = { reason, snapshot, W->lb.size };
    this->items.push_back(item);

    if (this->items.size() > MAX_UNDO_ITEMS) {
        const void *data = this->items.front().data;
        free((void *)data);
        this->items.erase(this->items.begin());
    }

#ifdef DEBUG
    auto done = std::chrono::high_resolution_clock::now();
    size_t total_size_bytes = 0;
    for (const struct undo_item &item : this->items) {
        total_size_bytes += item.size;
    }
    tms_debugf(
        "undo_checkpoint:\n"
        " - item: %lu bytes; ptr %p\n"
        " - total size: %lu bytes\n"
        " - histsize: %lu/%u\n"
        " - time: %lu ns\n",
        item.size, item.data,
        total_size_bytes,
        this->items.size(), MAX_UNDO_ITEMS,
        std::chrono::duration_cast<std::chrono::nanoseconds>(done - started).count()
    );
#endif

    tms_printf(
        "undo_checkpoint: histsize: %lu/%u\n",
        this->items.size(), MAX_UNDO_ITEMS
    );
}

const char* undo_stack::restore(uint8_t flags) {
    // Save the current camera position
    float cx, cy, cz;
    if (flags & UNDO_KEEP_CAM_POS) {
        cx = G->cam->_position.x;
        cy = G->cam->_position.y;
        cz = G->cam->_position.z;
    }

    // Save the current selection
    uint32_t saved_selection_id = 0;
    if (flags & UNDO_KEEP_SELECTION) {
        if (G->selection.e != nullptr) {
            saved_selection_id = G->selection.e->id;
        } else {
            tms_warnf("undo_restore: no selection to keep");
        }
    }

    if (this->items.size() == 0) {
        tms_fatalf("undo_load: no items to load");
    }
    struct undo_item &item = this->items.back();
    this->items.pop_back();

    G->open_sandbox_snapshot_mem(item.data, item.size);

    tms_infof("Restored level from undo stack");

    free(item.data);

    if (flags & UNDO_KEEP_CAM_POS) {
        G->cam->set_position(cx, cy, cz);
    }
    if (flags & UNDO_KEEP_SELECTION) {
        G->selection.disable();
        if (saved_selection_id) {
            if (entity *e = W->get_entity_by_id(saved_selection_id)) {
                G->selection.select(e);
            } else {
                tms_warnf("undo_restore: selection entity not found");
            }
        }
    }

    return item.reason;
}
