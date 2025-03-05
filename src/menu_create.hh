#pragma once

#include "menu-base.hh" // ;-)
#include "main.hh"

class principia_wdg;
class widget_manager;
class p_text;

class menu_create : public menu_base
{
  public:
    /* LEFT */
    principia_wdg *wdg_create_new_level;
    principia_wdg *wdg_adventure;
    principia_wdg *wdg_empty_adventure;
    principia_wdg *wdg_custom;

    /* LEFT SUB */
    principia_wdg *wdg_open;
    principia_wdg *wdg_continue;

    /* RIGHT */
    principia_wdg *wdg_getting_started;
    std::vector<principia_wdg*> wdg_gs_entries;

    /* CONTEST BASE */
    principia_wdg *wdg_contest_thumb;

    /* CONTEST TOP */
    principia_wdg *wdg_contest_title;

    /* CONTEST BOTTOM */
    principia_wdg *wdg_contest_entry[MAX_FEATURED_LEVELS_FETCHED];

    bool has_autosave;

    bool widget_clicked(principia_wdg *w, uint8_t button_id, int pid);

    menu_create();

    int render();
    int resume();
    int pause();
    int step(double dt);
    int handle_input(tms::event *ev, int action);

    void refresh_widgets();
};
