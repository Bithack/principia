#include "ui_gtk3_levelbrowser.hh"
#include <string>


#ifdef GTK3_LEVEL_BROWSER_ENABLE

#include "main.hh"
#include "tms/backend/print.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <SDL_mutex.h>
#include <glib.h>
#include <gtk/gtk.h>

namespace api {
    using json = nlohmann::json;

    template <typename T>
    std::unique_ptr<T> get_nullable(const nlohmann::json &j, const std::string &field_name) {
        if (j.contains(field_name) && !j.at(field_name).is_null()) {
            return std::make_unique<T>(j.at(field_name).get<T>());
        }
        return nullptr;
    }

    user::user(const json &j) {
        id = j["u_id"];
        name = j.at("u_name").get<std::string>();
        customcolor = get_nullable<std::string>(j, "u_customcolor");
    }

    recent_level::recent_level(const json &j): u(j) {
        id = j["id"];
        title = j["title"].get<std::string>();
    }

    // recent_level::recent_level(const struct level l): u(l.u) {
    //     id = l.id;
    //     title = l.title;
    // }

    level::level(const json &j): u(j) {
        id = j["id"];
        cat = j["cat"];
        title = j["title"].get<std::string>();
        description = j["description"].get<std::string>();
        author = j["author"];
        time = j["time"];
        parent = get_nullable<uint32_t>(j, "parent");
        revision = j["revision"];
        revision_time = get_nullable<uint32_t>(j, "revision_time");
        visibility = j["visibility"];
        views = j["views"];
        likes = j["likes"];
        downloads = j["downloads"];
        platform = j["platform"].get<std::string>();
    }

    // TODO move to network.cc?

    // TODO non-blocking request

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    static std::vector<struct recent_level> get_recent_levels(uint32_t offset, uint32_t limit) {
        if (!P.curl) tms_fatalf("curl not initialized");
        SDL_LockMutex(P.curl_mutex);

        std::vector<api::recent_level> levels;

        char url[256];
        snprintf(url, 255, "https://%s/api/latest_levels?offset=%u&limit=%u", P.community_host, offset, limit);

        curl_easy_setopt(P.curl, CURLOPT_URL, url);

        std::string response;
        curl_easy_setopt(P.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(P.curl, CURLOPT_WRITEDATA, &response);

        // TODO handle error
        CURLcode res = curl_easy_perform(P.curl);
        if (res != CURLE_OK) tms_fatalf("[fuck] curl error");

        // TODO handle error
        long http_code = 0;
        curl_easy_getinfo(P.curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200) tms_fatalf("[fuck] HTTP error %ld", http_code);

        SDL_UnlockMutex(P.curl_mutex);

        json data = json::parse(response);

        // Data contains an array of api::recent_level

        for (const auto& item : data) {
            levels.emplace_back(item);
        }

        return levels;
    }

    // TODO implement this
    static struct level get_level(uint32_t id) {
        tms_fatalf("not implemented"); exit(1);
    }
}

namespace gtk_community {
    // Callback function for clicking on the level title
    static void on_level_clicked(GtkWidget *widget, gpointer data) {
        g_print("Level clicked: %s\n", (char *)data);
    }

    // Callback function for clicking on the username
    static void on_username_clicked(GtkWidget *widget, gpointer data) {
        g_print("Username clicked: %s\n", (char *)data);
    }

    static GtkWidget* create_level_tile(const api::recent_level &level) {
        // TODO this is placeholder
        const char *title = level.title.c_str();
        const char *username = level.u.name.c_str();
        // Create a box to hold the tile elements vertically
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

        // Placeholder for the level image (just a blank box for now)
        GtkWidget *image = gtk_drawing_area_new();
        gtk_widget_set_size_request(image, 100, 100); // Placeholder size
        gtk_widget_set_name(image, "level-image");

        // Level title (clickable button)
        GtkWidget *level_button = gtk_button_new_with_label(title);
        g_signal_connect(level_button, "clicked", G_CALLBACK(on_level_clicked), (gpointer)title);

        // Username (clickable label)
        // TODO set user color
        GtkWidget *username_button = gtk_button_new_with_label(username);
        gpointer user_id = (gpointer)(size_t)level.u.id;
        g_signal_connect(username_button, "clicked", G_CALLBACK(on_username_clicked), (gpointer)user_id);

        // Pack elements into the box
        gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), level_button, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), username_button, FALSE, FALSE, 0);

        return box;
    }

    static GtkWidget* create_dialog(const std::vector<api::recent_level> &levels) {
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Community Levels",
            NULL,
            (GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL),
            ("_Close"),
            GTK_RESPONSE_CLOSE,
            NULL
        );
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);

        // Get the content area of the dialog
        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

        // Create a grid to hold the level tiles
        GtkWidget *grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
        gtk_container_add(GTK_CONTAINER(content_area), grid);

        // Add level tiles to the grid
        int num_levels = 6;
        int rows = 2;  // 2 rows for now
        int cols = 3;  // 3 columns for now

        for (int i = 0; i < num_levels; i++) {
            GtkWidget *level_tile = create_level_tile(levels[i]);

            // Attach each tile in the grid
            int row = i / cols;
            int col = i % cols;
            gtk_grid_attach(GTK_GRID(grid), level_tile, col, row, 1, 1);
        }

        return dialog;
    }
}

gboolean open_community_level_browser(gpointer _) {
    tms_infof("====== Open level browser ======");

    tms_infof("Fetching recent levels...");
    std::vector<api::recent_level> levels = api::get_recent_levels(0, 6);

    tms_infof("Creating dialog...");
    GtkWidget *dialog = gtk_community::create_dialog(levels);
    gtk_widget_show_all(dialog);

    return false;
}

#endif

