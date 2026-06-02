#pragma once

#include <stdint.h>
#include <map>

/// Class representing progress information of a specific level
class lvl_progress {
  public:
    // Whether the level has been completed at least once
    uint8_t  completed;
    /// Top score achieved by the player in this level (or lowest, if LVL_LOWER_SCORE_IS_BETTER)
    uint32_t top_score;
    /// Last score achieved by the player in this level
    uint32_t last_score;
    /// Amount of time in seconds it took to complete the level? (Currently unused, always 0)
    uint32_t time;
    /// Amount of times a level has been played
    uint32_t num_plays;

    lvl_progress() {
        completed = 0;
        top_score = 0;
        last_score = 0;
        time = 0;
        num_plays = 0;
    }
};

/// Class used for storing and loading player persistent progress in a way that
/// is not easily tampered with.
class progress {
  public:
    static bool initialized;
    static char path[1024];

    static std::map<uint32_t, lvl_progress*> levels[3];

    /**
     * Return a pointer to a level progress instance. If there is no progress for the
     * given level, a new instance is created, added to the internal map and returned.
     */
    static lvl_progress *get_level_progress(int level_type, uint32_t level_id);

    /// Initialise progress and load from disk if it exists
    static void init(char *data_path);

    /// Save progress to disk
    static void commit();
};
