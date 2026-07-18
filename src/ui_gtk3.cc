/**
 * Note: The GTK3 dialog backend is deprecated and will be replaced with the
 * Imgui backend on desktop platforms when it is finished. Don't spend any
 * more time than absolutely necessary on this backend.
 */

#include "game.hh"
#include "main.hh"
#include "menu-play.hh"
#include "object_factory.hh"
#include "pkgman.hh"
#include "ui.hh"
#include <SDL3/SDL.h>
#include <tms/cpp.hh>

#if !defined(SDL_PLATFORM_ANDROID) && !defined(NO_UI)

#ifndef PRINCIPIA_BACKEND_IMGUI

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

static gboolean _close_all_dialogs(gpointer unused);

bool   ui_ready = false;
SDL_Condition  *ui_cond;
SDL_Mutex *ui_lock;
static gboolean _sig_ui_ready(gpointer unused);

/* open window columns */
enum {
    OC_ID,
    OC_NAME,
    OC_VERSION,
    OC_DATE,

    OC_NUM_COLUMNS
};

enum {
    OSC_ID,
    OSC_NAME,
    OSC_DATE,
    OSC_SAVE_ID,
    OSC_ID_TYPE,

    OSC_NUM_COLUMNS
};

/** --Open state **/
GtkWindow    *open_state_window;
GtkTreeModel *open_state_treemodel;
GtkTreeView  *open_state_treeview;
GtkButton    *open_state_btn_open;
GtkButton    *open_state_btn_cancel;
static bool   open_state_no_testplaying = false;

/** --Multi config **/
GtkWindow    *multi_config_window;
GtkNotebook  *multi_config_nb;
GtkButton    *multi_config_apply;
GtkButton    *multi_config_cancel;
int           multi_config_cur_tab = 0;
enum {
    TAB_JOINT_STRENGTH,
    TAB_PLASTIC_COLOR,
    TAB_PLASTIC_DENSITY,
    TAB_CONNECTION_RENDER_TYPE,
    TAB_MISCELLANEOUS,

    NUM_MULTI_CONFIG_TABS
};
/* Joint strength */
GtkScale    *multi_config_joint_strength;
/* Plastic color */
GtkColorChooserWidget *multi_config_plastic_color;
/* Plastic density */
GtkScale    *multi_config_plastic_density;
/* Connection render type */
GtkRadioButton  *multi_config_render_type_normal;
GtkRadioButton  *multi_config_render_type_small;
GtkRadioButton  *multi_config_render_type_hide;
/* Miscellaneous */
GtkButton       *multi_config_unlock_all;
GtkButton       *multi_config_disconnect_all;

/** --Open object **/
bool         object_window_multiemitter;
GtkWindow    *object_window;
GtkTreeModel *object_treemodel;
GtkTreeView  *object_treeview;
GtkButton    *object_btn_open;
GtkButton    *object_btn_cancel;

static gboolean on_window_close(GtkWidget *w, void *unused) {
    P.focused = true;
    gtk_widget_hide(w);
    return true;
}

static GtkWidget* help_widget(const char *text) {
    GtkWidget *r = gtk_image_new_from_icon_name("help-about", GTK_ICON_SIZE_MENU); //16px
    gtk_widget_set_tooltip_text(r, text);
    return r;
}

static GtkCellRenderer *add_text_column(GtkTreeView *tv, const char *title, int id) {
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
    column = GTK_TREE_VIEW_COLUMN(gtk_tree_view_column_new_with_attributes(title, renderer, "text", id, NULL));

    gtk_tree_view_column_set_sort_column_id(column, id);
    gtk_tree_view_append_column(tv, column);

    return renderer;
}

static GtkWidget *new_lbl(const char *text) {
    GtkWidget *r = gtk_label_new(0);
    gtk_label_set_markup(GTK_LABEL(r), text);

    return r;
}

static GtkButton *new_lbtn(const char *text, gboolean (*on_click)(GtkWidget*, GdkEventButton*, gpointer)) {
    GtkButton *btn = GTK_BUTTON(gtk_button_new_with_label(text));
    g_signal_connect(btn, "clicked",
            G_CALLBACK(on_click), 0);

    return btn;
}


static void notebook_append(GtkNotebook *nb, const char *title, GtkBox *base) {
    gtk_notebook_append_page(nb, GTK_WIDGET(base), new_lbl(title));
}

static void apply_dialog_defaults(
    void *w,
    GtkCallback on_show=0,
    gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)=0
) {
    gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);
    gtk_window_set_keep_above(GTK_WINDOW(w), TRUE);
    g_signal_connect(w, "delete-event", G_CALLBACK(on_window_close), 0);

    if (on_show)
        g_signal_connect(w, "show", G_CALLBACK(on_show), 0);

    if (on_keypress)
        g_signal_connect(w, "key-press-event", G_CALLBACK(on_keypress), 0);
}

static GtkWindow *new_window_defaults(const char *title, GtkCallback on_show=0, gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)=0) {
    GtkWidget *r = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(r), 10);
    gtk_window_set_title(GTK_WINDOW(r), title);
    gtk_window_set_resizable(GTK_WINDOW(r), false);

    apply_dialog_defaults(r, on_show, on_keypress);

    return GTK_WINDOW(r);
}

static gchar *format_joint_strength(GtkScale *scale, gdouble value) {
    if (value >= 1.0)
        return g_strdup("Indestructible");
    else
        return g_strdup_printf("%0.*f", gtk_scale_get_digits(scale), value);
}

bool btn_pressed(GtkWidget *ref, GtkButton *btn, gpointer user_data) {
    return (
        ref == GTK_WIDGET(btn) &&
        (
            ((gtk_widget_get_state_flags(ref) & GTK_STATE_ACTIVE) != 0) ||
            GPOINTER_TO_INT(user_data) == 1
        )
    );
}

void on_object_show(GtkWidget *wdg, void *unused) {
    GtkTreeIter iter;

    gtk_list_store_clear(GTK_LIST_STORE(object_treemodel));

    lvlfile *level = pkgman::get_levels(LEVEL_PARTIAL);

    while (level) {
        gtk_list_store_append(GTK_LIST_STORE(object_treemodel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(object_treemodel), &iter,
                OC_ID, level->id,
                OC_NAME, level->name,
                OC_DATE, level->modified_date,
                -1
                );
        lvlfile *next = level->next;
        delete level;
        level = next;
    }

    GtkTreePath      *path;
    GtkTreeSelection *sel;

    path = gtk_tree_path_new_from_indices(0, -1);
    sel  = gtk_tree_view_get_selection(object_treeview);

    gtk_tree_model_get_iter(object_treemodel,
                            &iter,
                            path);

    GValue val = {0, };

    gtk_tree_model_get_value(object_treemodel,
                             &iter,
                             0,
                             &val);

    gtk_tree_selection_select_path(sel, path);

    gtk_tree_path_free(path);
}

/** --Open state **/
void on_open_state_show(GtkWidget *wdg, void *unused) {
    GtkTreeIter iter;

    gtk_list_store_clear(GTK_LIST_STORE(open_state_treemodel));

    lvlfile *level = pkgman::get_levels(LEVEL_LOCAL_STATE);

    while (level) {
        gtk_list_store_append(GTK_LIST_STORE(open_state_treemodel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(open_state_treemodel), &iter,
                OSC_ID, level->id,
                OSC_NAME, level->name,
                OSC_DATE, level->modified_date,
                OSC_SAVE_ID, level->save_id,
                OSC_ID_TYPE, level->id_type,
                -1
                );
        lvlfile *next = level->next;
        delete level;
        level = next;
    }

    GtkTreePath      *path;
    GtkTreeSelection *sel;

    path = gtk_tree_path_new_from_indices(0, -1);
    sel  = gtk_tree_view_get_selection(open_state_treeview);

    gtk_tree_model_get_iter(open_state_treemodel,
                            &iter,
                            path);

    GValue val = {0, };

    gtk_tree_model_get_value(open_state_treemodel,
                             &iter,
                             0,
                             &val);

    gtk_tree_selection_select_path(sel, path);

    tms_infof("got id: %d", g_value_get_uint(&val));
    gtk_tree_path_free(path);
}

static void open_state_row(GtkTreeIter *iter) {
    if (!iter)
        return;

    guint _level_id;
    gtk_tree_model_get(open_state_treemodel, iter,
            OSC_ID, &_level_id,
            -1);
    guint _save_id;
    gtk_tree_model_get(open_state_treemodel, iter,
            OSC_SAVE_ID, &_save_id,
            -1);

    guint _level_id_type;
    gtk_tree_model_get(open_state_treemodel, iter,
            OSC_ID_TYPE, &_level_id_type,
            -1);

    uint32_t level_id = (uint32_t)_level_id;
    uint32_t save_id = (uint32_t)_save_id;
    uint32_t id_type = (uint32_t)_level_id_type;

    tms_infof("clicked level id %u save %u ", level_id, save_id);

    uint32_t *info = (uint32_t*)malloc(sizeof(uint32_t)*3);
    info[0] = id_type;
    info[1] = level_id;
    info[2] = save_id;

    if (open_state_no_testplaying) {
        G->state.test_playing = false;
        G->screen_back = P.s_menu_play;
    }

    P.add_action(ACTION_OPEN_STATE, info);

    gtk_widget_hide(GTK_WIDGET(open_state_window));
}

static void activate_open_state_row(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data) {
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    open_state_row(&iter);
}

static void confirm_import(uint32_t level_id) {
    if (object_window_multiemitter)
        P.add_action(ACTION_MULTIEMITTER_SET, level_id);
    else
        P.add_action(ACTION_SELECT_IMPORT_OBJECT, level_id);
}

void activate_object_row(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data) {
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    guint _level_id;
    gtk_tree_model_get(model, &iter,
                       OC_ID, &_level_id,
                       -1);

    confirm_import((uint32_t)_level_id);

    gtk_widget_hide(GTK_WIDGET(object_window));
}

gboolean on_open_state_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data) {
    if (btn_pressed(w, open_state_btn_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(open_state_window));
    } else if (btn_pressed(w, open_state_btn_open, user_data)) {
        /* open ! */
        GtkTreeSelection *sel;
        GtkTreeIter       iter;

        sel = gtk_tree_view_get_selection(open_state_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */
            open_state_row(&iter);

        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean on_object_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data) {
    if (btn_pressed(w, object_btn_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(object_window));
    } else if (btn_pressed(w, object_btn_open, user_data)) {
        /* open ! */
        GtkTreeSelection *sel;
        GtkTreeIter       iter;
        GValue            val = {0, };

        sel = gtk_tree_view_get_selection(object_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */

            /* Fetch the value of the first column into `val' */
            gtk_tree_model_get_value(object_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t level_id = g_value_get_uint(&val);

            confirm_import(level_id);

            gtk_widget_hide(GTK_WIDGET(object_window));
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean on_object_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused) {
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        GtkTreeSelection *sel;
        GtkTreeIter       iter;
        GValue            val = {0, };

        sel = gtk_tree_view_get_selection(object_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */

            /* Fetch the value of the first column into `val' */
            gtk_tree_model_get_value(object_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t level_id = g_value_get_uint(&val);

            confirm_import(level_id);

            gtk_widget_hide(w);
            return true;
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean on_open_state_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused) {
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        GtkTreeSelection *sel;
        GtkTreeIter       iter;

        sel = gtk_tree_view_get_selection(open_state_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */
            guint _level_id;
            gtk_tree_model_get(open_state_treemodel, &iter,
                               OSC_ID, &_level_id,
                               -1);
            guint _save_id;
            gtk_tree_model_get(open_state_treemodel, &iter,
                               OSC_SAVE_ID, &_save_id,
                               -1);

            guint _level_id_type;
            gtk_tree_model_get(open_state_treemodel, &iter,
                               OSC_ID_TYPE, &_level_id_type,
                               -1);

            uint32_t level_id = (uint32_t)_level_id;
            uint32_t save_id = (uint32_t)_save_id;
            uint32_t id_type = (uint32_t)_level_id_type;

            tms_infof("clicked level id %u save %u ", level_id, save_id);

            uint32_t *info = (uint32_t*)malloc(sizeof(uint32_t)*3);
            info[0] = id_type;
            info[1] = level_id;
            info[2] = save_id;

            P.add_action(ACTION_OPEN_STATE, info);

            gtk_widget_hide(w);
            return true;
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

void activate_open_state(GtkMenuItem *i, gpointer unused) {
    gtk_widget_show_all(GTK_WIDGET(open_state_window));
}

void activate_object(GtkMenuItem *i, gpointer unused) {
    gtk_widget_show_all(GTK_WIDGET(object_window));
}

/** --Multi config **/
static void on_multi_config_show(GtkWidget *wdg, void *unused) {
    gtk_range_set_value(GTK_RANGE(multi_config_joint_strength), 1.0);

    bool any_entity_locked = false;

    bool enabled_tabs[NUM_MULTI_CONFIG_TABS];
    for (int x=0; x<NUM_MULTI_CONFIG_TABS; ++x) {
        enabled_tabs[x] = false;
    }

    enabled_tabs[TAB_JOINT_STRENGTH]            = true;
    enabled_tabs[TAB_CONNECTION_RENDER_TYPE]    = true;
    enabled_tabs[TAB_MISCELLANEOUS]             = true;

    if (G->state.sandbox && W->is_paused() && !G->state.test_playing) {
        if (G->get_mode() == GAME_MODE_MULTISEL && G->selection.m) {
            for (std::set<entity*>::iterator i = G->selection.m->begin();
                    i != G->selection.m->end(); i++) {
                entity *e = *i;

                if (e->flag_active(ENTITY_IS_PLASTIC)) {
                    enabled_tabs[TAB_PLASTIC_COLOR] = true;
                    enabled_tabs[TAB_PLASTIC_DENSITY] = true;
                }

                if (e->flag_active(ENTITY_IS_LOCKED)) {
                    any_entity_locked = true;
                }
            }
        }
    }

    for (int x=0; x<NUM_MULTI_CONFIG_TABS; ++x) {
        GtkWidget *page = gtk_notebook_get_nth_page(multi_config_nb, x);

        if (!enabled_tabs[x])
            gtk_widget_hide(page);
        else
            gtk_widget_show(page);
    }

    gtk_widget_set_sensitive(GTK_WIDGET(multi_config_unlock_all), any_entity_locked);
}

static gboolean on_multi_config_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data) {
    if (btn_pressed(w, multi_config_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    } else if (btn_pressed(w, multi_config_apply, user_data)) {
        tms_debugf("cur tab: %d", multi_config_cur_tab);

        switch (multi_config_cur_tab) {
            case TAB_JOINT_STRENGTH:
                {
                    float val = tclampf(gtk_range_get_value(GTK_RANGE(multi_config_joint_strength)), 0.f, 1.f);
                    P.add_action(ACTION_MULTI_JOINT_STRENGTH, INT_TO_VOID(val * 100.f));
                }
                break;

            case TAB_PLASTIC_COLOR:
                {
                    GdkRGBA color;
                    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(multi_config_plastic_color), &color);

                    tvec4 *vec = (tvec4*)malloc(sizeof(tvec4));
                    vec->r = color.red;
                    vec->g = color.green;
                    vec->b = color.blue;
                    vec->a = 1.0f;

                    P.add_action(ACTION_MULTI_PLASTIC_COLOR, (void*)vec);
                }
                break;

            case TAB_PLASTIC_DENSITY:
                {
                    float val = tclampf(gtk_range_get_value(GTK_RANGE(multi_config_plastic_density)), 0.f, 1.f);
                    P.add_action(ACTION_MULTI_PLASTIC_DENSITY, INT_TO_VOID(val * 100.f));
                }
                break;

            case TAB_CONNECTION_RENDER_TYPE:
                {
                    uint8_t render_type = CONN_RENDER_DEFAULT;

                    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_config_render_type_normal))) {
                        render_type = CONN_RENDER_DEFAULT;
                    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_config_render_type_small))) {
                        render_type = CONN_RENDER_SMALL;
                    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_config_render_type_hide))) {
                        render_type = CONN_RENDER_HIDE;
                    }

                    P.add_action(ACTION_MULTI_CHANGE_CONNECTION_RENDER_TYPE, UINT_TO_VOID(render_type));
                }
                break;

            default:
                tms_errorf("Unknown multi config tab: %d", multi_config_cur_tab);
                return false;
                break;
        }

        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    } else if (btn_pressed(w, multi_config_unlock_all, user_data)) {
        P.add_action(ACTION_MULTI_UNLOCK_ALL, 0);

        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    } else if (btn_pressed(w, multi_config_disconnect_all, user_data)) {
        P.add_action(ACTION_MULTI_DISCONNECT_ALL, 0);

        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    }

    return false;
}

static void on_multi_config_tab_changed(GtkNotebook *nb, GtkWidget *page, gint tab_num, gpointer unused) {
    multi_config_cur_tab = tab_num;

    gtk_widget_set_sensitive(GTK_WIDGET(multi_config_apply), (tab_num != TAB_MISCELLANEOUS));
}

int _gtk_loop(void *p) {
    gtk_init(NULL, NULL);

    g_object_set(
        gtk_settings_get_default(),
        "gtk-application-prefer-dark-theme", true,
        "gtk-tooltip-timeout", 100,
        NULL
    );

    /** --Open object **/
    {
        object_window = new_window_defaults("Import object", &on_object_show, &on_object_keypress);
        gtk_window_set_default_size(GTK_WINDOW(object_window), 600, 600);
        gtk_widget_set_size_request(GTK_WIDGET(object_window), 600, 600);
        gtk_window_set_resizable(GTK_WINDOW(object_window), true);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkListStore *store;

        store = gtk_list_store_new(OC_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

        object_treemodel = GTK_TREE_MODEL(store);

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), OC_DATE, GTK_SORT_DESCENDING);

        object_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(object_treemodel));
        gtk_tree_view_set_search_column(object_treeview, OC_NAME);
        g_signal_connect(GTK_WIDGET(object_treeview), "row-activated", G_CALLBACK(activate_object_row), 0);

        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_box_set_spacing(GTK_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Open button */
        object_btn_open   = GTK_BUTTON(gtk_button_new_with_label("Open"));
        g_signal_connect(object_btn_open, "clicked",
                G_CALLBACK(on_object_btn_click), 0);

        object_btn_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(object_btn_cancel, "clicked",
                G_CALLBACK(on_object_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(object_btn_open));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(object_btn_cancel));

        gtk_box_pack_start(content, GTK_WIDGET(ew), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(object_treeview));
        gtk_container_add(GTK_CONTAINER(object_window), GTK_WIDGET(content));

        add_text_column(object_treeview, "ID", OC_ID);
        add_text_column(object_treeview, "Name", OC_NAME);
        add_text_column(object_treeview, "Version", OC_VERSION);
        add_text_column(object_treeview, "Modified", OC_DATE);
    }

    /** --Open state **/
    {
        open_state_window = new_window_defaults("Load saved game", &on_open_state_show, &on_open_state_keypress);
        gtk_window_set_default_size(GTK_WINDOW(open_state_window), 600, 600);
        gtk_widget_set_size_request(GTK_WIDGET(open_state_window), 600, 600);
        gtk_window_set_resizable(GTK_WINDOW(open_state_window), true);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkListStore *store;

        store = gtk_list_store_new(OSC_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);

        open_state_treemodel = GTK_TREE_MODEL(store);

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), OSC_DATE, GTK_SORT_DESCENDING);

        open_state_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(open_state_treemodel));
        gtk_tree_view_set_search_column(open_state_treeview, OSC_NAME);
        g_signal_connect(GTK_WIDGET(open_state_treeview), "row-activated", G_CALLBACK(activate_open_state_row), 0);

        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Open button */
        open_state_btn_open   = GTK_BUTTON(gtk_button_new_with_label("Open"));
        g_signal_connect(open_state_btn_open, "clicked",
                G_CALLBACK(on_open_state_btn_click), 0);

        open_state_btn_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(open_state_btn_cancel, "clicked",
                G_CALLBACK(on_open_state_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(open_state_btn_open));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(open_state_btn_cancel));

        gtk_box_pack_start(content, GTK_WIDGET(ew), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(open_state_treeview));
        gtk_container_add(GTK_CONTAINER(open_state_window), GTK_WIDGET(content));

        add_text_column(open_state_treeview, "Name", OSC_ID);
        add_text_column(open_state_treeview, "Modified", OSC_NAME);
    }

    /** --Multi config **/
    {
        multi_config_window = new_window_defaults("Multi config", &on_multi_config_show);
        gtk_window_set_default_size(GTK_WINDOW(multi_config_window), 600, 350);
        gtk_widget_set_size_request(GTK_WIDGET(multi_config_window), 600, 350);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *entries = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkNotebook *nb = GTK_NOTEBOOK(gtk_notebook_new());
        gtk_notebook_set_tab_pos(nb, GTK_POS_TOP);
        g_signal_connect(nb, "switch-page", G_CALLBACK(on_multi_config_tab_changed), 0);

        /* Buttons and button box */
        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Log in button */
        multi_config_apply = GTK_BUTTON(gtk_button_new_with_label("Apply"));
        g_signal_connect(multi_config_apply, "clicked",
                G_CALLBACK(on_multi_config_btn_click), 0);

        /* Cancel button */
        multi_config_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(multi_config_cancel, "clicked",
                G_CALLBACK(on_multi_config_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(multi_config_apply));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(multi_config_cancel));

        {
            /* Joint strength */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_joint_strength = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05));
            g_signal_connect(multi_config_joint_strength, "format-value", G_CALLBACK(format_joint_strength), 0);

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_joint_strength), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("Settings a new joint might make your selection change it's position/state slightly.\nMake sure you save your level before you press Apply."), 0, 0, 0);

            notebook_append(nb, "Joint strength", box);
        }

        {
            /* Plastic color */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_plastic_color = GTK_COLOR_CHOOSER_WIDGET(gtk_color_chooser_widget_new());

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_plastic_color), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("This will change the color of all plastic objects in your current selection."), 1, 1, 0);

            notebook_append(nb, "Plastic color", box);
        }

        {
            /* Plastic density */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_plastic_density = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05));

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_plastic_density), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("This will change the density of all plastic objects in your current selection."), 1, 1, 0);

            notebook_append(nb, "Plastic density", box);
        }

        {
            /* Connection render type */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_render_type_normal = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
                        0, "Default"));
            multi_config_render_type_small = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
                        gtk_radio_button_get_group(multi_config_render_type_normal), "Small"));
            multi_config_render_type_hide = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
                        gtk_radio_button_get_group(multi_config_render_type_normal), "Hide"));

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_render_type_normal), 0, 0, 0);
            gtk_box_pack_start(box, GTK_WIDGET(multi_config_render_type_small), 0, 0, 0);
            gtk_box_pack_start(box, GTK_WIDGET(multi_config_render_type_hide), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("This will change the render type of all connections in your current selection."), 1, 1, 0);

            notebook_append(nb, "Connection render type", box);
        }

        {
            /* Miscellaneous */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_unlock_all = new_lbtn("Unlock all", &on_multi_config_btn_click);
            multi_config_disconnect_all = new_lbtn("Disconnect all", &on_multi_config_btn_click);

            {
                GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
                gtk_box_pack_start(hbox, GTK_WIDGET(multi_config_unlock_all), 1, 1, 0);
                gtk_box_pack_start(hbox, help_widget("Unlock any previously locked entities.\nOnly active if at least one of the selected entities is locked."), 0, 0, 0);

                gtk_box_pack_start(box, GTK_WIDGET(hbox), 0, 0, 0);
            }
            gtk_box_pack_start(box, GTK_WIDGET(multi_config_disconnect_all), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("Click on any of the buttons above to perform the given action on your current selection."), 1, 1, 0);

            notebook_append(nb, "Miscellaneous", box);
        }

        gtk_box_pack_start(entries, GTK_WIDGET(nb), 1, 1, 0);

        multi_config_nb = nb;

        gtk_box_pack_start(content, GTK_WIDGET(entries), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(multi_config_window), GTK_WIDGET(content));
    }

    gdk_threads_add_idle(_sig_ui_ready, 0);

    gtk_main();

    return T_OK;
}

static gboolean _sig_ui_ready(gpointer unused) {
    SDL_LockMutex(ui_lock);
    ui_ready = true;
    SDL_SignalCondition(ui_cond);
    SDL_UnlockMutex(ui_lock);

    return false;
}

static gboolean _open_open_state_dialog(gpointer unused) {
    activate_open_state(NULL, 0);
    return false;
}

static gboolean _open_multiemitter_dialog(gpointer unused) {
    object_window_multiemitter = true;
    activate_object(NULL, 0);
    return false;
}

static gboolean _open_object_dialog(gpointer unused) {
    object_window_multiemitter = false;
    activate_object(NULL, 0);
    return false;
}

static gboolean _open_multi_config(gpointer unused) {
    g_object_set(
        G_OBJECT(multi_config_plastic_color),
        "show-editor", FALSE,
        NULL
    );

    gtk_widget_show_all(GTK_WIDGET(multi_config_window));

    return false;
}

static gboolean _close_all_dialogs(gpointer unused) {
    gtk_widget_hide(GTK_WIDGET(open_state_window));
    gtk_widget_hide(GTK_WIDGET(object_window));
    return false;
}

static gboolean _close_absolutely_all_dialogs(gpointer unused) {
    _close_all_dialogs(0);

    return false;
}

static void wait_ui_ready() {
    SDL_LockMutex(ui_lock);
    if (!ui_ready) {
        SDL_WaitConditionTimeout(ui_cond, ui_lock, 4000);
        if (!ui_ready) tms_fatalf("Could not initialise game (GTK not ready)");
    }
    SDL_UnlockMutex(ui_lock);
}

#endif

#include "imgui.hh"
#include "ui_imgui.hh"

static ImguiDriver imgui_driver;

void ui::init() {
#ifndef PRINCIPIA_BACKEND_IMGUI
    ui_lock = SDL_CreateMutex();
    ui_cond = SDL_CreateCondition();
    ui_ready = false;

    SDL_Thread *gtk_thread;

    gtk_thread = SDL_CreateThread(_gtk_loop, "_gtk_loop", 0);

    if (gtk_thread == NULL)
        tms_errorf("SDL_CreateThread failed: %s", SDL_GetError());
#endif

    imgui_driver = ImguiDriver();
    imgui_driver.init();
}

void ui::open_dialog(int num, void *data/*=0*/) {
#ifndef PRINCIPIA_BACKEND_IMGUI
    wait_ui_ready();
#endif

    switch (num) {
        case DIALOG_SANDBOX_MENU:
            UiSandboxMenu::open();
            break;

        case DIALOG_LEVEL_PROPERTIES:
            UiLevelProperties::open();
            break;
        case DIALOG_EXPORT:
            UiExport::open();
            break;
        case DIALOG_PLAY_MENU:
            UiPlayMenu::open();
            break;
        case DIALOG_QUICKADD:
            UiQuickadd::open();
            break;

        case DIALOG_SHAPEEXTRUDER:
            UiShapeExtruder::open();
            break;
        case DIALOG_CURSORFIELD:
            UiCursorField::open();
            break;
        case DIALOG_ESCRIPT:
            UiLuaEditor::open();
            break;
        case DIALOG_JUMPER:
            UiJumper::open();
            break;
        case DIALOG_BEAM_COLOR:
        case DIALOG_PIXEL_COLOR:
        case DIALOG_POLYGON_COLOR:
            UiObjColorPicker::open();
            break;

        case DIALOG_SAVE:
            UiSave::open(false);
            break;
        case DIALOG_SAVE_COPY:
            UiSave::open(true);
            break;
        case DIALOG_OPEN:
            UiLevelManager::open();
            break;

#ifndef PRINCIPIA_BACKEND_IMGUI
        case DIALOG_OPEN_STATE:
            if (data && VOID_TO_UINT8(data) == 1) {
                open_state_no_testplaying = true;
            } else {
                open_state_no_testplaying = false;
            }

            gdk_threads_add_idle(_open_open_state_dialog, 0);
            break;

        case DIALOG_OPEN_OBJECT:    gdk_threads_add_idle(_open_object_dialog, 0); break;
        case DIALOG_MULTIEMITTER:   gdk_threads_add_idle(_open_multiemitter_dialog, 0); break;
#endif

        case DIALOG_EMITTER:
            UiEmitter::open();
            break;
        case DIALOG_NEW_LEVEL:
            UiNewLevel::open();
            break;
        case DIALOG_SANDBOX_MODE:
            UiSandboxMode::open();
            break;
        case DIALOG_SET_FREQUENCY:
            UiFrequency::open(false);
            break;
        case DIALOG_CONFIRM_QUIT:
            UiConfirmQuit::open();
            break;
        case DIALOG_SET_COMMAND:
            UiCommandPad::open();
            break;
        case DIALOG_STICKY:
            UiSticky::open();
            break;
        case DIALOG_DIGITALDISPLAY:
            UiDigitalDisplay::open();
            break;
        case DIALOG_FXEMITTER:
            UiFXEmitter::open();
            break;
        case DIALOG_EVENTLISTENER:
            UiEventListener::open();
            break;
        case DIALOG_SFXEMITTER:
            UiSfxEmitterLegacy::open();
            break;
        case DIALOG_SFXEMITTER_2:
            UiSfxEmitter::open();
            break;
        case DIALOG_CAMTARGETER:
            UiCamTargeter::open();
            break;
        case DIALOG_SET_FREQ_RANGE:
            UiFrequency::open(true);
            break;
        case DIALOG_SET_PKG_LEVEL:
            UiPkgLvlSelector::open();
            break;
        case DIALOG_ROBOT:
            UiRobot::open();
            break;
        case DIALOG_TIMER:
            UiTimer::open();
            break;
        case DIALOG_SYNTHESIZER:
            UiSynthesizer::open();
            break;
        case DIALOG_SEQUENCER:
            UiSequencer::open();
            break;
        case DIALOG_SETTINGS:
            UiSettings::open();
            break;
        case DIALOG_VARIABLE:
            UiVariable::open();
            break;
        case DIALOG_ITEM:
            UiItem::open();
            break;
        case DIALOG_DECORATION:
            UiDecoration::open();
            break;
        case DIALOG_SET_FACTION:
            UiSetFaction::open();
            break;
        case DIALOG_RESOURCE:
            UiResource::open();
            break;
        case DIALOG_VENDOR:
            UiVendor::open();
            break;
        case DIALOG_FACTORY:
            UiFactory::open();
            break;
        case DIALOG_TREASURE_CHEST:
            UiTreasureChest::open();
            break;
        case DIALOG_RUBBER:
            UiRubber::open();
            break;
        case DIALOG_PUBLISHED:
            UiPublished::open();
            break;
        case DIALOG_COMMUNITY:
            UiCommunity::open();
            break;
        case DIALOG_ANIMAL:
            UiAnimal::open();
            break;
        case DIALOG_SOUNDMAN:
            UiSoundManager::open();
            break;
        case DIALOG_POLYGON:
            UiPolygon::open();
            break;
        case DIALOG_KEY_LISTENER:
            UiKeyListener::open();
            break;
#ifndef PRINCIPIA_BACKEND_IMGUI
        case DIALOG_MULTI_CONFIG:   gdk_threads_add_idle(_open_multi_config, 0); break;

        case CLOSE_ALL_DIALOGS:     gdk_threads_add_idle(_close_all_dialogs, 0); break;
        case CLOSE_ABSOLUTELY_ALL_DIALOGS: gdk_threads_add_idle(_close_absolutely_all_dialogs, 0); break;
#else
        case CLOSE_ABSOLUTELY_ALL_DIALOGS:
        case CLOSE_ALL_DIALOGS:
            tms_infof("XXX: CLOSE_ALL_DIALOGS/CLOSE_ABSOLUTELY_ALL_DIALOGS (200/201) are intentionally ignored");
            break;
#endif

        case DIALOG_PUBLISH:
            UiPublish::open();
            break;
        case DIALOG_LOGIN:
            UiLogin::open();
            break;

        case DIALOG_LEVEL_INFO:
            UiMessage::open((char *)data, MessageType::LevelInfo);
            break;

        case DIALOG_PROMPT:
            if (G)
                G->reset_touch(false);
            UiPrompt::open();
            break;

        case DIALOG_PROMPT_SETTINGS:
            UiPromptSettings::open();
            break;

        default:
            tms_warnf("Unhandled dialog ID: %d", num);
            break;
    }

#ifndef PRINCIPIA_BACKEND_IMGUI
    gdk_display_flush(gdk_display_get_default());
#endif
}

void ui::open_sandbox_tips() {
    UiTips::open();
}

void ui::set_next_action(int action_id) {
    tms_infof("set_next_Actino: %d", action_id);
    ui::next_action = action_id;
}

void ui::emit_signal(int num, void *data/*=0*/) {
#ifndef PRINCIPIA_BACKEND_IMGUI
    wait_ui_ready();
#endif

    switch (num) {
        case SIGNAL_LOGIN_SUCCESS:
            UiLogin::complete_login(num);
            if (ui::next_action != ACTION_IGNORE) {
                P.add_action(ui::next_action, 0);
                ui::next_action = ACTION_IGNORE;
            }
            break;

        case SIGNAL_LOGIN_FAILED:
            ui::next_action = ACTION_IGNORE;
            UiLogin::complete_login(num);
            break;

        case SIGNAL_REFRESH_BORDERS:
            UiLevelProperties::reload_border_sizes();
            break;
    }

    ui::next_action = ACTION_IGNORE;
}

void ui::quit() {
    /* TODO: add proper quit stuff here */
    _tms.state = TMS_STATE_QUITTING;

    imgui_driver.quit();
}

void ui::open_error_dialog(const char *error_msg) {
    UiMessage::open(error_msg, MessageType::Error);
}

void ui::confirm(const char *text,
        const char *button1, principia_action action1,
        const char *button2, principia_action action2,
        const char *button3/*=0*/, principia_action action3/*=ACTION_IGNORE*/,
        struct confirm_data _confirm_data/*=none*/
        ) {

    UiConfirm::open(text, button1, action1, button2, action2, button3, action3, _confirm_data);
}

void ui::alert(const char *text, uint8_t alert_type/*=ALERT_INFORMATION*/) {
    UiMessage::open(text, MessageType::Message);
}

void ui::render() {
    imgui_driver.pre_render();

    UiSandboxMenu::layout();
    UiSandboxMode::layout();
    UiQuickadd::layout();
    UiPlayMenu::layout();
    UiNewLevel::layout();
    UiLogin::layout();
    UiAnimal::layout();
    UiRubber::layout();
    UiObjColorPicker::layout();
    UiPolygon::layout();
    UiVariable::layout();
    UiSticky::layout();
    UiJumper::layout();
    UiDecoration::layout();
    UiEmitter::layout();
    UiCommandPad::layout();
    UiFrequency::layout();
    UiPkgLvlSelector::layout();
    UiEventListener::layout();
    UiResource::layout();
    UiItem::layout();
    UiSave::layout();
    UiLevelManager::layout();
    UiShapeExtruder::layout();
    UiCursorField::layout();
    UiSettings::layout();
    UiSetFaction::layout();
    UiConfirm::layout();
    UiMessage::layout();
    UiKeyListener::layout();
    UiTips::layout();
    UiPublish::layout();
    UiPublished::layout();
    UiTimer::layout();
    UiCommunity::layout();
    UiConfirmQuit::layout();
    UiLuaEditor::layout();
    UiCamTargeter::layout();
    UiVendor::layout();
    UiFXEmitter::layout();
    UiPrompt::layout();
    UiPromptSettings::layout();
    UiSoundManager::layout();
    UiSequencer::layout();
    UiExport::layout();
    UiTreasureChest::layout();
    UiLevelProperties::layout();
    UiRobot::layout();
    UiDigitalDisplay::layout();
    UiFactory::layout();
    UiSfxEmitter::layout();
    UiSfxEmitterLegacy::layout();
    UiSynthesizer::layout();

    imgui_driver.post_render();
}

#endif
