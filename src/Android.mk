
ROOT := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PATH = $(ROOT)

LOCAL_MODULE := main
SDL_PATH := $(ROOT)/SDL-mobile/SDL
TMS_PATH := $(ROOT)/tms

$(warning $(ROOT))

LOCAL_SRC_FILES = 

LOCAL_CPP_EXTENSION := .cc

LOCAL_C_INCLUDES += $(SDL_PATH)/include $(ROOT)/curl-7.32.0/include $(ROOT)/curl-7.32.0/lib $(TMS_PATH)/backends/android/ \
					$(ROOT)/SDL-mobile/ $(ROOT)/SDL_ttf/ $(ROOT)/SDL_image/ $(ROOT)/SDL_mixer/ $(ROOT)/freetype/builds $(ROOT)/freetype/include \
					$(ROOT)/src/ $(ROOT)/jpeg/ $(ROOT)/png/ $(ROOT)/lua/src/

GLOBAL_FLAGS := -DGL_GLEXT_PROTOTYPES \
				-DANDROID \
				-D__ANDROID__ \
				-DSDL_NO_COMPAT \
				-DUNICODE \
				-DTMS_BACKEND_ANDROID \
				-DTMS_BACKEND_MOBILE \
				-Os -ffast-math

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	GLOBAL_FLAGS += -DTMS_BACKEND_ANDROID_ARMEABI_V7A
endif
ifeq ($(TARGET_ARCH_ABI),armeabi)
	GLOBAL_FLAGS += -DTMS_BACKEND_ANDROID_ARMEABI
endif
ifeq ($(TARGET_ARCH_ABI),x86)
	GLOBAL_FLAGS += -DTMS_BACKEND_ANDROID_X86
endif

LOCAL_CONLYFLAGS := -std=gnu99
LOCAL_CFLAGS += $(GLOBAL_FLAGS) -DFT2_BUILD_LIBRARY -D__STDC_FORMAT_MACROS=1
LOCAL_CPPFLAGS += $(GLOBAL_FLAGS)
LOCAL_LDLIBS := -lz -ldl -lGLESv1_CM -lGLESv2 -llog

# for SDL_image
LOCAL_CFLAGS += -DLOAD_JPG -DLOAD_PNG -DLOAD_BMP -DLOAD_GIF -DLOAD_LBM \
	-DLOAD_PCX -DLOAD_PNM -DLOAD_TGA -DLOAD_XCF -DLOAD_XPM \
	-DLOAD_XV

include $(ROOT)/curl-7.32.0/Android.mk
include $(ROOT)/jpeg/Android.mk
include $(ROOT)/png/Android.mk
include $(ROOT)/freetype/Android.mk
include $(ROOT)/lua/Android.mk
include $(ROOT)/SDL_image/Android.mk
include $(ROOT)/SDL_ttf/Android.mk
include $(ROOT)/SDL_mixer/Android.mk
include $(TMS_PATH)/Android.mk
include $(SDL_PATH)/Android.mk
include $(ROOT)/src/Android.mk
#include $(call all-subdir-makefiles)

# reset path before building

$(warning $(LOCAL_SRC_FILES))

LOCAL_PATH := $(ROOT)

$(warning $(LOCAL_PATH))

include $(BUILD_SHARED_LIBRARY)
