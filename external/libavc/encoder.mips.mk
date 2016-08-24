libavce_inc_dir_mips    +=  $(LOCAL_PATH)/common/mips
libavce_inc_dir_mips    +=  $(LOCAL_PATH)/encoder/mips

libavce_srcs_c_mips     +=  encoder/mips/ih264e_function_selector.c

LOCAL_C_INCLUDES_mips   += $(libavce_inc_dir_mips)
LOCAL_SRC_FILES_mips    += $(libavce_srcs_c_mips)
