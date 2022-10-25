
ROOT := $(call my-dir)

LOCAL_PATH = $(ROOT)
DEPROOT := ../build-android/deps/$(TARGET_ARCH_ABI)
include $(CLEAR_VARS)

include $(CLEAR_VARS)
LOCAL_MODULE := libcurl
LOCAL_SRC_FILES := $(DEPROOT)/curl/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libmbedcrypto
LOCAL_SRC_FILES := $(DEPROOT)/curl/libmbedcrypto.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libmbedtls
LOCAL_SRC_FILES := $(DEPROOT)/curl/libmbedtls.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libmbedx509
LOCAL_SRC_FILES := $(DEPROOT)/curl/libmbedx509.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := freetype
LOCAL_SRC_FILES := $(DEPROOT)/freetype/libfreetype.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjpeg
LOCAL_SRC_FILES := $(DEPROOT)/libjpeg/libjpeg.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libpng
LOCAL_SRC_FILES := $(DEPROOT)/libpng/libpng.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := main
SDL_PATH := $(ROOT)/SDL-mobile/SDL
TMS_PATH := $(ROOT)/tms

$(warning $(ROOT))

LOCAL_CPP_EXTENSION := .cc

LOCAL_C_INCLUDES += \
	$(SDL_PATH)/include \
	$(TMS_PATH)/backends/android/ \
	$(ROOT)/SDL-mobile/ \
	$(ROOT)/SDL_ttf/ \
	$(ROOT)/SDL_image/ \
	$(ROOT)/SDL_mixer/ \
	$(ROOT)/src/ $(ROOT)/lua/src/ \
	../deps/$(TARGET_ARCH_ABI)/curl/include/ \
	../deps/$(TARGET_ARCH_ABI)/freetype/include/freetype2/ \
	../deps/$(TARGET_ARCH_ABI)/freetype/include/ \
	../deps/$(TARGET_ARCH_ABI)/libjpeg/include/ \
	../deps/$(TARGET_ARCH_ABI)/libpng/include/

GLOBAL_FLAGS := -DGL_GLEXT_PROTOTYPES \
				-DANDROID \
				-D__ANDROID__ \
				-DSDL_NO_COMPAT \
				-DUNICODE \
				-DTMS_BACKEND_ANDROID \
				-DTMS_BACKEND_MOBILE \
                -DHAVE_GCC_ATOMICS \
				-Os -ffast-math

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
	GLOBAL_FLAGS += -DTMS_BACKEND_ANDROID_ARM64_V8A
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
	GLOBAL_FLAGS += -DTMS_BACKEND_ANDROID_X86_64
endif
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
LOCAL_STATIC_LIBRARIES := \
	freetype libjpeg libpng \
	libcurl libmbedtls libmbedx509 libmbedcrypto
LOCAL_LDLIBS := -lz -ldl -lGLESv1_CM -lGLESv2 -llog

# for SDL_image
LOCAL_CFLAGS += -DLOAD_JPG -DLOAD_PNG -DLOAD_BMP -DLOAD_GIF -DLOAD_LBM \
	-DLOAD_PCX -DLOAD_PNM -DLOAD_TGA -DLOAD_XCF -DLOAD_XPM \
	-DLOAD_XV

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
