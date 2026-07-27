#include "network.hh"
#include "main.hh"
#include "menu_shared.hh"
#include "object_factory.hh"
#include "ui.hh"
#include "version.hh"
#include "world.hh"
#include <tms/cpp.hh>

#if !defined(SCREENSHOT_BUILD) && !defined(SDL_PLATFORM_EMSCRIPTEN)

#include <curl/curl.h>

#define CURL_CUDDLES \
        part = curl_mime_addpart(mime); \
        curl_mime_name(part, "key"); \
        curl_mime_data(part, "cuddles", CURL_ZERO_TERMINATED);

static char          cookie_file[1024];

struct MemoryStruct {
    char *memory;
    size_t size;
};

struct level_write {
    const char *save_path;
    FILE *stream;
};

static void lock_curl(const char *invoker="N/A") {
    tms_infof("%s locking curl...", invoker);
    SDL_LockMutex(P.curl_mutex);
    tms_infof("%s locked curl!", invoker);
}

static void unlock_curl(const char *invoker="N/A") {
    tms_infof("%s unlocking curl...", invoker);
    SDL_UnlockMutex(P.curl_mutex);
    tms_infof("%s unlocked curl!", invoker);
}

#ifdef SDL_PLATFORM_SWITCH
#include <switch.h>
#endif

void network::init() {
#ifdef SDL_PLATFORM_SWITCH
    socketInitializeDefault();
#endif

    P.curl_mutex = SDL_CreateMutex();
    if (!P.curl_mutex)
        tms_fatalf("Unable to create curl mutex.");

    tms_infof("Initializing curl (v" LIBCURL_VERSION ")...");
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
    if (r != CURLE_OK) {
        tms_infof("ERR: %s", curl_easy_strerror(r));
        exit(1);
    }

    snprintf(cookie_file, 1024, "%s/cookie_jar", tms_storage_path());

    lock_curl("initial_loader-curl_init");
    P.curl = curl_easy_init();
    unlock_curl("initial_loader-curl_init");

    if (!P.curl) tms_fatalf("cURL could not be initialised.");
}

void network::soft_resume() {
    lock_curl("tproject_soft_resume");
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
    if (r != CURLE_OK) {
        tms_infof("ERR: %s", curl_easy_strerror(r));
        exit(1);
    }

    P.curl = curl_easy_init();
    unlock_curl("tproject_soft_resume");

    if (!P.curl) tms_fatalf("cURL could not be initialised.");
}

void network::soft_pause() {
    lock_curl("tproject_soft_pause");

    if (P.curl) {
        curl_easy_cleanup(P.curl);
        P.curl = 0;
    }
    curl_global_cleanup();

    unlock_curl("tproject_soft_pause");
}

void network::quit() {
    tms_infof("CURL easy cleanup...");

    lock_curl("tproject_quit");
    if (P.curl) {
        curl_easy_cleanup(P.curl);
        P.curl = 0;
    }
    curl_global_cleanup();

    unlock_curl("tproject_quit");
}

// Function used for saving a level that gets downloaded from a community site
static size_t _save_level(void *buffer, size_t size, size_t nmemb, void *stream) {
    tms_infof("Saving level...");
    if (!stream) {
        tms_errorf("No stream!");
        return 0;
    }

    long http_code = 0;
    curl_easy_getinfo(P.curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        struct level_write *out = (struct level_write*)stream;

        if (!out->stream) {
            tms_infof("opening stream at %s", out->save_path);
            out->stream = fopen(out->save_path, "wb");
            if (!out->stream) {
                tms_errorf("Unable to open stream to save_path %s", out->save_path);
                return 0;
            }
        }

        tms_infof("returning fwrite data");
        return fwrite(buffer, size, nmemb, out->stream);
    } else if (http_code == 303) {
        tms_debugf("http_code was 303. do nothing!");
        return 1;
    } else {
        tms_errorf("Unhandled http code: %ld", http_code);
    }

    return 0;
}

static size_t _parse_headers(char *buffer, size_t size, size_t nitems, void *userdata) {
    header_data *hd = static_cast<header_data*>(userdata);

    size_t len = size * nitems;

    char *colon = static_cast<char*>(memchr(buffer, ':', len));
    if (!colon)
        return len;

    std::string name(buffer, colon - buffer);

    const char *value = colon + 1;
    while (*value == ' ')
        value++;

    std::string val(value, buffer + len - value);

    while (!val.empty() && (val.back() == '\r' || val.back() == '\n'))
        val.pop_back();

    process_response_headers(name.c_str(), val.c_str(), hd);

    return nitems;
}

static void print_cookies(CURL *curl) {
    CURLcode res;
    struct curl_slist *cookies;
    struct curl_slist *nc;
    int i;

    tms_infof("Cookies, curl knows:");
    res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
    if (res != CURLE_OK) {
        tms_errorf("Curl curl_easy_getinfo failed: %s", curl_easy_strerror(res));
        exit(1);
    }

    nc = cookies, i = 1;
    while (nc) {
        tms_infof("[%d]: %s", i, nc->data);
        nc = nc->next;
        i++;
    }

    if (i == 1) {
        tms_infof("(none)\n");
    }

    curl_slist_free_all(cookies);
}

static int progress_cb(
        void *userdata,
        curl_off_t dltotal, curl_off_t dlnow,
        curl_off_t ultotal, curl_off_t ulnow
        ) {
    if (_tms.state == TMS_STATE_QUITTING) {
        tms_infof("Quitting in-progress request.");
        return 1;
    }

    return 0;
}

/**
 * Initialise defaults for cURL on each request
 */
static void init_curl_defaults(void *curl) {
    curl_easy_reset(P.curl);

#ifdef SDL_PLATFORM_ANDROID
    // XXX: Fix cert verification on Android
    curl_easy_setopt(P.curl, CURLOPT_SSL_VERIFYPEER, 0);
#endif

    char ua[512];
    snprintf(ua, 511, "Principia/%d (%s) (%s)",
        principia_version_code(), SDL_GetPlatform(), principia_version_string());

    curl_easy_setopt(P.curl, CURLOPT_USERAGENT, ua);

    curl_easy_setopt(P.curl, CURLOPT_HEADERFUNCTION, _parse_headers);

    CURLcode res = curl_easy_setopt(P.curl, CURLOPT_COOKIEFILE, cookie_file);

    if (res != CURLE_OK) {
        tms_errorf("!!! curl is not compiled with cookie support !!!");
    }

    curl_easy_setopt(P.curl, CURLOPT_COOKIEJAR, cookie_file);

    curl_easy_setopt(P.curl, CURLOPT_XFERINFOFUNCTION, progress_cb);
    curl_easy_setopt(P.curl, CURLOPT_NOPROGRESS, 0);

#ifdef DEBUG
    curl_easy_setopt(P.curl, CURLOPT_VERBOSE, 1);

    if (strcmp(P.community_host, "principia-web.uwu") == 0) {
        tms_warnf("[DEBUG] Using local community host 'principia-web.uwu', disabling certificate verification.");
        curl_easy_setopt(P.curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(P.curl, CURLOPT_SSL_VERIFYHOST, 0);
    }
#endif

    // Note: this may put token cookie in the log output
    //print_cookies(P.curl);
}

static size_t write_memory_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        tms_fatalf("write_memory_cb out of memory!");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int network::check_version_code(void *_unused) {
    CURLcode r;

    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;

    lock_curl("check_version_code");
    if (!P.curl) {
        unlock_curl("check_version_code");
        tms_errorf("unable to initialize curl handle!");
        return 0;
    }

    init_curl_defaults(P.curl);

    COMMUNITY_URL("internal/version_code");
    curl_easy_setopt(P.curl, CURLOPT_URL, url);

    curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
    curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 35L);

    if ((r = curl_easy_perform(P.curl)) == CURLE_OK) {
        if (chunk.size > 0) {
            handle_version_check(chunk.memory);
        } else {
            tms_errorf("could not check for latest version: invalid data");
        }
    } else {
        tms_errorf("could not check for latest version: %s", curl_easy_strerror(r));
    }

    unlock_curl("check_version_code");

    if (_tms.state == TMS_STATE_QUITTING)
        return 0;

    P.add_action(ACTION_REFRESH_HEADER_DATA, 0);
    return 0;
}

int network::get_featured_levels(void *_num) {
    CURLcode r;

    char featured_data_path[1024];
    snprintf(featured_data_path, 1023, "%s/fl.cache", tms_storage_cache_path());

    struct MemoryStruct chunk = {(char *)malloc(1), 0};

    char *fl_buf = 0;
    size_t fl_buf_size = 0;

    lock_curl("get_featured_levels");
    if (!P.curl) {
        tms_errorf("unable to initialize curl handle!");
        unlock_curl("get_featured_levels");
        return 0;
    }

    init_curl_defaults(P.curl);

    char url[256];
    snprintf(url, 255, "https://%s/internal/get_featured", P.community_host);

    curl_easy_setopt(P.curl, CURLOPT_URL, url);

    curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
    curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, (void*)&chunk);

    curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, 0);

    curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 35L);
    curl_easy_setopt(P.curl, CURLOPT_FOLLOWLOCATION, 1);

    if ((r = curl_easy_perform(P.curl)) == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(P.curl, CURLINFO_RESPONSE_CODE, &http_code);
        /* Anything other than 200 will make us attempt to read from our cache */

        if (http_code == 200) {
            fl_buf = chunk.memory;
            fl_buf_size = chunk.size;
        }
    } else {
        tms_errorf("curl error: %s", curl_easy_strerror(r));
    }

    unlock_curl("get_featured_levels");

    if (_tms.state == TMS_STATE_QUITTING)
        return 0;

    if (!fl_buf) {
        if (!load_featured_cache(featured_data_path, &fl_buf, &fl_buf_size))
            return 0;
    }

    if (!fl_buf || !parse_featured_levels(fl_buf, fl_buf_size))
        return 0;

    save_featured_cache(featured_data_path, fl_buf, fl_buf_size);

    menu_shared::fl_state = FL_UPLOAD;

    return 0;
}

int network::publish_level(void *p) {
    uint32_t level_id = _publish_lvl_id;
    int community_id    = 0;

    _publish_lvl_community_id = 0;
    _publish_lvl_uploading_error = false;

    struct MemoryStruct chunk = {(char *)malloc(1), 0};

    lvledit lvl;
    if (!lvl.open(LEVEL_LOCAL, level_id)) {
        tms_errorf("could not open level");
        return false;
    }

    lvl.lvl.revision++;
    lvl.save();

    char level_path[1024];
    pkgman::get_level_full_path(LEVEL_LOCAL, level_id, 0, level_path);

    lock_curl("publish_level");
    if (!P.curl) {
        unlock_curl("publish_level");
        tms_errorf("unable to initialize curl handle!");
        return 0;
    }

    struct header_data hd = {0};
    init_curl_defaults(P.curl);

    curl_mime *mime = curl_mime_init(P.curl);
    curl_mimepart *part;

    part = curl_mime_addpart(mime);
    curl_mime_name(part, "level");
    curl_mime_filedata(part, level_path);

    CURL_CUDDLES;

    COMMUNITY_URL("internal/upload");
    curl_easy_setopt(P.curl, CURLOPT_URL, url);

    curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, &hd);
    curl_easy_setopt(P.curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 15L);

    tms_infof("Publishing level %d...", level_id);
    CURLcode r = curl_easy_perform(P.curl);
    if (r == CURLE_OK) {
        handle_successful_publish(lvl, hd, &community_id);
    } else {
        tms_errorf("lvl publish curl_easy_perform failed: %s\n", curl_easy_strerror(r));

        if (r == CURLE_OPERATION_TIMEDOUT)
            ui::message("Operation timed out. Your internet connection seems to be unstable! Please try again.", true);
        else if (r == CURLE_COULDNT_RESOLVE_HOST)
            ui::message("Error: Unable to resolve hostname. Please check your internet connection.", true);
        else
            ui::message("An unknown error occurred when publishing your level. Check your internet connection.", true);

        _publish_lvl_uploading = false;
        _publish_lvl_uploading_error = true;
    }

    curl_mime_free(mime);
    unlock_curl("publish_level");

    P.add_action(ACTION_AUTOSAVE, 0);

    if (!lvl.save()) {
        tms_errorf("Unable to save the level after publish!");
    }

    _publish_lvl_community_id = community_id;
    _publish_lvl_uploading = false;

    return T_OK;
}

int network::submit_score(void *p) {
    submit_score_data data;

    if (!prepare_submit_score(&data)) {
        _submit_score_done = true;
        return T_ERR;
    }

    lock_curl("submit_score");
    if (!P.curl) {
        tms_errorf("unable to initialize curl handle!");
        unlock_curl("submit_score");
        _submit_score_done = true;
        return T_ERR;
    }

    struct header_data hd = {0};
    init_curl_defaults(P.curl);

    curl_mime* mime = curl_mime_init(P.curl);
    curl_mimepart* part;

    part = curl_mime_addpart(mime);
    curl_mime_name(part, "data.bin");
    curl_mime_filedata(part, data.progress_path);

    char tmp[32];
    sprintf(tmp, "%u", W->level.community_id);

    part = curl_mime_addpart(mime);
    curl_mime_name(part, "lvl_id");
    curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);

    CURL_CUDDLES;

    COMMUNITY_URL("internal/submit_score");
    curl_easy_setopt(P.curl, CURLOPT_URL, url);

    curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, &hd);

    curl_easy_setopt(P.curl, CURLOPT_MIMEPOST, mime);

    curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 15L);

    CURLcode r = curl_easy_perform(P.curl);
    if (r == CURLE_OK)
        handle_submit_score(hd, 200);
    else if (r == CURLE_OPERATION_TIMEDOUT || r == CURLE_COULDNT_RESOLVE_HOST)
        ui::message("Unable to submit your score.\nYour internet connection seems to be unstable! Please try again.", true);
    else
        ui::message("An unknown error occurred when submitting your score. Check your internet connection and try again.", true);

    curl_mime_free(mime);

    _submit_score_done = true;

    unlock_curl("submit_score");

    return T_OK;
}

/** --Login **/
int network::login(void *p) {
    struct login_data *data = static_cast<struct login_data*>(p);

    long http_code = 0;

    lock_curl("login");
    if (!P.curl) {
        tms_errorf("unable to initialize curl handle!");
        unlock_curl("login");
        return 0;
    }

    struct header_data hd = {0};
    init_curl_defaults(P.curl);

    curl_mime *mime = curl_mime_init(P.curl);
    curl_mimepart* part;

    part = curl_mime_addpart(mime);
    curl_mime_name(part, "username");
    curl_mime_data(part, data->username, CURL_ZERO_TERMINATED);

    part = curl_mime_addpart(mime);
    curl_mime_name(part, "password");
    curl_mime_data(part, data->password, CURL_ZERO_TERMINATED);

    CURL_CUDDLES;

    COMMUNITY_URL("internal/login");
    curl_easy_setopt(P.curl, CURLOPT_URL, url);

    curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, &hd);
    curl_easy_setopt(P.curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 15L);

    CURLcode r;
    if ((r = curl_easy_perform(P.curl)) == CURLE_OK) {
        curl_easy_getinfo(P.curl, CURLINFO_RESPONSE_CODE, &http_code);
        handle_login(hd, http_code);
    } else
        tms_errorf("curl_easy_perform failed: %s\n", curl_easy_strerror(r));

    curl_mime_free(mime);

    unlock_curl("login");

    delete data;

    return 0;
}

/** --Register **/
int network::register_user(void *p) {
    struct register_data *data = static_cast<struct register_data*>(p);
    int res = T_OK;

    CURLcode r;

    lock_curl("register");

    if (P.curl) {
        struct header_data hd = {0};
        init_curl_defaults(P.curl);

        curl_mime *mime = curl_mime_init(P.curl);
        curl_mimepart *part;

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "username");
        curl_mime_data(part, data->username, CURL_ZERO_TERMINATED);

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "email");
        curl_mime_data(part, data->email, CURL_ZERO_TERMINATED);

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "password");
        curl_mime_data(part, data->password, CURL_ZERO_TERMINATED);

        CURL_CUDDLES;

        COMMUNITY_URL("internal/register");
        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, &hd);
        curl_easy_setopt(P.curl, CURLOPT_MIMEPOST, mime);
        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 15L);

        r = curl_easy_perform(P.curl);

        if (r == CURLE_OK) {
            // Check for messages
            if (hd.error_message) {
                ui::message(hd.error_message);
                ui::emit_signal(SIGNAL_REGISTER_FAILED);

                free(hd.error_message);
            }

            if (hd.notify_message) {
                ui::message(hd.notify_message);
                ui::emit_signal(SIGNAL_REGISTER_SUCCESS);

                free(hd.notify_message);
            }
        } else {
            if (r != CURLE_OK) {
                tms_errorf("curl_easy_perform failed: %s", curl_easy_strerror(r));
            } else {
                tms_errorf("No data received.");
            }
            res = T_ERR;
        }

        curl_mime_free(mime);
    } else {
        tms_errorf("Unable to initialize curl handle.");
        res = T_ERR;
    }

    unlock_curl("register");

    if (res != T_OK) {
        ui::emit_signal(SIGNAL_REGISTER_FAILED);
    }

    free(data);

    return res;
}

int network::download_pkg(void *_p) {
    CURLcode res;

    char save_path[1024];
    sprintf(save_path, "%s/%d.ppkg",
            pkgman::get_pkg_path(_play_pkg_type),
            _play_pkg_id);

    tms_debugf("save: %s", save_path);

    COMMUNITY_URL("internal/get_package?i=%d", _play_pkg_id);
    long http_code = 0;

    struct level_write save_data = {
        save_path,
        NULL
    };

    lock_curl("download_pkg");
    if (P.curl) {
        init_curl_defaults(P.curl);

        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, _save_level);
        curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, &save_data);
        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 30L);

        res = curl_easy_perform(P.curl);

        if (res != CURLE_OK) {
            tms_errorf("error while downloadnig file: %s", curl_easy_strerror(res));
            _play_pkg_downloading_error = true;
        } else {
            curl_easy_getinfo(P.curl, CURLINFO_RESPONSE_CODE, &http_code);

            if (http_code == 404) {
                _play_pkg_downloading_error = true;
            } else {
                tms_debugf("got http code %ld", http_code);
            }
        }
    }
    unlock_curl("download_pkg");

    if (save_data.stream) {
        fclose(save_data.stream);
    }

    pkginfo p;
    if (p.open(_play_pkg_type, _play_pkg_id)) {
        tms_debugf("pkg name: %s", p.name);
        tms_debugf("pkg num_levels: %d", p.num_levels);
        tms_debugf("pkg first_is_menu: %d", p.first_is_menu);
        tms_debugf("pkg unlock_count: %d", p.unlock_count);

        /* package was successfully downloaded, now loop through the levels
         * and download them all !!!! */
        for (int x=0; x<p.num_levels; x++) {
            _play_downloading_error = 0;
            _play_id = p.levels[x];
            _play_type = LEVEL_DB;
            _play_download_for_pkg = true;
            network::download_level(0);

            if (_play_downloading_error) {
                _play_pkg_downloading_error = true;
                break;
            }
        }
    } else {
        _play_pkg_downloading_error = true;
    }
    //p.save();

    _play_pkg_downloading = false;

    return T_OK;
}

/* start with p = 1 to download a derivative to sandbox, p=2 to edit the level (assuming it's your own) */
int network::download_level(void *p) {
    // begin by resetting error state
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

    CURLcode res;

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

    if (type == LEVEL_LOCAL) {
        if (derive) {
            tms_debugf("downloading derivative level");
        } else {
            tms_debugf("Attempting to edit a level as your own.");
        }
    }

    char save_path[1024];
    sprintf(save_path, "%s/%d.plvl",
            pkgman::get_level_path(type),
            new_id);

    tms_debugf("save: %s", save_path);

    uint32_t r = 0;

    if (type == LEVEL_DB) {
        lvledit e;
        if(e.open(LEVEL_DB, new_id)) {
            /* File already exists, check if we actually need to download a new version. */
            r = e.lvl.revision;
            tms_debugf("we already have this DB level of revision %u", r);
        }
    }

    const char *host = strlen(_community_host) > 0 ? _community_host : P.community_host;

    char url[256];
    snprintf(url, 255, "https://%s/internal/%s_level?i=%d&h=%u",
            host,
            _play_download_for_pkg ? "get_package" :
                (type == LEVEL_DB ? "get" :
                    (derive ? "derive" : "edit")),
            _play_id, r);

    tms_infof("url: %s", url);

    long http_code = 0;
    bool require_login = false;

    struct level_write save_data ={
        save_path,
        NULL
    };

    _play_id = new_id;

    tms_infof("_play_id = %d -----------------------", _play_id);

    lock_curl("download_level");
    if (P.curl) {
        init_curl_defaults(P.curl);

        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, _save_level);
        curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, &save_data);

        curl_easy_setopt(P.curl, CURLOPT_WRITEHEADER, &_play_header_data);

        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 30L);
        curl_easy_setopt(P.curl, CURLOPT_TIMEOUT, 60L);

        tms_infof("we get here first");

        res = curl_easy_perform(P.curl);
        P.add_action(ACTION_REFRESH_HEADER_DATA, 0);

        if (res != CURLE_OK) {
            tms_infof("we get here");
            if (res == CURLE_WRITE_ERROR) {
                _play_downloading_error = DOWNLOAD_WRITE_ERROR;
            } else {
                _play_downloading_error = DOWNLOAD_CHECK_INTERNET_CONNECTION;
            }

            tms_errorf("error while downloading file: [%d]%s", res, curl_easy_strerror(res));
        }

        curl_easy_getinfo(P.curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code == 404) {
            _play_downloading_error = DOWNLOAD_GENERIC_ERROR;
        } else {
            tms_debugf("got http code %ld", http_code);

            if (http_code == 303) {
                if (type == LEVEL_LOCAL && !derive) {
                    require_login = true;
                    tms_errorf("user must log in before editing the level.");
                    _play_downloading_error = 1;
                }
            } else if (http_code == 500) {
                switch (_play_header_data.error_action) {
                    case ERROR_ACTION_LOG_IN:
                        require_login = true;
                        break;
                }

                _play_downloading_error = 1;
            }
        }
    }
    unlock_curl("download_level");

    if (require_login) {
        if (type == LEVEL_DB) {
            ui::next_action = ACTION_OPEN_PLAY;
        } else {
            if (derive) {
                ui::next_action = ACTION_DERIVE;
            } else {
                ui::next_action = ACTION_EDIT;
            }
        }

        ui::open_dialog(DIALOG_LOGIN);
    }

    if (save_data.stream) {
        tms_debugf("Closing save data stream.");
        fclose(save_data.stream);
    }

    if (!_play_downloading_error) {
        lvledit e;
        if (e.open(type, new_id)) {
            if (derive) {
                tms_debugf("derive = true");
            } else {
                tms_debugf("derive = false");
            }
            /* make sure the level has its community_id set correctly, just in case.
             * This should have been set when the level was published but it is possible
             * for the publisher to alter the id stored in the level file using tools
             * such as wireshark */
            if (type == LEVEL_LOCAL && derive) {
                tms_debugf("setting derive properties");
                e.lvl.community_id = 0;
                e.lvl.parent_id = old_id;
                e.lvl.parent_revision = e.lvl.revision;
                e.lvl.revision = 0;
                e.lb.size -= 1;
            } else if (type == LEVEL_LOCAL && !derive) {
                tms_debugf("editing level, do nothing");
                e.lb.size -= 1;
            } else {
                e.lvl.community_id = old_id;
            }
            e.save();
        } else {
            tms_errorf("wtf? we just downloaded it and couldnt open it");
        }
    } else {
        tms_debugf("An error occurred while downloading the level.");
        _play_id = old_id;
    }

    _play_downloading = false;

    return T_OK;
}

/**
 * Get the community site login token from cURL, intended for the user to be automatically
 * logged into the Android webview.
*/
extern "C" void P_get_cookie_data(char **token) {
    *token = 0;

    if (_tms.state == TMS_STATE_QUITTING) {
        return;
    }

    lock_curl("get_cookie_data");
    if (P.curl) {
        init_curl_defaults(P.curl);

        COMMUNITY_URL("internal/login");
        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        struct curl_slist *cookies;
        CURLcode res = curl_easy_getinfo(P.curl, CURLINFO_COOKIELIST, &cookies);

        if (res == CURLE_OK) {
            P.add_action(ACTION_REFRESH_HEADER_DATA, 0);

            while (cookies) {
                int nt = 0;
                int found_token = 0;
                char *d = cookies->data;
                tms_debugf("cookie: %s", d);
                while (*d != '\0') {
                    if (nt == 5) {
                        if (strncmp(d, "_PRINCSECURITY", 14) == 0)
                            found_token = 1;
                    }
                    if (nt == 6) {
                        if (found_token) *token = d;
                        break;
                    }
                    if (*d == '\t') nt++;
                    d ++;
                }
                cookies = cookies->next;
            }
        }
    }
    unlock_curl("get_cookie_data");
}

#endif
