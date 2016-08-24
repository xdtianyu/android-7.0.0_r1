LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/Makefile.sources

LOCAL_SRC_FILES := $(filter-out %.h,$(PLANETEST_COMMON_FILES) $(PLANETEST_FILES))

LOCAL_MODULE := planetest

LOCAL_SHARED_LIBRARIES := libdrm

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/Makefile.sources

LOCAL_SRC_FILES := $(filter-out %.h,$(PLANETEST_COMMON_FILES) $(ATOMICTEST_FILES))

LOCAL_MODULE := atomictest

LOCAL_SHARED_LIBRARIES := libdrm

include $(BUILD_EXECUTABLE)
