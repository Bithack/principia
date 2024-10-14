#pragma once

#include <unordered_map>
#if !defined(GTK3_LEVEL_BROWSER_DISABLE) && defined(TMS_BACKEND_PC) && !defined(NO_UI) && !defined(TMS_BACKEND_EMSCRIPTEN)

#include <string>
#include <cstdint>
// #include "optional.hh"

#define COMMUNITY_LEVELS_PER_PAGE 16

class CommunityUser {
    uint32_t id;
    std::string name;
    std::string customcolor; // TODO store as u32 instead
};

class CommunityRecentLevel {
    uint32_t id;
    std::string title;
    CommunityUser u;
};

// {"id":1,"cat":1,"title":"not so smiley man","description":"finally\r\n\r\nnot so smiley man","author":1,"time":1608998715,"parent":null,"revision":1,"revision_time":null,"likes":21,"visibility":0,"views":433,"downloads":484,"platform":"Linux","u_id":1,"u_name":"ROllerozxa","u_customcolor":"31C03B"}

class CommunityLevel {
    uint32_t id;

    uint8_t cat;

    std::string title;
    std::string description;

    uint32_t author;

    uint32_t time;
    uint32_t parent;

    uint32_t revision;
    uint32_t revision_time;

    uint8_t visibility;

    uint32_t views;
    uint32_t likes;
    uint32_t downloads;

    std::string platform;

    CommunityUser u;
};

class GtkCommunityState {
    uint16_t cur_page;
    // std::unordered_map<uint32_t, uint8_t*> cache_thumbnails;
    // std::unordered_map<uint16_t, uint8_t*> cache_pages;
};

class GtkCommunityDialog {
    GtkCommunityDialog();
    ~GtkCommunityDialog();
};

#endif
