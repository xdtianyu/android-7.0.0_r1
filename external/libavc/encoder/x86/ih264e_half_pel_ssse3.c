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
 *  ih264e_half_pel_ssse3.c
 *
 * @brief
 *  Contains the x86 intrinsic function definitions for 6-tap vertical filter
 *  and cascaded 2D filter used in motion estimation in H264 encoder.
 *
 * @author
 *  Ittiam
 *
 * @par List of Functions:
 *  ih264e_sixtapfilter_horz_ssse3
 *  ih264e_sixtap_filter_2dvh_vert_ssse3
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* System include files */
#include <stdio.h>
#include <assert.h>
#include <limits.h>

/* User include files */
#include "ih264_typedefs.h"
#include "ithread.h"
#include "ih264_platform_macros.h"
#include "ih264_defs.h"
#include "ih264e_half_pel.h"
#include "ih264_macros.h"
#include "ih264e_debug.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
/*
*******************************************************************************
*
* @brief
*  Interprediction luma filter for horizontal input(Filter run for width = 17
*  and height =16)
*
* @par Description:
*  Applies a 6 tap horizontal filter .The output is  clipped to 8 bits sec.
*  8.4.2.2.1 titled "Luma sample interpolation process"
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[out] pu1_dst
*  UWORD8 pointer to the destination
*
* @param[in] src_strd
*  integer source stride
*
* @param[in] dst_strd
*  integer destination stride
*
* @returns
*  None
*
* @remarks
*  None
*
*******************************************************************************
*/
void ih264e_sixtapfilter_horz_ssse3(UWORD8 *pu1_src,
                                    UWORD8 *pu1_dst,
                                    WORD32 src_strd,
                                    WORD32 dst_strd)
{
    WORD32 ht;
    WORD32 tmp;

    __m128i src_r0_16x8b, src_r1_16x8b, src_r0_sht_16x8b, src_r1_sht_16x8b;
    __m128i src_r0_t1_16x8b, src_r1_t1_16x8b;

    __m128i res_r0_t1_8x16b, res_r0_t2_8x16b, res_r0_t3_8x16b;
    __m128i res_r1_t1_8x16b, res_r1_t2_8x16b, res_r1_t3_8x16b;

    __m128i coeff0_1_16x8b, coeff2_3_16x8b, coeff4_5_16x8b;
    __m128i const_val16_8x16b;

    ht = 16;
    pu1_src -= 2; // the filter input starts from x[-2] (till x[3])

    coeff0_1_16x8b = _mm_set1_epi32(0xFB01FB01); //c0 c1 c0 c1 c0 c1 c0 c1 c0 c1 c0 c1 c0 c1 c0 c1
    coeff2_3_16x8b = _mm_set1_epi32(0x14141414); //c2 c3 c2 c3 c2 c3 c2 c3 c2 c3 c2 c3 c2 c3 c2 c3
    coeff4_5_16x8b = _mm_set1_epi32(0x01FB01FB); //c4 c5 c4 c5 c4 c5 c4 c5 c4 c5 c4 c5 c4 c5 c4 c5
                                                 //c0 = c5 = 1, c1 = c4 = -5, c2 = c3 = 20
    const_val16_8x16b = _mm_set1_epi16(16);

    //Row0 : a0 a1 a2 a3 a4 a5 a6 a7 a8 a9.....
    //Row0 :                         b0 b1 b2 b3 b4 b5 b6 b7 b8 b9.....
    //b0 is same a8. Similarly other bn pixels are same as a(n+8) pixels.

    do
    {
        src_r0_16x8b = _mm_loadu_si128((__m128i *)pu1_src);                     //a0 a1 a2 a3 a4 a5 a6 a7 a8 a9....a15
        src_r1_16x8b = _mm_loadu_si128((__m128i *)(pu1_src + 8));               //b0 b1 b2 b3 b4 b5 b6 b7 b8 b9....b15

        src_r0_sht_16x8b = _mm_srli_si128(src_r0_16x8b, 1);                      //a1 a2 a3 a4 a5 a6 a7 a8 a9....a15 0
        src_r1_sht_16x8b = _mm_srli_si128(src_r1_16x8b, 1);                      //b1 b2 b3 b4 b5 b6 b7 b8 b9....b15 0

        src_r0_t1_16x8b = _mm_unpacklo_epi8(src_r0_16x8b, src_r0_sht_16x8b);     //a0 a1 a1 a2 a2 a3 a3 a4 a4 a5 a5 a6 a6 a7 a7 a8
        src_r1_t1_16x8b = _mm_unpacklo_epi8(src_r1_16x8b, src_r1_sht_16x8b);     //b0 b1 b1 b2 b2 b3 b3 b4 b4 b5 b5 b6 b6 b7 b7 b8

        res_r0_t1_8x16b = _mm_maddubs_epi16(src_r0_t1_16x8b, coeff0_1_16x8b);    //a0*c0+a1*c1 a1*c0+a2*c1 a2*c0+a3*c1 a3*c0+a4*c1
                                                                                 //a4*c0+a5*c1 a5*c0+a6*c1 a6*c0+a7*c1 a7*c0+a8*c1
        res_r1_t1_8x16b = _mm_maddubs_epi16(src_r1_t1_16x8b, coeff0_1_16x8b);    //b0*c0+b1*c1 b1*c0+b2*c1 b2*c0+b3*c1 b3*c0+b4*c1
                                                                                 //b4*c0+b5*c1 b5*c0+b6*c1 b6*c0+b7*c1 b7*c0+b8*c1

        src_r0_16x8b = _mm_srli_si128(src_r0_16x8b, 2);                          //a2 a3 a4 a5 a6 a7 a8 a9....a15 0 0
        src_r1_16x8b = _mm_srli_si128(src_r1_16x8b, 2);                          //b2 b3 b4 b5 b6 b7 b8 b9....b15 0 0

        src_r0_sht_16x8b = _mm_srli_si128(src_r0_sht_16x8b, 2);                  //a3 a4 a5 a6 a7 a8 a9....a15 0  0  0
        src_r1_sht_16x8b = _mm_srli_si128(src_r1_sht_16x8b, 2);                  //b3 b4 b5 b6 b7 b8 b9....b15 0  0  0

        src_r0_t1_16x8b = _mm_unpacklo_epi8(src_r0_16x8b, src_r0_sht_16x8b);     //a2 a3 a3 a4 a4 a5 a5 a6 a6 a7 a7 a8 a8 a9 a9 a10
        src_r1_t1_16x8b = _mm_unpacklo_epi8(src_r1_16x8b, src_r1_sht_16x8b);     //b2 b3 b3 b4 b4 b5 b5 b6 b6 b7 b7 b8 a8 a9 a9 a10

        res_r0_t2_8x16b = _mm_maddubs_epi16(src_r0_t1_16x8b, coeff2_3_16x8b);    //a2*c2+a3*c3 a3*c2+a4*c3 a4*c2+a5*c3 a5*c2+a6*c3
                                                                                 //a6*c2+a7*c3 a7*c2+a8*c3 a8*c2+a9*c3 a9*c2+a10*c3
        res_r1_t2_8x16b = _mm_maddubs_epi16(src_r1_t1_16x8b, coeff2_3_16x8b);    //b2*c2+b3*c3 b3*c2+b4*c3 b2*c4+b5*c3 b5*c2+b6*c3
                                                                                 //b6*c2+b7*c3 b7*c2+b8*c3 b8*c2+b9*c3 b9*c2+b10*c3

        src_r0_16x8b = _mm_srli_si128(src_r0_16x8b, 2);                          //a4 a5 a6 a7 a8 a9....a15 0  0  0  0
        src_r1_16x8b = _mm_srli_si128(src_r1_16x8b, 2);                          //b4 b5 b6 b7 b8 b9....b15 0  0  0  0

        src_r0_sht_16x8b = _mm_srli_si128(src_r0_sht_16x8b, 2);                  //a5 a6 a7 a8 a9....a15 0  0  0  0  0
        src_r1_sht_16x8b = _mm_srli_si128(src_r1_sht_16x8b, 2);                  //b5 b6 b7 b8 b9....b15 0  0  0  0  0

        src_r0_t1_16x8b = _mm_unpacklo_epi8(src_r0_16x8b, src_r0_sht_16x8b);     //a4 a5 a5 a6 a6 a7 a7 a8 a8 a9 a9 a10 a10 a11 a11 a12
        src_r1_t1_16x8b = _mm_unpacklo_epi8(src_r1_16x8b, src_r1_sht_16x8b);     //b4 b5 b5 b6 b6 b7 b7 b8 b8 b9 b9 b10 b10 b11 b11 b12

        res_r0_t3_8x16b = _mm_maddubs_epi16(src_r0_t1_16x8b, coeff4_5_16x8b);    //a4*c4+a5*c5 a5*c4+a6*c5  a6*c4+a7*c5   a7*c4+a8*c5
                                                                                 //a8*c4+a9*c5 a9*c4+a10*c5 a10*c4+a11*c5 a11*c4+a12*c5
        res_r1_t3_8x16b = _mm_maddubs_epi16(src_r1_t1_16x8b, coeff4_5_16x8b);    //b4*c4+b5*c5 b5*c4+b6*c5  b6*c4+b7*c5   b7*c4+b8*c5
                                                                                 //b8*c4+b9*c5 b9*c4+b10*c5 b10*c4+b11*c5 b11*c4+b12*c5
        res_r0_t1_8x16b = _mm_add_epi16(res_r0_t1_8x16b, res_r0_t2_8x16b);
        res_r1_t1_8x16b = _mm_add_epi16(res_r1_t1_8x16b, res_r1_t2_8x16b);
        res_r0_t3_8x16b = _mm_add_epi16(res_r0_t3_8x16b, const_val16_8x16b);
        res_r1_t3_8x16b = _mm_add_epi16(res_r1_t3_8x16b, const_val16_8x16b);
        res_r0_t1_8x16b = _mm_add_epi16(res_r0_t1_8x16b, res_r0_t3_8x16b);
        res_r1_t1_8x16b = _mm_add_epi16(res_r1_t1_8x16b, res_r1_t3_8x16b);

        tmp = ((pu1_src[18] + pu1_src[19]) << 2) - pu1_src[17] - pu1_src[20];
        tmp = pu1_src[16] + pu1_src[21] + (tmp << 2) + tmp;

        res_r0_t1_8x16b = _mm_srai_epi16(res_r0_t1_8x16b, 5);                    //shifting right by 5 bits.
        res_r1_t1_8x16b = _mm_srai_epi16(res_r1_t1_8x16b, 5);
        tmp = (tmp + 16) >> 5;

        src_r0_16x8b = _mm_packus_epi16(res_r0_t1_8x16b, res_r1_t1_8x16b);
        pu1_dst[16] = CLIP_U8(tmp);

        _mm_storeu_si128((__m128i *)pu1_dst, src_r0_16x8b);

        ht--;
        pu1_src += src_strd;
        pu1_dst += dst_strd;
    }
    while(ht > 0);
}

/*
*******************************************************************************
*
* @brief
*   This function implements a two stage cascaded six tap filter. It
*    applies the six tap filter in the vertical direction on the
*    predictor values, followed by applying the same filter in the
*    horizontal direction on the output of the first stage. The six tap
*    filtering operation is described in sec 8.4.2.2.1 titled "Luma sample
*    interpolation process" (Filter run for width = 17 and height =17)
*
* @par Description:
*    The function interpolates the predictors first in the vertical direction
*    and then in the horizontal direction to output the (1/2,1/2). The output
*    of the first stage of the filter is stored in the buffer pointed to by
*    pi16_pred1(only in C) in 16 bit precision.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[out] pu1_dst1
*  UWORD8 pointer to the destination(Vertical filtered output)
*
* @param[out] pu1_dst2
*  UWORD8 pointer to the destination(out put after applying horizontal filter
*  to the intermediate vertical output)
*
* @param[in] src_strd
*  integer source stride

* @param[in] dst_strd
*  integer destination stride of pu1_dst
*
* @param[in]pi16_pred1
*  Pointer to 16bit intermediate buffer(used only in c)
*
* @param[in] pi16_pred1_strd
*  integer destination stride of pi16_pred1
*
* @returns
*  None
*
* @remarks
*  None
*
*******************************************************************************
*/
void ih264e_sixtap_filter_2dvh_vert_ssse3(UWORD8 *pu1_src,
                                          UWORD8 *pu1_dst1,
                                          UWORD8 *pu1_dst2,
                                          WORD32 src_strd,
                                          WORD32 dst_strd,
                                          WORD32 *pi4_pred1,
                                          WORD32 pred1_strd)
{
    WORD32 ht;
    WORD16 *pi2_pred1;

    ht = 17;
    pi2_pred1 = (WORD16 *)pi4_pred1;
    pred1_strd = pred1_strd << 1;

    // Vertical 6-tap filter
    {
        __m128i src1_r0_16x8b, src1_r1_16x8b, src1_r2_16x8b;
        __m128i src1_r3_16x8b, src1_r4_16x8b, src1_r5_16x8b;
        __m128i src2_r0_16x8b, src2_r1_16x8b, src2_r2_16x8b;
        __m128i src2_r3_16x8b, src2_r4_16x8b, src2_r5_16x8b;

        __m128i src_r0r1_16x8b, src_r2r3_16x8b, src_r4r5_16x8b;

        __m128i res_t1_8x16b, res_t2_8x16b, res_t3_8x16b;
        __m128i coeff0_1_16x8b, coeff2_3_16x8b, coeff4_5_16x8b;

        coeff0_1_16x8b = _mm_set1_epi32(0xFB01FB01); //c0 c1 c0 c1 c0 c1 c0 c1 c0 c1 c0 c1 c0 c1 c0 c1
        coeff2_3_16x8b = _mm_set1_epi32(0x14141414); //c2 c3 c2 c3 c2 c3 c2 c3 c2 c3 c2 c3 c2 c3 c2 c3
        coeff4_5_16x8b = _mm_set1_epi32(0x01FB01FB); //c4 c5 c4 c5 c4 c5 c4 c5 c4 c5 c4 c5 c4 c5 c4 c5
                                                     //c0 = c5 = 1, c1 = c4 = -5, c2 = c3 = 20

        pu1_src -= 2;
        pu1_src -= src_strd << 1; // the filter input starts from x[-2] (till x[3])

        // Loading first five rows to start first row processing.
        // 22 values loaded in each row.
        src1_r0_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        src2_r0_16x8b = _mm_loadl_epi64((__m128i *)(pu1_src + 14));
        pu1_src += src_strd;

        src1_r1_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        src2_r1_16x8b = _mm_loadl_epi64((__m128i *)(pu1_src + 14));
        pu1_src += src_strd;

        src1_r2_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        src2_r2_16x8b = _mm_loadl_epi64((__m128i *)(pu1_src + 14));
        pu1_src += src_strd;

        src1_r3_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        src2_r3_16x8b = _mm_loadl_epi64((__m128i *)(pu1_src + 14));
        pu1_src += src_strd;

        src1_r4_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        src2_r4_16x8b = _mm_loadl_epi64((__m128i *)(pu1_src + 14));
        pu1_src += src_strd;

        do
        {
            src1_r5_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
            src2_r5_16x8b = _mm_loadl_epi64((__m128i *)(pu1_src + 14));

            src_r0r1_16x8b = _mm_unpacklo_epi8(src1_r0_16x8b, src1_r1_16x8b);
            src_r2r3_16x8b = _mm_unpacklo_epi8(src1_r2_16x8b, src1_r3_16x8b);
            src_r4r5_16x8b = _mm_unpacklo_epi8(src1_r4_16x8b, src1_r5_16x8b);

            res_t1_8x16b = _mm_maddubs_epi16(src_r0r1_16x8b, coeff0_1_16x8b);
            res_t2_8x16b = _mm_maddubs_epi16(src_r2r3_16x8b, coeff2_3_16x8b);
            res_t3_8x16b = _mm_maddubs_epi16(src_r4r5_16x8b, coeff4_5_16x8b);

            res_t1_8x16b = _mm_add_epi16(res_t1_8x16b, res_t2_8x16b);
            res_t1_8x16b = _mm_add_epi16(res_t3_8x16b, res_t1_8x16b);

            _mm_storeu_si128((__m128i *)pi2_pred1, res_t1_8x16b);

            src_r0r1_16x8b = _mm_unpackhi_epi8(src1_r0_16x8b, src1_r1_16x8b);
            src_r2r3_16x8b = _mm_unpackhi_epi8(src1_r2_16x8b, src1_r3_16x8b);
            src_r4r5_16x8b = _mm_unpackhi_epi8(src1_r4_16x8b, src1_r5_16x8b);

            res_t1_8x16b = _mm_maddubs_epi16(src_r0r1_16x8b, coeff0_1_16x8b);
            res_t2_8x16b = _mm_maddubs_epi16(src_r2r3_16x8b, coeff2_3_16x8b);
            res_t3_8x16b = _mm_maddubs_epi16(src_r4r5_16x8b, coeff4_5_16x8b);

            res_t1_8x16b = _mm_add_epi16(res_t1_8x16b, res_t2_8x16b);
            res_t1_8x16b = _mm_add_epi16(res_t3_8x16b, res_t1_8x16b);

            _mm_storeu_si128((__m128i *)(pi2_pred1 + 8), res_t1_8x16b);

            src_r0r1_16x8b = _mm_unpacklo_epi8(src2_r0_16x8b, src2_r1_16x8b);
            src_r2r3_16x8b = _mm_unpacklo_epi8(src2_r2_16x8b, src2_r3_16x8b);
            src_r4r5_16x8b = _mm_unpacklo_epi8(src2_r4_16x8b, src2_r5_16x8b);

            res_t1_8x16b = _mm_maddubs_epi16(src_r0r1_16x8b, coeff0_1_16x8b);
            res_t2_8x16b = _mm_maddubs_epi16(src_r2r3_16x8b, coeff2_3_16x8b);
            res_t3_8x16b = _mm_maddubs_epi16(src_r4r5_16x8b, coeff4_5_16x8b);

            res_t1_8x16b = _mm_add_epi16(res_t1_8x16b, res_t2_8x16b);
            res_t1_8x16b = _mm_add_epi16(res_t3_8x16b, res_t1_8x16b);

            _mm_storeu_si128((__m128i *)(pi2_pred1 + 14), res_t1_8x16b);

            src1_r0_16x8b = src1_r1_16x8b;
            src1_r1_16x8b = src1_r2_16x8b;
            src1_r2_16x8b = src1_r3_16x8b;
            src1_r3_16x8b = src1_r4_16x8b;
            src1_r4_16x8b = src1_r5_16x8b;

            src2_r0_16x8b = src2_r1_16x8b;
            src2_r1_16x8b = src2_r2_16x8b;
            src2_r2_16x8b = src2_r3_16x8b;
            src2_r3_16x8b = src2_r4_16x8b;
            src2_r4_16x8b = src2_r5_16x8b;

            ht--;
            pu1_src += src_strd;
            pi2_pred1 += pred1_strd;
        }
        while(ht > 0);
    }

    ht = 17;
    pi2_pred1 = (WORD16 *)pi4_pred1;

    // Horizontal 6-tap filter
    {
        WORD32 temp;

        __m128i src_r0_8x16b, src_r1_8x16b, src_r2_8x16b, src_r3_8x16b;
        __m128i src_r4_8x16b, src_r5_8x16b;
        __m128i src_r0r1_8x16b, src_r2r3_8x16b, src_r4r5_8x16b;
        __m128i res_vert1_8x16b, res_vert2_8x16b, res_16x8b;

        __m128i res_t0_4x32b, res_t1_4x32b, res_t2_4x32b, res_t3_4x32b;
        __m128i res_c0_8x16b, res_c1_8x16b;

        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b;
        __m128i const_val512_4x32b, const_val16_8x16b;

        coeff0_1_8x16b = _mm_set1_epi32(0xFFFB0001); //c0 c1 c0 c1 c0 c1 c0 c1
        coeff2_3_8x16b = _mm_set1_epi32(0x00140014); //c2 c3 c2 c3 c2 c3 c2 c3
        coeff4_5_8x16b = _mm_set1_epi32(0x0001FFFB); //c4 c5 c4 c5 c4 c5 c4 c5
                                                     //c0 = c5 = 1, c1 = c4 = -5, c2 = c3 = 20
        const_val512_4x32b = _mm_set1_epi32(512);
        const_val16_8x16b = _mm_set1_epi16(16);

        do
        {
            src_r0_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1));
            src_r1_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1 + 1));
            src_r2_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1 + 2));
            src_r3_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1 + 3));
            src_r4_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1 + 4));
            src_r5_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1 + 5));

            res_vert1_8x16b = _mm_add_epi16(src_r2_8x16b, const_val16_8x16b);
            res_vert1_8x16b = _mm_srai_epi16(res_vert1_8x16b, 5); //shifting right by 5 bits.

            src_r0r1_8x16b = _mm_unpacklo_epi16(src_r0_8x16b, src_r1_8x16b);
            src_r2r3_8x16b = _mm_unpacklo_epi16(src_r2_8x16b, src_r3_8x16b);
            src_r4r5_8x16b = _mm_unpacklo_epi16(src_r4_8x16b, src_r5_8x16b);

            res_t1_4x32b = _mm_madd_epi16(src_r0r1_8x16b, coeff0_1_8x16b);
            res_t2_4x32b = _mm_madd_epi16(src_r2r3_8x16b, coeff2_3_8x16b);
            res_t3_4x32b = _mm_madd_epi16(src_r4r5_8x16b, coeff4_5_8x16b);

            res_t1_4x32b = _mm_add_epi32(res_t1_4x32b, res_t2_4x32b);
            res_t3_4x32b = _mm_add_epi32(res_t3_4x32b, const_val512_4x32b);
            res_t1_4x32b = _mm_add_epi32(res_t1_4x32b, res_t3_4x32b);
            res_t0_4x32b = _mm_srai_epi32(res_t1_4x32b, 10);

            src_r0r1_8x16b = _mm_unpackhi_epi16(src_r0_8x16b, src_r1_8x16b);
            src_r2r3_8x16b = _mm_unpackhi_epi16(src_r2_8x16b, src_r3_8x16b);
            src_r4r5_8x16b = _mm_unpackhi_epi16(src_r4_8x16b, src_r5_8x16b);

            res_t1_4x32b = _mm_madd_epi16(src_r0r1_8x16b, coeff0_1_8x16b);
            res_t2_4x32b = _mm_madd_epi16(src_r2r3_8x16b, coeff2_3_8x16b);
            res_t3_4x32b = _mm_madd_epi16(src_r4r5_8x16b, coeff4_5_8x16b);

            res_t1_4x32b = _mm_add_epi32(res_t1_4x32b, res_t2_4x32b);
            res_t3_4x32b = _mm_add_epi32(res_t3_4x32b, const_val512_4x32b);
            res_t1_4x32b = _mm_add_epi32(res_t1_4x32b, res_t3_4x32b);
            res_t1_4x32b = _mm_srai_epi32(res_t1_4x32b, 10);

            res_c0_8x16b = _mm_packs_epi32(res_t0_4x32b, res_t1_4x32b);

            src_r0_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1 + 8));
            src_r1_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1 + 8 + 1));
            src_r2_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1 + 8 + 2));
            src_r3_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1 + 8 + 3));
            src_r4_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1 + 8 + 4));
            src_r5_8x16b = _mm_loadu_si128((__m128i *)(pi2_pred1 + 8 + 5));

            res_vert2_8x16b = _mm_add_epi16(src_r2_8x16b, const_val16_8x16b);
            res_vert2_8x16b = _mm_srai_epi16(res_vert2_8x16b, 5); //shifting right by 5 bits.

            src_r0r1_8x16b = _mm_unpacklo_epi16(src_r0_8x16b, src_r1_8x16b);
            src_r2r3_8x16b = _mm_unpacklo_epi16(src_r2_8x16b, src_r3_8x16b);
            src_r4r5_8x16b = _mm_unpacklo_epi16(src_r4_8x16b, src_r5_8x16b);

            res_t1_4x32b = _mm_madd_epi16(src_r0r1_8x16b, coeff0_1_8x16b);
            res_t2_4x32b = _mm_madd_epi16(src_r2r3_8x16b, coeff2_3_8x16b);
            res_t3_4x32b = _mm_madd_epi16(src_r4r5_8x16b, coeff4_5_8x16b);

            res_t1_4x32b = _mm_add_epi32(res_t1_4x32b, res_t2_4x32b);
            res_t3_4x32b = _mm_add_epi32(res_t3_4x32b, const_val512_4x32b);
            res_t1_4x32b = _mm_add_epi32(res_t1_4x32b, res_t3_4x32b);
            res_t0_4x32b = _mm_srai_epi32(res_t1_4x32b ,10);

            src_r0r1_8x16b = _mm_unpackhi_epi16(src_r0_8x16b, src_r1_8x16b);
            src_r2r3_8x16b = _mm_unpackhi_epi16(src_r2_8x16b, src_r3_8x16b);
            src_r4r5_8x16b = _mm_unpackhi_epi16(src_r4_8x16b, src_r5_8x16b);

            res_t1_4x32b = _mm_madd_epi16(src_r0r1_8x16b, coeff0_1_8x16b);
            res_t2_4x32b = _mm_madd_epi16(src_r2r3_8x16b, coeff2_3_8x16b);
            res_t3_4x32b = _mm_madd_epi16(src_r4r5_8x16b, coeff4_5_8x16b);

            res_t1_4x32b = _mm_add_epi32(res_t1_4x32b, res_t2_4x32b);
            res_t3_4x32b = _mm_add_epi32(res_t3_4x32b, const_val512_4x32b);
            res_t1_4x32b = _mm_add_epi32(res_t1_4x32b, res_t3_4x32b);
            res_t1_4x32b = _mm_srai_epi32(res_t1_4x32b, 10);

            res_c1_8x16b = _mm_packs_epi32(res_t0_4x32b, res_t1_4x32b);

            res_16x8b = _mm_packus_epi16(res_vert1_8x16b, res_vert2_8x16b);
            _mm_storeu_si128((__m128i *)pu1_dst1, res_16x8b);
            pu1_dst1[16] = CLIP_U8((pi2_pred1[18] + 16) >> 5);

            res_16x8b = _mm_packus_epi16(res_c0_8x16b, res_c1_8x16b);
            _mm_storeu_si128((__m128i *)pu1_dst2, res_16x8b);
            temp = ((pi2_pred1[18] + pi2_pred1[19]) << 2) - pi2_pred1[17] - pi2_pred1[20];
            temp = pi2_pred1[16] + pi2_pred1[21] + (temp << 2) + temp;
            pu1_dst2[16] = CLIP_U8((temp + 512) >> 10);

            ht--;
            pi2_pred1 += pred1_strd;
            pu1_dst1 += dst_strd;
            pu1_dst2 += dst_strd;
        }
        while(ht > 0);
    }
}
