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
 *  impeg2_inter_pred_sse42_intr.c
 *
 * @brief
 *  Contains Motion compensation function definitions for MPEG2 decoder
 *
 * @author
 *  Mohit [100664]
 *
 * - impeg2_copy_mb_sse42()
 * - impeg2_interpolate_sse42()
 * - impeg2_mc_halfx_halfy_8x8_sse42()
 * - impeg2_mc_halfx_fully_8x8_sse42()
 * - impeg2_mc_fullx_halfy_8x8_sse42()
 * - impeg2_mc_fullx_fully_8x8_sse42()
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include "iv_datatypedef.h"
#include "impeg2_macros.h"
#include "impeg2_defs.h"
#include "impeg2_inter_pred.h"

#include <immintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>

/*******************************************************************************
*  Function Name   : impeg2_copy_mb
*
*  Description     : copies 3 components to the frame from mc_buf
*
*  Arguments       :
*  src_buf         : Source Buffer
*  dst_buf         : Destination Buffer
*  src_wd          : Source Width
*  dst_wd          : destination Width
*
*  Values Returned : None
*******************************************************************************/
void impeg2_copy_mb_sse42(yuv_buf_t *src_buf,
                    yuv_buf_t *dst_buf,
                    UWORD32 src_wd,
                    UWORD32 dst_wd)
{
    UWORD8 *src;
    UWORD8 *dst;
    __m128i src_r0, src_r1, src_r2, src_r3;

    /*******************************************************/
    /* copy Y                                              */
    /*******************************************************/
    src = src_buf->pu1_y;
    dst = dst_buf->pu1_y;
    // Row 0-3
    src_r0 = _mm_loadu_si128((__m128i *) (src));
    src_r1 = _mm_loadu_si128((__m128i *) (src + src_wd));
    src_r2 = _mm_loadu_si128((__m128i *) (src + 2 * src_wd));
    src_r3 = _mm_loadu_si128((__m128i *) (src + 3 * src_wd));

    _mm_storeu_si128((__m128i *) dst, src_r0);
    _mm_storeu_si128((__m128i *) (dst + dst_wd), src_r1);
    _mm_storeu_si128((__m128i *) (dst + 2 * dst_wd), src_r2);
    _mm_storeu_si128((__m128i *) (dst + 3 * dst_wd), src_r3);

    // Row 4-7
    src += 4 * src_wd;
    dst += 4 * dst_wd;
    src_r0 = _mm_loadu_si128((__m128i *) (src));
    src_r1 = _mm_loadu_si128((__m128i *) (src + src_wd));
    src_r2 = _mm_loadu_si128((__m128i *) (src + 2 * src_wd));
    src_r3 = _mm_loadu_si128((__m128i *) (src + 3 * src_wd));

    _mm_storeu_si128((__m128i *) dst, src_r0);
    _mm_storeu_si128((__m128i *) (dst + dst_wd), src_r1);
    _mm_storeu_si128((__m128i *) (dst + 2 * dst_wd), src_r2);
    _mm_storeu_si128((__m128i *) (dst + 3 * dst_wd), src_r3);

    // Row 8-11
    src += 4 * src_wd;
    dst += 4 * dst_wd;
    src_r0 = _mm_loadu_si128((__m128i *) (src));
    src_r1 = _mm_loadu_si128((__m128i *) (src + src_wd));
    src_r2 = _mm_loadu_si128((__m128i *) (src + 2 * src_wd));
    src_r3 = _mm_loadu_si128((__m128i *) (src + 3 * src_wd));

    _mm_storeu_si128((__m128i *) dst, src_r0);
    _mm_storeu_si128((__m128i *) (dst + dst_wd), src_r1);
    _mm_storeu_si128((__m128i *) (dst + 2 * dst_wd), src_r2);
    _mm_storeu_si128((__m128i *) (dst + 3 * dst_wd), src_r3);

    // Row 12-15
    src += 4 * src_wd;
    dst += 4 * dst_wd;
    src_r0 = _mm_loadu_si128((__m128i *) (src));
    src_r1 = _mm_loadu_si128((__m128i *) (src + src_wd));
    src_r2 = _mm_loadu_si128((__m128i *) (src + 2 * src_wd));
    src_r3 = _mm_loadu_si128((__m128i *) (src + 3 * src_wd));

    _mm_storeu_si128((__m128i *) dst, src_r0);
    _mm_storeu_si128((__m128i *) (dst + dst_wd), src_r1);
    _mm_storeu_si128((__m128i *) (dst + 2 * dst_wd), src_r2);
    _mm_storeu_si128((__m128i *) (dst + 3 * dst_wd), src_r3);

    src_wd >>= 1;
    dst_wd >>= 1;

    /*******************************************************/
    /* copy U                                              */
    /*******************************************************/
    src = src_buf->pu1_u;
    dst = dst_buf->pu1_u;

    // Row 0-3
    src_r0 =  _mm_loadl_epi64((__m128i *)src);
    src_r1 =  _mm_loadl_epi64((__m128i *)(src + src_wd));
    src_r2 =  _mm_loadl_epi64((__m128i *)(src + 2 * src_wd));
    src_r3 =  _mm_loadl_epi64((__m128i *)(src + 3 * src_wd));

    _mm_storel_epi64((__m128i *)dst, src_r0);
    _mm_storel_epi64((__m128i *)(dst + dst_wd), src_r1);
    _mm_storel_epi64((__m128i *)(dst + 2 * dst_wd), src_r2);
    _mm_storel_epi64((__m128i *)(dst + 3 * dst_wd), src_r3);

    // Row 4-7
    src += 4 * src_wd;
    dst += 4 * dst_wd;

    src_r0 =  _mm_loadl_epi64((__m128i *)src);
    src_r1 =  _mm_loadl_epi64((__m128i *)(src + src_wd));
    src_r2 =  _mm_loadl_epi64((__m128i *)(src + 2 * src_wd));
    src_r3 =  _mm_loadl_epi64((__m128i *)(src + 3 * src_wd));

    _mm_storel_epi64((__m128i *)dst, src_r0);
    _mm_storel_epi64((__m128i *)(dst + dst_wd), src_r1);
    _mm_storel_epi64((__m128i *)(dst + 2 * dst_wd), src_r2);
    _mm_storel_epi64((__m128i *)(dst + 3 * dst_wd), src_r3);

    /*******************************************************/
    /* copy V                                              */
    /*******************************************************/
    src = src_buf->pu1_v;
    dst = dst_buf->pu1_v;
    // Row 0-3
    src_r0 =  _mm_loadl_epi64((__m128i *)src);
    src_r1 =  _mm_loadl_epi64((__m128i *)(src + src_wd));
    src_r2 =  _mm_loadl_epi64((__m128i *)(src + 2 * src_wd));
    src_r3 =  _mm_loadl_epi64((__m128i *)(src + 3 * src_wd));

    _mm_storel_epi64((__m128i *)dst, src_r0);
    _mm_storel_epi64((__m128i *)(dst + dst_wd), src_r1);
    _mm_storel_epi64((__m128i *)(dst + 2 * dst_wd), src_r2);
    _mm_storel_epi64((__m128i *)(dst + 3 * dst_wd), src_r3);

    // Row 4-7
    src += 4 * src_wd;
    dst += 4 * dst_wd;

    src_r0 =  _mm_loadl_epi64((__m128i *)src);
    src_r1 =  _mm_loadl_epi64((__m128i *)(src + src_wd));
    src_r2 =  _mm_loadl_epi64((__m128i *)(src + 2 * src_wd));
    src_r3 =  _mm_loadl_epi64((__m128i *)(src + 3 * src_wd));

    _mm_storel_epi64((__m128i *)dst, src_r0);
    _mm_storel_epi64((__m128i *)(dst + dst_wd), src_r1);
    _mm_storel_epi64((__m128i *)(dst + 2 * dst_wd), src_r2);
    _mm_storel_epi64((__m128i *)(dst + 3 * dst_wd), src_r3);
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_interpolate                                       */
/*                                                                           */
/*  Description   : averages the contents of buf_src1 and buf_src2 and stores*/
/*                  result in buf_dst                                        */
/*                                                                           */
/*  Inputs        : buf_src1 -  First Source                                 */
/*                  buf_src2 -  Second Source                                */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Avg the values from two sources and store the result in  */
/*                  destination buffer                                       */
/*                                                                           */
/*  Outputs       : buf_dst  -  Avg of contents of buf_src1 and buf_src2     */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : Assumes that all 3 buffers are of same size              */
/*                                                                           */
/*****************************************************************************/
void impeg2_interpolate_sse42(yuv_buf_t *buf_src1,
                        yuv_buf_t *buf_src2,
                        yuv_buf_t *buf_dst,
                        UWORD32 stride)
{
    UWORD8 *src1, *src2;
    UWORD8 *dst;
    __m128i src1_r0, src1_r1, src1_r2, src1_r3;
    __m128i src2_r0, src2_r1, src2_r2, src2_r3;

    /*******************************************************/
    /* interpolate Y                                       */
    /*******************************************************/
    src1 = buf_src1->pu1_y;
    src2 = buf_src2->pu1_y;
    dst  = buf_dst->pu1_y;
    // Row 0-3
    src1_r0 = _mm_loadu_si128((__m128i *) (src1));
    src1_r1 = _mm_loadu_si128((__m128i *) (src1 + 16));
    src1_r2 = _mm_loadu_si128((__m128i *) (src1 + 2 * 16));
    src1_r3 = _mm_loadu_si128((__m128i *) (src1 + 3 * 16));

    src2_r0 = _mm_loadu_si128((__m128i *) (src2));
    src2_r1 = _mm_loadu_si128((__m128i *) (src2 + 16));
    src2_r2 = _mm_loadu_si128((__m128i *) (src2 + 2 * 16));
    src2_r3 = _mm_loadu_si128((__m128i *) (src2 + 3 * 16));

    src1_r0 = _mm_avg_epu8 (src1_r0, src2_r0);
    src1_r1 = _mm_avg_epu8 (src1_r1, src2_r1);
    src1_r2 = _mm_avg_epu8 (src1_r2, src2_r2);
    src1_r3 = _mm_avg_epu8 (src1_r3, src2_r3);

    _mm_storeu_si128((__m128i *) dst, src1_r0);
    _mm_storeu_si128((__m128i *) (dst + stride), src1_r1);
    _mm_storeu_si128((__m128i *) (dst + 2 * stride), src1_r2);
    _mm_storeu_si128((__m128i *) (dst + 3 * stride), src1_r3);

    // Row 4-7
    src1 += 4 * 16;
    src2 += 4 * 16;
    dst += 4 * stride;
    src1_r0 = _mm_loadu_si128((__m128i *) (src1));
    src1_r1 = _mm_loadu_si128((__m128i *) (src1 + 16));
    src1_r2 = _mm_loadu_si128((__m128i *) (src1 + 2 * 16));
    src1_r3 = _mm_loadu_si128((__m128i *) (src1 + 3 * 16));

    src2_r0 = _mm_loadu_si128((__m128i *) (src2));
    src2_r1 = _mm_loadu_si128((__m128i *) (src2 + 16));
    src2_r2 = _mm_loadu_si128((__m128i *) (src2 + 2 * 16));
    src2_r3 = _mm_loadu_si128((__m128i *) (src2 + 3 * 16));

    src1_r0 = _mm_avg_epu8 (src1_r0, src2_r0);
    src1_r1 = _mm_avg_epu8 (src1_r1, src2_r1);
    src1_r2 = _mm_avg_epu8 (src1_r2, src2_r2);
    src1_r3 = _mm_avg_epu8 (src1_r3, src2_r3);

    _mm_storeu_si128((__m128i *) dst, src1_r0);
    _mm_storeu_si128((__m128i *) (dst + stride), src1_r1);
    _mm_storeu_si128((__m128i *) (dst + 2 * stride), src1_r2);
    _mm_storeu_si128((__m128i *) (dst + 3 * stride), src1_r3);

    // Row 8-11
    src1 += 4 * 16;
    src2 += 4 * 16;
    dst += 4 * stride;
    src1_r0 = _mm_loadu_si128((__m128i *) (src1));
    src1_r1 = _mm_loadu_si128((__m128i *) (src1 + 16));
    src1_r2 = _mm_loadu_si128((__m128i *) (src1 + 2 * 16));
    src1_r3 = _mm_loadu_si128((__m128i *) (src1 + 3 * 16));

    src2_r0 = _mm_loadu_si128((__m128i *) (src2));
    src2_r1 = _mm_loadu_si128((__m128i *) (src2 + 16));
    src2_r2 = _mm_loadu_si128((__m128i *) (src2 + 2 * 16));
    src2_r3 = _mm_loadu_si128((__m128i *) (src2 + 3 * 16));

    src1_r0 = _mm_avg_epu8 (src1_r0, src2_r0);
    src1_r1 = _mm_avg_epu8 (src1_r1, src2_r1);
    src1_r2 = _mm_avg_epu8 (src1_r2, src2_r2);
    src1_r3 = _mm_avg_epu8 (src1_r3, src2_r3);

    _mm_storeu_si128((__m128i *) dst, src1_r0);
    _mm_storeu_si128((__m128i *) (dst + stride), src1_r1);
    _mm_storeu_si128((__m128i *) (dst + 2 * stride), src1_r2);
    _mm_storeu_si128((__m128i *) (dst + 3 * stride), src1_r3);

    // Row 12-15
    src1 += 4 * 16;
    src2 += 4 * 16;
    dst += 4 * stride;
    src1_r0 = _mm_loadu_si128((__m128i *) (src1));
    src1_r1 = _mm_loadu_si128((__m128i *) (src1 + 16));
    src1_r2 = _mm_loadu_si128((__m128i *) (src1 + 2 * 16));
    src1_r3 = _mm_loadu_si128((__m128i *) (src1 + 3 * 16));

    src2_r0 = _mm_loadu_si128((__m128i *) (src2));
    src2_r1 = _mm_loadu_si128((__m128i *) (src2 + 16));
    src2_r2 = _mm_loadu_si128((__m128i *) (src2 + 2 * 16));
    src2_r3 = _mm_loadu_si128((__m128i *) (src2 + 3 * 16));

    src1_r0 = _mm_avg_epu8 (src1_r0, src2_r0);
    src1_r1 = _mm_avg_epu8 (src1_r1, src2_r1);
    src1_r2 = _mm_avg_epu8 (src1_r2, src2_r2);
    src1_r3 = _mm_avg_epu8 (src1_r3, src2_r3);

    _mm_storeu_si128((__m128i *) dst, src1_r0);
    _mm_storeu_si128((__m128i *) (dst + stride), src1_r1);
    _mm_storeu_si128((__m128i *) (dst + 2 * stride), src1_r2);
    _mm_storeu_si128((__m128i *) (dst + 3 * stride), src1_r3);

    stride >>= 1;

    /*******************************************************/
    /* interpolate U                                       */
    /*******************************************************/
    src1 = buf_src1->pu1_u;
    src2 = buf_src2->pu1_u;
    dst  = buf_dst->pu1_u;
    // Row 0-3
    src1_r0 = _mm_loadl_epi64((__m128i *) (src1));
    src1_r1 = _mm_loadl_epi64((__m128i *) (src1 + 8));
    src1_r2 = _mm_loadl_epi64((__m128i *) (src1 + 2 * 8));
    src1_r3 = _mm_loadl_epi64((__m128i *) (src1 + 3 * 8));

    src2_r0 = _mm_loadl_epi64((__m128i *) (src2));
    src2_r1 = _mm_loadl_epi64((__m128i *) (src2 + 8));
    src2_r2 = _mm_loadl_epi64((__m128i *) (src2 + 2 * 8));
    src2_r3 = _mm_loadl_epi64((__m128i *) (src2 + 3 * 8));

    src1_r0 = _mm_avg_epu8 (src1_r0, src2_r0);
    src1_r1 = _mm_avg_epu8 (src1_r1, src2_r1);
    src1_r2 = _mm_avg_epu8 (src1_r2, src2_r2);
    src1_r3 = _mm_avg_epu8 (src1_r3, src2_r3);

    _mm_storel_epi64((__m128i *) dst, src1_r0);
    _mm_storel_epi64((__m128i *) (dst + stride), src1_r1);
    _mm_storel_epi64((__m128i *) (dst + 2 * stride), src1_r2);
    _mm_storel_epi64((__m128i *) (dst + 3 * stride), src1_r3);

    // Row 4-7
    src1 += 4 * 8;
    src2 += 4 * 8;
    dst += 4 * stride;

    src1_r0 = _mm_loadl_epi64((__m128i *) (src1));
    src1_r1 = _mm_loadl_epi64((__m128i *) (src1 + 8));
    src1_r2 = _mm_loadl_epi64((__m128i *) (src1 + 2 * 8));
    src1_r3 = _mm_loadl_epi64((__m128i *) (src1 + 3 * 8));

    src2_r0 = _mm_loadl_epi64((__m128i *) (src2));
    src2_r1 = _mm_loadl_epi64((__m128i *) (src2 + 8));
    src2_r2 = _mm_loadl_epi64((__m128i *) (src2 + 2 * 8));
    src2_r3 = _mm_loadl_epi64((__m128i *) (src2 + 3 * 8));

    src1_r0 = _mm_avg_epu8 (src1_r0, src2_r0);
    src1_r1 = _mm_avg_epu8 (src1_r1, src2_r1);
    src1_r2 = _mm_avg_epu8 (src1_r2, src2_r2);
    src1_r3 = _mm_avg_epu8 (src1_r3, src2_r3);

    _mm_storel_epi64((__m128i *) dst, src1_r0);
    _mm_storel_epi64((__m128i *) (dst + stride), src1_r1);
    _mm_storel_epi64((__m128i *) (dst + 2 * stride), src1_r2);
    _mm_storel_epi64((__m128i *) (dst + 3 * stride), src1_r3);

    /*******************************************************/
    /* interpolate V                                       */
    /*******************************************************/
    src1 = buf_src1->pu1_v;
    src2 = buf_src2->pu1_v;
    dst  = buf_dst->pu1_v;

    // Row 0-3
    src1_r0 = _mm_loadl_epi64((__m128i *) (src1));
    src1_r1 = _mm_loadl_epi64((__m128i *) (src1 + 8));
    src1_r2 = _mm_loadl_epi64((__m128i *) (src1 + 2 * 8));
    src1_r3 = _mm_loadl_epi64((__m128i *) (src1 + 3 * 8));

    src2_r0 = _mm_loadl_epi64((__m128i *) (src2));
    src2_r1 = _mm_loadl_epi64((__m128i *) (src2 + 8));
    src2_r2 = _mm_loadl_epi64((__m128i *) (src2 + 2 * 8));
    src2_r3 = _mm_loadl_epi64((__m128i *) (src2 + 3 * 8));

    src1_r0 = _mm_avg_epu8 (src1_r0, src2_r0);
    src1_r1 = _mm_avg_epu8 (src1_r1, src2_r1);
    src1_r2 = _mm_avg_epu8 (src1_r2, src2_r2);
    src1_r3 = _mm_avg_epu8 (src1_r3, src2_r3);

    _mm_storel_epi64((__m128i *) dst, src1_r0);
    _mm_storel_epi64((__m128i *) (dst + stride), src1_r1);
    _mm_storel_epi64((__m128i *) (dst + 2 * stride), src1_r2);
    _mm_storel_epi64((__m128i *) (dst + 3 * stride), src1_r3);

    // Row 4-7
    src1 += 4 * 8;
    src2 += 4 * 8;
    dst += 4 * stride;

    src1_r0 = _mm_loadl_epi64((__m128i *) (src1));
    src1_r1 = _mm_loadl_epi64((__m128i *) (src1 + 8));
    src1_r2 = _mm_loadl_epi64((__m128i *) (src1 + 2 * 8));
    src1_r3 = _mm_loadl_epi64((__m128i *) (src1 + 3 * 8));

    src2_r0 = _mm_loadl_epi64((__m128i *) (src2));
    src2_r1 = _mm_loadl_epi64((__m128i *) (src2 + 8));
    src2_r2 = _mm_loadl_epi64((__m128i *) (src2 + 2 * 8));
    src2_r3 = _mm_loadl_epi64((__m128i *) (src2 + 3 * 8));

    src1_r0 = _mm_avg_epu8 (src1_r0, src2_r0);
    src1_r1 = _mm_avg_epu8 (src1_r1, src2_r1);
    src1_r2 = _mm_avg_epu8 (src1_r2, src2_r2);
    src1_r3 = _mm_avg_epu8 (src1_r3, src2_r3);

    _mm_storel_epi64((__m128i *) dst, src1_r0);
    _mm_storel_epi64((__m128i *) (dst + stride), src1_r1);
    _mm_storel_epi64((__m128i *) (dst + 2 * stride), src1_r2);
    _mm_storel_epi64((__m128i *) (dst + 3 * stride), src1_r3);
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_mc_halfx_halfy_8x8_sse42()                                 */
/*                                                                           */
/*  Description   : Gets the buffer from (0.5,0.5) to (8.5,8.5)              */
/*                  and the above block of size 8 x 8 will be placed as a    */
/*                  block from the current position of out_buf               */
/*                                                                           */
/*  Inputs        : ref - Reference frame from which the block will be       */
/*                        block will be extracted.                           */
/*                  ref_wid - WIdth of reference frame                       */
/*                  out_wid - WIdth of the output frame                      */
/*                  blk_width  - width of the block                          */
/*                  blk_width  - height of the block                         */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Point to the (0,0),(1,0),(0,1),(1,1) position in         */
/*                  the ref frame.Interpolate these four values to get the   */
/*                  value at(0.5,0.5).Repeat this to get an 8 x 8 block      */
/*                  using 9 x 9 block from reference frame                   */
/*                                                                           */
/*  Outputs       : out -  Output containing the extracted block             */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*****************************************************************************/
void impeg2_mc_halfx_halfy_8x8_sse42(UWORD8 *out,
                            UWORD8 *ref,
                            UWORD32 ref_wid,
                            UWORD32 out_wid)
{
    UWORD8 *ref_p0,*ref_p1,*ref_p2,*ref_p3;
    /* P0-P3 are the pixels in the reference frame and Q is the value being */
    /* estimated                                                            */
    /*
       P0 P1
         Q
       P2 P3
    */
    __m128i src_r0, src_r0_1, src_r1, src_r1_1;
    __m128i tmp0, tmp1;
    __m128i value_2 = _mm_set1_epi16(2);

    ref_p0 = ref;
    ref_p1 = ref + 1;
    ref_p2 = ref + ref_wid;
    ref_p3 = ref + ref_wid + 1;

    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p0));     //Row 0
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p1));
    src_r1 = _mm_loadl_epi64((__m128i *) (ref_p2));     //Row 1
    src_r1_1 = _mm_loadl_epi64((__m128i *) (ref_p3));

    src_r0 =  _mm_cvtepu8_epi16(src_r0);
    src_r0_1 =  _mm_cvtepu8_epi16(src_r0_1);
    src_r1 =  _mm_cvtepu8_epi16(src_r1);
    src_r1_1 =  _mm_cvtepu8_epi16(src_r1_1);

    tmp0 = _mm_add_epi16(src_r0, src_r0_1);             //Row 0 horizontal interpolation
    tmp1 = _mm_add_epi16(src_r1, src_r1_1);             //Row 1 horizontal interpolation
    tmp0 = _mm_add_epi16(tmp0, tmp1);                   //Row 0 vertical interpolation
    tmp0 = _mm_add_epi16(tmp0, value_2);
    tmp0 =  _mm_srli_epi16(tmp0, 2);
    tmp0 = _mm_packus_epi16(tmp0, value_2);

    _mm_storel_epi64((__m128i *)out, tmp0);

    //Row 1
    ref_p2 += ref_wid;
    ref_p3 += ref_wid;
    out += out_wid;

    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p2));     //Row 2
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p3));

    src_r0 =  _mm_cvtepu8_epi16(src_r0);
    src_r0_1 =  _mm_cvtepu8_epi16(src_r0_1);

    tmp0 = _mm_add_epi16(src_r0, src_r0_1);         //Row 2 horizontal interpolation
    tmp1 = _mm_add_epi16(tmp0, tmp1);               //Row 1 vertical interpolation
    tmp1 = _mm_add_epi16(tmp1, value_2);
    tmp1 =  _mm_srli_epi16(tmp1, 2);
    tmp1 = _mm_packus_epi16(tmp1, value_2);

    _mm_storel_epi64((__m128i *)out, tmp1);

    //Row 2
    ref_p2 += ref_wid;
    ref_p3 += ref_wid;
    out += out_wid;

    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p2));     //Row 3
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p3));

    src_r0 =  _mm_cvtepu8_epi16(src_r0);
    src_r0_1 =  _mm_cvtepu8_epi16(src_r0_1);

    tmp1 = _mm_add_epi16(src_r0, src_r0_1);         //Row 3 horizontal interpolation

    tmp0 = _mm_add_epi16(tmp0, tmp1);               //Row 2 vertical interpolation
    tmp0 = _mm_add_epi16(tmp0, value_2);
    tmp0 =  _mm_srli_epi16(tmp0, 2);
    tmp0 = _mm_packus_epi16(tmp0, value_2);

    _mm_storel_epi64((__m128i *)out, tmp0);

    //Row 3
    ref_p2 += ref_wid;
    ref_p3 += ref_wid;
    out += out_wid;

    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p2));     //Row 4
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p3));

    src_r0 =  _mm_cvtepu8_epi16(src_r0);
    src_r0_1 =  _mm_cvtepu8_epi16(src_r0_1);

    tmp0 = _mm_add_epi16(src_r0, src_r0_1);         //Row 4 horizontal interpolation

    tmp1 = _mm_add_epi16(tmp0, tmp1);               //Row 3 vertical interpolation
    tmp1 = _mm_add_epi16(tmp1, value_2);
    tmp1 =  _mm_srli_epi16(tmp1, 2);
    tmp1 = _mm_packus_epi16(tmp1, value_2);

    _mm_storel_epi64((__m128i *)out, tmp1);

    //Row 4
    ref_p2 += ref_wid;
    ref_p3 += ref_wid;
    out += out_wid;

    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p2));     //Row 5
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p3));

    src_r0 =  _mm_cvtepu8_epi16(src_r0);
    src_r0_1 =  _mm_cvtepu8_epi16(src_r0_1);

    tmp1 = _mm_add_epi16(src_r0, src_r0_1);     //Row 5 horizontal interpolation

    tmp0 = _mm_add_epi16(tmp0, tmp1);           //Row 4 vertical interpolation
    tmp0 = _mm_add_epi16(tmp0, value_2);
    tmp0 =  _mm_srli_epi16(tmp0, 2);
    tmp0 = _mm_packus_epi16(tmp0, value_2);

    _mm_storel_epi64((__m128i *)out, tmp0);

    //Row 5
    ref_p2 += ref_wid;
    ref_p3 += ref_wid;
    out += out_wid;

    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p2));     //Row 6
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p3));

    src_r0 =  _mm_cvtepu8_epi16(src_r0);
    src_r0_1 =  _mm_cvtepu8_epi16(src_r0_1);

    tmp0 = _mm_add_epi16(src_r0, src_r0_1);             //Row 6 horizontal interpolation

    tmp1 = _mm_add_epi16(tmp0, tmp1);                   //Row 5 vertical interpolation
    tmp1 = _mm_add_epi16(tmp1, value_2);
    tmp1 =  _mm_srli_epi16(tmp1, 2);
    tmp1 = _mm_packus_epi16(tmp1, value_2);

    _mm_storel_epi64((__m128i *)out, tmp1);

    //Row 6
    ref_p2 += ref_wid;
    ref_p3 += ref_wid;
    out += out_wid;

    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p2));     //Row 7
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p3));

    src_r0 =  _mm_cvtepu8_epi16(src_r0);
    src_r0_1 =  _mm_cvtepu8_epi16(src_r0_1);

    tmp1 = _mm_add_epi16(src_r0, src_r0_1);             //Row 7 horizontal interpolation

    tmp0 = _mm_add_epi16(tmp0, tmp1);                   //Row 6 vertical interpolation
    tmp0 = _mm_add_epi16(tmp0, value_2);
    tmp0 =  _mm_srli_epi16(tmp0, 2);
    tmp0 = _mm_packus_epi16(tmp0, value_2);

    _mm_storel_epi64((__m128i *)out, tmp0);

    //Row 7
    ref_p2 += ref_wid;
    ref_p3 += ref_wid;
    out += out_wid;

    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p2));     //Row 8
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p3));

    src_r0 =  _mm_cvtepu8_epi16(src_r0);
    src_r0_1 =  _mm_cvtepu8_epi16(src_r0_1);

    tmp0 = _mm_add_epi16(src_r0, src_r0_1);             //Row 8 horizontal interpolation

    tmp1 = _mm_add_epi16(tmp0, tmp1);                   //Row 7 vertical interpolation
    tmp1 = _mm_add_epi16(tmp1, value_2);
    tmp1 =  _mm_srli_epi16(tmp1, 2);
    tmp1 = _mm_packus_epi16(tmp1, value_2);

    _mm_storel_epi64((__m128i *)out, tmp1);

    return;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_mc_halfx_fully_8x8_sse42()                                 */
/*                                                                           */
/*  Description   : Gets the buffer from (0.5,0) to (8.5,8)                  */
/*                  and the above block of size 8 x 8 will be placed as a    */
/*                  block from the current position of out_buf               */
/*                                                                           */
/*  Inputs        : ref - Reference frame from which the block will be       */
/*                        block will be extracted.                           */
/*                  ref_wid - WIdth of reference frame                       */
/*                  out_wid - WIdth of the output frame                      */
/*                  blk_width  - width of the block                          */
/*                  blk_width  - height of the block                         */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Point to the (0,0) and (1,0) position in the ref frame   */
/*                  Interpolate these two values to get the value at(0.5,0)  */
/*                  Repeat this to get an 8 x 8 block using 9 x 8 block from */
/*                  reference frame                                          */
/*                                                                           */
/*  Outputs       : out -  Output containing the extracted block             */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*****************************************************************************/
void impeg2_mc_halfx_fully_8x8_sse42(UWORD8 *out,
                            UWORD8 *ref,
                            UWORD32 ref_wid,
                            UWORD32 out_wid)
{
    UWORD8 *ref_p0,*ref_p1;
    __m128i src_r0, src_r0_1, src_r1, src_r1_1;
    /* P0-P3 are the pixels in the reference frame and Q is the value being */
    /* estimated                                                            */
    /*
       P0 Q P1
    */

    ref_p0 = ref;
    ref_p1 = ref + 1;

    // Row 0 and 1
    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p0));     //Row 0
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p1));
    src_r1 = _mm_loadl_epi64((__m128i *) (ref_p0 + ref_wid));       //Row 1
    src_r1_1 = _mm_loadl_epi64((__m128i *) (ref_p1 + ref_wid));

    src_r0 = _mm_avg_epu8(src_r0, src_r0_1);
    src_r1 = _mm_avg_epu8(src_r1, src_r1_1);

    _mm_storel_epi64((__m128i *)out, src_r0);
    _mm_storel_epi64((__m128i *)(out + out_wid), src_r1);

    // Row 2 and 3
    ref_p0 += 2*ref_wid;
    ref_p1 += 2*ref_wid;
    out += 2*out_wid;

    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p0));     //Row 2
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p1));
    src_r1 = _mm_loadl_epi64((__m128i *) (ref_p0 + ref_wid));       //Row 3
    src_r1_1 = _mm_loadl_epi64((__m128i *) (ref_p1 + ref_wid));

    src_r0 = _mm_avg_epu8(src_r0, src_r0_1);
    src_r1 = _mm_avg_epu8(src_r1, src_r1_1);

    _mm_storel_epi64((__m128i *)out, src_r0);
    _mm_storel_epi64((__m128i *)(out + out_wid), src_r1);

    // Row 4 and 5
    ref_p0 += 2*ref_wid;
    ref_p1 += 2*ref_wid;
    out += 2*out_wid;

    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p0));     //Row 4
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p1));
    src_r1 = _mm_loadl_epi64((__m128i *) (ref_p0 + ref_wid));       //Row 5
    src_r1_1 = _mm_loadl_epi64((__m128i *) (ref_p1 + ref_wid));

    src_r0 = _mm_avg_epu8(src_r0, src_r0_1);
    src_r1 = _mm_avg_epu8(src_r1, src_r1_1);

    _mm_storel_epi64((__m128i *)out, src_r0);
    _mm_storel_epi64((__m128i *)(out + out_wid), src_r1);

    // Row 6 and 7
    ref_p0 += 2*ref_wid;
    ref_p1 += 2*ref_wid;
    out += 2*out_wid;

    src_r0 = _mm_loadl_epi64((__m128i *) (ref_p0));     //Row 6
    src_r0_1 = _mm_loadl_epi64((__m128i *) (ref_p1));
    src_r1 = _mm_loadl_epi64((__m128i *) (ref_p0 + ref_wid));       //Row 7
    src_r1_1 = _mm_loadl_epi64((__m128i *) (ref_p1 + ref_wid));

    src_r0 = _mm_avg_epu8(src_r0, src_r0_1);
    src_r1 = _mm_avg_epu8(src_r1, src_r1_1);

    _mm_storel_epi64((__m128i *)out, src_r0);
    _mm_storel_epi64((__m128i *)(out + out_wid), src_r1);

    return;
}


/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_mc_fullx_halfy_8x8_sse42()                                 */
/*                                                                           */
/*  Description   : Gets the buffer from (0,0.5) to (8,8.5)                  */
/*                  and the above block of size 8 x 8 will be placed as a    */
/*                  block from the current position of out_buf               */
/*                                                                           */
/*  Inputs        : ref - Reference frame from which the block will be       */
/*                        block will be extracted.                           */
/*                  ref_wid - WIdth of reference frame                       */
/*                  out_wid - WIdth of the output frame                      */
/*                  blk_width  - width of the block                          */
/*                  blk_width  - height of the block                         */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Point to the (0,0) and (0,1)   position in the ref frame */
/*                  Interpolate these two values to get the value at(0,0.5)  */
/*                  Repeat this to get an 8 x 8 block using 8 x 9 block from */
/*                  reference frame                                          */
/*                                                                           */
/*  Outputs       : out -  Output containing the extracted block             */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*****************************************************************************/
void impeg2_mc_fullx_halfy_8x8_sse42(UWORD8 *out,
                            UWORD8 *ref,
                            UWORD32 ref_wid,
                            UWORD32 out_wid)
{
    __m128i src_r0, src_r1, src_r2, temp0, temp1;
    /* P0-P3 are the pixels in the reference frame and Q is the value being */
    /* estimated                                                            */
    /*
       P0
        x
       P1
    */
    src_r0 = _mm_loadl_epi64((__m128i *)ref);               //Row 0
    src_r1 = _mm_loadl_epi64((__m128i *)(ref + ref_wid));   //Row 1
    src_r2 = _mm_loadl_epi64((__m128i *)(ref + 2 * ref_wid));   //Row 2
    temp0 = _mm_avg_epu8(src_r0, src_r1);
    temp1 = _mm_avg_epu8(src_r1, src_r2);
    _mm_storel_epi64((__m128i *)out, temp0);                //Row 0
    _mm_storel_epi64((__m128i *)(out + out_wid), temp1);    //Row 1

    ref+= 3*ref_wid;
    out+= 2*out_wid;

    src_r0 = _mm_loadl_epi64((__m128i *)ref);               //Row 3
    src_r1 = _mm_loadl_epi64((__m128i *)(ref + ref_wid));   //Row 4
    temp0 = _mm_avg_epu8(src_r2, src_r0);
    temp1 = _mm_avg_epu8(src_r0, src_r1);
    _mm_storel_epi64((__m128i *)out, temp0);                //Row 2
    _mm_storel_epi64((__m128i *)(out + out_wid), temp1);    //Row 3

    ref += 2*ref_wid;
    out+= 2*out_wid;

    src_r2 = _mm_loadl_epi64((__m128i *)ref);               //Row 5
    src_r0 = _mm_loadl_epi64((__m128i *)(ref + ref_wid));   //Row 6
    temp0 = _mm_avg_epu8(src_r1, src_r2);
    temp1 = _mm_avg_epu8(src_r2, src_r0);
    _mm_storel_epi64((__m128i *)out, temp0);                //Row 4
    _mm_storel_epi64((__m128i *)(out + out_wid), temp1);    //Row 5

    ref += 2*ref_wid;
    out+= 2*out_wid;

    src_r1 = _mm_loadl_epi64((__m128i *)ref);               //Row 7
    src_r2 = _mm_loadl_epi64((__m128i *) (ref + ref_wid));  //Row 8
    temp0 = _mm_avg_epu8(src_r0, src_r1);
    temp1 = _mm_avg_epu8(src_r1, src_r2);
    _mm_storel_epi64((__m128i *)out, temp0);                //Row 6
    _mm_storel_epi64((__m128i *)(out + out_wid), temp1);    //Row 7

    return;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_mc_fullx_fully_8x8_sse42()                                 */
/*                                                                           */
/*  Description   : Gets the buffer from (x,y) to (x+8,y+8)                  */
/*                  and the above block of size 8 x 8 will be placed as a    */
/*                  block from the current position of out_buf               */
/*                                                                           */
/*  Inputs        : ref - Reference frame from which the block will be       */
/*                        block will be extracted.                           */
/*                  ref_wid - WIdth of reference frame                       */
/*                  out_wid - WIdth of the output frame                      */
/*                  blk_width  - width of the block                          */
/*                  blk_width  - height of the block                         */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Point to the (0,0) position in the ref frame             */
/*                  Get an 8 x 8 block from reference frame                  */
/*                                                                           */
/*  Outputs       : out -  Output containing the extracted block             */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*****************************************************************************/
void impeg2_mc_fullx_fully_8x8_sse42(UWORD8 *out,
                            UWORD8 *ref,
                            UWORD32 ref_wid,
                            UWORD32 out_wid)
{
    __m128i src_r0, src_r1, src_r2, src_r3;
    // Row 0-3
    src_r0 =  _mm_loadl_epi64((__m128i *)ref);
    src_r1 =  _mm_loadl_epi64((__m128i *)(ref + ref_wid));
    src_r2 =  _mm_loadl_epi64((__m128i *)(ref + 2 * ref_wid));
    src_r3 =  _mm_loadl_epi64((__m128i *)(ref + 3 * ref_wid));

    _mm_storel_epi64((__m128i *)out, src_r0);
    _mm_storel_epi64((__m128i *)(out + out_wid), src_r1);
    _mm_storel_epi64((__m128i *)(out + 2 * out_wid), src_r2);
    _mm_storel_epi64((__m128i *)(out + 3 * out_wid), src_r3);

    // Row 4-7
    ref += 4 * ref_wid;
    out += 4 * out_wid;

    src_r0 =  _mm_loadl_epi64((__m128i *)ref);
    src_r1 =  _mm_loadl_epi64((__m128i *)(ref + ref_wid));
    src_r2 =  _mm_loadl_epi64((__m128i *)(ref + 2 * ref_wid));
    src_r3 =  _mm_loadl_epi64((__m128i *)(ref + 3 * ref_wid));

    _mm_storel_epi64((__m128i *)out, src_r0);
    _mm_storel_epi64((__m128i *)(out + out_wid), src_r1);
    _mm_storel_epi64((__m128i *)(out + 2 * out_wid), src_r2);
    _mm_storel_epi64((__m128i *)(out + 3 * out_wid), src_r3);
    return;
}
