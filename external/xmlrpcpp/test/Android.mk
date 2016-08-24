LOCAL_PATH := $(call my-dir)
ifeq ($(HOST_OS), linux)

include $(CLEAR_VARS)
LOCAL_MODULE := libxmlrpc++-tests-helloserver

LOCAL_CLANG := true
LOCAL_RTTI_FLAG := -frtti
LOCAL_CPPFLAGS := -Wall -fexceptions
LOCAL_C_INCLUDES = $(LOCAL_PATH)/../src
LOCAL_SHARED_LIBRARIES := libxmlrpc++

xmlrpc_test_files := $(LOCAL_PATH)/HelloServer.cpp
LOCAL_SRC_FILES := \
    $(xmlrpc_test_files:$(LOCAL_PATH)/%=%)
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := libxmlrpc++-tests-helloclient

LOCAL_CLANG := true
LOCAL_RTTI_FLAG := -frtti
LOCAL_CPPFLAGS := -Wall -fexceptions
LOCAL_C_INCLUDES = $(LOCAL_PATH)/../src
LOCAL_SHARED_LIBRARIES := libxmlrpc++

xmlrpc_test_files := $(LOCAL_PATH)/HelloClient.cpp
LOCAL_SRC_FILES := \
    $(xmlrpc_test_files:$(LOCAL_PATH)/%=%)
include $(BUILD_EXECUTABLE)

endif # HOST_OS == linux
