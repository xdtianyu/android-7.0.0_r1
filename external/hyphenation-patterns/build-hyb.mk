
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .hyb
LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/hyphen-data

include $(BUILD_SYSTEM)/base_rules.mk

MK_HYB_FILE := frameworks/minikin/tools/mk_hyb_file.py
$(LOCAL_BUILT_MODULE) : $(addprefix $(LOCAL_PATH)/, $(LOCAL_SRC_FILES)) $(MK_HYB_FILE)
	@echo "Build hyb $@ <- $<"
	@mkdir -p $(dir $@)
	$(hide) $(MK_HYB_FILE) $< $@
