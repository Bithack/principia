#pragma once

#include <tms/bindings/cpp/cpp.hh>
#include "text.hh"

#define LOAD_CONT 0
#define LOAD_ERROR 1
#define LOAD_DONE 2
#define LOAD_RETRY 3
#define LOAD_RETURN_NUM_STEPS -1

class loading_screen : public tms::screen
{
  private:
    int step;
    int num_steps;
    int (*loader)(int step);
    tms::screen *next;
    struct tms_ddraw *dd;
    p_text *text;

  public:
    loading_screen();

    inline void load(int (*loader)(int), tms::screen *s)
    {
        this->num_steps = loader(LOAD_RETURN_NUM_STEPS);
        this->loader = loader;
        this->next = s;
        tms::set_screen(this);
    }

    tms::screen *get_next_screen();
    void set_next_screen(tms::screen *s){this->next = s;};

    void set_text(const char *text);

    inline void retry()
    {
        this->step--;
    }

    inline void advance_to(int step)
    {
        if (step > this->num_steps) this->step = this->num_steps;

        this->step = step;
    }
    int step_loading();
    int render(void);
    int pause(void);
    int resume(void);

    void window_size_changed();
};
