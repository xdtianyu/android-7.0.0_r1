LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, library/src)
LOCAL_SRC_FILES += $(call all-java-files-under, third_party/disklrucache/src)
LOCAL_SRC_FILES += $(call all-java-files-under, third_party/gif_decoder/src)
LOCAL_SRC_FILES += $(call all-java-files-under, third_party/gif_encoder/src)
LOCAL_MANIFEST_FILE := library/src/main/AndroidManifest.xml

LOCAL_STATIC_JAVA_LIBRARIES := volley
LOCAL_STATIC_JAVA_LIBRARIES += android-support-v4

LOCAL_MODULE := glide
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := 19

include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
