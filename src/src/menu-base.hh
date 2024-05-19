#pragma once

#include "pscreen.hh"

enum {
    BTN_VERSION,
    BTN_USERNAME,
    BTN_MESSAGE,
    BTN_BITHACK,
    BTN_SETTINGS,
    BTN_BACK,
    BTN_ENTITY,
    BTN_CONTEST,

    BTN_UPDATE,
    BTN_PLAY,
    BTN_CREATE,
    BTN_BROWSE_COMMUNITY,

    BTN_ADVENTURE,
    BTN_EMPTY_ADVENTURE,
    BTN_CUSTOM,

    BTN_OPEN,
    BTN_CONTINUE,

    BTN_PLAY_ADVENTURE,
    BTN_OPEN_STATE_DIALOG,
    BTN_OPEN_LATEST_STATE,
    BTN_OPEN_STATE,
    BTN_PUZZLES,
    BTN_GETTING_STARTED,

    BTN_IGNORE
};


class menu_base : public pscreen
{
  protected:
    /* Default widgets */
    principia_wdg *wdg_version;
    principia_wdg *wdg_username;
    principia_wdg *wdg_message;
    principia_wdg *wdg_bithack;
    principia_wdg *wdg_settings;
    principia_wdg *wdg_back;

    bool include_logo;

    float highlight;

  public:
    virtual bool widget_clicked(principia_wdg *w, uint8_t button_id, int pid);

    menu_base(bool _include_logo);
    ~menu_base();

    void refresh_scale();
    void window_size_changed();

    int resume();
    int step(double dt);
    int render();
    void refresh_widgets();

    float scale;
};
