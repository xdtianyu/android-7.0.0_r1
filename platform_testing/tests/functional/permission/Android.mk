LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_SDK_VERSION := current

LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_JAVA_LIBRARIES := android.test.runner
LOCAL_STATIC_JAVA_LIBRARIES := ub-uiautomator launcher-helper-lib

LOCAL_PACKAGE_NAME := PermissionFunctionalTests
LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)
