libhevcd_cflags_arm64 += -DARMV8
libhevcd_cflags_arm64 += -DDISABLE_NEONINTR  -DARM -DARMGCC

libhevcd_inc_dir_arm64   +=  $(LOCAL_PATH)/decoder/arm
libhevcd_inc_dir_arm64   +=  $(LOCAL_PATH)/common/arm
libhevcd_inc_dir_arm64   +=  $(LOCAL_PATH)/decoder/arm64
libhevcd_inc_dir_arm64   +=  $(LOCAL_PATH)/common/arm64

libhevcd_srcs_c_arm64    +=  decoder/arm/ihevcd_function_selector.c
libhevcd_srcs_c_arm64    +=  decoder/arm/ihevcd_function_selector_noneon.c

libhevcd_srcs_c_arm64    +=  decoder/arm64/ihevcd_function_selector_av8.c

libhevcd_srcs_c_arm64    +=  common/arm/ihevc_intra_pred_filters_neon_intr.c
libhevcd_srcs_c_arm64    +=  common/arm/ihevc_weighted_pred_neon_intr.c

libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_mem_fns.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_itrans_recon_32x32.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_weighted_pred_bi_default.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_weighted_pred_bi.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_weighted_pred_uni.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_deblk_luma_horz.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_deblk_luma_vert.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_deblk_chroma_vert.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_deblk_chroma_horz.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_sao_band_offset_luma.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_sao_band_offset_chroma.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_sao_edge_offset_class0.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_sao_edge_offset_class0_chroma.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_sao_edge_offset_class1.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_sao_edge_offset_class1_chroma.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_sao_edge_offset_class2.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_sao_edge_offset_class2_chroma.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_sao_edge_offset_class3.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_sao_edge_offset_class3_chroma.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_luma_horz_w16out.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_filters_luma_horz.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_filters_luma_vert.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_chroma_horz.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_chroma_horz_w16out.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_chroma_vert.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_chroma_vert_w16out.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_chroma_vert_w16inp.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_chroma_vert_w16inp_w16out.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_filters_luma_vert_w16inp.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_filters_luma_vert_w16out.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_luma_vert_w16inp_w16out.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_luma_copy_w16out.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_luma_copy.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_chroma_copy.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_inter_pred_chroma_copy_w16out.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_itrans_recon_4x4_ttype1.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_itrans_recon_4x4.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_itrans_recon_8x8.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_itrans_recon_16x16.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_chroma_planar.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_chroma_dc.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_chroma_horz.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_chroma_ver.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_chroma_mode2.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_chroma_mode_18_34.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_filters_chroma_mode_11_to_17.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_filters_chroma_mode_19_to_25.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_chroma_mode_3_to_9.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_chroma_mode_27_to_33.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_luma_planar.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_luma_horz.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_luma_mode2.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_luma_mode_27_to_33.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_luma_mode_18_34.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_luma_vert.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_luma_dc.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_filters_luma_mode_11_to_17.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_filters_luma_mode_19_to_25.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_intra_pred_luma_mode_3_to_9.s
libhevcd_srcs_asm_arm64   +=  common/arm64/ihevc_padding.s



libhevcd_srcs_asm_arm64    +=  decoder/arm64/ihevcd_itrans_recon_dc_luma.s
libhevcd_srcs_asm_arm64    +=  decoder/arm64/ihevcd_itrans_recon_dc_chroma.s
libhevcd_srcs_asm_arm64    +=  decoder/arm64/ihevcd_fmt_conv_420sp_to_420p.s
libhevcd_srcs_asm_arm64    +=  decoder/arm64/ihevcd_fmt_conv_420sp_to_420sp.s
libhevcd_srcs_asm_arm64    +=  decoder/arm64/ihevcd_fmt_conv_420sp_to_rgba8888.s

libhevcd_cflags_arm64 += -DDEFAULT_ARCH=D_ARCH_ARMV8_GENERIC




LOCAL_SRC_FILES_arm64 += $(libhevcd_srcs_c_arm64) $(libhevcd_srcs_asm_arm64)
LOCAL_C_INCLUDES_arm64 += $(libhevcd_inc_dir_arm64)
LOCAL_CFLAGS_arm64 += $(libhevcd_cflags_arm64)

# Clang doesn't pass -I flags to the assembler when building a .s file.
# We need to tell it to pass them to the assembler specifically (doesn't hurt
# with gcc either, and may actually help future gcc versions if they decide
# to start making a difference between assembly and C includes).
comma := ,
LOCAL_ASFLAGS_arm64 += $(addprefix -Wa$(comma)-I,$(libhevcd_inc_dir_arm64))
