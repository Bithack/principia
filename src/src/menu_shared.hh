#pragma once

#include <tms/bindings/cpp/cpp.hh>
#include "main.hh"

class p_text;

enum fl_state {
    FL_WORKING,
    FL_UPLOAD,
    FL_WAITING,
    FL_INIT,
    FL_ALPHA_IN,
    FL_DONE,
};

struct gs_entry {
    const char *title;
    const char *link;

    gs_entry(const char *_title, const char *_link)
        : title(_title)
        , link(_link)
    { }
};

class menu_shared
{
  public:
    static tms::texture *tex_bg;
    static tms::texture *tex_vignette;
    static tms::texture *tex_menu_bottom;
    static tms::texture *tex_principia;
    static tms::texture *tex_vert_line;
    static tms::texture *tex_hori_line;

    static enum fl_state fl_state;
    static struct featured_level {
        struct tms_sprite *sprite;
        uint32_t id;
        char name[256];
        char creator[256];
    } fl[MAX_FEATURED_LEVELS_FETCHED];

    static enum fl_state gs_state;
    static std::vector<struct gs_entry> gs_entries;

    static enum fl_state contest_state;
    static struct featured_level contest;
    static struct featured_level contest_entries[MAX_FEATURED_LEVELS_FETCHED];

    static float fl_alpha;
    static float contest_alpha;
    static float gs_alpha;

    static p_text *text_version;
    static p_text *text_message;

    static int bar_height;

    static void init();
    static void refresh_message();
    static void step();
};
