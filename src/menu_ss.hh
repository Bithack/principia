#pragma once

#ifdef SCREENSHOT_BUILD

#include "menu-base.hh" // ;-)

class principia_wdg;
class widget_manager;
class p_text;

class menu_ss : public menu_base
{
  private:
    p_text *text;

  public:
    bool widget_clicked(principia_wdg *w, uint8_t button_id, int pid) { return true; }

    menu_ss();

    int render();
    int resume() { return T_OK; }
    int pause() { return T_OK; }
    int step(double dt) { return T_OK; }
    int handle_input(tms::event *ev, int action) { return T_OK; }

    void refresh_widgets() { }
};

#endif
