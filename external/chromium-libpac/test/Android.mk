LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_CPP_EXTENSION := .cc

# Set up the target identity
LOCAL_MODULE := proxy_resolver_v8_unittest

LOCAL_SRC_FILES := \
  proxy_resolver_v8_unittest.cc

LOCAL_CFLAGS += \
  -Wno-endif-labels \
  -Wno-import \
  -Wno-format \

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../src $(LOCAL_PATH)/ external/v8

LOCAL_SHARED_LIBRARIES := libpac libutils liblog

include $(BUILD_NATIVE_TEST)
