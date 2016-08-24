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
*  icv_variance_sse42.c
*
* @brief
*  This file contains the functions to compute variance
*
* @author
*  Ittiam
*
* @par List of Functions:
*  icv_variance_8x4_ssse3()
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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <immintrin.h>

/* User include files */
#include "icv_datatypes.h"
#include "icv_macros.h"
#include "icv_platform_macros.h"
#include "icv.h"

/**
*******************************************************************************
*
* @brief
*  Computes variance of a given 8x4 block
*
* @par   Description
*  Compute variance of a given 8x4 block
*
* @param[in] pu1_src
*  Source
*
* @param[in] src_strd
*  Source stride
*
* @param[in] wd
*  Assumed to be 8
*
* @param[in] ht
*  Assumed to be 4
*
* @returns
*  Variance
*
* @remarks
*
*******************************************************************************
*/
WORD32 icv_variance_8x4_ssse3(UWORD8 *pu1_src, WORD32 src_strd, WORD32 wd, WORD32 ht)
{
    WORD32 sum;
    WORD32 sum_sqr;
    WORD32 blk_sz;
    WORD32 vrnc;
    __m128  src_r0, src_r1;
    __m128i ssrc_r0, ssrc_r1, ssrc_r2, ssrc_r3;
    __m128i sum_r0, sum_r1;
    __m128i sqr_r0, sqr_r1, sqr_r2, sqr_r3;
    __m128i vsum, vsum_sqr;
    __m128i zero;
    UNUSED(wd);
    UNUSED(ht);

    ASSERT(wd == 8);
    ASSERT(ht == 4);

    sum     = 0;
    sum_sqr = 0;

    blk_sz = 8 * 4;

    zero = _mm_setzero_si128();

    /* Load source */
    src_r0 = (__m128)_mm_loadl_epi64((__m128i *) (pu1_src));
    pu1_src += src_strd;

    src_r1 = (__m128)_mm_loadl_epi64((__m128i *) (pu1_src));
    pu1_src += src_strd;

    src_r0 = _mm_loadh_pi (src_r0, (__m64 *) (pu1_src));
    pu1_src += src_strd;

    src_r1 = _mm_loadh_pi (src_r1, (__m64 *) (pu1_src));
    pu1_src += src_strd;

    /* Compute sum of all elements */
    /* Use SAD with 0, since there is no pairwise addition */
    sum_r0  = _mm_sad_epu8((__m128i)src_r0, zero);
    sum_r1  = _mm_sad_epu8((__m128i)src_r1, zero);

    /* Accumulate SAD */
    vsum    = _mm_add_epi64(sum_r0, sum_r1);
    vsum    = _mm_add_epi64(vsum, _mm_srli_si128(vsum, 8));

    sum = _mm_cvtsi128_si32(vsum);

    /* Unpack to 16 bits */
    ssrc_r0 = _mm_unpacklo_epi8((__m128i)src_r0, zero);
    ssrc_r1 = _mm_unpacklo_epi8((__m128i)src_r1, zero);
    ssrc_r2 = _mm_unpackhi_epi8((__m128i)src_r0, zero);
    ssrc_r3 = _mm_unpackhi_epi8((__m128i)src_r1, zero);

    /* Compute sum of squares */
    sqr_r0 = _mm_madd_epi16(ssrc_r0,  ssrc_r0);
    sqr_r1 = _mm_madd_epi16(ssrc_r1,  ssrc_r1);
    sqr_r2 = _mm_madd_epi16(ssrc_r2,  ssrc_r2);
    sqr_r3 = _mm_madd_epi16(ssrc_r3,  ssrc_r3);

    vsum_sqr = _mm_add_epi32(sqr_r0,   sqr_r1);
    vsum_sqr = _mm_add_epi32(vsum_sqr, sqr_r2);
    vsum_sqr = _mm_add_epi32(vsum_sqr, sqr_r3);

    vsum_sqr = _mm_add_epi32(vsum_sqr, _mm_srli_si128(vsum_sqr, 8));
    vsum_sqr = _mm_add_epi32(vsum_sqr, _mm_srli_si128(vsum_sqr, 4));
    sum_sqr  = _mm_cvtsi128_si32(vsum_sqr);

    /* Compute variance */
    vrnc = ((sum_sqr * blk_sz) - (sum * sum)) / (blk_sz * blk_sz);

    return vrnc;
}

