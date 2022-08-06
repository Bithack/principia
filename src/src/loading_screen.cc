#include "loading_screen.hh"
#include "menu_main.hh"
#include "settings.hh"
#include "text.hh"

loading_screen::loading_screen()
{
    float projection[16];
    tmat4_set_ortho(projection, 0, _tms.window_width, 0, _tms.window_height, -1, 1);
    this->dd = tms_ddraw_alloc();
    tms_ddraw_set_matrices(this->dd, 0, projection);

    this->text = 0;
}

int
loading_screen::pause(void)
{
#ifdef TMS_BACKEND_PC
    /* return the pervious vsync status */
    if (settings["vsync"]->v.b) {
        if (SDL_GL_SetSwapInterval(-1) == -1)
            SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }
#endif

    tms_infof("pause loading");
    return T_OK;
}

int
loading_screen::resume(void)
{
#ifdef TMS_BACKEND_PC
    /* Disable vsync during loading screen */
    SDL_GL_SetSwapInterval(0);
#endif

    tms_infof("resume loading -----------------------------------");
    this->step = 0;
    return T_OK;
}

int
loading_screen::render()
{
#ifdef TMS_BACKEND_IOS
    glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);
#endif
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    float width = _tms.xppcm * 3.f;
    float fill = width * ((float)this->step / (float)this->num_steps);
    float x = fill/2.f;

    tms_ddraw_set_color(this->dd, .1f, .1f, .1f, 1.f);
    tms_ddraw_square(this->dd,
            _tms.window_width/2.f,
            _tms.window_height/2.f,
            width,
            _tms.yppcm/3.f);
    tms_ddraw_set_color(this->dd, 0.6f, 0.6f, 1.0f, 1.f);
    tms_ddraw_square(this->dd,
            _tms.window_width/2.f - width/2.f + x,
            _tms.window_height/2.f,
            fill,
            _tms.yppcm/3.f);

    if (font::medium && this->text && this->text->active) {
        tms_ddraw_set_color(this->dd, 1.f, 1.f, 1.f, 1.f);
        this->text->render(this->dd);
    }

    return T_OK;
}

int
loading_screen::step_loading()
{
    if (!this->loader) return LOAD_ERROR;
    return this->loader(this->step++);
}

tms::screen *loading_screen::get_next_screen()
{
    if (next == 0) {
        return P.s_menu_main;
    } else {
        return next;
    }
}

void
loading_screen::window_size_changed()
{
    if (this->dd) {
        float projection[16];
        tmat4_set_ortho(projection, 0, _tms.window_width, 0, _tms.window_height, -1, 1);
        tms_ddraw_set_matrices(this->dd, 0, projection);
    }
}

void
loading_screen::set_text(const char *text)
{
    if (font::medium) {
        if (!this->text) {
            this->text = new p_text(font::medium, ALIGN_LEFT, ALIGN_BOTTOM);

            float bar_width  = _tms.xppcm * 3.f;
            float bar_height = _tms.yppcm / 3.f;

            float x = _tms.window_width/2.f - bar_width/2.f;
            float y = _tms.window_height/2.f + bar_height/1.5f;

            this->text->set_position(x, y);
        }

        this->text->active = false;

        if (text) {
            this->text->active = true;
            this->text->set_text(text);
        }
    }
}
