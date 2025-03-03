#include "menu-base.hh"
#include "menu_shared.hh"
#include "menu_main.hh"
#include "menu_create.hh"
#include "widget_manager.hh"
#include "gui.hh"
#include "ui.hh"
#include "misc.hh"
#include "main.hh"
#include "game.hh"
#include "version.hh"

extern struct tms_program *menu_bg_program;
extern GLuint              menu_bg_color_loc;

bool
menu_base::widget_clicked(principia_wdg *w, uint8_t button_id, int pid)
{

    switch (button_id) {
        case BTN_VERSION:
            char msg[1024];
            snprintf(msg, 1023, "Principia %s commit %s",
                principia_version_string(), principia_version_hash());

            ui::message(msg);
            break;

        case BTN_USERNAME:
            {
                if (P.username) {
                    P.num_unread_messages = 0;
                    pscreen::refresh_username();
                    P.add_action(ACTION_REFRESH_WIDGETS, 0);

                    COMMUNITY_URL("user/%s", P.username);
                    ui::open_url(url);
                } else {
                    ui::open_dialog(DIALOG_LOGIN);
                }
            }
            break;

        case BTN_MESSAGE: {
            COMMUNITY_URL("version-redir");
            ui::open_url(url);
	    } break;

        case BTN_BITHACK:
            ui::open_url("https://www.bithack.com/");
            break;

        case BTN_SETTINGS:
            ui::open_dialog(DIALOG_SETTINGS);
            break;

        case BTN_ENTITY:
            {
                uint32_t id = VOID_TO_UINT32(w->data3);
                COMMUNITY_URL("level/%u", id);
                ui::open_url(url);
            }
            break;

        case BTN_CONTEST:
            {
                uint32_t id = VOID_TO_UINT32(w->data3);
                COMMUNITY_URL("contest/%u", id);
                ui::open_url(url);
            }
            break;

        case BTN_IGNORE: break;
        default:
            return false;
    }

    return true;
}

menu_base::menu_base(bool _include_logo)
    : include_logo(_include_logo)
{
    this->highlight = 0.f;

    this->refresh_scale();

    this->set_surface(new tms::surface());
    this->get_surface()->atlas = gui_spritesheet::atlas;

    this->wm = new widget_manager(this, false, true);

    this->wdg_username = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            BTN_USERNAME, AREA_MENU_TOP_LEFT);
    this->wdg_username->priority = 500;
    this->wdg_username->label = pscreen::text_username;
    this->wdg_username->add();

    this->wdg_version = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            BTN_VERSION, AREA_NOMARGIN_BOTTOM_RIGHT);
    this->wdg_version->priority = 1000;
    this->wdg_version->label = menu_shared::text_version;
    this->wdg_version->add();

    this->wdg_message = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            BTN_MESSAGE, AREA_NOMARGIN_BOTTOM_CENTER);
    this->wdg_message->label = menu_shared::text_message;

    this->wdg_bithack = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            BTN_BITHACK, AREA_NOMARGIN_BOTTOM_LEFT,
            gui_spritesheet::get_sprite(S_BITHACK), 0, 1.0f);
    this->wdg_bithack->add();

    this->wdg_settings = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            BTN_SETTINGS, AREA_MENU_TOP_RIGHT,
            gui_spritesheet::get_sprite(S_CONFIG), 0,
            0.7f);
    this->wdg_settings->priority = 500;
    this->wdg_settings->add();
}

menu_base::~menu_base()
{
    delete this->wm;
}

void
menu_base::refresh_scale()
{
    if (_tms.window_width < 1000) {
        this->scale = (float)_tms.window_width / 1000.f;
    } else {
#ifdef TMS_BACKEND_MOBILE
        if (_tms.window_width > 1200) {
            this->scale = (float)_tms.window_width / 1200.f;
        } else {
            this->scale = 1.f;
        }
#else
        this->scale = 1.f;
#endif
    }
}

void
menu_base::window_size_changed()
{
    this->refresh_scale();

    if (this->get_surface() && this->get_surface()->ddraw) {
        float projection[16];
        tmat4_set_ortho(projection, 0, _tms.window_width, 0, _tms.window_height, 1, -1);
        tms_ddraw_set_matrices(this->get_surface()->ddraw, 0, projection);
    }

    this->wm->init_areas();
    this->wm->rearrange();

    this->refresh_widgets();
}

int
menu_base::render()
{
#ifdef SCREENSHOT_BUILD
    return T_OK;
#endif

    if (!P.focused) {
#ifndef TMS_BACKEND_IOS
        SDL_Delay(100);
#endif
    }

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    struct tms_fb fb;
    fb.num_textures = 1;
    fb.toggle = 0;
    fb.fb_texture[0][0] = menu_shared::tex_bg->gl_texture;

    {
        tms_program_bind(menu_bg_program);
        float hl = 1.f+this->highlight;
        glUniform4f(menu_bg_color_loc, hl, hl, hl, 1.f);
    }
    tms_fb_render(&fb, menu_bg_program);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    menu_shared::tex_vignette->render();

    if (this->include_logo) {
        int w = 0.75f * menu_shared::tex_principia->width*this->scale;
        int h = 0.75f * menu_shared::tex_principia->height*this->scale;

        glViewport(
                _tms.opengl_width / 2 - w/2,
                _tms.opengl_height - h - menu_shared::bar_height,
                w, h);
        menu_shared::tex_principia->render();
    }

    glDisable(GL_BLEND);

    glViewport(
            0,
            0,
            _tms.opengl_width, menu_shared::bar_height);
    menu_shared::tex_menu_bottom->render();

    glViewport(
            0,
            _tms.opengl_height-menu_shared::bar_height,
            _tms.opengl_width, menu_shared::bar_height);
    menu_shared::tex_menu_bottom->render();

    glViewport(
            0,
            0,
            _tms.opengl_width, _tms.opengl_height);

    // yes we call step from within render! ultracool
    menu_shared::step();

    pscreen::render();

    return T_OK;
}

int
menu_base::resume()
{
    this->highlight = 1.f;

    return T_OK;
}

int
menu_base::step(double dt)
{
    this->highlight -= dt*4.f;
    if (this->highlight <= 0.f) {
        this->highlight = 0.f;
    }

    return T_OK;
}

void
menu_base::refresh_widgets()
{
    if (!this->wdg_message->surface && menu_shared::text_message && menu_shared::text_message->text) {
        this->wdg_message->add();

        this->wdg_message->size.x = this->wdg_message->label->get_width();
        this->wdg_message->size.y = this->wdg_message->label->get_height();
    }

    if (this->wdg_username->label) {
        this->wdg_username->size.x = pscreen::text_username->get_width();
        this->wdg_username->size.y = pscreen::text_username->get_height();
    }
}
