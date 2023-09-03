LOCAL_SRC_FILES += \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/tms/core/*.c) \
	$(wildcard $(LOCAL_PATH)/tms/math/*.c) \
	$(wildcard $(LOCAL_PATH)/tms/util/*.c) )\
	tms/backends/android/main.c \
	tms/bindings/cpp/cpp.cc\
	tms/modules/3ds.c
