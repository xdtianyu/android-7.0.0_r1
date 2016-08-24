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
*  ih264e_fmt_conv.c
*
* @brief
*  Contains functions for format conversion or frame copy of output buffer
*
* @author
*  ittiam
*
* @par List of Functions:
*  - ih264e_fmt_conv_420sp_to_rgb565()
*  - ih264e_fmt_conv_420sp_to_rgba8888()
*  - ih264e_fmt_conv_420sp_to_420sp()
*  - ih264e_fmt_conv_420sp_to_420sp_swap_uv()
*  - ih264e_fmt_conv_420sp_to_420p()
*  - ih264e_fmt_conv_420p_to_420sp()
*  - ih264e_fmt_conv_422i_to_420sp()
*  - ih264e_fmt_conv()
*
* @remarks
*  None
*
*******************************************************************************
*/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* System Include files */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* User Include files */
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264e.h"
#include "ithread.h"
#include "ih264_defs.h"
#include "ih264_debug.h"
#include "ime_distortion_metrics.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "ih264_error.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "ih264_macros.h"
#include "ih264_platform_macros.h"
#include "ih264_buf_mgr.h"
#include "ih264e_defs.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_fmt_conv.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

void ih264e_fmt_conv_420sp_to_rgb565(UWORD8 *pu1_y_src,
                                     UWORD8 *pu1_uv_src,
                                     UWORD16 *pu2_rgb_dst,
                                     WORD32 wd,
                                     WORD32 ht,
                                     WORD32 src_y_strd,
                                     WORD32 src_uv_strd,
                                     WORD32 dst_strd,
                                     WORD32 is_u_first)
{
    WORD16 i2_r, i2_g, i2_b;
    UWORD32 u4_r, u4_g, u4_b;
    WORD16 i2_i, i2_j;
    UWORD8 *pu1_y_src_nxt;
    UWORD16 *pu2_rgb_dst_NextRow;

    UWORD8 *pu1_u_src, *pu1_v_src;

    if (is_u_first)
    {
        pu1_u_src = (UWORD8 *) pu1_uv_src;
        pu1_v_src = (UWORD8 *) pu1_uv_src + 1;
    }
    else
    {
        pu1_u_src = (UWORD8 *) pu1_uv_src + 1;
        pu1_v_src = (UWORD8 *) pu1_uv_src;
    }

    pu1_y_src_nxt = pu1_y_src + src_y_strd;
    pu2_rgb_dst_NextRow = pu2_rgb_dst + dst_strd;

    for (i2_i = 0; i2_i < (ht >> 1); i2_i++)
    {
        for (i2_j = (wd >> 1); i2_j > 0; i2_j--)
        {
            i2_b = ((*pu1_u_src - 128) * COEFF4 >> 13);
            i2_g = ((*pu1_u_src - 128) * COEFF2 + (*pu1_v_src - 128) * COEFF3)
                            >> 13;
            i2_r = ((*pu1_v_src - 128) * COEFF1) >> 13;

            pu1_u_src += 2;
            pu1_v_src += 2;
            /* pixel 0 */
            /* B */
            u4_b = CLIP_U8(*pu1_y_src + i2_b);
            u4_b >>= 3;
            /* G */
            u4_g = CLIP_U8(*pu1_y_src + i2_g);
            u4_g >>= 2;
            /* R */
            u4_r = CLIP_U8(*pu1_y_src + i2_r);
            u4_r >>= 3;

            pu1_y_src++;
            *pu2_rgb_dst++ = ((u4_r << 11) | (u4_g << 5) | u4_b);

            /* pixel 1 */
            /* B */
            u4_b = CLIP_U8(*pu1_y_src + i2_b);
            u4_b >>= 3;
            /* G */
            u4_g = CLIP_U8(*pu1_y_src + i2_g);
            u4_g >>= 2;
            /* R */
            u4_r = CLIP_U8(*pu1_y_src + i2_r);
            u4_r >>= 3;

            pu1_y_src++;
            *pu2_rgb_dst++ = ((u4_r << 11) | (u4_g << 5) | u4_b);

            /* pixel 2 */
            /* B */
            u4_b = CLIP_U8(*pu1_y_src_nxt + i2_b);
            u4_b >>= 3;
            /* G */
            u4_g = CLIP_U8(*pu1_y_src_nxt + i2_g);
            u4_g >>= 2;
            /* R */
            u4_r = CLIP_U8(*pu1_y_src_nxt + i2_r);
            u4_r >>= 3;

            pu1_y_src_nxt++;
            *pu2_rgb_dst_NextRow++ = ((u4_r << 11) | (u4_g << 5) | u4_b);

            /* pixel 3 */
            /* B */
            u4_b = CLIP_U8(*pu1_y_src_nxt + i2_b);
            u4_b >>= 3;
            /* G */
            u4_g = CLIP_U8(*pu1_y_src_nxt + i2_g);
            u4_g >>= 2;
            /* R */
            u4_r = CLIP_U8(*pu1_y_src_nxt + i2_r);
            u4_r >>= 3;

            pu1_y_src_nxt++;
            *pu2_rgb_dst_NextRow++ = ((u4_r << 11) | (u4_g << 5) | u4_b);

        }

        pu1_u_src = pu1_u_src + src_uv_strd - wd;
        pu1_v_src = pu1_v_src + src_uv_strd - wd;

        pu1_y_src = pu1_y_src + (src_y_strd << 1) - wd;
        pu1_y_src_nxt = pu1_y_src_nxt + (src_y_strd << 1) - wd;

        pu2_rgb_dst = pu2_rgb_dst_NextRow - wd + dst_strd;
        pu2_rgb_dst_NextRow = pu2_rgb_dst_NextRow + (dst_strd << 1) - wd;
    }

}

void ih264e_fmt_conv_420sp_to_rgba8888(UWORD8 *pu1_y_src,
                                       UWORD8 *pu1_uv_src,
                                       UWORD32 *pu4_rgba_dst,
                                       WORD32 wd,
                                       WORD32 ht,
                                       WORD32 src_y_strd,
                                       WORD32 src_uv_strd,
                                       WORD32 dst_strd,
                                       WORD32 is_u_first)
{
    WORD16 i2_r, i2_g, i2_b;
    UWORD32 u4_r, u4_g, u4_b;
    WORD16 i2_i, i2_j;
    UWORD8 *pu1_y_src_nxt;
    UWORD32 *pu4_rgba_dst_NextRow;
    UWORD8 *pu1_u_src, *pu1_v_src;

    if (is_u_first)
    {
        pu1_u_src = (UWORD8 *) pu1_uv_src;
        pu1_v_src = (UWORD8 *) pu1_uv_src + 1;
    }
    else
    {
        pu1_u_src = (UWORD8 *) pu1_uv_src + 1;
        pu1_v_src = (UWORD8 *) pu1_uv_src;
    }

    pu1_y_src_nxt = pu1_y_src + src_y_strd;

    pu4_rgba_dst_NextRow = pu4_rgba_dst + dst_strd;

    for (i2_i = 0; i2_i < (ht >> 1); i2_i++)
    {
        for (i2_j = (wd >> 1); i2_j > 0; i2_j--)
        {
            i2_b = ((*pu1_u_src - 128) * COEFF4 >> 13);
            i2_g = ((*pu1_u_src - 128) * COEFF2 + (*pu1_v_src - 128) * COEFF3)
                            >> 13;
            i2_r = ((*pu1_v_src - 128) * COEFF1) >> 13;

            pu1_u_src += 2;
            pu1_v_src += 2;
            /* pixel 0 */
            /* B */
            u4_b = CLIP_U8(*pu1_y_src + i2_b);
            /* G */
            u4_g = CLIP_U8(*pu1_y_src + i2_g);
            /* R */
            u4_r = CLIP_U8(*pu1_y_src + i2_r);

            pu1_y_src++;
            *pu4_rgba_dst++ = ((u4_r << 16) | (u4_g << 8) | (u4_b << 0));

            /* pixel 1 */
            /* B */
            u4_b = CLIP_U8(*pu1_y_src + i2_b);
            /* G */
            u4_g = CLIP_U8(*pu1_y_src + i2_g);
            /* R */
            u4_r = CLIP_U8(*pu1_y_src + i2_r);

            pu1_y_src++;
            *pu4_rgba_dst++ = ((u4_r << 16) | (u4_g << 8) | (u4_b << 0));

            /* pixel 2 */
            /* B */
            u4_b = CLIP_U8(*pu1_y_src_nxt + i2_b);
            /* G */
            u4_g = CLIP_U8(*pu1_y_src_nxt + i2_g);
            /* R */
            u4_r = CLIP_U8(*pu1_y_src_nxt + i2_r);

            pu1_y_src_nxt++;
            *pu4_rgba_dst_NextRow++ =
                            ((u4_r << 16) | (u4_g << 8) | (u4_b << 0));

            /* pixel 3 */
            /* B */
            u4_b = CLIP_U8(*pu1_y_src_nxt + i2_b);
            /* G */
            u4_g = CLIP_U8(*pu1_y_src_nxt + i2_g);
            /* R */
            u4_r = CLIP_U8(*pu1_y_src_nxt + i2_r);

            pu1_y_src_nxt++;
            *pu4_rgba_dst_NextRow++ =
                            ((u4_r << 16) | (u4_g << 8) | (u4_b << 0));

        }

        pu1_u_src = pu1_u_src + src_uv_strd - wd;
        pu1_v_src = pu1_v_src + src_uv_strd - wd;

        pu1_y_src = pu1_y_src + (src_y_strd << 1) - wd;
        pu1_y_src_nxt = pu1_y_src_nxt + (src_y_strd << 1) - wd;

        pu4_rgba_dst = pu4_rgba_dst_NextRow - wd + dst_strd;
        pu4_rgba_dst_NextRow = pu4_rgba_dst_NextRow + (dst_strd << 1) - wd;
    }

}

/**
*******************************************************************************
*
* @brief Function used for copying a 420SP buffer
*
* @par   Description
*  Function used for copying a 420SP buffer
*
* @param[in] pu1_y_src
*  Input Y pointer
*
* @param[in] pu1_uv_src
*  Input UV pointer (UV is interleaved either in UV or VU format)
*
* @param[in] pu1_y_dst
*  Output Y pointer
*
* @param[in] pu1_uv_dst
*  Output UV pointer (UV is interleaved in the same format as that of input)
*
* @param[in] wd
*  Width
*
* @param[in] ht
*  Height
*
* @param[in] src_y_strd
*  Input Y Stride
*
* @param[in] src_uv_strd
*  Input UV stride
*
* @param[in] dst_y_strd
*  Output Y stride
*
* @param[in] dst_uv_strd
*  Output UV stride
*
* @returns None
*
* @remarks In case there is a need to perform partial frame copy then
* by passion appropriate source and destination pointers and appropriate
* values for wd and ht it can be done
*
*******************************************************************************
*/
void ih264e_fmt_conv_420sp_to_420sp(UWORD8 *pu1_y_src,
                                    UWORD8 *pu1_uv_src,
                                    UWORD8 *pu1_y_dst,
                                    UWORD8 *pu1_uv_dst,
                                    WORD32 wd,
                                    WORD32 ht,
                                    WORD32 src_y_strd,
                                    WORD32 src_uv_strd,
                                    WORD32 dst_y_strd,
                                    WORD32 dst_uv_strd)
{
    UWORD8 *pu1_src, *pu1_dst;
    WORD32 num_rows, num_cols, src_strd, dst_strd;
    WORD32 i;

    /* copy luma */
    pu1_src = (UWORD8 *) pu1_y_src;
    pu1_dst = (UWORD8 *) pu1_y_dst;

    num_rows = ht;
    num_cols = wd;

    src_strd = src_y_strd;
    dst_strd = dst_y_strd;

    for (i = 0; i < num_rows; i++)
    {
        memcpy(pu1_dst, pu1_src, num_cols);
        pu1_dst += dst_strd;
        pu1_src += src_strd;
    }

    /* copy U and V */
    pu1_src = (UWORD8 *) pu1_uv_src;
    pu1_dst = (UWORD8 *) pu1_uv_dst;

    num_rows = ht >> 1;
    num_cols = wd;

    src_strd = src_uv_strd;
    dst_strd = dst_uv_strd;

    for (i = 0; i < num_rows; i++)
    {
        memcpy(pu1_dst, pu1_src, num_cols);
        pu1_dst += dst_strd;
        pu1_src += src_strd;
    }
    return;
}


void ih264e_fmt_conv_420sp_to_420sp_swap_uv(UWORD8 *pu1_y_src,
                                            UWORD8 *pu1_uv_src,
                                            UWORD8 *pu1_y_dst,
                                            UWORD8 *pu1_uv_dst,
                                            WORD32 wd,
                                            WORD32 ht,
                                            WORD32 src_y_strd,
                                            WORD32 src_uv_strd,
                                            WORD32 dst_y_strd,
                                            WORD32 dst_uv_strd)
{
    UWORD8 *pu1_src, *pu1_dst;
    WORD32 num_rows, num_cols, src_strd, dst_strd;
    WORD32 i;

    /* copy luma */
    pu1_src = (UWORD8 *) pu1_y_src;
    pu1_dst = (UWORD8 *) pu1_y_dst;

    num_rows = ht;
    num_cols = wd;

    src_strd = src_y_strd;
    dst_strd = dst_y_strd;

    for (i = 0; i < num_rows; i++)
    {
        memcpy(pu1_dst, pu1_src, num_cols);
        pu1_dst += dst_strd;
        pu1_src += src_strd;
    }

    /* copy U and V */
    pu1_src = (UWORD8 *) pu1_uv_src;
    pu1_dst = (UWORD8 *) pu1_uv_dst;

    num_rows = ht >> 1;
    num_cols = wd;

    src_strd = src_uv_strd;
    dst_strd = dst_uv_strd;

    for (i = 0; i < num_rows; i++)
    {
        WORD32 j;
        for (j = 0; j < num_cols; j += 2)
        {
            pu1_dst[j + 0] = pu1_src[j + 1];
            pu1_dst[j + 1] = pu1_src[j + 0];
        }
        pu1_dst += dst_strd;
        pu1_src += src_strd;
    }
    return;
}

void ih264e_fmt_conv_420sp_to_420p(UWORD8 *pu1_y_src,
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
                                   WORD32 disable_luma_copy)
{
    UWORD8 *pu1_src, *pu1_dst;
    UWORD8 *pu1_u_src, *pu1_v_src;
    WORD32 num_rows, num_cols, src_strd, dst_strd;
    WORD32 i, j;

    if (0 == disable_luma_copy)
    {
        /* copy luma */
        pu1_src = (UWORD8 *) pu1_y_src;
        pu1_dst = (UWORD8 *) pu1_y_dst;

        num_rows = ht;
        num_cols = wd;

        src_strd = src_y_strd;
        dst_strd = dst_y_strd;

        for (i = 0; i < num_rows; i++)
        {
            memcpy(pu1_dst, pu1_src, num_cols);
            pu1_dst += dst_strd;
            pu1_src += src_strd;
        }
    }
    /* de-interleave U and V and copy to destination */
    if (is_u_first)
    {
        pu1_u_src = (UWORD8 *) pu1_uv_src;
        pu1_v_src = (UWORD8 *) pu1_uv_src + 1;
    }
    else
    {
        pu1_u_src = (UWORD8 *) pu1_uv_src + 1;
        pu1_v_src = (UWORD8 *) pu1_uv_src;
    }

    num_rows = ht >> 1;
    num_cols = wd >> 1;

    src_strd = src_uv_strd;
    dst_strd = dst_uv_strd;

    for (i = 0; i < num_rows; i++)
    {
        for (j = 0; j < num_cols; j++)
        {
            pu1_u_dst[j] = pu1_u_src[j * 2];
            pu1_v_dst[j] = pu1_v_src[j * 2];
        }

        pu1_u_dst += dst_strd;
        pu1_v_dst += dst_strd;
        pu1_u_src += src_strd;
        pu1_v_src += src_strd;
    }
    return;
}

/**
*******************************************************************************
*
* @brief Function used to perform color space conversion from 420P to 420SP
*
* @par   Description
* Function used to perform color space conversion from 420P to 420SP
*
* @param[in] pu1_y_src
*  Input Y pointer
*
* @param[in] pu1_u_src
*  Input U pointer
*
* @param[in] pu1_v_dst
*  Input V pointer
*
* @param[in] pu1_y_dst
*  Output Y pointer
*
* @param[in] pu1_uv_dst
*  Output UV pointer
*
* @param[in] u4_width
*  Width
*
* @param[in] u4_height
*  Height
*
* @param[in] src_y_strd
*  Input Y Stride
*
* @param[in] src_u_strd
*  Input U stride
*
* @param[in] src_v_strd
*  Input V stride
*
* @param[in] dst_y_strd
*  Output Y stride
*
* @param[in] dst_uv_strd
*  Output UV stride
*
* @param[in] convert_uv_only
*  Flag to indicate if only UV copy needs to be done
*
* @returns none
*
* @remarks In case there is a need to perform partial frame copy then
* by passion appropriate source and destination pointers and appropriate
* values for wd and ht it can be done
*
*******************************************************************************
*/
void ih264e_fmt_conv_420p_to_420sp(UWORD8 *pu1_y_src,
                                   UWORD8 *pu1_u_src,
                                   UWORD8 *pu1_v_src,
                                   UWORD8 *pu1_y_dst,
                                   UWORD8 *pu1_uv_dst,
                                   UWORD16 u2_height,
                                   UWORD16 u2_width,
                                   UWORD16 src_y_strd,
                                   UWORD16 src_u_strd,
                                   UWORD16 src_v_strd,
                                   UWORD16 dst_y_strd,
                                   UWORD16 dst_uv_strd,
                                   UWORD32 convert_uv_only)
{
    UWORD8 *pu1_src, *pu1_dst;
    UWORD8 *pu1_src_u, *pu1_src_v;
    UWORD16 i;
    UWORD32 u2_width_uv;
    UWORD32 dest_inc_Y = 0, dest_inc_UV = 0;

    dest_inc_UV = dst_uv_strd;

    if (0 == convert_uv_only)
    {

        /* Copy Y buffer */
        pu1_dst = (UWORD8 *) pu1_y_dst;
        pu1_src = (UWORD8 *) pu1_y_src;

        dest_inc_Y = dst_y_strd;

        for (i = 0; i < u2_height; i++)
        {
            memcpy((void *) pu1_dst, (void *) pu1_src, u2_width);
            pu1_dst += dest_inc_Y;
            pu1_src += src_y_strd;
        }
    }

    /* Interleave Cb and Cr buffers */
    pu1_src_u = pu1_u_src;
    pu1_src_v = pu1_v_src;
    pu1_dst = pu1_uv_dst;

    u2_height = (u2_height + 1) >> 1;
    u2_width_uv = (u2_width + 1) >> 1;
    for (i = 0; i < u2_height; i++)
    {
        UWORD32 j;
        for (j = 0; j < u2_width_uv; j++)
        {
            *pu1_dst++ = *pu1_src_u++;
            *pu1_dst++ = *pu1_src_v++;
        }

        pu1_dst += dest_inc_UV - u2_width;
        pu1_src_u += src_u_strd - u2_width_uv;
        pu1_src_v += src_v_strd - u2_width_uv;
    }
}

/**
*******************************************************************************
*
* @brief Function used to convert 422 interleaved to 420sp
*
* @par   Description
*  Function used to convert 422 interleaved to 420sp
*
* @param[in] pu1_y_buf
*  Output Y pointer
*
* @param[in] pu1_u_buf
*  Output u pointer
*
* @param[in[ pu1_v_buf
*  Output V pointer
*
* @param[in] pu1_422i_buf
*  Input 422i pointer
*
* @param[in] u4_y_width
*  Width of Y component
*
* @param[in] u4_y_height
*  Height of Y component
*
* @param[in] u4_y_stride
*  Stride of pu1_y_buf
*
* @param[in] u4_u_stride
*  Stride of pu1_u_buf
*
* @param[in] u4_v_stride
*  Stride of pu1_v_buf
*
* @param[in] u4_422i_stride
*  Stride of pu1_422i_buf
*
* @returns None
*
* @remarks For conversion
* pu1_v_buf = pu1_u_buf+1
* u4_u_stride = u4_v_stride
*
* The extra parameters are for maintaining API with assembly function
*
*******************************************************************************
*/
void ih264e_fmt_conv_422i_to_420sp(UWORD8 *pu1_y_buf,
                                   UWORD8 *pu1_u_buf,
                                   UWORD8 *pu1_v_buf,
                                   UWORD8 *pu1_422i_buf,
                                   WORD32 u4_y_width,
                                   WORD32 u4_y_height,
                                   WORD32 u4_y_stride,
                                   WORD32 u4_u_stride,
                                   WORD32 u4_v_stride,
                                   WORD32 u4_422i_stride)
{
    WORD32 row, col;
    UWORD8 *row_even_422 = pu1_422i_buf;
    UWORD8 *row_odd_422 = row_even_422 + (u4_422i_stride << 1);
    UWORD8 *row_even_luma = pu1_y_buf;
    /* Since at the end of loop, we have row_even_luma += (luma_width << 1),
     * it should be same here right? */
    UWORD8 *row_odd_luma = row_even_luma + u4_y_stride;
    UWORD8 *row_cb = pu1_u_buf;
    UWORD8 *row_cr = pu1_v_buf;

    for (row = 0; row < u4_y_height; row = row + 2)
    {
        for (col = 0; col < (u4_y_width << 1); col = col + 4)
        {
            UWORD8 cb_even = row_even_422[col];
            UWORD8 cr_even = row_even_422[col + 2];

            row_cb[col >> 1] = cb_even;
            row_cr[col >> 1] = cr_even;

            row_even_luma[col >> 1] = row_even_422[col + 1];
            row_even_luma[(col >> 1) + 1] = row_even_422[col + 3];

            row_odd_luma[col >> 1] = row_odd_422[col + 1];
            row_odd_luma[(col >> 1) + 1] = row_odd_422[col + 3];
        }

        row_even_422 += (u4_422i_stride << 2);
        row_odd_422 += (u4_422i_stride << 2);

        row_even_luma += (u4_y_stride << 1);
        row_odd_luma += (u4_y_stride << 1);

        row_cb += u4_u_stride;
        row_cr += u4_v_stride;
    }
}

/**
*******************************************************************************
*
* @brief Function used from format conversion or frame copy
*
* @par   Description
* Function used from copying or converting a reference frame to display buffer
* in non shared mode
*
* @param[in] pu1_y_dst
*  Output Y pointer
*
* @param[in] pu1_u_dst
*  Output U/UV pointer ( UV is interleaved in the same format as that of input)
*
* @param[in] pu1_v_dst
*  Output V pointer ( used in 420P output case)
*
* @param[in] u4_dst_y_strd
*  Stride of destination Y buffer
*
* @param[in] u4_dst_u_strd
*  Stride of destination  U/V buffer
*
* @param[in] blocking
*  To indicate whether format conversion should wait till frame is reconstructed
*  and then return after complete copy is done. To be set to 1 when called at the
*  end of frame processing and set to 0 when called between frame processing modules
*  in order to utilize available MCPS
*
* @returns error status
*
* @remarks
* Assumes that the stride of U and V buffers are same.
* This is correct in most cases
* If a case comes where this is not true we need to modify the fmt conversion
* functions called inside also
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_fmt_conv(codec_t *ps_codec,
                               pic_buf_t *ps_pic,
                               UWORD8 *pu1_y_dst,
                               UWORD8 *pu1_u_dst,
                               UWORD8 *pu1_v_dst,
                               UWORD32 u4_dst_y_strd,
                               UWORD32 u4_dst_uv_strd,
                               WORD32 cur_row,
                               WORD32 num_rows)
{
    IH264E_ERROR_T ret = IH264E_SUCCESS;
    UWORD8 *pu1_y_src, *pu1_uv_src;
    UWORD8 *pu1_y_dst_tmp, *pu1_uv_dst_tmp;
    UWORD8 *pu1_u_dst_tmp, *pu1_v_dst_tmp;
    UWORD16 *pu2_rgb_dst_tmp;
    UWORD32 *pu4_rgb_dst_tmp;
    WORD32 is_u_first;
    UWORD8 *pu1_luma;
    UWORD8 *pu1_chroma;
    WORD32 dst_stride, wd;


    if (0 == num_rows)
        return ret;

    pu1_luma = ps_pic->pu1_luma;
    pu1_chroma = ps_pic->pu1_chroma;


    dst_stride = ps_codec->s_cfg.u4_wd;
    wd = ps_codec->s_cfg.u4_disp_wd;
    is_u_first = (IV_YUV_420SP_UV == ps_codec->e_codec_color_format) ? 1 : 0;

    /* In case of 420P output luma copy is disabled for shared mode */
    {
        pu1_y_src = pu1_luma + cur_row * ps_codec->i4_rec_strd;
        pu1_uv_src = pu1_chroma + (cur_row / 2) * ps_codec->i4_rec_strd;

        pu2_rgb_dst_tmp = (UWORD16 *) pu1_y_dst;
        pu2_rgb_dst_tmp += cur_row * dst_stride;
        pu4_rgb_dst_tmp = (UWORD32 *) pu1_y_dst;
        pu4_rgb_dst_tmp += cur_row * dst_stride;

        pu1_y_dst_tmp = pu1_y_dst + cur_row * u4_dst_y_strd;
        pu1_uv_dst_tmp = pu1_u_dst + (cur_row / 2) * u4_dst_uv_strd;
        pu1_u_dst_tmp = pu1_u_dst + (cur_row / 2) * u4_dst_uv_strd;
        pu1_v_dst_tmp = pu1_v_dst + (cur_row / 2) * u4_dst_uv_strd;

        /* If the call is non-blocking and there are no rows to be copied then return */
        /* In non-shared mode, reference buffers are in 420SP UV format,
         * if output also is in 420SP_UV, then just copy
         * if output is in 420SP_VU then swap UV values
         */
        if ((IV_YUV_420SP_UV == ps_codec->s_cfg.e_recon_color_fmt) ||
                        (IV_YUV_420SP_VU == ps_codec->s_cfg.e_recon_color_fmt))
        {
            ih264e_fmt_conv_420sp_to_420sp(pu1_y_src, pu1_uv_src, pu1_y_dst_tmp,
                                           pu1_uv_dst_tmp, wd, num_rows,
                                           ps_codec->i4_rec_strd,
                                           ps_codec->i4_rec_strd, u4_dst_y_strd,
                                           u4_dst_uv_strd);
        }
        else if (IV_YUV_420P == ps_codec->s_cfg.e_recon_color_fmt)
        {
            ih264e_fmt_conv_420sp_to_420p(pu1_y_src, pu1_uv_src, pu1_y_dst_tmp,
                                          pu1_u_dst_tmp, pu1_v_dst_tmp, wd,
                                          num_rows, ps_codec->i4_rec_strd,
                                          ps_codec->i4_rec_strd, u4_dst_y_strd,
                                          u4_dst_uv_strd, is_u_first, 0);
        }
    }
    return(ret);
}

