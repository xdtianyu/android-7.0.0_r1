libmpeg2d_inc_dir_arm   +=  $(LOCAL_PATH)/decoder/arm
libmpeg2d_inc_dir_arm   +=  $(LOCAL_PATH)/common/arm

libmpeg2d_srcs_c_arm    +=  decoder/arm/impeg2d_function_selector.c
libmpeg2d_srcs_c_arm    +=  common/arm/ideint_function_selector.c
libmpeg2d_cflags_arm    += -DDISABLE_NEONINTR  -DARM -DARMGCC

LOCAL_ARM_MODE         := arm

ifeq ($(ARCH_ARM_HAVE_NEON),true)
libmpeg2d_srcs_c_arm    +=  decoder/arm/impeg2d_function_selector_a9q.c
libmpeg2d_srcs_c_arm    +=  common/arm/ideint_function_selector_a9.c
libmpeg2d_srcs_asm_arm    +=  common/arm/icv_sad_a9.s
libmpeg2d_srcs_asm_arm    +=  common/arm/icv_variance_a9.s
libmpeg2d_srcs_asm_arm    +=  common/arm/ideint_spatial_filter_a9.s
libmpeg2d_srcs_asm_arm    +=  common/arm/ideint_cac_a9.s
libmpeg2d_srcs_asm_arm    +=  common/arm/impeg2_format_conv.s
libmpeg2d_srcs_asm_arm    +=  common/arm/impeg2_idct.s
libmpeg2d_srcs_asm_arm    +=  common/arm/impeg2_inter_pred.s
libmpeg2d_srcs_asm_arm    +=  common/arm/impeg2_mem_func.s
libmpeg2d_cflags_arm += -DDEFAULT_ARCH=D_ARCH_ARM_A9Q
else
libmpeg2d_cflags_arm += -DDISABLE_NEON -DDEFAULT_ARCH=D_ARCH_ARM_NONEON
endif

LOCAL_SRC_FILES_arm += $(libmpeg2d_srcs_c_arm) $(libmpeg2d_srcs_asm_arm)
LOCAL_C_INCLUDES_arm += $(libmpeg2d_inc_dir_arm)
LOCAL_CFLAGS_arm += $(libmpeg2d_cflags_arm)

# CLANG WORKAROUNDS
LOCAL_CLANG_ASFLAGS_arm += -no-integrated-as
LOCAL_CLANG_ASFLAGS_arm += $(addprefix -Wa$(comma)-I,$(libmpeg2d_inc_dir_arm))
