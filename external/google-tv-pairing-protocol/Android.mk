LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := google-tv-pairing-protocol

LOCAL_SRC_FILES := \
	        $(call all-java-files-under, java/src) \
	        $(call all-proto-files-under, proto)

LOCAL_PROTOC_OPTIMIZE_TYPE := nano

LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := current

LOCAL_STATIC_JAVA_LIBRARIES := \
    bouncycastle-unbundled

include $(BUILD_STATIC_JAVA_LIBRARY)

