LOCAL_PATH := $(call my-dir)

utf_src := \
    rune.c \
    runestrcat.c \
    runestrchr.c \
    runestrcmp.c \
    runestrcpy.c \
    runestrdup.c \
    runestrlen.c \
    runestrecpy.c \
    runestrncat.c \
    runestrncmp.c \
    runestrncpy.c \
    runestrrchr.c \
    runestrstr.c \
    runetype.c \
    utfecpy.c \
    utflen.c \
    utfnlen.c \
    utfrrune.c \
    utfrune.c \
    utfutf.c

utf_cflags := -Wall -Wno-missing-braces -Wno-parentheses -Wno-switch

# We only build the static library at the moment.
include $(CLEAR_VARS)

LOCAL_MODULE := libutf
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(utf_src)
LOCAL_CFLAGS += -O3 $(utf_cflags)
LOCAL_ARM_MODE := arm
LOCAL_SDK_VERSION := 14

include $(BUILD_STATIC_LIBRARY)

