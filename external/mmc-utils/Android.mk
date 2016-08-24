LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_SRC_FILES:= mmc.c mmc_cmds.c
LOCAL_SRC_FILES += 3rdparty/hmac_sha/sha2.c 3rdparty/hmac_sha/hmac_sha2.c
LOCAL_MODULE := mmc_utils
LOCAL_SHARED_LIBRARIES := libcutils libc
include $(BUILD_EXECUTABLE)
