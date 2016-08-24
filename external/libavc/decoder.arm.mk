libavcd_inc_dir_arm +=  $(LOCAL_PATH)/decoder/arm
libavcd_inc_dir_arm +=  $(LOCAL_PATH)/common/arm

libavcd_srcs_c_arm  += decoder/arm/ih264d_function_selector.c
libavcd_cflags_arm  += -DARM

#LOCAL_ARM_MODE         := arm

ifeq ($(ARCH_ARM_HAVE_NEON),true)
libavcd_srcs_c_arm      += decoder/arm/ih264d_function_selector_a9q.c

libavcd_srcs_asm_arm    +=  common/arm/ih264_intra_pred_chroma_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_intra_pred_luma_16x16_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_intra_pred_luma_4x4_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_intra_pred_luma_8x8_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_inter_pred_chroma_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_inter_pred_filters_luma_horz_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_inter_pred_filters_luma_vert_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_inter_pred_luma_copy_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_inter_pred_luma_horz_qpel_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_inter_pred_luma_vert_qpel_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_inter_pred_luma_horz_hpel_vert_hpel_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_inter_pred_luma_horz_qpel_vert_qpel_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_inter_pred_luma_horz_qpel_vert_hpel_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_inter_pred_luma_horz_hpel_vert_qpel_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_default_weighted_pred_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_weighted_pred_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_weighted_bi_pred_a9q.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_deblk_chroma_a9.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_deblk_luma_a9.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_padding_neon.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_iquant_itrans_recon_a9.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_iquant_itrans_recon_dc_a9.s
libavcd_srcs_asm_arm    +=  common/arm/ih264_ihadamard_scaling_a9.s

libavcd_cflags_arm += -DDEFAULT_ARCH=D_ARCH_ARM_A9Q
else
libavcd_cflags_arm += -DDISABLE_NEON -DDEFAULT_ARCH=D_ARCH_ARM_NONEON
endif

libavcd_srcs_asm_arm    +=  common/arm/ih264_arm_memory_barrier.s

LOCAL_SRC_FILES_arm += $(libavcd_srcs_c_arm) $(libavcd_srcs_asm_arm)
LOCAL_C_INCLUDES_arm += $(libavcd_inc_dir_arm)
LOCAL_CFLAGS_arm += $(libavcd_cflags_arm)
