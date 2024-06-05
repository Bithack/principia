
#ifdef TMS_BACKEND_IOS

extern "C" {
/* these functions are defined in ios-ui.m */
void ui_message(const char *msg, bool long_duration);
void ui_init();
void ui_open_dialog(int num);
void ui_open_url(const char *url);
void ui_open_help_dialog(const char *title, const char *description);
void ui_emit_signal(int num);

    void ui_cb_update_jumper()
    {
        ((jumper*)G->selection.e)->update_color();
    }
    int ui_cb_get_hide_tips()
    {
        return settings["hide_tips"]->v.b ? 1 : 0;
    }
    void ui_cb_set_hide_tips(int h)
    {
        settings["hide_tips"]->v.b = h ? true : false;
    }

    /* XXX GID XXX */
    uint8_t ui_get_entity_gid()
    {
        return G->selection.e->g_id;
    }

    const char *ui_cb_get_tip()
    {
        if (ctip == -1) ctip = rand()%num_tips;

        ctip = (ctip+1)%num_tips;
        return tips[ctip];
    }

void ui_cb_set_color(float r, float g, float b, float a)
{
    if (!G->selection.e && W->is_adventure() && adventure::player && adventure::is_player_alive()) {
        robot_parts::tool *t = adventure::player->get_tool();
        if (t && t->tool_id == TOOL_PAINTER) {
            t->set_property(0, r);
            t->set_property(1, g);
            t->set_property(2, b);
            ((robot_parts::painter*)t)->update_appearance();
        }
    } else {
        ui_set_property_float(1, r);
        ui_set_property_float(2, g);
        ui_set_property_float(3, b);

        if (ui_get_entity_gid() == O_PIXEL) {
            /* pixel */
            ui_set_property_uint8(4, (uint8_t)roundf(a * 255.f));
            ((pixel*)G->selection.e)->update_appearance();
        } else if (ui_get_entity_gid() == O_PLASTIC_BOX) {
            ((box*)G->selection.e)->update_appearance();
        } else if (ui_get_entity_gid() == O_PLASTIC_BEAM) {
            ((beam*)G->selection.e)->update_appearance();
        }
    }
}

void ui_cb_update_rubber()
{
    entity *e = G->selection.e;
    if (!e) return;
    if (e->g_id == O_RUBBER_BEAM) {
        ((beam*)e)->do_update_fixture = true;
    } else {
        ((wheel*)e)->do_update_fixture = true;
    }

    P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
    P.add_action(ACTION_RESELECT, 0);
}

void ui_cb_set_locked(int v)
{
    W->level.visibility = v ? LEVEL_LOCKED : LEVEL_VISIBLE;
}

int ui_cb_get_locked()
{
    return W->level.visibility == LEVEL_LOCKED;
}

uint8_t ui_get_property_uint8(int index)
{
    return G->selection.e->properties[index].v.u8;
}

void ui_set_property_uint8(int index, uint8_t val)
{
    G->selection.e->properties[index].v.u8 = val;
}

uint32_t ui_get_property_uint32(int index)
{
    return G->selection.e->properties[index].v.i;
}

void ui_set_property_uint32(int index, uint32_t val)
{
    G->selection.e->properties[index].v.i = val;
}

float ui_get_property_float(int index)
{
    return G->selection.e->properties[index].v.f;
}

void ui_set_property_float(int index, float val)
{
    G->selection.e->properties[index].v.f = val;
}

    const char* ui_get_property_string(int index)
    {
        return G->selection.e->properties[index].v.s.buf;
    }

    void ui_set_property_string(int index, const char* val)
    {
        G->selection.e->set_property(index, val);
    }
    int ui_settings_get_enable_border_scrolling()
    {
        return settings["border_scroll_enabled"]->v.b;
    }

    float ui_settings_get_border_scrolling_speed()
    {
        return settings["border_scroll_speed"]->v.f;
    }

    int ui_settings_get_enable_object_ids()
    {
        return settings["display_object_id"]->v.b;

    }
    void ui_settings_set_enable_border_scrolling(int s)
    {
        settings["border_scroll_enabled"]->v.b = s;
    }

    void ui_settings_set_border_scrolling_speed(float s)
    {
        settings["border_scroll_speed"]->v.f = s;
    }

    void ui_settings_set_enable_object_ids(int s)
    {
        settings["display_object_id"]->v.b = s;

    }
    int ui_settings_get_shadow_resx()
    {
        return settings["shadow_map_resx"]->v.i;
    }
    int ui_settings_get_shadow_resy()
    {
        return settings["shadow_map_resy"]->v.i;
    }
    int ui_settings_get_enable_shadows()
    {
        return settings["enable_shadows"]->v.b;
    }
    int ui_settings_get_shadow_quality()
    {
        return settings["shadow_quality"]->v.i;
    }
    int ui_settings_get_ao_quality()
    {
        return settings["ao_map_res"]->v.i;
    }
    int ui_settings_get_enable_ao()
    {
        return settings["enable_ao"]->v.b;
    }

    float ui_settings_get_ui_scale()
    {
        return settings["uiscale"]->v.f;
    }
    float ui_settings_get_cam_speed()
    {
        return settings["cam_speed_modifier"]->v.f;
    }
    float ui_settings_get_zoom_speed()
    {
        return settings["zoom_speed"]->v.f;
    }
    int ui_settings_get_enable_smooth_cam()
    {
        return settings["smooth_cam"]->v.b;
    }
    int ui_settings_get_enable_smooth_zoom()
    {
        return settings["smooth_zoom"]->v.b;
    }

    void ui_settings_set_shadow_resx(int v)
    {
        settings["shadow_map_resx"]->v.i = v;
    }
    void ui_settings_set_shadow_resy(int v)
    {
        settings["shadow_map_resy"]->v.i = v;
    }
    void ui_settings_set_enable_shadows(int e)
    {
        settings["enable_shadows"]->v.b = e;
    }
    void ui_settings_set_shadow_quality(int v)
    {
        settings["shadow_quality"]->v.i =v;
    }
    void ui_settings_set_ao_quality(int v)
    {
        settings["ao_map_res"]->v.i = v;
    }
    void ui_settings_set_enable_ao(int v)
    {
        settings["enable_ao"]->v.b = v;
    }

    void ui_settings_set_ui_scale(float v)
    {
        settings["uiscale"]->v.f = v;
    }
    void ui_settings_set_cam_speed(float v)
    {
        settings["cam_speed_modifier"]->v.f = v;
    }
    void ui_settings_set_zoom_speed(float v)
    {
        settings["zoom_speed"]->v.f = v;
    }
    void ui_settings_set_enable_smooth_cam(int v)
    {
        settings["smooth_cam"]->v.b = v;
    }
    void ui_settings_set_enable_smooth_zoom(int v)
    {
        settings["smooth_zoom"]->v.b = v;
    }
    void ui_settings_save()
    {
        settings.save();
    }
    void P_set_can_reload_graphics(int v)
    {
        P.can_reload_graphics = (v?true:false);
    }
    void P_set_can_set_settings(int v)
    {
        P.can_set_settings = (v?true:false);
    }
    int P_get_can_reload_graphics(void)
    {
        return P.can_reload_graphics;
    }
    int P_get_can_set_settings(void)
    {
        return P.can_set_settings;
    }

/* callback functions from the objective-c shitbucket */
void ui_cb_menu_item_selected(int n)
{
    switch (n) {
        case 0: ui::open_dialog(DIALOG_LEVEL_PROPERTIES); break;
        case 1: ui::open_dialog(DIALOG_NEW_LEVEL); break;
        case 2: ui::open_dialog(DIALOG_SAVE); break;
        case 3: ui::open_dialog(DIALOG_SAVE_COPY); break;
        case 4: ui::open_dialog(DIALOG_OPEN); break;
        case 5: ui::open_dialog(DIALOG_PUBLISH); break;
        case 6: ui::open_dialog(DIALOG_SETTINGS); break;
        case 7: G->back(); break;
    }
}

void ui_cb_set_robot_dir(int dir)
{
    robot_base *r = (robot_base*)G->selection.e;

    switch (dir) {
        case 0:ui_set_property_uint8(4, 1);r->set_i_dir(DIR_LEFT);break;
        case 1:ui_set_property_uint8(4, 0);r->set_i_dir(0.f);break;
        case 2:ui_set_property_uint8(4, 2);r->set_i_dir(DIR_RIGHT);break;
    }
}

void ui_cb_reset_variable(const char *var)
{
    std::map<std::string, float>::size_type num_deleted = W->level_variables.erase(var);
    if (num_deleted != 0) {
        if (W->save_cache(W->level_id_type, W->level.local_id)) {
            ui::message("Successfully deleted data for this variable");
        } else {
            ui::message("Unable to delete variable data for this level.");
        }
    } else {
        ui::message("No data found for this variable");
    }

}
void ui_cb_reset_all_variables()
{
    W->level_variables.clear();
    if (W->save_cache(W->level_id_type, W->level.local_id)) {
        ui::message("All level-specific variables cleared.");
    } else {
        ui::message("Unable to delete variable data for this level.");
    }
}

void ui_cb_set_level_type(int type){/*W->set_level.type(type);*/P.add_action(ACTION_SET_LEVEL_TYPE, (void*)type);};
int ui_cb_get_level_type(){return W->level.type;};
char *ui_cb_get_level_title(){char *fuckxcode = (char*)malloc(W->level.name_len+1); memcpy(fuckxcode, W->level.name, W->level.name_len); fuckxcode[W->level.name_len] = '\0'; return fuckxcode;};
const char *ui_cb_get_level_description(){return W->level.descr != 0 ? W->level.descr : "";};

    float ui_cb_get_level_prism_tolerance(void)
    {
        return W->level.prismatic_tolerance;
    }
    void ui_cb_set_level_prism_tolerance(float s)
    {
        W->level.prismatic_tolerance = s;
    }

    float ui_cb_get_level_pivot_tolerance(void)
    {
        return W->level.pivot_tolerance;
    }
    void ui_cb_set_level_pivot_tolerance(float s)
    {
        W->level.pivot_tolerance = s;
    }

    uint8_t ui_cb_get_level_pos_iter(void)
    {
        return W->level.position_iterations;
    }
    void ui_cb_set_level_pos_iter(uint8_t s)
    {
        W->level.position_iterations = s;
    }
    uint8_t ui_cb_get_level_vel_iter(void)
    {
        return W->level.velocity_iterations;
    }
    void ui_cb_set_level_vel_iter(uint8_t s)
    {
        W->level.velocity_iterations = s;
    }
    uint32_t ui_cb_get_level_final_score(void)
    {
        return W->level.final_score;
    }
    void ui_cb_set_level_final_score(uint32_t s)
    {
        W->level.final_score = s;
    }
    void ui_cb_set_level_gravity_x(float v)
    {
        if (v>MAX_GRAVITY) v=MAX_GRAVITY;
        if (v<-MAX_GRAVITY) v=-MAX_GRAVITY;
        W->level.gravity_x = v;
    }
    float ui_cb_get_level_gravity_x()
    {
        return W->level.gravity_x;
    }
    void ui_cb_set_level_gravity_y(float v)
    {
        if (v>MAX_GRAVITY) v=MAX_GRAVITY;
        if (v<-MAX_GRAVITY) v=-MAX_GRAVITY;
        W->level.gravity_y = v;
    }
    float ui_cb_get_level_gravity_y()
    {
        return W->level.gravity_y;
    }
    void ui_cb_set_level_bg(uint8_t bg){W->level.bg = bg;};
    uint8_t ui_cb_get_level_bg(){return W->level.bg;};

    uint16_t ui_cb_get_level_border(int d) {
        switch (d) {
            case 0: return W->level.size_x[1];
            case 1: return W->level.size_y[1];
            case 2: return W->level.size_x[0];
            case 3: return W->level.size_y[0];
        }
        return 0;
    }
    void ui_cb_set_level_border(int d, uint16_t w) {
        switch (d) {
            case 0: W->level.size_x[1] = w; break;
            case 1: W->level.size_y[1] = w; break;
            case 2: W->level.size_x[0] = w; break;
            case 3: W->level.size_y[0] = w; break;
        }
    }
void ui_cb_set_level_title(const char *s){int len = strlen(s); if(len > 255) len = 255; memcpy(W->level.name, s, len); W->level.name_len = len;tms_infof("set level title: %s", s);};
void ui_cb_set_level_description(const char*s){if (W->level.descr) free(W->level.descr); size_t len = strlen(s); W->level.descr = (char*)malloc(len+1); memcpy(W->level.descr, s, len); W->level.descr[len]='\0';tms_infof("set level description: %s", W->level.descr);};
int ui_cb_get_pause_on_win(){return W->level.pause_on_finish;};
int ui_cb_get_display_score(){return W->level.show_score;};
void ui_cb_set_pause_on_win(int v){W->level.pause_on_finish = v?true:false;};
void ui_cb_set_display_score(int v){W->level.show_score = v?true:false;};
int ui_cb_get_level_flag(uint64_t f){return W->level.flag_active(f);};
void ui_cb_set_level_flag(uint64_t f, int v){if (v) W->level.flags |= f; else W->level.flags &= ~f;};

int ui_cb_get_followmode(){
    return G->selection.e->properties[1].v.i;
};
void ui_cb_set_followmode(int n)
{
    if (G->selection.e && G->selection.e->g_id == 133)
        G->selection.e->properties[1].v.i = n;

    P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
}

int ui_cb_get_command(){return((command*)G->selection.e)->get_command();};
void ui_cb_set_command(int n)
{
    if (G->selection.e && G->selection.e->g_id == 64) {
        ((command*)G->selection.e)->set_command(n);

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}
    void ui_cb_set_consumable(int n)
    {
        if (G->selection.e && G->selection.e->g_id == O_ITEM) {
            ((item*)G->selection.e)->set_item_type(n);
            ((item*)G->selection.e)->do_recreate_shape = true;

            P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P.add_action(ACTION_RESELECT, 0);
        }
    }
int ui_cb_get_event(){return G->selection.e->properties[0].v.i;};
void ui_cb_set_event(int n)
{
    if (G->selection.e && G->selection.e->g_id == 156) {
        G->selection.e->properties[0].v.i = n;

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

void
ui_cb_back_to_community(void)
{
    COMMUNITY_URL("level/%d", W->level.community_id);

    ui::open_url(tmp);
}


void ui_cb_refresh_sequencer()
{
    ((sequencer*)G->selection.e)->refresh_sequence();
}

int ui_cb_get_fx(int n){return G->selection.e->properties[3+n].v.i == FX_INVALID ? 0 : G->selection.e->properties[3+n].v.i+1;};
void ui_cb_set_fx(int n, int fx){
    G->selection.e->properties[3+n].v.i = (fx == 0 ? FX_INVALID : fx - 1);

    P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
    P.add_action(ACTION_RESELECT, 0);
}
};

void
ui_cb_prompt_response(int r)
{
    if (G->current_prompt) {
        G->current_prompt->set_response((uint8_t)r);
    }
}

const char *_prompt_buttons[3] = {0,0,0};
const char *_prompt_text = 0;

struct ios_menu_object *g_all_menu_objects = 0;
int g_num_menu_objects = 0;

/* forward calls to objective-c versions (ios-ui.m) */
void ui::message(const char *msg, bool long_duration){ui_message(msg, long_duration);};
void
ui::messagef(const char *format, ...)
{
    va_list vl;
    va_start(vl, format);

    char short_msg[256];
    const size_t sz = vsnprintf(short_msg, sizeof short_msg, format, vl) + 1;
    if (sz <= sizeof short_msg) {
        ui::message(short_msg, false);
    } else {
        char *long_msg = (char*)malloc(sz);
        vsnprintf(long_msg, sz, format, vl);
        ui::message(long_msg, false);
    }
}
void ui::init(){ui_init();};

#include <cstdlib>

int menusort(const void* a, const void* b)
{
    struct ios_menu_object *ma = (struct ios_menu_object*)a;
    struct ios_menu_object *mb = (struct ios_menu_object*)b;

    return strcasecmp(ma->name, mb->name);
}

void
ui::open_dialog(int num, void *data/*=0*/)
{
    if (num == DIALOG_QUICKADD) {
        if (g_all_menu_objects == 0) {
            g_all_menu_objects = (struct ios_menu_object*)malloc(sizeof(struct ios_menu_object)*menu_objects.size());
            g_num_menu_objects = menu_objects.size();

            for (int x=0; x<g_num_menu_objects; x++) {
                g_all_menu_objects[x].name = menu_objects[x].e->get_name();
                g_all_menu_objects[x].g_id = menu_objects[x].e->g_id;
            }

            qsort((void*)g_all_menu_objects, (size_t)g_num_menu_objects, sizeof(struct ios_menu_object), menusort);
        }
    }

    if (num == DIALOG_PROMPT) {
        P.focused = false;
        _prompt_buttons[0] = G->current_prompt->properties[0].v.s.buf;
        _prompt_buttons[1] = G->current_prompt->properties[1].v.s.buf;
        _prompt_buttons[2] = G->current_prompt->properties[2].v.s.buf;
        _prompt_text = G->current_prompt->properties[3].v.s.buf;
    }
    ui_open_dialog(num);
};
void
ui::confirm(const char *text, const char *button1, int action1, const char *button2, int action2)
{
    /* TODO FIXME: implement this */
}
void
ui::alert(const char *text, uint8_t alert_type/*=ALERT_INFORMATION*/)
{
    /* TODO FIXME: implement this */
}
void ui::open_url(const char *url){ui_open_url(url);};
void ui::open_sandbox_tips(){ui_open_sandbox_tips();};
void ui::open_help_dialog(const char *title, const char *description){ui_open_help_dialog(title,description);};
void ui::emit_signal(int num, void *data/*=0*/){ui_emit_signal(num);};
void ui::set_next_action(int action_id){/*TODO FIXME: implement this*/};
void ui::quit(){exit(0);};
void ui::open_error_dialog(const char *error_msg){
    ui_open_error_dialog(error_msg);
};

#endif
