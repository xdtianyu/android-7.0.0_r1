libavcd_inc_dir_mips    +=  $(LOCAL_PATH)/common/mips

libavcd_srcs_c_mips     +=  decoder/mips/ih264d_function_selector.c

LOCAL_C_INCLUDES_mips   += $(libavcd_inc_dir_mips)
LOCAL_SRC_FILES_mips    += $(libavcd_srcs_c_mips)
