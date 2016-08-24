LOCAL_PATH:= $(call my-dir)

# --------------------------------------------------------------
# Use prebuilt aprotoc to compile .proto files

ifeq ($(HOST_OS),darwin)
PROTOC := $(TOPDIR)prebuilts/misc/darwin-x86/protobuf/aprotoc
else
PROTOC := $(TOPDIR)prebuilts/misc/linux-x86/protobuf/aprotoc
endif

# --------------------------------------------------------------
# Build the apk. This generates an standalone apk for USB tuner
# input service.
include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := UsbTunerTvInput
LOCAL_MODULE_TAGS := optional
# It's not required but keep it for a compatibility with the previous version.
LOCAL_PRIVILEGED_MODULE := true
LOCAL_SDK_VERSION := system_current
LOCAL_MIN_SDK_VERSION := 23  # M

LOCAL_STATIC_JAVA_LIBRARIES := \
    lib-exoplayer \
    usbtuner-tvinput

LOCAL_RESOURCE_DIR := \
    $(LOCAL_PATH)/res \
    $(LOCAL_PATH)/../common/res \
    $(TOP)/prebuilts/sdk/current/support/v7/recyclerview/res \
    $(TOP)/prebuilts/sdk/current/support/v17/leanback/res

LOCAL_AAPT_FLAGS := --auto-add-overlay \
    --extra-packages android.support.v7.recyclerview \
    --extra-packages android.support.v17.leanback \
    --extra-packages com.android.tv.common

LOCAL_JNI_SHARED_LIBRARIES := \
    libusbtuner_jni

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)

# --------------------------------------------------------------
# The final static library that apps can link against.
# The R class is automatically excluded from the generated library.
# Applications that use this library must specify LOCAL_RESOURCE_DIR
# in their makefiles to include the resources in their package.

include $(CLEAR_VARS)

LOCAL_MODULE := usbtuner-tvinput
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    $(call all-java-files-under, src) \
    $(call all-proto-files-under, proto)
LOCAL_SDK_VERSION := system_current
LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-v4 \
    android-support-v7-recyclerview \
    android-support-v17-leanback \
    icu4j-usbtuner \
    lib-exoplayer \
    libprotobuf-java-nano \
    tv-common


LOCAL_PROGUARD_ENABLED := disabled

LOCAL_RESOURCE_DIR := \
    $(LOCAL_PATH)/res \
    $(LOCAL_PATH)/../common/res \
    $(TOP)/prebuilts/sdk/current/support/v7/recyclerview/res \
    $(TOP)/prebuilts/sdk/current/support/v17/leanback/res

LOCAL_AAPT_FLAGS := --auto-add-overlay \
    --extra-packages android.support.v17.leanback \
    --extra-packages com.android.tv.common \

LOCAL_PROTOC_OPTIMIZE_TYPE := nano
LOCAL_PROTOC_FLAGS := --proto_path=$(LOCAL_PATH)/proto/

include $(BUILD_STATIC_JAVA_LIBRARY)

# --------------------------------------------------------------
# Build a tiny icu4j library out of the classes necessary for the project.

include $(CLEAR_VARS)

LOCAL_MODULE := icu4j-usbtuner
LOCAL_MODULE_TAGS := optional
icu4j_path := icu/icu4j
LOCAL_SRC_FILES := \
    $(icu4j_path)/main/classes/core/src/com/ibm/icu/text/SCSU.java \
    $(icu4j_path)/main/classes/core/src/com/ibm/icu/text/UnicodeDecompressor.java
LOCAL_SDK_VERSION := system_current

include $(BUILD_STATIC_JAVA_LIBRARY)
#############################################################
# Pre-built dependency jars
#############################################################

# --------------------------------------------------------------
# ExoPlayer library version 1.5.6
# https://github.com/google/ExoPlayer/archive/r1.5.6.zip
# TODO: Add ExoPlayer source code to external/ android repository.

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \
   lib-exoplayer:libs/exoplayer_1.5.6.jar

include $(BUILD_MULTI_PREBUILT)

include $(call all-makefiles-under, $(LOCAL_PATH))
