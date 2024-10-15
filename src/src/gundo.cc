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

    // if (W->lb.buf) {
    //     free(W->lb.buf);
    // }
    // W->lb.buf = (uint8_t*)item.data;
    W->lb.clear();
    W->lb.ensure(item.size);
    memcpy(W->lb.buf, item.data, item.size);
    W->lb.size = item.size;

    G->reset();
    W->open_internal(
        item.size,
        W->level_id_type,
        W->level.local_id,
        true,
        true,
        W->level.save_id,
        false,
        true
    );
    G->apply_level_properties();
    G->add_entities(&W->all_entities, &W->groups, &W->connections, &W->cables);
    W->begin();

    G->refresh_widgets();

    // W->cwindow->preloader.read_gentypes(&W->level, &W->lb);
    // W->cwindow->preloader.read_chunks(&W->level, &W->lb);
    // W->state_ptr = W->lb.rp;
    // W->lb.rp += W->level.state_size;

    // free(item.data); DO NOT!
    // P.add_action(ACTION_RELOAD_LEVEL, 0);
    // W->reset();

    tms_infof("Restored level from undo stack");
}
