LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

libavce_source_dir := $(LOCAL_PATH)

## Arch-common settings
LOCAL_MODULE := libavcenc
#LOCAL_32_BIT_ONLY := true

LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_CFLAGS += -DNDEBUG -UHP_PL -DN_MB_ENABLE -fPIC
LOCAL_CFLAGS += -O3

LOCAL_C_INCLUDES := $(LOCAL_PATH)/encoder $(LOCAL_PATH)/common

libavce_srcs_c  += common/ih264_resi_trans_quant.c
libavce_srcs_c  += common/ih264_iquant_itrans_recon.c
libavce_srcs_c  += common/ih264_ihadamard_scaling.c
libavce_srcs_c  += common/ih264_inter_pred_filters.c
libavce_srcs_c  += common/ih264_luma_intra_pred_filters.c
libavce_srcs_c  += common/ih264_chroma_intra_pred_filters.c
libavce_srcs_c  += common/ih264_padding.c
libavce_srcs_c  += common/ih264_mem_fns.c
libavce_srcs_c  += common/ih264_deblk_edge_filters.c
libavce_srcs_c  += common/ih264_deblk_tables.c
libavce_srcs_c  += common/ih264_cavlc_tables.c
libavce_srcs_c  += common/ih264_cabac_tables.c
libavce_srcs_c  += common/ih264_common_tables.c
libavce_srcs_c  += common/ih264_trans_data.c
libavce_srcs_c  += common/ih264_buf_mgr.c
libavce_srcs_c  += common/ih264_dpb_mgr.c
libavce_srcs_c  += common/ih264_list.c


libavce_srcs_c  += common/ithread.c

libavce_srcs_c  += encoder/ih264e_globals.c
libavce_srcs_c  += encoder/ih264e_intra_modes_eval.c
libavce_srcs_c  += encoder/ih264e_half_pel.c
libavce_srcs_c  += encoder/ih264e_mc.c
libavce_srcs_c  += encoder/ih264e_me.c
libavce_srcs_c  += encoder/ih264e_rc_mem_interface.c
libavce_srcs_c  += encoder/ih264e_time_stamp.c
libavce_srcs_c  += encoder/ih264e_modify_frm_rate.c
libavce_srcs_c  += encoder/ih264e_rate_control.c
libavce_srcs_c  += encoder/ih264e_core_coding.c
libavce_srcs_c  += encoder/ih264e_deblk.c
libavce_srcs_c  += encoder/ih264e_api.c
libavce_srcs_c  += encoder/ih264e_process.c
libavce_srcs_c  += encoder/ih264e_encode.c
libavce_srcs_c  += encoder/ih264e_utils.c
libavce_srcs_c  += encoder/ih264e_version.c
libavce_srcs_c  += encoder/ih264e_bitstream.c
libavce_srcs_c  += encoder/ih264e_cavlc.c
libavce_srcs_c  += encoder/ih264e_cabac_init.c
libavce_srcs_c  += encoder/ih264e_cabac.c
libavce_srcs_c  += encoder/ih264e_cabac_encode.c
libavce_srcs_c  += encoder/ih264e_encode_header.c
libavce_srcs_c  += encoder/ih264e_function_selector_generic.c
libavce_srcs_c  += encoder/ih264e_fmt_conv.c

#Rate Control
libavce_srcs_c  += encoder/irc_rate_control_api.c
libavce_srcs_c  += encoder/irc_bit_allocation.c
libavce_srcs_c  += encoder/irc_cbr_buffer_control.c
libavce_srcs_c  += encoder/irc_est_sad.c
libavce_srcs_c  += encoder/irc_fixed_point_error_bits.c
libavce_srcs_c  += encoder/irc_frame_info_collector.c
libavce_srcs_c  += encoder/irc_mb_model_based.c
libavce_srcs_c  += encoder/irc_picture_type.c
libavce_srcs_c  += encoder/irc_rd_model.c
libavce_srcs_c  += encoder/irc_vbr_storage_vbv.c
libavce_srcs_c  += encoder/irc_vbr_str_prms.c

#ME files
libavce_srcs_c  += encoder/ime.c
libavce_srcs_c  += encoder/ime_distortion_metrics.c



LOCAL_SRC_FILES := $(libavce_srcs_c) $(libavce_srcs_asm)


# Load the arch-specific settings
include $(LOCAL_PATH)/encoder.arm.mk
include $(LOCAL_PATH)/encoder.arm64.mk
include $(LOCAL_PATH)/encoder.x86.mk
include $(LOCAL_PATH)/encoder.x86_64.mk
include $(LOCAL_PATH)/encoder.mips.mk
include $(LOCAL_PATH)/encoder.mips64.mk

include $(BUILD_STATIC_LIBRARY)
