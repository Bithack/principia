#pragma once

#include <stdint.h>
#include <map>

class lvl_progress
{
  public:
    uint8_t  completed;
    uint32_t top_score;
    uint32_t last_score;
    uint32_t time;
    uint32_t num_plays;

    lvl_progress()
    {
        completed = 0;
        top_score = 0;
        last_score = 0;
        time = 0;
        num_plays = 0;
    }
};

class progress
{
  public:
    static bool initialized;
    static std::map<uint32_t, lvl_progress*> levels[3];
    static lvl_progress *get_level_progress(int level_type, uint32_t level_id);

    static void init(char *custom_path=0);
    static void commit();
};
