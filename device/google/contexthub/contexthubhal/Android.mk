LOCAL_PATH := $(call my-dir)

# HAL module implemenation stored in
# hw/<CONTEXT_HUB_MODULE_ID>.<ro.hardware>.so
include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MULTILIB := both
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_SRC_FILES := nanohubhal.cpp system_comms.cpp
LOCAL_CFLAGS := -Wall -Werror -Wextra
LOCAL_MODULE_OWNER := google

# Include target-specific files.
LOCAL_SRC_FILES += nanohubhal_default.cpp

LOCAL_MODULE := context_hub.default
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
