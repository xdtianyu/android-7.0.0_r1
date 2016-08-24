libavce_cflags_arm64 += -DARMV8
libavce_cflags_arm64 += -DARM

libavce_inc_dir_arm64   +=  $(LOCAL_PATH)/encoder/arm
libavce_inc_dir_arm64   +=  $(LOCAL_PATH)/encoder/armv8
libavce_inc_dir_arm64   +=  $(LOCAL_PATH)/common/armv8

libavce_srcs_c_arm64    += encoder/arm/ih264e_function_selector.c

ifeq ($(ARCH_ARM_HAVE_NEON),true)
libavce_srcs_c_arm64    += encoder/arm/ih264e_function_selector_av8.c

libavce_srcs_asm_arm64    +=  common/armv8/ih264_resi_trans_quant_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_iquant_itrans_recon_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_iquant_itrans_recon_dc_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_ihadamard_scaling_av8.s

libavce_srcs_asm_arm64    +=  common/armv8/ih264_intra_pred_chroma_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_intra_pred_luma_16x16_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_intra_pred_luma_4x4_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_intra_pred_luma_8x8_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_luma_copy_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_chroma_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_filters_luma_horz_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_inter_pred_filters_luma_vert_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_padding_neon_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_mem_fns_neon_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_deblk_luma_av8.s
libavce_srcs_asm_arm64    +=  common/armv8/ih264_deblk_chroma_av8.s

libavce_srcs_asm_arm64    +=  encoder/armv8/ih264e_evaluate_intra16x16_modes_av8.s
libavce_srcs_asm_arm64    +=  encoder/armv8/ih264e_evaluate_intra_chroma_modes_av8.s
libavce_srcs_asm_arm64    +=  encoder/armv8/ih264e_half_pel_av8.s

#ME
libavce_srcs_asm_arm64    +=  encoder/armv8/ime_distortion_metrics_av8.s

else
libavce_cflags_arm64 += -DDISABLE_NEON
endif




LOCAL_SRC_FILES_arm64 += $(libavce_srcs_c_arm64) $(libavce_srcs_asm_arm64)
LOCAL_C_INCLUDES_arm64 += $(libavce_inc_dir_arm64)
LOCAL_CFLAGS_arm64 += $(libavce_cflags_arm64)

# CLANG WORKAROUNDS
LOCAL_CLANG_ASFLAGS_arm64 += $(addprefix -Wa$(comma)-I,$(libavce_inc_dir_arm64))
