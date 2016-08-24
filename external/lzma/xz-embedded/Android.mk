# Copyright 2013 The Android Open Source Project
#
LOCAL_PATH := $(call my-dir)

xz_embedded_sources := \
    xz_crc32.c \
    xz_dec_bcj.c \
    xz_dec_lzma2.c \
    xz_dec_stream.c

include $(CLEAR_VARS)
LOCAL_MODULE := libxz
LOCAL_SRC_FILES := $(xz_embedded_sources)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libxz-host
LOCAL_SRC_FILES := $(xz_embedded_sources)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_HOST_STATIC_LIBRARY)
