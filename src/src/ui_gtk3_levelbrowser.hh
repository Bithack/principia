#pragma once

#include <vector>
#if !defined(GTK3_LEVEL_BROWSER_DISABLE) && defined(TMS_BACKEND_PC) && !defined(TMS_BACKEND_EMSCRIPTEN) && !defined(NO_UI) && defined(BUILD_CURL)
    #define GTK3_LEVEL_BROWSER_ENABLE
#endif

#ifdef GTK3_LEVEL_BROWSER_ENABLE

#include <memory>
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <glib.h>
#include <gtk/gtk.h>

using json = nlohmann::json;

#define COMMUNITY_LEVELS_PER_PAGE 16

namespace api {
    struct user {
        uint32_t id;
        std::string name;
        std::unique_ptr<std::string> customcolor; // TODO store as u32 instead

        user(const json &j);
        // user(uint32_t id, std::string name);
    };

    struct recent_level {
        uint32_t id;
        std::string title;
        struct user u;

        recent_level(const json &j);
        // recent_level(uint32_t id, const std::string &title, const struct user &u);
        // recent_level(const struct level &l);
    };

    struct level {
        uint32_t id;
        uint8_t cat;
        std::string title;
        std::string description;
        uint32_t author;
        uint32_t time;
        std::unique_ptr<uint32_t> parent;
        uint32_t revision;
        std::unique_ptr<uint32_t> revision_time;
        uint32_t likes;
        uint8_t visibility;
        uint32_t views;
        uint32_t downloads;
        std::string platform;
        struct user u;

        level(const json &j);
    };

    static std::vector<recent_level> get_recent_levels(uint32_t offset, uint32_t limit);
    static level get_level(uint32_t id);
    static std::vector<uint8_t> get_level_thumbnail(uint32_t id, bool use_global_curl = true);
};

namespace gtk_community {
    static GtkWidget *global_dialog;

    static void on_level_clicked(GtkWidget *widget, gpointer data);
    static void on_username_clicked(GtkWidget *widget, gpointer data);
    static void on_search_activate(GtkWidget *widget, gpointer data);

    static GtkWidget *create_level_tile(const api::recent_level &level);
    static GtkWidget *create_level_grid(const std::vector<api::recent_level> &levels);
    static GtkWidget *create_top_shelf_content();
    static GtkWidget *create_shelf(std::string name, GtkWidget *content);
    static GtkWidget *create_dialog(const std::vector<api::recent_level> &levels);
}

// DO NOT USE THIS FUNCTION DIRECTLY
// Use ui::open_dialog(DIALOG_HC_LEVEL_BROWSER) instead!
//
// (If calling from ui::open_dialog, make sure to wrap it in gdk_threads_add_idle!)
gboolean open_community_level_browser(gpointer _);

// Call in ui::init() after gtk_init()
void init_community_level_browser();

// class gtk_community_state {
//     uint16_t cur_page;
//     // std::unordered_map<uint32_t, uint8_t*> cache_thumbnails;
//     // std::unordered_map<uint16_t, uint8_t*> cache_pages;
// };

// class gtk_community_dialog {
//     gtk_community_dialog();
//     ~gtk_community_dialog();
// };

#endif
