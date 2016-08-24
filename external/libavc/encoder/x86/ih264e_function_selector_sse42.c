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
*  ih264e_function_selector_sse42.c
*
* @brief
*  Contains functions to initialize function pointers of codec context
*
* @author
*  Ittiam
*
* @par List of Functions:
*  - ih264e_init_function_ptr_sse42
*
* @remarks
*  None
*
*******************************************************************************
*/


/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/


/* System Include files */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* User Include files */
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264_defs.h"
#include "ih264_size_defs.h"
#include "ih264e_defs.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "ime_distortion_metrics.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "ih264_error.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_cabac.h"
#include "ih264e_platform_macros.h"
#include "ih264e_core_coding.h"
#include "ih264_cavlc_tables.h"
#include "ih264e_cavlc.h"
#include "ih264e_intra_modes_eval.h"
#include "ih264e_fmt_conv.h"
#include "ih264e_half_pel.h"

/**
*******************************************************************************
*
* @brief Initialize the intra/inter/transform/deblk function pointers of
* codec context
*
* @par Description: the current routine initializes the function pointers of
* codec context basing on the architecture in use
*
* @param[in] ps_codec
*  Codec context pointer
*
* @returns  none
*
* @remarks none
*
*******************************************************************************
*/
void ih264e_init_function_ptr_sse42(codec_t *ps_codec)
{
    WORD32 i;
    process_ctxt_t *ps_proc = NULL;
    me_ctxt_t *ps_me_ctxt = NULL;

    /* Init luma forward transform fn ptr */
    ps_codec->pf_resi_trans_quant_4x4 = ih264_resi_trans_quant_4x4_sse42;
    ps_codec->pf_resi_trans_quant_chroma_4x4     = ih264_resi_trans_quant_chroma_4x4_sse42;
    ps_codec->pf_hadamard_quant_4x4              = ih264_hadamard_quant_4x4_sse42;
    ps_codec->pf_hadamard_quant_2x2_uv           = ih264_hadamard_quant_2x2_uv_sse42;

    /* Init inverse transform fn ptr */
    ps_codec->pf_iquant_itrans_recon_4x4 = ih264_iquant_itrans_recon_4x4_sse42;
    ps_codec->pf_iquant_itrans_recon_chroma_4x4   = ih264_iquant_itrans_recon_chroma_4x4_sse42;
    ps_codec->pf_ihadamard_scaling_4x4            = ih264_ihadamard_scaling_4x4_sse42;

    /* sad me level functions */
    ps_codec->apf_compute_sad_16x16[0] = ime_compute_sad_16x16_sse42;
    ps_codec->apf_compute_sad_16x16[1] = ime_compute_sad_16x16_fast_sse42;
    ps_codec->pf_compute_sad_16x8 = ime_compute_sad_16x8_sse42;

    /* sad me level functions */
    for(i = 0; i < (MAX_PROCESS_CTXT); i++)
    {
        ps_proc = &ps_codec->as_process[i];

        ps_me_ctxt = &ps_proc->s_me_ctxt;
        ps_me_ctxt->pf_ime_compute_sad_16x16[0] = ime_compute_sad_16x16_sse42;
        ps_me_ctxt->pf_ime_compute_sad_16x16[1] = ime_compute_sad_16x16_fast_sse42;
        ps_me_ctxt->pf_ime_compute_sad_16x8 = ime_compute_sad_16x8_sse42;
        ps_me_ctxt->pf_ime_compute_sad4_diamond = ime_calculate_sad4_prog_sse42;
        ps_me_ctxt->pf_ime_sub_pel_compute_sad_16x16 = ime_sub_pel_compute_sad_16x16_sse42;
        ps_me_ctxt->pf_ime_compute_sad_stat_luma_16x16      = ime_compute_satqd_16x16_lumainter_sse42;
    }
}
