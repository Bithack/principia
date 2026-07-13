#include "network.hh"
#include <tms/cpp.hh>

/* Publish level variables */
uint32_t      _publish_lvl_community_id;
uint32_t      _publish_lvl_id;
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

/* Submit score variables */
bool _submit_score_done = false;
