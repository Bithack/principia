#include "menu_create.hh"
#include "menu_shared.hh"
#include "menu_pkg.hh"
#include "main.hh"
#include "misc.hh"
#include "settings.hh"
#include "ui.hh"
#include "game.hh"
#include "widget_manager.hh"
#include "text.hh"
#include "game-message.hh"
#include "gui.hh"

#define MAX_X 8.f
#define MIN_X -8.f

#define MENU_PADDING 3.f

bool
menu_create::widget_clicked(principia_wdg *w, uint8_t button_id, int pid)
{
    if (menu_base::widget_clicked(w, button_id, pid)) {
        return true;
    }

    switch (button_id) {
        case BTN_BACK:
            P.add_action(ACTION_GOTO_MAINMENU, 0x1);
            break;

        case BTN_ADVENTURE:
            G->resume_action = GAME_RESUME_NEW;
            G->resume_level_type = LCAT_ADVENTURE;
            G->screen_back = 0;
            tms::set_screen(G);
            break;

        case BTN_EMPTY_ADVENTURE:
            G->resume_action = GAME_RESUME_NEW_EMPTY;
            G->resume_level_type = LCAT_ADVENTURE;
            G->screen_back = 0;
            tms::set_screen(G);
            break;

        case BTN_CUSTOM:
            G->resume_action = GAME_RESUME_NEW_EMPTY;
            G->resume_level_type = LCAT_CUSTOM;
            G->screen_back = 0;
            tms::set_screen(G);
            break;

        case BTN_OPEN:
            ui::open_dialog(DIALOG_OPEN);
            break;

        case BTN_CONTINUE:
            P.add_action(ACTION_OPEN_AUTOSAVE, 0);
            G->resume_action = GAME_RESUME_OPEN;
            G->screen_back = 0;
            tms::set_screen(G);
            break;

        case BTN_GETTING_STARTED:
            ui::open_url((char*)w->data3);
            break;

        default: return false;
    }

    return true;
}

menu_create::menu_create()
    : menu_base(false)
    , has_autosave(false)
{
    this->wdg_back = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            BTN_BACK, AREA_MENU_TOP_LEFT,
            gui_spritesheet::get_sprite(S_LEFT), 0,
            0.7f);
    this->wdg_back->priority = 500;
    this->wdg_back->add();

    float w_percentage = 0.25f;

    if (_tms.window_width > 1280.f) {
        w_percentage /= _tms.window_width/1280.f*.75f;
    }

    /*
    if (_tms.window_width > 2000) {
        w_percentage = 0.15f;
    } else if (_tms.window_width > 1500) {
        w_percentage = 0.25f;
    } else if (_tms.window_width > 1000) {
        w_percentage = 0.35f;
    }
    */
    float h_percentage = w_percentage * 0.1f;

    {
        const enum WidgetArea area = AREA_MENU_LEFT_HCENTER;
        /* LEFT */
        this->wdg_create_new_level = this->wm->create_widget(
                this->get_surface(), TMS_WDG_LABEL,
                BTN_IGNORE, area);
        this->wdg_create_new_level->set_label("Create new level", font::large);
        this->wdg_create_new_level->priority = 1000;
        this->wdg_create_new_level->clickthrough = true;
        this->wdg_create_new_level->add();

        this->wdg_create_new_level->resize_percentage(
                _tms.window_width,  w_percentage,
                _tms.window_height, h_percentage);

        this->wdg_adventure = this->wm->create_widget(
                this->get_surface(), TMS_WDG_LABEL,
                BTN_ADVENTURE, area);
        this->wdg_adventure->set_label("Adventure", font::xmedium);
        this->wdg_adventure->priority = 990;
        this->wdg_adventure->render_background = true;
        this->wdg_adventure->add();

        this->wdg_adventure->label->set_scale(this->wdg_create_new_level->label->get_scale());

        this->wdg_empty_adventure = this->wm->create_widget(
                this->get_surface(), TMS_WDG_LABEL,
                BTN_EMPTY_ADVENTURE, area);
        this->wdg_empty_adventure->set_label("Empty Adventure", font::xmedium);
        this->wdg_empty_adventure->priority = 980;
        this->wdg_empty_adventure->render_background = true;
        this->wdg_empty_adventure->add();

        this->wdg_empty_adventure->label->set_scale(this->wdg_create_new_level->label->get_scale());

        this->wdg_custom = this->wm->create_widget(
                this->get_surface(), TMS_WDG_LABEL,
                BTN_CUSTOM, area);
        this->wdg_custom->set_label("Custom", font::xmedium);
        this->wdg_custom->priority = 995;
        this->wdg_custom->render_background = true;
        this->wdg_custom->add();

        this->wdg_custom->label->set_scale(this->wdg_create_new_level->label->get_scale());
    }

    {
        const enum WidgetArea area = AREA_CREATE_LEFT_SUB;
        /* LEFT SUB */
        this->wdg_open = this->wm->create_widget(
                this->get_surface(), TMS_WDG_LABEL,
                BTN_OPEN, area);
        this->wdg_open->set_label("Open", font::xmedium);
        this->wdg_open->priority = 1000;
        this->wdg_open->render_background = true;
        this->wdg_open->add();

        this->wdg_open->label->set_scale(this->wdg_create_new_level->label->get_scale());

        this->wdg_continue = this->wm->create_widget(
                this->get_surface(), TMS_WDG_LABEL,
                BTN_CONTINUE, area);
        this->wdg_continue->set_label("Continue building", font::xmedium);
        this->wdg_continue->render_background = true;
        this->wdg_continue->priority = 900;

        this->wdg_continue->label->set_scale(this->wdg_create_new_level->label->get_scale());
    }

    {
        /* RIGHT */
        this->wdg_getting_started = this->wm->create_widget(
                this->get_surface(), TMS_WDG_LABEL,
                BTN_IGNORE, AREA_MENU_RIGHT_HCENTER);
        this->wdg_getting_started->set_label("Getting started", font::large);
        this->wdg_getting_started->priority = 1000;
        this->wdg_getting_started->clickthrough = true;
        this->wdg_getting_started->add();

        this->wdg_getting_started->label->set_scale(this->wdg_create_new_level->label->get_scale());

        this->wm->areas[AREA_MENU_RIGHT_HCENTER].set_alpha(0.f);
    }

    {
        /* CONTEST BASE */
        this->wdg_contest_thumb = this->wm->create_widget(
                this->get_surface(), TMS_WDG_BUTTON,
                BTN_CONTEST, AREA_MENU_BOTTOM_LEFT,
                0);
        this->wdg_contest_thumb->priority = 1000;
        this->wdg_contest_thumb->alpha = 0.f;
    }

    {
        /* CONTEST TOP */
        this->wdg_contest_title = this->wm->create_widget(
                this->get_surface(), TMS_WDG_LABEL,
                BTN_CONTEST, AREA_CREATE_CONTEST_TOP);
        this->wdg_contest_title->priority = 1000;
    }

    {
        /* CONTEST BOTTOM */
        for (int x=0; x<MAX_FEATURED_LEVELS_FETCHED; ++x) {
            this->wdg_contest_entry[x] = this->wm->create_widget(
                    this->get_surface(), TMS_WDG_BUTTON,
                    BTN_ENTITY, AREA_CREATE_CONTEST_BOTTOM,
                    0);
            this->wdg_contest_entry[x]->priority = 100-MAX_FEATURED_LEVELS_FETCHED-x;
            this->wdg_contest_entry[x]->alpha = 0.f;
        }
    }

    this->refresh_widgets();
}

int
menu_create::resume()
{
    menu_base::resume();

    this->has_autosave = G->autosave_exists();

    this->refresh_widgets();

    return T_OK;
}

int
menu_create::pause(void)
{
    return T_OK;
}

int
menu_create::handle_input(tms::event *ev, int action)
{
#if defined(TMS_BACKEND_PC) && !defined(NO_UI)
    if (ev->type == TMS_EV_POINTER_DOWN) {
        if (!P.focused) {
            if (prompt_is_open) return T_OK;
            else P.focused = 1;
        }
    }
#endif

    if (pscreen::handle_input(ev, action) == EVENT_DONE) {
        return EVENT_DONE;
    }

    if (ev->type == TMS_EV_KEY_PRESS) {
        switch (ev->data.key.keycode) {
            case TMS_KEY_A:
            case TMS_KEY_1:
                this->wdg_adventure->click();
                break;

            case TMS_KEY_E:
            case TMS_KEY_2:
                this->wdg_empty_adventure->click();
                break;

            case TMS_KEY_C:
            case TMS_KEY_3:
                this->wdg_custom->click();
                break;

            case TMS_KEY_O:
                this->wdg_open->click();
                break;

            case TMS_KEY_R:
                this->refresh_widgets();
                return T_OK;

            case SDL_SCANCODE_AC_BACK:
            case TMS_KEY_B:
            case TMS_KEY_ESC:
                this->wdg_back->click();
                return T_OK;
        }
    }
    return T_OK;
}

int
menu_create::step(double dt)
{
    menu_base::step(dt);

    if (this->wdg_message->label && this->wdg_message->label->color.a < 1.2f) {
        float incr = _tms.dt * 1.0f;

        this->wdg_message->label->color.a += incr;
        this->wdg_message->label->outline_color.a += incr;
    }

    this->wm->step();

    return T_OK;
}

int
menu_create::render()
{
#ifdef SCREENSHOT_BUILD
    return T_OK;
#endif

    menu_base::render();

    const int MARGIN_X = this->wm->get_margin_x();
    const int MARGIN_Y = this->wm->get_margin_y();

    const int line_thickness = _tms.xppcm * .05f;
    int vert_y = menu_shared::bar_height + MARGIN_Y;
    if (menu_shared::contest_state > FL_INIT && this->wm->areas[AREA_MENU_BOTTOM_LEFT].enabled) {
        vert_y += this->wm->areas[AREA_MENU_BOTTOM_LEFT].last_height + MARGIN_Y + MARGIN_Y + line_thickness;
    }
    int vert_h = _tms.opengl_height - vert_y - menu_shared::bar_height - MARGIN_Y;
    int vert_x = _tms.opengl_width/2.f - line_thickness/2.f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(
            vert_x, vert_y,
            line_thickness, vert_h);
    menu_shared::tex_vert_line->render();

    const int hori_x = MARGIN_X;

    if (menu_shared::contest_state > FL_INIT && this->wm->areas[AREA_MENU_BOTTOM_LEFT].enabled) {
        glViewport(
                hori_x, vert_y - MARGIN_Y,
                _tms.opengl_width-MARGIN_X-hori_x, line_thickness);
        menu_shared::tex_hori_line->render();
    }

    struct widget_area *left = this->wdg_create_new_level->area;

    glViewport(
            hori_x, left->last_pos.y - left->last_height,
            _tms.opengl_width/2.f-MARGIN_X-hori_x, line_thickness);
    menu_shared::tex_hori_line->render();

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glViewport(
            0,
            0,
            _tms.opengl_width, _tms.opengl_height);
    //tms_texture_render(&gui_spritesheet::atlas->texture);

    return T_OK;
}

void
menu_create::refresh_widgets()
{
    this->wm->remove_all();

    menu_base::refresh_widgets();

    this->wdg_back->add();
    this->wdg_version->add();
    this->wdg_username->add();
    this->wdg_message->add();
    this->wdg_bithack->add();
    this->wdg_settings->add();

    this->wdg_create_new_level->add();
    this->wdg_adventure->add();
    this->wdg_empty_adventure->add();
    this->wdg_custom->add();
    this->wdg_open->add();

    this->wdg_getting_started->add();
    for (std::vector<principia_wdg*>::iterator it = this->wdg_gs_entries.begin();
            it != this->wdg_gs_entries.end(); ++it) {
        (*it)->add();
    }

    if (this->has_autosave) {
        this->wdg_continue->add();
    }

    if (this->wdg_contest_thumb->s[0]) {
        this->wdg_contest_thumb->add();
        this->wdg_contest_title->add();
        for (int x=0; x<MAX_FEATURED_LEVELS_FETCHED; ++x) {
            if (this->wdg_contest_entry[x] && this->wdg_contest_entry[x]->s[0]) {
                this->wdg_contest_entry[x]->add();
            }
        }
    }

    if (menu_shared::contest_state == FL_INIT) {
        if (menu_shared::contest.sprite) {
            struct tms_sprite *s = menu_shared::contest.sprite;

            this->wdg_contest_thumb->s[0] = s;
            tms_infof("height: %.2f", s->height);
            this->wdg_contest_thumb->size.x = s->width;
            this->wdg_contest_thumb->size.y = s->height;
            this->wdg_contest_thumb->data3 = UINT_TO_VOID(menu_shared::contest.id);
            this->wdg_contest_thumb->add();

            this->wdg_contest_title->data3 = UINT_TO_VOID(menu_shared::contest.id);
            char tmp[512];
            snprintf(tmp, 511, "Contest: %s", menu_shared::contest.name);
            this->wdg_contest_title->set_label(tmp, font::xmedium);
            this->wdg_contest_title->add();

            for (int x=0; x<MAX_FEATURED_LEVELS_FETCHED; ++x) {
                if (menu_shared::contest_entries[x].sprite) {
                    s = menu_shared::contest_entries[x].sprite;

                    this->wdg_contest_entry[x]->s[0] = s;
                    this->wdg_contest_entry[x]->size.x = s->width;
                    this->wdg_contest_entry[x]->size.y = s->height;
                    this->wdg_contest_entry[x]->data3 = UINT_TO_VOID(menu_shared::contest_entries[x].id);
                    this->wdg_contest_entry[x]->add();
                }
            }
        }

        menu_shared::contest_state = FL_ALPHA_IN;

        /* an extra rearrange to make sure the "browse more community levels"
         * text is at its proper location */
        this->wm->rearrange();
    }

    if (menu_shared::gs_state == FL_INIT) {
        int n = 500;

        for (std::vector<struct gs_entry>::iterator it = menu_shared::gs_entries.begin();
                it != menu_shared::gs_entries.end(); ++it) {
            const struct gs_entry &gse = *it;

            principia_wdg *wdg = this->wm->create_widget(
                    this->get_surface(), TMS_WDG_LABEL,
                    BTN_GETTING_STARTED, AREA_MENU_RIGHT_HCENTER);

            wdg->set_label(gse.title, font::xmedium);
            wdg->render_background = true;
            wdg->priority = n --;
            wdg->data3 = (void*)gse.link;
            wdg->add();
            wdg->label->set_scale(this->wdg_create_new_level->label->get_scale());

            this->wdg_gs_entries.push_back(wdg);
        }

        menu_shared::gs_state = FL_ALPHA_IN;

        /* an extra rearrange to make sure the "browse more community levels"
         * text is at its proper location */
        this->wm->rearrange();
    }

    this->wm->rearrange();
}
