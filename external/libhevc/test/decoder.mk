LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := hevcdec
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -DPROFILE_ENABLE -DARM  -fPIC -DMD5_DISABLE
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../decoder $(LOCAL_PATH)/../common $(LOCAL_PATH)/
LOCAL_SRC_FILES := decoder/main.c
LOCAL_STATIC_LIBRARIES := libhevcdec

include $(BUILD_EXECUTABLE)
