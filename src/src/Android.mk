DIR := src

LOCAL_SRC_FILES += \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/$(DIR)/*.cc) \
	$(wildcard $(LOCAL_PATH)/$(DIR)/*.c) \
	$(wildcard $(LOCAL_PATH)/$(DIR)/Box2D/Dynamics/*.cc) \
	$(wildcard $(LOCAL_PATH)/$(DIR)/Box2D/Collision/*.cc) \
	$(wildcard $(LOCAL_PATH)/$(DIR)/Box2D/Collision/Shapes/*.cc) \
	$(wildcard $(LOCAL_PATH)/$(DIR)/Box2D/Common/*.cc) \
	$(wildcard $(LOCAL_PATH)/$(DIR)/Box2D/Dynamics/Contacts/*.cc) \
	$(wildcard $(LOCAL_PATH)/$(DIR)/Box2D/Dynamics/Joints/*.cc) \
	$(wildcard $(LOCAL_PATH)/$(DIR)/Box2D/Particle/*.cc))
