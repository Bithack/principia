#include "network.hh"
#include "crc.hh"
#include "game.hh"
#include "gui.hh"
#include "main.hh"
#include "menu_shared.hh"
#include "object_factory.hh"
#include "progress.hh"
#include "text.hh"
#include "tms/backend/print.h"
#include "tms/core/err.h"
#include "ui.hh"
#include "version.hh"

/* Publish level variables */
uint32_t      _publish_lvl_community_id;
uint32_t      _publish_lvl_id;
bool          _publish_lvl_with_pkg = false;
bool          _publish_lvl_set_locked = false;
bool          _publish_lvl_lock = false;
volatile bool _publish_lvl_uploading = false;
bool          _publish_lvl_uploading_error = false;

/* Download pkg variables */
uint32_t      _play_pkg_id;
uint32_t      _play_pkg_type;
uint32_t      _play_pkg_downloading = false;
uint32_t      _play_pkg_downloading_error = false;

/* Download level variables */
uint32_t      _play_id;
char          _community_host[512] = {0}; /* Temporary input host from principia:// url, not to be confused with P.community_host */
uint32_t      _play_type;
bool          _play_lock;
volatile bool _play_downloading = false;
volatile bool _play_download_for_pkg = false;
volatile int  _play_downloading_error = 0;
struct header_data _play_header_data = {0};

#ifdef BUILD_CURL

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

static void
lock_curl(const char *invoker="N/A")
{
    tms_infof("%s locking curl...", invoker);
    SDL_LockMutex(P.curl_mutex);
    tms_infof("%s locked curl!", invoker);
}

static void
unlock_curl(const char *invoker="N/A")
{
    tms_infof("%s unlocking curl...", invoker);
    SDL_UnlockMutex(P.curl_mutex);
    tms_infof("%s unlocked curl!", invoker);
}

void
init_curl()
{
    tms_debugf("Creating curl mutex");
    P.curl_mutex = SDL_CreateMutex();
    if (!P.curl_mutex) {
        tms_fatalf("Unable to create curl mutex.");
    }

    P.focused = 1;

    tms_infof("Initializing curl (v" LIBCURL_VERSION ")...");
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
    if (r != CURLE_OK) {
        tms_infof("ERR: %s", curl_easy_strerror(r));
        exit(1);
    }

    snprintf(cookie_file, 1024, "%s/c", tbackend_get_storage_path());

    lock_curl("initial_loader-curl_init");
    P.curl = curl_easy_init();
    unlock_curl("initial_loader-curl_init");

    if (!P.curl) tms_fatalf("cURL could not be initialised.");
}

void
soft_resume_curl()
{
    lock_curl("tproject_soft_resume");
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
    if (r != CURLE_OK) {
        tms_infof("ERR: %s", curl_easy_strerror(r));
        exit(1);
    }
    P.curl = curl_easy_init();
    unlock_curl("tproject_soft_resume");
}

void
soft_pause_curl()
{
    lock_curl("tproject_soft_pause");

    if (P.curl) {
        curl_easy_cleanup(P.curl);
        P.curl = 0;
    }
    curl_global_cleanup();

    unlock_curl("tproject_soft_pause");
}

void
quit_curl()
{
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
static size_t _save_level(void *buffer, size_t size, size_t nmemb, void *stream)
{
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


static size_t
_parse_headers(void *buffer, size_t size, size_t nmemb, void *data)
{
    char *buf = (char*)buffer;
    char *pch = strchr(buf, ':');

    if (pch && strlen(buf) > pch-buf+1) {
        buf[pch-buf] = '\0';
        buf[nmemb-2] = '\0';
        char *v = pch+2;

        if (data) {
            if (strcmp(buf, "x-error-message") == 0)
                ((struct header_data*)data)->error_message = strdup(v);
            else if (strcmp(buf, "x-error-action") == 0)
                ((struct header_data*)data)->error_action = atoi(v);
            else if (strcmp(buf, "x-notify-message") == 0)
                ((struct header_data*)data)->notify_message = strdup(v);
        }

        if (strcmp(buf, "x-principia-user-id") == 0)
            P.user_id = atoi(v);
        else if (strcmp(buf, "x-principia-user-name") == 0)
            P.username = strdup(v);
        else if (strcmp(buf, "x-principia-unread") == 0)
            P.num_unread_messages = atoi(v);
    }

    return nmemb;
}

static void
print_cookies(CURL *curl)
{
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

static int
progress_cb(
        void *userdata,
        curl_off_t dltotal, curl_off_t dlnow,
        curl_off_t ultotal, curl_off_t ulnow
        )
{
    if (_tms.state == TMS_STATE_QUITTING) {
        tms_infof("Quitting in-progress request.");
        return 1;
    }

    return 0;
}

/**
 * Initialise defaults for cURL on each request
 */
void
init_curl_defaults(void *curl)
{
    curl_easy_reset(P.curl);

#ifdef TMS_BACKEND_ANDROID
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
#endif

    // Note: this may put token cookie in the log output
    //print_cookies(P.curl);
}

static size_t
write_memory_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
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

int
_check_version_code(void *_unused)
{
    CURLcode r;

    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;

    lock_curl("check_version_code");
    if (P.curl) {
        init_curl_defaults(P.curl);

        COMMUNITY_URL("internal/version_code");
        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
        curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, (void*)&chunk);
        curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 35L);

        if ((r = curl_easy_perform(P.curl)) == CURLE_OK) {
            if (chunk.size > 0) {
                int server_version_code = atoi(chunk.memory);

                if (server_version_code > principia_version_code()) {
                    P.new_version_available = true;
                    ui::message("A new version of Principia is available!", true);
                }

                tms_debugf("Client: %d. Server: %d", principia_version_code(), server_version_code);
                if (P.message) {
                    free(P.message);
                }
                P.message = strdup(chunk.memory);
            } else {
                tms_errorf("could not check for lateset version: invalid data");
            }
        } else {
            tms_errorf("could not check for latest version: %s", curl_easy_strerror(r));
         }
    } else {
        tms_errorf("unable to initialize curl handle!");
    }
    unlock_curl("check_version_code");

    if (_tms.state == TMS_STATE_QUITTING) {
        return 0;
    }

    P.add_action(ACTION_REFRESH_HEADER_DATA, 0);

    tms_debugf("exiting version check thread");
    return 0;
}

char *featured_levels_buf = 0;
size_t featured_levels_buf_size = 0;

int
_get_featured_levels(void *_num)
{
    uint32_t num_featured_levels = VOID_TO_UINT32(_num);
    CURLcode r;

    char featured_data_path[1024];
    snprintf(featured_data_path, 1023, "%s/fl.cache", tbackend_get_storage_path());

    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;

    featured_levels_buf = 0;
    featured_levels_buf_size = 0;

    lock_curl("get_featured_levels");
    if (P.curl) {
        init_curl_defaults(P.curl);

        char url[256];
        snprintf(url, 255, "https://%s/internal/get_featured?num=%u", P.community_host, num_featured_levels);

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
                featured_levels_buf = chunk.memory;
                featured_levels_buf_size = chunk.size;
            }
        } else {
            tms_errorf("curl error: %s", curl_easy_strerror(r));
        }
    } else {
        tms_errorf("unable to initialize curl handle!");
    }

    unlock_curl("get_featured_levels");

    if (_tms.state == TMS_STATE_QUITTING) {
        return 0;
    }

    if (!featured_levels_buf) {
        FILE *fh = fopen(featured_data_path, "rb");
        if (fh) {
            fseek(fh, 0, SEEK_END);
            featured_levels_buf_size = ftell(fh);
            fseek(fh, 0, SEEK_SET);

            featured_levels_buf = (char*)malloc(featured_levels_buf_size);
            fread(featured_levels_buf, 1, featured_levels_buf_size, fh);

            fclose(fh);
        } else {
            tms_infof("Error opening cache file!");
            return 0;
        }
    }

    /* Second pass! */
    if (!featured_levels_buf) {
        return 0;
    }

    lvlbuf lb;

    lb.reset();
    lb.size = 0;
    lb.ensure(featured_levels_buf_size);

    lb.buf = (uint8_t*)featured_levels_buf;
    lb.size = featured_levels_buf_size;

    tms_debugf("featured data size: %d", (int)featured_levels_buf_size);

    uint32_t count = lb.r_uint32();

    uint32_t n = std::min(count, num_featured_levels);
    for (uint32_t i=0; i<n; ++i) {
        menu_shared::fl[i].id = lb.r_uint32();

        uint32_t name_len = lb.r_uint32();
        char *name = (char*)calloc(name_len+1, 1);
        lb.r_buf(name, name_len);
        strcpy(menu_shared::fl[i].name, name);
        free(name);

        uint32_t creator_len = lb.r_uint32();
        char *creator = (char*)calloc(creator_len+1, 1);
        lb.r_buf(creator, creator_len);
        strcpy(menu_shared::fl[i].creator, creator);
        free(creator);

        uint32_t thumb_len = lb.r_uint32();

        if (thumb_len == 0) {
            tms_errorf("Featured level received with no thumbnail!");
            continue;
        }
        char *thumb = (char*)malloc(thumb_len);
        lb.r_buf(thumb, thumb_len);

        tms::texture *tex = new tms::texture();
        tex->load_mem2(thumb, thumb_len, 0);
        tex->flip_y();
        tex->add_alpha(1.f);

        menu_shared::fl[i].sprite = tms_atlas_add_bitmap(
                gui_spritesheet::atlas,
                tex->width,
                tex->height,
                tex->num_channels,
                tex->data
                );

        free(thumb);
        tex->free_buffer();

        delete tex;
    }

    tms_infof("loading contest stuff..");
    uint32_t contest_active = lb.r_uint32();

    if (contest_active == 1) {
        menu_shared::contest.id = lb.r_uint32();

        uint32_t contest_name_len = lb.r_uint32();
        char *contest_name = (char*)calloc(contest_name_len+1, 1);
        lb.r_buf(contest_name, contest_name_len);

        uint32_t contest_thumb_len = lb.r_uint32();
        if (contest_thumb_len > 0) {
            char *contest_thumb = (char*)calloc(contest_thumb_len, 1);
            lb.r_buf(contest_thumb, contest_thumb_len);

            tms::texture *tex = new tms::texture();
            tex->load_mem2(contest_thumb, contest_thumb_len, 0);
            tex->flip_y();
            tex->add_alpha(1.f);

            menu_shared::contest.sprite = tms_atlas_add_bitmap(
                    gui_spritesheet::atlas,
                    tex->width,
                    tex->height,
                    tex->num_channels,
                    tex->data
                    );

            strcpy(menu_shared::contest.name, contest_name);

            free(contest_name);
            free(contest_thumb);

            tex->free_buffer();

            delete tex;

            uint32_t num_entries = lb.r_uint32();

            n = std::min(num_entries, (uint32_t)MAX_FEATURED_LEVELS_FETCHED);
            for (uint32_t i=0; i<n; ++i) {
                menu_shared::contest_entries[i].id = lb.r_uint32();

                uint32_t thumb_len = lb.r_uint32();

                if (thumb_len == 0) {
                    tms_errorf("We received a contest entry which did not have a thumbnail!");
                    continue;
                }

                char *thumb = (char*)calloc(thumb_len, 1);
                lb.r_buf(thumb, thumb_len);

                tex = new tms::texture();
                tex->load_mem2(thumb, thumb_len, 0);
                tex->flip_y();
                tex->add_alpha(1.f);

                menu_shared::contest_entries[i].sprite = tms_atlas_add_bitmap(
                        gui_spritesheet::atlas,
                        tex->width,
                        tex->height,
                        tex->num_channels,
                        tex->data
                        );

                free(thumb);
                tex->free_buffer();

                delete tex;
            }

            menu_shared::contest_state = FL_WAITING;
        } else {
            tms_errorf("Contest has no thumbnail :(");
        }
    }

    uint32_t num_getting_started_links = lb.r_uint32();

    tms_infof("Num getting started links: %u", num_getting_started_links);

    menu_shared::gs_entries.clear();

    for (uint32_t x=0; x<num_getting_started_links; ++x) {
        uint32_t title_len, link_len;
        char    *title,    *link;

        title_len = lb.r_uint32();
        title = (char*)calloc(title_len+1, 1);
        lb.r_buf(title, title_len);

        link_len = lb.r_uint32();
        link = (char*)calloc(link_len+1, 1);
        lb.r_buf(link, link_len);

        menu_shared::gs_entries.push_back(gs_entry(strdup(title), strdup(link)));

        free(title);
        free(link);
    }

    if (num_getting_started_links) {
        menu_shared::gs_state = FL_WAITING;
    }

    {
        FILE *fh = fopen(featured_data_path, "wb");

        if (fh) {
            fwrite(featured_levels_buf, 1, featured_levels_buf_size, fh);

            fclose(fh);
        }
    }

    menu_shared::fl_state = FL_UPLOAD;

    return 0;
}

#ifdef BUILD_PKGMGR

int
_publish_pkg(void *_unused)
{
    uint32_t pkg_id = _publish_pkg_id;

    pkginfo p;
    if (p.open(LEVEL_LOCAL, pkg_id)) {

        if (p.num_levels) {
            uint32_t *community_ids = (uint32_t*)malloc(p.num_levels*sizeof(uint32_t));
            bool error = false;

            for (int x=0; x<p.num_levels; x++) {
                _publish_lvl_id = p.levels[x];
                _publish_lvl_with_pkg = true;
                _publish_lvl_pkg_index = x;
                _publish_lvl_set_locked = x >= p.unlock_count;

                _publish_level(0);

                if (_publish_lvl_uploading_error || _publish_lvl_community_id == 0) {
                    error = true;
                    break;
                }

                community_ids[x] = _publish_lvl_community_id;
            }

            if (!error) {
                /* ok, all levels are uploaded. now upload the package itself. */
                CURLcode r;

                char level_list[4096];
                char tmp[20];
                level_list[0] = '\0';
                for (int x=0; x<p.num_levels; x++) {
                    sprintf(tmp, "%u,", community_ids[x]);
                    strcat(level_list, tmp);
                }
                level_list[strlen(level_list)-1] ='\0';

                struct MemoryStruct chunk;
                chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
                chunk.size = 0;    /* no data at this point */

                lock_curl("publish_pkg");
                if (P.curl) {
                    init_curl_defaults(P.curl);

                    curl_mime *mime = curl_mime_init(P.curl);
                    curl_mimepart *part = NULL;

                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "xFxlax");
                    curl_mime_data(part, "zPaod", CURL_ZERO_TERMINATED);

                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "name");
                    curl_mime_data(part, p.name, CURL_ZERO_TERMINATED);

                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "levels");
                    curl_mime_data(part, level_list, CURL_ZERO_TERMINATED);

                    sprintf(tmp, "%d", p.unlock_count);
                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "unlock_count");
                    curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);

                    sprintf(tmp, "%u", (uint32_t)p.first_is_menu);
                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "first_is_menu");
                    curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);

                    sprintf(tmp, "%u", (uint32_t)p.return_on_finish);
                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "return_on_finish");
                    curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);

                    sprintf(tmp, "%u", (uint32_t)p.version);
                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "version");
                    curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);

                    sprintf(tmp, "%u", (uint32_t)p.community_id);
                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "id");
                    curl_mime_data(part, tmp, CURL_ZERO_TERMINATED);


                    COMMUNITY_URL("internal/upload_package");
                    curl_easy_setopt(P.curl, CURLOPT_URL, url);

                    curl_easy_setopt(P.curl, CURLOPT_MIMEPOST, mime);

                    curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
                    curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, (void*)&chunk);
                    curl_easy_setopt(P.curl, CURLOPT_CONNECTTIMEOUT, 15L);

                    tms_debugf("Publishing package..");
                    if ((r = curl_easy_perform(P.curl)) != CURLE_OK) {
                        tms_errorf("pkg publish curl_easy_perform failed: %s\n", curl_easy_strerror(r));
                        unlock_curl("publish_pkg");
                        _publish_pkg_done = true;
                        _publish_pkg_error = true;
                        return 1;
                    }

                    if (chunk.size != 0) {
                        char *pch;

                        pch = strchr(chunk.memory, '-');

                        if ((pch-chunk.memory) == 0) {
                            int notify_id = atoi(chunk.memory);
                            _publish_pkg_error = true;

                            switch (notify_id) {
                                default:
                                    ui::message("Unknown error message");
                                    break;
                            }
                        } else {
                            p.community_id = atoi(chunk.memory);
                            tms_debugf("saved pkg community id: %u", p.community_id);
                            p.save();
                        }
                    } else {
                        /* we did not recieve any data back, an unknown error occured */
                        tms_errorf("no data received");
                        _publish_pkg_error = true;
                    }

                    curl_mime_free(mime);
                } else {
                    tms_errorf("lock_curl failed :3");
                    _publish_pkg_error = true;
                }
                unlock_curl("publish_pkg");
            } else {
                //ui::message("An error occurred while uploading levels in package.");
                tms_errorf("An error occured while uploading the levels contained in the package.");
                _publish_pkg_error = true;
            }

            free(community_ids);
        } else {
            ui::message("Can not upload an empty package.");
        }
    } else {
        _publish_pkg_error = true;
    }

    _publish_pkg_done = true;

    return T_OK;
}
#endif

int
_publish_level(void *p)
{
    uint32_t level_id = _publish_lvl_id;
    int community_id    = 0;

    _publish_lvl_community_id = 0;
    _publish_lvl_uploading_error = false;

    CURLcode r;

    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

    lvledit lvl;

    if (!lvl.open(LEVEL_LOCAL, level_id)) {
        tms_errorf("could not open level");
        return false;
    }

    /* if we publish this as part of a package, update the locked
     * value according to the packages settings */

    /* TODO: use hidden for non-locked levels */
    if (_publish_lvl_with_pkg) {
        lvl.lvl.visibility = (_publish_lvl_set_locked ? LEVEL_LOCKED : LEVEL_VISIBLE);
    }

    tms_debugf("old revision: %d", lvl.lvl.revision);
    lvl.lvl.revision++;
    tms_debugf("new revision: %d", lvl.lvl.revision);
    lvl.save();

    char level_path[1024];
    pkgman::get_level_full_path(LEVEL_LOCAL, level_id, 0, level_path);

    lock_curl("publish_level");
    if (P.curl) {
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
        r = curl_easy_perform(P.curl);
        if (r == CURLE_OK) {
            // Check for messages
            if (hd.error_message) {
                ui::message(hd.error_message);

                _publish_lvl_uploading_error = true;

                free(hd.error_message);
            } else if (hd.notify_message) {
                tms_infof("got data: %s", hd.notify_message);
                community_id = atoi(hd.notify_message);

                W->level.revision = lvl.lvl.revision;
                lvl.lvl.community_id = community_id;
                tms_infof("community id: %d", community_id);
                tms_infof("parent id:    %u", lvl.lvl.parent_id);
                tms_infof("revision:     %u", lvl.lvl.revision);

                free(hd.notify_message);

            } else {
                /* we did not recieve any data back, an unknown error occured */
                tms_errorf("no data received");
                _publish_lvl_uploading_error = true;
            }
        } else {
            tms_errorf("lvl publish curl_easy_perform failed: %s\n", curl_easy_strerror(r));

            switch (r) {
                case CURLE_OPERATION_TIMEDOUT:
                    ui::message("Operation timed out. Your internet connection seems to be unstable! Please try again.", true);
                    break;

                case CURLE_COULDNT_RESOLVE_HOST:
                    ui::message("Error: Unable to resolve hostname. Please check your internet connection.", true);
                    break;

                default:
                    ui::message("An unknown error occured when publishing your level. Check your internet connection.", true);
                    break;
            }
            _publish_lvl_uploading = false;
            _publish_lvl_uploading_error = true;
        }

        curl_mime_free(mime);
    }
    unlock_curl("publish_level");

    P.add_action(ACTION_AUTOSAVE, 0);

    if (!lvl.save()) {
        tms_errorf("Unable to save the level after publish!");
    }

    _publish_lvl_community_id = community_id;
    _publish_lvl_uploading = false;

    return T_OK;
}

/* Submit score variables */
bool _submit_score_done = false;

int
_submit_score(void *p)
{
    tms_assertf(W->is_playing(), "submit score called when the level was paused");

    CURLcode r;

    char data_path[1024];

    const char *storage = tbackend_get_storage_path();
    snprintf(data_path, 1023, "%s/data.bin", storage);

    int highscore_level_offset = highscore_offset(W->level.community_id);

    lvledit lvl;

    if (!lvl.open(LEVEL_DB, W->level.local_id)) {
        tms_errorf("could not open level");
        return false;
    }

    uint32_t timestamp = (uint32_t)time(NULL);
    const lvl_progress *base_lp = progress::get_level_progress(W->level_id_type, W->level.local_id);
    uint32_t last_score = base_lp->last_score;

    uint32_t crc32_data[5];
    uint32_t more_data[5];
    more_data[HS_VER_DATA_TIMESTAMP] = timestamp;
    more_data[HS_VER_DATA_REVISION] = lvl.lvl.revision;
    more_data[HS_VER_DATA_TYPE] = W->level_id_type;
    more_data[HS_VER_DATA_PRINCIPIA_VERSION] = principia_version_code();
    more_data[HS_VER_DATA_VERSION] = W->level.version;

    for (int x=0; x<5; ++x) {
        tms_infof("fetching progress for level with id %d", BASE_HIGHSCORE_LEVEL_ID+x);
        lvl_progress *lp = progress::get_level_progress(LEVEL_MAIN, BASE_HIGHSCORE_LEVEL_ID+x);
        crc32_data[x] = crc32_level(lvl.lvl, lvl.lb, timestamp, last_score, x);
        tms_debugf("%d got crc: %08X", x, crc32_data[x]);

        lp->last_score = more_data[(x+highscore_level_offset)%5];

        tms_infof("Last score: %u", lp->last_score);
    }

    for (int x=0; x<5; ++x) {
        lvl_progress *lp = progress::get_level_progress(LEVEL_MAIN, BASE_HIGHSCORE_LEVEL_ID+x);
        lp->top_score = crc32_data[(x+highscore_level_offset)%5];
        tms_infof("Top score: %08X", lp->top_score);
    }

    progress::commit();

    /* TODO: add cool stuff, like crc32 stuff here */

    /* and when we're done with sbuuuuutmi score, we remove that secret stuff again */

    lock_curl("submit_score");
    if (P.curl) {
        struct header_data hd = {0};
        init_curl_defaults(P.curl);

        curl_mime* mime = curl_mime_init(P.curl);
        curl_mimepart* part;

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "data.bin");
        curl_mime_filedata(part, data_path);

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

        r = curl_easy_perform(P.curl);
        if (r == CURLE_OK) {
            // Check for messages
            bool displayed_message = false;
            if (hd.error_message) {
                if (!displayed_message) {
                    ui::message(hd.error_message);
                    displayed_message = true;
                }

                free(hd.error_message);
            }
            if (hd.notify_message) {
                if (!displayed_message) {
                    ui::message(hd.notify_message);
                    displayed_message = true;
                }

                free(hd.notify_message);

                G->state.submitted_score = true;
            }

            switch (hd.error_action) {
                case ERROR_ACTION_LOG_IN:
                    ui::next_action = ACTION_SUBMIT_SCORE;
                    ui::open_dialog(DIALOG_LOGIN);
                    break;
            }
        } else {
            switch (r) {
                case CURLE_OPERATION_TIMEDOUT:
                    ui::message("Unable to submit your score.\nYour internet connection seems to be unstable! Please try again.", true);
                    break;

                case CURLE_COULDNT_RESOLVE_HOST:
                    ui::message("Unable to submit your score.\nError: Unable to resolve hostname. Please check your internet connection.", true);
                    break;

                default:
                    ui::message("An unknown error occured when submitting your score. Check your internet connection and try again.", true);
                    break;
            }
        }

        curl_mime_free(mime);
    }

    _submit_score_done = true;

    unlock_curl("submit_score");

    return T_OK;
}

/** --Login **/
int
_login(void *p)
{
    struct login_data *data = static_cast<struct login_data*>(p);

    int res = T_OK;

    CURLcode r;

    lock_curl("login");
    if (P.curl) {
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

        r = curl_easy_perform(P.curl);
        if (r == CURLE_OK) {
            // Check for messages
            if (hd.error_message) {
                ui::message(hd.error_message);
                ui::emit_signal(SIGNAL_LOGIN_FAILED);

                free(hd.error_message);
            }

            if (hd.notify_message) {
                ui::message(hd.notify_message);

                P.username = strdup(data->username);
                P.add_action(ACTION_REFRESH_HEADER_DATA, 0);

                ui::emit_signal(SIGNAL_LOGIN_SUCCESS);

                free(hd.notify_message);
            }
        } else {
            tms_errorf("curl_easy_perform failed: %s\n", curl_easy_strerror(r));
            res = 1;
        }
        curl_mime_free(mime);
    } else {
        tms_errorf("Unable to initialize curl handle.");
        res = 1;
    }
    unlock_curl("login");

    free(data);

    return res;
}

/** --Register **/
int
_register(void *p)
{
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

int
_download_pkg(void *_p)
{
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
            _download_level(0);

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
int
_download_level(void *p)
{
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
        tms_debugf("An error occured while downloading the level.");
        _play_id = old_id;
    }

    _play_downloading = false;

    return T_OK;
}

/**
 * Get the community site login token from cURL, intended for the user to be automatically
 * logged into the Android webview.
*/
extern "C" void
P_get_cookie_data(char **token)
{
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

#else

void init_curl() { }
void soft_resume_curl() { }
void soft_pause_curl() { }
void quit_curl() { }
int _check_version_code(void *_unused) { return 0; }
int _get_featured_levels(void *_unused) { return 0; }
int _publish_pkg(void *_unused) { return 0; }
int _publish_level(void *_unused) { return 0; }
int _submit_score(void *_unused) { return 0; }
int _login(void *_unused) { return 0; }
int _register(void *_unused) { return 0; }
int _download_pkg(void *_unused) { return 0; }
int _download_level(void *_unused) { return 0; }

#endif
