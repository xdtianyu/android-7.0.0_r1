libhevcd_inc_dir_arm   +=  $(LOCAL_PATH)/decoder/arm
libhevcd_inc_dir_arm   +=  $(LOCAL_PATH)/common/arm

libhevcd_srcs_c_arm    +=  decoder/arm/ihevcd_function_selector.c
libhevcd_srcs_c_arm    +=  decoder/arm/ihevcd_function_selector_noneon.c
libhevcd_cflags_arm    += -DDISABLE_NEONINTR  -DARM -DARMGCC -fno-tree-vectorize

LOCAL_ARM_MODE         := arm

ifeq ($(ARCH_ARM_HAVE_NEON),true)
libhevcd_srcs_c_arm    +=  decoder/arm/ihevcd_function_selector_a9q.c
libhevcd_srcs_c_arm    +=  common/arm/ihevc_intra_ref_substitution_a9q.c
libhevcd_srcs_c_arm    +=  common/arm/ihevc_intra_pred_filters_neon_intr.c
libhevcd_srcs_c_arm    +=  common/arm/ihevc_weighted_pred_neon_intr.c

libhevcd_srcs_asm_arm   +=  common/arm/ihevc_mem_fns.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_itrans_recon_32x32.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_weighted_pred_bi_default.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_weighted_pred_bi.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_weighted_pred_uni.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_deblk_luma_horz.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_deblk_luma_vert.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_deblk_chroma_vert.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_deblk_chroma_horz.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_sao_band_offset_luma.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_sao_band_offset_chroma.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_sao_edge_offset_class0.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_sao_edge_offset_class0_chroma.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_sao_edge_offset_class1.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_sao_edge_offset_class1_chroma.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_sao_edge_offset_class2.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_sao_edge_offset_class2_chroma.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_sao_edge_offset_class3.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_sao_edge_offset_class3_chroma.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_luma_horz_w16out.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_filters_luma_horz.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_filters_luma_vert.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_chroma_horz.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_chroma_horz_w16out.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_chroma_vert.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_chroma_vert_w16out.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_chroma_vert_w16inp.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_chroma_vert_w16inp_w16out.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_filters_luma_vert_w16inp.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_luma_vert_w16inp_w16out.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_luma_copy_w16out.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_luma_copy.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_chroma_copy.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_inter_pred_chroma_copy_w16out.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_itrans_recon_4x4_ttype1.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_itrans_recon_4x4.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_itrans_recon_8x8.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_itrans_recon_16x16.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_chroma_planar.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_chroma_dc.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_chroma_horz.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_chroma_ver.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_chroma_mode2.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_chroma_mode_18_34.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_filters_chroma_mode_11_to_17.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_filters_chroma_mode_19_to_25.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_chroma_mode_3_to_9.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_chroma_mode_27_to_33.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_luma_planar.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_luma_horz.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_luma_mode2.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_luma_mode_27_to_33.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_luma_mode_18_34.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_luma_vert.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_luma_dc.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_filters_luma_mode_11_to_17.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_filters_luma_mode_19_to_25.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_intra_pred_luma_mode_3_to_9.s
libhevcd_srcs_asm_arm   +=  common/arm/ihevc_padding.s

libhevcd_srcs_asm_arm    +=  decoder/arm/ihevcd_itrans_recon_dc_luma.s
libhevcd_srcs_asm_arm    +=  decoder/arm/ihevcd_itrans_recon_dc_chroma.s
libhevcd_srcs_asm_arm    +=  decoder/arm/ihevcd_fmt_conv_420sp_to_420p.s
libhevcd_srcs_asm_arm    +=  decoder/arm/ihevcd_fmt_conv_420sp_to_420sp.s
libhevcd_srcs_asm_arm    +=  decoder/arm/ihevcd_fmt_conv_420sp_to_rgba8888.s
libhevcd_cflags_arm += -DDEFAULT_ARCH=D_ARCH_ARM_A9Q
else
libhevcd_cflags_arm += -DDISABLE_NEON -DDEFAULT_ARCH=D_ARCH_ARM_NONEON
endif

LOCAL_SRC_FILES_arm += $(libhevcd_srcs_c_arm) $(libhevcd_srcs_asm_arm)
LOCAL_C_INCLUDES_arm += $(libhevcd_inc_dir_arm)
LOCAL_CFLAGS_arm += $(libhevcd_cflags_arm)
