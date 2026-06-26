#pragma once

#include "game.hh"
#include "menu-base.hh"
#include "pkgman.hh"
#include <tms/cpp.hh>

class menu_pkg : public menu_base {
    pkginfo             pkg;
    struct tms_camera  *cam;
    struct tms_ddraw   *dd;

    float scale;
    float base_x;
    float base_y;
    float icon_width;
    float icon_spacing;
    float icon_outer;
    float icon_height;
    float block_spacing;

    bool active;

    bool down[MAX_P];
    tvec2 touch_pos[MAX_P];
    uint64_t touch_time[MAX_P];
    bool dragging[MAX_P];

  public:
    menu_pkg();

    bool set_pkg(int type, uint32_t id);

    int render();
    int resume();
    int pause();
    int step(double dt);
    int handle_input(tms::event *ev, int action);
    void window_size_changed();
    void refresh_widgets();
    bool widget_clicked(principia_wdg *w, uint8_t button_id, int pid);

    void recalculate_layout();

    tvec2 get_cell_pos(int cell);
    bool is_unlocked(int cell);
};
