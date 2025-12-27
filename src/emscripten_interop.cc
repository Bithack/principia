#ifdef __EMSCRIPTEN__
#include "emscripten_interop.hh"
#include "const.hh"
#include "main.hh"
#include "network.hh"
#include "pkgman.hh"
#include "tms/backend/print.h"
#include "tms/core/err.h"
#include <cstdlib>
#include <emscripten/fetch.h>

/**
 * Emscripten-based level downloader using emscripten_fetch (async callbacks).
 * Slightly sloppy and copied from _download_level() in network.cc
 */
int _download_level_emscripten(void *p)
{
    _play_downloading_error = 0;
    if (_play_header_data.error_message) {
        free(_play_header_data.error_message);
        _play_header_data.error_message = 0;
    }
    if (_play_header_data.notify_message) {
        free(_play_header_data.notify_message);
        _play_header_data.notify_message = 0;
    }

    _play_header_data.error_action = 0;

    int arg = (intptr_t)p;
    int type = LEVEL_DB;
    bool derive = true;

    if (arg == 0) {
        type = LEVEL_DB;
        derive = false;
    } else if (arg == 1) {
        type = LEVEL_LOCAL;
        derive = true;
    } else if (arg == 2) {
        type = LEVEL_LOCAL;
        derive = false;
    }

    tms_infof("before: %d ++++++++++++++++++++++ ", _play_id);
    uint32_t new_id = type == LEVEL_LOCAL ? pkgman::get_next_level_id() : _play_id;
    uint32_t old_id = _play_id;

    char save_path[1024];
    sprintf(save_path, "%s/%d.plvl",
            pkgman::get_level_path(type),
            new_id);

    tms_debugf("save: %s", save_path);

    uint32_t r = 0;

    if (type == LEVEL_DB) {
        lvledit e;
        if (e.open(LEVEL_DB, new_id)) {
            r = e.lvl.revision;
            tms_debugf("we already have this DB level of revision %u", r);
        }
    }

    const char *host = strlen(_community_host) > 0 ? _community_host : P.community_host;

    char url[512];
    snprintf(url, sizeof(url) - 1, "https://%s/internal/%s_level?i=%d&h=%u",
            host,
            _play_download_for_pkg ? "get_package" :
                (type == LEVEL_DB ? "get" :
                    (derive ? "derive" : "edit")),
            _play_id, r);

    tms_infof("url: %s", url);

    _play_id = new_id;

    tms_infof("_play_id = %d -----------------------", _play_id);

    // callbacks
    static auto success_cb = [](emscripten_fetch_t *fetch) {
        const char *path = (const char*)fetch->userData;
        FILE *f = fopen(path, "wb");
        if (f) {
            fwrite(fetch->data, 1, fetch->numBytes, f);
            fclose(f);
            tms_infof("Saved level to %s", path);
        } else {
            tms_errorf("Could not open %s for writing", path);
            _play_downloading_error = DOWNLOAD_WRITE_ERROR;
        }
        emscripten_fetch_close(fetch);
        free((void*)path);
        _play_downloading = false;
    };

    static auto error_cb = [](emscripten_fetch_t *fetch) {
        const char *path = (const char*)fetch->userData;
        tms_errorf("Failed to download level (status %d) -> %s", fetch->status, path ? path : "(null)");
        emscripten_fetch_close(fetch);
        free((void*)path);
        if (fetch->status == 404) {
            _play_downloading_error = DOWNLOAD_GENERIC_ERROR;
        } else {
            _play_downloading_error = DOWNLOAD_CHECK_INTERNET_CONNECTION;
        }
        _play_downloading = false;
    };

    // allocate and pass save path as userData
    char *user = strdup(save_path);

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.onsuccess = +[](emscripten_fetch_t *fetch){ success_cb(fetch); };
    attr.onerror = +[](emscripten_fetch_t *fetch){ error_cb(fetch); };
    attr.userData = user;
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    _play_downloading = true;
    emscripten_fetch(&attr, url);
    return T_OK;
}

#endif
