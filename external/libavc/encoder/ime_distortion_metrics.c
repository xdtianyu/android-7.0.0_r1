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
* @file ih264e_distortion_metrics.c
*
* @brief
*  This file contains definitions of routines that compute distortion
*  between two macro/sub blocks of identical dimensions
*
* @author
*  Ittiam
*
* @par List of Functions:
*  - ime_sub_pel_compute_sad_16x16()
*  - ime_calculate_sad4_prog()
*  - ime_calculate_sad3_prog()
*  - ime_calculate_sad2_prog()
*  - ime_compute_sad_16x16()
*  - ime_compute_sad_16x16_fast()
*  - ime_compute_sad_16x16_ea8()
*  - ime_compute_sad_8x8()
*  - ime_compute_sad_4x4()
*  - ime_compute_sad_16x8()
*  - ime_compute_satqd_16x16_lumainter()
*  - ime_compute_satqd_8x16_chroma()
*  - ime_compute_satqd_16x16_lumaintra()
*
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


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

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
void ime_sub_pel_compute_sad_16x16(UWORD8 *pu1_src,
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

    WORD32 row, col;

    memset(pi4_sad, 0, 8 * sizeof(WORD32));

    for(row = 0; row < MB_SIZE; row++)
    {
        for(col = 0; col < MB_SIZE; col++)
        {
            WORD32 src;
            WORD32 diff;

            src = pu1_src[col];

            diff = src - pu1_ref_half_x[col];
            pi4_sad[0] += ABS(diff);

            diff = src - pu1_ref_half_x_left[col];
            pi4_sad[1] += ABS(diff);

            diff = src - pu1_ref_half_y[col];
            pi4_sad[2] += ABS(diff);

            diff = src - pu1_ref_half_y_top[col];
            pi4_sad[3] += ABS(diff);

            diff = src - pu1_ref_half_xy[col];
            pi4_sad[4] += ABS(diff);

            diff = src - pu1_ref_half_xy_left[col];
            pi4_sad[5] += ABS(diff);

            diff = src - pu1_ref_half_xy_top[col];
            pi4_sad[6] += ABS(diff);

            diff = src - pu1_ref_half_xy_top_left[col];
            pi4_sad[7] += ABS(diff);
        }

        pu1_src += src_strd;

        pu1_ref_half_x += ref_strd;
        pu1_ref_half_x_left += ref_strd;

        pu1_ref_half_y += ref_strd;
        pu1_ref_half_y_top += ref_strd;

        pu1_ref_half_xy += ref_strd;
        pu1_ref_half_xy_left += ref_strd;
        pu1_ref_half_xy_top += ref_strd;
        pu1_ref_half_xy_top_left += ref_strd;
    }
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
void ime_calculate_sad4_prog(UWORD8 *pu1_ref,
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

    /* temp var */
    WORD32 count2, count3;
    UWORD32 u4_ref_buf_offset = ref_strd - MB_SIZE;
    UWORD32 u4_cur_buf_offset = src_strd - MB_SIZE;

    memset(pi4_sad, 0, 4 * sizeof(WORD32));

    for(count2 = MB_SIZE; count2 > 0; count2--)
    {
        for(count3 = MB_SIZE; count3 > 0 ; count3--)
        {
            WORD32 src;
            WORD32 diff;

            src = *pu1_src++;

            diff = src - *left_ptr++;
            pi4_sad[0] += ABS(diff);

            diff = src - *right_ptr++;
            pi4_sad[1] += ABS(diff);

            diff = src - *top_ptr++;
            pi4_sad[2] += ABS(diff);

            diff = src - *bot_ptr++;
            pi4_sad[3]  += ABS(diff);
        }

        bot_ptr    += u4_ref_buf_offset;
        left_ptr   += u4_ref_buf_offset;
        right_ptr  += u4_ref_buf_offset;
        top_ptr    += u4_ref_buf_offset;

        pu1_src += u4_cur_buf_offset;
    }

}

/**
*******************************************************************************
*
* @brief compute sad
*
* @par Description: This function computes the sad at vertices of diamond grid
* centered at reference pointer and at unit distance from it.
*
* @param[in] pu1_ref1, pu1_ref2, pu1_ref3
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
void ime_calculate_sad3_prog(UWORD8 *pu1_ref1,
                             UWORD8 *pu1_ref2,
                             UWORD8 *pu1_ref3,
                             UWORD8 *pu1_src,
                             WORD32 ref_strd,
                             WORD32 src_strd,
                             WORD32 *pi4_sad)
{
    /* temp var */
    WORD32 i;
    UWORD32 u4_ref_buf_offset = ref_strd - MB_SIZE;
    UWORD32 u4_cur_buf_offset = src_strd - MB_SIZE;

    for(i = 16; i > 0; i--)
    {
        USADA8(pu1_src, pu1_ref1, pi4_sad[0]);
        USADA8(pu1_src, pu1_ref2, pi4_sad[1]);
        USADA8(pu1_src, pu1_ref3, pi4_sad[2]);
        pu1_src += 4;
        pu1_ref1 += 4;
        pu1_ref2 += 4;
        pu1_ref3 += 4;

        USADA8(pu1_src, pu1_ref1, pi4_sad[0]);
        USADA8(pu1_src, pu1_ref2, pi4_sad[1]);
        USADA8(pu1_src, pu1_ref3, pi4_sad[2]);
        pu1_src += 4;
        pu1_ref1 += 4;
        pu1_ref2 += 4;
        pu1_ref3 += 4;

        USADA8(pu1_src, pu1_ref1, pi4_sad[0]);
        USADA8(pu1_src, pu1_ref2, pi4_sad[1]);
        USADA8(pu1_src, pu1_ref3, pi4_sad[2]);
        pu1_src += 4;
        pu1_ref1 += 4;
        pu1_ref2 += 4;
        pu1_ref3 += 4;

        USADA8(pu1_src, pu1_ref1, pi4_sad[0]);
        USADA8(pu1_src, pu1_ref2, pi4_sad[1]);
        USADA8(pu1_src, pu1_ref3, pi4_sad[2]);
        pu1_src += 4;
        pu1_ref1 += 4;
        pu1_ref2 += 4;
        pu1_ref3 += 4;

        pu1_src += u4_cur_buf_offset;
        pu1_ref1 += u4_ref_buf_offset;
        pu1_ref2 += u4_ref_buf_offset;
        pu1_ref3 += u4_ref_buf_offset;
    }

}

/**
*******************************************************************************
*
* @brief compute sad
*
* @par Description: This function computes the sad at vertices of diamond grid
* centered at reference pointer and at unit distance from it.
*
* @param[in] pu1_ref1, pu1_ref2
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
void ime_calculate_sad2_prog(UWORD8 *pu1_ref1,
                             UWORD8 *pu1_ref2,
                             UWORD8 *pu1_src,
                             WORD32 ref_strd,
                             WORD32 src_strd,
                             WORD32 *pi4_sad)
{
    /* temp var */
    WORD32 i;
    UWORD32 u4_ref_buf_offset = ref_strd - MB_SIZE;
    UWORD32 u4_cur_buf_offset = src_strd - MB_SIZE;

    for(i = 16; i > 0; i--)
    {
        USADA8(pu1_src, pu1_ref1, pi4_sad[0]);
        USADA8(pu1_src, pu1_ref2, pi4_sad[1]);
        pu1_src += 4;
        pu1_ref1 += 4;
        pu1_ref2 += 4;

        USADA8(pu1_src, pu1_ref1, pi4_sad[0]);
        USADA8(pu1_src, pu1_ref2, pi4_sad[1]);
        pu1_src += 4;
        pu1_ref1 += 4;
        pu1_ref2 += 4;

        USADA8(pu1_src, pu1_ref1, pi4_sad[0]);
        USADA8(pu1_src, pu1_ref2, pi4_sad[1]);
        pu1_src += 4;
        pu1_ref1 += 4;
        pu1_ref2 += 4;

        USADA8(pu1_src, pu1_ref1, pi4_sad[0]);
        USADA8(pu1_src, pu1_ref2, pi4_sad[1]);
        pu1_src += 4;
        pu1_ref1 += 4;
        pu1_ref2 += 4;

        pu1_src += u4_cur_buf_offset;
        pu1_ref1 += u4_ref_buf_offset;
        pu1_ref2 += u4_ref_buf_offset;
    }

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
void ime_compute_sad_16x16(UWORD8 *pu1_src,
                           UWORD8 *pu1_est,
                           WORD32 src_strd,
                           WORD32 est_strd,
                           WORD32 i4_max_sad,
                           WORD32 *pi4_mb_distortion)
{
    WORD32 i4_sad = 0;
    UWORD32 u4_src_offset = src_strd - 16;
    UWORD32 u4_est_offset = est_strd - 16;
    UWORD32 i;

GATHER_16x16_SAD_EE_STATS(gu4_16x16_sad_ee_stats, 16);

    for(i = 16; i > 0; i--)
    {
        USADA8(pu1_src, pu1_est, i4_sad);
        pu1_src += 4;
        pu1_est += 4;

        USADA8(pu1_src, pu1_est, i4_sad);
        pu1_src += 4;
        pu1_est += 4;

        USADA8(pu1_src, pu1_est, i4_sad);
        pu1_src += 4;
        pu1_est += 4;

        USADA8(pu1_src, pu1_est, i4_sad);
        pu1_src += 4;
        pu1_est += 4;

        /* early exit */
        if(i4_max_sad < i4_sad)
        {

GATHER_16x16_SAD_EE_STATS(gu4_16x16_sad_ee_stats, 16-i);

            *pi4_mb_distortion = i4_sad;
            return ;
        }
        pu1_src += u4_src_offset;
        pu1_est += u4_est_offset;
    }

    *pi4_mb_distortion = i4_sad;
    return ;
}

/**
******************************************************************************
*
* @brief computes distortion (SAD) between 2 16x16 blocks (fast mode)
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
void ime_compute_sad_16x16_fast(UWORD8 *pu1_src,
                                UWORD8 *pu1_est,
                                WORD32 src_strd,
                                WORD32 est_strd,
                                WORD32 i4_max_sad,
                                WORD32 *pi4_mb_distortion)
{

    WORD32 i4_sad = 0;
    UWORD32 u4_src_offset = 2 * src_strd - 16;
    UWORD32 u4_est_offset = 2 * est_strd - 16;
    UWORD32 i;

    UNUSED(i4_max_sad);

    for(i = 16; i > 0; i-= 2)
    {
        USADA8(pu1_src, pu1_est, i4_sad);
        pu1_src += 4;
        pu1_est += 4;

        USADA8(pu1_src, pu1_est, i4_sad);
        pu1_src += 4;
        pu1_est += 4;

        USADA8(pu1_src, pu1_est, i4_sad);
        pu1_src += 4;
        pu1_est += 4;

        USADA8(pu1_src, pu1_est, i4_sad);
        pu1_src += 4;
        pu1_est += 4;

        pu1_src += u4_src_offset;
        pu1_est += u4_est_offset;
    }

    *pi4_mb_distortion = (i4_sad << 1);
    return ;
}

/**
******************************************************************************
*
*  @brief computes distortion (SAD) between 2 8x8 blocks
*
*  @par   Description
*   This functions computes SAD between 2 8x8 blocks. There is a provision
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
* @param[out] i4_sad
*  integer evaluated sad
*
* @remarks
*
******************************************************************************
 */

void ime_compute_sad_8x8(UWORD8 *pu1_src,
                         UWORD8 *pu1_est,
                         WORD32 src_strd,
                         WORD32 est_strd,
                         WORD32 i4_max_sad,
                         WORD32 *pi4_mb_distortion)
{
    WORD32 i4_sad = 0;
    UWORD32 u4_src_offset = src_strd - 8;
    UWORD32 u4_est_offset = est_strd - 8;
    UWORD32 i, j;
    WORD16 temp;

    for(i = 8; i > 0; i--)
    {
        for(j = 8; j > 0; j--)
        {
            /* SAD */
            temp = *pu1_src++ - *pu1_est++;
            i4_sad += ABS(temp);
        }
        /* early exit */
        if(i4_max_sad < i4_sad)
        {
            *pi4_mb_distortion = i4_sad;
            return;
        }
        pu1_src += u4_src_offset;
        pu1_est += u4_est_offset;
    }
    *pi4_mb_distortion = i4_sad;
}

/**
******************************************************************************
*
*  @brief computes distortion (SAD) between 2 4x4 blocks
*
*  @par   Description
*   This functions computes SAD between 2 4x4 blocks. There is a provision
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
void ime_compute_sad_4x4
        (
            UWORD8 *pu1_src,
            UWORD8 *pu1_est,
            WORD32 src_strd,
            WORD32 est_strd,
            WORD32 i4_max_sad,
            WORD32 *pi4_mb_distortion
        )
{
    WORD32 i4_sad = 0;

    UNUSED(i4_max_sad);

    USADA8(pu1_src, pu1_est, i4_sad);
    pu1_src += src_strd;
    pu1_est += est_strd;

    USADA8(pu1_src, pu1_est, i4_sad);
    pu1_src += src_strd;
    pu1_est += est_strd;

    USADA8(pu1_src, pu1_est, i4_sad);
    pu1_src += src_strd;
    pu1_est += est_strd;

    USADA8(pu1_src, pu1_est, i4_sad);
    *pi4_mb_distortion = i4_sad;
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
void ime_compute_sad_16x8
        (
            UWORD8 *pu1_src,
            UWORD8 *pu1_est,
            WORD32 src_strd,
            WORD32 est_strd,
            WORD32 i4_max_sad,
            WORD32 *pi4_mb_distortion
        )
{
    WORD32 i4_sad = 0;
    UWORD32 u4_src_offset = src_strd - 16;
    UWORD32 u4_est_offset = est_strd - 16;
    UWORD32 i, j;
    WORD16 temp;

GATHER_16x8_SAD_EE_STATS(gu4_16x8_sad_ee_stats, 8);

    for(i = 8; i > 0; i--)
    {
        for(j = 16; j > 0; j--)
        {
            /* SAD */
            temp = *pu1_src++ - *pu1_est++;
            i4_sad += ABS(temp);
        }
        /* early exit */
        if(i4_max_sad < i4_sad)
        {

GATHER_16x8_SAD_EE_STATS(gu4_16x8_sad_ee_stats, 8-i);

            *pi4_mb_distortion = i4_sad;

            return;
        }
        pu1_src += u4_src_offset;
        pu1_est += u4_est_offset;
    }

    *pi4_mb_distortion = i4_sad;
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
void ime_compute_sad_16x16_ea8(UWORD8 *pu1_src,
                               UWORD8 *pu1_est,
                               WORD32 src_strd,
                               WORD32 est_strd,
                               WORD32 i4_max_sad,
                               WORD32 *pi4_mb_distortion)
{
    WORD32 i4_sad = 0;
    UWORD32 u4_src_offset = src_strd - 16;
    UWORD32 u4_est_offset = est_strd - 16;
    UWORD32 i, j;
    WORD16 temp;
    UWORD8 *pu1_src_temp = pu1_src + src_strd;
    UWORD8 *pu1_est_temp = pu1_est + est_strd;

    for(i = 16; i > 0; i -= 2)
    {
        for(j = 16; j > 0; j--)
        {
            /* SAD */
            temp = *pu1_src++ - *pu1_est++;
            i4_sad += ABS(temp);
        }

        pu1_src += (u4_src_offset + src_strd);
        pu1_est += (u4_est_offset + est_strd);

    }

    /* early exit */
    if(i4_max_sad < i4_sad)
    {
        *pi4_mb_distortion = i4_sad;
        return;
    }

    pu1_src = pu1_src_temp;
    pu1_est = pu1_est_temp;

    for(i = 16; i > 0; i -= 2)
    {
        for(j = 16; j > 0; j--)
        {
            /* SAD */
            temp = *pu1_src++ - *pu1_est++;
            i4_sad += ABS(temp);
        }

        pu1_src += u4_src_offset + src_strd;
        pu1_est += u4_est_offset + est_strd;
    }

    *pi4_mb_distortion = i4_sad;
    return;
}


/**
*******************************************************************************
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
void ime_compute_satqd_16x16_lumainter(UWORD8 *pu1_src,
                                         UWORD8 *pu1_est,
                                         WORD32 src_strd,
                                         WORD32 est_strd,
                                         UWORD16 *pu2_thrsh,
                                         WORD32 *pi4_mb_distortion,
                                         UWORD32 *pu4_is_non_zero)
{
    UWORD32 i,j;
    WORD16 s1,s2,s3,s4,sad_1,sad_2,ls1,ls2,ls3,ls4,ls5,ls6,ls7,ls8;
    UWORD8 *pu1_src_lp,*pu1_est_lp;
    UWORD32 sad = 0;

    (*pi4_mb_distortion) = 0;
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            pu1_src_lp = pu1_src + 4*j;
            pu1_est_lp = pu1_est + 4*j;

            s1 = ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[3] - (WORD16)pu1_est_lp[3]);
            s4 = ABS((WORD16)pu1_src_lp[1] - (WORD16)pu1_est_lp[1])+ ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2]);

            pu1_src_lp += src_strd;
            pu1_est_lp += est_strd;

            s2 = ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[3] - (WORD16)pu1_est_lp[3]);
            s3 = ABS((WORD16)pu1_src_lp[1] - (WORD16)pu1_est_lp[1])+ ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2]);

            pu1_src_lp += src_strd;
            pu1_est_lp += est_strd;

            s2 += ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[3] - (WORD16)pu1_est_lp[3]);
            s3 += ABS((WORD16)pu1_src_lp[1] - (WORD16)pu1_est_lp[1])+ ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2]);

            pu1_src_lp += src_strd;
            pu1_est_lp += est_strd;

            s1 += ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[3] - (WORD16)pu1_est_lp[3]);
            s4 += ABS((WORD16)pu1_src_lp[1] - (WORD16)pu1_est_lp[1])+ ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2]);

            sad_1 = s1+s2+s3+s4;

            if(sad == 0)
            {
                sad_2 = sad_1<<1;

                ls1 = sad_2 -(s2 + s3);
                ls2 = sad_2 -(s1 + s4);
                ls3 = sad_2 -(s3 + s4);
                ls4 = sad_2 -(s3 - (s1<<1));
                ls5 = sad_2 -(s4 - (s2<<1));
                ls6 = sad_2 -(s1 + s2);
                ls7 = sad_2 -(s2 - (s4<<1));
                ls8 = sad_2 -(s1 - (s3<<1));

                if(
                        pu2_thrsh[8] <= sad_1   ||
                        pu2_thrsh[0] <=  ls2    ||
                        pu2_thrsh[1] <=  ls1    ||
                        pu2_thrsh[2] <=  ls8    ||
                        pu2_thrsh[3] <=  ls5    ||

                        pu2_thrsh[4] <=  ls6    ||
                        pu2_thrsh[5] <=  ls3    ||
                        pu2_thrsh[6] <=  ls7    ||
                        pu2_thrsh[7] <=  ls4

                )sad = 1;
            }
            (*pi4_mb_distortion) += sad_1;
        }
        pu1_src +=  (src_strd *4);
        pu1_est +=  (est_strd *4);
    }
    *pu4_is_non_zero = sad;
}


/**
******************************************************************************
*
* @brief computes distortion (SAD and SAQTD) between 2 16x8 (interleaved) chroma blocks
*
*
* @par   Description
*   This functions computes SAD between2 16x8 chroma blocks(interleaved)
*   It also checks if the SATDD(Sum of absolute transformed wuqntized differnce beteern the blocks
*   If SAQTD is zero, it gives back zero
*   Other wise sad is retrned
*   There is no provison for early exit
*
*   The transform done here is the transform for chroma blocks in H264
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
* @remarks
* Fucntion code is nit updated.
* Will require debugging and minor modifications
*
******************************************************************************
*/
void ime_compute_satqd_8x16_chroma(UWORD8 *pu1_src,
                                     UWORD8 *pu1_est,
                                     WORD32 src_strd,
                                     WORD32 est_strd,
                                     WORD32 max_sad,
                                     UWORD16 *thrsh)
{
    WORD32 i,j,plane;
    WORD16 s1,s2,s3,s4,sad_1,sad_2,ls1,ls2,ls3,ls4,ls5,ls6,ls7,ls8;
    UWORD8 *pu1_src_lp,*pu1_est_lp,*pu1_src_plane,*pu1_est_plane;
    WORD32 sad =0;
    UNUSED(max_sad);

    pu1_src_plane = pu1_src;
    pu1_est_plane = pu1_est;

    for(plane =0;plane<2;plane++)
    {
        for(i=0;i<4;i++)
        {
            for(j=0;j<4;j++)
            {
                pu1_src_lp = pu1_src + 8*j;
                pu1_est_lp = pu1_est + 8*j;

                s1 = ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[6] - (WORD16)pu1_est_lp[6]);
                s4 = ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2])+ ABS((WORD16)pu1_src_lp[4] - (WORD16)pu1_est_lp[4]);

                pu1_src_lp += src_strd;
                pu1_est_lp += est_strd;

                s2 = ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[6] - (WORD16)pu1_est_lp[6]);
                s3 = ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2])+ ABS((WORD16)pu1_src_lp[4] - (WORD16)pu1_est_lp[4]);

                pu1_src_lp += src_strd;
                pu1_est_lp += est_strd;

                s2 += ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[6] - (WORD16)pu1_est_lp[6]);
                s3 += ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2])+ ABS((WORD16)pu1_src_lp[4] - (WORD16)pu1_est_lp[4]);

                pu1_src_lp += src_strd;
                pu1_est_lp += est_strd;

                s1 += ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[6] - (WORD16)pu1_est_lp[6]);
                s4 += ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2])+ ABS((WORD16)pu1_src_lp[4] - (WORD16)pu1_est_lp[4]);

                sad_1 = s1+s2+s3+s4;
                sad_2 = sad_1<<1;

                ls1 = sad_2 -(s2 + s3);
                ls2 = sad_2 -(s1 + s4);
                ls3 = sad_2 -(s3 + s4);
                ls4 = sad_2 -(s3 - (s1<<1));
                ls5 = sad_2 -(s4 - (s2<<1));
                ls6 = sad_2 -(s1 + s2);
                ls7 = sad_2 -(s2 - (s4<<1));
                ls8 = sad_2 -(s1 - (s3<<1));

                if(
                        //thrsh[0] >  sad_1     && Chroma Dc is checked later
                        thrsh[1] >  ls1     &&
                        thrsh[2] >  sad_1   &&
                        thrsh[3] >  ls2     &&

                        thrsh[4] >  ls3     &&
                        thrsh[5] >  ls4     &&
                        thrsh[6] >  ls3     &&
                        thrsh[7] >  ls5     &&

                        thrsh[8] >  sad_1   &&
                        thrsh[9] >  ls1     &&
                        thrsh[10]>  sad_1   &&
                        thrsh[11]>  ls2     &&

                        thrsh[12]>  ls6     &&
                        thrsh[13]>  ls7     &&
                        thrsh[14]>  ls6     &&
                        thrsh[15]>  ls8
                )
                {
                    /*set current sad to be zero*/
                }
                else
                    return ;

                sad += sad_1;
            }
            pu1_src +=  (src_strd *4);
            pu1_est +=  (est_strd *4);
        }
        if(sad < (thrsh[0]<<1))sad = 0;
        else return ;

        pu1_src = pu1_src_plane+1;
        pu1_est = pu1_est_plane+1;
    }
    return ;
}


/**
******************************************************************************
*
* @brief computes distortion (SAD and SAQTD) between 2 16x16 blocks
*
* @par   Description
*   This functions computes SAD between 2 16x16 blocks.
*   It also checks if the SATDD(Sum of absolute transformed wuqntized differnce beteern the blocks
*   If SAQTD is zero, it gives back zero
*   Other wise sad is retrned
*   There is no provison for early exit
*
*   The transform done here is the transform for inter 16x16 blocks in H264
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
* @remarks
*
******************************************************************************
*/
void ime_compute_satqd_16x16_lumaintra(UWORD8 *pu1_src,
                                         UWORD8 *pu1_est,
                                         WORD32 src_strd,
                                         WORD32 est_strd,
                                         WORD32 max_sad,
                                         UWORD16 *thrsh,
                                         WORD32 *pi4_mb_distortion,
                                         UWORD8 *sig_nz_sad)
{
    UWORD32 i,j;
    WORD16 s1[4],s2[4],s3[4],s4[4],sad[4];
    UWORD8 *pu1_src_lp,*pu1_est_lp;
    UWORD8 *sig_sad_dc;
    UWORD32 nz_sad_sig = 0;
    UNUSED(max_sad);
    *pi4_mb_distortion =0;

    sig_sad_dc = sig_nz_sad;
    sig_nz_sad++;

    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            pu1_src_lp = pu1_src + 4*j;
            pu1_est_lp = pu1_est + 4*j;

            s1[j] = ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[3] - (WORD16)pu1_est_lp[3]);
            s4[j] = ABS((WORD16)pu1_src_lp[1] - (WORD16)pu1_est_lp[1])+ ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2]);

            pu1_src_lp += src_strd;
            pu1_est_lp += est_strd;

            s2[j] = ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[3] - (WORD16)pu1_est_lp[3]);
            s3[j] = ABS((WORD16)pu1_src_lp[1] - (WORD16)pu1_est_lp[1])+ ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2]);

            pu1_src_lp += src_strd;
            pu1_est_lp += est_strd;

            s2[j] += ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[3] - (WORD16)pu1_est_lp[3]);
            s3[j] += ABS((WORD16)pu1_src_lp[1] - (WORD16)pu1_est_lp[1])+ ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2]);

            pu1_src_lp += src_strd;
            pu1_est_lp += est_strd;

            s1[j] += ABS((WORD16)pu1_src_lp[0] - (WORD16)pu1_est_lp[0])+ ABS((WORD16)pu1_src_lp[3] - (WORD16)pu1_est_lp[3]);
            s4[j] += ABS((WORD16)pu1_src_lp[1] - (WORD16)pu1_est_lp[1])+ ABS((WORD16)pu1_src_lp[2] - (WORD16)pu1_est_lp[2]);

            sad[j] = ((s1[j]+s2[j]+s3[j]+s4[j])<<1);
        }

        for(j=0;j<4;j++)
        {

            if(
                    //thrsh[0] > (sad[j] >> 1) &&Dc goes in the other part
                    thrsh[1] > (sad[j] -(s2[j] + s3[j])) &&
                    thrsh[2] > (sad[j]>>1) &&
                    thrsh[3] > (sad[j] -(s1[j] + s4[j])) &&

                    thrsh[4] > (sad[j] -(s3[j] + s4[j])) &&
                    thrsh[5] > (sad[j] -(s3[j] - (s1[j]<<1))) &&
                    thrsh[6] > (sad[j] -(s3[j] + s4[j])) &&
                    thrsh[7] > (sad[j] -(s4[j] - (s2[j]<<1))) &&

                    thrsh[8] > (sad[j]>>1) &&
                    thrsh[9] > (sad[j] -(s2[j] + s3[j])) &&
                    thrsh[10]> (sad[j]>>1) &&
                    thrsh[11]> (sad[j] -(s1[j] + s4[j])) &&

                    thrsh[12]> (sad[j] -(s1[j] + s2[j])) &&
                    thrsh[13]> (sad[j] -(s2[j] - (s4[j]<<1))) &&
                    thrsh[14]> (sad[j] -(s1[j] + s2[j])) &&
                    thrsh[15]> (sad[j] -(s1[j] - (s3[j]<<1)))
            )
            {
                //sad[j] = 0;   /*set current sad to be zero*/
                sig_nz_sad[j] = 0;/*Signal that the sad is zero*/
            }
            else
            {
                sig_nz_sad[j] = 1;/*signal that sad is non zero*/
                nz_sad_sig = 1;
            }

            (*pi4_mb_distortion) += (sad[j]>>1);
            //if((*pi4_mb_distortion) >= max_sad)return; /*return or some thing*/
        }

        sig_nz_sad += 4;
        pu1_src +=  (src_strd *4);
        pu1_est +=  (est_strd *4);
    }

    if((*pi4_mb_distortion) < thrsh[0]<<2)
    {
        *sig_sad_dc = 0;
        if(nz_sad_sig == 0)(*pi4_mb_distortion) = 0;
    }
    else *sig_sad_dc = 1;
}


