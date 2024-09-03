#pragma once

#include "font.hh"
#include <tms/bindings/cpp/cpp.hh>
#include <deque>
#include "game-graph.hh"

class p_text;
class p_font;
class widget_manager;
class principia_wdg;
class game_message;

enum ps_render_type {
    RT_NONE,

    RT_TEXT,
    RT_ROUNDED_SQUARE,
};

enum text_subtypes {
    ST_NONE,

    ST_TEXT,
    ST_GLYPH,
};

#define EVENT_CONT                 0
#define EVENT_DONE                 1
#define EVENT_SPECIAL              2

class pending_render
{
  public:
    uint8_t type;
    uint8_t subtype;
    uint8_t index;
    float x;
    float y;

    pending_render(uint8_t _type, uint8_t _index)
        : type(_type)
        , subtype(ST_NONE)
        , index(_index)
    { }
};

class rounded_square : public pending_render
{
  public:
    float width;
    float height;
    tvec4 color;
    float outline_width;

    rounded_square(uint8_t index)
        : pending_render(RT_ROUNDED_SQUARE, index)
    { }
};

class pending_tog : public pending_render
{
  public:
    bool set_color;
    float r;
    float g;
    float b;
    float a;
    float o_r;
    float o_g;
    float o_b;
    bool render_outline;
    bool do_free;

    pending_tog(uint8_t index, uint8_t _subtype)
        : pending_render(RT_TEXT, index)
    {
        this->subtype = _subtype;
        this->render_outline = true;
        this->do_free = false;
        this->set_color = false;
    }
};

class pending_text : public pending_tog
{
  public:
    p_text *text;

    pending_text(uint8_t index)
        : pending_tog(index, ST_TEXT)
    {
        this->text = 0;
    }

    pending_text(uint8_t index, p_text *t);
};

class pending_glyph : public pending_tog
{
  public:
    struct glyph *glyph;
    float scale;

    pending_glyph(uint8_t index)
        : pending_tog(index, ST_GLYPH)
    {
        this->glyph = 0;
    }
};

class pscreen : public tms::screen
{
  protected:
    widget_manager *wm;

    std::deque<pending_render*> pending_renders;

  public:
    pscreen();
    virtual ~pscreen() {};

    void add_rounded_square(float x, float y, float width, float height, tvec4 color, float outline_width, uint8_t index=50);
    void add_pending_text(pending_text *pt);
    void add_text(p_text *text, bool render_outline=true, bool do_free=false, uint8_t index=60);
    void add_text(p_text *text,
            float x, float y,
            bool render_outline=true,
            bool do_free=false,
            float r=1.f, float g=1.f, float b=1.f, float a=1.f,
            float o_r=0.f, float o_g=0.f, float o_b=0.f,
            uint8_t index=60
            );
    void add_glyph(struct glyph *glyph,
            float x, float y, float scale=1.f,
            bool render_outline=true,
            bool do_free=false,
            float r=1.f, float g=1.f, float b=1.f, float a=1.f,
            float o_r=0.f, float o_g=0.f, float o_b=0.f,
            uint8_t index=60
            );

    /* slow helper-function */
    void add_text(float num, p_font *font,
            float x, float y,
            tvec4 color,
            int precision=0,
            bool render_outline=true,
            enum text_align h=ALIGN_LEFT, enum text_align valign=ALIGN_CENTER,
            uint8_t index=60
            );
    void add_text(const char *str, p_font *font,
            float x, float y,
            tvec4 color,
            bool render_outline=true,
            enum text_align h=ALIGN_LEFT, enum text_align valign=ALIGN_CENTER,
            uint8_t index=60
            );

    virtual int handle_input(tms::event *ev, int action);

    virtual int render();
    virtual int post_render();

    virtual void refresh_widgets() = 0;

    static game_message *message;
    static game_graph fps_graph;
    static p_text *text_username;
    static void init();
    static void refresh_username();

    virtual bool widget_clicked(principia_wdg *w, uint8_t button_id, int pid) = 0;

    inline widget_manager *get_wm()
    {
        return this->wm;
    }
};
