#include "menu_ss.hh"
#include "text.hh"
#include "widget_manager.hh"

#ifdef SCREENSHOT_BUILD

menu_ss::menu_ss() : menu_base(false) {
    this->text = new p_text(font::medium, ALIGN_CENTER, ALIGN_BOTTOM);

    this->text->set_text("Screenshotter is ready and listening on pipe...");
}

int menu_ss::render() {
    SDL_Delay(250);

    glClearColor(0.02f, 0.02f, 0.02f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    this->text->set_position(_tms.window_width/2.f, _tms.window_height/2.f);

    tms_ddraw_set_color(this->get_surface()->ddraw, 1.f, 1.f, 1.f, 1.f);
    this->text->render(this->get_surface()->ddraw);
    return T_OK;
}

#endif
