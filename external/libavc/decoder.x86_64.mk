libavcd_cflags_x86_64 += -DX86 -msse4.2 -DDEFAULT_ARCH=D_ARCH_X86_SSE42

libavcd_inc_dir_x86_64   +=  $(LOCAL_PATH)/decoder/x86
libavcd_inc_dir_x86_64   +=  $(LOCAL_PATH)/common/x86

libavcd_srcs_c_x86_64       +=  decoder/x86/ih264d_function_selector.c
libavcd_srcs_c_x86_64       +=  decoder/x86/ih264d_function_selector_sse42.c
libavcd_srcs_c_x86_64       +=  decoder/x86/ih264d_function_selector_ssse3.c

libavcd_srcs_c_x86_64       +=  common/x86/ih264_inter_pred_filters_ssse3.c
libavcd_srcs_c_x86_64       +=  common/x86/ih264_deblk_luma_ssse3.c
libavcd_srcs_c_x86_64       +=  common/x86/ih264_deblk_chroma_ssse3.c
libavcd_srcs_c_x86_64       +=  common/x86/ih264_padding_ssse3.c
libavcd_srcs_c_x86_64       +=  common/x86/ih264_mem_fns_ssse3.c
libavcd_srcs_c_x86_64       +=  common/x86/ih264_iquant_itrans_recon_dc_ssse3.c
libavcd_srcs_c_x86_64       +=  common/x86/ih264_iquant_itrans_recon_ssse3.c
libavcd_srcs_c_x86_64       +=  common/x86/ih264_luma_intra_pred_filters_ssse3.c
libavcd_srcs_c_x86_64       +=  common/x86/ih264_chroma_intra_pred_filters_ssse3.c
libavcd_srcs_c_x86_64       +=  common/x86/ih264_iquant_itrans_recon_sse42.c
libavcd_srcs_c_x86_64       +=  common/x86/ih264_weighted_pred_sse42.c
libavcd_srcs_c_x86_64       +=  common/x86/ih264_ihadamard_scaling_sse42.c


LOCAL_SRC_FILES_x86_64 += $(libavcd_srcs_c_x86_64) $(libavcd_srcs_asm_x86_64)
LOCAL_C_INCLUDES_x86_64 += $(libavcd_inc_dir_x86_64)
LOCAL_CFLAGS_x86_64 += $(libavcd_cflags_x86_64)



