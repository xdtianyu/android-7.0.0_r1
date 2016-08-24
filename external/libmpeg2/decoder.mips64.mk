libmpeg2d_inc_dir_mips64    +=  $(LOCAL_PATH)/common/mips

libmpeg2d_srcs_c_mips64     +=  decoder/mips/impeg2d_function_selector.c
libmpeg2d_srcs_c_mips64     +=  common/mips/ideint_function_selector.c
LOCAL_C_INCLUDES_mips64     += $(libmpeg2d_inc_dir_mips)
LOCAL_SRC_FILES_mips64      += $(libmpeg2d_srcs_c_mips)
