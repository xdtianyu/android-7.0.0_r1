# SVOX Pico TTS Engine
# This makefile builds both an activity and a shared library.

#disable build in PDK
ifneq ($(TARGET_BUILD_PDK),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := PicoTts

LOCAL_SRC_FILES := \
    $(call all-java-files-under, src) \
    $(call all-java-files-under, compat)

LOCAL_JNI_SHARED_LIBRARIES := libttscompat libttspico
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

include $(BUILD_PACKAGE)


include $(LOCAL_PATH)/compat/jni/Android.mk \
    $(LOCAL_PATH)/lib/Android.mk \
    $(LOCAL_PATH)/tts/Android.mk

endif #TARGET_BUILD_PDK
