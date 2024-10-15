#include "gundo.hh"

#include "game.hh"
#include "tms/backend/print.h"
#include "world.hh"
#include <cstdlib>

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

void undo_stack::checkpoint(const char *reason) {
    W->save(SAVE_TYPE_UNDO);

    void *data_copy = malloc(W->lb.size);
    memcpy(data_copy, W->lb.buf, W->lb.size);

    struct undo_item item = { reason, data_copy, W->lb.size };
    this->items.push_back(item);

    tms_debugf(
        "undo_checkpoint: item: %lu bytes; ptr %p",
        item.size, item.data
    );

    if (this->items.size() > MAX_UNDO_ITEMS) {
        const void *data = this->items.front().data;
        free((void *)data);
        this->items.erase(this->items.begin());
    }

#ifdef DEBUG
    size_t total_size_bytes = 0;
    for (const struct undo_item &item : this->items) {
        total_size_bytes += item.size;
    }
    tms_debugf(
        "undo_checkpoint: total size: %lu bytes\n",
        total_size_bytes
    );
#endif

    tms_printf(
        "undo_checkpoint: histsize: %lu/%u\n",
        this->items.size(), MAX_UNDO_ITEMS
    );
}

const char* undo_stack::restore() {
    if (this->items.size() == 0) {
        tms_fatalf("undo_load: no items to load");
    }
    struct undo_item &item = this->items.back();
    this->items.pop_back();

    G->open_sandbox_snapshot_mem(item.data, item.size);

    tms_infof("Restored level from undo stack");

    free(item.data);
    return item.reason;
}
