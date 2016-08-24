LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := mpeg2dec
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -DPROFILE_ENABLE -DMD5_DISABLE -DARM  -fPIC
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../decoder $(LOCAL_PATH)/../common $(LOCAL_PATH)/decoder/
LOCAL_SRC_FILES := decoder/main.c
LOCAL_STATIC_LIBRARIES := libmpeg2dec

include $(BUILD_EXECUTABLE)
