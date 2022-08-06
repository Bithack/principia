SDL_ROOT := $(LOCAL_PATH)/SDL-mobile/SDL

#$(error $(SDL_ROOT))
#$(error $(subst $(LOCAL_PATH)/,, $(wildcard $(SDL_ROOT)/src/*.c)))

LOCAL_SRC_FILES += \
	$(subst $(LOCAL_PATH)/,, $(SDL_ROOT)/src/main/android/SDL_android_main.cc ) \
	$(subst $(LOCAL_PATH)/,, $(SDL_ROOT)/src/atomic/SDL_atomic.c ) \
	$(subst $(LOCAL_PATH)/,, $(SDL_ROOT)/src/atomic/SDL_spinlock.c.arm ) \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(SDL_ROOT)/src/*.c) \
	$(wildcard $(SDL_ROOT)/src/audio/*.c) \
	$(wildcard $(SDL_ROOT)/src/audio/android/*.c) \
	$(wildcard $(SDL_ROOT)/src/audio/dummy/*.c) \
	$(wildcard $(SDL_ROOT)/src/core/android/*.cc) \
	$(wildcard $(SDL_ROOT)/src/cpuinfo/*.c) \
	$(wildcard $(SDL_ROOT)/src/events/*.c) \
	$(wildcard $(SDL_ROOT)/src/file/*.c) \
	$(wildcard $(SDL_ROOT)/src/haptic/*.c) \
	$(wildcard $(SDL_ROOT)/src/haptic/dummy/*.c) \
	$(wildcard $(SDL_ROOT)/src/joystick/*.c) \
	$(wildcard $(SDL_ROOT)/src/joystick/android/*.c) \
	$(wildcard $(SDL_ROOT)/src/loadso/dlopen/*.c) \
	$(wildcard $(SDL_ROOT)/src/power/*.c) \
	$(wildcard $(SDL_ROOT)/src/power/android/*.c) \
	$(wildcard $(SDL_ROOT)/src/render/*.c) \
	$(wildcard $(SDL_ROOT)/src/render/*/*.c) \
	$(wildcard $(SDL_ROOT)/src/stdlib/*.c) \
	$(wildcard $(SDL_ROOT)/src/thread/*.c) \
	$(wildcard $(SDL_ROOT)/src/thread/pthread/*.c) \
	$(wildcard $(SDL_ROOT)/src/timer/*.c) \
	$(wildcard $(SDL_ROOT)/src/timer/unix/*.c) \
	$(wildcard $(SDL_ROOT)/src/video/*.c) \
	$(wildcard $(SDL_ROOT)/src/video/android/*.c))

#<<<<<<< local
#$(warning tja 1)
#$(warning $(subst $(LOCAL_PATH)/,, $(wildcard jni/SDL/src/video/android/*.c)))
#$(warning tja 2)
#=======
#include $(CLEAR_VARS)

#LOCAL_MODULE := SDL2
#
#LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
#
#LOCAL_SRC_FILES := \
##	$(subst $(LOCAL_PATH)/,, \
#	$(wildcard $(LOCAL_PATH)/src/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/audio/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/audio/android/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/audio/dummy/*.c) \
#	$(LOCAL_PATH)/src/atomic/SDL_atomic.c \
#	$(LOCAL_PATH)/src/atomic/SDL_spinlock.c.arm \
#	$(wildcard $(LOCAL_PATH)/src/core/android/*.cpp) \
#	$(wildcard $(LOCAL_PATH)/src/cpuinfo/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/events/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/file/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/haptic/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/haptic/dummy/*.c) \
##	$(wildcard $(LOCAL_PATH)/src/joystick/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/joystick/android/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/loadso/dlopen/*.c) \
##	$(wildcard $(LOCAL_PATH)/src/power/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/power/android/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/render/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/render/*/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/stdlib/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/thread/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/thread/pthread/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/timer/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/timer/unix/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/video/*.c) \
#	$(wildcard $(LOCAL_PATH)/src/video/android/*.c))
#
#LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES
#LOCAL_LDLIBS := -ldl -lGLESv1_CM -lGLESv2 -llog
#
#include $(BUILD_SHARED_LIBRARY)
##>>>>>>> other
