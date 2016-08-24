libhevcd_inc_dir_mips64 +=  $(LOCAL_PATH)/decoder/mips
libhevcd_inc_dir_mips64 +=  $(LOCAL_PATH)/common/mips

libhevcd_srcs_c_mips64  +=  decoder/mips/ihevcd_function_selector.c
libhevcd_srcs_c_mips64  +=  decoder/mips/ihevcd_function_selector_mips_generic.c

LOCAL_SRC_FILES_mips64 += $(libhevcd_srcs_c_mips64) $(libhevcd_srcs_asm_mips64)
LOCAL_C_INCLUDES_mips64 += $(libhevcd_inc_dir_mips64)
LOCAL_CFLAGS_mips64 += $(libhevcd_cflags_mips64)

