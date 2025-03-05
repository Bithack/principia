#pragma once

#include "menu-base.hh" // ;-)

class principia_wdg;
class widget_manager;
class p_text;

#define MAX_STATES 4

class menu_play : public menu_base
{
  private:
    principia_wdg *wdg_play_adventure;
    principia_wdg *wdg_open_latest_state;
    principia_wdg *wdg_open;
    principia_wdg *wdg_puzzles;

    principia_wdg *wdg_state[MAX_STATES];
    principia_wdg *wdg_state_date[MAX_STATES];

    bool has_saved_state;

  public:
    bool widget_clicked(principia_wdg *w, uint8_t button_id, int pid);

    menu_play();

    int render();
    int resume();
    int pause();
    int step(double dt);
    int handle_input(tms::event *ev, int action);

    void refresh_widgets();
};
