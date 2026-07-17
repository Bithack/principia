/**
 * Note: The GTK3 dialog backend is deprecated and will be replaced with the
 * Imgui backend on desktop platforms when it is finished. Don't spend any
 * more time than absolutely necessary on this backend.
 */

#include "display.hh"
#include "faction.hh"
#include "factory.hh"
#include "game.hh"
#include "item.hh"
#include "main.hh"
#include "menu-play.hh"
#include "object_factory.hh"
#include "pkgman.hh"
#include "sfxemitter.hh"
#include "soundmanager.hh"
#include "speaker.hh"
#include "ui.hh"
#include <SDL3/SDL.h>
#include <sstream>
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

static guint valid_keys[9] = {
    GDK_KEY_1,
    GDK_KEY_2,
    GDK_KEY_3,
    GDK_KEY_4,
    GDK_KEY_5,
    GDK_KEY_6,
    GDK_KEY_7,
    GDK_KEY_8,
    GDK_KEY_9
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

/** --Factory **/
GtkDialog       *factory_dialog;
GtkSpinButton   *factory_faction;
GtkSpinButton   *factory_oil;
GtkSpinButton   *factory_resources[NUM_RESOURCES];
GtkListStore    *factory_liststore;
GtkTreeView     *factory_treeview;
GtkButton       *factory_cancel;
enum {
  FACTORY_COLUMN_ENABLED,
  FACTORY_COLUMN_INDEX,
  FACTORY_COLUMN_RECIPE,
  FACTORY_COLUMN_RECIPE_ID,
};

/** --Digital Display **/
GtkDialog       *digi_dialog;
GtkCheckButton  *digi_wrap;
GtkToggleButton *digi_check[7][5];
GtkSpinButton   *digi_initial;

GtkLabel   *digi_label;

GtkButton   *digi_prev;
GtkButton   *digi_next;
GtkButton   *digi_insert;
GtkButton   *digi_append;
GtkButton   *digi_delete;

/** --SFX Emitter **/
GtkDialog       *sfx_dialog;
GtkComboBoxText *sfx_cb;
GtkCheckButton  *sfx_global;

/** --SFX Emitter 2 **/
GtkDialog       *sfx2_dialog;
GtkComboBoxText *sfx2_cb;
GtkComboBoxText *sfx2_sub_cb;
GtkCheckButton  *sfx2_global;
GtkCheckButton  *sfx2_loop;


/** --Synthesizer **/
GtkDialog       *synth_dialog;
GtkSpinButton   *synth_hz_low;
GtkSpinButton   *synth_hz_high;

GtkRange       *synth_bitcrushing;

GtkRange       *synth_freq_vibrato_hz;
GtkRange       *synth_freq_vibrato_extent;

GtkRange       *synth_vol_vibrato_hz;
GtkRange       *synth_vol_vibrato_extent;

GtkRange       *synth_pulse_width;

GtkComboBoxText *synth_waveform;

gboolean on_digi_next_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data);
gboolean on_digi_prev_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data);
gboolean on_digi_insert_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data);
gboolean on_digi_append_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data);
gboolean on_digi_delete_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data);

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

static GtkComboBoxText *new_item_cb() {
    GtkListStore *store;
    GtkComboBoxText *cb;

    store = gtk_list_store_new(1, G_TYPE_STRING);

    cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    g_object_unref(store);

    return cb;
}

static void item_cb_append(GtkComboBoxText *cb, uint32_t item_id, bool first_is_none) {
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(cb));
    int num = gtk_tree_model_iter_n_children(model, 0);

    if (first_is_none && num == 0)
        gtk_combo_box_text_append_text(cb, "None");
    else
        gtk_combo_box_text_append_text(cb, item::get_ui_name(item_id));
}

static void clear_cb(GtkComboBoxText *cb) {
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(cb));
    gtk_list_store_clear(GTK_LIST_STORE(model));
}

static GtkButton *new_lbtn(const char *text, gboolean (*on_click)(GtkWidget*, GdkEventButton*, gpointer)) {
    GtkButton *btn = GTK_BUTTON(gtk_button_new_with_label(text));
    g_signal_connect(btn, "clicked",
            G_CALLBACK(on_click), 0);

    return btn;
}

static GtkWidget *new_clbl(const char *text) {
    GtkWidget *r = gtk_label_new(0);
    gtk_label_set_markup(GTK_LABEL(r), text);
    gtk_label_set_xalign(GTK_LABEL(r), 0.0f);
    gtk_label_set_yalign(GTK_LABEL(r), 0.5f);
    return r;
}

static GtkWidget *new_rlbl(const char *text) {
    GtkWidget *r = gtk_label_new(0);
    gtk_label_set_markup(GTK_LABEL(r), text);
    gtk_label_set_xalign(GTK_LABEL(r), 1.0f);
    gtk_label_set_yalign(GTK_LABEL(r), 0.5f);
    return r;
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

static GtkGrid *create_settings_table() {
    GtkGrid *tbl = GTK_GRID(gtk_grid_new());

    gtk_grid_set_column_spacing(tbl, 15);
    gtk_grid_set_row_spacing(tbl, 6);

    gtk_grid_set_column_homogeneous(tbl, false);
    gtk_grid_set_row_homogeneous(tbl, false);

    g_object_set (
        G_OBJECT(tbl),
        "margin", 10,
        NULL
    );

    return tbl;
}

static void add_setting_row(GtkGrid *tbl, int y, const char *label, GtkWidget *widget, const char *help_text = NULL) {
    //label
    gtk_grid_attach(
        tbl, new_rlbl(label),
        0, y,
        1, 1
    );

    //widget
    gtk_widget_set_hexpand(widget, true);
    gtk_grid_attach(
        tbl, widget,
        1, y,
        1, 1
    );

    //help
    if (help_text) {
        gtk_grid_attach(
            tbl, help_widget(help_text),
            2, y,
            1, 1
        );
    }
}

static GtkDialog *new_dialog_defaults(const char *title, GtkCallback on_show=0, gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)=0) {
    GtkWidget *r = gtk_dialog_new_with_buttons(
            title,
            0, (GtkDialogFlags)(0),
            "_OK", GTK_RESPONSE_ACCEPT,
            "_Cancel", GTK_RESPONSE_REJECT,
            NULL);

    apply_dialog_defaults(r, on_show, on_keypress);

    return GTK_DIALOG(r);
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

/** --digital display **/

uint64_t symbols[DISPLAY_MAX_SYMBOLS];
int num_digi_symbols = 0;
int curr_digi_symbol = 0;

void digi_load_symbols() {
    display *e = (display*)G->selection.e;

    num_digi_symbols = e->num_symbols;
    curr_digi_symbol = e->properties[1].v.i8;

    memcpy(symbols, e->symbols, DISPLAY_MAX_SYMBOLS*sizeof(uint64_t));
}

void digi_refresh_symbol() {
    char txt[256];

    if (curr_digi_symbol < 0) curr_digi_symbol = num_digi_symbols-1;
    else if (curr_digi_symbol >= num_digi_symbols) curr_digi_symbol = 0;

    sprintf(txt, "<b>Symbol %d/%d</b>", curr_digi_symbol+1, num_digi_symbols);
    gtk_label_set_markup(GTK_LABEL(digi_label), txt);

    for (int y=0; y<7; y++) {
        for (int x=0; x<5; x++) {
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(digi_check[y][x]),
                (symbols[curr_digi_symbol] & (1ull << ((uint64_t)y*5ull + (uint64_t)x))) ? true : false
            );
        }
    }
}

void on_digi_toggle(GtkToggleButton *togglebutton, gpointer user_data) {
    uint64_t which = (uint64_t)user_data;

    if (gtk_toggle_button_get_active(togglebutton))
        symbols[curr_digi_symbol] |= (1ull << which);
    else
        symbols[curr_digi_symbol] &= ~(1ull << which);
}

gboolean on_digi_next_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data) {
    curr_digi_symbol ++;
    digi_refresh_symbol();
    return false;
}

gboolean on_digi_prev_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data) {
    curr_digi_symbol --;
    digi_refresh_symbol();
    return false;
}

gboolean on_digi_insert_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data) {
    if (num_digi_symbols < DISPLAY_MAX_SYMBOLS) {
        num_digi_symbols ++;

        size_t sz = (num_digi_symbols - curr_digi_symbol - 1)*sizeof(uint64_t);
        if (sz > 0)
            memcpy(&symbols[curr_digi_symbol+1], &symbols[curr_digi_symbol], sz);
        memset(&symbols[curr_digi_symbol], 0, sizeof(uint64_t));
        digi_refresh_symbol();
    }
    return false;
}

gboolean on_digi_append_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data) {
    if (num_digi_symbols < DISPLAY_MAX_SYMBOLS) {
        num_digi_symbols ++;
        digi_refresh_symbol();
    }
    return false;
}

gboolean on_digi_delete_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data) {
    if (num_digi_symbols > 1) {
        if (curr_digi_symbol == num_digi_symbols-1) {
            num_digi_symbols --;
        } else {
            size_t sz = (num_digi_symbols - (curr_digi_symbol+1))*sizeof(uint64_t);
            if (sz > 0) {
                memcpy(&symbols[curr_digi_symbol], &symbols[curr_digi_symbol+1], sz);
            }
            num_digi_symbols --;
        }
        digi_refresh_symbol();
    }
    return false;
}

void on_digi_show(GtkWidget *wdg, void *unused) {
    entity *e = G->selection.e;

    if (e && (e->g_id == O_PASSIVE_DISPLAY || e->g_id == O_ACTIVE_DISPLAY)) {

        digi_load_symbols();
        digi_refresh_symbol();

        gtk_spin_button_set_value(digi_initial, e->properties[1].v.i8+1);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(digi_wrap), e->properties[0].v.i8);

        if (e->g_id == O_ACTIVE_DISPLAY)
            gtk_widget_set_sensitive(GTK_WIDGET(digi_wrap), false);
        else
            gtk_widget_set_sensitive(GTK_WIDGET(digi_wrap), true);
    }
}

/** --Synthesizer **/
void on_synth_show(GtkWidget *wdg, void *unused) {
    entity *e = G->selection.e;

    if (e && e->g_id == O_SYNTHESIZER) {
        float low = e->properties[0].v.f;
        float high = e->properties[1].v.f;
        gtk_spin_button_set_value(synth_hz_low, low);
        gtk_spin_button_set_value(synth_hz_high, high);

        gtk_range_set_value(synth_bitcrushing, e->properties[3].v.f);

        gtk_range_set_value(synth_pulse_width, e->properties[8].v.f);
        gtk_range_set_value(synth_vol_vibrato_hz, e->properties[4].v.f);
        gtk_range_set_value(synth_freq_vibrato_hz, e->properties[5].v.f);
        gtk_range_set_value(synth_vol_vibrato_extent, e->properties[6].v.f);
        gtk_range_set_value(synth_freq_vibrato_extent, e->properties[7].v.f);
        gtk_combo_box_set_active(GTK_COMBO_BOX(synth_waveform), e->properties[2].v.i);

        gtk_widget_grab_focus(GTK_WIDGET(synth_hz_low));
    }
}

/** --SFX Emitter **/
void on_sfx_show(GtkWidget *wdg, void *ununused) {
    entity *e = G->selection.e;

    if (e && e->g_id == O_SFX_EMITTER) {
        if (e->properties[0].v.i >= NUM_SFXEMITTER_OPTIONS) e->properties[0].v.i = NUM_SFXEMITTER_OPTIONS-1;

        gtk_combo_box_set_active(GTK_COMBO_BOX(sfx_cb), e->properties[0].v.i);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sfx_global), (e->properties[1].v.i8 == 1));
    }
}

/** --SFX Emitter 2 **/
static void on_sfx2_cb_changed(GtkComboBoxText *cb, gpointer user_data) {
    int index = gtk_combo_box_get_active(GTK_COMBO_BOX(cb));
    if (index < 0) {
        return;
    }

    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(sfx2_sub_cb));
    int num = gtk_tree_model_iter_n_children(model, 0);
    for (int x=0; x<num; ++x) {
        gtk_combo_box_text_remove(sfx2_sub_cb, 0);
    }

    const sm_sound *snd = sm::get_sound_by_id(index);

    if (!snd) {
        return;
    }

    gtk_combo_box_text_append_text(sfx2_sub_cb, "Random");
    for (int x=0; x<snd->num_chunks; ++x) {
        const sm_chunk &chunk = snd->chunks[x];

        if (chunk.name) {
            gtk_combo_box_text_append_text(sfx2_sub_cb, chunk.name);
        }
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(sfx2_sub_cb), 0);
}
void on_sfx2_show(GtkWidget *wdg, void *ununused) {
    entity *e = G->selection.e;

    if (e && e->g_id == O_SFX_EMITTER) {
        if (e->properties[0].v.i >= SND__NUM) e->properties[0].v.i = SND__NUM-1;

        gtk_combo_box_set_active(GTK_COMBO_BOX(sfx2_cb), e->properties[0].v.i);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sfx2_global), (e->properties[1].v.i8 == 1));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sfx2_loop), (e->properties[3].v.i8 == 1));

        gtk_combo_box_set_active(GTK_COMBO_BOX(sfx2_sub_cb), e->properties[2].v.i == SFX_CHUNK_RANDOM ? 0 : e->properties[2].v.i+1);
    }
}

/** --Factory **/
static void factory_calculate_indices() {
    tms_debugf("Calculating indices...");
    GtkTreeModel *model = GTK_TREE_MODEL(factory_liststore);
    GtkTreeIter iter;
    int index = 0;

    if (gtk_tree_model_get_iter_first(
            model,
            &iter)) {
        do {
            GValue val = {0, };
            gtk_tree_model_get_value(model,
                                     &iter,
                                     FACTORY_COLUMN_ENABLED,
                                     &val);
            gboolean enabled = g_value_get_boolean(&val);
            if (enabled == TRUE) {
                gtk_list_store_set(factory_liststore, &iter, FACTORY_COLUMN_INDEX, ++index, -1);
            } else {
                gtk_list_store_set(factory_liststore, &iter, FACTORY_COLUMN_INDEX, -1, -1);
            }
        } while (gtk_tree_model_iter_next(model, &iter));
    }
}

static void factory_enable_toggled(GtkCellRendererToggle *cell, gchar *path_str, gpointer data) {
    GtkTreeModel *model = (GtkTreeModel *)data;
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean fixed;

    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, 0, &fixed, -1);

    fixed ^= 1;

    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, fixed, -1);

    gtk_tree_path_free(path);

    factory_calculate_indices();
}

gboolean on_factory_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused) {
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(factory_cancel))) {
            gtk_dialog_response(factory_dialog, GTK_RESPONSE_CANCEL);
        } else {
            gtk_dialog_response(factory_dialog, GTK_RESPONSE_ACCEPT);
        }
    }

    return false;
}

void on_factory_show(GtkWidget *wdg, void *ununused) {
    entity *e = G->selection.e;

    if (e && IS_FACTORY(e->g_id)) {
        gtk_spin_button_set_value(factory_oil, e->properties[1].v.i);
        gtk_spin_button_set_value(factory_faction, e->properties[2].v.i);
        for (int x=0; x<NUM_RESOURCES; ++x) {
            gtk_spin_button_set_value(factory_resources[x], e->properties[FACTORY_NUM_EXTRA_PROPERTIES+x].v.i);
        }

        factory *fa = static_cast<factory*>(e);

        std::vector<struct factory_object> &objs = fa->objects();

        gtk_list_store_clear(factory_liststore);

        std::vector<uint32_t> recipes;
        factory::generate_recipes(&recipes, fa->properties[0].v.s.buf);

        GtkTreeIter iter;
        for (std::vector<struct factory_object>::const_iterator it = objs.begin();
                it != objs.end(); ++it) {
            const struct factory_object &fo = *it;
            int x = it - objs.begin();

            gtk_list_store_append(factory_liststore, &iter);
            gboolean enabled = FALSE;
            for (std::vector<uint32_t>::iterator it = recipes.begin(); it != recipes.end(); ++it) {
                if (*it == x) {
                    enabled = TRUE;
                    break;
                }
            }
            gtk_list_store_set(factory_liststore, &iter,
                    FACTORY_COLUMN_ENABLED, enabled,
                    FACTORY_COLUMN_INDEX, -1,
                    FACTORY_COLUMN_RECIPE, ((fa->factory_type == FACTORY_ARMORY || fa->factory_type == FACTORY_OIL_MIXER) ? item_options[fo.gid].name : of::get_object_name_by_gid(fo.gid)),
                    FACTORY_COLUMN_RECIPE_ID, x,
                    -1
                    );
        }

        factory_calculate_indices();
    }
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

gboolean on_synth_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused) {
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        /* duplicate code from _open_synth */
        entity *e = G->selection.e;

        if (e && e->g_id == O_SYNTHESIZER) {
            gtk_spin_button_update(synth_hz_low);
            gtk_spin_button_update(synth_hz_high);
            //gtk_spin_button_update(synth_vol_vibrato);
            //gtk_spin_button_update(synth_freq_vibrato);
            //gtk_spin_button_update(synth_bitcrushing);
            float low = gtk_spin_button_get_value(synth_hz_low);
            float high = gtk_spin_button_get_value(synth_hz_high);
            float pw = gtk_range_get_value(synth_pulse_width);
            float vb = gtk_range_get_value(synth_vol_vibrato_hz);
            float fb = gtk_range_get_value(synth_freq_vibrato_hz);
            float vba = gtk_range_get_value(synth_vol_vibrato_extent);
            float fba = gtk_range_get_value(synth_freq_vibrato_extent);
            float bitcrushing = gtk_range_get_value(synth_bitcrushing);

            if (high < low) high = low;

            e->properties[0].v.f = low;
            e->properties[1].v.f = high;

            int index = gtk_combo_box_get_active(GTK_COMBO_BOX(synth_waveform));

            e->properties[2].v.i = index;

            e->properties[3].v.f = bitcrushing;
            e->properties[4].v.f = vb;
            e->properties[5].v.f = fb;

            e->properties[6].v.f = vba;
            e->properties[7].v.f = fba;

            e->properties[8].v.f = pw;
        }
        gtk_widget_hide(w);
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

void activate_controls(GtkMenuItem *i, gpointer unused) {
    G->render_controls = true;
}

void activate_restart_level(GtkMenuItem *i, gpointer unused) {
    P.add_action(ACTION_RESTART_LEVEL, 0);
}

void activate_back(GtkMenuItem *i, gpointer unused) {
    P.add_action(ACTION_BACK, 0);
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

const gchar* css_global = R"(
    .display-cell {
        border: none;
        box-shadow: none;
        border-radius: 0;
        background: #101010;
    }

    .display-cell:checked {
        background: #5fbd5a;
    }

    .code-editor {
        font-family: "Cascadia Mono Normal", "Cascadia Mono", "Ubuntu Mono Normal", "Ubuntu Mono", monospace, mono;
        font-size: 1.25em;
    }
)";

void load_gtk_css() {
    //Load global CSS
    {
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

    //Try to load debug.css in debug builds
    #ifdef DEBUG
    {
        GtkCssProvider* css_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_path (
            css_provider,
            "debug.css",
            NULL
        );
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    #endif
}

int _gtk_loop(void *p) {
    gtk_init(NULL, NULL);

    //Load CSS themes
    load_gtk_css();

    g_object_set(
        gtk_settings_get_default(),
        "gtk-application-prefer-dark-theme", true,
        "gtk-tooltip-timeout", 100,
        NULL
    );

    GtkDialog *dialog;

    /** --Menu **/

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

    /** --Digital display **/
    {
        digi_dialog = new_dialog_defaults("Display settings", &on_digi_show);

        gtk_widget_set_size_request(GTK_WIDGET(digi_dialog), 200, 450);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(digi_dialog));
        gtk_box_set_spacing(GTK_BOX(content), 7);

        {
            GtkBox *spin = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));
            digi_wrap = GTK_CHECK_BUTTON(gtk_check_button_new());
            gtk_box_pack_start(GTK_BOX(spin), new_lbl("<b>Wrap around</b>"), false, false, 0);
            gtk_box_pack_start(GTK_BOX(spin), GTK_WIDGET(digi_wrap), false, false, 0);
            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(spin), false, false, 0);
        }
        {
            GtkBox *spin = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));
            digi_initial = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 40, 1, 1, 0)),
                    1, 0));
            gtk_box_pack_start(GTK_BOX(spin), new_lbl("<b>Initial position</b>"), false, false, 0);
            gtk_box_pack_start(GTK_BOX(spin), GTK_WIDGET(digi_initial), false, false, 0);
            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(spin), false, false, 0);
        }

        {
            digi_label = GTK_LABEL(new_clbl("<b>Symbol 1/36</b>"));

            GtkGrid *tbl_symbol = GTK_GRID(gtk_grid_new());

            gtk_grid_set_row_homogeneous(tbl_symbol, true);
            gtk_grid_set_column_homogeneous(tbl_symbol, true);
            gtk_grid_set_row_spacing(tbl_symbol, 0);
            gtk_grid_set_column_spacing(tbl_symbol, 0);

            for (int y=0; y < 7; y++) {
                for (int x=0; x < 5; x++) {
                    //Create ToggleButton
                    GtkToggleButton* check = GTK_TOGGLE_BUTTON(gtk_toggle_button_new());
                    gtk_toggle_button_set_mode(check, false);
                    digi_check[y][x] = check;

                    //Add .display-cell class
                    GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(check));
                    gtk_style_context_add_class(context, "display-cell");

                    //Add to table
                    gtk_grid_attach(
                        tbl_symbol,
                        GTK_WIDGET(check),
                        x, y, 1, 1
                    );

                    //Connect toggled signal
                    g_signal_connect(
                        check, "toggled",
                        G_CALLBACK(on_digi_toggle),
                        (void*)(uintptr_t)((y * 5) + x)
                    );
                }
            }

            //Create aspect frame and put tbl_symbol into it, to ensure perfect aspect ratio
            GtkAspectFrame* aspect_frame = GTK_ASPECT_FRAME(gtk_aspect_frame_new(NULL, .5f, .5f, 5.f / 7.f, false));
            gtk_container_add(GTK_CONTAINER(aspect_frame), GTK_WIDGET(tbl_symbol));

            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(digi_label), false, false, 0);
            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(aspect_frame), true, true, 0);
        }

        {
            GtkBox *btns = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

            digi_prev = GTK_BUTTON(gtk_button_new_with_label("Previous"));
            gtk_box_pack_start(GTK_BOX(btns), GTK_WIDGET(digi_prev), false, false, 0);
            g_signal_connect(digi_prev, "clicked", G_CALLBACK(on_digi_prev_click), 0);

            digi_next = GTK_BUTTON(gtk_button_new_with_label("Next"));
            gtk_box_pack_start(GTK_BOX(btns), GTK_WIDGET(digi_next), false, false, 0);
            g_signal_connect(digi_next, "clicked", G_CALLBACK(on_digi_next_click), 0);
            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(btns), false, false, 0);

            btns = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));

            digi_insert = GTK_BUTTON(gtk_button_new_with_label("Insert before"));
            gtk_box_pack_start(GTK_BOX(btns), GTK_WIDGET(digi_insert), false, false, 0);
            g_signal_connect(digi_insert, "clicked", G_CALLBACK(on_digi_insert_click), 0);

            digi_append = GTK_BUTTON(gtk_button_new_with_label("Append"));
            gtk_box_pack_start(GTK_BOX(btns), GTK_WIDGET(digi_append), false, false, 0);
            g_signal_connect(digi_append, "clicked", G_CALLBACK(on_digi_append_click), 0);

            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(btns), false, false, 0);
            btns = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));

            digi_delete = GTK_BUTTON(gtk_button_new_with_label("Delete current symbol"));
            gtk_box_pack_start(GTK_BOX(btns), GTK_WIDGET(digi_delete), false, false, 0);
            g_signal_connect(digi_delete, "clicked", G_CALLBACK(on_digi_delete_click), 0);

            gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(btns), false, false, 0);
        }

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --SFX Emitter 2 **/
    {
        dialog = new_dialog_defaults("SFX Emitter", &on_sfx2_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        sfx2_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        for (int x=0; x<SND__NUM; x++) {
            gtk_combo_box_text_append_text(sfx2_cb, sm::sound_lookup[x]->name);
        }

        sfx2_sub_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());

        g_signal_connect(sfx2_cb, "changed", G_CALLBACK(on_sfx2_cb_changed), 0);

        sfx2_global = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Global sound"));
        sfx2_loop = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Loop"));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Sound</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx2_cb), false, false, 10);
        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Sound chunk</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx2_sub_cb), false, false, 10);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx2_global), false, false, 10);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx2_loop), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));

        sfx2_dialog = dialog;
    }

    /** --Factory **/
    {
        dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Factory",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                "_OK", GTK_RESPONSE_ACCEPT,
                NULL));
        factory_cancel = GTK_BUTTON(gtk_dialog_add_button(dialog, "_Cancel", GTK_RESPONSE_REJECT));

        apply_dialog_defaults(dialog);

        gtk_widget_set_size_request(GTK_WIDGET(dialog), 450, 300);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));
        GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
        GtkWidget *l;

        GtkGrid *tbl = GTK_GRID(gtk_grid_new());

        gtk_grid_set_row_spacing(tbl, 5);
        gtk_grid_set_column_spacing(tbl, 5);

        int x = 0;

        l = gtk_label_new("Oil");
        gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
        gtk_label_set_yalign(GTK_LABEL(l), 0.5f);
        factory_oil = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 65535, 1, 1, 0)),
                    1, 0));
        gtk_grid_attach(tbl, l, 0, x, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(factory_oil), 1, x, 1, 1);
        ++x;

        for (; x<NUM_RESOURCES+1; ++x) {
            GtkWidget *l = gtk_label_new(resource_data[x-1].name);
            gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
            gtk_label_set_yalign(GTK_LABEL(l), 0.5f);
            factory_resources[x-1] = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 65535, 1, 1, 0)),
                        1, 0));
            gtk_grid_attach(tbl, l, 0, x, 1, 1);
            gtk_grid_attach(tbl, GTK_WIDGET(factory_resources[x-1]), 1, x, 1, 1);
        }

        l = gtk_label_new("Faction");
        gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
        gtk_label_set_yalign(GTK_LABEL(l), 0.5f);
        factory_faction = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, NUM_FACTIONS-1, 1, 1, 0)),
                    1, 0));
        gtk_grid_attach(tbl, l, 0, x, 1, 1);
        gtk_grid_attach(tbl, GTK_WIDGET(factory_faction), 1, x, 1, 1);
        ++x;

        {
            /*                                        Included        Order        Name,         ID */
            factory_liststore = gtk_list_store_new(4, G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);
            GtkTreeModel *model = GTK_TREE_MODEL(factory_liststore);

            factory_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(model));

            GtkCellRenderer *renderer;
            GtkTreeViewColumn *column;
            model = gtk_tree_view_get_model(factory_treeview);

            renderer = gtk_cell_renderer_toggle_new();
            g_signal_connect(renderer, "toggled", G_CALLBACK(factory_enable_toggled), model);

            column = gtk_tree_view_column_new_with_attributes(
                "Enabled",
                renderer,
                "active", 0,
                NULL
            );
            gtk_tree_view_append_column(factory_treeview, column);

            renderer = gtk_cell_renderer_text_new();
            column = gtk_tree_view_column_new_with_attributes(
                "Index",
                renderer,
                "text",
                1,
                NULL
            );
            gtk_tree_view_column_set_sort_column_id(column, 1);
            gtk_tree_view_append_column(factory_treeview, column);

            renderer = gtk_cell_renderer_text_new();
            column = gtk_tree_view_column_new_with_attributes(
                "Recipe",
                renderer,
                "text",
                2,
                NULL
            );
            gtk_tree_view_column_set_sort_column_id(column, 2);
            gtk_tree_view_column_set_expand(column, true);
            gtk_tree_view_append_column(factory_treeview, column);
        }

        GtkWidget *sw = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                GTK_POLICY_AUTOMATIC,
                GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(factory_treeview));

        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(tbl), false, false, 0);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(sw), true, true, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(hbox), true, true, 0);
        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(dialog, "key-press-event", G_CALLBACK(on_factory_keypress), 0);
        g_signal_connect(dialog, "show", G_CALLBACK(on_factory_show), 0);

        factory_dialog = dialog;
    }

    /** --SFX Emitter dialog **/
    {
        sfx_dialog = new_dialog_defaults("SFX Emitter", &on_sfx_show);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(sfx_dialog));

        sfx_cb = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        for (int x=0; x<NUM_SFXEMITTER_OPTIONS; x++) {
            gtk_combo_box_text_append_text(sfx_cb, sfxemitter_options[x].name);
        }

        sfx_global = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Global sound"));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Sound</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx_cb), false, false, 10);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(sfx_global), false, false, 10);

        gtk_widget_show_all(GTK_WIDGET(content));
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

    /** --Synth **/
    {
        synth_dialog = new_dialog_defaults("Synthesizer", &on_synth_show, &on_synth_keypress);

        gtk_widget_set_size_request(GTK_WIDGET(synth_dialog), 350, -1);
        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(synth_dialog));

        GtkGrid *tbl_settings = create_settings_table();
        {
            int y = -1;

            synth_hz_low = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 440*8, 20, .1, 0)),
                        50, 0));

            synth_hz_high = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 440*8, 20, .1, 0)),
                        50, 0));

            synth_bitcrushing = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 64, 1));

            synth_pulse_width = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1., .01f));

            synth_freq_vibrato_hz = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 32, 1));
            synth_freq_vibrato_extent = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1., .01));

            synth_vol_vibrato_hz = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 32, 1));
            synth_vol_vibrato_extent = GTK_RANGE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 1, .01));

            synth_waveform = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());

            for (int x=0; x<NUM_WAVEFORMS; ++x) {
                gtk_combo_box_text_append_text(synth_waveform, speaker_options[x]);
            }

            add_setting_row(
                tbl_settings, ++y,
                "Base frequency",
                GTK_WIDGET(synth_hz_low)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Peak frequency",
                GTK_WIDGET(synth_hz_high)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Waveform",
                GTK_WIDGET(synth_waveform)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Pulse width",
                GTK_WIDGET(synth_pulse_width)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Bitcrushing",
                GTK_WIDGET(synth_bitcrushing)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Volume vibrato Hz",
                GTK_WIDGET(synth_vol_vibrato_hz)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Volume vibrato extent",
                GTK_WIDGET(synth_vol_vibrato_extent)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Freq vibrato Hz",
                GTK_WIDGET(synth_freq_vibrato_hz)
            );

            add_setting_row(
                tbl_settings, ++y,
                "Freq vibrato extent",
                GTK_WIDGET(synth_freq_vibrato_extent)
            );
        }

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(tbl_settings), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));
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

/** --Synthesizer **/
static gboolean _open_synth(gpointer unused) {
    gint result = gtk_dialog_run(synth_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_SYNTHESIZER) {
            float low = gtk_spin_button_get_value(synth_hz_low);
            float high = gtk_spin_button_get_value(synth_hz_high);
            float pw = gtk_range_get_value(synth_pulse_width);
            float vb = gtk_range_get_value(synth_vol_vibrato_hz);
            float fb = gtk_range_get_value(synth_freq_vibrato_hz);
            float vbe = gtk_range_get_value(synth_vol_vibrato_extent);
            float fbe = gtk_range_get_value(synth_freq_vibrato_extent);
            float bitcrushing = gtk_range_get_value(synth_bitcrushing);

            if (high < low) high = low;

            e->properties[0].v.f = low;
            e->properties[1].v.f = high;

            int index = gtk_combo_box_get_active(GTK_COMBO_BOX(synth_waveform));

            e->properties[2].v.i = index;

            e->properties[3].v.f = bitcrushing;
            e->properties[4].v.f = vb;
            e->properties[5].v.f = fb;

            e->properties[6].v.f = vbe;
            e->properties[7].v.f = fbe;

            e->properties[8].v.f = pw;
        }
    }

    gtk_widget_hide(GTK_WIDGET(synth_dialog));

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

static gboolean _open_digi_window(gpointer unused) {
    gint result = gtk_dialog_run(digi_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;
        if (e && (e->g_id == O_PASSIVE_DISPLAY || e->g_id == O_ACTIVE_DISPLAY)) {
            e->properties[0].v.i8 = (uint8_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(digi_wrap));
            e->properties[1].v.i8 = gtk_spin_button_get_value(digi_initial)-1;

            char str[DISPLAY_MAX_SYMBOLS*35+1];
            int ss=0;

            for (int s=0; s<num_digi_symbols; s++) {
                for (int y=0; y<7; y++) {
                    for (int x=0; x<5; x++) {
                        if (symbols[s] & (1ull << (uint64_t)(y*5+x)))
                            str[ss] = '1';
                        else
                            str[ss] = '0';
                        ss++;
                    }
                }
            }

            str[ss] = '\0';
            e->set_property(2, str);
            ((display*)e)->load_symbols();
        }
    }

    gtk_widget_hide(GTK_WIDGET(digi_dialog));

    return false;
}

static gboolean _open_sfx_window(gpointer unused) {
    gint result = gtk_dialog_run(sfx_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_SFX_EMITTER) {
            e->set_property(0, (uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sfx_cb)));
            e->set_property(1, (uint8_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sfx_global)));
        }
    }

    gtk_widget_hide(GTK_WIDGET(sfx_dialog));

    return false;
}

static gboolean _open_sfx2_window(gpointer unused) {
    gint result = gtk_dialog_run(sfx2_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && e->g_id == O_SFX_EMITTER) {
            e->set_property(0, (uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sfx2_cb)));
            e->set_property(1, (uint8_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sfx2_global)));
            e->set_property(3, (uint8_t)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sfx2_loop)));

            uint32_t active_sub = (uint32_t)gtk_combo_box_get_active(GTK_COMBO_BOX(sfx2_sub_cb));

            if (active_sub == 0) {
                e->properties[2].v.i = SFX_CHUNK_RANDOM;
            } else {
                e->properties[2].v.i = active_sub - 1;
            }
        }
    }

    gtk_widget_hide(GTK_WIDGET(sfx2_dialog));

    return false;
}

/** --Factory **/
static gboolean _open_factory(gpointer unused) {
    GtkDialog *d = factory_dialog;

    gint result = gtk_dialog_run(d);

    if (result == GTK_RESPONSE_ACCEPT) {
        entity *e = G->selection.e;

        if (e && IS_FACTORY(e->g_id)) {
            factory *f = static_cast<factory*>(e);

            gtk_spin_button_update(factory_oil);
            gtk_spin_button_update(factory_faction);

            f->properties[1].v.i = gtk_spin_button_get_value(factory_oil);
            f->properties[2].v.i = gtk_spin_button_get_value(factory_faction);
            for (int x=0; x<NUM_RESOURCES; ++x) {
                gtk_spin_button_update(factory_resources[x]);
                f->properties[FACTORY_NUM_EXTRA_PROPERTIES+x].v.i = gtk_spin_button_get_value(factory_resources[x]);
            }

            GtkTreeModel *model = GTK_TREE_MODEL(factory_liststore);
            GtkTreeIter iter;
            int x = 0;
            std::stringstream ss;

            if (gtk_tree_model_get_iter_first(model, &iter)) {
                do {
                    GValue val = {0, };
                    GValue val_id = {0, };
                    gtk_tree_model_get_value(model, &iter, FACTORY_COLUMN_ENABLED, &val);
                    gtk_tree_model_get_value(model, &iter, FACTORY_COLUMN_RECIPE_ID, &val_id);
                    gboolean enabled = g_value_get_boolean(&val);
                    gint id = g_value_get_int(&val_id);
                    if (enabled == TRUE) {
                        if (x != 0) {
                            ss << ';';
                        }

                        ss << id;

                        ++ x;
                    }
                } while (gtk_tree_model_iter_next(model, &iter));
            }

            f->set_property(0, ss.str().c_str());
            tms_debugf("Recipe string: %s", f->properties[0].v.s.buf);

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }

    gtk_widget_hide(GTK_WIDGET(d));

    return false;
}

static gboolean _close_all_dialogs(gpointer unused) {
    gtk_widget_hide(GTK_WIDGET(open_state_window));
    gtk_widget_hide(GTK_WIDGET(object_window));
    gtk_widget_hide(GTK_WIDGET(synth_dialog));
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
#ifndef PRINCIPIA_BACKEND_IMGUI
        case DIALOG_DIGITALDISPLAY: gdk_threads_add_idle(_open_digi_window, 0); break;
#endif
        case DIALOG_FXEMITTER:
            UiFXEmitter::open();
            break;
        case DIALOG_EVENTLISTENER:
            UiEventListener::open();
            break;
#ifndef PRINCIPIA_BACKEND_IMGUI
        case DIALOG_SFXEMITTER:     gdk_threads_add_idle(_open_sfx_window, 0); break;
        case DIALOG_SFXEMITTER_2:   gdk_threads_add_idle(_open_sfx2_window, 0); break;
#endif
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
#ifdef PRINCIPIA_BACKEND_IMGUI
            UiSynthesizer::open();
#else
            gdk_threads_add_idle(_open_synth, 0);
#endif
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
#ifndef PRINCIPIA_BACKEND_IMGUI
        case DIALOG_FACTORY:        gdk_threads_add_idle(_open_factory, 0); break;
#endif
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

    /* XXX this stuff probably needs to be added to gdk_threads_idle_add()! */

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

#ifdef PRINCIPIA_BACKEND_IMGUI
    UiSynthesizer::layout();
#endif

    imgui_driver.post_render();
}

#endif
