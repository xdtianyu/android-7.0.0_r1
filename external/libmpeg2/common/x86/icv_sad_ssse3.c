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
*  icv_sad.c
*
* @brief
*  This file contains the functions to compute SAD
*
* @author
*  Ittiam
*
* @par List of Functions:
*  icv_sad_8x4_ssse3()
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
*  Compute 8x4 SAD
*
* @par   Description
*  Compute 8x4 sum of absolute differences between source and reference block
*
* @param[in] pu1_src
*  Source buffer
*
* @param[in] pu1_ref
*  Reference buffer
*
* @param[in] src_strd
*  Source stride
*
* @param[in] ref_strd
*  Reference stride
*
* @param[in] wd
*  Assumed to be 8
*
* @param[in] ht
*  Assumed to be 4

* @returns
*  SAD
*
* @remarks
*
*******************************************************************************
*/
WORD32 icv_sad_8x4_ssse3(UWORD8 *pu1_src,
                         UWORD8 *pu1_ref,
                         WORD32 src_strd,
                         WORD32 ref_strd,
                         WORD32 wd,
                         WORD32 ht)
{
    WORD32 sad;
    __m128 src_r0, src_r1;
    __m128 ref_r0, ref_r1;
    __m128i res_r0, res_r1;

    UNUSED(wd);
    UNUSED(ht);
    ASSERT(wd == 8);
    ASSERT(ht == 4);

    /* Load source */
    src_r0 = (__m128)_mm_loadl_epi64((__m128i *) (pu1_src));
    pu1_src += src_strd;

    src_r1 = (__m128)_mm_loadl_epi64((__m128i *) (pu1_src));
    pu1_src += src_strd;

    src_r0 = _mm_loadh_pi (src_r0, (__m64 *) (pu1_src));
    pu1_src += src_strd;

    src_r1 = _mm_loadh_pi (src_r1, (__m64 *) (pu1_src));
    pu1_src += src_strd;


    /* Load reference */
    ref_r0 = (__m128)_mm_loadl_epi64((__m128i *) (pu1_ref));
    pu1_ref += ref_strd;

    ref_r1 = (__m128)_mm_loadl_epi64((__m128i *) (pu1_ref));
    pu1_ref += ref_strd;

    ref_r0 = _mm_loadh_pi (ref_r0, (__m64 *) (pu1_ref));
    pu1_ref += ref_strd;

    ref_r1 = _mm_loadh_pi (ref_r1, (__m64 *) (pu1_ref));
    pu1_ref += ref_strd;

    /* Compute SAD for each row */
    res_r0 = _mm_sad_epu8((__m128i)src_r0, (__m128i)ref_r0);
    res_r1 = _mm_sad_epu8((__m128i)src_r1, (__m128i)ref_r1);

    /* Accumulate SAD */
    res_r0 = _mm_add_epi64(res_r0,  res_r1);
    res_r0 = _mm_add_epi64(res_r0, _mm_srli_si128(res_r0, 8));

    sad  = _mm_cvtsi128_si32(res_r0);

    return sad;
}
