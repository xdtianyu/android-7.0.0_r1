LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libxmlrpc++
LOCAL_MODULE_HOST_OS := linux

LOCAL_CLANG := true
LOCAL_RTTI_FLAG := -frtti
LOCAL_CPPFLAGS := -Wall -fexceptions
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/src

LOCAL_SRC_FILES := $(call all-cpp-files-under,src)
include $(BUILD_SHARED_LIBRARY)
