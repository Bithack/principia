package com.bithack.principia;

import com.bithack.principia.shared.Settings;

public class PrincipiaBackend
{
    public static native int getLevelVersion();
    public static native String getCookies();
    public static native String getCommunityHost();

    public static native void addAction(int id, String action_string);
    public static native void addActionAsInt(int action_id, long action_data);
    public static native void addActionAsPair(int action_id, long data1, long data2);
    public static native void addActionAsTriple(int action_id, long data1, long data2, long data3);
    public static native void addActionAsVec4(int action_id, float r, float g, float b, float a);

    public static native long getPropertyInt(int property_index);
    public static native int getPropertyInt8(int property_index);
    public static native String getPropertyString(int property_index);
    public static native float getPropertyFloat(int property_index);
    public static native void setPropertyInt(int property_index, long value);
    public static native void setPropertyInt8(int property_index, int value);
    public static native void setPropertyString(int property_index, String value);
    public static native void setPropertyFloat(int property_index, float value);
    public static native String getAvailableBgs();

    public static native void setSettings(
            boolean enable_shadows,
            boolean enable_ao,
            int shadow_quality,
            int shadow_map_resx,
            int shadow_map_resy,
            int ao_map_res,
            float uiscale,
            float cam_speed,
            float zoom_speed,
            boolean smooth_cam,
            boolean smooth_zoom,
            boolean border_scroll_enabled,
            float border_scroll_speed,
            boolean display_object_ids,
            boolean display_grapher_value,
            boolean display_wireless_frequency,
            float volume,
            boolean muted,
            boolean hide_tips,
            boolean sandbox_back_dna,
            int display_fps
                );
    public static native Settings getSettings();
    public static native boolean getSettingBool(String setting_name);
    public static native void setSetting(String setting_name, boolean value);
    public static native void focusGL(boolean f);

    public static native String getSandboxTip();
    public static native void setNextAction(int action_id);
    public static native void login(String username, String password);
    public static native void register(String username, String email, String password);

    public static native void resetLevelFlags();
    public static native void setLevelFlag(long flag);
    public static native boolean getLevelFlag(long flag);
    public static native String getLevelName();
    public static native String getLevelDescription();
    public static native String getLevelInfo();
    public static native void setLevelInfo(
            int bg,
            int left_border,
            int right_border,
            int bottom_border,
            int top_border,
            float gravity_x,
            float gravity_y,
            int position_iterations,
            int velocity_iterations,
            int final_score,
            boolean pause_on_win,
            boolean display_score,
            float prismatic_tolerance,
            float pivot_tolerance,
            int bg_color,
            float linear_damping,
            float angular_damping,
            float joint_friction,
            float creature_absorb_time,
            float player_respawn_time
                );

    public static native void setLevelLocked(boolean state);

    public static native void setarg(String arg);

    public static native String getLevels(int level_type);

    public static native String getStickyText();
    public static native boolean getStickyCenterHoriz();
    public static native boolean getStickyCenterVert();
    public static native int getStickySize();
    public static native void setStickyStuff(String text, boolean center_horiz, boolean center_vert, int size);

    public static native int getEntityColor();
    public static native void setEntityColor(int color);
    public static native float getEntityAlpha();
    public static native void setEntityAlpha(float alpha);

    public static native int setFrequency(long frequency);
    public static native int setFrequencyRange(long frequency, long range);

    public static native int saveObject(String name);

    public static native int getRobotState();
    public static native boolean getRobotRoam();
    public static native int getRobotDir();
    public static native void setRobotStuff(int state, int faction, boolean roam, int dir);

    public static native void setLevelName(String name);
    public static native void setLevelDescription(String descr);
    public static native void triggerSave(boolean save_copy);
    public static native boolean isPaused();
    public static native void setPaused(boolean b);

    public static native void setPromptResponse(int response);
    public static native void setPromptPropertyString(int property_index, String str);
    public static native String getPromptPropertyString(int property_index);
    public static native void refreshPrompt();

    public static native void setLevelType(int type);
    public static native int getLevelType();

    public static native boolean isAdventure();

    public static native void updateJumper(float value);
    public static native String getLevelPage();
    public static native String getObjects();
    public static native void createObject(String str);
    public static native int getMaxLevelVersion();
    public static native String getCurrentCommunityUrl();

    public static native String getResources();
    public static native String getFactoryResources();
    public static native String getRecipes();
    public static native int getFactoryNumExtraProperties();
    public static native void setMultiemitterObject(long get_id);
    public static native void setImportObject(long get_id);
    public static native int getLevelIdType();

    public static native String getRobotData();
    public static native String getRobotEquipment();
    public static native String getEquipmentsHeadEquipment();
    public static native String getEquipmentsHead();
    public static native String getEquipmentsBackEquipment();
    public static native String getEquipmentsFrontEquipment();
    public static native String getEquipmentsFeet();
    public static native String getEquipmentsBoltSet();
    public static native String getEquipmentsWeapons();
    public static native String getEquipmentsTools();
    public static native String getCompatibleCircuits();

    public static native void fixed();

    public static native void openState(int level_type, int level_id, int save_id, boolean from_menu);

    public static native void openDialog(int dialog_id);
}
