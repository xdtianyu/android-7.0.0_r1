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
*  impeg2d_function_selector.c
*
* @brief
*  Contains functions to initialize function pointers used in mpeg2
*
* @author
*  Naveen
*
* @par List of Functions:
* @remarks
*  None
*
*******************************************************************************
*/
/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* User include files */
#include "iv_datatypedef.h"
#include "iv.h"
#include "ivd.h"
#include "ithread.h"

#include "impeg2_macros.h"
#include "impeg2_buf_mgr.h"
#include "impeg2_disp_mgr.h"
#include "impeg2_defs.h"
#include "impeg2_platform_macros.h"
#include "impeg2_inter_pred.h"
#include "impeg2_idct.h"
#include "impeg2_format_conv.h"
#include "impeg2_mem_func.h"

#include "impeg2d.h"
#include "impeg2d_bitstream.h"
#include "impeg2d_debug.h"
#include "impeg2d_structs.h"

void impeg2d_init_function_ptr_sse42(void *pv_codec)
{
    dec_state_t *dec = (dec_state_t *)pv_codec;

    dec->pf_idct_recon[0]                   = &impeg2_idct_recon_dc_sse42;
    dec->pf_idct_recon[1]                   = &impeg2_idct_recon_dc_mismatch_sse42;
    dec->pf_idct_recon[2]                   = &impeg2_idct_recon_sse42;
    dec->pf_idct_recon[3]                   = &impeg2_idct_recon_sse42;

    dec->pf_copy_mb                         = &impeg2_copy_mb_sse42;
    dec->pf_interpolate                     = &impeg2_interpolate_sse42;
    dec->pf_halfx_halfy_8x8                 = &impeg2_mc_halfx_halfy_8x8_sse42;
    dec->pf_halfx_fully_8x8                 = &impeg2_mc_halfx_fully_8x8_sse42;
    dec->pf_fullx_halfy_8x8                 = &impeg2_mc_fullx_halfy_8x8_sse42;
    dec->pf_fullx_fully_8x8                 = &impeg2_mc_fullx_fully_8x8_sse42;

    dec->pf_memset_8bit_8x8_block           = &impeg2_memset_8bit_8x8_block_sse42;
    dec->pf_memset_16bit_8x8_linear_block   = &impeg2_memset0_16bit_8x8_linear_block_sse42;
}
