#pragma once

#include <tms/bindings/cpp/cpp.hh>
#include "menu-base.hh"
#include "pkgman.hh"

class menu_pkg : public menu_base
{
    pkginfo             pkg;
    struct tms_camera  *cam;
    struct tms_camera  *cam_screen;
    struct tms_ddraw   *dd;

    float scale;
    float base_x;
    float base_y;
    float icon_width;
    float icon_spacing;
    float icon_outer;
    float icon_height;
    float block_spacing;

  public:
    menu_pkg();

    bool set_pkg(int type, uint32_t id);

    int render();
    int resume(void);
    int pause(void);
    int step(double dt);
    int handle_input(tms::event *ev, int action);
    void window_size_changed();
    void refresh_widgets();
    bool widget_clicked(principia_wdg *w, uint8_t button_id, int pid);
};
