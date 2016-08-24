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
 *  impeg2_mem_func_sse42_intr.c
 *
 * @brief
 *  Contains utility function definitions for MPEG2 codec
 *
 * @author
 *  Mohit [100664]
 *
* @par List of Functions:
* - impeg2_memset0_16bit_8x8_linear_block_sse42()
* - impeg2_memset_8bit_8x8_block_sse42()
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include "iv_datatypedef.h"
#include "impeg2_defs.h"

#include <immintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>

/*******************************************************************************
*  Function Name   : impeg2_memset0_16bit_8x8_linear_block
*
*  Description     : memsets resudial buf to 0
*
*  Arguments       : destination buffer
*
*  Values Returned : None
*******************************************************************************/


void impeg2_memset0_16bit_8x8_linear_block_sse42 (WORD16 *buf)
 {
    __m128i zero_8x8_16b = _mm_set1_epi16(0);
    _mm_storeu_si128((__m128i *) buf, zero_8x8_16b);
    _mm_storeu_si128((__m128i *) (buf + 8), zero_8x8_16b);
    _mm_storeu_si128((__m128i *) (buf + 16), zero_8x8_16b);
    _mm_storeu_si128((__m128i *) (buf + 24), zero_8x8_16b);
    _mm_storeu_si128((__m128i *) (buf + 32), zero_8x8_16b);
    _mm_storeu_si128((__m128i *) (buf + 40), zero_8x8_16b);
    _mm_storeu_si128((__m128i *) (buf + 48), zero_8x8_16b);
    _mm_storeu_si128((__m128i *) (buf + 56), zero_8x8_16b);
}



/*******************************************************************************
*  Function Name   : impeg2_memset_8bit_8x8_block
*
*  Description     : memsets residual buf to value
*
*  Arguments       : destination buffer, value and stride
*
*  Values Returned : None
*******************************************************************************/


void impeg2_memset_8bit_8x8_block_sse42(UWORD8 *dst, WORD32 dc_val, WORD32 dst_wd)
{
    __m128i value = _mm_set1_epi8((WORD8)dc_val);

    _mm_storel_epi64((__m128i *)dst, value);
    _mm_storel_epi64((__m128i *) (dst + dst_wd), value);
    _mm_storel_epi64((__m128i *) (dst + 2 * dst_wd), value);
    _mm_storel_epi64((__m128i *) (dst + 3 * dst_wd), value);
    _mm_storel_epi64((__m128i *) (dst + 4 * dst_wd), value);
    _mm_storel_epi64((__m128i *) (dst + 5 * dst_wd), value);
    _mm_storel_epi64((__m128i *) (dst + 6 * dst_wd), value);
    _mm_storel_epi64((__m128i *) (dst + 7 * dst_wd), value);
}
