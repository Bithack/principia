#include <tms/core/tms.h>
#include "settings.hh"
#include <cmath>
#include "game.hh"

#ifdef BUILD_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include <errno.h>

struct shadow_res {int x; int y;};

_settings settings;

void
_settings::init()
{
    this->_data.clear();

    /** -Graphics **/
    this->add("debug",              S_BOOL,  false);
    this->add("postprocess",        S_BOOL,  false);
    this->add("enable_shadows",     S_BOOL,  true);

    this->add("uiscale",            S_FLOAT, 1.3f);

    this->add("blur_shadow_map",    S_BOOL,  false);
    this->add("swap_shadow_map",    S_BOOL,  false);

    // XXX: see git history
    this->add("shadow_map_resx",    S_INT32,   1280);
    this->add("shadow_map_resy",    S_INT32,   720);

#ifndef TMS_BACKEND_ANDROID
    // vsync is managed by the OS on Android
    this->add("vsync",              S_BOOL,  true);

    this->add("window_width",       S_INT32,   _tms.window_width);
    this->add("window_height",      S_INT32,   _tms.window_height);
    this->add("window_maximized",   S_BOOL,  0);
    this->add("window_fullscreen",  S_BOOL, false);
    this->add("window_resizable", S_BOOL, false);
    this->add("autosave_screensize",S_BOOL,  true);
#endif

    this->add("shadow_quality",     S_UINT8,  1);

#ifdef TMS_BACKEND_MOBILE
    this->add("ao_map_res",         S_INT32,   256);
#else
    this->add("ao_map_res",         S_INT32,   512);
#endif

    this->add("discard_framebuffer",        S_BOOL, -1);
    this->add("shadow_map_depth_texture",   S_BOOL, -1);
    this->add("shadow_map_precision",       S_BOOL, -1);

    this->add("swap_ao_map",        S_BOOL,  true);
#ifdef TMS_BACKEND_MOBILE
    this->add("gamma_correct",      S_BOOL,  false);
#else
    this->add("gamma_correct",      S_BOOL,  -1);
#endif
    this->add("enable_ao",          S_BOOL,  true);

#ifdef TMS_BACKEND_MOBILE
    this->add("shadow_ao_combine",         S_BOOL, 0);
#else
    this->add("shadow_ao_combine",         S_BOOL, 0);
#endif

    this->add("enable_bloom",       S_BOOL,  false);
    this->add("render_gui",         S_BOOL,  true);
    this->add("render_edev_labels", S_BOOL,  true);


    this->add("fv",                 S_INT32,   2); /* settings file version */
    this->add("jail_cursor",        S_BOOL,  false);
    this->add("smooth_cam",         S_BOOL,  false);
    this->add("cam_speed_modifier", S_FLOAT, 1.f);
    this->add("smooth_zoom",        S_BOOL,  false);
    this->add("zoom_speed",         S_FLOAT, 1.f);
    this->add("smooth_menu",        S_BOOL,  true);
    this->add("border_scroll_enabled",  S_BOOL,  true);
    this->add("border_scroll_speed",    S_FLOAT, 1.f);
    this->add("menu_speed",         S_FLOAT, 1.f);
    this->add("always_reload_data", S_BOOL,  false);
    /* TODO: Add setting for enabling or disabling menu sliding */

    /** -Player **/
    this->add("widget_control_sensitivity", S_FLOAT, 1.5f);
    this->add("rc_lock_cursor",     S_BOOL,  false);
    this->add("control_type",       S_UINT8,  1);

    /** -Audio **/
    this->add("volume", S_FLOAT,    1.0f,   true);
    this->add("muted",  S_BOOL,     false);

    /** -Debug **/
    this->add("display_object_id",          S_BOOL, false);
    this->add("display_grapher_value",      S_BOOL, false);
    this->add("display_wireless_frequency", S_BOOL, true);

    this->add("emulate_touch",      S_BOOL, false);

#ifdef TMS_BACKEND_MOBILE
    this->add("touch_controls",     S_BOOL, true);
#else
    this->add("touch_controls",     S_BOOL, false);
#endif

    this->add("tutorial", S_UINT32, 0);
    this->add("display_fps", S_UINT8, 0);

    this->add("num_workers", S_UINT8, SDL_GetCPUCount());
    tms_infof("num workers (real): %d", SDL_GetCPUCount());

    this->add("dna_sandbox_back", S_BOOL, false);
    this->add("hide_tips", S_BOOL, false);
    this->add("first_adventure", S_BOOL, true);

    this->add("score_ask_before_submitting", S_BOOL, false);
    this->add("score_automatically_submit", S_BOOL, true);

    char filename[1024];
    sprintf(filename, "%s/settings.ini", tbackend_get_storage_path());
    FILE *fh;

    if ((fh = fopen(filename, "r")) == NULL) {
        if (errno == ENOENT) {
            tms_infof("file doesn't exist, create it!");
            this->save();
        } else {
            /* todo: is the dir Principia created before this is called? */
            tms_infof("another error occured, no permissinos to the dir/file?");
        }
    } else {
        fclose(fh);
    }
}

bool
_settings::load(void)
{
    char filename[1024];
    sprintf(filename, "%s/settings.ini", tbackend_get_storage_path());
    FILE *fh = fopen(filename, "r");

    if (!fh) {
        tms_errorf("Unable to open settings file for reading.");
        return false;
    }

    char buf[256];

    while (fgets(buf, 256, fh) != NULL) {
        if (strchr(buf, '=') == NULL) continue;
        size_t sz = strlen(buf);
        char key[64];
        char val[64];
        int k = 0;

        bool on_key = true;

        for (int i = 0; i < sz; ++i) {
            if (buf[i] == '\n' || buf[i] == ' ') continue;

            if (k == 62) {
                if (on_key) {
                    key[k++] = '\0';
                    k = 0;
                    on_key = false;
                }
                else {
                    break;
                }
            }

            if (buf[i] == '=') {
                if (on_key) key[k++] = '\0';
                k = 0;
                on_key = false;
                continue;
            }

            if (on_key) {
                key[k++] = buf[i];
            } else {
                val[k++] = buf[i];
            }
        }

        val[k] = '\0';

        std::map<const char*, setting*>::iterator it = this->_data.find(key);
        if (it != this->_data.end()) {
            switch (it->second->type) {
                case S_INT8:
                    it->second->v.i = (int8_t)strtol(val, NULL, 10);
                    break;
                case S_INT32:
                    it->second->v.i = (int32_t)strtol(val, NULL, 10);
                    break;
                case S_UINT8:
                    it->second->v.u8 = (uint8_t)strtoul(val, NULL, 10);
                    break;
                case S_UINT32:
                    it->second->v.u32 = (uint32_t)strtoul(val, NULL, 10);
                    break;
                case S_FLOAT:
                    if (it->second->clamp) {
                        it->second->v.f = tclampf(atof(val), 0.f, 1.f);
                    } else {
                        it->second->v.f = atof(val);
                    }
                    break;
                case S_BOOL:
                    it->second->v.b = (int8_t)strtol(val, NULL, 10);
                    break;
                default:
                    tms_fatalf("Unknown setting type: %d", it->second->type);
                    break;
            }
        } else {
            tms_warnf("Unknown settings key: %s", key);
        }
    }

    fclose(fh);

    tms_infof("num workers (user): %d", settings["num_workers"]->v.i);

#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) {
        tms_debugf("Running on valgrind, forcing settings to bad!");
        settings["num_workers"]->v.i = 0;
        settings["ao_map_res"]->v.i = 256;
        settings["shadow_quality"]->v.u8 = 0;
        settings["shadow_map_resx"]->v.i = 256;
        settings["shadow_map_resy"]->v.i = 256;
        settings["shadow_map_precision"]->v.i = 0;
        settings["postprocess"]->v.b = false;
        settings["debug"]->v.b = false;
        settings["enable_shadows"]->v.b = false;
        settings["enable_bloom"]->v.b = false;
        settings["enable_ao"]->v.b = false;
        settings["render_edev_labels"]->v.b = false;
        settings["hide_tips"]->v.b = true;
    }
#endif

    return true;
}

bool
_settings::save(void)
{
#ifdef BUILD_VALGRIND
    // don't save the shitty valgrind settings file
    if (RUNNING_ON_VALGRIND) return true;
#endif

    /* TODO: store filename in the class, char[1024] settings_file, to be
     * set in init*/
    char filename[1024];
    sprintf(filename, "%s/settings.ini", tbackend_get_storage_path());
    FILE *fh = fopen(filename, "w+");

    if (!fh) {
        tms_errorf("An error occured when attempting to open settings file.");
        return false;
    }

    for (std::map<const char*, setting*>::iterator it = this->_data.begin();
            it != this->_data.end(); ++it) {
        switch (it->second->type) {
            case S_INT8:
                fprintf(fh, "%s=%d\n", it->first, it->second->v.i8);
                break;
            case S_INT32:
                fprintf(fh, "%s=%d\n", it->first, it->second->v.i);
                break;
            case S_UINT8:
                fprintf(fh, "%s=%u\n", it->first, it->second->v.u8);
                break;
            case S_UINT32:
                fprintf(fh, "%s=%u\n", it->first, it->second->v.u32);
                break;
            case S_FLOAT:
                fprintf(fh, "%s=%f\n", it->first, it->second->v.f);
                break;
            case S_BOOL:
                fprintf(fh, "%s=%d\n", it->first, it->second->v.b);
                break;
            default:
                tms_fatalf("Unknown setting type: %d", it->second->type);
                break;
        }
    }

    fclose(fh);

    return true;
}

void
_settings::clean(void)
{
}

_settings::~_settings()
{
    for (std::map<const char*, setting*>::iterator it = this->_data.begin();
            it != this->_data.end(); ++it) {
        delete it->second;
    }
}
