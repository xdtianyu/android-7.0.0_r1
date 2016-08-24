# Build the unit tests.
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_SHARED_LIBRARIES := \
    libOpenSLES \

LOCAL_C_INCLUDES := $(call include-path-for, wilhelm)
LOCAL_SRC_FILES := mimeUri_test.cpp
LOCAL_MODULE := libopenslestests
LOCAL_MODULE_TAGS := tests
LOCAL_CFLAGS := -Werror -Wall
include $(BUILD_NATIVE_TEST)

# Build the manual test programs.
include $(call all-makefiles-under,$(LOCAL_PATH))
