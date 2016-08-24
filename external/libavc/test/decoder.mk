LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := avcdec
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -DPROFILE_ENABLE -DARM  -DMD5_DISABLE -fPIC
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../decoder $(LOCAL_PATH)/../common $(LOCAL_PATH)/decoder/
LOCAL_SRC_FILES := decoder/main.c
LOCAL_STATIC_LIBRARIES := libavcdec

include $(BUILD_EXECUTABLE)
