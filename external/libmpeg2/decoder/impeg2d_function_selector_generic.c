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

#include "iv_datatypedef.h"
#include "iv.h"
#include "ithread.h"


#include "impeg2_macros.h"
#include "impeg2_buf_mgr.h"
#include "impeg2_disp_mgr.h"
#include "impeg2_defs.h"
#include "impeg2_platform_macros.h"
#include "impeg2_inter_pred.h"
#include "impeg2_idct.h"
#include "impeg2_mem_func.h"
#include "impeg2_format_conv.h"
#include "impeg2_job_queue.h"
#include "impeg2_globals.h"

#include "ivd.h"
#include "impeg2d.h"
#include "impeg2d_api.h"
#include "impeg2d_debug.h"
#include "impeg2d_bitstream.h"
#include "impeg2d_structs.h"
#include "impeg2d_mc.h"
#include "impeg2d_pic_proc.h"
#include "impeg2d_vld_tables.h"
#include "impeg2d_vld.h"
#include "impeg2d_pic_proc.h"

void impeg2d_init_function_ptr_generic(void *pv_codec)
{
    dec_state_t *ps_dec = (dec_state_t *)pv_codec;

    ps_dec->pf_idct_recon[0]                   = &impeg2_idct_recon_dc;
    ps_dec->pf_idct_recon[1]                   = &impeg2_idct_recon_dc_mismatch;
    ps_dec->pf_idct_recon[2]                   = &impeg2_idct_recon;
    ps_dec->pf_idct_recon[3]                   = &impeg2_idct_recon;

    ps_dec->pf_mc[0]                              = &impeg2d_mc_fullx_fully;
    ps_dec->pf_mc[1]                              = &impeg2d_mc_fullx_halfy;
    ps_dec->pf_mc[2]                              = &impeg2d_mc_halfx_fully;
    ps_dec->pf_mc[3]                              = &impeg2d_mc_halfx_halfy;

    ps_dec->pf_interpolate                     = &impeg2_interpolate;
    ps_dec->pf_copy_mb                         = &impeg2_copy_mb;

    ps_dec->pf_fullx_halfy_8x8                 = &impeg2_mc_fullx_halfy_8x8;
    ps_dec->pf_halfx_fully_8x8                 = &impeg2_mc_halfx_fully_8x8;
    ps_dec->pf_halfx_halfy_8x8                 = &impeg2_mc_halfx_halfy_8x8;
    ps_dec->pf_fullx_fully_8x8                 = &impeg2_mc_fullx_fully_8x8;

    ps_dec->pf_memset_8bit_8x8_block           = &impeg2_memset_8bit_8x8_block;
    ps_dec->pf_memset_16bit_8x8_linear_block   = &impeg2_memset0_16bit_8x8_linear_block;

    ps_dec->pf_copy_yuv420p_buf                = &impeg2_copy_frm_yuv420p;
    ps_dec->pf_fmt_conv_yuv420p_to_yuv422ile   = &impeg2_fmt_conv_yuv420p_to_yuv422ile;
    ps_dec->pf_fmt_conv_yuv420p_to_yuv420sp_uv = &impeg2_fmt_conv_yuv420p_to_yuv420sp_uv;
    ps_dec->pf_fmt_conv_yuv420p_to_yuv420sp_vu = &impeg2_fmt_conv_yuv420p_to_yuv420sp_vu;
}
