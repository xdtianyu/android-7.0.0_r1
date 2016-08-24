
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := OtaFunctionalTests
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_MODULE_TAGS := tests
LOCAL_STATIC_JAVA_LIBRARIES := \
    easymocklib \
    objenesis-target \
    ub-uiautomator

include $(BUILD_PACKAGE)
