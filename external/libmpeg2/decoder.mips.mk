libmpeg2d_inc_dir_mips  +=  $(LOCAL_PATH)/common/mips

libmpeg2d_srcs_c_mips   +=  decoder/mips/impeg2d_function_selector.c
libmpeg2d_srcs_c_mips   +=  common/mips/ideint_function_selector.c
LOCAL_C_INCLUDES_mips   += $(libmpeg2d_inc_dir_mips)
LOCAL_SRC_FILES_mips    += $(libmpeg2d_srcs_c_mips)
