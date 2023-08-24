#pragma once

#include "entity.hh"

#define STICKY_MAX_LINES 4
#define STICKY_MAX_PER_LINE 32

class sticky : public entity
{
  private:
    int slot;
    char lines[STICKY_MAX_LINES][STICKY_MAX_PER_LINE];
    int linelen[STICKY_MAX_LINES];

    int currline;

    void draw_text(const char *txt);
    void add_word(const char *word, int len);

    void on_load(bool created, bool has_state);

  public:
    static tms_texture texture;

    static void _init();
    static void _deinit();

    sticky();
    ~sticky();
    void update_text();
    void set_text(const char *txt);
    void next_line();
    inline int get_slot(){return this->slot;};
    virtual void update(void);
    virtual void add_to_world();
    const char *get_name(void){return "Sticky note";};
};
