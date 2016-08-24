ifneq ($(BOARD_GPT_INI),)

LOCAL_PATH := $(call my-dir)
PRIVATE_TOOL_PATH := $(LOCAL_PATH)/gpt_ini2bin.py

include $(CLEAR_VARS)

LOCAL_MODULE := gpt.bin
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(PRODUCT_OUT)

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): $(BOARD_GPT_INI) | $(PRIVATE_TOOL_PATH)
	$(hide) $(PRIVATE_TOOL_PATH) $^ > $@

endif
