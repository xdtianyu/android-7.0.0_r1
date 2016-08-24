ifneq ($(TARGET_BOARD_AUTO),true)
  ifeq ($(BOARD_WLAN_DEVICE),qcwcn)
    include $(call all-subdir-makefiles)
  endif
endif
