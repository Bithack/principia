#include "ui_gtk3_levelbrowser.hh"

#ifdef GTK3_LEVEL_BROWSER_ENABLE

#include "main.hh"
#include "const.hh"
#include "network.hh"
#include "ui.hh"
#include "tms/backend/print.h"

#include <string>
#include <vector>
#include <curl/curl.h>
#include <curl/easy.h>
#include <nlohmann/json.hpp>
#include <SDL_mutex.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "gio/gio.h"
#include "glib-object.h"
#include "pango/pango-layout.h"
// #include <thread>

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

    static size_t WriteCallbackBinary(void *contents, size_t size, size_t nmemb, void *userp) {
        std::vector<uint8_t> *vec = (std::vector<uint8_t>*)userp;
        vec->insert(vec->end(), (uint8_t*)contents, (uint8_t*)contents + size * nmemb);
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

    static std::vector<uint8_t> get_level_thumbnail(uint32_t id, bool use_global_curl /* = true */) {
        CURL* curl;

        if (use_global_curl) {
            // TODO instad of this use the multi API
            curl = curl_easy_init();
        } else {
            if (!P.curl) tms_fatalf("curl not initialized");
            SDL_LockMutex(P.curl_mutex);
            curl = P.curl;
        }

        char url[256];
        snprintf(url, 255, "https://%s/thumbs/low/%u.jpg", P.community_host, id);

        curl_easy_setopt(curl, CURLOPT_URL, url);

        std::vector<uint8_t> response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackBinary);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // TODO handle error
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) tms_fatalf("[fuck] curl error");

        // TODO handle error
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200) tms_fatalf("[fuck] HTTP error %ld", http_code);

        // Sleep to simulate slow connection
        // std::this_thread::sleep_for(std::chrono::seconds(5));

        if (use_global_curl) {
            curl_easy_cleanup(curl);
        } else {
            SDL_UnlockMutex(P.curl_mutex);
        }

        return response;
    }
}

void _api_get_level_thumbnail_async(uint32_t id, GAsyncReadyCallback callback, gpointer user_data) {
    GTask *task = g_task_new(NULL, NULL, callback, user_data);
    g_task_set_source_tag(task, (gpointer)_api_get_level_thumbnail_async);
    g_task_set_task_data(task, (gpointer)(size_t)id, NULL);

    g_task_run_in_thread(task, [](GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) {
        uint32_t id = (size_t)task_data;
        std::vector<uint8_t> thumbnail = api::get_level_thumbnail(id, false);
        std::vector<uint8_t> *thumbnail_heap = new std::vector<uint8_t>(thumbnail);
        tms_infof("Async thumbnail loaded %p", thumbnail_heap);
        g_task_return_pointer(task, thumbnail_heap, free);
    });
    // g_task_return_pointer(task, NULL, g_free);
}

namespace gtk_community {
    // Callback function for clicking on the level title
    static void on_level_clicked(GtkWidget *widget, gpointer data) {
        uint32_t level_id = (size_t)data;
        g_print("Level clicked: %d\n", level_id);
        _play_id = level_id;
        _play_type = LEVEL_DB;
        P.add_action(ACTION_OPEN_PLAY, 0);

        gtk_widget_destroy(global_dialog);
     }

    // Callback function for clicking on the username
    static void on_username_clicked(GtkWidget *widget, gpointer data) {
        uint32_t user_id = (size_t)data;
        g_print("Level clicked: %d\n", user_id);
        {
            COMMUNITY_URL("user/%d", user_id);
            ui::open_url(url);
        };
    }

    // Callback function for activating the search entry
    static void on_search_activate(GtkWidget *widget, gpointer data) {
        const char *text = gtk_entry_get_text(GTK_ENTRY(widget));
        g_print("Search activated: %s\n", text);
        // Check if id is entered
        // Valid syntax:
        // number or "id:number": open level by ID
        // otherwise, use ui::open_url to search on the website

        bool should_play = false;
        uint32_t play_db_id;

        // Case 1: text *only* contains ASCII digits
        bool is_number = true;
        for (int i = 0; text[i] != '\0'; i++) {
            if (text[i] < '0' || text[i] > '9') {
                is_number = false;
                break;
            }
        }
        if (is_number) {
            uint32_t level_id = atoi(text);
            g_print("Level ID: %d\n", level_id);
            should_play = true;
            play_db_id = level_id;
        }

        // Case 2: starts with id:
        if (text[0] == 'i' && text[1] == 'd' && text[2] == ':') {
            uint32_t level_id = atoi(text + 3);
            g_print("Level ID: %d\n", level_id);
            should_play = true;
            play_db_id = level_id;
        }

        if (should_play) {
            _play_id = play_db_id;
            _play_type = LEVEL_DB;
            P.add_action(ACTION_OPEN_PLAY, 0);

            gtk_widget_destroy(global_dialog);
            return;
        }

        // Case 3: search on the website
        {
            COMMUNITY_URL("search?query=%s", text);
            ui::open_url(url);
        }
    }

    static GtkWidget* create_level_tile(const api::recent_level &level) {
        // TODO this is placeholder
        const char *title = level.title.c_str();
        const char *username = level.u.name.c_str();
        gpointer user_id = (gpointer)(size_t)level.u.id;
        gpointer level_id = (gpointer)(size_t)level.id;

        // Create a flow child (activatable)
        // GtkWidget *child = gtk_flow_box_child_new();
        // g_signal_connect(child, "activate", G_CALLBACK(on_level_clicked), (gpointer)level_id);
        // g_signal_connect(child, "clicked", G_CALLBACK(on_level_clicked), (gpointer)level_id);

        // Create a box to hold the tile elements vertically
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(box));
        gtk_style_context_add_class(context, "hc-level-tile");

        // gtk_container_add(GTK_CONTAINER(child), box);

        // Placeholder for the level image (just a blank box for now)
        GtkWidget *image = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_size_request(image, 240, 135); // Placeholder size
        // Kick off async thumbnail fetch
        GAsyncReadyCallback callback = [](GObject *source_object, GAsyncResult *res, gpointer user_data) {
            // XXX: some of this may leak memory :<

            GTask *task = G_TASK(res);
            std::vector<uint8_t> *result = (std::vector<uint8_t>*)g_task_propagate_pointer(task, NULL);
            tms_infof("Thumbnail loaded %p", result);
            GtkWidget *image = GTK_WIDGET(user_data);

            tms_debugf("Image data size: %zu", result->size());

            GdkPixbufLoader *loader = gdk_pixbuf_loader_new_with_type("jpeg", NULL);
            gdk_pixbuf_loader_write(loader, result->data(), result->size(), NULL);
            gdk_pixbuf_loader_close(loader, NULL);
            GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);

            tms_debugf("pixbuf pointer: %p", pixbuf);
            tms_debugf("pixbuf image size: %d x %d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));

            GtkWidget *image_widget = gtk_image_new_from_pixbuf(pixbuf);
            gtk_container_add(GTK_CONTAINER(image), image_widget);
            gtk_widget_show_all(image);

            g_object_unref(loader);
            delete result;
        };
        _api_get_level_thumbnail_async(level.id, callback, image);

        // Level title (clickable button)
        GtkWidget *level_label = gtk_label_new(title);
        gtk_label_set_xalign(GTK_LABEL(level_label), 0.5);
        // gtk_label_set_lines(GTK_LABEL(level_label), 2);
        gtk_label_set_ellipsize(GTK_LABEL(level_label), PANGO_ELLIPSIZE_END);
        gtk_label_set_width_chars(GTK_LABEL(level_label), 20);
        gtk_label_set_max_width_chars(GTK_LABEL(level_label), 20);

        GtkWidget *level_button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(level_button), GTK_RELIEF_NONE);
        gtk_container_add(GTK_CONTAINER(level_button), level_label);
        g_signal_connect(level_button, "clicked", G_CALLBACK(on_level_clicked), (gpointer)level_id);

        // Username (clickable label)
        // TODO set user color
        GtkWidget *username_label = gtk_label_new(username);
        gtk_label_set_xalign(GTK_LABEL(username_label), 0.5);
        gtk_label_set_lines(GTK_LABEL(username_label), 1);
        gtk_label_set_ellipsize(GTK_LABEL(username_label), PANGO_ELLIPSIZE_END);
        gtk_label_set_width_chars(GTK_LABEL(username_label), 20);
        gtk_label_set_max_width_chars(GTK_LABEL(username_label), 20);
        // HACK: override color
        const static GdkRGBA rgba_color = {
            .red = 109. / 255. ,
            .green = 160. / 255.,
            .blue = 253 / 255.,
            .alpha = 1.0,
        };
        gtk_widget_override_color(username_label, GTK_STATE_FLAG_NORMAL, &rgba_color);

        GtkWidget *username_button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(username_button), GTK_RELIEF_NONE);
        gtk_container_add(GTK_CONTAINER(username_button), username_label);
        g_signal_connect(username_button, "clicked", G_CALLBACK(on_username_clicked), (gpointer)user_id);

        // Pack elements into the box
        gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), level_button, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), username_button, FALSE, FALSE, 0);

        return box;
    }

    static GtkWidget *create_level_grid(const std::vector<api::recent_level> &levels) {
        // Create a grid to hold the level tiles
        GtkWidget* flow_box = gtk_flow_box_new();
        gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flow_box), 6);
        gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flow_box), 4);
        gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow_box), GTK_SELECTION_NONE);
        // gtk_flow_box_unselect_all(GTK_FLOW_BOX(flow_box));
        // gtk_flow_box_set_activate_on_single_click(GTK_FLOW_BOX(flow_box), true);
        // gtk_container_add(GTK_CONTAINER(content_area), flow_box);

        // Add level tiles to the grid
        for (auto &level : levels) {
            // Create level tile
            GtkWidget *level_tile = create_level_tile(level);

            // Attach each tile in the grid
            gtk_container_add(GTK_CONTAINER(flow_box), level_tile);
        }

        return flow_box;
    }

    static GtkWidget* create_shelf(std::string name, GtkWidget *content) {
        // Create a box to hold the shelf elements vertically
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

        // Shelf header
        GtkWidget *header = gtk_label_new(name.c_str());
        // HACK: override font size
        PangoFontDescription *font_desc = pango_font_description_new();
        pango_font_description_set_size(font_desc, 24 * PANGO_SCALE);
        gtk_widget_override_font(header, font_desc);
        gtk_container_add(GTK_CONTAINER(box), header);

        // Shelf content
        gtk_container_add(GTK_CONTAINER(box), content);

        return box;
    }

    // Create top shelf. It contains:
    // Input box + button to open level by ID or search
    // (If number is entered, open level by ID, otherwise search)
    static GtkWidget* create_top_shelf_content() {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_widget_set_halign(GTK_WIDGET(box), GTK_ALIGN_CENTER);
        gtk_widget_set_valign(GTK_WIDGET(box), GTK_ALIGN_CENTER);

        // Search icon
        GtkWidget *search_icon = gtk_image_new_from_icon_name("edit-find-symbolic", GTK_ICON_SIZE_BUTTON);
        gtk_container_add(GTK_CONTAINER(box), search_icon);

        // Input box
        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter level ID or search query");
        gtk_entry_set_width_chars(GTK_ENTRY(entry), 40);
        gtk_box_pack_start(GTK_BOX(box), entry, FALSE, TRUE, 0);
        gtk_box_set_center_widget(GTK_BOX(box), entry);
        g_signal_connect(entry, "activate", G_CALLBACK(on_search_activate), NULL);

        return box;
    }

    static GtkWidget* create_dialog(const std::vector<api::recent_level> &levels) {
        // Create a dialog window
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Community Levels",
            NULL,
            (GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL),
            ("_Open Full Website"),
            GTK_RESPONSE_ACCEPT,
            ("_Close"),
            GTK_RESPONSE_CLOSE,
            NULL
        );
        gtk_window_set_keep_above(GTK_WINDOW(dialog), true);
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);

        // Get the content area of the dialog
        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

        // Add the top shelf
        GtkWidget *top_shelf_content = create_top_shelf_content();
        GtkWidget *top_shelf = create_shelf("Open level", top_shelf_content);
        gtk_container_add(GTK_CONTAINER(content_area), top_shelf);

        // Add the level shelf to the dialog
        GtkWidget *level_grid = create_level_grid(levels);
        GtkWidget *level_shelf = create_shelf("Recent Levels", level_grid);
        gtk_container_add(GTK_CONTAINER(content_area), level_shelf);

        return dialog;
    }
}

gboolean open_community_level_browser(gpointer _) {
    tms_infof("====== Open level browser ======");

    tms_infof("Fetching recent levels...");
    std::vector<api::recent_level> levels = api::get_recent_levels(0, 12);

    tms_infof("Creating dialog...");
    GtkWidget *dialog = gtk_community::create_dialog(levels);
    gtk_community::global_dialog = dialog;
    gtk_widget_show_all(dialog);

    return false;
}

void init_community_level_browser() {
    const gchar* css_global = R"(
        .hc-level-tile {
            padding: 5px;
            border: 1px solid #444;
            border-radius: 4px;
            background-color: #060606;
        }
        .hc-level-tile button {
            padding: 0;
        }
    )";
    GtkCssProvider* css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(
        css_provider,
        css_global,
        -1, NULL
    );
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
}

#endif

