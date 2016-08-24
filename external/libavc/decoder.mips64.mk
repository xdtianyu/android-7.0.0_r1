libavcd_inc_dir_mips64  +=  $(LOCAL_PATH)/common/mips

libavcd_srcs_c_mips64   +=  decoder/mips/ih264d_function_selector.c

LOCAL_C_INCLUDES_mips64 += $(libavcd_inc_dir_mips64)
LOCAL_SRC_FILES_mips64  += $(libavcd_srcs_c_mips64)
