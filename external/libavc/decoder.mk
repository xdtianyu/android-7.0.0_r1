LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

libavcd_source_dir := $(LOCAL_PATH)

## Arch-common settings
LOCAL_MODULE := libavcdec
#LOCAL_32_BIT_ONLY := true

LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_CFLAGS += -fPIC
LOCAL_CFLAGS += -O3

LOCAL_C_INCLUDES := $(LOCAL_PATH)/decoder $(LOCAL_PATH)/common

libavcd_srcs_c  += common/ih264_buf_mgr.c
libavcd_srcs_c  += common/ih264_disp_mgr.c
libavcd_srcs_c  += common/ih264_inter_pred_filters.c
libavcd_srcs_c  += common/ih264_luma_intra_pred_filters.c
libavcd_srcs_c  += common/ih264_chroma_intra_pred_filters.c
libavcd_srcs_c  += common/ih264_padding.c
libavcd_srcs_c  += common/ih264_mem_fns.c
libavcd_srcs_c  += common/ih264_deblk_edge_filters.c
libavcd_srcs_c  += common/ih264_iquant_itrans_recon.c
libavcd_srcs_c  += common/ih264_ihadamard_scaling.c
libavcd_srcs_c  += common/ih264_weighted_pred.c

libavcd_srcs_c  += common/ithread.c

libavcd_srcs_c  += decoder/ih264d_cabac.c
libavcd_srcs_c  += decoder/ih264d_parse_mb_header.c
libavcd_srcs_c  += decoder/ih264d_parse_cabac.c
libavcd_srcs_c  += decoder/ih264d_process_intra_mb.c
libavcd_srcs_c  += decoder/ih264d_inter_pred.c
libavcd_srcs_c  += decoder/ih264d_parse_bslice.c
libavcd_srcs_c  += decoder/ih264d_parse_pslice.c
libavcd_srcs_c  += decoder/ih264d_parse_islice.c
libavcd_srcs_c  += decoder/ih264d_cabac_init_tables.c
libavcd_srcs_c  += decoder/ih264d_bitstrm.c
libavcd_srcs_c  += decoder/ih264d_compute_bs.c
libavcd_srcs_c  += decoder/ih264d_deblocking.c
libavcd_srcs_c  += decoder/ih264d_parse_headers.c
libavcd_srcs_c  += decoder/ih264d_mb_utils.c
libavcd_srcs_c  += decoder/ih264d_mvpred.c
libavcd_srcs_c  += decoder/ih264d_utils.c
libavcd_srcs_c  += decoder/ih264d_process_bslice.c
libavcd_srcs_c  += decoder/ih264d_process_pslice.c
libavcd_srcs_c  += decoder/ih264d_parse_slice.c
libavcd_srcs_c  += decoder/ih264d_quant_scaling.c
libavcd_srcs_c  += decoder/ih264d_parse_cavlc.c
libavcd_srcs_c  += decoder/ih264d_dpb_mgr.c
libavcd_srcs_c  += decoder/ih264d_nal.c
libavcd_srcs_c  += decoder/ih264d_sei.c
libavcd_srcs_c  += decoder/ih264d_tables.c
libavcd_srcs_c  += decoder/ih264d_vui.c
libavcd_srcs_c  += decoder/ih264d_format_conv.c
libavcd_srcs_c  += decoder/ih264d_thread_parse_decode.c
libavcd_srcs_c  += decoder/ih264d_api.c
libavcd_srcs_c  += decoder/ih264d_thread_compute_bs.c
libavcd_srcs_c  += decoder/ih264d_function_selector_generic.c


LOCAL_SRC_FILES := $(libavcd_srcs_c) $(libavcd_srcs_asm)


# Load the arch-specific settings
include $(LOCAL_PATH)/decoder.arm.mk
include $(LOCAL_PATH)/decoder.arm64.mk
include $(LOCAL_PATH)/decoder.x86.mk
include $(LOCAL_PATH)/decoder.x86_64.mk
include $(LOCAL_PATH)/decoder.mips.mk
include $(LOCAL_PATH)/decoder.mips64.mk

include $(BUILD_STATIC_LIBRARY)
