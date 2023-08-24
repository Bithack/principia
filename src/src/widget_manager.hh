#pragma once

#include "pscreen.hh"
#include "font.hh"

#include <tms/core/wdg.h>
#include <tms/bindings/cpp/cpp.hh>

#include <deque>

#define TMS_WDG_LABEL   1000
#define TMS_WDG_KNOB    1001

class p_text;

enum WidgetArea {
    AREA_TOP_LEFT,
    AREA_TOP_LEFT2,
    AREA_BOTTOM_LEFT,
    AREA_NOMARGIN_BOTTOM_LEFT,
    AREA_BOTTOM_RIGHT,
    AREA_NOMARGIN_BOTTOM_RIGHT,
    AREA_BOTTOM_CENTER,
    AREA_NOMARGIN_BOTTOM_CENTER,
    AREA_TOP_CENTER,
    AREA_SUB_HEALTH,
    AREA_TOP_RIGHT,

    /* Special case of AREA_TOP_RIGHT, it cares about the menu width if it's up */
    AREA_GAME_TOP_RIGHT,

    AREA_MENU_TOP_RIGHT,
    AREA_MENU_TOP_LEFT,
    AREA_MENU_TOP_CENTER,
    AREA_MENU_CENTER,
    AREA_MENU_LEVELS,
    AREA_MENU_SUB_LEVELS,

    AREA_MENU_LEFT_HCENTER,
    AREA_MENU_RIGHT_HCENTER,

    AREA_MENU_LEFT_HLEFT,
    AREA_MENU_RIGHT_HRIGHT,

    AREA_CREATE_LEFT_SUB,

    AREA_MENU_BOTTOM_LEFT,
    AREA_MENU_BOTTOM_CENTER,
    AREA_CREATE_CONTEST_BOTTOM,
    AREA_CREATE_CONTEST_TOP,

    NUM_AREAS
};

struct widget_area
{
    float base_x;
    float x;
    float base_y;
    float y;
    float imodx;
    float imody;
    float modx;
    float mody;
    float tmodx;
    float tmody;
    float tsmodx;
    float tsmody;
    enum WidgetArea checkarea;
    bool used;
    enum text_align horizontal_align;
    enum text_align vertical_align;
    enum text_align label_halign;
    bool do_set_alpha;
    float alpha;
    bool enabled;

    bool first_widget;
    tvec2 first_off;
    tvec2 first_pos;
    tvec2 last_pos;
    float last_height;
    float last_width;

    tvec2 top;
    tvec2 bot;

    widget_area()
        : base_x(0.f)
        , base_y(0.f)
        , imodx(0.f)
        , imody(0.f)
        , modx(0.f)
        , mody(0.f)
        , tmodx(0.f)
        , tmody(0.f)
        , tsmodx(0.f)
        , tsmody(0.f)
        , checkarea(NUM_AREAS)
        , used(false)
        , horizontal_align(ALIGN_LEFT)
        , vertical_align(ALIGN_TOP)
        , label_halign(ALIGN_CENTER)
        , do_set_alpha(false)
        , alpha(1.f)
        , last_height(0.f)
        , last_width(0.f)
    {
        this->first_pos.x = 0.f;
        this->first_pos.y = 0.f;
        this->last_pos.x = 0.f;
        this->last_pos.y = 0.f;
        this->top.x = 0.f;
        this->top.y = 0.f;
        this->bot.x = 0.f;
        this->bot.y = 0.f;
        this->enabled = true;
    }

    void set_alpha(float a)
    {
        this->alpha = a;
        this->do_set_alpha = true;
    }
};

typedef void (*touch_cb)(struct tms_wdg *w, int pid, int ox, int oy, float rx, float ry);

class principia_wdg : public tms_wdg
{
  private:
    tms::surface *_surface;
    bool draggable;
    bool draggable_x;
    bool draggable_y;
    bool dragging;

  public:
    bool has_limit_x;
    float lower_limit_x;
    float upper_limit_x;
    bool has_limit_y;
    float lower_limit_y;
    float upper_limit_y;
    void (*on_dragged)(principia_wdg *w, float value_x, float value_y);
    widget_manager *parent;
    bool render_background;

    struct render_pos {
        float x;
        float y;
        bool use;
    } render_pos;

  public:
    principia_wdg(tms::surface *surface, int widget_type,
                  struct tms_sprite *s0, struct tms_sprite *s1,
                  float scale/*=1.f*/);
    ~principia_wdg();

    void step();
    void click();

    void add();
    void remove();

    void set_tooltip(const char *text, p_font *font=font::medium);
    void set_label(const char *text, p_font *font=font::medium);
    inline void set_position(float x, float y)
    {
        this->pos.x = x;
        this->pos.y = y;
    }

    void resize_percentage(
            int base_width,  float width_percentage,
            int base_height, float height_percentage
            );

    inline void set_padding(float x, float y)
    {
        this->padding.x = x;
        this->padding.y = y;
    }

    void set_draggable(bool val);
    void set_dragging(bool val);

    inline void set_draggable_x(bool val, bool has_limit=false, float lower_limit=0.f, float upper_limit=0.f)
    {
        this->draggable_x = val;
        this->has_limit_x = has_limit;
        this->lower_limit_x = lower_limit;
        this->upper_limit_x = upper_limit;
    }
    inline void set_draggable_y(bool val, bool has_limit=false, float lower_limit=0.f, float upper_limit=0.f)
    {
        this->draggable_y = val;
        this->has_limit_y = has_limit;
        this->lower_limit_y = lower_limit;
        this->upper_limit_y = upper_limit;
    }
    inline bool can_drag_x() { return this->draggable_x; }
    inline bool can_drag_y() { return this->draggable_y; }
    inline bool is_label() { return this->_type == TMS_WDG_LABEL; }

    int priority;
    struct widget_area *area;

    int _type;
    double down_time;
    double tooltip_time;
    bool tooltip_active;
    bool moved_out;
    p_text *tooltip;
    p_text *label;

    void (*on_long_press)(principia_wdg *w);

    tvec2 padding;

    bool marker;
    tvec3 marker_color;
    float lmodx;
    float lmody;

    pscreen* get_home();

    friend class widget_manager;
};

class widget_manager
{
  private:
    std::deque<principia_wdg*> widgets;

    pscreen *home;
    bool override_down;
    bool override_up;

    void refresh_areas();

  public:
    widget_manager(pscreen *home, bool override_down, bool override_up);
    ~widget_manager(); /* The widget manager is responsible for freeing the memory of all widgets */

    struct widget_area areas[NUM_AREAS];

    struct widget_area* get_area(enum WidgetArea area)
    {
        return &this->areas[area];
    }

    int get_margin_x();
    int get_margin_y();

    void init_areas();

    principia_wdg *create_widget(tms::surface *surface, int widget_type,
                                 uint8_t id, WidgetArea area,
                                 struct tms_sprite *s0=0, struct tms_sprite *s1=0,
                                 float scale=1.f);

    principia_wdg *get_widget(enum WidgetArea area, uint8_t id);

    void step();
    void remove_all();
    void render();
    void rearrange();

    inline pscreen* get_home()
    {
        return this->home;
    }
};
