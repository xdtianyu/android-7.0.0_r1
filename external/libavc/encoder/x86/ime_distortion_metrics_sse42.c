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
******************************************************************************
* @file ime_distortion_metrics_sse42.c
*
* @brief
*  This file contains definitions of routines that compute distortion
*  between two macro/sub blocks of identical dimensions
*
* @author
*  Ittiam
*
* @par List of Functions:
*  - ime_compute_sad_16x16_sse42()
*  - ime_compute_sad_16x16_fast_sse42()
*  - ime_compute_sad_16x16_ea8_sse42()
*  - ime_compute_sad_16x8_sse42()
*  - ime_calculate_sad4_prog_sse42()
*  - ime_sub_pel_compute_sad_16x16_sse42()
*  - ime_compute_satqd_16x16_lumainter_sse42()
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
#include <stdlib.h>
#include <string.h>

/* User include files */
#include "ime_typedefs.h"
#include "ime_defs.h"
#include "ime_macros.h"
#include "ime_statistics.h"
#include "ime_platform_macros.h"
#include "ime_distortion_metrics.h"
#include <immintrin.h>

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
******************************************************************************
*
* @brief computes distortion (SAD) between 2 16x16 blocks
*
* @par   Description
*   This functions computes SAD between 2 16x16 blocks. There is a provision
*   for early exit if the up-to computed SAD exceeds maximum allowed SAD. To
*   compute the distortion of the entire block set u4_max_sad to USHRT_MAX.
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
* @param[in] i4_max_sad
*  integer maximum allowed distortion
*
* @param[out] pi4_mb_distortion
*  integer evaluated sad
*
* @remarks
*
******************************************************************************
*/
void ime_compute_sad_16x16_sse42(UWORD8 *pu1_src,
                           UWORD8 *pu1_est,
                           WORD32 src_strd,
                           WORD32 est_strd,
                           WORD32 i4_max_sad,
                           WORD32 *pi4_mb_distortion)
{
    __m128i src_r0, src_r1, src_r2, src_r3;
    __m128i est_r0, est_r1, est_r2, est_r3;
    __m128i res_r0, res_r1, res_r2, res_r3;
    __m128i sad_val;
    int val1, val2;
    UNUSED (i4_max_sad);

    // Row 0-3 sad calculation
    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 2*src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 3*src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 2*est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 3*est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(res_r0, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    // Row 4-7 sad calculation
    pu1_src += 4*src_strd;
    pu1_est += 4*est_strd;

    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 2*src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 3*src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 2*est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 3*est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(sad_val, res_r0);
    sad_val = _mm_add_epi64(sad_val, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    // Row 8-11 sad calculation
    pu1_src += 4*src_strd;
    pu1_est += 4*est_strd;
    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 2*src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 3*src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 2*est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 3*est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(sad_val, res_r0);
    sad_val = _mm_add_epi64(sad_val, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    // Row 12-15 sad calculation
    pu1_src += 4*src_strd;
    pu1_est += 4*est_strd;
    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 2*src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 3*src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 2*est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 3*est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(sad_val, res_r0);
    sad_val = _mm_add_epi64(sad_val, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    val1 = _mm_extract_epi32(sad_val,0);
    val2 = _mm_extract_epi32(sad_val, 2);
    *pi4_mb_distortion = (val1+val2);

    return;
}

/**
******************************************************************************
*
*  @brief computes distortion (SAD) between 2 16x8  blocks
*
*
*  @par   Description
*   This functions computes SAD between 2 16x8 blocks. There is a provision
*   for early exit if the up-to computed SAD exceeds maximum allowed SAD. To
*   compute the distortion of the entire block set u4_max_sad to USHRT_MAX.
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
* @param[in] u4_max_sad
*  integer maximum allowed distortion
*
* @param[out] pi4_mb_distortion
*  integer evaluated sad
*
* @remarks
*
******************************************************************************
*/
void ime_compute_sad_16x8_sse42(UWORD8 *pu1_src,
                    UWORD8 *pu1_est,
                    WORD32 src_strd,
                    WORD32 est_strd,
                    WORD32 i4_max_sad,
                    WORD32 *pi4_mb_distortion)
{
    __m128i src_r0, src_r1, src_r2, src_r3;
    __m128i est_r0, est_r1, est_r2, est_r3;
    __m128i res_r0, res_r1, res_r2, res_r3;
    __m128i sad_val;
    int val1, val2;
    UNUSED (i4_max_sad);

    // Row 0-3 sad calculation
    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 2*src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 3*src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 2*est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 3*est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(res_r0, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    // Row 4-7 sad calculation
    pu1_src += 4*src_strd;
    pu1_est += 4*est_strd;

    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 2*src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 3*src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 2*est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 3*est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(sad_val, res_r0);
    sad_val = _mm_add_epi64(sad_val, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    val1 = _mm_extract_epi32(sad_val,0);
    val2 = _mm_extract_epi32(sad_val, 2);
    *pi4_mb_distortion = (val1+val2);
    return;
}

/**
******************************************************************************
*
* @brief computes distortion (SAD) between 2 16x16 blocks
*
* @par   Description
*   This functions computes SAD between 2 16x16 blocks. There is a provision
*   for early exit if the up-to computed SAD exceeds maximum allowed SAD. To
*   compute the distortion of the entire block set u4_max_sad to USHRT_MAX.
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
* @param[in] i4_max_sad
*  integer maximum allowed distortion
*
* @param[out] pi4_mb_distortion
*  integer evaluated sad
*
* @remarks
*
******************************************************************************
*/
void ime_compute_sad_16x16_ea8_sse42(UWORD8 *pu1_src,
                               UWORD8 *pu1_est,
                               WORD32 src_strd,
                               WORD32 est_strd,
                               WORD32 i4_max_sad,
                               WORD32 *pi4_mb_distortion)
{
    __m128i src_r0, src_r1, src_r2, src_r3;
    __m128i est_r0, est_r1, est_r2, est_r3;
    __m128i res_r0, res_r1, res_r2, res_r3;
    __m128i sad_val;
    WORD32 val1, val2;
    WORD32 i4_sad;
    UWORD8 *pu1_src_temp = pu1_src + src_strd;
    UWORD8 *pu1_est_temp = pu1_est + est_strd;

    // Row 0,2,4,6 sad calculation
    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + 2*src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 4*src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 6*src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + 2*est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 4*est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 6*est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(res_r0, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    // Row 8,10,12,14 sad calculation
    pu1_src += 8*src_strd;
    pu1_est += 8*est_strd;

    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + 2*src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 4*src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 6*src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + 2*est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 4*est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 6*est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(sad_val, res_r0);
    sad_val = _mm_add_epi64(sad_val, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    pu1_src = pu1_src_temp;
    pu1_est = pu1_est_temp;

    val1 = _mm_extract_epi32(sad_val, 0);
    val2 = _mm_extract_epi32(sad_val, 2);

    i4_sad = val1 + val2;
    if (i4_max_sad < i4_sad)
    {
        *pi4_mb_distortion = i4_sad;
        return ;
    }
    // Row 1,3,5,7 sad calculation
    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + 2*src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 4*src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 6*src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + 2*est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 4*est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 6*est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(sad_val, res_r0);
    sad_val = _mm_add_epi64(sad_val, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    // Row 9,11,13,15 sad calculation
    pu1_src += 8*src_strd;
    pu1_est += 8*est_strd;
    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + 2*src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 4*src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 6*src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + 2*est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 4*est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 6*est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(sad_val, res_r0);
    sad_val = _mm_add_epi64(sad_val, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    val1 = _mm_extract_epi32(sad_val, 0);
    val2 = _mm_extract_epi32(sad_val, 2);
    *pi4_mb_distortion = (val1+val2);

    return;
}

/**
******************************************************************************
*
* @brief computes distortion (SAD) between 2 16x16 blocks (fast mode)
*
* @par   Description
*   This functions computes SAD between 2 16x16 blocks by processing alternate
*   rows (fast mode). For fast mode it is assumed sad obtained by processing
*   alternate rows is approximately twice as that for the whole block.
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
* @param[in] i4_max_sad
*  integer maximum allowed distortion
*
* @param[out] pi4_mb_distortion
*  integer evaluated sad
*
* @remarks
*
******************************************************************************
*/
void ime_compute_sad_16x16_fast_sse42(UWORD8 *pu1_src,
                                UWORD8 *pu1_est,
                                WORD32 src_strd,
                                WORD32 est_strd,
                                WORD32 i4_max_sad,
                                WORD32 *pi4_mb_distortion)
{
    __m128i src_r0, src_r1, src_r2, src_r3;
    __m128i est_r0, est_r1, est_r2, est_r3;
    __m128i res_r0, res_r1, res_r2, res_r3;
    __m128i sad_val;
    WORD32 val1, val2;
    WORD32 i4_sad;
    UWORD8 *pu1_src_temp = pu1_src + src_strd;
    UWORD8 *pu1_est_temp = pu1_est + est_strd;
    UNUSED (i4_max_sad);

    // Row 0,2,4,6 sad calculation
    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + 2 * src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 4 * src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 6 * src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + 2 * est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 4 * est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 6 * est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(res_r0, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    // Row 8,10,12,14 sad calculation
    pu1_src += 8 * src_strd;
    pu1_est += 8 * est_strd;

    src_r0 = _mm_loadu_si128((__m128i *) (pu1_src));
    src_r1 = _mm_loadu_si128((__m128i *) (pu1_src + 2 * src_strd));
    src_r2 = _mm_loadu_si128((__m128i *) (pu1_src + 4 * src_strd));
    src_r3 = _mm_loadu_si128((__m128i *) (pu1_src + 6 * src_strd));

    est_r0 = _mm_loadu_si128((__m128i *) (pu1_est));
    est_r1 = _mm_loadu_si128((__m128i *) (pu1_est + 2 * est_strd));
    est_r2 = _mm_loadu_si128((__m128i *) (pu1_est + 4 * est_strd));
    est_r3 = _mm_loadu_si128((__m128i *) (pu1_est + 6 * est_strd));

    res_r0 = _mm_sad_epu8(src_r0, est_r0);
    res_r1 = _mm_sad_epu8(src_r1, est_r1);
    res_r2 = _mm_sad_epu8(src_r2, est_r2);
    res_r3 = _mm_sad_epu8(src_r3, est_r3);

    sad_val = _mm_add_epi64(sad_val, res_r0);
    sad_val = _mm_add_epi64(sad_val, res_r1);
    sad_val = _mm_add_epi64(sad_val, res_r2);
    sad_val = _mm_add_epi64(sad_val, res_r3);

    pu1_src = pu1_src_temp;
    pu1_est = pu1_est_temp;

    val1 = _mm_extract_epi32(sad_val, 0);
    val2 = _mm_extract_epi32(sad_val, 2);

    i4_sad = val1 + val2;
    *pi4_mb_distortion = (i4_sad<<1);
    return;
}

/**
*******************************************************************************
*
* @brief compute sad
*
* @par Description: This function computes the sad at vertices of diamond grid
* centered at reference pointer and at unit distance from it.
*
* @param[in] pu1_ref
*  UWORD8 pointer to the reference
*
* @param[out] pu1_src
*  UWORD8 pointer to the source
*
* @param[in] ref_strd
*  integer reference stride
*
* @param[in] src_strd
*  integer source stride
*
* @param[out] pi4_sad
*  pointer to integer array evaluated sad
*
* @returns  sad at all evaluated vertexes
*
* @remarks  none
*
*******************************************************************************
*/
void ime_calculate_sad4_prog_sse42(UWORD8 *pu1_ref,
                             UWORD8 *pu1_src,
                             WORD32 ref_strd,
                             WORD32 src_strd,
                             WORD32 *pi4_sad)
{
    /* reference ptrs at unit 1 distance in diamond pattern centered at pu1_ref */
    UWORD8 *left_ptr    = pu1_ref - 1;
    UWORD8 *right_ptr   = pu1_ref + 1;
    UWORD8 *top_ptr     = pu1_ref - ref_strd;
    UWORD8 *bot_ptr     = pu1_ref + ref_strd;

    WORD32 val1, val2;
    __m128i src, ref_left, ref_right, ref_top, ref_bot;
    __m128i res_r0, res_r1, res_r2, res_r3;
    __m128i sad_r0, sad_r1, sad_r2, sad_r3;

    // Row 0 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    sad_r0 = _mm_sad_epu8(src, ref_left);
    sad_r1 = _mm_sad_epu8(src, ref_right);
    sad_r2 = _mm_sad_epu8(src, ref_top);
    sad_r3 = _mm_sad_epu8(src, ref_bot);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 1 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 2 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 3 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 4 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 5 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 6 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 7 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 8 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 9 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 10 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 11 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 12 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 13 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 14 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    pu1_src += src_strd;
    left_ptr += ref_strd;
    right_ptr += ref_strd;
    top_ptr += ref_strd;
    bot_ptr += ref_strd;

    // Row 15 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_left = _mm_loadu_si128((__m128i *) (left_ptr));
    ref_right = _mm_loadu_si128((__m128i *) (right_ptr));
    ref_top = _mm_loadu_si128((__m128i *) (top_ptr));
    ref_bot = _mm_loadu_si128((__m128i *) (bot_ptr));

    res_r0 = _mm_sad_epu8(src, ref_left);
    res_r1 = _mm_sad_epu8(src, ref_right);
    res_r2 = _mm_sad_epu8(src, ref_top);
    res_r3 = _mm_sad_epu8(src, ref_bot);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);

    val1 = _mm_extract_epi32(sad_r0, 0);
    val2 = _mm_extract_epi32(sad_r0, 2);
    pi4_sad[0] = (val1 + val2);

    val1 = _mm_extract_epi32(sad_r1, 0);
    val2 = _mm_extract_epi32(sad_r1, 2);
    pi4_sad[1] = (val1 + val2);

    val1 = _mm_extract_epi32(sad_r2, 0);
    val2 = _mm_extract_epi32(sad_r2, 2);
    pi4_sad[2] = (val1 + val2);

    val1 = _mm_extract_epi32(sad_r3, 0);
    val2 = _mm_extract_epi32(sad_r3, 2);
    pi4_sad[3] = (val1 + val2);
}

/**
******************************************************************************
*
* @brief computes distortion (SAD) at all subpel points about the src location
*
* @par Description
*   This functions computes SAD at all points at a subpel distance from the
*   current source location.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[out] pu1_ref_half_x
*  UWORD8 pointer to half pel buffer
*
* @param[out] pu1_ref_half_y
*  UWORD8 pointer to half pel buffer
*
* @param[out] pu1_ref_half_xy
*  UWORD8 pointer to half pel buffer
*
* @param[in] src_strd
*  integer source stride
*
* @param[in] ref_strd
*  integer ref stride
*
* @param[out] pi4_sad
*  integer evaluated sad
*  pi4_sad[0] - half x
*  pi4_sad[1] - half x - 1
*  pi4_sad[2] - half y
*  pi4_sad[3] - half y - 1
*  pi4_sad[4] - half xy
*  pi4_sad[5] - half xy - 1
*  pi4_sad[6] - half xy - strd
*  pi4_sad[7] - half xy - 1 - strd
*
* @remarks
*
******************************************************************************
*/
void ime_sub_pel_compute_sad_16x16_sse42(UWORD8 *pu1_src,
                                   UWORD8 *pu1_ref_half_x,
                                   UWORD8 *pu1_ref_half_y,
                                   UWORD8 *pu1_ref_half_xy,
                                   WORD32 src_strd,
                                   WORD32 ref_strd,
                                   WORD32 *pi4_sad)
{
    UWORD8 *pu1_ref_half_x_left = pu1_ref_half_x - 1;
    UWORD8 *pu1_ref_half_y_top = pu1_ref_half_y - ref_strd;
    UWORD8 *pu1_ref_half_xy_left = pu1_ref_half_xy - 1;
    UWORD8 *pu1_ref_half_xy_top = pu1_ref_half_xy - ref_strd;
    UWORD8 *pu1_ref_half_xy_top_left = pu1_ref_half_xy - ref_strd - 1;
    WORD32 val1, val2;

    __m128i src, ref_half_x, ref_half_y, ref_half_xy;
    __m128i ref_half_x_left, ref_half_y_top, ref_half_xy_left, ref_half_xy_top, ref_half_xy_top_left;
    __m128i res_r0, res_r1, res_r2, res_r3, res_r4, res_r5, res_r6, res_r7;
    __m128i sad_r0, sad_r1, sad_r2, sad_r3, sad_r4, sad_r5, sad_r6, sad_r7;
    // Row 0 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    sad_r0 = _mm_sad_epu8(src, ref_half_x);
    sad_r1 = _mm_sad_epu8(src, ref_half_x_left);
    sad_r2 = _mm_sad_epu8(src, ref_half_y);
    sad_r3 = _mm_sad_epu8(src, ref_half_y_top);
    sad_r4 = _mm_sad_epu8(src, ref_half_xy);
    sad_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    sad_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    sad_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 1 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 2 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 3 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 4 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;


    // Row 5 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 6 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 7 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 8 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 9 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 10 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 11 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 12 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 13 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 14 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    pu1_src += src_strd;
    pu1_ref_half_x += ref_strd;
    pu1_ref_half_x_left += ref_strd;
    pu1_ref_half_y += ref_strd;
    pu1_ref_half_y_top += ref_strd;
    pu1_ref_half_xy += ref_strd;
    pu1_ref_half_xy_left += ref_strd;
    pu1_ref_half_xy_top += ref_strd;
    pu1_ref_half_xy_top_left += ref_strd;

    // Row 15 sad calculation
    src = _mm_loadu_si128((__m128i *) (pu1_src));
    ref_half_x = _mm_loadu_si128((__m128i *) (pu1_ref_half_x));
    ref_half_y = _mm_loadu_si128((__m128i *) (pu1_ref_half_y));
    ref_half_xy = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy));
    ref_half_x_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_x_left));
    ref_half_y_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_y_top));
    ref_half_xy_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_left));
    ref_half_xy_top = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top));
    ref_half_xy_top_left = _mm_loadu_si128((__m128i *) (pu1_ref_half_xy_top_left));

    res_r0 = _mm_sad_epu8(src, ref_half_x);
    res_r1 = _mm_sad_epu8(src, ref_half_x_left);
    res_r2 = _mm_sad_epu8(src, ref_half_y);
    res_r3 = _mm_sad_epu8(src, ref_half_y_top);
    res_r4 = _mm_sad_epu8(src, ref_half_xy);
    res_r5 = _mm_sad_epu8(src, ref_half_xy_left);
    res_r6 = _mm_sad_epu8(src, ref_half_xy_top);
    res_r7 = _mm_sad_epu8(src, ref_half_xy_top_left);

    sad_r0 = _mm_add_epi64(sad_r0, res_r0);
    sad_r1 = _mm_add_epi64(sad_r1, res_r1);
    sad_r2 = _mm_add_epi64(sad_r2, res_r2);
    sad_r3 = _mm_add_epi64(sad_r3, res_r3);
    sad_r4 = _mm_add_epi64(sad_r4, res_r4);
    sad_r5 = _mm_add_epi64(sad_r5, res_r5);
    sad_r6 = _mm_add_epi64(sad_r6, res_r6);
    sad_r7 = _mm_add_epi64(sad_r7, res_r7);

    val1 = _mm_extract_epi32(sad_r0, 0);
    val2 = _mm_extract_epi32(sad_r0, 2);
    pi4_sad[0] = (val1 + val2);

    val1 = _mm_extract_epi32(sad_r1, 0);
    val2 = _mm_extract_epi32(sad_r1, 2);
    pi4_sad[1] = (val1 + val2);

    val1 = _mm_extract_epi32(sad_r2, 0);
    val2 = _mm_extract_epi32(sad_r2, 2);
    pi4_sad[2] = (val1 + val2);

    val1 = _mm_extract_epi32(sad_r3, 0);
    val2 = _mm_extract_epi32(sad_r3, 2);
    pi4_sad[3] = (val1 + val2);

    val1 = _mm_extract_epi32(sad_r4, 0);
    val2 = _mm_extract_epi32(sad_r4, 2);
    pi4_sad[4] = (val1 + val2);

    val1 = _mm_extract_epi32(sad_r5, 0);
    val2 = _mm_extract_epi32(sad_r5, 2);
    pi4_sad[5] = (val1 + val2);

    val1 = _mm_extract_epi32(sad_r6, 0);
    val2 = _mm_extract_epi32(sad_r6, 2);
    pi4_sad[6] = (val1 + val2);

    val1 = _mm_extract_epi32(sad_r7, 0);
    val2 = _mm_extract_epi32(sad_r7, 2);
    pi4_sad[7] = (val1 + val2);

    return;
}
/*
*
* @brief This function computes SAD between two 16x16 blocks
*        It also computes if the block will be zero after H264 transform and quant for
*        Intra 16x16 blocks
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
* @param[in] pu2_thrsh
*  Threshold for each element of transofrmed quantized block
*
* @param[out] pi4_mb_distortion
*  integer evaluated sad
*
* @param[out] pu4_is_zero
*  Poitner to store if the block is zero after transform and quantization
*
* @remarks
*
******************************************************************************
*/
void ime_compute_satqd_16x16_lumainter_sse42(UWORD8 *pu1_src,
                                         UWORD8 *pu1_est,
                                         WORD32 src_strd,
                                         WORD32 est_strd,
                                         UWORD16 *pu2_thrsh,
                                         WORD32 *pi4_mb_distortion,
                                         UWORD32 *pu4_is_zero)
{
    __m128i src_r0, src_r1, src_r2, src_r3;
    __m128i est_r0, est_r1, est_r2, est_r3;
    __m128i temp0, temp1, temp2, temp3, temp4;
    __m128i zero = _mm_setzero_si128();          // all bits reset to zero
    __m128i all_one = _mm_set1_epi8(0xFF);
    __m128i sad_b1, sad_b2, threshold;
    WORD16 sad_1, sad_2;
    WORD32 i;
    UWORD32 flag = 0;
    WORD32 test1, test2;
    threshold = _mm_loadu_si128((__m128i *) pu2_thrsh);
    (*pi4_mb_distortion) = 0;

    for (i=0; i<4; i++)
    {
        src_r0 = _mm_loadl_epi64((__m128i *) pu1_src);  //Row 0 - Block1 and 2
        src_r1 = _mm_loadl_epi64((__m128i *) (pu1_src + src_strd)); //Row 1 - Block1 and 2
        src_r2 = _mm_loadl_epi64((__m128i *) (pu1_src + 2 * src_strd)); //Row 2 - Block1 and 2
        src_r3 = _mm_loadl_epi64((__m128i *) (pu1_src + 3 * src_strd)); //Row 3 - Block1 and 2

        src_r0 = _mm_cvtepu8_epi16(src_r0);
        src_r1 = _mm_cvtepu8_epi16(src_r1);
        src_r2 = _mm_cvtepu8_epi16(src_r2);
        src_r3 = _mm_cvtepu8_epi16(src_r3);

        est_r0 = _mm_loadl_epi64((__m128i *) pu1_est);
        est_r1 = _mm_loadl_epi64((__m128i *) (pu1_est + est_strd));
        est_r2 = _mm_loadl_epi64((__m128i *) (pu1_est + 2 * est_strd));
        est_r3 = _mm_loadl_epi64((__m128i *) (pu1_est + 3 * est_strd));

        est_r0 = _mm_cvtepu8_epi16(est_r0);
        est_r1 = _mm_cvtepu8_epi16(est_r1);
        est_r2 = _mm_cvtepu8_epi16(est_r2);
        est_r3 = _mm_cvtepu8_epi16(est_r3);

        src_r0 = _mm_sub_epi16(src_r0, est_r0);
        src_r1 = _mm_sub_epi16(src_r1, est_r1);
        src_r2 = _mm_sub_epi16(src_r2, est_r2);
        src_r3 = _mm_sub_epi16(src_r3, est_r3);

        src_r0 = _mm_abs_epi16(src_r0);
        src_r1 = _mm_abs_epi16(src_r1);
        src_r2 = _mm_abs_epi16(src_r2);
        src_r3 = _mm_abs_epi16(src_r3);

        src_r0 = _mm_add_epi16(src_r0, src_r3);     //s1 s4 s4 s1 a1 a4 a4 a1
        src_r1 = _mm_add_epi16(src_r1, src_r2);     //s2 s3 s3 s2 a2 a3 a3 a2

        //SAD calculation
        temp0 = _mm_add_epi16(src_r0, src_r1);      //s1+s2 s4+s3 s4+s3 s1+s2 a1+a2 a4+a3 a4+a3 a1+a2
        temp0 = _mm_hadd_epi16(temp0, zero);
        temp0 = _mm_hadd_epi16(temp0, zero);        //sad1, sad2 - 16bit values

        sad_1 = _mm_extract_epi16(temp0, 0);
        sad_2 = _mm_extract_epi16(temp0, 1);

        (*pi4_mb_distortion) += sad_1 + sad_2;

        if (flag == 0) {
            sad_b1 = _mm_set1_epi16((sad_1 << 1));
            sad_b2 = _mm_set1_epi16((sad_2 << 1));

            src_r0 = _mm_shufflelo_epi16(src_r0, 0x9c); //Block 0 s1 s1 s4 s4 a1 a4 a4 a1
            src_r0 = _mm_shufflehi_epi16(src_r0, 0x9c); //Block 1 s1 s1 s4 s4 a1 a1 a4 a4

            src_r1 = _mm_shufflelo_epi16(src_r1, 0x9c); //Block 0 s2 s2 s3 s3 a2 a3 a3 a2
            src_r1 = _mm_shufflehi_epi16(src_r1, 0x9c); //Block 1 s2 s2 s3 s3 a2 a2 a3 a3

            src_r0 = _mm_hadd_epi16(src_r0, zero);      //s1 s4 a1 a4 0 0 0 0
            src_r1 = _mm_hadd_epi16(src_r1, zero);      //s2 s3 a2 a3 0 0 0 0

            temp0 = _mm_slli_epi16(src_r0, 1);//s1<<1 s4<<1 a1<<1 a4<<1 0 0 0 0
            temp1 = _mm_slli_epi16(src_r1, 1);//s2<<1 s3<<1 a2<<1 a3<<1 0 0 0 0

            temp0 = _mm_shufflelo_epi16(temp0, 0xb1);//s4<<1 s1<<1 a4<<1 a1<<1 0 0 0 0
            temp1 = _mm_shufflelo_epi16(temp1, 0xb1);//s3<<1 s2<<1 a3<<1 a2<<1 0 0 0 0

            temp2 = _mm_sub_epi16(src_r0, temp1);//(s1-s3<<1) (s4-s2<<1) (a1-a3<<1) (a4-a2<<1) 0 0 0 0
            temp3 = _mm_sub_epi16(src_r1, temp0);//(s2-s4<<1) (s3-s1<<1) (a2-a4<<1) (a3-a1<<1) 0 0 0 0

            temp4 = _mm_add_epi16(src_r0, src_r1);//s1+s2 s4+s3 a1+a2 a4+a3 0 0 0 0

            temp0 = _mm_hadd_epi16(src_r0, zero);   //s1+s4 a1+a4 0 0 0 0 0 0
            temp1 = _mm_hadd_epi16(src_r1, zero);   //s2+s3 a2+a3 0 0 0 0 0 0

            temp0 = _mm_unpacklo_epi16(temp0, temp1);//s1+s4 s2+s3 a1+a4 a2+a3 0 0 0 0

            temp0 = _mm_unpacklo_epi32(temp0, temp2);//s1+s4 s2+s3 (s1-s3<<1) (s4-s2<<1) a1+a4 a2+a3 (a1-a3<<1) (a4-a2<<1)
            temp1 = _mm_unpacklo_epi32(temp4, temp3);//s1+s2 s4+s3 (s2-s4<<1) (s3-s1<<1) a1+a2 a4+a3 (a2-a4<<1) (a3-a1<<1)

            temp2 = _mm_unpacklo_epi64(temp0, temp1);//s1+s4 s2+s3 (s1-s3<<1) (s4-s2<<1) s1+s2 s4+s3 (s2-s4<<1) (s3-s1<<1)
            temp3 = _mm_unpackhi_epi64(temp0, temp1); //a1+a4 a2+a3 (a1-a3<<1) (a4-a2<<1) a1+a2 a4+a3 (s2-s4<<1) (s3-s1<<1)

            sad_b1 = _mm_sub_epi16(sad_b1, temp2);      //lsi values Block0
            sad_b2 = _mm_sub_epi16(sad_b2, temp3);      //lsi values Block1

            temp0 = _mm_cmpgt_epi16(threshold, sad_b1); //if any threshold[i]>ls[i], corresponding 16-bit value in temp becomes 0xffff

            temp1 = _mm_cmpgt_epi16(threshold, sad_b2);

            temp0 = _mm_xor_si128(temp0, all_one);      //Xor with 1 => NOT operation
            temp1 = _mm_xor_si128(temp1, all_one);

            test1 = _mm_test_all_zeros(temp0, all_one);
            test2 = _mm_test_all_zeros(temp1, all_one);

            if (test1 == 0 || test2 == 0 || pu2_thrsh[8] <= sad_1
                    || pu2_thrsh[8] <= sad_2)
                flag = 1;
        }

        pu1_src += 8;
        pu1_est += 8;

        src_r0 = _mm_loadl_epi64((__m128i *) pu1_src);  //Row 0 - Block1 and 2
        src_r1 = _mm_loadl_epi64((__m128i *) (pu1_src + src_strd)); //Row 1 - Block1 and 2
        src_r2 = _mm_loadl_epi64((__m128i *) (pu1_src + 2 * src_strd)); //Row 2 - Block1 and 2
        src_r3 = _mm_loadl_epi64((__m128i *) (pu1_src + 3 * src_strd)); //Row 3 - Block1 and 2

        src_r0 = _mm_cvtepu8_epi16(src_r0);
        src_r1 = _mm_cvtepu8_epi16(src_r1);
        src_r2 = _mm_cvtepu8_epi16(src_r2);
        src_r3 = _mm_cvtepu8_epi16(src_r3);

        est_r0 = _mm_loadl_epi64((__m128i *) pu1_est);
        est_r1 = _mm_loadl_epi64((__m128i *) (pu1_est + est_strd));
        est_r2 = _mm_loadl_epi64((__m128i *) (pu1_est + 2 * est_strd));
        est_r3 = _mm_loadl_epi64((__m128i *) (pu1_est + 3 * est_strd));

        est_r0 = _mm_cvtepu8_epi16(est_r0);
        est_r1 = _mm_cvtepu8_epi16(est_r1);
        est_r2 = _mm_cvtepu8_epi16(est_r2);
        est_r3 = _mm_cvtepu8_epi16(est_r3);

        src_r0 = _mm_sub_epi16(src_r0, est_r0);
        src_r1 = _mm_sub_epi16(src_r1, est_r1);
        src_r2 = _mm_sub_epi16(src_r2, est_r2);
        src_r3 = _mm_sub_epi16(src_r3, est_r3);

        src_r0 = _mm_abs_epi16(src_r0);
        src_r1 = _mm_abs_epi16(src_r1);
        src_r2 = _mm_abs_epi16(src_r2);
        src_r3 = _mm_abs_epi16(src_r3);

        src_r0 = _mm_add_epi16(src_r0, src_r3);     //s1 s4 s4 s1 a1 a4 a4 a1
        src_r1 = _mm_add_epi16(src_r1, src_r2);     //s2 s3 s3 s2 a2 a3 a3 a2

        //SAD calculation
        temp0 = _mm_add_epi16(src_r0, src_r1);
        temp0 = _mm_hadd_epi16(temp0, zero);
        temp0 = _mm_hadd_epi16(temp0, zero);        //sad1, sad2 - 16bit values

        sad_1 = _mm_extract_epi16(temp0, 0);
        sad_2 = _mm_extract_epi16(temp0, 1);

        (*pi4_mb_distortion) += sad_1 + sad_2;

        if (flag == 0) {
            sad_b1 = _mm_set1_epi16((sad_1 << 1));
            sad_b2 = _mm_set1_epi16((sad_2 << 1));

            src_r0 = _mm_shufflelo_epi16(src_r0, 0x9c); //Block 0 s1 s1 s4 s4 a1 a4 a4 a1
            src_r0 = _mm_shufflehi_epi16(src_r0, 0x9c); //Block 1 s1 s1 s4 s4 a1 a1 a4 a4

            src_r1 = _mm_shufflelo_epi16(src_r1, 0x9c); //Block 0 s2 s2 s3 s3 a2 a3 a3 a2
            src_r1 = _mm_shufflehi_epi16(src_r1, 0x9c); //Block 1 s2 s2 s3 s3 a2 a2 a3 a3

            src_r0 = _mm_hadd_epi16(src_r0, zero);      //s1 s4 a1 a4 0 0 0 0
            src_r1 = _mm_hadd_epi16(src_r1, zero);      //s2 s3 a2 a3 0 0 0 0

            temp0 = _mm_slli_epi16(src_r0, 1);//s1<<1 s4<<1 a1<<1 a4<<1 0 0 0 0
            temp1 = _mm_slli_epi16(src_r1, 1);//s2<<1 s3<<1 a2<<1 a3<<1 0 0 0 0

            temp0 = _mm_shufflelo_epi16(temp0, 0xb1);//s4<<1 s1<<1 a4<<1 a1<<1 0 0 0 0
            temp1 = _mm_shufflelo_epi16(temp1, 0xb1);//s3<<1 s2<<1 a3<<1 a2<<1 0 0 0 0

            temp2 = _mm_sub_epi16(src_r0, temp1);//(s1-s3<<1) (s4-s2<<1) (a1-a3<<1) (a4-a2<<1) 0 0 0 0
            temp3 = _mm_sub_epi16(src_r1, temp0);//(s2-s4<<1) (s3-s1<<1) (a2-a4<<1) (a3-a1<<1) 0 0 0 0

            temp4 = _mm_add_epi16(src_r0, src_r1);//s1+s2 s4+s3 a1+a2 a4+a3 0 0 0 0

            temp0 = _mm_hadd_epi16(src_r0, zero);   //s1+s4 a1+a4 0 0 0 0 0 0
            temp1 = _mm_hadd_epi16(src_r1, zero);   //s2+s3 a2+a3 0 0 0 0 0 0

            temp0 = _mm_unpacklo_epi16(temp0, temp1);//s1+s4 s2+s3 a1+a4 a2+a3 0 0 0 0

            temp0 = _mm_unpacklo_epi32(temp0, temp2);//s1+s4 s2+s3 (s1-s3<<1) (s4-s2<<1) a1+a4 a2+a3 (a1-a3<<1) (a4-a2<<1)
            temp1 = _mm_unpacklo_epi32(temp4, temp3);//s1+s2 s4+s3 (s2-s4<<1) (s3-s1<<1) a1+a2 a4+a3 (a2-a4<<1) (a3-a1<<1)

            temp2 = _mm_unpacklo_epi64(temp0, temp1);//s1+s4 s2+s3 (s1-s3<<1) (s4-s2<<1) s1+s2 s4+s3 (s2-s4<<1) (s3-s1<<1)
            temp3 = _mm_unpackhi_epi64(temp0, temp1); //a1+a4 a2+a3 (a1-a3<<1) (a4-a2<<1) a1+a2 a4+a3 (s2-s4<<1) (s3-s1<<1)

            sad_b1 = _mm_sub_epi16(sad_b1, temp2);      //lsi values Block0
            sad_b2 = _mm_sub_epi16(sad_b2, temp3);      //lsi values Block1

            temp0 = _mm_cmpgt_epi16(threshold, sad_b1); //if any threshold[i]>ls[i], corresponding 16-bit value in temp becomes 0xffff

            temp1 = _mm_cmpgt_epi16(threshold, sad_b2);

            temp0 = _mm_xor_si128(temp0, all_one);      //Xor with 1 => NOT operation
            temp1 = _mm_xor_si128(temp1, all_one);

            test1 = _mm_test_all_zeros(temp0, all_one);
            test2 = _mm_test_all_zeros(temp1, all_one);

            if (test1 == 0 || test2 == 0 || pu2_thrsh[8] <= sad_1
                    || pu2_thrsh[8] <= sad_2)
                flag = 1;
        }

        pu1_src += 4*src_strd - 8;
        pu1_est += 4*est_strd - 8;
    }

        *pu4_is_zero = flag;
}
