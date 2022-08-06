package org.libsdl.app;

import com.bithack.principia.shared.Settings;

public class PrincipiaBackend
{
	public static native int getLevelVersion();
	public static native String getCookies();
	public static native void addAction(int id, String action_string);
	public static native void addActionAsInt(int action_id, long action_data);
	public static native long getPropertyInt(int property_index);
	public static native int getPropertyInt8(int property_index);
	public static native String getPropertyString(int property_index);
	public static native float getPropertyFloat(int property_index);
	public static native void setPropertyInt(int property_index, long value);
	public static native void setPropertyInt8(int property_index, int value);
	public static native void setPropertyString(int property_index, String value);
	public static native void setPropertyFloat(int property_index, float value);
	public static native String getAvailableBgs();
	
	public static native void setSettings(boolean enable_shadows, boolean enable_ao, int shadow_quality, int shadow_map_resx, int shadow_map_resy, int ao_map_res, int texture_quality, float uiscale, float cam_speed, float zoom_speed, boolean smooth_cam, boolean smooth_zoom, boolean border_scroll_enabled, float border_scroll_speed, boolean display_object_ids);
    public static native Settings getSettings();
    public static native boolean getSettingBool(String setting_name);
    public static native void setSetting(String setting_name, boolean value);
    public static native void focusGL(boolean f);
    
    public static native String getSandboxTip();
    public static native void setNextAction(int action_id);
    public static native void login(String username, String password);
    public static native void register(String username, String email, String password, String signature, String userdata);
    public static native void linkAccount(String username, String password, String signature, String userdata);

    public static native void resetLevelFlags();
    public static native void setLevelFlag(long flag);
    public static native boolean getLevelFlag(long flag);
    public static native String getLevelName();
    public static native String getLevelDescription();
    public static native String getLevelInfo();
    public static native void setLevelInfo(int bg, int left_border, int right_border, int bottom_border, int top_border, float gravity_x, float gravity_y, int position_iterations, int velocity_iterations, int final_score, boolean pause_on_win, boolean display_score, float prismatic_tolerance, float pivot_tolerance);
    public static native void setLevelAllowDerivatives(boolean state);
    public static native void setLevelLocked(boolean state);
    
    public static native void setarg(String arg);

    public static native String getLevels(int level_type);

    public static native String getStickyText();
    public static native boolean getStickyCenterHoriz();
    public static native boolean getStickyCenterVert();
    public static native int getStickySize();
    public static native void setStickyStuff(String text, boolean center_horiz, boolean center_vert, int size);

    public static native int getCamTargeterFollowMode();
    public static native void setCamTargeterFollowMode(int follow_mode);

    public static native int getCommandPadCommand();
    public static native void setCommandPadCommand(int command);

    public static native String getFxEmitterEffects();
    public static native void setFxEmitterEffects(int effect_1, int effect_2, int effect_3, int effect_4);

    public static native int getEventListenerEventType();
    public static native void setEventListenerEventType(int event_type);
    
    public static native int getPkgItemLevelId();
    public static native void setPkgItemLevelId(int pkg_level_id);

    public static native int getEntityColor();
    public static native void setEntityColor(int color);
    public static native float getEntityAlpha();
    public static native void setEntityAlpha(float alpha);

    public static native void setDigitalDisplayStuff(boolean wrap_around, int initial_position, String symbols);

    public static native int setFrequency(long frequency);
    public static native int setFrequencyRange(long frequency, long range);
    
    public static native int saveObject(String name);
    
    public static native void setTimerData(int seconds, int milliseconds, int num_ticks);

    public static native int getRobotState();
    public static native boolean getRobotRoam();
    public static native int getRobotDir();
    public static native void setRobotStuff(int state, boolean roam, int dir);

    public static native void setLevelName(String name);
    public static native void setLevelDescription(String descr);
    public static native void triggerSave(boolean save_copy);
    public static native void triggerCreateLevel(int level_type);
	public static native boolean isPaused();
	public static native void setPaused(boolean b);

	public static native void setPromptResponse(int response);
	public static native void setPromptPropertyString(int property_index, String str);
	public static native String getPromptPropertyString(int property_index);
	public static native void refreshPrompt();

	public static native void reset_variable(String variable_name);
	public static native void reset_all_variables();

	public static native String getSfxSounds();
	public static native String getSynthWaveforms();
	
	public static native int getSelectionGid();
	
	public static native void setLevelType(int type);
	public static native int getLevelType();
	public static native void setSequencerData(String sequence, int seconds, int milliseconds, boolean wrap_around);
	
	public static native void updateJumper(float value);
	public static native byte[] getMiscValues();
	public static native String getMiscSettings();
	public static native String getLevelPage();
	public static native String getObjects();
	public static native void createObject(String str);
	public static native int getMaxLevelVersion();
	public static native int getConsumableType();
	public static native String getConsumables();
	public static native void setConsumableType(int consumable_type);
	public static native String getCurrentCommunityUrl();
	public static native void setGameMode(int i);
	
	public static native void updateRubberEntity(float restitution, float friction);
	public static native void updateShapeExtruder(float v_right, float v_up, float v_left, float v_down);
}
