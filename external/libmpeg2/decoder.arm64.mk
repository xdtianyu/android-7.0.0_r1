libmpeg2d_cflags_arm64 += -DARMV8
libmpeg2d_cflags_arm64 += -DDISABLE_NEONINTR  -DARM -DARMGCC

libmpeg2d_inc_dir_arm64   +=  $(LOCAL_PATH)/decoder/arm
libmpeg2d_inc_dir_arm64   +=  $(LOCAL_PATH)/common/armv8

libmpeg2d_srcs_c_arm64    +=  decoder/arm/impeg2d_function_selector.c

ifeq ($(ARCH_ARM_HAVE_NEON),true)
libmpeg2d_srcs_c_arm64      +=  decoder/arm/impeg2d_function_selector_av8.c
libmpeg2d_srcs_c_arm64      +=  common/arm/ideint_function_selector.c
libmpeg2d_srcs_c_arm64      +=  common/arm/ideint_function_selector_av8.c
libmpeg2d_srcs_asm_arm64    +=  common/armv8/icv_sad_av8.s
libmpeg2d_srcs_asm_arm64    +=  common/armv8/icv_variance_av8.s
libmpeg2d_srcs_asm_arm64    +=  common/armv8/ideint_spatial_filter_av8.s
libmpeg2d_srcs_asm_arm64    +=  common/armv8/ideint_cac_av8.s

libmpeg2d_srcs_asm_arm64    +=  common/armv8/impeg2_neon_macros.s
libmpeg2d_srcs_asm_arm64    +=  common/armv8/impeg2_format_conv.s
libmpeg2d_srcs_asm_arm64    +=  common/armv8/impeg2_idct.s
libmpeg2d_srcs_asm_arm64    +=  common/armv8/impeg2_inter_pred.s
libmpeg2d_srcs_asm_arm64    +=  common/armv8/impeg2_mem_func.s
libmpeg2d_cflags_arm += -DDEFAULT_ARCH=D_ARCH_ARMV8_GENERIC
else
libmpeg2d_cflags_arm64 += -DDISABLE_NEON -DDEFAULT_ARCH=D_ARCH_ARM_NONEON
endif




LOCAL_SRC_FILES_arm64 += $(libmpeg2d_srcs_c_arm64) $(libmpeg2d_srcs_asm_arm64)
LOCAL_C_INCLUDES_arm64 += $(libmpeg2d_inc_dir_arm64)
LOCAL_CFLAGS_arm64 += $(libmpeg2d_cflags_arm64)

# CLANG WORKAROUNDS
LOCAL_CLANG_ASFLAGS_arm64 += -no-integrated-as
LOCAL_CLANG_ASFLAGS_arm64 += $(addprefix -Wa$(comma)-I,$(libmpeg2d_inc_dir_arm64))
