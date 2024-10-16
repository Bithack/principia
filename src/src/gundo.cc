#include "gundo.hh"

#include "game.hh"
#include "tms/backend/print.h"
#include "world.hh"

#include <SDL_thread.h>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <zlib.h>

#ifdef DEBUG
#include <chrono>
#endif

#define COMPRESSION_LEVEL 3

#define UNDOTHR_LOG "undo_thread_worker: "

// TODO griffi-gh: call reset on level load
// TODO griffi-gh: support redo
// TODO griffi-gh: compress undo items?

struct undo_stack undo;

undo_stack::undo_stack() {
    undo_stack::start_thread();
}

undo_stack::~undo_stack() {
    undo_stack::reset();
    undo_stack::kill_thread();
}

int _thread_worker(void *data) {
    undo_stack *stack = (undo_stack *)data;

    while (true) {
        std::unique_lock<std::mutex> run_compressor_lock(stack->m_run_compressor);
        stack->c_run_compressor.wait(run_compressor_lock, [stack] {
            return stack->run_compressor != WORKER_IDLE;
        });
        if (stack->run_compressor == WORKER_KYS) {
            stack->run_compressor = WORKER_IDLE;
            run_compressor_lock.unlock();
            break;
        } else {
            stack->run_compressor = WORKER_IDLE;
            run_compressor_lock.unlock();
        }

        tms_debugf(UNDOTHR_LOG "=== RUNNING COMPRESSOR ===");

        undo_item *item_to_compress = nullptr;

        while (true) {
            // Find the first uncompressed undo item
            tms_debugf(UNDOTHR_LOG "look for item to compress...");
            {
                std::lock_guard<std::mutex> lock(stack->m_items);
                for (struct undo_item *&item : stack->items) {
                    std::lock_guard<std::mutex> lock(item->mutex);
                    if (!item->compressed) {
                        item_to_compress = item;
                        break;
                    }
                }
            }


            // If no item was found, break the loop and wait for the next signal
            if (item_to_compress == nullptr) {
                tms_debugf(UNDOTHR_LOG "done compressing, waiting for next signal...");
                break;
            }

            tms_debugf(UNDOTHR_LOG "found item: %p", item_to_compress);

            // Lock the item and compress it in-place
            {
                std::lock_guard<std::mutex> lock(item_to_compress->mutex);

                size_t _original_size = item_to_compress->size;

                // 1. Allocate a temporary buffer for compression
                unsigned long compressed_size = compressBound(item_to_compress->size);
                unsigned char* temp_buffer = new unsigned char[compressed_size];

                // 2. Compress the data into the temporary buffer
                int result = compress2(
                    temp_buffer, &compressed_size,
                    (const Bytef*)item_to_compress->data, item_to_compress->size,
                    COMPRESSION_LEVEL
                );

                if (result != Z_OK) {
                    tms_fatalf(UNDOTHR_LOG "compression failed: %d", result);
                }

                // 3. Resize the item's data buffer
                item_to_compress->data = realloc(item_to_compress->data, compressed_size);
                item_to_compress->size = compressed_size;

                // 4. Copy compressed data back to the original buffer
                std::memcpy(item_to_compress->data, temp_buffer, compressed_size);
                delete[] temp_buffer;

                // mark the item as compressed
                item_to_compress->compressed = true;

                tms_debugf(UNDOTHR_LOG "undo_thread_worker: compressed %lu bytes to %lu bytes", _original_size, compressed_size);
                item_to_compress = nullptr;
            }
        }
    }

    return 0;
}

void undo_stack::signal_to_run_compressor() {
    tms_debugf("signal_to_run_compressor: enter");
    this->m_run_compressor.lock();
    this->run_compressor = true;
    this->m_run_compressor.unlock();
    this->c_run_compressor.notify_all();
    tms_debugf("signal_to_run_compressor: leave");
}

void undo_stack::start_thread() {
    if (this->compressor_thread != nullptr) {
        tms_warnf("undo_thread_worker: thread already running");
        return;
    }
    this->compressor_thread = SDL_CreateThread(_thread_worker, "undocomp_thr", this);
}

void undo_stack::kill_thread() {
    if (this->compressor_thread == nullptr) {
        tms_fatalf("thread not running");
    }

    this->m_run_compressor.lock();
    this->run_compressor = WORKER_KYS;
    this->m_run_compressor.unlock();
    this->c_run_compressor.notify_all();

    int status = 0;
    SDL_WaitThread(this->compressor_thread, &status);
    tms_debugf("undo_thread_worker: thread exited with status %d", status);

    this->compressor_thread = nullptr;
}

size_t undo_stack::amount() {
    std::lock_guard<std::mutex> guard(this->m_items);
    return this->items.size();
}

void undo_stack::reset() {
    std::lock_guard<std::mutex> guard(this->m_items);
    this->items.clear();
}

void* undo_stack::snapshot_state() {
    W->save(SAVE_TYPE_UNDO);

    void *data_copy = malloc(W->lb.size);
    memcpy(data_copy, W->lb.buf, W->lb.size);

    return data_copy;
}

void undo_stack::checkpoint(const char *reason, void *snapshot /* = nullptr */) {
    std::lock_guard<std::mutex> guard_items(this->m_items);

    if (!W->paused) {
        tms_warnf("undo_checkpoint: SKIPPING, BECAUSE not paused");
        return;
    } else if (!G->state.sandbox) {
        tms_warnf("undo_checkpoint: SKIPPING, BECAUSE state != sandbox");
        return;
    }

#ifdef DEBUG
    auto started = std::chrono::high_resolution_clock::now();
#endif

    if (snapshot == nullptr) {
        snapshot = this->snapshot_state();
    }

    struct undo_item *item = new undo_item {
        reason,
        snapshot,
        static_cast<size_t>(W->lb.size),
        static_cast<size_t>(W->lb.size),
        false,
    };
    std::lock_guard<std::mutex> guard_cur_item(item->mutex);
    this->items.push_back(item);

    if (this->items.size() > MAX_UNDO_ITEMS) {
        undo_item *front_item = this->items.front();
        std::lock_guard<std::mutex> guard(front_item->mutex);
        free((void *)front_item->data);
        this->items.erase(this->items.begin());
    }

#ifdef DEBUG
    auto done = std::chrono::high_resolution_clock::now();
    size_t total_size_bytes = 0;
    for (struct undo_item *&item : this->items) {
        total_size_bytes += item->size;
    }
    tms_debugf(
        "undo_checkpoint:\n"
        " - item: %lu bytes; ptr %p\n"
        " - total size: %lu bytes\n"
        " - histsize: %lu/%u\n"
        " - time: %lu ns\n",
        item->size, item->data,
        total_size_bytes,
        this->items.size(), MAX_UNDO_ITEMS,
        std::chrono::duration_cast<std::chrono::nanoseconds>(done - started).count()
    );
#endif

    tms_printf(
        "undo_checkpoint: histsize: %lu/%u\n",
        this->items.size(), MAX_UNDO_ITEMS
    );

    this->signal_to_run_compressor();
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

    struct undo_item *item;
    {
        std::lock_guard<std::mutex> guard(this->m_items);
        if (this->items.size() == 0) {
            tms_fatalf("undo_load: no items to load");
        }
        item = this->items.back();
        this->items.pop_back();
    }
    tms_assertf(item, "undo_restore: item is null");

    std::lock_guard<std::mutex> guard(item->mutex);

    void *uncompressed_data;
    size_t uncompressed_size;
    if (item->compressed) {
        tms_debugf("data is compressed, uncompressing...");
        uncompressed_data = std::malloc(item->size_uncompressed);
        uncompressed_size = item->size_uncompressed;
        uncompress(
            (Bytef*)uncompressed_data, (uLongf*)&uncompressed_size,
            (Bytef*)item->data, item->size
        );
    } else {
        tms_debugf("data is not compressed, using as is...");
        uncompressed_data = item->data;
        uncompressed_size = item->size;
    }

    G->lock();
    G->open_sandbox_snapshot_mem(uncompressed_data, uncompressed_size);

    tms_infof("Restored level from undo stack");

    // Free the item's data
    free(item->data);
    if (item->compressed) {
        free(uncompressed_data);
    }

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

    G->unlock();

    return item->reason;
}
