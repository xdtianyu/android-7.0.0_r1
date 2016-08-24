LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_SDK_VERSION := current

LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_JAVA_LIBRARIES := android.test.runner
LOCAL_STATIC_JAVA_LIBRARIES := launcher-helper-lib ub-uiautomator app-helpers

LOCAL_PACKAGE_NAME := ExternalStorageFunctionalTests
LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)