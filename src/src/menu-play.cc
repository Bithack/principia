#include "menu-play.hh"
#include "menu_shared.hh"
#include "menu_pkg.hh"
#include "misc.hh"
#include "gui.hh"
#include "ui.hh"
#include "game.hh"
#include "widget_manager.hh"
#include "text.hh"
#include "game-message.hh"

#define MAX_X 8.f
#define MIN_X -8.f

#define MENU_PADDING 3.f

bool
menu_play::widget_clicked(principia_wdg *w, uint8_t button_id, int pid)
{
    if (menu_base::widget_clicked(w, button_id, pid)) {
        return true;
    }

    switch (button_id) {
        case BTN_BACK:
            P.add_action(ACTION_GOTO_MAINMENU, 0x1);
            break;

        case BTN_PLAY_ADVENTURE:
            G->resume_action = GAME_START_NEW_ADVENTURE;
            G->screen_back = this;
            tms::set_screen(G);
            break;

        case BTN_OPEN_LATEST_STATE:
            P.add_action(ACTION_OPEN_LATEST_STATE, this);
            break;

        case BTN_OPEN_STATE_DIALOG:
            ui::open_dialog(DIALOG_OPEN_STATE, UINT_TO_VOID(1));
            break;

        case BTN_OPEN_STATE:
            P.add_action(ACTION_OPEN_STATE, w->data3);
            break;

        case BTN_PUZZLES:
            P.add_action(ACTION_MAIN_MENU_PKG, 0);
            break;

        default: return false;
    }

    return true;
}

menu_play::menu_play()
    : menu_base(false)
    , has_saved_state(false)
{
    this->wdg_back = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            BTN_BACK, AREA_MENU_TOP_LEFT,
            gui_spritesheet::get_sprite(S_LEFT), 0,
            0.7f);
    this->wdg_back->priority = 500;
    this->wdg_back->add();

    this->wdg_puzzles = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            BTN_PUZZLES, AREA_MENU_CENTER);
    this->wdg_puzzles->set_label("Learn with puzzles", font::large);
    this->wdg_puzzles->priority = 1000;
    this->wdg_puzzles->render_background = true;
    this->wdg_puzzles->add();

    this->wdg_puzzles->resize_percentage(
            _tms.window_width,  0.45,
            _tms.window_height, 0.20);

    this->wdg_open_latest_state = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            BTN_OPEN_LATEST_STATE, AREA_MENU_CENTER);
    this->wdg_open_latest_state->set_label("Continue playing", font::xmedium);
    this->wdg_open_latest_state->priority = 950;
    this->wdg_open_latest_state->render_background = true;
    this->wdg_open_latest_state->add();

    this->wdg_open_latest_state->label->set_scale(this->wdg_puzzles->label->get_scale());

    this->wdg_open = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            BTN_OPEN_STATE_DIALOG, AREA_MENU_CENTER);
    this->wdg_open->set_label("Open", font::xmedium);
    this->wdg_open->priority = 900;
    this->wdg_open->render_background = true;
    this->wdg_open->add();

    this->wdg_open->label->set_scale(this->wdg_puzzles->label->get_scale());

    this->wdg_play_adventure = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            BTN_PLAY_ADVENTURE, AREA_MENU_CENTER);
    this->wdg_play_adventure->set_label("Adventure exploration mode", font::xmedium);
    this->wdg_play_adventure->render_background = true;
    this->wdg_play_adventure->priority = 850;
    this->wdg_play_adventure->add();

    this->wdg_play_adventure->label->set_scale(this->wdg_puzzles->label->get_scale());

    for (int n=0; n<MAX_STATES; ++n) {
        this->wdg_state[n] = this->wm->create_widget(
                this->get_surface(), TMS_WDG_LABEL,
                BTN_OPEN_STATE, AREA_MENU_LEFT_HLEFT);
        this->wdg_state[n]->priority = 600+MAX_STATES-n;

        this->wdg_state_date[n] = this->wm->create_widget(
                this->get_surface(), TMS_WDG_LABEL,
                BTN_IGNORE, AREA_MENU_RIGHT_HRIGHT);
        this->wdg_state_date[n]->priority = 600+MAX_STATES-n;
        this->wdg_state_date[n]->clickthrough = true;
    }

    this->refresh_widgets();
}

int
menu_play::resume()
{
    menu_base::resume();

    this->refresh_widgets();

    return T_OK;

    std::vector<lvlfile*> states;

    lvlfile *level  = pkgman::get_levels(LEVEL_LOCAL_STATE);
    while (level) {
        states.push_back(level);
        lvlfile *next = level->next;
        level = next;
    }

    std::sort(states.begin(), states.end(), &pkgman::mtime_desc);

    int n;
    for (n=0; n<MAX_STATES; ++n) {
        if (this->wdg_state[n]) {
            this->wdg_state[n]->remove();

            if (this->wdg_state[n]->data3) {
                free(this->wdg_state[n]->data3);
                this->wdg_state[n]->data3 = 0;
            }
        }
        if (this->wdg_state_date[n]) {
            this->wdg_state_date[n]->remove();
        }
    }

    n = 0;
    for (std::vector<lvlfile*>::iterator it = states.begin();
            it != states.end(); ++it) {
        lvlfile *lvl = *it;

        this->wdg_state[n]->set_label(lvl->name);
        uint32_t *info = (uint32_t*)malloc(sizeof(uint32_t)*3);
        info[0] = lvl->id_type;
        info[1] = lvl->id;
        info[2] = lvl->save_id;
        this->wdg_state[n]->data3 = info;
        this->wdg_state[n]->add();

        this->wdg_state_date[n]->set_label(lvl->modified_date);
        this->wdg_state_date[n]->add();

        if (++n == MAX_STATES) {
            break;
        }
    }

    //this->has_autosave = G->autosave_exists();

    return T_OK;
}

int
menu_play::pause(void)
{
    return T_OK;
}

int
menu_play::handle_input(tms::event *ev, int action)
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
                this->wdg_play_adventure->click();
                break;

            case TMS_KEY_2:
                this->wdg_open_latest_state->click();
                break;

            case TMS_KEY_3:
                this->wdg_open->click();
                break;

            case TMS_KEY_4:
                this->wdg_puzzles->click();
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
menu_play::step(double dt)
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
menu_play::render()
{
#ifdef SCREENSHOT_BUILD
    return T_OK;
#endif

    menu_base::render();

    glViewport(
            0,
            0,
            _tms.opengl_width, _tms.opengl_height);

    return T_OK;
}

void
menu_play::refresh_widgets()
{
    menu_base::refresh_widgets();

    this->wm->rearrange();
}
