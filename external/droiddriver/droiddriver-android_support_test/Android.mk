LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_MODULE := droiddriver-android_support_test
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := 19

LOCAL_JAVACFLAGS += -Xlint:deprecation -Xlint:unchecked

# android-support-test requires /frameworks/testing, /external/junit, /external/hamcrest
LOCAL_JAVA_LIBRARIES := droiddriver android-support-test

include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)

include $(call all-makefiles-under,$(LOCAL_PATH))
