
#if defined(TMS_BACKEND_ANDROID)

#include "SDL.h"
#include "network.hh"
#include <jni.h>
#include <sstream>

void ui::init(){};

void
ui::set_next_action(int action_id)
{
    ui::next_action = action_id;
}

/* TODO: handle this in some way */
void ui::emit_signal(int signal_id, void *data/*=0*/)
{
    switch (signal_id) {
        case SIGNAL_LOGIN_SUCCESS:
            P.add_action(ui::next_action, 0);
            break;

        case SIGNAL_LOGIN_FAILED:
            /* XXX */
            break;

        case SIGNAL_REGISTER_SUCCESS:
            ui::open_dialog(CLOSE_REGISTER_DIALOG);
            tms_infof("Register success!!!!!!!!!");
            break;

        case SIGNAL_REGISTER_FAILED:
            ui::open_dialog(DISABLE_REGISTER_LOADER);
            tms_infof("Register failed!!!!!!!!!");
            break;

        case SIGNAL_REFRESH_BORDERS:
            /* XXX */
            break;

        default:
            {
                /* By default, passthrough the signal to the Java part */
                JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
                jobject activity = (jobject)SDL_AndroidGetActivity();
                jclass cls = env->GetObjectClass(activity);

                jmethodID mid = env->GetStaticMethodID(cls, "emit_signal", "(I)V");

                if (mid) {
                    env->CallStaticVoidMethod(cls, mid, (jvalue*)(jint)signal_id);
                }
            }
            break;
    }

    ui::next_action = ACTION_IGNORE;
}

void ui::open_url(const char *url)
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls = env->GetObjectClass(activity);

    jmethodID mid = env->GetStaticMethodID(cls, "open_url", "(Ljava/lang/String;)V");

    if (mid) {
        jstring str = env->NewStringUTF(url);
        env->CallStaticVoidMethod(cls, mid, (jvalue*)str);
    }
}

void
ui::confirm(const char *text,
        const char *button1, principia_action action1,
        const char *button2, principia_action action2,
        const char *button3/*=0*/, principia_action action3/*=ACTION_IGNORE*/,
        struct confirm_data _confirm_data/*=none*/
        )
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls = env->GetObjectClass(activity);

    jmethodID mid = env->GetStaticMethodID(cls, "confirm", "(Ljava/lang/String;Ljava/lang/String;IJLjava/lang/String;IJLjava/lang/String;IJZ)V");

    if (mid) {
        jstring _text = env->NewStringUTF(text);
        jstring _button1 = env->NewStringUTF(button1);
        jstring _button2 = env->NewStringUTF(button2);
        jstring _button3 = env->NewStringUTF(button3 ? button3 : "");
        env->CallStaticVoidMethod(cls, mid,
                _text,
                _button1, (jint)action1.action_id, (jlong)action1.action_data,
                _button2, (jint)action2.action_id, (jlong)action2.action_data,
                _button3, (jint)action3.action_id, (jlong)action3.action_data,
                (jboolean)_confirm_data.confirm_type == CONFIRM_TYPE_BACK_SANDBOX);
    } else {
        tms_errorf("Unable to run confirm");
    }
}

void
ui::alert(const char *text, uint8_t alert_type/*=ALERT_INFORMATION*/)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Principia", text, NULL);
}

void
ui::open_error_dialog(const char *error_msg)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_msg, NULL);
}

void
ui::open_dialog(int num, void *data/*=0*/)
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls = env->GetObjectClass(activity);

    jmethodID mid = env->GetStaticMethodID(cls, "open_dialog", "(IZ)V");

    if (mid) {
        env->CallStaticVoidMethod(cls, mid, (jvalue*)(jint)num, (jboolean)(data ? true : false));
    }
}

void
ui::quit()
{
    _tms.state = TMS_STATE_QUITTING;
}

void ui::open_help_dialog(const char *title, const char *description)
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls = env->GetObjectClass(activity);

    jmethodID mid = env->GetStaticMethodID(cls, "showHelpDialog", "(Ljava/lang/String;Ljava/lang/String;)V");

    if (mid) {
        jstring t = env->NewStringUTF(title);
        jstring d = env->NewStringUTF(description);
        env->CallStaticVoidMethod(cls, mid, (jvalue*)t, (jvalue*)d);
    } else
        tms_errorf("could not run showHelpDialog");
}

void
ui::open_sandbox_tips()
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls = env->GetObjectClass(activity);

    jmethodID mid = env->GetStaticMethodID(cls, "showSandboxTips", "()V");

    if (mid) {
        env->CallStaticVoidMethod(cls, mid, 0);
    } else
        tms_errorf("could not run showSandboxTips");
}

/** ++Generic **/

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getLevelPage(JNIEnv *env, jclass jcls)
{
    COMMUNITY_URL("level/%d", W->level.community_id);

    return env->NewStringUTF(url);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getCommunityHost(JNIEnv *env, jclass jcls)
{
    return env->NewStringUTF(P.community_host);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getCookies(JNIEnv *env, jclass jcls)
{
    char *token;
    P_get_cookie_data(&token);

    return env->NewStringUTF(token);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_addAction(JNIEnv *env, jclass jcls,
        jint action_id, jstring action_string)
{
    SDL_LockMutex(P.action_mutex);
    if (P.num_actions < MAX_ACTIONS) {
        P.actions[P.num_actions].id = (int)action_id;

        const char *str = env->GetStringUTFChars(action_string, 0);
        P.actions[P.num_actions].id = (int)action_id;
        P.actions[P.num_actions].data = INT_TO_VOID(atoi(str));
        P.num_actions ++;

        env->ReleaseStringUTFChars(action_string, str);
    }
    SDL_UnlockMutex(P.action_mutex);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_addActionAsInt(JNIEnv *env, jclass jcls,
        jint action_id, jlong action_data)
{
    uint32_t d = (uint32_t)((int64_t)action_data);
    P.add_action(action_id, d);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_addActionAsVec4(JNIEnv *env, jclass jcls,
        jint action_id, jfloat r, jfloat g, jfloat b, jfloat a)
{
    tvec4 *vec = (tvec4*)malloc(sizeof(tvec4));
    vec->r = r;
    vec->g = g;
    vec->b = b;
    vec->a = a;
    P.add_action(action_id, (void*)vec);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_addActionAsPair(JNIEnv *env, jclass jcls,
        jint action_id, jlong data0, jlong data1)
{
    uint32_t *vec = (uint32_t*)malloc(sizeof(uint32_t)*2);
    vec[0] = data0;
    vec[1] = data1;
    P.add_action(action_id, (void*)vec);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_addActionAsTriple(JNIEnv *env, jclass jcls,
        jint action_id, jlong data0, jlong data1, jlong data2)
{
    uint32_t *vec = (uint32_t*)malloc(sizeof(uint32_t)*3);
    vec[0] = data0;
    vec[1] = data1;
    vec[2] = data2;
    P.add_action(action_id, (void*)vec);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_openState(JNIEnv *env, jclass jcls,
        jint level_type, jint local_id, jint save_id, jboolean from_menu)
{
    uint32_t *info = (uint32_t*)malloc(sizeof(uint32_t)*3);
    info[0] = level_type;
    info[1] = local_id;
    info[2] = save_id;

    if (from_menu) {
        G->state.test_playing = false;
        G->screen_back = P.s_menu_play;
    }

    P.add_action(ACTION_OPEN_STATE, info);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setMultiemitterObject(JNIEnv *env, jclass jcls,
        jlong level_id)
{
    P.add_action(ACTION_MULTIEMITTER_SET, (uint32_t)level_id);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setImportObject(JNIEnv *env, jclass jcls,
        jlong level_id)
{
    P.add_action(ACTION_SELECT_IMPORT_OBJECT, (uint32_t)level_id);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getPropertyString(JNIEnv *env, jclass _jcls, jint property_index)
{
    char *nm = 0;
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_STR) {
        nm = e->properties[property_index].v.s.buf;
    }

    if (nm == 0) {
        return env->NewStringUTF("");
    }

    return env->NewStringUTF(nm);
}

extern "C" jlong
Java_org_libsdl_app_PrincipiaBackend_getPropertyInt(JNIEnv *env, jclass _jcls, jint property_index)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_INT) {
        return (jlong)e->properties[property_index].v.i;
    }

    return 0;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getPropertyInt8(JNIEnv *env, jclass _jcls, jint property_index)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_INT8) {
        return (jint)e->properties[property_index].v.i8;
    }

    return 0;
}

extern "C" jfloat
Java_org_libsdl_app_PrincipiaBackend_getPropertyFloat(JNIEnv *env, jclass _jcls, jint property_index)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_FLT) {
        return (jfloat)G->selection.e->properties[property_index].v.f;
    }

    return 0.f;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPropertyString(JNIEnv *env, jclass _jcls,
        jint property_index, jstring value)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_STR) {
        const char *tmp = env->GetStringUTFChars(value, 0);
        e->set_property(property_index, tmp);
        env->ReleaseStringUTFChars(value, tmp);
    } else {
        tms_errorf("Invalid set_property string");
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPropertyInt(JNIEnv *env, jclass _jcls,
        jint property_index, jlong value)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_INT) {
        e->properties[property_index].v.i = (uint32_t)value;
    } else {
        tms_errorf("Invalid set_property int");
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPropertyInt8(JNIEnv *env, jclass _jcls,
        jint property_index, jint value)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_INT8) {
        e->properties[property_index].v.i8 = (uint8_t)value;
    } else {
        tms_errorf("Invalid set_property int8");
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPropertyFloat(JNIEnv *env, jclass _jcls,
        jint property_index, jfloat value)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_FLT) {
        e->properties[property_index].v.f = (float)value;
    } else {
        tms_errorf("Invalid set_property float");
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_createObject(JNIEnv *env, jclass _jcls,
        jstring _name)
{
    const char *name = env->GetStringUTFChars(_name, 0);
    /* there seems to be absolutely no way of retrieving the top completion entry...
     * we have to find it manually */

    int len = strlen(name);
    uint32_t gid = 0;
    entity *found = 0;

    for (int x=0; x<menu_objects.size(); x++) {
        if (strncasecmp(name, menu_objects[x].e->get_name(), len) == 0) {
            found = menu_objects[x].e;
            break;
        }
    }

    if (found) {
        uint32_t g_id = found->g_id;
        P.add_action(ACTION_CONSTRUCT_ENTITY, g_id);
    } else
        tms_infof("'%s' matched no entity name", name);

    env->ReleaseStringUTFChars(_name, name);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getObjects(JNIEnv *env, jclass _jcls)
{
    std::stringstream b("", std::ios_base::app | std::ios_base::out);

    tms_infof("menu_objects size: %d", (int)menu_objects.size());
    for (int x=0; x<menu_objects.size(); x++) {
        const char *n = menu_objects[x].e->get_name();
        if (x != 0) b << ',';
        b << n;
    }

    tms_infof("got objects: '%s'", b.str().c_str());

    jstring str;
    str = env->NewStringUTF(b.str().c_str());
    return str;
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getSandboxTip(JNIEnv *env, jclass _jcls)
{
    jstring str;
    char *nm = 0;

    if (ctip == -1) ctip = rand()%num_tips;

    str = env->NewStringUTF(tips[ctip]);

    ctip = (ctip+1)%num_tips;

    return str;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_updateRubberEntity(JNIEnv *env, jclass _jcls,
        jfloat restitution, jfloat friction)
{
    entity *e = G->selection.e;

    if (e && (e->g_id == O_WHEEL || e->g_id == O_RUBBER_BEAM)) {
        e->properties[1].v.f = restitution;
        e->properties[2].v.f = friction;

        if (e->g_id == O_RUBBER_BEAM) {
            ((beam*)e)->do_update_fixture = true;
        } else {
            ((wheel*)e)->do_update_fixture = true;
        }

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_updateShapeExtruder(JNIEnv *env, jclass _jcls,
        jfloat right, jfloat up, jfloat left, jfloat down)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_SHAPE_EXTRUDER) {
        e->properties[0].v.f = (float)right;
        e->properties[1].v.f = (float)up;
        e->properties[2].v.f = (float)left;
        e->properties[3].v.f = (float)down;

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_updateJumper(JNIEnv *env, jclass _jcls,
        jfloat value)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_JUMPER) {
        float v = (float)value;
        if (v < 0.f) v = 0.f;
        else if (v > 1.f) v = 1.f;
        e->properties[0].v.f = v;

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
        ((jumper*)e)->update_color();
    }
}

extern "C" jobject
Java_org_libsdl_app_PrincipiaBackend_getSettings(JNIEnv *env, jclass _jcls)
{
    jobject ret = 0;
    jclass cls = 0;
    jmethodID constructor;

    cls = env->FindClass("com/bithack/principia/shared/Settings");

    if (cls) {
        constructor = env->GetMethodID(cls, "<init>", "()V");
        if (constructor) {
            ret = env->NewObject(cls, constructor);

            if (ret) {
                jfieldID f;

                f = env->GetFieldID(cls, "enable_shadows", "Z");
                env->SetBooleanField(ret, f, settings["enable_shadows"]->v.b);

                f = env->GetFieldID(cls, "shadow_quality", "I");
                env->SetIntField(ret, f, settings["shadow_quality"]->v.i);

                f = env->GetFieldID(cls, "shadow_map_resx", "I");
                env->SetIntField(ret, f, settings["shadow_map_resx"]->v.i);

                f = env->GetFieldID(cls, "shadow_map_resy", "I");
                env->SetIntField(ret, f, settings["shadow_map_resy"]->v.i);

                f = env->GetFieldID(cls, "ao_map_res", "I");
                env->SetIntField(ret, f, settings["ao_map_res"]->v.i);

                f = env->GetFieldID(cls, "enable_ao", "Z");
                env->SetBooleanField(ret, f, settings["enable_ao"]->v.b);

                f = env->GetFieldID(cls, "uiscale", "F");
                env->SetFloatField(ret, f, settings["uiscale"]->v.f);

                f = env->GetFieldID(cls, "cam_speed", "F");
                env->SetFloatField(ret, f, settings["cam_speed_modifier"]->v.f);

                f = env->GetFieldID(cls, "zoom_speed", "F");
                env->SetFloatField(ret, f, settings["zoom_speed"]->v.f);

                f = env->GetFieldID(cls, "smooth_cam", "Z");
                env->SetBooleanField(ret, f, settings["smooth_cam"]->v.b);

                f = env->GetFieldID(cls, "smooth_zoom", "Z");
                env->SetBooleanField(ret, f, settings["smooth_zoom"]->v.b);

                f = env->GetFieldID(cls, "border_scroll_enabled", "Z");
                env->SetBooleanField(ret, f, settings["border_scroll_enabled"]->v.b);

                f = env->GetFieldID(cls, "border_scroll_speed", "F");
                env->SetFloatField(ret, f, settings["border_scroll_speed"]->v.f);

                f = env->GetFieldID(cls, "display_object_ids", "Z");
                env->SetBooleanField(ret, f, settings["display_object_id"]->v.b);

                f = env->GetFieldID(cls, "display_grapher_value", "Z");
                env->SetBooleanField(ret, f, settings["display_grapher_value"]->v.b);

                f = env->GetFieldID(cls, "display_wireless_frequency", "Z");
                env->SetBooleanField(ret, f, settings["display_wireless_frequency"]->v.b);

                f = env->GetFieldID(cls, "hide_tips", "Z");
                env->SetBooleanField(ret, f, settings["hide_tips"]->v.b);

                f = env->GetFieldID(cls, "sandbox_back_dna", "Z");
                env->SetBooleanField(ret, f, settings["dna_sandbox_back"]->v.b);

                f = env->GetFieldID(cls, "display_fps", "I");
                env->SetIntField(ret, f, settings["display_fps"]->v.u8);

                f = env->GetFieldID(cls, "volume", "F");
                env->SetFloatField(ret, f, settings["volume"]->v.f);

                f = env->GetFieldID(cls, "muted", "Z");
                env->SetBooleanField(ret, f, settings["muted"]->v.b);
            }
        }
    }

    return ret;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setSetting(JNIEnv *env, jclass _jcls,
        jstring setting_name, jboolean value)
{
    const char *str = env->GetStringUTFChars(setting_name, 0);
    tms_infof("Setting setting %s to %s", str, value ? "TRUE" : "FALSE");
    settings[str]->v.b = (bool)value;

    env->ReleaseStringUTFChars(setting_name, str);
}

extern "C" jboolean
Java_org_libsdl_app_PrincipiaBackend_getSettingBool(JNIEnv *env, jclass _jcls,
        jstring setting_name)
{
    const char *str = env->GetStringUTFChars(setting_name, 0);
    jboolean ret = (jboolean)settings[str]->v.b;
    env->ReleaseStringUTFChars(setting_name, str);

    return ret;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_login(JNIEnv *env, jclass _jcls,
        jstring username, jstring password)
{
    const char *tmp_username = env->GetStringUTFChars(username, 0);
    const char *tmp_password = env->GetStringUTFChars(password, 0);
    struct login_data *data = (struct login_data*)malloc(sizeof(struct login_data));

    strcpy(data->username, tmp_username);
    strcpy(data->password, tmp_password);

    env->ReleaseStringUTFChars(username, tmp_username);
    env->ReleaseStringUTFChars(password, tmp_password);

    P.add_action(ACTION_LOGIN, (void*)data);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_register(JNIEnv *env, jclass _jcls,
        jstring username, jstring email, jstring password)
{
    const char *tmp_username = env->GetStringUTFChars(username, 0);
    const char *tmp_email = env->GetStringUTFChars(email, 0);
    const char *tmp_password = env->GetStringUTFChars(password, 0);
    struct register_data *data = (struct register_data*)malloc(sizeof(struct register_data));

    strcpy(data->username, tmp_username);
    strcpy(data->email,    tmp_email);
    strcpy(data->password, tmp_password);

    env->ReleaseStringUTFChars(username, tmp_username);
    env->ReleaseStringUTFChars(email, tmp_email);
    env->ReleaseStringUTFChars(password, tmp_password);

    P.add_action(ACTION_REGISTER, (void*)data);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_focusGL(JNIEnv *env, jclass _jcls,
        jboolean focus)
{
    P.focused = (int)(bool)focus;
    if (P.focused) {
        sm::resume_all();
    } else {
        sm::pause_all();
    }
    tms_infof("received focus event: %d", (int)(bool)focus);
}

extern "C" jboolean
Java_org_libsdl_app_PrincipiaBackend_isPaused(JNIEnv *env, jclass _cls)
{
    return (jboolean)(_tms.is_paused == 1 ? true : false);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPaused(JNIEnv *env, jclass _cls,
        jboolean b)
{
    _tms.is_paused = (b ? 1 : 0);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setSettings(JNIEnv *env, jclass _jcls,
        jboolean enable_shadows,
        jboolean enable_ao, jint shadow_quality,
        jint shadow_map_resx, jint shadow_map_resy, jint ao_map_res,
        jfloat uiscale,
        jfloat cam_speed, jfloat zoom_speed,
        jboolean smooth_cam, jboolean smooth_zoom,
        jboolean border_scroll_enabled, jfloat border_scroll_speed,
        jboolean display_object_ids,
        jboolean display_grapher_value,
        jboolean display_wireless_frequency,
        jfloat volume,
        jboolean muted,
        jboolean hide_tips,
        jboolean sandbox_back_dna,
        jint display_fps
        )
{
    bool do_reload_graphics = false;
    if (settings["enable_shadows"]->v.b != (bool)enable_shadows) {
        do_reload_graphics = true;
    } else if (settings["enable_ao"]->v.b != (bool)enable_ao) {
        do_reload_graphics = true;
    } else if (settings["shadow_quality"]->v.u8 != (int)shadow_quality) {
        do_reload_graphics = true;
    } else if (settings["shadow_map_resx"]->v.i != (int)shadow_map_resx) {
        do_reload_graphics = true;
    } else if (settings["shadow_map_resy"]->v.i != (int)shadow_map_resy)  {
        do_reload_graphics = true;
    } else if (settings["ao_map_res"]->v.i != (int)ao_map_res) {
        do_reload_graphics = true;
    }

    if (do_reload_graphics) {
        P.can_reload_graphics = false;
        P.can_set_settings = false;
        P.add_action(ACTION_RELOAD_GRAPHICS, 0);

        /* XXX: causes infinite loops on certain devices e.g. nexus 7 (WTF?)
        while (!P.can_set_settings) {
            tms_debugf("waiting for can set settings");
            SDL_Delay(5);
        }*/
    }

    settings["enable_shadows"]->v.b = (bool)enable_shadows;
    settings["enable_ao"]->v.b = (bool)enable_ao;
    settings["shadow_quality"]->v.u8 = (int)shadow_quality;
    settings["shadow_map_resx"]->v.i = (int)shadow_map_resx;
    settings["shadow_map_resy"]->v.i = (int)shadow_map_resy;
    settings["ao_map_res"]->v.i = (int)ao_map_res;

    if (settings["uiscale"]->set((float)uiscale)) {
        ui::message("You need to restart Principia before the UI scale change takes effect.");
    }
    settings["cam_speed_modifier"]->v.f = (float)cam_speed;
    settings["zoom_speed"]->v.f = (float)zoom_speed;
    settings["smooth_cam"]->v.b = (bool)smooth_cam;
    settings["smooth_zoom"]->v.b = (bool)smooth_zoom;
    settings["border_scroll_enabled"]->v.b = (bool)border_scroll_enabled;
    settings["border_scroll_speed"]->v.f = (float)border_scroll_speed;
    settings["display_object_id"]->v.b = (bool)display_object_ids;
    settings["display_grapher_value"]->v.b = (bool)display_grapher_value;
    settings["display_wireless_frequency"]->v.b = (bool)display_wireless_frequency;
    settings["hide_tips"]->v.b = (bool)hide_tips;
    settings["dna_sandbox_back"]->v.b = (bool)sandbox_back_dna;
    settings["display_fps"]->v.u8 = (uint8_t)display_fps;

    settings["muted"]->v.b = (bool)muted;
    settings["volume"]->v.f = (float)volume;

    if (do_reload_graphics) {
        P.can_reload_graphics = true;
    }

    if ((bool)enable_shadows) {
        tms_debugf("Shadows ENABLED. Resolution: %dx%d. Quality: %d",
                (int)shadow_map_resx, (int)shadow_map_resy, (int)shadow_quality);
    }
    if ((bool)enable_ao) {
        tms_debugf("AO ENABLED. Resolution: %dx%d",
                (int)ao_map_res, (int)ao_map_res);
    }

    tms_debugf("UI Scale: %.2f. Cam speed: %.2f. Zoom speed: %.2f",
            (float)uiscale, (float)cam_speed, (float)zoom_speed);

    settings.save();

    sm::load_settings();
}

/** ++Prompt **/
extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPromptResponse(JNIEnv *env, jclass _jcls, jint new_response)
{
    if (G->current_prompt) {
        base_prompt *bp = G->current_prompt->get_base_prompt();
        if (bp) {
            bp->set_response((uint8_t)new_response);
        }
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPromptPropertyString(JNIEnv *env, jclass _jcls,
        jint property_index, jstring value)
{
    if (G->current_prompt) {
        const char *tmp = env->GetStringUTFChars(value, 0);
        G->current_prompt->set_property(property_index, tmp);
        env->ReleaseStringUTFChars(value, tmp);
    }
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getPromptPropertyString(JNIEnv *env, jclass _jcls, jint property_index)
{
    char *nm = 0;

    if (G->current_prompt) {
        tms_infof("Current prompt is set!");
        nm = G->current_prompt->properties[property_index].v.s.buf;
    } else {
        tms_infof("Current prompt is not set!");
    }

    if (nm == 0) {
        return env->NewStringUTF("");
    }

    return env->NewStringUTF(nm);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_refreshPrompt(JNIEnv *env, jclass _jcls)
{
    if (G->current_prompt) {
        ui::message("Prompt properties saved!");
        G->current_prompt = 0;
    }
}

/** ++Sticky **/
extern "C" jboolean
Java_org_libsdl_app_PrincipiaBackend_getStickyCenterHoriz(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && G->selection.e->g_id == 60)
        return (jboolean)G->selection.e->properties[1].v.i8;

    return JNI_FALSE;
}

extern "C" jboolean
Java_org_libsdl_app_PrincipiaBackend_getStickyCenterVert(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && G->selection.e->g_id == 60)
        return (jboolean)G->selection.e->properties[2].v.i8;

    return JNI_FALSE;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getStickySize(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && G->selection.e->g_id == 60)
        return (jint)G->selection.e->properties[3].v.i8;

    return 0;
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getStickyText(JNIEnv *env, jclass _jcls)
{
    jstring str;
    char *nm = 0;

    if (G->selection.e && G->selection.e->g_id == 60) {
        nm = G->selection.e->properties[0].v.s.buf;
    }

    if (nm == 0) {
        return env->NewStringUTF("");
    }

    return env->NewStringUTF(nm);
}

/** ++Cam targeter **/
extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getCamTargeterFollowMode(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && G->selection.e->g_id == 133)
        return (jint)G->selection.e->properties[1].v.i;

    return 0;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setCamTargeterFollowMode(
        JNIEnv *env, jclass _jcls,
        jint follow_mode)
{
    if (G->selection.e && G->selection.e->g_id == 133) {
        G->selection.e->properties[1].v.i = follow_mode;

        ui::message("Cam targeter properties saved!");
        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getConsumables(JNIEnv *env, jclass _jcls)
{
    std::stringstream b("", std::ios_base::app | std::ios_base::out);

    for (int x=0; x<NUM_ITEMS; x++) {
        if (x != 0) b << ',';
        b << item_options[x].name;
    }

    jstring str;
    str = env->NewStringUTF(b.str().c_str());
    return str;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getConsumableType(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && G->selection.e->g_id == O_ITEM) {
        return (jint)(((item*)G->selection.e)->get_item_type());
    }

    return 0;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setConsumableType(JNIEnv *env, jclass _jcls, jint t)
{
    if (G->selection.e && G->selection.e->g_id == O_ITEM) {
        tms_debugf("New item type: %d", t);
        ((item*)G->selection.e)->set_item_type(t);
        ((item*)G->selection.e)->do_recreate_shape = true;
        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getCurrentCommunityUrl(JNIEnv *env, jclass _jcls)
{
    COMMUNITY_URL("level/%d", W->level.community_id);

    return env->NewStringUTF(url);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setGameMode(JNIEnv *env, jclass _jcls, jint mode)
{
    G->set_mode(mode);
}

/** ++Command pad **/
extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getCommandPadCommand(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && G->selection.e->g_id == 64) {
        return (jint)((command*)G->selection.e)->get_command();
    }

    return 0;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setCommandPadCommand(
        JNIEnv *env, jclass _jcls,
        jint cmd)
{
    if (G->selection.e && G->selection.e->g_id == 64) {
        ((command*)G->selection.e)->set_command(cmd);

        ui::message("Command pad properties saved!");
        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

/** ++FX Emitter **/
extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getFxEmitterEffects(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && G->selection.e->g_id == 135) {
        entity *e = G->selection.e;
        char effects[128];

        sprintf(effects, "%u,%u,%u,%u",
                e->properties[3+0].v.i, e->properties[3+1].v.i,
                e->properties[3+2].v.i, e->properties[3+3].v.i);

        jstring str;
        str = env->NewStringUTF(effects);

        return str;
    }

    /* XXX: Will this break? */
    return 0;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setFxEmitterEffects(
        JNIEnv *env, jclass _jcls,
        jint effect_1, jint effect_2, jint effect_3, jint effect_4)
{
    if (G->selection.e && G->selection.e->g_id == 135) {
        entity *e = G->selection.e;

        if (effect_1 == 0) {
            e->properties[3+0].v.i = FX_INVALID;
        } else {
            e->properties[3+0].v.i = effect_1 - 1;
        }

        if (effect_2 == 0) {
            e->properties[3+1].v.i = FX_INVALID;
        } else {
            e->properties[3+1].v.i = effect_2 - 1;
        }

        if (effect_3 == 0) {
            e->properties[3+2].v.i = FX_INVALID;
        } else {
            e->properties[3+2].v.i = effect_3 - 1;
        }

        if (effect_4 == 0) {
            e->properties[3+3].v.i = FX_INVALID;
        } else {
            e->properties[3+3].v.i = effect_4 - 1;
        }

        ui::message("FX Emitter properties saved!");
        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

/** ++Event Listener **/
extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getEventListenerEventType(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && G->selection.e->g_id == 156) {
        return (jint)G->selection.e->properties[0].v.i;
    }

    return 0;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setEventListenerEventType(
        JNIEnv *env, jclass _jcls,
        jint event_type)
{
    if (G->selection.e && G->selection.e->g_id == 156) {
        G->selection.e->properties[0].v.i = event_type;

        ui::message("Event listener properties saved!");
        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

/** ++Package level chooser **/
extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getPkgItemLevelId(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && (G->selection.e->g_id == 131 || G->selection.e->g_id == 132)) {
        return (jint)G->selection.e->properties[0].v.i8;
    }

    return 0;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPkgItemLevelId(
        JNIEnv *env, jclass _jcls,
        jint level_id)
{
    if (G->selection.e && (G->selection.e->g_id == 131 || G->selection.e->g_id == 132)) {
        G->selection.e->properties[0].v.i8 = level_id;

        ui::message("Package object properties saved!");
        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_resetVariable(
        JNIEnv *env, jclass _jcls,
        jstring variable_name)
{
    const char *vn = env->GetStringUTFChars(variable_name, 0);

    std::map<std::string, float>::size_type num_deleted = W->level_variables.erase(vn);
    if (num_deleted != 0) {
        if (W->save_cache(W->level_id_type, W->level.local_id)) {
            ui::message("Successfully deleted data for this variable");
        } else {
            ui::message("Unable to delete variable data for this level.");
        }
    } else {
        ui::message("No data found for this variable");
    }

    env->ReleaseStringUTFChars(variable_name, vn);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_resetAllVariables(
        JNIEnv *env, jclass _jcls)
{
    W->level_variables.clear();
    if (W->save_cache(W->level_id_type, W->level.local_id)) {
        ui::message("All level-specific variables cleared.");
    } else {
        ui::message("Unable to delete variable data for this level.");
    }
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getLevelIdType(
        JNIEnv *env, jclass _jcls)
{
    return W->level_id_type;
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getEquipmentsHeadEquipment(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=0; x<NUM_HEAD_EQUIPMENT_TYPES; ++x) {
        uint32_t item_id = _head_equipment_to_item[x];
        const struct item_option &i = item_options[item_id];

        if (x == 0) {
            ss << "None,";
        } else {
            ss << i.name << ",";
        }
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getEquipmentsHead(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=0; x<NUM_HEAD_TYPES; ++x) {
        const struct item_option &i = item_options[_head_to_item[x]];

        if (x == 0) {
            ss << "None,";
        } else {
            ss << i.name << ",";
        }
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getEquipmentsBackEquipment(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=0; x<NUM_BACK_EQUIPMENT_TYPES; ++x) {
        const struct item_option &i = item_options[_back_to_item[x]];

        if (x == 0) {
            ss << "None,";
        } else {
            ss << i.name << ",";
        }
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getEquipmentsFrontEquipment(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=0; x<NUM_FRONT_EQUIPMENT_TYPES; ++x) {
        const struct item_option &i = item_options[_front_to_item[x]];

        if (x == 0) {
            ss << "None,";
        } else {
            ss << i.name << ",";
        }
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getEquipmentsFeet(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=0; x<NUM_FEET_TYPES; ++x) {
        const struct item_option &i = item_options[_feet_to_item[x]];

        if (x == 0) {
            ss << "None,";
        } else {
            ss << i.name << ",";
        }
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getEquipmentsBoltSet(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=0; x<NUM_BOLT_SETS; ++x) {
        const struct item_option &i = item_options[_bolt_to_item[x]];

        ss << i.name << ",";
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getEquipmentsWeapons(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=1; x<NUM_WEAPONS; ++x) {
        uint32_t item_id = _weapon_to_item[x];
        const struct item_option &i = item_options[_weapon_to_item[x]];

        ss << item_id << "=" << i.name << ",";
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getEquipmentsTools(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=1; x<NUM_TOOLS; ++x) {
        uint32_t item_id = _tool_to_item[x];
        if (item_id == 0) {
            continue;
        }

        const struct item_option &i = item_options[_tool_to_item[x]];

        ss << item_id << "=" << i.name << ",";
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getCompatibleCircuits(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    if (!G->selection.e || !G->selection.e->is_robot()) {
        return env->NewStringUTF("");
    }

    robot_base *r = static_cast<robot_base*>(G->selection.e);

    for (int x=0; x<NUM_CIRCUITS; ++x) {
        uint32_t item_id = _circuit_flag_to_item(1ULL << x);
        if (item_id == 0) {
            continue;
        }

        if ((1ULL << x) & r->circuits_compat) {
            const struct item_option &i = item_options[item_id];
            ss << item_id << "=" << i.name << ",";
        }
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_fixed(
        JNIEnv *env, jclass _jcls)
{
    if (G->selection.e) {
        entity *e = G->selection.e;

        if (e->is_robot()) {
            W->add_action(e->id, ACTION_CALL_ON_LOAD);
        } else {
            switch (e->g_id) {
                case O_PLASTIC_POLYGON:
                    ((polygon*)e)->do_recreate_shape = true;
                    break;

                case O_ANIMAL:
                    W->add_action(e->id, ACTION_SET_ANIMAL_TYPE, UINT_TO_VOID(e->properties[0].v.i));
                    break;

                case O_DECORATION:
                    ((decoration*)e)->set_decoration_type(e->properties[0].v.i);
                    ((decoration*)e)->do_recreate_shape = true;
                    break;
            }
        }

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getKeys(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=0; x<TMS_KEY__NUM; ++x) {
        const char *s = key_names[x];

        if (s) {
            // ;-)
            ss << x << "=_=" << s << ",.,";
        }
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getDecorations(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=0; x<NUM_DECORATIONS; ++x) {
        ss << decorations[x].name << ",.,";
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getAnimals(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=0; x<NUM_ANIMAL_TYPES; ++x) {
        ss << animal_data[x].name << ",.,";
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getSounds(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    for (int x=0; x<SND__NUM; x++) {
        ss << sm::sound_lookup[x]->name << ",.,";
    }

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setResourceType(JNIEnv *env, jclass _jcls,
        jlong value)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_RESOURCE) {
        ((resource*)e)->set_resource_type((uint32_t)value);
        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getRobotData(
        JNIEnv *env, jclass _jcls)
{
    std::stringstream ss;

    if (!G->selection.e || !G->selection.e->is_robot()) {
        return env->NewStringUTF("");
    }

    robot_base *r = static_cast<robot_base*>(G->selection.e);

    ss << r->is_player() << ","
       << r->has_feature(CREATURE_FEATURE_HEAD) << ","
       << r->has_feature(CREATURE_FEATURE_BACK_EQUIPMENT) << ","
       << r->has_feature(CREATURE_FEATURE_FRONT_EQUIPMENT) << ","
       << r->has_feature(CREATURE_FEATURE_WEAPONS) << ","
       << r->has_feature(CREATURE_FEATURE_TOOLS) << ","
       << (int)r->properties[ROBOT_PROPERTY_HEAD_EQUIPMENT].v.i8 << ","
       << (int)r->properties[ROBOT_PROPERTY_HEAD].v.i8 << ","
       << (int)r->properties[ROBOT_PROPERTY_BACK].v.i8 << ","
       << (int)r->properties[ROBOT_PROPERTY_FRONT].v.i8 << ","
       << (int)r->properties[ROBOT_PROPERTY_FEET].v.i8 << ","
       << (int)r->properties[ROBOT_PROPERTY_BOLT_SET].v.i8 << ","
       << (int)r->properties[ROBOT_PROPERTY_STATE].v.i8 << ","
       << (int)r->properties[ROBOT_PROPERTY_ROAMING].v.i8 << ","
       << (int)r->properties[ROBOT_PROPERTY_DIR].v.i8 << ","
       << (int)r->properties[ROBOT_PROPERTY_FACTION].v.i8 << ","
       << r->circuits_compat << ","
        ;

    return env->NewStringUTF(ss.str().c_str());
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getRobotEquipment(
        JNIEnv *env, jclass _jcls)
{
    if (!G->selection.e || !G->selection.e->is_robot()) {
        return env->NewStringUTF("");
    }

    robot_base *r = static_cast<robot_base*>(G->selection.e);

    return env->NewStringUTF(r->properties[ROBOT_PROPERTY_EQUIPMENT].v.s.buf);
}

/** ++Color Chooser **/
extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getEntityColor(JNIEnv *env, jclass _jcls)
{
    int color = 0;

    if (G->selection.e) {
        entity *e = G->selection.e;

        tvec4 c = e->get_color();
        color = ((int)(c.a * 255.f) << 24)
            + ((int)(c.r * 255.f) << 16)
            + ((int)(c.g * 255.f) << 8)
            +  (int)(c.b * 255.f);
    }

    return (jint)color;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setEntityColor(
        JNIEnv *env, jclass _jcls,
        jint color)
{
    int alpha;
    float r,g,b;
    alpha = ((color & 0xFF000000) >> 24);
    if (alpha < 0) alpha = 0;

    r = (float)((color & 0x00FF0000) >> 16) / 255.f;
    g = (float)((color & 0x0000FF00) >> 8 ) / 255.f;
    b = (float)((color & 0x000000FF)      ) / 255.f;

    if (G->selection.e) {
        entity *e = G->selection.e;

        e->set_color4(r, g, b);

        if (e->g_id == O_PIXEL) {
            uint8_t frequency = (uint8_t)alpha;
            e->set_property(4, frequency);
        }

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

extern "C" jfloat
Java_org_libsdl_app_PrincipiaBackend_getEntityAlpha(
        JNIEnv *env, jclass _jcls,
        jfloat alpha)
{
    jfloat a = 1.f;

    if (G->selection.e && G->selection.e->g_id == O_PIXEL) {
        a = (jfloat)(G->selection.e->properties[4].v.i8 / 255);
    }

    return a;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setEntityAlpha(
        JNIEnv *env, jclass _jcls,
        jfloat alpha)
{
    if (G->selection.e && G->selection.e->g_id == O_PIXEL) {
        G->selection.e->properties[4].v.i8 = (uint8_t)(alpha * 255);
    }
}

/** ++Digital Display **/
extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setDigitalDisplayStuff(
        JNIEnv *env, jclass _jcls,
        jboolean wrap_around,
        jint initial_position,
        jstring new_symbols)
{
    entity *e = G->selection.e;

    if (e && (e->g_id == O_PASSIVE_DISPLAY || e->g_id == O_ACTIVE_DISPLAY)) {
        display *d = static_cast<display*>(e);
        const char *symbols = env->GetStringUTFChars(new_symbols, 0);

        d->properties[0].v.i8 = (wrap_around?1:0);
        d->properties[1].v.i8 = initial_position;
        d->set_property(2, symbols);

        d->set_active_symbol(initial_position);
        d->load_symbols();

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);

        env->ReleaseStringUTFChars(new_symbols, symbols);
    }
}

/** ++Frequency Dialog **/
extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setFrequency(
        JNIEnv *env, jclass _jcls,
        jlong frequency)
{
    if (G->selection.e && G->selection.e->is_wireless()) {
        int64_t f = (int64_t)frequency;

        if (f < 0) f = 0;

        G->selection.e->properties[0].v.i = (uint32_t)f;

        ui::messagef("Frequency set to %u", G->selection.e->properties[0].v.i);

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setFrequencyRange(
        JNIEnv *env, jclass _jcls,
        jlong frequency, jlong range)
{
    if (G->selection.e && G->selection.e->g_id == 125) {
        int64_t f, r;
        f = (int64_t)frequency;
        r = (int64_t)range;

        if (f < 0) f = 0;
        if (r < 0) r = 0;

        G->selection.e->properties[0].v.i = (uint32_t)f;
        G->selection.e->properties[1].v.i = (uint32_t)r;

        ui::messagef("Frequency set to %u (+%u)", G->selection.e->properties[0].v.i, G->selection.e->properties[1].v.i);

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

/** ++Export **/
extern "C" void
Java_org_libsdl_app_PrincipiaBackend_saveObject(
        JNIEnv *env, jclass _jcls,
        jstring name)
{
    const char *tmp = env->GetStringUTFChars(name, 0);
    char *_name = strdup(tmp);

    P.add_action(ACTION_EXPORT_OBJECT, _name);
    ui::message("Saved object!");

    env->ReleaseStringUTFChars(name, tmp);
}

/** ++Sequencer **/
extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setSequencerData(
        JNIEnv *env, jclass _jcls,
        jstring _sequence,
        jint _seconds, jint _milliseconds,
        jboolean _wrap_around)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_SEQUENCER) {
        const char *sequence = env->GetStringUTFChars(_sequence, 0);
        uint32_t seconds = (uint32_t)_seconds;
        uint32_t milliseconds = (uint32_t)_milliseconds;
        uint32_t full_time = (seconds * 1000) + milliseconds;
        uint8_t wrap_around = _wrap_around ? 1 : 0;

        if (full_time < TIMER_MIN_TIME)
            full_time = TIMER_MIN_TIME;

        e->set_property(0, sequence);
        e->properties[1].v.i = full_time;
        e->properties[2].v.i8 = wrap_around;

        ((sequencer*)e)->refresh_sequence();

        ui::message("Sequencer properties saved!");

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

/** ++Timer **/
extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setTimerData(
        JNIEnv *env, jclass _jcls,
        jint _seconds, jint _milliseconds, jint _num_ticks, jboolean use_system_time)
{
    entity *e = G->selection.e;

    if (e && e->g_id == O_TIMER) {
        uint32_t seconds = (uint32_t)_seconds;
        uint32_t milliseconds = (uint32_t)_milliseconds;
        uint32_t full_time = (seconds * 1000) + milliseconds;
        uint8_t num_ticks = (uint8_t)_num_ticks;

        if (full_time < TIMER_MIN_TIME) {
            full_time = TIMER_MIN_TIME;
        }

        e->properties[0].v.i = full_time;
        e->properties[1].v.i8 = num_ticks;
        e->properties[2].v.i = use_system_time ? 1 : 0;

        ui::message("Timer properties saved!");

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

/** ++Robot **/
extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getRobotState(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && G->selection.e->flag_active(ENTITY_IS_ROBOT))
        return (jint)G->selection.e->properties[1].v.i8;

    return 0;
}

extern "C" jboolean
Java_org_libsdl_app_PrincipiaBackend_getRobotRoam(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && G->selection.e->flag_active(ENTITY_IS_ROBOT))
        return (jboolean)G->selection.e->properties[2].v.i8;

    return JNI_FALSE;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getRobotDir(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e && G->selection.e->flag_active(ENTITY_IS_ROBOT))
        return (jint)G->selection.e->properties[4].v.i8;

    return 0;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setRobotStuff(
        JNIEnv *env, jclass _jcls,
        jint state,
        jint faction,
        jboolean roam,
        jint dir)
{

    if (G->selection.e && G->selection.e->flag_active(ENTITY_IS_ROBOT)) {
        G->selection.e->properties[1].v.i8 = state;
        G->selection.e->properties[2].v.i8 = roam?1:0;
        G->selection.e->properties[4].v.i8 = dir;
        G->selection.e->properties[ROBOT_PROPERTY_FACTION].v.i8 = faction;
        switch (dir) {
            case 0: ((robot_base*)G->selection.e)->set_i_dir(0.f); break;
            case 1: ((robot_base*)G->selection.e)->set_i_dir(DIR_LEFT); break;
            case 2: ((robot_base*)G->selection.e)->set_i_dir(DIR_RIGHT); break;
        }

        ui::message("Robot properties saved!");

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

/** ++++++++++++++++++++++++++ **/

static char *_tmp_args[2];
static char _tmp_arg1[256];

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setarg(JNIEnv *env, jclass _jcls,
        jstring arg)
{
    _tmp_args[0] = 0;
    _tmp_args[1] = _tmp_arg1;

    const char *tmp = env->GetStringUTFChars(arg, 0);
    int len = env->GetStringUTFLength(arg);

    if (len > 255)
        len = 255;

    memcpy(&_tmp_arg1, tmp, len);
    _tmp_arg1[len] = '\0';

    tproject_set_args(2, _tmp_args);

    env->ReleaseStringUTFChars(arg, tmp);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelName(JNIEnv *env, jclass _jcls,
        jstring name)
{
    const char *tmp = env->GetStringUTFChars(name, 0);
    int len = env->GetStringUTFLength(name);

    if (len > LEVEL_NAME_MAX_LEN) {
        len = LEVEL_NAME_MAX_LEN;
    }

    memcpy(W->level.name, tmp, len);
    W->level.name_len = len;

    env->ReleaseStringUTFChars(name, tmp);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getLevelName(JNIEnv *env, jclass _jcls)
{
    jstring str;
    char tmp[257];

    char *nm = W->level.name;
    memcpy(tmp, nm, W->level.name_len);

    tmp[W->level.name_len] = '\0';

    str = env->NewStringUTF(tmp);
    return str;
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getLevels(JNIEnv *env, jclass _jcls, jint level_type)
{
    std::stringstream b("", std::ios_base::app | std::ios_base::out);

    lvlfile *level = pkgman::get_levels((int)level_type);

    while (level) {
        b << level->id << ',';
        b << level->save_id << ',';
        b << level->id_type << ',';
        b << level->name;
        b << '\n';
        lvlfile *next = level->next;
        delete level;
        level = next;
    }

    tms_infof("getLevels: %s", b.str().c_str());

    jstring str;
    str = env->NewStringUTF(b.str().c_str());
    return str;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getSelectionGid(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e) {
        return (jint)G->selection.e->g_id;
    }

    return 0;
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getSfxSounds(JNIEnv *env, jclass _jcls)
{
    std::stringstream b("", std::ios_base::app | std::ios_base::out);

    for (int x=0; x<NUM_SFXEMITTER_OPTIONS; x++) {
        if (x != 0) b << ',';
        b << sfxemitter_options[x].name;
    }

    jstring str;
    str = env->NewStringUTF(b.str().c_str());
    return str;
}

extern "C" jboolean
Java_org_libsdl_app_PrincipiaBackend_isAdventure(JNIEnv *env, jclass _jcls)
{
    return (jboolean)W->is_adventure();
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getLevelType(JNIEnv *env, jclass _jcls)
{
    tms_infof("Level type: %d", W->level.type);
    return (jint)W->level.type;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelType(JNIEnv *env, jclass _jcls,
        jint type)
{
    if (type >= LCAT_PUZZLE && type <= LCAT_CUSTOM) {
        P.add_action(ACTION_SET_LEVEL_TYPE, (void*)type);
    }
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getSynthWaveforms(JNIEnv *env, jclass _jcls)
{
    std::stringstream b("", std::ios_base::app | std::ios_base::out);

    for (int x=0; x<NUM_WAVEFORMS; x++) {
        if (x != 0) b << ',';
        b << speaker_options[x];
    }

    jstring str;
    str = env->NewStringUTF(b.str().c_str());
    return str;
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getAvailableBgs(JNIEnv *env, jclass _jcls)
{
    std::stringstream b("", std::ios_base::app | std::ios_base::out);

    for (int x=0; x<num_bgs; ++x) {
        b << available_bgs[x];
        b << '\n';
    }

    jstring str;
    str = env->NewStringUTF(b.str().c_str());
    return str;
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getLevelDescription(JNIEnv *env, jclass _jcls)
{
    char *descr = W->level.descr;
    if (descr == 0 || W->level.descr_len == 0) {
        return env->NewStringUTF("");
    }

    return env->NewStringUTF(descr);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelDescription(JNIEnv *env, jclass _jcls,
        jstring descr)
{
    lvlinfo *l = &W->level;
    const char *tmp = env->GetStringUTFChars(descr, 0);
    int len = env->GetStringUTFLength(descr);

    if (len > LEVEL_DESCR_MAX_LEN)
        len = LEVEL_DESCR_MAX_LEN;

    if (len == 0) {
        l->descr = 0;
    } else {
        l->descr = (char*)realloc(l->descr, len+1);

        memcpy(l->descr, tmp, len);
        l->descr[len] = '\0';
    }

    l->descr_len = len;

    env->ReleaseStringUTFChars(descr, tmp);

    tms_debugf("New description: '%s'", l->descr);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getFactoryResources(JNIEnv *env, jclass _jcls)
{
    char info[2048];
    char *target = info;

    if (G->selection.e && IS_FACTORY(G->selection.e->g_id)) {
        entity *e = G->selection.e;

        target += sprintf(target, "%d", e->properties[1].v.i);
        for (int n=0; n<NUM_RESOURCES; ++n) {
            target += sprintf(target, ",%d", e->properties[FACTORY_NUM_EXTRA_PROPERTIES+n].v.i);
        }
    }

    jstring str;
    str = env->NewStringUTF(info);
    return str;
}

/* Returns a list of all resources, including "Oil" */
extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getResources(JNIEnv *env, jclass _jcls)
{
    char info[2048];

    strcpy(info, "Oil");

    for (int n=0; n<NUM_RESOURCES; ++n) {
        strcat(info, ",");
        strcat(info, resource_data[n].name);
    }

    jstring str;
    str = env->NewStringUTF(info);
    return str;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getFactoryNumExtraProperties(JNIEnv *env, jclass _jcls)
{
    return FACTORY_NUM_EXTRA_PROPERTIES;
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getRecipes(JNIEnv *env, jclass _jcls)
{
    char info[2048];
    char *target = info;

    strcat(info, ""); // XXX: is this necessary?

    if (G->selection.e && IS_FACTORY(G->selection.e->g_id)) {
        factory *fa = static_cast<factory*>(G->selection.e);

        std::vector<struct factory_object> &objs = fa->objects();

        std::vector<uint32_t> recipes;
        factory::generate_recipes(&recipes, fa->properties[0].v.s.buf);

        for (std::vector<struct factory_object>::const_iterator it = objs.begin();
                it != objs.end(); ++it) {
            const struct factory_object &fo = *it;
            int x = it - objs.begin();
            int enabled = 0;

            for (std::vector<uint32_t>::iterator it = recipes.begin(); it != recipes.end(); ++it) {
                if (*it == x) {
                    enabled = 1;
                    break;
                }
            }

            target += sprintf(target, "%s;%d;%d,",
                    ((fa->factory_type == FACTORY_ARMORY || fa->factory_type == FACTORY_OIL_MIXER) ? item_options[objs[x].gid].name : of::get_object_name_by_gid(objs[x].gid)),
                    enabled,x);
        }
    }

    jstring str;
    str = env->NewStringUTF(info);
    return str;
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getLevelInfo(JNIEnv *env, jclass _jcls)
{
    char info[2048];

    lvlinfo *l = &W->level;

    /**
     * int bg,
     * int left_border, int right_border, int bottom_border, int top_border,
     * float gravity_x, float gravity_y,
     * int position_iterations, int velocity_iterations,
     * int final_score,
     * boolean pause_on_win, boolean display_score,
     * float prismatic_tolerance, float pivot_tolerance,
     * int color,
     * float linear_damping,
     * float angular_damping,
     * float joint_friction,
     * float creature_absorb_time
     **/

    sprintf(info,
            "%u,"
            "%u,"
            "%u,"
            "%u,"
            "%u,"
            "%.1f,"
            "%.1f,"
            "%u,"
            "%u,"
            "%u,"
            "%s,"
            "%s,"
            "%f,"
            "%f,"
            "%d,"
            "%f,"
            "%f,"
            "%f,"
            "%f,"
            "%f,"
            ,
                l->bg,
                l->size_x[0], l->size_x[1], l->size_y[0], l->size_y[1],
                l->gravity_x, l->gravity_y,
                l->position_iterations, l->velocity_iterations,
                l->final_score,
                l->pause_on_finish ? "true" : "false",
                l->show_score ? "true" : "false",
                l->prismatic_tolerance, l->pivot_tolerance,
                l->bg_color,
                l->linear_damping,
                l->angular_damping,
                l->joint_friction,
                l->dead_enemy_absorb_time,
                l->time_before_player_can_respawn
            );

    jstring str;
    str = env->NewStringUTF(info);
    return str;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getLevelVersion(JNIEnv *env, jclass _jcls)
{
    lvlinfo *l = &W->level;

    return (jint)l->version;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getMaxLevelVersion(JNIEnv *env, jclass _jcls)
{
    return (jint)LEVEL_VERSION;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setStickyStuff(
        JNIEnv *env, jclass _jcls,
        jstring text,
        jint center_horiz, jint center_vert,
        jint size)
{

    if (G->selection.e && G->selection.e->g_id == O_STICKY_NOTE) {
        const char *tmp = env->GetStringUTFChars(text, 0);
        G->selection.e->set_property(0, tmp);
        env->ReleaseStringUTFChars(text, tmp);
        G->selection.e->set_property(1, (uint8_t)center_horiz);
        G->selection.e->set_property(2, (uint8_t)center_vert);
        G->selection.e->set_property(3, (uint8_t)size);

        P.add_action(ACTION_STICKY, 0);
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelLocked(
        JNIEnv *env, jclass _jcls,
        jboolean locked)
{
    lvlinfo *l = &W->level;

    l->visibility = ((bool)locked ? LEVEL_LOCKED : LEVEL_VISIBLE);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelInfo(
        JNIEnv *env, jclass _jcls,
        jint bg,
        jint border_left, jint border_right, jint border_bottom, jint border_top,
        jfloat gravity_x, jfloat gravity_y,
        jint position_iterations, jint velocity_iterations,
        jint final_score,
        jboolean pause_on_win, jboolean display_score,
        jfloat prismatic_tolerance, jfloat pivot_tolerance,
        jint bg_color,
        jfloat linear_damping,
        jfloat angular_damping,
        jfloat joint_friction,
        jfloat dead_enemy_absorb_time,
        jfloat time_before_player_can_respawn
        )
{
    lvlinfo *l = &W->level;

    /**
      * int bg,
      * int left_border, int right_border, int bottom_border, int top_border,
      * float gravity_x, float gravity_y,
      * int position_iterations, int velocity_iterations,
      * int final_score,
      * boolean pause_on_win,
      * boolean display_score
      **/
    l->bg = (uint8_t)bg;

    uint16_t left  = (uint16_t)border_left;
    uint16_t right = (uint16_t)border_right;
    uint16_t down  = (uint16_t)border_bottom;
    uint16_t up    = (uint16_t)border_top;

    float w = (float)left + (float)right;
    float h = (float)down + (float)up;

    bool resized = false;

    if (w < 5.f) {
        resized = true;
        left += 6-(uint16_t)w;
    }
    if (h < 5.f) {
        resized = true;
        down += 6-(uint16_t)w;
    }

    if (resized) {
        ui::message("Your level size was increased to the minimum allowed.");
    }

    l->size_x[0] = left;
    l->size_x[1] = right;
    l->size_y[0] = down;
    l->size_y[1] = up;
    l->gravity_x = (float)gravity_x;
    l->gravity_y = (float)gravity_y;
    l->final_score = (uint32_t)final_score;
    l->position_iterations = (uint8_t)position_iterations;
    l->velocity_iterations = (uint8_t)velocity_iterations;
    l->pause_on_finish = (bool)pause_on_win;
    l->show_score = (bool)display_score;
    l->prismatic_tolerance = (float)prismatic_tolerance;
    l->pivot_tolerance = (float)pivot_tolerance;
    l->bg_color = (int)bg_color;
    l->linear_damping = (float)linear_damping;
    l->angular_damping = (float)angular_damping;
    l->joint_friction = (float)joint_friction;
    l->dead_enemy_absorb_time = (float)dead_enemy_absorb_time;
    l->time_before_player_can_respawn = (float)time_before_player_can_respawn;

    P.add_action(ACTION_RELOAD_LEVEL, 0);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_resetLevelFlags(
        JNIEnv *env, jclass _jcls,
        jlong flag)
{
    lvlinfo *l = &W->level;

    l->flags = 0;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelFlag(
        JNIEnv *env, jclass _jcls,
        jlong _flag)
{
    lvlinfo *l = &W->level;

    uint32_t flag = (uint32_t)_flag;

    //tms_infof("x: %d", flag);
    uint64_t f = (uint64_t)(1ULL << flag);
    //tms_infof("f: %llu", f);

    //tms_infof("Flags before: %llu", l->flags);
    l->flags |= f;
    //tms_infof("Flags after: %llu", l->flags);
}

extern "C" jboolean
Java_org_libsdl_app_PrincipiaBackend_getLevelFlag(
        JNIEnv *env, jclass _jcls,
        jlong _flag)
{
    lvlinfo *l = &W->level;

    uint32_t flag = (uint32_t)_flag;
    uint64_t f = (1ULL << flag);

    return (jboolean)(l->flag_active(f));
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_triggerSave(
        JNIEnv *env, jclass _jcls, jboolean save_copy)
{
    if (save_copy)
        P.add_action(ACTION_SAVE_COPY, 0);
    else
        P.add_action(ACTION_SAVE, 0);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_triggerCreateLevel(
        JNIEnv *env, jclass _jcls, jint level_type)
{
    P.add_action(ACTION_NEW_LEVEL, level_type);
}

#endif
