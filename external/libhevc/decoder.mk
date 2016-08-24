LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

libhevc_source_dir := $(LOCAL_PATH)

## Arch-common settings
LOCAL_MODULE := libhevcdec
#LOCAL_32_BIT_ONLY := true

LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_CFLAGS += -D_LIB -DMULTICORE -fPIC
LOCAL_CFLAGS += -O3 -DANDROID

LOCAL_C_INCLUDES := $(LOCAL_PATH)/decoder $(LOCAL_PATH)/common

libhevcd_srcs_c   +=  common/ihevc_quant_tables.c
libhevcd_srcs_c   +=  common/ihevc_inter_pred_filters.c
libhevcd_srcs_c   +=  common/ihevc_weighted_pred.c
libhevcd_srcs_c   +=  common/ihevc_padding.c
libhevcd_srcs_c   +=  common/ihevc_deblk_edge_filter.c
libhevcd_srcs_c   +=  common/ihevc_deblk_tables.c
libhevcd_srcs_c   +=  common/ihevc_cabac_tables.c
libhevcd_srcs_c   +=  common/ihevc_common_tables.c
libhevcd_srcs_c   +=  common/ihevc_intra_pred_filters.c
libhevcd_srcs_c   +=  common/ihevc_chroma_intra_pred_filters.c
libhevcd_srcs_c   +=  common/ihevc_mem_fns.c
libhevcd_srcs_c   +=  common/ihevc_sao.c
libhevcd_srcs_c   +=  common/ihevc_trans_tables.c
libhevcd_srcs_c   +=  common/ihevc_recon.c
libhevcd_srcs_c   +=  common/ihevc_itrans.c
libhevcd_srcs_c   +=  common/ihevc_itrans_recon.c
libhevcd_srcs_c   +=  common/ihevc_iquant_recon.c
libhevcd_srcs_c   +=  common/ihevc_iquant_itrans_recon.c
libhevcd_srcs_c   +=  common/ihevc_itrans_recon_32x32.c
libhevcd_srcs_c   +=  common/ihevc_itrans_recon_16x16.c
libhevcd_srcs_c   +=  common/ihevc_itrans_recon_8x8.c
libhevcd_srcs_c   +=  common/ihevc_chroma_itrans_recon.c
libhevcd_srcs_c   +=  common/ihevc_chroma_iquant_recon.c
libhevcd_srcs_c   +=  common/ihevc_chroma_iquant_itrans_recon.c
libhevcd_srcs_c   +=  common/ihevc_chroma_recon.c
libhevcd_srcs_c   +=  common/ihevc_chroma_itrans_recon_16x16.c
libhevcd_srcs_c   +=  common/ihevc_chroma_itrans_recon_8x8.c
libhevcd_srcs_c   +=  common/ihevc_buf_mgr.c
libhevcd_srcs_c   +=  common/ihevc_disp_mgr.c
libhevcd_srcs_c   +=  common/ihevc_dpb_mgr.c
libhevcd_srcs_c   +=  common/ithread.c



libhevcd_srcs_c   +=  decoder/ihevcd_version.c
libhevcd_srcs_c   +=  decoder/ihevcd_api.c
libhevcd_srcs_c   +=  decoder/ihevcd_decode.c
libhevcd_srcs_c   +=  decoder/ihevcd_nal.c
libhevcd_srcs_c   +=  decoder/ihevcd_bitstream.c
libhevcd_srcs_c   +=  decoder/ihevcd_parse_headers.c
libhevcd_srcs_c   +=  decoder/ihevcd_parse_slice_header.c
libhevcd_srcs_c   +=  decoder/ihevcd_parse_slice.c
libhevcd_srcs_c   +=  decoder/ihevcd_parse_residual.c
libhevcd_srcs_c   +=  decoder/ihevcd_cabac.c
libhevcd_srcs_c   +=  decoder/ihevcd_intra_pred_mode_prediction.c
libhevcd_srcs_c   +=  decoder/ihevcd_process_slice.c
libhevcd_srcs_c   +=  decoder/ihevcd_utils.c
libhevcd_srcs_c   +=  decoder/ihevcd_job_queue.c
libhevcd_srcs_c   +=  decoder/ihevcd_ref_list.c
libhevcd_srcs_c   +=  decoder/ihevcd_get_mv.c
libhevcd_srcs_c   +=  decoder/ihevcd_mv_pred.c
libhevcd_srcs_c   +=  decoder/ihevcd_mv_merge.c
libhevcd_srcs_c   +=  decoder/ihevcd_iquant_itrans_recon_ctb.c
libhevcd_srcs_c   +=  decoder/ihevcd_itrans_recon_dc.c
libhevcd_srcs_c   +=  decoder/ihevcd_common_tables.c
libhevcd_srcs_c   +=  decoder/ihevcd_boundary_strength.c
libhevcd_srcs_c   +=  decoder/ihevcd_deblk.c
libhevcd_srcs_c   +=  decoder/ihevcd_inter_pred.c
libhevcd_srcs_c   +=  decoder/ihevcd_sao.c
libhevcd_srcs_c   +=  decoder/ihevcd_ilf_padding.c
libhevcd_srcs_c   +=  decoder/ihevcd_fmt_conv.c

LOCAL_SRC_FILES := $(libhevcd_srcs_c) $(libhevcd_srcs_asm)


# Load the arch-specific settings
include $(LOCAL_PATH)/decoder.arm.mk
include $(LOCAL_PATH)/decoder.arm64.mk
include $(LOCAL_PATH)/decoder.x86.mk
include $(LOCAL_PATH)/decoder.x86_64.mk
include $(LOCAL_PATH)/decoder.mips.mk
include $(LOCAL_PATH)/decoder.mips64.mk

include $(BUILD_STATIC_LIBRARY)
