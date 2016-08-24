libavce_inc_dir_arm +=  $(LOCAL_PATH)/encoder/arm
libavce_inc_dir_arm +=  $(LOCAL_PATH)/common/arm

libavce_cflags_arm  += -DARM

libavce_srcs_c_arm  += encoder/arm/ih264e_function_selector.c

ifeq ($(ARCH_ARM_HAVE_NEON),true)
libavce_srcs_c_arm      += encoder/arm/ih264e_function_selector_a9q.c

libavce_srcs_asm_arm    +=  common/arm/ih264_resi_trans_quant_a9.s
libavce_srcs_asm_arm    +=  common/arm/ih264_iquant_itrans_recon_a9.s
libavce_srcs_asm_arm    +=  common/arm/ih264_iquant_itrans_recon_dc_a9.s
libavce_srcs_asm_arm    +=  common/arm/ih264_ihadamard_scaling_a9.s
libavce_srcs_asm_arm    +=  common/arm/ih264_deblk_chroma_a9.s
libavce_srcs_asm_arm    +=  common/arm/ih264_deblk_luma_a9.s
libavce_srcs_asm_arm    +=  common/arm/ih264_intra_pred_chroma_a9q.s
libavce_srcs_asm_arm    +=  common/arm/ih264_intra_pred_luma_16x16_a9q.s
libavce_srcs_asm_arm    +=  common/arm/ih264_intra_pred_luma_4x4_a9q.s
libavce_srcs_asm_arm    +=  common/arm/ih264_intra_pred_luma_8x8_a9q.s
libavce_srcs_asm_arm    +=  common/arm/ih264_inter_pred_chroma_a9q.s
libavce_srcs_asm_arm    +=  common/arm/ih264_inter_pred_filters_luma_horz_a9q.s
libavce_srcs_asm_arm    +=  common/arm/ih264_inter_pred_filters_luma_vert_a9q.s
libavce_srcs_asm_arm    +=  common/arm/ih264_inter_pred_luma_bilinear_a9q.s
libavce_srcs_asm_arm    +=  common/arm/ih264_inter_pred_luma_copy_a9q.s
libavce_srcs_asm_arm    +=  common/arm/ih264_padding_neon.s
libavce_srcs_asm_arm    +=  common/arm/ih264_mem_fns_neon.s

libavce_srcs_asm_arm    +=  encoder/arm/ih264e_evaluate_intra16x16_modes_a9q.s
libavce_srcs_asm_arm    +=  encoder/arm/ih264e_evaluate_intra4x4_modes_a9q.s
libavce_srcs_asm_arm    +=  encoder/arm/ih264e_evaluate_intra_chroma_modes_a9q.s
libavce_srcs_asm_arm    +=  encoder/arm/ih264e_half_pel.s
libavce_srcs_asm_arm    +=  encoder/arm/ih264e_fmt_conv.s

#ME
libavce_srcs_asm_arm    +=  encoder/arm/ime_distortion_metrics_a9q.s

else #No Neon
libavce_cflags_arm += -DDISABLE_NEON
endif #Neon check

libavce_srcs_asm_arm    +=  common/arm/ih264_arm_memory_barrier.s

LOCAL_SRC_FILES_arm += $(libavce_srcs_c_arm) $(libavce_srcs_asm_arm)
LOCAL_C_INCLUDES_arm += $(libavce_inc_dir_arm)
LOCAL_CFLAGS_arm += $(libavce_cflags_arm)
