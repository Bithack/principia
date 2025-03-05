#pragma once

#include "menu-base.hh" // ;-)
#include "main.hh"

class principia_wdg;
class widget_manager;
class p_text;

class menu_main : public menu_base
{
  public:
    /** Widgets **/
    principia_wdg *wdg_update_available;

    principia_wdg *wdg_play;
    principia_wdg *wdg_create;
    principia_wdg *wdg_browse_community;

    principia_wdg *wdg_featured_level[MAX_FEATURED_LEVELS_FETCHED];

  public:
    bool widget_clicked(principia_wdg *w, uint8_t button_id, int pid);

    menu_main();

    int resume();
    int pause();
    int render();
    int step(double dt);
    int handle_input(tms::event *ev, int action);

    void refresh_widgets();
};
