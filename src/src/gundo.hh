#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <SDL_thread.h>
#include <condition_variable>
#include <mutex>

#define MAX_UNDO_ITEMS 100
// TODO make background compression optional

struct undo_item {
    const char *reason;
    void *data;
    size_t size;
    size_t size_uncompressed;
    bool compressed;
    std::mutex mutex;
};

enum {
    UNDO_KEEP_NONE      = 0b00,
    UNDO_KEEP_CAM_POS   = 0b01,
    UNDO_KEEP_SELECTION = 0b10,
    UNDO_KEEP_ALL       = 0b11,
};

enum {
    WORKER_IDLE = 0,
    WORKER_RUN  = 1,
    WORKER_KYS  = 2,
};

static int _thread_worker_main(void *data);

struct undo_stack {
    // XXX: this should be private
    private:
        std::mutex m_items;
        std::vector<undo_item*> items;

        SDL_Thread *compressor_thread = nullptr;

        std::mutex m_run_compressor;
        uint8_t run_compressor = WORKER_IDLE;
        std::condition_variable c_run_compressor;

        void signal_to_run_compressor();
        void _ensure_thread_running();
    public:
        undo_stack();
        ~undo_stack();

        // Implementation detail, do not use
        void _thread_worker();

        // Start the background compression thread
        void start_thread();

        // Kill the background compression thread
        void kill_thread();

        // Get the number of items in the undo stack
        size_t amount();

        // Reset the undo stack
        // Call this while loading a new level
        //
        // Avoid using this function directly if possible, use P.add_action(ACTION_UNDO_RESET, 0) instead
        void reset();

        // Create a snapshot of the current level state
        void* snapshot_state();

        // Save the current level state to the undo stack
        //
        // Avoid using this function directly if possible, use P.add_action(ACTION_UNDO_CHECKPOINT, reason) instead
        // Although, if you're calling it right before the state of the level changes, you MUST call it directly
        void checkpoint(const char* reason = nullptr, void* snapshot = nullptr);

        // Restore the last saved level state from the undo stack
        // Returns the reason for the last checkpoint
        //
        // Avoid using this function directly, use P.add_action(ACTION_UNDO_RESTORE, 0) instead
        const char* restore(uint8_t flags = UNDO_KEEP_ALL);
};

extern struct undo_stack undo;

