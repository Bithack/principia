#include "pscreen.hh"
#include "gui.hh"
#include "widget_manager.hh"
#include "game-message.hh"
#include "game-graph.hh"
#include "text.hh"
#include "settings.hh"
#include "ui.hh"
#include "soundmanager.hh"
#include "game.hh"

#include <SDL.h>

p_text *pscreen::text_username;
game_message *pscreen::message;
game_graph pscreen::fps_graph("FPS");
int ping_cooldown = 0;

pending_text::pending_text(uint8_t index, p_text *t)
    : pending_tog(index, ST_TEXT)
{
    this->text = t;
    this->x = t->get_x();
    this->y = t->get_y();
    this->r = t->color.r;
    this->g = t->color.g;
    this->b = t->color.b;
    this->a = t->color.a;
    this->o_r = t->outline_color.r;
    this->o_g = t->outline_color.g;
    this->o_b = t->outline_color.b;
}

pscreen::pscreen()
    : wm(0)
{ }

void
pscreen::add_rounded_square(float x, float y, float width, float height, tvec4 color, float outline_width, uint8_t index/*=50*/)
{
    rounded_square *rs = new rounded_square(index);
    rs->x = x;
    rs->y = y;
    rs->width = width;
    rs->height = height;
    rs->color = color;
    rs->outline_width = outline_width;

    this->pending_renders.push_back(rs);
}

void
pscreen::add_pending_text(pending_text *pt)
{
    this->pending_renders.push_back(pt);
}

void
pscreen::add_text(p_text *text, bool render_outline/*=true*/, bool do_free/*=false*/, uint8_t index/*=60*/)
{
    pending_text *pt = new pending_text(index);
    pt->text = text;
    pt->render_outline = render_outline;
    pt->x = text->get_x();
    pt->y = text->get_y();
    pt->set_color = false;
    pt->do_free = do_free;
    pt->render_outline = render_outline;

    this->pending_renders.push_back(pt);
}

void
pscreen::add_text(p_text *text, float x, float y,
        bool render_outline/*=true*/,
        bool do_free/*=false*/,
        float r/*=1.f*/, float g/*=1.f*/, float b/*=1.f*/, float a/*=1.f*/,
        float o_r/*=0.f*/, float o_g/*=0.f*/, float o_b/*=0.f*/,
        uint8_t index/*=60*/
        )
{
    pending_text *pt = new pending_text(index);
    pt->text = text;
    pt->x = x;
    pt->y = y;
    pt->set_color = true;
    pt->r = r;
    pt->g = g;
    pt->b = b;
    pt->a = a;
    pt->o_r = o_r;
    pt->o_g = o_g;
    pt->o_b = o_b;
    pt->do_free = do_free;
    pt->render_outline = render_outline;

    this->pending_renders.push_back(pt);
}

void
pscreen::add_glyph(struct glyph *glyph, float x, float y, float scale/*=1.f*/,
        bool render_outline/*=true*/,
        bool do_free/*=false*/,
        float r/*=1.f*/, float g/*=1.f*/, float b/*=1.f*/, float a/*=1.f*/,
        float o_r/*=0.f*/, float o_g/*=0.f*/, float o_b/*=0.f*/,
        uint8_t index/*=60*/
        )
{
    pending_glyph *pg = new pending_glyph(index);
    pg->glyph = glyph;
    pg->scale = scale;
    pg->x = x;
    pg->y = y;
    pg->set_color = true;
    pg->r = r;
    pg->g = g;
    pg->b = b;
    pg->a = a;
    pg->o_r = o_r;
    pg->o_g = o_g;
    pg->o_b = o_b;
    pg->do_free = do_free;
    pg->render_outline = render_outline;

    this->pending_renders.push_back(pg);
}

void
pscreen::add_text(float num, p_font *font,
        float x, float y,
        tvec4 color,
        int precision/*=0*/,
        bool render_outline/*=true*/,
        enum text_align halign/*=ALIGN_LEFT*/, enum text_align valign/*=ALIGN_CENTER*/,
        uint8_t index/*=60*/
        )
{
    char tmp[64];
    snprintf(tmp, 63, "%.*f", precision, num);

    p_text *tmp_text = new p_text(font, halign, valign);
    tmp_text->set_text(tmp);
    tmp_text->color = color;

    this->add_text(tmp_text, x, y, render_outline, true, index);
}

void
pscreen::add_text(const char *str, p_font *font,
        float x, float y,
        tvec4 color,
        bool render_outline/*=true*/,
        enum text_align halign/*=ALIGN_LEFT*/, enum text_align valign/*=ALIGN_CENTER*/,
        uint8_t index/*=60*/
        )
{
    p_text *tmp_text = new p_text(font, halign, valign);
    tmp_text->set_text(str);
    tmp_text->color = color;

    this->add_text(tmp_text, x, y, render_outline, true, index);
}

struct render_sorter
{
    static bool asc(const pending_render* a, const pending_render* b)
    {
        return a->index < b->index;
    }

    static bool desc(const pending_render* a, const pending_render* b)
    {
        return a->index > b->index;
    }
};

//#ifdef DEBUG
#define NUM_FPS_MODES 4
//#else
//#define NUM_FPS_MODES 3
//#endif

int
pscreen::handle_input(tms::event *ev, int action)
{
    if (ev->type == TMS_EV_KEY_PRESS) {
        switch (ev->data.key.keycode) {
#ifdef DEBUG
            case TMS_KEY_F1:
                settings["debug"]->set(!settings["debug"]->v.b);
                break;
#endif

            case TMS_KEY_F2:
                if (ev->data.key.mod & TMS_MOD_SHIFT) {
                    settings["display_fps"]->v.u8 ++;
                    settings["display_fps"]->v.u8 = settings["display_fps"]->v.u8 % NUM_FPS_MODES;

                    switch (settings["display_fps"]->v.u8) {
                        case 0:
                        default:
                            ui::message("FPS Display mode: OFF");
                            break;

                        case 1:
                            ui::message("FPS Display mode: Simple");
                            break;

                        case 2:
                            ui::message("FPS Display mode: Mean graph");
                            break;

                        case 3:
                            ui::message("FPS Display mode: Real FPS graph");
                            break;
                    }
                } else {
                    if (settings["render_gui"]->is_true()) {
                        ui::message("GUI: Hidden");
                        settings["render_gui"]->set(false);
                    } else {
                        ui::message("GUI: Shown");
                        settings["render_gui"]->set(true);
                    }
                    this->refresh_widgets();
                }
                break;

            case TMS_KEY_F3:
                if (ev->data.key.mod & TMS_MOD_SHIFT) {
                    static int snd_counter = 0;

                    int snd_id = snd_counter % SND__NUM;

                    sm_sound *snd = sm::get_sound_by_id(snd_id);

                    sm::play(snd, 0, 0, rand(), 1.f, false, 0, true);

                    ui::messagef("Played sound: %s", snd->name);

                    ++ snd_counter;
                }
                break;

            case TMS_KEY_F11:
                uint32_t flags = SDL_GetWindowFlags(_tms._window);

                if (settings["window_resizable"]->v.b) {
                    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
                        SDL_SetWindowFullscreen(_tms._window, 0);
                    else
                        SDL_SetWindowFullscreen(_tms._window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                } else {
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Principia",
                        "Principia does not support resizing the window while in-game. For fullscreen changes to take effect, please restart the game.",
                        0);
                }

                settings["window_fullscreen"]->v.b = (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == 0;
                break;
        }
    } else if (ev->type == TMS_EV_POINTER_UP) {
#ifndef NO_UI
        if (P.focused
# ifdef TMS_BACKEND_PC
                && !prompt_is_open
#endif
           ) {
            if (ping_cooldown == 25) {
                P.add_action(ACTION_VERSION_CHECK, 0);
                ping_cooldown = 0;
            } else {
                ping_cooldown++;
            }
        }
#endif
    }

    return EVENT_CONT;
}

int
pscreen::render()
{
    if (this->wm) {
        this->wm->render();
    }

    if (settings["display_fps"]->v.u8) {
        if (settings["display_fps"]->v.u8 == 3) {
            pscreen::fps_graph.push(_tms.dt, _tms.fps);
        } else {
            pscreen::fps_graph.push(_tms.dt, _tms.fps_mean);
        }

        pscreen::fps_graph.render(this);
    }

    return T_OK;
}

int
pscreen::post_render()
{
    if (_tms.state == TMS_STATE_QUITTING) {
        return T_OK;
    }

    glDisable(GL_DEPTH_TEST);

    if (!this->pending_renders.empty()) {
        std::sort(this->pending_renders.begin(), this->pending_renders.end(), &render_sorter::asc);

        uint8_t last_type = RT_NONE;
        bool call_opengl_stuff = false;

        for (std::deque<pending_render*>::const_iterator it = this->pending_renders.begin();
                it != this->pending_renders.end(); ++it) {
            const pending_render *pr = *it;

            switch (pr->type) {
                case RT_TEXT:
                    call_opengl_stuff = (last_type != pr->type);

                    switch (pr->subtype) {
                        case ST_TEXT: {
                            const pending_text *pt = static_cast<const pending_text*>(pr);

                            if (pt->set_color) {
                                pt->text->color = tvec4f(pt->r, pt->g, pt->b, pt->a);
                                pt->text->outline_color = tvec4f(pt->o_r, pt->o_g, pt->o_b, pt->a);
                            }

                            pt->text->render_at_pos(this->get_surface()->ddraw, pt->x, pt->y, pt->render_outline, call_opengl_stuff);

                            if (pt->do_free) {
                                delete pt->text;
                            }
                        } break;

                        case ST_GLYPH: {
                            const pending_glyph *pg = static_cast<const pending_glyph*>(pr);

                            render_glyph(this->get_surface()->ddraw, pg->glyph,
                                    pg->x, pg->y,
                                    tvec4f(pg->r, pg->g, pg->b, pg->a),
                                    tvec4f(pg->o_r, pg->o_g, pg->o_b, pg->a),
                                    pg->scale,
                                    pg->render_outline,
                                    call_opengl_stuff);
                        } break;
                    }
                    break;

                case RT_ROUNDED_SQUARE: {
                    const rounded_square *rs = static_cast<const rounded_square*>(pr);

                    if (last_type != pr->type) {
                        glBindTexture(GL_TEXTURE_2D, gui_spritesheet::atlas->texture.gl_texture);
                    }

                    tms_ddraw_set_rsprite_color(this->get_surface()->ddraw, TVEC4_INLINE(rs->color));
                    tms_ddraw_rsprite(this->get_surface()->ddraw, gui_spritesheet::get_sprite(S_ROUNDED_SQUARE),
                            rs->x, rs->y,
                            rs->width, rs->height,
                            rs->outline_width);
                } break;
            }

            last_type = pr->type;

            delete pr;
        }

        this->pending_renders.clear();
    }

    pscreen::message->render(this);

    ui::render();

    return T_OK;
}

void
pscreen::init()
{
    pscreen::text_username = new p_text(font::medium);
    pscreen::text_username->set_text(" ");

    pscreen::message = new game_message();
    pscreen::message->set_position(_tms.window_width/2.f, _tms.window_height/4.f);

    {
        float graph_width = _tms.xppcm * 1.5f;
        float graph_height = _tms.yppcm * 1.f;

        float x_offset = _tms.xppcm * 0.5f;
        float y_offset = _tms.yppcm * 0.5f;

        pscreen::fps_graph.set_size(graph_width, graph_height);
        pscreen::fps_graph.set_interval(0.1);
        pscreen::fps_graph.set_max_data_points(250);
        pscreen::fps_graph.set_min(0.0);

        /* Top center */
        //pscreen::fps_graph.set_position(_tms.window_width/2.f, _tms.window_height-graph_height/2.f - _tms.yppcm * 0.1f);

        pscreen::fps_graph.set_position(
                x_offset + graph_width/2.f,
                y_offset + graph_height/2.f);
    }
}

void
pscreen::refresh_username()
{
    char tmp[256];

    if (P.username) {
        if (P.num_unread_messages) {
            snprintf(tmp, 255, "%s [%d]", P.username, P.num_unread_messages);
        } else {
            snprintf(tmp, 255, "%s", P.username);
        }
    } else {
        snprintf(tmp, 255, "Not logged in");
    }

    pscreen::text_username->set_text(tmp);

    tms_infof("USERNAME REFRESHED ---------------");
}
