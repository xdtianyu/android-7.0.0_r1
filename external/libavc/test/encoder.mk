LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := avcenc
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -DPROFILE_ENABLE -DARM -DMD5_DISABLE -fPIC -pie
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../encoder $(LOCAL_PATH)/../common $(LOCAL_PATH)/encoder/
LOCAL_SRC_FILES := encoder/main.c encoder/psnr.c encoder/input.c encoder/output.c encoder/recon.c
LOCAL_STATIC_LIBRARIES := libavcenc

include $(BUILD_EXECUTABLE)
