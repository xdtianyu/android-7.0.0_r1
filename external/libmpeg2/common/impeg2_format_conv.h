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
/*****************************************************************************/
/*                                                                           */
/*  File Name         : impeg2_format_conv.h                                */
/*                                                                           */
/*  Description       : Contains coefficients and constant reqquired for     */
/*                      converting from rgb and gray color spaces to yuv422i */
/*                      color space                                          */
/*                                                                           */
/*  List of Functions : None                                                 */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         27 08 2007  Naveen Kumar T        Draft                           */
/*                                                                           */
/*****************************************************************************/

#ifndef __IMPEG2_FORMAT_CONV_H__
#define __IMPEG2_FORMAT_CONV_H__

/*****************************************************************************/
/* Typedefs                                                                  */
/*****************************************************************************/

#define COEFF_0_Y       66
#define COEFF_1_Y       129
#define COEFF_2_Y       25
#define COEFF_0_U       -38
#define COEFF_1_U       -75
#define COEFF_2_U       112
#define COEFF_0_V       112
#define COEFF_1_V       -94
#define COEFF_2_V       -18
#define CONST_RGB_YUV1  4096
#define CONST_RGB_YUV2  32768
#define CONST_GRAY_YUV  128
#define COEF_2_V2_U  0xFFEE0070

#define COF_2Y_0Y          0X00190042
#define COF_1U_0U          0XFFB5FFDA
#define COF_1V_0V          0XFFA20070

/*****************************************************************************/
/* Enums */
/*****************************************************************************/
typedef enum {
GRAY_SCALE   = 0,
YUV444      = 1,
YUV420      = 2,
YUV422H     = 3,
YUV422V     = 4,
YUV411      = 5,
RGB24       = 6,
RGB24i      = 7
}input_format_t;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/
typedef void pf_copy_yuv420p_buf_t(UWORD8 *pu1_src_y,
                                   UWORD8 *pu1_src_u,
                                   UWORD8 *pu1_src_v,
                                   UWORD8 *pu1_dst_y,
                                   UWORD8 *pu1_dst_u,
                                   UWORD8 *pu1_dst_v,
                                   UWORD32 u4_width,
                                   UWORD32 u4_height,
                                   UWORD32 u4_src_stride_y,
                                   UWORD32 u4_src_stride_u,
                                   UWORD32 u4_src_stride_v,
                                   UWORD32 u4_dst_stride_y,
                                   UWORD32 u4_dst_stride_u,
                                   UWORD32 u4_dst_stride_v);

typedef void pf_fmt_conv_yuv420p_to_yuv422ile_t(UWORD8 *pu1_y,
                                                UWORD8 *pu1_u,
                                                UWORD8 *pu1_v,
                                                void *pv_yuv422i,
                                                UWORD32 u4_width,
                                                UWORD32 u4_height,
                                                UWORD32 u4_stride_y,
                                                UWORD32 u4_stride_u,
                                                UWORD32 u4_stride_v,
                                                UWORD32 u4_stride_yuv422i);

typedef void pf_fmt_conv_yuv420p_to_yuv420sp_t(UWORD8 *pu1_y,
                                               UWORD8 *pu1_u,
                                               UWORD8 *pu1_v,
                                               UWORD8 *pu1_dest_y,
                                               UWORD8 *pu1_dest_uv,
                                               UWORD32 u2_height,
                                               UWORD32 u2_width,
                                               UWORD32 u2_stridey,
                                               UWORD32 u2_strideu,
                                               UWORD32 u2_stridev,
                                               UWORD32 u2_dest_stride_y,
                                               UWORD32 u2_dest_stride_uv,
                                               UWORD32 convert_uv_only);

pf_copy_yuv420p_buf_t impeg2_copy_frm_yuv420p;
pf_fmt_conv_yuv420p_to_yuv422ile_t impeg2_fmt_conv_yuv420p_to_yuv422ile;
pf_fmt_conv_yuv420p_to_yuv420sp_t impeg2_fmt_conv_yuv420p_to_yuv420sp_vu;
pf_fmt_conv_yuv420p_to_yuv420sp_t impeg2_fmt_conv_yuv420p_to_yuv420sp_uv;

pf_fmt_conv_yuv420p_to_yuv420sp_t impeg2_fmt_conv_yuv420p_to_yuv420sp_uv_a9q;
pf_fmt_conv_yuv420p_to_yuv420sp_t impeg2_fmt_conv_yuv420p_to_yuv420sp_vu_a9q;

pf_fmt_conv_yuv420p_to_yuv420sp_t impeg2_fmt_conv_yuv420p_to_yuv420sp_uv_av8;
pf_fmt_conv_yuv420p_to_yuv420sp_t impeg2_fmt_conv_yuv420p_to_yuv420sp_vu_av8;


#endif /* __IMPEG2_FORMAT_CONV_H__ */
