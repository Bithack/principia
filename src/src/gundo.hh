#pragma once

#include <cstddef>
#include <vector>

#define MAX_UNDO_ITEMS 100

struct undo_item {
    void *data;
    size_t size;
};

struct undo_stack {
    private:
        std::vector<undo_item> items;

    public:
        // Do not use this function directly, use P.add_action(ACTION_UNDO_RESET, 0) instead
        void reset();
        // Do not use this function directly, use P.add_action(ACTION_UNDO_CHECKPOINT, 0) instead
        void checkpoint();
        // Do not use this function directly, use P.add_action(ACTION_UNDO_RESTORE, 0) instead
        void restore();
};

extern struct undo_stack undo;

