LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# encoder
include $(LOCAL_PATH)/encoder.mk

# decoder
include $(LOCAL_PATH)/decoder.mk
