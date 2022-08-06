LOCAL_SRC_FILES += $(subst $(LOCAL_PATH)/,,$(filter-out %/showimage.c, $(wildcard $(LOCAL_PATH)/SDL_image/*.c)))
