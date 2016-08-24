libhevcd_inc_dir_mips   +=  $(LOCAL_PATH)/decoder/mips
libhevcd_inc_dir_mips   +=  $(LOCAL_PATH)/common/mips

libhevcd_srcs_c_mips    +=  decoder/mips/ihevcd_function_selector.c
libhevcd_srcs_c_mips    +=  decoder/mips/ihevcd_function_selector_mips_generic.c


LOCAL_SRC_FILES_mips += $(libhevcd_srcs_c_mips) $(libhevcd_srcs_asm_mips)
LOCAL_C_INCLUDES_mips += $(libhevcd_inc_dir_mips)
LOCAL_CFLAGS_mips += $(libhevcd_cflags_mips)



