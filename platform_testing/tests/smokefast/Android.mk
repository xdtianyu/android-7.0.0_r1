LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_SDK_VERSION := current

media_framework_app_base := frameworks/base/media/tests/MediaFrameworkTest/src/com/android/mediaframeworktest

LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_JAVA_LIBRARIES := android.test.runner
LOCAL_STATIC_JAVA_LIBRARIES := android-support-test ub-uiautomator
# TODO: AuptLib and aupt-helpers should be static deps as well

LOCAL_PACKAGE_NAME := SmokeFastTests
LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)
