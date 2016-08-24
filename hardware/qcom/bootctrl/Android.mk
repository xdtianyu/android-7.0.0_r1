ifneq ($(filter msm8996,$(TARGET_BOARD_PLATFORM)),)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += hardware/libhardware/include
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/gpt-utils/inc
LOCAL_CFLAGS += -Wall -Werror
LOCAL_SHARED_LIBRARIES += liblog librecovery_updater_msm
LOCAL_SRC_FILES := boot_control.c
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := bootctrl.$(TARGET_BOARD_PLATFORM)
include $(BUILD_SHARED_LIBRARY)
endif
