#include "network.hh"

#ifdef SCREENSHOT_BUILD

void network::init() {}
void network::soft_resume() {}
void network::soft_pause() {}
void network::quit() {}

int network::check_version_code(void *p) { return 0; }
int network::get_featured_levels(void *p) { return 0; }
int network::publish_level(void *p) { return 0; }
int network::submit_score(void *p) { return 0; }
int network::login(void *p) { return 0; }
int network::register_user(void *p) { return 0; }
int network::download_pkg(void *p) { return 0; }
int network::download_level(void *p) { return 0; }

#endif
