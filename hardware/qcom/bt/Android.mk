# TODO:  Find a better way to separate build configs for ADP vs non-ADP devices
ifneq ($(TARGET_BOARD_AUTO),true)
  ifneq ($(filter msm8960 msm8x27 msm8226,$(TARGET_BOARD_PLATFORM)),)
    include $(call all-named-subdir-makefiles,msm8960)
  else
    ifneq ($(filter msm8994 msm8992,$(TARGET_BOARD_PLATFORM)),)
      include $(call all-named-subdir-makefiles,msm8992)
    else
      ifneq ($(filter msm8996,$(TARGET_BOARD_PLATFORM)),)
        include $(call all-named-subdir-makefiles,msm8996)
      else
        ifneq ($(filter msm8909 ,$(TARGET_BOARD_PLATFORM)),)
          #For msm8909 target
          include $(call all-named-subdir-makefiles,msm8909)
        endif
      endif
    endif
  endif
endif
