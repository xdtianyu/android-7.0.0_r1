LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-subdir-java-files)

#LOCAL_JAVA_STATIC_LIBRARIES := android-common
#LOCAL_JAVA_LIBRARIES := android.common

LOCAL_MODULE := ksoap2
LOCAL_JAVA_LIBRARIES := okhttp
include $(BUILD_STATIC_JAVA_LIBRARY)

# additionally, build unit tests in a separate .apk
include $(call all-makefiles-under,$(LOCAL_PATH))

