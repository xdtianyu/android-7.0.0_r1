libavce_inc_dir_mips64  +=  $(LOCAL_PATH)/common/mips
libavce_inc_dir_mips64  +=  $(LOCAL_PATH)/encoder/mips

libavce_srcs_c_mips64   +=  encoder/mips/ih264e_function_selector.c

LOCAL_C_INCLUDES_mips64 += $(libavce_inc_dir_mips64)
LOCAL_SRC_FILES_mips64  += $(libavce_srcs_c_mips64)
