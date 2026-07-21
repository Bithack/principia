#include "const.hh"
#include "main.hh"
#include "menu_shared.hh"
#include "network.hh"
#include "pkgman.hh"
#include "ui.hh"
#include <cstdlib>
#include <string>

#include <tms/cpp.hh>

#ifdef SDL_PLATFORM_EMSCRIPTEN

#include <emscripten.h>
#include <emscripten/fetch.h>

void network::init() {}
void network::soft_resume() {}
void network::soft_pause() {}
void network::quit() {}

typedef void (*fetch_callback_t)(int status, const char *headers, const void *body, size_t body_size, void *userdata);

static void parse_js_headers(int status, const char *headers, void *userdata) {
    header_data *hd = static_cast<header_data*>(userdata);

    char *copy = strdup(headers);

    char *saveptr;
    for (char *line = strtok_r(copy, "\n", &saveptr); line; line = strtok_r(nullptr, "\n", &saveptr)) {
        char *colon = strchr(line, ':');
        if (!colon)
            continue;

        *colon = 0;

        char *value = colon + 1;
        while (*value == ' ')
            value++;

        process_response_headers(line, value, hd);
    }

    free(copy);
}

extern "C" {

EMSCRIPTEN_KEEPALIVE void fetch_finished(int status, const char *headers,  const void *body, size_t body_size, fetch_callback_t callback, void *userdata) {
    callback(status, headers, body, body_size, userdata);
}

}

EM_JS(void, emscripten_fetch_request,
        (const char *method, const char *url, const char *headers, const void *body, size_t body_size, fetch_callback_t callback, void *userdata), {
    const methodStr = UTF8ToString(method);
    const urlStr = UTF8ToString(url);

    let requestBody = null;
    if (body && body_size > 0) {
        requestBody = HEAPU8.slice(body, body + body_size);
    }

    const headerString = headers ? UTF8ToString(headers) : "";

    const requestHeaders = {};

    if (headerString.length > 0) {
        headerString.split("\n").forEach(line => {
            const split = line.indexOf(":");
            if (split >= 0) {
                const key = line.substring(0, split);
                const value = line.substring(split + 1).trim();
                requestHeaders[key] = value;
            }
        });
    }

    fetch(urlStr, {
        method: methodStr,
        headers: requestHeaders,
        credentials: "include",
        body: requestBody
    }).then(response => {
        let headersOut = "";

        response.headers.forEach((value, key) => {
            headersOut += key + ":" + value + "\n";
        });

        return response.arrayBuffer().then(buffer => {
            const data = new Uint8Array(buffer);

            const headersPtr = stringToNewUTF8(headersOut);

            const bodyPtr = _malloc(data.length);
            HEAPU8.set(data, bodyPtr);

            Module.ccall(
                "fetch_finished",
                null,
                ["number", "number", "number", "number", "number", "number"],
                [response.status, headersPtr, bodyPtr, data.length, callback, userdata]
            );

            _free(headersPtr);
            _free(bodyPtr);
        });
    }).catch(() => {
        Module.ccall(
            "fetch_finished",
            null,
            ["number", "number", "number", "number", "number", "number"],
            [0, 0, 0, 0, callback, userdata]
        );
    });
});


int network::check_version_code(void *p) {
    COMMUNITY_URL("internal/version_code");

    static auto version_fetch_callback = [](int status, const char *headers, const void *body, size_t body_size, void *userdata) {
        if (status != 200 || !body) {
            tms_errorf("could not check for latest version: invalid data");
            return;
        }

        header_data hd = {};
        parse_js_headers(status, headers, &hd);

        handle_version_check((char *)body);

        P.add_action(ACTION_REFRESH_HEADER_DATA, 0);
    };

    emscripten_fetch_request(
        "GET",
        url,
        nullptr,
        nullptr,
        0,
        version_fetch_callback,
        nullptr);

    return 0;
}

static char *fl_buf = 0;
static size_t fl_buf_size = 0;

int network::get_featured_levels(void *_num) {
    char url[256];
    snprintf(url, sizeof(url), "https://%s/internal/get_featured", P.community_host);

    static auto success_cb = [](emscripten_fetch_t *fetch) {
        fl_buf = (char*)malloc(fetch->numBytes);
        memcpy(fl_buf, fetch->data, fetch->numBytes);
        fl_buf_size = fetch->numBytes;

        char featured_data_path[1024];
        snprintf(featured_data_path, 1023, "%s/fl.cache", tms_storage_cache_path());

        tms_infof("(Emscripten) Finished downloading featured levels");

        emscripten_fetch_close(fetch);

        if (_tms.state == TMS_STATE_QUITTING)
            return;

        if (!fl_buf) {
            if (!load_featured_cache(featured_data_path, &fl_buf, &fl_buf_size))
                return;
        }

        if (!fl_buf || !parse_featured_levels(fl_buf, fl_buf_size))
            return;

        save_featured_cache(featured_data_path, fl_buf, fl_buf_size);

        menu_shared::fl_state = FL_UPLOAD;
    };

    static auto error_cb = [](emscripten_fetch_t *fetch) {
        tms_infof("(Emscripten) Failed to download featured levels");
        emscripten_fetch_close(fetch);
    };

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);

    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = +[](emscripten_fetch_t *fetch){ success_cb(fetch); };
    attr.onerror = +[](emscripten_fetch_t *fetch){ error_cb(fetch); };

    emscripten_fetch(&attr, url);

    return 0;
}

int network::publish_level(void *p) {
    uint32_t level_id = _publish_lvl_id;

    auto *lvl = new lvledit;

    _publish_lvl_community_id = 0;
    _publish_lvl_uploading_error = false;

    if (!lvl->open(LEVEL_LOCAL, level_id)) {
        tms_errorf("could not open level");
        return false;
    }

    lvl->lvl.revision++;
    lvl->save();

    char level_path[1024];
    pkgman::get_level_full_path(
        LEVEL_LOCAL,
        level_id,
        0,
        level_path);

    FILE *f = fopen(level_path, "rb");
    if (!f) {
        tms_errorf("could not open level file");
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *data = malloc(size);

    if (fread(data, 1, size, f) != size) {
        fclose(f);
        free(data);
        tms_errorf("could not read level file");
        return false;
    }
    fclose(f);

    tms_infof("Publishing level %d...", level_id);

    std::string boundary = "PrincipiaBoundary" + std::to_string(rand());
    std::string payload =
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"key\"\r\n"
        "\r\n"
        "cuddles\r\n"
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"level\"; filename=\"level\"\r\n"
        "Content-Type: application/octet-stream\r\n"
        "\r\n" +
        std::string((char*)data, size) +
        "\r\n--" + boundary + "--\r\n";

    static auto publish_level_fetch_callback = [](int status, const char *headers, const void *body, size_t body_size, void *userdata) {
        tms_infof("(Emscripten) Publish level callback with status %d", status);

        if (status == 0) {
            tms_errorf("(Emscripten) Publish level failed");

            ui::message("An unknown error occurred when publishing your level. Check your internet connection.", true);

            _publish_lvl_uploading = false;
            _publish_lvl_uploading_error = true;
            return;
        }

        auto *lvl = static_cast<lvledit *>(userdata);

        header_data hd = {};
        parse_js_headers(status, headers, &hd);

        int community_id = 0;
        if (status == 200) {
            handle_successful_publish(*lvl, hd, &community_id);
        } else {
            tms_errorf("level publish failed with HTTP status %d", status);

            ui::message("An unknown error occurred when publishing your level. Check your internet connection.", true);

            _publish_lvl_uploading = false;
            _publish_lvl_uploading_error = true;
        }

        if (!lvl->save()) {
            tms_errorf("Unable to save the level after publish!");
        }

        _publish_lvl_community_id = community_id;
        _publish_lvl_uploading = false;

        P.add_action(ACTION_AUTOSAVE, 0);

        free(lvl);
    };

    COMMUNITY_URL("internal/upload");

    emscripten_fetch_request(
        "POST",
        url,
        std::string("Content-Type: multipart/form-data; boundary=" + boundary).c_str(),
        payload.c_str(),
        payload.size(),
        publish_level_fetch_callback,
        lvl);

    free(data);

    return T_OK;
}

int network::submit_score(void *p) {
    submit_score_data data;

    if (!prepare_submit_score(&data)) {
        _submit_score_done = true;
        return false;
    }

    FILE *f = fopen(data.progress_path, "rb");
    if (!f) {
        tms_errorf("could not open progress file");
        _submit_score_done = true;
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *data_bin = malloc(size);

    if (fread(data_bin, 1, size, f) != size) {
        fclose(f);
        tms_errorf("could not read progress file");
        _submit_score_done = true;
        return false;
    }
    fclose(f);

    std::string boundary = "PrincipiaBoundary" + std::to_string(rand());
    std::string payload =
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"data.bin\"; "
        "filename=\"data.bin\"\r\n"
        "Content-Type: application/octet-stream\r\n"
        "\r\n" +
        std::string(static_cast<const char *>(data_bin), size) +
        "\r\n"
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"lvl_id\"\r\n"
        "\r\n" +
        std::to_string(data.community_id) +
        "\r\n"
        "--" + boundary + "--\r\n";

    static auto submit_score_fetch_callback = [](int status, const char *headers, const void *body, size_t body_size, void *userdata) {
        tms_infof("(Emscripten) Submit score callback with status %d", status);

        header_data hd = {};
        parse_js_headers(status, headers, &hd);

        handle_submit_score(hd, status);
        _submit_score_done = true;
    };

    COMMUNITY_URL("internal/submit_score");

    emscripten_fetch_request(
        "POST",
        url,
        std::string("Content-Type: multipart/form-data; boundary=" + boundary).c_str(),
        payload.data(),
        payload.size(),
        submit_score_fetch_callback,
        nullptr);

    return T_OK;
}

int network::login(void *p) {
    login_data *data = static_cast<login_data*>(p);

    char *post_data = nullptr;
    SDL_asprintf(
        &post_data,
        "username=%s&password=%s&key=cuddles",
        data->username,
        data->password);

    COMMUNITY_URL("internal/login");

    static auto login_fetch_callback = [](int status, const char *headers, const void *body, size_t body_size, void *userdata) {
        header_data hd = {};
        parse_js_headers(status, headers, &hd);

        handle_login(hd, status);
    };

    emscripten_fetch_request(
        "POST",
        url,
        "Content-Type: application/x-www-form-urlencoded",
        post_data,
        SDL_strlen(post_data),
        login_fetch_callback,
        data);

    SDL_free(post_data);

    return T_OK;
}

int network::register_user(void *p) { return 0; }
int network::download_pkg(void *p) { return 0; }

/**
 * Emscripten-based level downloader using emscripten_fetch (async callbacks).
 * Slightly sloppy and copied from _download_level() in network.cc
 */
int network::download_level(void *p) {
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
