# Copyright 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)

###############################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        dsp/alpha_processing.c \
        dsp/alpha_processing_mips_dsp_r2.c \
        dsp/alpha_processing_sse2.c \
        dsp/alpha_processing_sse41.c \
        dsp/argb.c \
        dsp/argb_mips_dsp_r2.c \
        dsp/argb_sse2.c \
        dsp/cost.c \
        dsp/cost_mips32.c \
        dsp/cost_mips_dsp_r2.c \
        dsp/cost_sse2.c \
        dsp/cpu-features.c \
        dsp/cpu.c \
        dsp/enc.c \
        dsp/enc_avx2.c \
        dsp/enc_mips32.c \
        dsp/enc_mips_dsp_r2.c \
        dsp/enc_neon.c \
        dsp/enc_sse2.c \
        dsp/enc_sse41.c \
        dsp/lossless_enc.c \
        dsp/lossless_enc_mips32.c \
        dsp/lossless_enc_mips_dsp_r2.c \
        dsp/lossless_enc_neon.c \
        dsp/lossless_enc_sse2.c \
        dsp/lossless_enc_sse41.c \
        enc/alpha.c \
        enc/analysis.c \
        enc/backward_references.c \
        enc/config.c \
        enc/cost.c \
        enc/delta_palettization.c \
        enc/filter.c \
        enc/frame.c \
        enc/histogram.c \
        enc/iterator.c \
        enc/near_lossless.c \
        enc/picture.c \
        enc/picture_csp.c \
        enc/picture_psnr.c \
        enc/picture_rescale.c \
        enc/picture_tools.c \
        enc/quant.c \
        enc/syntax.c \
        enc/token.c \
        enc/tree.c \
        enc/vp8l.c \
        enc/webpenc.c \
        utils/bit_reader.c \
        utils/bit_writer.c \
        utils/color_cache.c \
        utils/filters.c \
        utils/huffman.c \
        utils/huffman_encode.c \
        utils/quant_levels.c \
        utils/random.c \
        utils/rescaler.c \
        utils/thread.c \
        utils/utils.c

LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -O2 -DANDROID -DWEBP_SWAP_16BIT_CSP

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/enc \
        $(LOCAL_PATH)/../include

LOCAL_MODULE := libwebp-encode

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

include $(BUILD_STATIC_LIBRARY)

###############################################

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        dec/alpha.c \
        dec/buffer.c \
        dec/frame.c \
        dec/idec.c \
        dec/io.c \
        dec/quant.c \
        dec/tree.c \
        dec/vp8.c \
        dec/vp8l.c \
        dec/webp.c \
        demux/demux.c \
        dsp/alpha_processing.c \
        dsp/alpha_processing_mips_dsp_r2.c \
        dsp/alpha_processing_sse2.c \
        dsp/alpha_processing_sse41.c \
        dsp/cpu-features.c \
        dsp/cpu.c \
        dsp/dec.c \
        dsp/dec_clip_tables.c \
        dsp/dec_mips32.c \
        dsp/dec_mips_dsp_r2.c \
        dsp/dec_neon.c \
        dsp/dec_sse2.c \
        dsp/dec_sse41.c \
        dsp/filters.c \
        dsp/filters_mips_dsp_r2.c \
        dsp/filters_sse2.c \
        dsp/lossless.c \
        dsp/lossless_mips_dsp_r2.c \
        dsp/lossless_neon.c \
        dsp/lossless_sse2.c \
        dsp/rescaler.c \
        dsp/rescaler_mips32.c \
        dsp/rescaler_mips_dsp_r2.c \
        dsp/rescaler_neon.c \
        dsp/rescaler_sse2.c \
        dsp/upsampling.c \
        dsp/upsampling_mips_dsp_r2.c \
        dsp/upsampling_neon.c \
        dsp/upsampling_sse2.c \
        dsp/yuv.c \
        dsp/yuv_mips32.c \
        dsp/yuv_mips_dsp_r2.c \
        dsp/yuv_sse2.c \
        utils/bit_reader.c \
        utils/color_cache.c \
        utils/filters.c \
        utils/huffman.c \
        utils/quant_levels_dec.c \
        utils/random.c \
        utils/rescaler.c \
        utils/thread.c \
        utils/utils.c

LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -O2 -DANDROID -DWEBP_SWAP_16BIT_CSP

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/dec \
        $(LOCAL_PATH)/../include

LOCAL_SDK_VERSION := 9

LOCAL_MODULE := libwebp-decode

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

include $(BUILD_STATIC_LIBRARY)
