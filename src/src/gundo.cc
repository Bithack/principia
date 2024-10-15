#include "gundo.hh"

#include "game.hh"
#include "main.hh"
#include "tms/backend/print.h"
#include "world.hh"

// TODO griffi-gh: compress undo items
// TODO griffi-gh: support redo

struct undo_stack undo;

void undo_stack::reset() {
    this->items.clear();
}

void undo_stack::checkpoint() {
    W->save(SAVE_TYPE_UNDO);

    void *data_copy = malloc(W->lb.size);
    memcpy(data_copy, W->lb.buf, W->lb.size);

    struct undo_item item = {data_copy, W->lb.size};
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

    tms_printf(
        "undo_checkpoint: histsize: %lu/%u\n",
        this->items.size(), MAX_UNDO_ITEMS
    );
}

void undo_stack::restore() {
    if (this->items.size() == 0) {
        tms_fatalf("undo_load: no items to load");
    }
    struct undo_item &item = this->items.back();
    this->items.pop_back();

    G->open_sandbox_snapshot_mem(item.data, item.size);

    tms_infof("Restored level from undo stack");
}
