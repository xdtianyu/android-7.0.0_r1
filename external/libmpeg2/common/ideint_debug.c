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
*  ideint_proc_fxns.c
*
* @brief
*  This file contains the definitions of the core  processing of the de
* interlacer.
*
* @author
*  Ittiam
*
* @par List of Functions:
*  ideint_corrupt_pic()
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
#include "icv_variance.h"
#include "icv_sad.h"
#include "ideint.h"

#include "ideint_defs.h"
#include "ideint_structs.h"
#include "ideint_utils.h"
#include "ideint_cac.h"
#include "ideint_debug.h"

/**
*******************************************************************************
*
* @brief
*  Corrupt a picture with given value
*
* @par   Description
*  Corrupt a picture with given value
*
* @param[in] ps_pic
*  Picture to be corrupted
*
* @param[in] val
*  Value to be used to corrupt the picture
*
* @returns
*  None
*
* @remarks
*
*******************************************************************************
*/
void ideint_corrupt_pic(icv_pic_t *ps_pic, WORD32 val)
{
    WORD32 i, j;
    WORD32 num_comp;

    num_comp = 3;
    for (i = 0; i < num_comp; i++)
    {
        WORD32 wd, ht, strd;
        UWORD8 *pu1_buf;
        wd = ps_pic->ai4_wd[i];
        ht = ps_pic->ai4_ht[i];
        strd = ps_pic->ai4_strd[i];
        pu1_buf = ps_pic->apu1_buf[i];

        for (j = 0; j < ht; j++)
        {
            memset(pu1_buf, val, wd);
            pu1_buf += strd;
        }

    }
}
