/******************************************************************************
 *
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
*/

/**
*******************************************************************************
* @file
*  ih264e_fmt_conv.h
*
* @brief
*  The file contains extern declarations of color space conversion routines
*
* @author
*  ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_FMT_CONV_H_
#define IH264E_FMT_CONV_H_

#define COEFF1          13073
#define COEFF2          -3207
#define COEFF3          -6664
#define COEFF4          16530

IH264E_ERROR_T ih264e_fmt_conv(codec_t *ps_codec,
                               pic_buf_t *ps_pic,
                               UWORD8 *pu1_y_dst,
                               UWORD8 *pu1_u_dst,
                               UWORD8 *pu1_v_dst,
                               UWORD32 u4_dst_y_strd,
                               UWORD32 u4_dst_uv_strd,
                               WORD32 cur_row,
                               WORD32 num_rows);

typedef void ih264e_fmt_conv_420sp_to_rgba8888_ft(UWORD8 *pu1_y_src,
                                                  UWORD8 *pu1_uv_src,
                                                  UWORD32 *pu4_rgba_dst,
                                                  WORD32 wd,
                                                  WORD32 ht,
                                                  WORD32 src_y_strd,
                                                  WORD32 src_uv_strd,
                                                  WORD32 dst_strd,
                                                  WORD32 is_u_first);

typedef void ih264e_fmt_conv_420sp_to_rgb565_ft(UWORD8 *pu1_y_src,
                                                UWORD8 *pu1_uv_src,
                                                UWORD16 *pu2_rgb_dst,
                                                WORD32 wd,
                                                WORD32 ht,
                                                WORD32 src_y_strd,
                                                WORD32 src_uv_strd,
                                                WORD32 dst_strd,
                                                WORD32 is_u_first);

typedef void ih264e_fmt_conv_420sp_to_420sp_ft(UWORD8 *pu1_y_src,
                                               UWORD8 *pu1_uv_src,
                                               UWORD8 *pu1_y_dst,
                                               UWORD8 *pu1_uv_dst,
                                               WORD32 wd,
                                               WORD32 ht,
                                               WORD32 src_y_strd,
                                               WORD32 src_uv_strd,
                                               WORD32 dst_y_strd,
                                               WORD32 dst_uv_strd);

typedef void ih264e_fmt_conv_420sp_to_420p_ft(UWORD8 *pu1_y_src,
                                              UWORD8 *pu1_uv_src,
                                              UWORD8 *pu1_y_dst,
                                              UWORD8 *pu1_u_dst,
                                              UWORD8 *pu1_v_dst,
                                              WORD32 wd,
                                              WORD32 ht,
                                              WORD32 src_y_strd,
                                              WORD32 src_uv_strd,
                                              WORD32 dst_y_strd,
                                              WORD32 dst_uv_strd,
                                              WORD32 is_u_first,
                                              WORD32 disable_luma_copy);

typedef void ih264e_fmt_conv_420p_to_420sp_ft(UWORD8 *pu1_y_src, UWORD8 *pu1_u_src, UWORD8 *pu1_v_src,
                                              UWORD8 *pu1_y_dst, UWORD8 *pu1_uv_dst,
                                              UWORD16 u2_height, UWORD16 u2_width, UWORD16 src_y_strd,
                                              UWORD16 src_u_strd, UWORD16 src_v_strd,
                                              UWORD16 dst_y_strd, UWORD16 dst_uv_strd,
                                              UWORD32 convert_uv_only);

typedef void ih264e_fmt_conv_422i_to_420sp_ft(UWORD8 *pu1_y_buf,UWORD8 *pu1_u_buf,UWORD8 *pu1_v_buf,
                                              UWORD8 *pu1_422i_buf,
                                              WORD32 u4_y_width,WORD32 u4_y_height,
                                              WORD32 u4_y_stride,WORD32 u4_u_stride,WORD32 u4_v_stride,
                                              WORD32 u4_422i_stride);


/* C function declarations */
ih264e_fmt_conv_420sp_to_rgba8888_ft    ih264e_fmt_conv_420sp_to_rgba8888;
ih264e_fmt_conv_420sp_to_rgb565_ft      ih264e_fmt_conv_420sp_to_rgb565;
ih264e_fmt_conv_420sp_to_420sp_ft       ih264e_fmt_conv_420sp_to_420sp;
ih264e_fmt_conv_420sp_to_420p_ft        ih264e_fmt_conv_420sp_to_420p;
ih264e_fmt_conv_420p_to_420sp_ft        ih264e_fmt_conv_420p_to_420sp;
ih264e_fmt_conv_422i_to_420sp_ft        ih264e_fmt_conv_422i_to_420sp;

/* A9Q function declarations */
ih264e_fmt_conv_420sp_to_rgba8888_ft    ih264e_fmt_conv_420sp_to_rgba8888_a9q;
ih264e_fmt_conv_420sp_to_420sp_ft       ih264e_fmt_conv_420sp_to_420sp_a9q;
ih264e_fmt_conv_420sp_to_420p_ft        ih264e_fmt_conv_420sp_to_420p_a9q;
ih264e_fmt_conv_420p_to_420sp_ft        ih264e_fmt_conv_420p_to_420sp_a9q;
ih264e_fmt_conv_422i_to_420sp_ft        ih264e_fmt_conv_422i_to_420sp_a9q;


/* A9A function declarations */
ih264e_fmt_conv_420sp_to_rgba8888_ft ih264e_fmt_conv_420sp_to_rgba8888_a9a;
ih264e_fmt_conv_420sp_to_420sp_ft ih264e_fmt_conv_420sp_to_420sp_a9a;
ih264e_fmt_conv_420sp_to_420p_ft ih264e_fmt_conv_420sp_to_420p_a9a;

/* SSSe31 function declarations */
ih264e_fmt_conv_420sp_to_420p_ft ih264e_fmt_conv_420sp_to_420p_ssse31;

/* SSE4 function declarations */
ih264e_fmt_conv_420sp_to_420p_ft ih264e_fmt_conv_420sp_to_420p_sse42;

#endif /* IH264E_FMT_CONV_H_ */
