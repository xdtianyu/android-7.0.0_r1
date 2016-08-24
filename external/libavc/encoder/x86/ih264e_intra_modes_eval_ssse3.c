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
*  ih264e_intra_modes_eval_ssse3.c
*
* @brief
*   This file contains definitions of routines that perform rate distortion
*  analysis on a macroblock if they are to be coded as intra.
*
* @author
*  Ittiam
*
* @par List of Functions:
*  ih264e_evaluate_intra16x16_modes_ssse3
*  ih264e_evaluate_intra_4x4_modes_ssse3
*  ih264e_evaluate_intra_chroma_modes_ssse3
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
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <immintrin.h>

/* User include files */
#include "ih264e_config.h"
#include "ih264_typedefs.h"
#include "ih264e_defs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264_debug.h"
#include "ih264_defs.h"
#include "ih264_macros.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_structs.h"
#include "ih264_common_tables.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_deblk_edge_filters.h"
#include "ime_distortion_metrics.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "ih264_cabac_tables.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"

#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_cabac.h"
#include "ih264e_intra_modes_eval.h"
#include "ih264e_globals.h"
#include "ime_platform_macros.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
/**
******************************************************************************
*
* @brief
*  evaluate best intra 16x16 mode (among VERT, HORZ and DC) and do the
*  prediction.
*
* @par Description
*  This function evaluates first three 16x16 modes and compute corresponding
*  SAD and returns the buffer predicted with best mode.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[in] pu1_ngbr_pels_i16
*  UWORD8 pointer to neighbouring pels
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
* @param[in] u4_n_avblty
*  availability of neighbouring pixels
*
* @param[in] u4_intra_mode
*  pointer to the variable in which best mode is returned
*
* @param[in] pu4_sadmin
*  pointer to the variable in which minimum sad is returned
*
* @param[in] u4_valid_intra_modes
*  says what all modes are valid
*
* @return
*  None
*
******************************************************************************
*/
void ih264e_evaluate_intra16x16_modes_ssse3(UWORD8 *pu1_src,
                                            UWORD8 *pu1_ngbr_pels_i16,
                                            UWORD8 *pu1_dst,
                                            UWORD32 src_strd,
                                            UWORD32 dst_strd,
                                            WORD32 n_avblty,
                                            UWORD32 *u4_intra_mode,
                                            WORD32 *pu4_sadmin,
                                            UWORD32 u4_valid_intra_modes)
{
    UWORD8 *pu1_src_temp;

    WORD32 left, top, horz_flag, vert_flag, dc_flag;
    WORD32 sad_vert, sad_horz, sad_dc, min_sad;

    WORD32 cnt, dcval;
    WORD32 src_strd2, src_strd3, src_strd4;
    WORD32 dst_strd2, dst_strd3, dst_strd4;

    __m128i src1_16x8b, src2_16x8b, src3_16x8b, src4_16x8b;
    __m128i val1_16x8b, val2_16x8b, val3_16x8b, val4_16x8b;
    __m128i sad1_8x16b, sad2_8x16b, sad3_8x16b, sad4_8x16b;

    __m128i sad_8x16b, val_16x8b, zero_vector;

    sad_vert = INT_MAX;
    sad_horz = INT_MAX;
    sad_dc = INT_MAX;

    src_strd2 = src_strd << 1;
    src_strd4 = src_strd << 2;
    src_strd3 = src_strd + src_strd2;

    dst_strd2 = dst_strd << 1;
    dst_strd4 = dst_strd << 2;
    dst_strd3 = dst_strd + dst_strd2;

    left = (n_avblty & LEFT_MB_AVAILABLE_MASK);
    top = (n_avblty & TOP_MB_AVAILABLE_MASK) >> 2;

    zero_vector = _mm_setzero_si128();

    horz_flag = left && ((u4_valid_intra_modes & 02) != 0);
    vert_flag = top && ((u4_valid_intra_modes & 01) != 0);
    dc_flag = (u4_valid_intra_modes & 04) != 0;

    if(horz_flag)
    {
        pu1_src_temp = pu1_src;

        val1_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[15]);
        val2_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[14]);
        val3_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[13]);
        val4_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[12]);

        src1_16x8b = _mm_loadu_si128((__m128i *)pu1_src_temp);
        src2_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd));
        src3_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd2));
        src4_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd3));

        sad1_8x16b = _mm_sad_epu8(val1_16x8b, src1_16x8b);
        sad2_8x16b = _mm_sad_epu8(val2_16x8b, src2_16x8b);
        sad3_8x16b = _mm_sad_epu8(val3_16x8b, src3_16x8b);
        sad4_8x16b = _mm_sad_epu8(val4_16x8b, src4_16x8b);

        sad1_8x16b = _mm_packs_epi32(sad1_8x16b, sad2_8x16b);
        sad3_8x16b = _mm_packs_epi32(sad3_8x16b, sad4_8x16b);

        cnt = 11;
        sad_8x16b = _mm_packs_epi32(sad1_8x16b, sad3_8x16b);
        do
        {
            pu1_src_temp += src_strd4;

            val1_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[cnt]);
            val2_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[cnt - 1]);
            val3_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[cnt - 2]);
            val4_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[cnt - 3]);

            src1_16x8b = _mm_loadu_si128((__m128i *)pu1_src_temp);
            src2_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd));
            src3_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd2));
            src4_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd3));

            sad1_8x16b = _mm_sad_epu8(val1_16x8b, src1_16x8b);
            sad2_8x16b = _mm_sad_epu8(val2_16x8b, src2_16x8b);
            sad3_8x16b = _mm_sad_epu8(val3_16x8b, src3_16x8b);
            sad4_8x16b = _mm_sad_epu8(val4_16x8b, src4_16x8b);

            sad1_8x16b = _mm_packs_epi32(sad1_8x16b, sad2_8x16b);
            sad3_8x16b = _mm_packs_epi32(sad3_8x16b, sad4_8x16b);
            sad1_8x16b = _mm_packs_epi32(sad1_8x16b, sad3_8x16b);

            cnt -= 4;
            sad_8x16b = _mm_add_epi16(sad_8x16b, sad1_8x16b);
        }
        while(cnt >= 0);

        sad_8x16b = _mm_hadd_epi16(sad_8x16b, sad_8x16b);
        sad_8x16b = _mm_hadd_epi16(sad_8x16b, sad_8x16b);
        sad_8x16b = _mm_hadd_epi16(sad_8x16b, sad_8x16b);

        sad_horz = _mm_extract_epi16(sad_8x16b, 0);
    }

    if(vert_flag)
    {
        pu1_src_temp = pu1_src;

        val1_16x8b = _mm_loadu_si128((__m128i *)(pu1_ngbr_pels_i16 + 17));

        src1_16x8b = _mm_loadu_si128((__m128i *)pu1_src_temp);
        src2_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd));
        src3_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd2));
        src4_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd3));

        sad1_8x16b = _mm_sad_epu8(val1_16x8b, src1_16x8b);
        sad2_8x16b = _mm_sad_epu8(val1_16x8b, src2_16x8b);
        sad3_8x16b = _mm_sad_epu8(val1_16x8b, src3_16x8b);
        sad4_8x16b = _mm_sad_epu8(val1_16x8b, src4_16x8b);

        sad1_8x16b = _mm_packs_epi32(sad1_8x16b, sad2_8x16b);
        sad3_8x16b = _mm_packs_epi32(sad3_8x16b, sad4_8x16b);

        cnt = 11;
        sad_8x16b = _mm_packs_epi32(sad1_8x16b, sad3_8x16b);
        do
        {
            pu1_src_temp += src_strd4;

            src1_16x8b = _mm_loadu_si128((__m128i *)pu1_src_temp);
            src2_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd));
            src3_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd2));
            src4_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd3));

            sad1_8x16b = _mm_sad_epu8(val1_16x8b, src1_16x8b);
            sad2_8x16b = _mm_sad_epu8(val1_16x8b, src2_16x8b);
            sad3_8x16b = _mm_sad_epu8(val1_16x8b, src3_16x8b);
            sad4_8x16b = _mm_sad_epu8(val1_16x8b, src4_16x8b);

            sad1_8x16b = _mm_packs_epi32(sad1_8x16b, sad2_8x16b);
            sad3_8x16b = _mm_packs_epi32(sad3_8x16b, sad4_8x16b);
            sad1_8x16b = _mm_packs_epi32(sad1_8x16b, sad3_8x16b);

            cnt -= 4;
            sad_8x16b = _mm_add_epi16(sad_8x16b, sad1_8x16b);
        }
        while(cnt >= 0);

        sad_8x16b = _mm_hadd_epi16(sad_8x16b, sad_8x16b);
        sad_8x16b = _mm_hadd_epi16(sad_8x16b, sad_8x16b);
        sad_8x16b = _mm_hadd_epi16(sad_8x16b, sad_8x16b);

        sad_vert = _mm_extract_epi16(sad_8x16b, 0);
    }

    dcval = 0;

    if(left)
    {
        val_16x8b = _mm_loadu_si128((__m128i *)pu1_ngbr_pels_i16);
        dcval += 8;

        sad1_8x16b = _mm_sad_epu8(val_16x8b, zero_vector);
        dcval += _mm_extract_epi16(sad1_8x16b, 0);
        dcval += _mm_extract_epi16(sad1_8x16b, 4);
    }
    if(top)
    {
        val_16x8b = _mm_loadu_si128((__m128i *)(pu1_ngbr_pels_i16 + 17));
        dcval += 8;

        sad1_8x16b = _mm_sad_epu8(val_16x8b, zero_vector);
        dcval += _mm_extract_epi16(sad1_8x16b, 0);
        dcval += _mm_extract_epi16(sad1_8x16b, 4);
    }
    dcval = dcval >> (3 + left + top);
    dcval += ((left == 0) & (top == 0)) << 7;

    if(dc_flag)
    {
        pu1_src_temp = pu1_src;
        val1_16x8b = _mm_set1_epi8(dcval);

        src1_16x8b = _mm_loadu_si128((__m128i *)pu1_src_temp);
        src2_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd));
        src3_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd2));
        src4_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd3));

        sad1_8x16b = _mm_sad_epu8(val1_16x8b, src1_16x8b);
        sad2_8x16b = _mm_sad_epu8(val1_16x8b, src2_16x8b);
        sad3_8x16b = _mm_sad_epu8(val1_16x8b, src3_16x8b);
        sad4_8x16b = _mm_sad_epu8(val1_16x8b, src4_16x8b);

        sad1_8x16b = _mm_packs_epi32(sad1_8x16b, sad2_8x16b);
        sad3_8x16b = _mm_packs_epi32(sad3_8x16b, sad4_8x16b);

        cnt = 12;
        sad_8x16b = _mm_packs_epi32(sad1_8x16b, sad3_8x16b);
        do
        {
            pu1_src_temp += src_strd4;

            src1_16x8b = _mm_loadu_si128((__m128i *)pu1_src_temp);
            src2_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd));
            src3_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd2));
            src4_16x8b = _mm_loadu_si128((__m128i *)(pu1_src_temp + src_strd3));

            sad1_8x16b = _mm_sad_epu8(val1_16x8b, src1_16x8b);
            sad2_8x16b = _mm_sad_epu8(val1_16x8b, src2_16x8b);
            sad3_8x16b = _mm_sad_epu8(val1_16x8b, src3_16x8b);
            sad4_8x16b = _mm_sad_epu8(val1_16x8b, src4_16x8b);

            sad1_8x16b = _mm_packs_epi32(sad1_8x16b, sad2_8x16b);
            sad3_8x16b = _mm_packs_epi32(sad3_8x16b, sad4_8x16b);
            sad1_8x16b = _mm_packs_epi32(sad1_8x16b, sad3_8x16b);

            cnt -= 4;
            sad_8x16b = _mm_add_epi16(sad_8x16b, sad1_8x16b);
        }
        while(cnt > 0);

        sad_8x16b = _mm_hadd_epi16(sad_8x16b, sad_8x16b);
        sad_8x16b = _mm_hadd_epi16(sad_8x16b, sad_8x16b);
        sad_8x16b = _mm_hadd_epi16(sad_8x16b, sad_8x16b);

        sad_dc = _mm_extract_epi16(sad_8x16b, 0);
    }

    // Doing prediction for minimum SAD
    min_sad = MIN3(sad_horz, sad_vert, sad_dc);
    if(min_sad < *pu4_sadmin)
    {
        *pu4_sadmin = min_sad;
        if(min_sad == sad_vert)
        {
            *u4_intra_mode = VERT_I16x16;
            val1_16x8b = _mm_loadu_si128((__m128i *)(pu1_ngbr_pels_i16 + 17));
            cnt = 15;
            do
            {
                _mm_storeu_si128((__m128i *)pu1_dst, val1_16x8b);
                _mm_storeu_si128((__m128i *)(pu1_dst + dst_strd), val1_16x8b);
                _mm_storeu_si128((__m128i *)(pu1_dst + dst_strd2), val1_16x8b);
                _mm_storeu_si128((__m128i *)(pu1_dst + dst_strd3), val1_16x8b);

                cnt -= 4;
                pu1_dst += dst_strd4;
            }
            while(cnt > 0);
        }
        else if(min_sad == sad_horz)
        {
            *u4_intra_mode = HORZ_I16x16;
            cnt = 15;
            do
            {
                val1_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[cnt]);
                val2_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[cnt - 1]);
                val3_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[cnt - 2]);
                val4_16x8b = _mm_set1_epi8(pu1_ngbr_pels_i16[cnt - 3]);

                _mm_storeu_si128((__m128i *)pu1_dst, val1_16x8b);
                _mm_storeu_si128((__m128i *)(pu1_dst + dst_strd), val2_16x8b);
                _mm_storeu_si128((__m128i *)(pu1_dst + dst_strd2), val3_16x8b);
                _mm_storeu_si128((__m128i *)(pu1_dst + dst_strd3), val4_16x8b);

                cnt -= 4;
                pu1_dst += dst_strd4;
            }
            while(cnt >= 0);
        }
        else
        {
            *u4_intra_mode = DC_I16x16;
            val1_16x8b = _mm_set1_epi8(dcval);
            cnt = 15;
            do
            {
                _mm_storeu_si128((__m128i *)pu1_dst, val1_16x8b);
                _mm_storeu_si128((__m128i *)(pu1_dst + dst_strd), val1_16x8b);
                _mm_storeu_si128((__m128i *)(pu1_dst + dst_strd2), val1_16x8b);
                _mm_storeu_si128((__m128i *)(pu1_dst + dst_strd3), val1_16x8b);

                cnt -= 4;
                pu1_dst += dst_strd4;
            }
            while(cnt > 0);
        }
    }
}

/**
******************************************************************************
*
* @brief :Evaluate best intra 4x4 mode and do the prediction.
*
* @par Description
*  This function evaluates intra 4x4 modes, computes corresponding sad
*  and returns the buffer predicted with best mode.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
** @param[in] pu1_ngbr_pels
*  UWORD8 pointer to neighbouring pels
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
* @param[in] u4_n_avblty
* availability of neighbouring pixels
*
* @param[in] u4_intra_mode
* Pointer to the variable in which best mode is returned
*
* @param[in] pu4_sadmin
* Pointer to the variable in which minimum cost is returned
*
* @param[in] u4_valid_intra_modes
* Says what all modes are valid
*
* * @param[in] u4_lambda
* Lamda value for computing cost from SAD
*
* @param[in] u4_predictd_mode
* Predicted mode for cost computation
*
* @return      none
*
******************************************************************************
*/
void ih264e_evaluate_intra_4x4_modes_ssse3(UWORD8 *pu1_src,
                                           UWORD8 *pu1_ngbr_pels,
                                           UWORD8 *pu1_dst,
                                           UWORD32 src_strd,
                                           UWORD32 dst_strd,
                                           WORD32 u4_n_avblty,
                                           UWORD32 *u4_intra_mode,
                                           WORD32 *pu4_sadmin,
                                           UWORD32 u4_valid_intra_modes,
                                           UWORD32 u4_lambda,
                                           UWORD32 u4_predictd_mode)
{
    WORD32 left, top;
    WORD32 sad[MAX_I4x4] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX,
                             INT_MAX, INT_MAX, INT_MAX, INT_MAX };
    WORD32 cost[MAX_I4x4] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX,
                              INT_MAX, INT_MAX, INT_MAX, INT_MAX };

    WORD32 min_cost;
    UWORD32 lambda4 = u4_lambda << 2;
    WORD32 dst_strd2, dst_strd3;

    __m128i left_top_16x8b, src_16x8b, pred0_16x8b, sad_8x16b;
    __m128i pred1_16x8b, pred2_16x8b, pred3_16x8b, pred4_16x8b;
    __m128i pred5_16x8b, pred6_16x8b, pred7_16x8b, pred8_16x8b;
    __m128i shuffle_16x8b, zero_vector, mask_low_32b;

    left = (u4_n_avblty & LEFT_MB_AVAILABLE_MASK);
    top  =  (u4_n_avblty & TOP_MB_AVAILABLE_MASK) >> 2;

    dst_strd2 = dst_strd << 1;
    dst_strd3 = dst_strd + dst_strd2;

    // loading the 4x4 source block and neighbouring pixels
    {
        __m128i row1_16x8b, row2_16x8b;

        row1_16x8b = _mm_loadl_epi64((__m128i *)pu1_src);
        row2_16x8b = _mm_loadl_epi64((__m128i *)(pu1_src + src_strd));
        left_top_16x8b = _mm_loadu_si128((__m128i *)pu1_ngbr_pels);

        pu1_src += src_strd << 1;
        src_16x8b = _mm_unpacklo_epi32(row1_16x8b, row2_16x8b);

        row1_16x8b = _mm_loadl_epi64((__m128i *)pu1_src);
        row2_16x8b = _mm_loadl_epi64((__m128i *)(pu1_src + src_strd));
        zero_vector = _mm_setzero_si128();

        row1_16x8b = _mm_unpacklo_epi32(row1_16x8b, row2_16x8b);
        src_16x8b = _mm_unpacklo_epi64(src_16x8b, row1_16x8b);
    }

    /* Computing SADs*/
    if(u4_valid_intra_modes & 1)/* VERT mode valid ????*/
    {
        pred0_16x8b = _mm_srli_si128(left_top_16x8b, 5);
        pred0_16x8b = _mm_shuffle_epi32(pred0_16x8b, 0);
        sad_8x16b = _mm_sad_epu8(src_16x8b, pred0_16x8b);

        sad[VERT_I4x4] = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
        cost[VERT_I4x4] = sad[VERT_I4x4] + ((u4_predictd_mode == VERT_I4x4) ? u4_lambda: lambda4);
    }

    if(u4_valid_intra_modes & 2)/* HORZ mode valid ????*/
    {
        shuffle_16x8b = _mm_setr_epi8(3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0);
        pred1_16x8b = _mm_shuffle_epi8(left_top_16x8b, shuffle_16x8b);

        sad_8x16b = _mm_sad_epu8(src_16x8b, pred1_16x8b);

        sad[HORZ_I4x4] = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
        cost[HORZ_I4x4] = sad[HORZ_I4x4] + ((u4_predictd_mode == HORZ_I4x4) ? u4_lambda: lambda4);
    }

    if(u4_valid_intra_modes & 4)/* DC mode valid ????*/
    {
        if(top + left)
        {
            WORD32 shft = 1, dcval = 0;

            __m128i val_16x8b, temp_16x8b, temp_8x16b;

            val_16x8b = _mm_setzero_si128();

            if(top)
            {
                temp_16x8b = _mm_srli_si128(left_top_16x8b, 5);
                val_16x8b = _mm_alignr_epi8(temp_16x8b, val_16x8b, 4);
                shft ++;
                dcval += 2;
            }
            if(left)
            {
                val_16x8b = _mm_alignr_epi8(left_top_16x8b, val_16x8b, 4);
                shft++;
                dcval += 2;
            }

            temp_8x16b = _mm_sad_epu8(val_16x8b, zero_vector);
            dcval += _mm_extract_epi16(temp_8x16b, 4);
            dcval = dcval >> shft;
            pred2_16x8b = _mm_set1_epi8(dcval);
        }
        else
            pred2_16x8b = _mm_set1_epi8(128);

        sad_8x16b = _mm_sad_epu8(src_16x8b, pred2_16x8b);

        sad[DC_I4x4] = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
        cost[DC_I4x4] = sad[DC_I4x4] + ((u4_predictd_mode == DC_I4x4) ? u4_lambda: lambda4);
    }

    if(u4_valid_intra_modes > 7)/* if modes other than VERT, HORZ and DC are  valid ????*/
    {
        __m128i w11_16x8b, w121_16x8b;
        __m128i temp1_16x8b, temp2_16x8b;

        /* Performing FILT121 and FILT11 operation for all neighbour values*/
        {
            __m128i temp1_8x16b, temp2_8x16b, temp3_8x16b;
            __m128i const_2_8x16b;

            const_2_8x16b = _mm_set1_epi16(2);

            temp1_8x16b = _mm_unpacklo_epi8(left_top_16x8b, zero_vector);   //l3 l2 l1 l0 tl t0 t1 t2
            temp2_8x16b = _mm_slli_si128(temp1_8x16b, 2);                   // 0 l3 l2 l1 l0 tl t0 t1
            temp2_8x16b = _mm_shufflelo_epi16(temp2_8x16b, 0xe5);           //l3 l3 l2 l1 l0 tl t0 t1

            temp1_8x16b = _mm_add_epi16(temp1_8x16b, temp2_8x16b);          //l3+l3  l3+l2       l2+l1...       t1+t2
            temp2_8x16b = _mm_slli_si128(temp1_8x16b, 2);                   //l3+l3  l3+l3       l3+l2...       t0+t1
            temp2_8x16b = _mm_shufflelo_epi16(temp2_8x16b, 0xe5);
            temp1_8x16b = _mm_add_epi16(temp1_8x16b, temp2_8x16b);          //4*l3   l3+2*l3+l2  l3+2*l2+l1...  t0+2*t1+t2

            temp1_8x16b = _mm_add_epi16(const_2_8x16b, temp1_8x16b);        //4*l3+2 3*l3+l2+2   l3+2*l2+l1+2.. t0+2*t1+t2+2
            temp1_8x16b = _mm_srli_epi16(temp1_8x16b, 2);

            temp1_16x8b = _mm_srli_si128(left_top_16x8b, 1);
            w11_16x8b = _mm_avg_epu8(left_top_16x8b, temp1_16x8b);

            temp2_16x8b = _mm_srli_si128(left_top_16x8b, 6);
            temp2_8x16b = _mm_unpacklo_epi8(temp2_16x8b, zero_vector);      //t1 t2 t3 t4 t5 t6 t7 0
            temp3_8x16b = _mm_srli_si128(temp2_8x16b, 2);                   //t2 t3 t4 t5 t6 t7 0  0
            temp3_8x16b = _mm_shufflehi_epi16(temp3_8x16b, 0xd4);           //t2 t3 t4 t5 t6 t7 t7 0

            temp2_8x16b = _mm_add_epi16(temp2_8x16b, temp3_8x16b);          //t1+t2      t2+t3...     t6+t7      t7+t7 0
            temp3_8x16b = _mm_srli_si128(temp2_8x16b, 2);                   //t2+t3      t3+t4...     t7+t7      0     0
            temp2_8x16b = _mm_add_epi16(temp2_8x16b, temp3_8x16b);          //t1+2*t2+t3 t2+2*t3+t4.. t6+2*t7+t7 t7+t7 0

            temp2_8x16b = _mm_add_epi16(const_2_8x16b, temp2_8x16b);        //t1+2*t2+t3+2 t2+2*t3+t4+2 t3+2*t4+t5+2... t6+2*t7+t7+2 t7+t7+2  2
            temp2_8x16b = _mm_srli_epi16(temp2_8x16b, 2);

            w121_16x8b = _mm_packus_epi16(temp1_8x16b, temp2_8x16b);
        }

        if(u4_valid_intra_modes & 8)/* DIAG_DL */
        {
            shuffle_16x8b = _mm_setr_epi8( 7,  8,  9,  10,
                                           8,  9,  10, 11,
                                           9,  10, 11, 12,
                                          10,  11, 12, 13);
            pred3_16x8b = _mm_shuffle_epi8(w121_16x8b, shuffle_16x8b);
            sad_8x16b = _mm_sad_epu8(src_16x8b, pred3_16x8b);

            sad[DIAG_DL_I4x4] = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
            cost[DIAG_DL_I4x4] = sad[DIAG_DL_I4x4] + ((u4_predictd_mode == DIAG_DL_I4x4) ? u4_lambda: lambda4);
        }

        if(u4_valid_intra_modes & 16)/* DIAG_DR */
        {
            shuffle_16x8b = _mm_setr_epi8(5, 6, 7, 8,
                                          4, 5, 6, 7,
                                          3, 4, 5, 6,
                                          2, 3, 4, 5);
            pred4_16x8b = _mm_shuffle_epi8(w121_16x8b, shuffle_16x8b);
            sad_8x16b = _mm_sad_epu8(src_16x8b, pred4_16x8b);

            sad[DIAG_DR_I4x4] = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
            cost[DIAG_DR_I4x4] = sad[DIAG_DR_I4x4] + ((u4_predictd_mode == DIAG_DR_I4x4) ? u4_lambda: lambda4);
        }

        if(u4_valid_intra_modes & 32)/* VERT_R mode valid ????*/
        {
            temp1_16x8b = _mm_srli_si128(w121_16x8b, 1);
            temp1_16x8b = _mm_unpacklo_epi64(temp1_16x8b, w11_16x8b);
            shuffle_16x8b = _mm_setr_epi8(12, 13, 14, 15,
                                           4,  5,  6,  7,
                                           3, 12, 13, 14,
                                           2,  4,  5,  6);
            pred5_16x8b = _mm_shuffle_epi8(temp1_16x8b, shuffle_16x8b);
            sad_8x16b = _mm_sad_epu8(src_16x8b, pred5_16x8b);

            sad[VERT_R_I4x4] = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
            cost[VERT_R_I4x4] = sad[VERT_R_I4x4] + ((u4_predictd_mode == VERT_R_I4x4) ? u4_lambda: lambda4);
        }

        if(u4_valid_intra_modes & 64)/* HORZ_D mode valid ????*/
        {
            temp1_16x8b = _mm_unpacklo_epi64(w121_16x8b, w11_16x8b);
            shuffle_16x8b = _mm_setr_epi8(11, 5,  6, 7,
                                          10, 4, 11, 5,
                                           9, 3, 10, 4,
                                           8, 2,  9, 3);
            pred6_16x8b = _mm_shuffle_epi8(temp1_16x8b, shuffle_16x8b);
            sad_8x16b = _mm_sad_epu8(src_16x8b, pred6_16x8b);

            sad[HORZ_D_I4x4] = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
            cost[HORZ_D_I4x4] = sad[HORZ_D_I4x4] + ((u4_predictd_mode == HORZ_D_I4x4) ? u4_lambda: lambda4);
        }

        if(u4_valid_intra_modes & 128)/* VERT_L mode valid ????*/
        {
            temp1_16x8b = _mm_srli_si128(w121_16x8b, 5);
            temp2_16x8b = _mm_srli_si128(w11_16x8b, 5);
            temp1_16x8b = _mm_unpacklo_epi64(temp1_16x8b, temp2_16x8b);
            shuffle_16x8b = _mm_setr_epi8(8,  9, 10, 11,
                                          2,  3,  4,  5,
                                          9, 10, 11, 12,
                                          3,  4,  5,  6);
            pred7_16x8b = _mm_shuffle_epi8(temp1_16x8b, shuffle_16x8b);
            sad_8x16b = _mm_sad_epu8(src_16x8b, pred7_16x8b);

            sad[VERT_L_I4x4] = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
            cost[VERT_L_I4x4] = sad[VERT_L_I4x4] + ((u4_predictd_mode == VERT_L_I4x4) ? u4_lambda: lambda4);
        }

        if(u4_valid_intra_modes & 256)/* HORZ_U mode valid ????*/
        {
            temp1_16x8b = _mm_unpacklo_epi64(w121_16x8b, w11_16x8b);
            shuffle_16x8b = _mm_setr_epi8(10, 3, 9, 2,
                                           9, 2, 8, 1,
                                           8, 1, 0, 0,
                                           0, 0, 0, 0);
            pred8_16x8b = _mm_shuffle_epi8(temp1_16x8b, shuffle_16x8b);
            sad_8x16b = _mm_sad_epu8(src_16x8b, pred8_16x8b);

            sad[HORZ_U_I4x4] = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
            cost[HORZ_U_I4x4] = sad[HORZ_U_I4x4] + ((u4_predictd_mode == HORZ_U_I4x4) ? u4_lambda: lambda4);
        }

        min_cost = MIN3(MIN3(cost[0], cost[1], cost[2]),
                        MIN3(cost[3], cost[4], cost[5]),
                        MIN3(cost[6], cost[7], cost[8]));
    }
    else
    {  /*Only first three modes valid*/
        min_cost = MIN3(cost[0], cost[1], cost[2]);
    }

    *pu4_sadmin = min_cost;

    if(min_cost == cost[0])
    {
        *u4_intra_mode = VERT_I4x4;
    }
    else if(min_cost == cost[1])
    {
        *u4_intra_mode = HORZ_I4x4;
        pred0_16x8b = pred1_16x8b;
    }
    else if(min_cost == cost[2])
    {
        *u4_intra_mode = DC_I4x4;
        pred0_16x8b = pred2_16x8b;
    }
    else if(min_cost == cost[3])
    {
        *u4_intra_mode = DIAG_DL_I4x4;
        pred0_16x8b = pred3_16x8b;
    }
    else if(min_cost == cost[4])
    {
        *u4_intra_mode = DIAG_DR_I4x4;
        pred0_16x8b = pred4_16x8b;
    }
    else if(min_cost == cost[5])
    {
        *u4_intra_mode = VERT_R_I4x4;
        pred0_16x8b = pred5_16x8b;
    }
    else if(min_cost == cost[6])
    {
        *u4_intra_mode = HORZ_D_I4x4;
        pred0_16x8b = pred6_16x8b;
    }
    else if(min_cost == cost[7])
    {
        *u4_intra_mode = VERT_L_I4x4;
        pred0_16x8b = pred7_16x8b;
    }
    else if(min_cost == cost[8])
    {
        *u4_intra_mode = HORZ_U_I4x4;
        pred0_16x8b = pred8_16x8b;
    }

    mask_low_32b = _mm_set1_epi8(0xff);
    mask_low_32b = _mm_srli_si128(mask_low_32b, 12);

    _mm_maskmoveu_si128(pred0_16x8b, mask_low_32b, (char*)pu1_dst);
    pred0_16x8b = _mm_srli_si128(pred0_16x8b, 4);
    _mm_maskmoveu_si128(pred0_16x8b, mask_low_32b, (char*)(pu1_dst + dst_strd));
    pred0_16x8b = _mm_srli_si128(pred0_16x8b, 4);
    _mm_maskmoveu_si128(pred0_16x8b, mask_low_32b, (char*)(pu1_dst + dst_strd2));
    pred0_16x8b = _mm_srli_si128(pred0_16x8b, 4);
    _mm_maskmoveu_si128(pred0_16x8b, mask_low_32b, (char*)(pu1_dst + dst_strd3));

}

/**
******************************************************************************
*
* @brief
*  Evaluate best intra chroma mode (among VERT, HORZ and DC) and do the prediction.
*
* @par Description
*  This function evaluates first three intra chroma modes and compute corresponding sad
*  and return the buffer predicted with best mode.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
** @param[in] pu1_ngbr_pels
*  UWORD8 pointer to neighbouring pels
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
* @param[in] u4_n_avblty
*  availability of neighbouring pixels
*
* @param[in] u4_intra_mode
*  pointer to the variable in which best mode is returned
*
* @param[in] pu4_sadmin
*  pointer to the variable in which minimum sad is returned
*
* @param[in] u4_valid_intra_modes
*  says what all modes are valid
*
* @return
*  none
*
******************************************************************************
*/

void ih264e_evaluate_intra_chroma_modes_ssse3(UWORD8 *pu1_src,
                                              UWORD8 *pu1_ngbr_pels,
                                              UWORD8 *pu1_dst,
                                              UWORD32 src_strd,
                                              UWORD32 dst_strd,
                                              WORD32 u4_n_avblty,
                                              UWORD32 *u4_intra_mode,
                                              WORD32 *pu4_sadmin,
                                              UWORD32 u4_valid_intra_modes)
{
    WORD32 left, top;
    WORD32 sad_vert = INT_MAX, sad_horz = INT_MAX, sad_dc = INT_MAX, min_sad;

    __m128i src1_16x8b, src2_16x8b, src3_16x8b, src4_16x8b;
    __m128i src5_16x8b, src6_16x8b, src7_16x8b, src8_16x8b;

    __m128i top_16x8b, left_16x8b;
    __m128i pred1_16x8b, pred2_16x8b;
    __m128i tmp1_8x16b, tmp2_8x16b, sad_8x16b;

    left = (u4_n_avblty & LEFT_MB_AVAILABLE_MASK);
    top = (u4_n_avblty & TOP_MB_AVAILABLE_MASK) >> 2;

    //Loading source
    {
        src1_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        pu1_src += src_strd;
        src2_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        pu1_src += src_strd;
        src3_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        pu1_src += src_strd;
        src4_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        pu1_src += src_strd;
        src5_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        pu1_src += src_strd;
        src6_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        pu1_src += src_strd;
        src7_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
        pu1_src += src_strd;
        src8_16x8b = _mm_loadu_si128((__m128i *)pu1_src);
    }

    if(left)
    {
        left_16x8b = _mm_loadu_si128((__m128i *)pu1_ngbr_pels);

        if(u4_valid_intra_modes & 02) //If HORZ mode is valid
        {
            __m128i left_tmp_16x8b, left_sh_16x8b;
            __m128i const_14_15_16x8b;

            const_14_15_16x8b = _mm_set1_epi16(0x0f0e);
            left_sh_16x8b = _mm_slli_si128(left_16x8b, 2);

            pred1_16x8b = _mm_shuffle_epi8(left_16x8b, const_14_15_16x8b);    //row 1
            pred2_16x8b = _mm_shuffle_epi8(left_sh_16x8b, const_14_15_16x8b); //row 2
            tmp1_8x16b = _mm_sad_epu8(src1_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src2_16x8b, pred2_16x8b);

            left_tmp_16x8b = _mm_slli_si128(left_16x8b, 4);
            left_sh_16x8b = _mm_slli_si128(left_sh_16x8b, 4);
            sad_8x16b = _mm_add_epi16(tmp1_8x16b, tmp2_8x16b);

            pred1_16x8b = _mm_shuffle_epi8(left_tmp_16x8b, const_14_15_16x8b); //row 3
            pred2_16x8b = _mm_shuffle_epi8(left_sh_16x8b, const_14_15_16x8b);  //row 4
            tmp1_8x16b = _mm_sad_epu8(src3_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src4_16x8b, pred2_16x8b);

            left_tmp_16x8b = _mm_slli_si128(left_tmp_16x8b, 4);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            left_sh_16x8b = _mm_slli_si128(left_sh_16x8b, 4);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            pred1_16x8b = _mm_shuffle_epi8(left_tmp_16x8b, const_14_15_16x8b); //row 5
            pred2_16x8b = _mm_shuffle_epi8(left_sh_16x8b, const_14_15_16x8b);  //row 6
            tmp1_8x16b = _mm_sad_epu8(src5_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src6_16x8b, pred2_16x8b);

            left_tmp_16x8b = _mm_slli_si128(left_tmp_16x8b, 4);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            left_sh_16x8b = _mm_slli_si128(left_sh_16x8b, 4);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            pred1_16x8b = _mm_shuffle_epi8(left_tmp_16x8b, const_14_15_16x8b); //row 7
            pred2_16x8b = _mm_shuffle_epi8(left_sh_16x8b, const_14_15_16x8b);  //row 8
            tmp1_8x16b = _mm_sad_epu8(src7_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src8_16x8b, pred2_16x8b);

            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            sad_horz = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
        }
    }

    if(top)
    {
        UWORD8 *pu1_top;

        pu1_top = pu1_ngbr_pels + 2 * BLK8x8SIZE + 2;
        top_16x8b = _mm_loadu_si128((__m128i *)pu1_top);

        if(u4_valid_intra_modes & 04) //If VERT mode is valid
        {
            tmp1_8x16b = _mm_sad_epu8(src1_16x8b, top_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src2_16x8b, top_16x8b);
            sad_8x16b = _mm_add_epi16(tmp1_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src3_16x8b, top_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src4_16x8b, top_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src5_16x8b, top_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src6_16x8b, top_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src7_16x8b, top_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src8_16x8b, top_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            sad_vert = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
        }
    }

    if(u4_valid_intra_modes & 01) //If DC mode is valid
    {
        if(left && top)
        {
            WORD32 left_up_u, left_down_u, left_up_v, left_down_v;
            WORD32 top_left_u, top_right_u, top_left_v, top_right_v;
            WORD32 dc_1u, dc_1v, dc_2u, dc_2v;

            __m128i val_sh_16x8b;
            __m128i intrlv_mask_8x16b, zero_vector;

            intrlv_mask_8x16b = _mm_set1_epi16(0x00ff);
            zero_vector = _mm_setzero_si128();

            val_sh_16x8b = _mm_srli_si128(left_16x8b, 1);

            tmp1_8x16b = _mm_and_si128(intrlv_mask_8x16b, left_16x8b);
            tmp2_8x16b = _mm_and_si128(intrlv_mask_8x16b, val_sh_16x8b);
            tmp1_8x16b = _mm_sad_epu8(zero_vector, tmp1_8x16b);
            tmp2_8x16b = _mm_sad_epu8(zero_vector, tmp2_8x16b);

            left_up_u = _mm_extract_epi16(tmp1_8x16b, 4);
            left_up_v = _mm_extract_epi16(tmp2_8x16b, 4);
            left_down_u = _mm_extract_epi16(tmp1_8x16b, 0);
            left_down_v = _mm_extract_epi16(tmp2_8x16b, 0);

            val_sh_16x8b = _mm_srli_si128(top_16x8b, 1);

            tmp1_8x16b = _mm_and_si128(intrlv_mask_8x16b, top_16x8b);
            tmp2_8x16b = _mm_and_si128(intrlv_mask_8x16b, val_sh_16x8b);
            tmp1_8x16b = _mm_sad_epu8(zero_vector, tmp1_8x16b);
            tmp2_8x16b = _mm_sad_epu8(zero_vector, tmp2_8x16b);

            top_left_u = _mm_extract_epi16(tmp1_8x16b, 0);
            top_left_v = _mm_extract_epi16(tmp2_8x16b, 0);
            top_right_u = _mm_extract_epi16(tmp1_8x16b, 4);
            top_right_v = _mm_extract_epi16(tmp2_8x16b, 4);

            // First four rows
            dc_1u = (left_up_u + top_left_u + 4) >> 3;
            dc_1v = (left_up_v + top_left_v + 4) >> 3;
            dc_2u = (top_right_u + 2) >> 2;
            dc_2v = (top_right_v + 2) >> 2;

            pred1_16x8b = _mm_setr_epi8(dc_1u, dc_1v, dc_1u, dc_1v, dc_1u, dc_1v, dc_1u, dc_1v,
                                        dc_2u, dc_2v, dc_2u, dc_2v, dc_2u, dc_2v, dc_2u, dc_2v);

            tmp1_8x16b = _mm_sad_epu8(src1_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src2_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(tmp1_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src3_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src4_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            // Second four rows
            dc_1u = (left_down_u + 2) >> 2;
            dc_1v = (left_down_v + 2) >> 2;
            dc_2u = (left_down_u + top_right_u + 4) >> 3;
            dc_2v = (left_down_v + top_right_v + 4) >> 3;

            pred2_16x8b = _mm_setr_epi8(dc_1u, dc_1v, dc_1u, dc_1v, dc_1u, dc_1v, dc_1u, dc_1v,
                                        dc_2u, dc_2v, dc_2u, dc_2v, dc_2u, dc_2v, dc_2u, dc_2v);

            tmp1_8x16b = _mm_sad_epu8(src5_16x8b, pred2_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src6_16x8b, pred2_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src7_16x8b, pred2_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src8_16x8b, pred2_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            sad_dc = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
        }
        else if(left)
        {
            WORD32 left_up_u, left_down_u, left_up_v, left_down_v;
            WORD32 dc_u, dc_v;

            __m128i left_sh_16x8b;
            __m128i intrlv_mask_8x16b, zero_vector;

            intrlv_mask_8x16b = _mm_set1_epi16(0x00ff);
            zero_vector = _mm_setzero_si128();

            left_sh_16x8b = _mm_srli_si128(left_16x8b, 1);

            tmp1_8x16b = _mm_and_si128(intrlv_mask_8x16b, left_16x8b);
            tmp2_8x16b = _mm_and_si128(intrlv_mask_8x16b, left_sh_16x8b);
            tmp1_8x16b = _mm_sad_epu8(zero_vector, tmp1_8x16b);
            tmp2_8x16b = _mm_sad_epu8(zero_vector, tmp2_8x16b);

            left_up_u = _mm_extract_epi16(tmp1_8x16b, 4);
            left_up_v = _mm_extract_epi16(tmp2_8x16b, 4);
            left_down_u = _mm_extract_epi16(tmp1_8x16b, 0);
            left_down_v = _mm_extract_epi16(tmp2_8x16b, 0);

            // First four rows
            dc_u = (left_up_u + 2) >> 2;
            dc_v = (left_up_v + 2) >> 2;

            pred1_16x8b = _mm_set1_epi16(dc_u | (dc_v << 8));

            tmp1_8x16b = _mm_sad_epu8(src1_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src2_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(tmp1_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src3_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src4_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            // Second four rows
            dc_u = (left_down_u + 2) >> 2;
            dc_v = (left_down_v + 2) >> 2;

            pred2_16x8b = _mm_set1_epi16(dc_u | (dc_v << 8));

            tmp1_8x16b = _mm_sad_epu8(src5_16x8b, pred2_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src6_16x8b, pred2_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src7_16x8b, pred2_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src8_16x8b, pred2_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            sad_dc = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
        }
        else if(top)
        {
            WORD32 top_left_u, top_right_u, top_left_v, top_right_v;
            WORD32 dc_1u, dc_1v, dc_2u, dc_2v;

            __m128i top_sh_16x8b;
            __m128i intrlv_mask_8x16b, zero_vector;

            intrlv_mask_8x16b = _mm_set1_epi16(0x00ff);
            zero_vector = _mm_setzero_si128();

            top_sh_16x8b = _mm_srli_si128(top_16x8b, 1);

            tmp1_8x16b = _mm_and_si128(intrlv_mask_8x16b, top_16x8b);
            tmp2_8x16b = _mm_and_si128(intrlv_mask_8x16b, top_sh_16x8b);
            tmp1_8x16b = _mm_sad_epu8(zero_vector, tmp1_8x16b);
            tmp2_8x16b = _mm_sad_epu8(zero_vector, tmp2_8x16b);

            top_left_u = _mm_extract_epi16(tmp1_8x16b, 0);
            top_left_v = _mm_extract_epi16(tmp2_8x16b, 0);
            top_right_u = _mm_extract_epi16(tmp1_8x16b, 4);
            top_right_v = _mm_extract_epi16(tmp2_8x16b, 4);

            dc_1u = (top_left_u + 2) >> 2;
            dc_1v = (top_left_v + 2) >> 2;
            dc_2u = (top_right_u + 2) >> 2;
            dc_2v = (top_right_v + 2) >> 2;

            pred1_16x8b = _mm_setr_epi8(dc_1u, dc_1v, dc_1u, dc_1v, dc_1u, dc_1v, dc_1u, dc_1v,
                                       dc_2u, dc_2v, dc_2u, dc_2v, dc_2u, dc_2v, dc_2u, dc_2v);

            tmp1_8x16b = _mm_sad_epu8(src1_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src2_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(tmp1_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src3_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src4_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src5_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src6_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src7_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src8_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            sad_dc = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
        }
        else
        {
            pred1_16x8b = _mm_set1_epi8(128);

            tmp1_8x16b = _mm_sad_epu8(src1_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src2_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(tmp1_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src3_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src4_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src5_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src6_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            tmp1_8x16b = _mm_sad_epu8(src7_16x8b, pred1_16x8b);
            tmp2_8x16b = _mm_sad_epu8(src8_16x8b, pred1_16x8b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp1_8x16b);
            sad_8x16b = _mm_add_epi16(sad_8x16b, tmp2_8x16b);

            sad_dc = _mm_extract_epi16(sad_8x16b, 0) + _mm_extract_epi16(sad_8x16b, 4);
        }
    }

    min_sad = MIN3(sad_horz, sad_vert, sad_dc);

    /* Finding minimum SAD and doing corresponding prediction*/
    if(min_sad < *pu4_sadmin)
    {
        *pu4_sadmin = min_sad;

        if(min_sad == sad_dc)
        {
            *u4_intra_mode = DC_CH_I8x8;

            if(!left)
                pred2_16x8b = pred1_16x8b;

            _mm_storeu_si128((__m128i *)pu1_dst, pred1_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred1_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred1_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred1_16x8b);
            pu1_dst += dst_strd;

            _mm_storeu_si128((__m128i *)pu1_dst, pred2_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred2_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred2_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred2_16x8b);
        }
        else if(min_sad == sad_horz)
        {
            __m128i left_sh_16x8b, const_14_15_16x8b;

            *u4_intra_mode = HORZ_CH_I8x8;

            const_14_15_16x8b = _mm_set1_epi16(0x0f0e);

            left_sh_16x8b = _mm_slli_si128(left_16x8b, 2);
            pred1_16x8b = _mm_shuffle_epi8(left_16x8b, const_14_15_16x8b);    //row 1
            pred2_16x8b = _mm_shuffle_epi8(left_sh_16x8b, const_14_15_16x8b); //row 2

            _mm_storeu_si128((__m128i *)pu1_dst, pred1_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred2_16x8b);

            left_16x8b = _mm_slli_si128(left_16x8b, 4);
            left_sh_16x8b = _mm_slli_si128(left_sh_16x8b, 4);
            pred1_16x8b = _mm_shuffle_epi8(left_16x8b, const_14_15_16x8b);    //row 3
            pred2_16x8b = _mm_shuffle_epi8(left_sh_16x8b, const_14_15_16x8b); //row 4

            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred1_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred2_16x8b);

            left_16x8b = _mm_slli_si128(left_16x8b, 4);
            left_sh_16x8b = _mm_slli_si128(left_sh_16x8b, 4);
            pred1_16x8b = _mm_shuffle_epi8(left_16x8b, const_14_15_16x8b);    //row 5
            pred2_16x8b = _mm_shuffle_epi8(left_sh_16x8b, const_14_15_16x8b); //row 6

            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred1_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred2_16x8b);

            left_16x8b = _mm_slli_si128(left_16x8b, 4);
            left_sh_16x8b = _mm_slli_si128(left_sh_16x8b, 4);
            pred1_16x8b = _mm_shuffle_epi8(left_16x8b, const_14_15_16x8b);    //row 7
            pred2_16x8b = _mm_shuffle_epi8(left_sh_16x8b, const_14_15_16x8b); //row 8

            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred1_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, pred2_16x8b);
        }
        else
        {
            *u4_intra_mode = VERT_CH_I8x8;

            _mm_storeu_si128((__m128i *)pu1_dst, top_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, top_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, top_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, top_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, top_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, top_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, top_16x8b);
            pu1_dst += dst_strd;
            _mm_storeu_si128((__m128i *)pu1_dst, top_16x8b);
        }
    }
}
