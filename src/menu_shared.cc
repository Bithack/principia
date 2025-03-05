#include "menu_shared.hh"
#include "menu_main.hh"
#include "menu_create.hh"
#include "text.hh"
#include "gui.hh"
#include "widget_manager.hh"
#include "version.hh"

tms::texture *menu_shared::tex_bg;
tms::texture *menu_shared::tex_vignette;
tms::texture *menu_shared::tex_menu_bottom;
tms::texture *menu_shared::tex_principia;
tms::texture *menu_shared::tex_vert_line;
tms::texture *menu_shared::tex_hori_line;

enum fl_state menu_shared::fl_state = FL_WORKING;
struct menu_shared::featured_level menu_shared::fl[MAX_FEATURED_LEVELS_FETCHED];

enum fl_state menu_shared::contest_state = FL_WORKING;
struct menu_shared::featured_level menu_shared::contest;
struct menu_shared::featured_level menu_shared::contest_entries[MAX_FEATURED_LEVELS_FETCHED];

float menu_shared::fl_alpha = 0.f;
float menu_shared::contest_alpha = 0.f;
float menu_shared::gs_alpha = 0.f;

enum fl_state menu_shared::gs_state = FL_WORKING;
std::vector<struct gs_entry> menu_shared::gs_entries;

p_text *menu_shared::text_version;
p_text *menu_shared::text_message;

int menu_shared::bar_height;

void
menu_shared::init()
{
    {
        tms::texture *tex = new tms::texture();

        tex->gamma_correction = 0;
        tex->load("data/textures/menu/menu_bg.jpg");
        tex->colors = GL_RGB;

        tex->upload();

        menu_shared::tex_bg = tex;
    }

    {
        tms::texture *tex = new tms::texture();

        tex->gamma_correction = 0;
        tex->colors = GL_RGBA;
        tex->load("data/textures/menu/vignette.png");

        tex->upload();

        menu_shared::tex_vignette = tex;
    }

    {
        tms::texture *tex = new tms::texture();

        tex->gamma_correction = 0;
        tex->colors = GL_RGBA;

        tex->load("data/textures/menu/menu_bottom.png");

        tex->upload();

        menu_shared::tex_menu_bottom = tex;
    }

    {
        tms::texture *tex = new tms::texture();

        tex->gamma_correction = 0;
        tex->colors = GL_RGBA;

        tex->load("data/textures/menu/menu_principia.png");

        tex->upload();

        menu_shared::tex_principia = tex;
    }

    {
        tms::texture *tex = new tms::texture();

        tex->gamma_correction = 0;
        tex->colors = GL_RGBA;

        tex->load("data/textures/menu/vert_line.png");

        tex->upload();

        menu_shared::tex_vert_line = tex;
    }

    {
        tms::texture *tex = new tms::texture();

        tex->gamma_correction = 0;
        tex->colors = GL_RGBA;

        tex->load("data/textures/menu/hori_line.png");

        tex->upload();

        menu_shared::tex_hori_line = tex;
    }

    menu_shared::contest.sprite = 0;
    for (int x=0; x<MAX_FEATURED_LEVELS_FETCHED; ++x) {
        menu_shared::fl[x].sprite = 0;
        menu_shared::contest_entries[x].sprite = 0;
    }

    menu_shared::text_version = new p_text(font::medium, ALIGN_CENTER, ALIGN_CENTER);
    menu_shared::text_version->set_text(principia_version_string());

    float h = (gui_spritesheet::get_sprite(S_CONFIG)->height / 64.f) * 0.5f * _tms.yppcm * .7f;

    menu_shared::bar_height = std::max(menu_shared::text_version->get_height() + (_tms.yppcm * .125f), h);

    menu_shared::text_message = new p_text(font::medium, ALIGN_CENTER, ALIGN_CENTER);
}

void
menu_shared::refresh_message()
{
    tms_debugf("Refreshing message...");
    if (P.message) {
        const char *pch = strchr(P.message, ':');
        if (pch && strlen(P.message) > pch-P.message+1) {
            if (menu_shared::text_message->text) {
                if (strcmp(text_message->text, pch+1) == 0) {
                    return;
                }
            }
            menu_shared::text_message->set_text(pch+1);
            menu_shared::text_message->color.a = 0.f;
            menu_shared::text_message->outline_color.a = 0.f;
        }
    }
}

void
menu_shared::step()
{
    switch (menu_shared::fl_state) {
        case FL_UPLOAD:
            tms_texture_upload(&gui_spritesheet::atlas->texture);

            menu_shared::fl_state = FL_INIT;

            P.s_menu_main->get_wm()->areas[AREA_MENU_LEVELS].set_alpha(menu_shared::fl_alpha);
            P.s_menu_main->get_wm()->areas[AREA_MENU_SUB_LEVELS].set_alpha(menu_shared::fl_alpha);

            P.add_action(ACTION_REFRESH_WIDGETS, 0);
            break;

        case FL_ALPHA_IN: {
            menu_shared::fl_alpha += _tms.dt*1.f;

            if (menu_shared::fl_alpha > 1.f) {
                menu_shared::fl_alpha = 1.f;
                menu_shared::fl_state = FL_DONE;
            }

            P.s_menu_main->get_wm()->areas[AREA_MENU_LEVELS].set_alpha(menu_shared::fl_alpha);
            P.s_menu_main->get_wm()->areas[AREA_MENU_SUB_LEVELS].set_alpha(menu_shared::fl_alpha);
        } break;

        default:
            break;
    }

    switch (menu_shared::contest_state) {
        case FL_WAITING:
            if (menu_shared::fl_state > FL_UPLOAD) {
                menu_shared::contest_state = FL_INIT;
                P.s_menu_create->get_wm()->areas[AREA_MENU_BOTTOM_LEFT].set_alpha(menu_shared::contest_alpha);
                P.s_menu_create->get_wm()->areas[AREA_CREATE_CONTEST_TOP].set_alpha(menu_shared::contest_alpha);
                P.s_menu_create->get_wm()->areas[AREA_CREATE_CONTEST_BOTTOM].set_alpha(menu_shared::contest_alpha);
            }
            break;

        case FL_ALPHA_IN: {
                menu_shared::contest_alpha += _tms.dt*1.f;

                if (menu_shared::contest_alpha > 1.f) {
                    menu_shared::contest_alpha = 1.f;
                    menu_shared::contest_state = FL_DONE;
                }

                P.s_menu_create->get_wm()->areas[AREA_MENU_BOTTOM_LEFT].set_alpha(menu_shared::contest_alpha);
                P.s_menu_create->get_wm()->areas[AREA_CREATE_CONTEST_TOP].set_alpha(menu_shared::contest_alpha);
                P.s_menu_create->get_wm()->areas[AREA_CREATE_CONTEST_BOTTOM].set_alpha(menu_shared::contest_alpha);
        } break;

        default:
            break;
    }

    switch (menu_shared::gs_state) {
        case FL_WAITING:
            if (menu_shared::fl_state > FL_UPLOAD) {
                P.s_menu_create->get_wm()->areas[AREA_MENU_RIGHT_HCENTER].set_alpha(menu_shared::gs_alpha);
                menu_shared::gs_state = FL_INIT;
            }
            break;

        case FL_ALPHA_IN: {
            menu_shared::gs_alpha += _tms.dt*1.f;

            if (menu_shared::gs_alpha > 1.f) {
                menu_shared::gs_alpha = 1.f;
                menu_shared::gs_state = FL_DONE;
            }

            P.s_menu_create->get_wm()->areas[AREA_MENU_RIGHT_HCENTER].set_alpha(menu_shared::gs_alpha);
        } break;

        default:
            break;
    }
}
