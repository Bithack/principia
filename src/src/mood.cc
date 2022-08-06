#include "mood.hh"
#include <cstring>

mood_data::mood_data()
{
    this->data = 0;
    this->mood_changed = 0;
    memset(this->moods, 0, sizeof(float) * NUM_MOODS);
}

float
mood_data::get(int what)
{
    if (what >= 0 && what < NUM_MOODS) {
        return this->moods[what];
    }

    return 0.f;
}

void
mood_data::set(int what, float v)
{
    if (what >= 0 && what < NUM_MOODS) {
        float old = this->moods[what];
        this->moods[what] = v;
        if (this->moods[what] < MIN_MOOD_VALUE) this->moods[what] = MIN_MOOD_VALUE;
        else if (this->moods[what] > MAX_MOOD_VALUE) this->moods[what] = MAX_MOOD_VALUE;

        if (this->mood_changed != 0) {
            this->mood_changed(what, old, this->moods[what], this->data);
        }
    }
}

void
mood_data::add(int what, float v)
{
    if (what >= 0 && what < NUM_MOODS) {
        float old = this->moods[what];
        this->moods[what] = old + v;
        if (this->moods[what] < MIN_MOOD_VALUE) this->moods[what] = MIN_MOOD_VALUE;
        else if (this->moods[what] > MAX_MOOD_VALUE) this->moods[what] = MAX_MOOD_VALUE;

        if (this->mood_changed != 0) {
            this->mood_changed(what, old, this->moods[what], this->data);
        }
    }
}
