ifeq ($(strip $(TARGET_USES_QCOM_MM_AUDIO)),true)
include $(call all-subdir-makefiles)
endif
