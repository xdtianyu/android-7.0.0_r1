LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

libmpeg2d_source_dir := $(LOCAL_PATH)

## Arch-common settings
LOCAL_MODULE := libmpeg2dec
#LOCAL_32_BIT_ONLY := true

LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_CFLAGS += -D_LIB -DMULTICORE -fPIC
LOCAL_CFLAGS += -O3 -DANDROID

LOCAL_C_INCLUDES := $(LOCAL_PATH)/decoder $(LOCAL_PATH)/common

libmpeg2d_srcs_c    += common/impeg2_buf_mgr.c
libmpeg2d_srcs_c    += common/impeg2_disp_mgr.c
libmpeg2d_srcs_c    += common/impeg2_format_conv.c
libmpeg2d_srcs_c    += common/impeg2_globals.c
libmpeg2d_srcs_c    += common/impeg2_idct.c
libmpeg2d_srcs_c    += common/impeg2_inter_pred.c
libmpeg2d_srcs_c    += common/impeg2_job_queue.c
libmpeg2d_srcs_c    += common/impeg2_mem_func.c

libmpeg2d_srcs_c    += common/ithread.c

libmpeg2d_srcs_c    += decoder/impeg2d_api_main.c
libmpeg2d_srcs_c    += decoder/impeg2d_bitstream.c
libmpeg2d_srcs_c    += decoder/impeg2d_debug.c
libmpeg2d_srcs_c    += decoder/impeg2d_dec_hdr.c
libmpeg2d_srcs_c    += decoder/impeg2d_decoder.c
libmpeg2d_srcs_c    += decoder/impeg2d_d_pic.c
libmpeg2d_srcs_c    += decoder/impeg2d_function_selector_generic.c
libmpeg2d_srcs_c    += decoder/impeg2d_globals.c
libmpeg2d_srcs_c    += decoder/impeg2d_i_pic.c
libmpeg2d_srcs_c    += decoder/impeg2d_mc.c
libmpeg2d_srcs_c    += decoder/impeg2d_mv_dec.c
libmpeg2d_srcs_c    += decoder/impeg2d_pic_proc.c
libmpeg2d_srcs_c    += decoder/impeg2d_pnb_pic.c
libmpeg2d_srcs_c    += decoder/impeg2d_vld.c
libmpeg2d_srcs_c    += decoder/impeg2d_vld_tables.c
libmpeg2d_srcs_c    += decoder/impeg2d_deinterlace.c

libmpeg2d_srcs_c    += common/icv_sad.c
libmpeg2d_srcs_c    += common/icv_variance.c
libmpeg2d_srcs_c    += common/ideint.c
libmpeg2d_srcs_c    += common/ideint_cac.c
libmpeg2d_srcs_c    += common/ideint_debug.c
libmpeg2d_srcs_c    += common/ideint_function_selector_generic.c
libmpeg2d_srcs_c    += common/ideint_utils.c

LOCAL_SRC_FILES := $(libmpeg2d_srcs_c) $(libmpeg2d_srcs_asm)


# Load the arch-specific settings
include $(LOCAL_PATH)/decoder.arm.mk
include $(LOCAL_PATH)/decoder.arm64.mk
include $(LOCAL_PATH)/decoder.x86.mk
include $(LOCAL_PATH)/decoder.x86_64.mk
include $(LOCAL_PATH)/decoder.mips.mk
include $(LOCAL_PATH)/decoder.mips64.mk

include $(BUILD_STATIC_LIBRARY)
