#pragma once

#include <cstddef>
#include <vector>

#define MAX_UNDO_ITEMS 100

struct undo_item {
    const char *reason;
    void *data;
    size_t size;
};

struct undo_stack {
    private:
        std::vector<undo_item> items;

    public:
        // Get the number of items in the undo stack
        size_t amount();


        // Reset the undo stack
        // Call this while loading a new level
        //
        // Avoid using this function directly if possible, use P.add_action(ACTION_UNDO_RESET, 0) instead
        void reset();

        // Save the current level state to the undo stack
        //
        // Avoid using this function directly if possible, use P.add_action(ACTION_UNDO_CHECKPOINT, reason) instead
        // Although, if you're calling it right before the state of the level changes, you MUST call it directly
        void checkpoint(const char* reason = nullptr);

        // Restore the last saved level state from the undo stack
        // Returns the reason for the last checkpoint
        //
        // Avoid using this function directly, use P.add_action(ACTION_UNDO_RESTORE, 0) instead
        const char* restore();
};

extern struct undo_stack undo;

