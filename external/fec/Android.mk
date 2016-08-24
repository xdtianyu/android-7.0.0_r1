LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfec_rs
LOCAL_CLANG := true
LOCAL_CFLAGS := -Wall -O3
LOCAL_SANITIZE := integer
LOCAL_SRC_FILES := \
	encode_rs_char.c \
	decode_rs_char.c \
	init_rs_char.c
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libfec_rs_host
LOCAL_CLANG := true
LOCAL_CFLAGS := -Wall -O3
ifeq ($(HOST_OS),linux)
LOCAL_SANITIZE := integer
endif
LOCAL_SRC_FILES := \
	encode_rs_char.c \
	decode_rs_char.c \
	init_rs_char.c
include $(BUILD_HOST_STATIC_LIBRARY)
