#pragma once

#define MIN_MOOD_VALUE 0.f
#define MAX_MOOD_VALUE 1.f

enum {
    MOOD_ANGER,
    MOOD_BRAVERY,
    MOOD_FEAR,

    NUM_MOODS
};

class mood_data {
  protected:
    float moods[NUM_MOODS];

  public:
    mood_data();
    float get(int what);
    void set(int what, float v);
    void add(int what, float v);

    void *data;
    void (*mood_changed)(int what, float old_value, float new_value, void *userdata);
};
