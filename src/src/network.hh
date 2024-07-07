#pragma once

#include <cstdint>

struct header_data {
    char *error_message;
    char *notify_message;
    int error_action;
};

/**
 * Global cURL initialisation
 */
void init_curl();

void soft_resume_curl();

void soft_pause_curl();

void quit_curl();

int _check_version_code(void *_unused);
int _get_featured_levels(void *_unused);

#ifdef BUILD_PKGMGR

/* Publish PKG variables */
extern uint8_t       _publish_lvl_pkg_index = 0;
extern uint32_t      _publish_pkg_id;
extern volatile bool _publish_pkg_done = false;
extern bool          _publish_pkg_error = false;

int _publish_pkg(void *p);

#endif

/* Publish level variables */
extern uint32_t      _publish_lvl_community_id;
extern uint32_t      _publish_lvl_id;
extern bool          _publish_lvl_with_pkg;
extern bool          _publish_lvl_set_locked;
extern bool          _publish_lvl_lock;
extern volatile bool _publish_lvl_uploading;
extern bool          _publish_lvl_uploading_error;

int _publish_level(void *p);

/* Submit score variables */
extern bool         _submit_score_done;

int _submit_score(void *p);

int _login(void *p);
int _register(void *p);

/* Download pkg variables */
extern uint32_t _play_pkg_id;
extern uint32_t _play_pkg_type;
extern uint32_t _play_pkg_downloading;
extern uint32_t _play_pkg_downloading_error;

int _download_pkg(void *p);

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

int _download_level(void *p);


#ifdef __cplusplus
extern "C" {
#endif
void P_get_cookie_data(char **token);
#ifdef __cplusplus
}
#endif
