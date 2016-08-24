libavcd_cflags_arm64 += -DARMV8
libavcd_cflags_arm64 += -DARM

libavcd_inc_dir_arm64   +=  $(LOCAL_PATH)/decoder/arm
libavcd_inc_dir_arm64   +=  $(LOCAL_PATH)/common/armv8

libavcd_srcs_c_arm64    += decoder/arm/ih264d_function_selector.c

ifeq ($(ARCH_ARM_HAVE_NEON),true)
libavcd_srcs_c_arm64    += decoder/arm/ih264d_function_selector_av8.c

libavcd_srcs_asm_arm64    +=  common/armv8/ih264_intra_pred_chroma_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_intra_pred_luma_16x16_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_intra_pred_luma_4x4_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_chroma_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_filters_luma_horz_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_filters_luma_vert_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_luma_copy_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_luma_horz_qpel_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_luma_vert_qpel_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_luma_horz_hpel_vert_hpel_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_luma_horz_qpel_vert_qpel_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_luma_horz_qpel_vert_hpel_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_luma_horz_hpel_vert_qpel_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_default_weighted_pred_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_weighted_pred_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_weighted_bi_pred_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_deblk_chroma_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_deblk_luma_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_padding_neon_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_iquant_itrans_recon_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_iquant_itrans_recon_dc_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_ihadamard_scaling_av8.s
libavcd_srcs_asm_arm64    +=  common/armv8/ih264_intra_pred_luma_8x8_av8.s

libavcd_cflags_arm64 += -DDEFAULT_ARCH=D_ARCH_ARMV8_GENERIC
else
libavcd_cflags_arm64 += -DDISABLE_NEON -DDEFAULT_ARCH=D_ARCH_ARM_NONEON
endif




LOCAL_SRC_FILES_arm64 += $(libavcd_srcs_c_arm64) $(libavcd_srcs_asm_arm64)
LOCAL_C_INCLUDES_arm64 += $(libavcd_inc_dir_arm64)
LOCAL_CFLAGS_arm64 += $(libavcd_cflags_arm64)

# CLANG WORKAROUNDS
LOCAL_CLANG_ASFLAGS_arm64 += $(addprefix -Wa$(comma)-I,$(libavcd_inc_dir_arm64))
