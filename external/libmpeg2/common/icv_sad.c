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
*  sad_8x4()
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
WORD32 icv_sad_8x4(UWORD8 *pu1_src,
                   UWORD8 *pu1_ref,
                   WORD32 src_strd,
                   WORD32 ref_strd,
                   WORD32 wd,
                   WORD32 ht)
{
    WORD32 sad;
    WORD32 i;
    WORD32 j;
    UNUSED(wd);
    UNUSED(ht);

    ASSERT(wd == 8);
    ASSERT(ht == 4);

    sad = 0;

    for(j = 0; j < 4; j++)
    {
        for(i = 0; i < 8; i++)
        {
            WORD32 src;
            WORD32 ref;

            src = *pu1_src++;
            ref = *pu1_ref++;

            sad += ABS_DIF(src, ref);
        }
        pu1_src += (src_strd - 8);
        pu1_ref += (ref_strd - 8);
    }

    return sad;
}
