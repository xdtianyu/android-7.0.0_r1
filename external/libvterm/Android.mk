LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libvterm
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
    src/input.c \
    src/vterm.c \
    src/encoding.c \
    src/parser.c \
    src/unicode.c \
    src/pen.c \
    src/screen.c \
    src/state.c

LOCAL_CFLAGS += \
    -std=c99 \
    -Wno-missing-field-initializers \
    -Wno-sign-compare \
    -Wno-unused-parameter \

LOCAL_CLANG := true

include $(BUILD_STATIC_LIBRARY)
