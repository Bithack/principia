#pragma once

#include <cstdint>
#include <SDL3/SDL.h>

struct header_data {
    char *error_message;
    char *notify_message;
    int error_action;
};

class network {
public:
    static void init();
    static void soft_resume();
    static void soft_pause();
    static void quit();

    static int check_version_code(void *p);
    static int get_featured_levels(void *p);
    static int publish_level(void *p);
    static int submit_score(void *p);
    static int login(void *p);
    static int register_user(void *p);
    static int download_pkg(void *p);
    static int download_level(void *p);
};

/* Publish level variables */
extern uint32_t      _publish_lvl_community_id;
extern uint32_t      _publish_lvl_id;
extern volatile bool _publish_lvl_uploading;
extern bool          _publish_lvl_uploading_error;

/* Submit score variables */
extern bool         _submit_score_done;

/* Download pkg variables */
extern uint32_t _play_pkg_id;
extern uint32_t _play_pkg_type;
extern uint32_t _play_pkg_downloading;
extern uint32_t _play_pkg_downloading_error;

enum {
    DOWNLOAD_GENERIC_ERROR              = 1,
    DOWNLOAD_WRITE_ERROR                = 2,
    DOWNLOAD_CHECK_INTERNET_CONNECTION  = 3,
};

/* Download level variables */
extern uint32_t      _play_id;
extern char          _community_host[512]; /* Temporary input host from principia:// url, not to be confused with P.community_host */
extern uint32_t      _play_type;
extern bool          _play_lock;
extern volatile bool _play_downloading;
extern volatile bool _play_download_for_pkg;
extern volatile int  _play_downloading_error;
extern struct header_data _play_header_data;

#ifdef SDL_PLATFORM_ANDROID
extern "C" {
    void P_get_cookie_data(char **token);
}
#endif
