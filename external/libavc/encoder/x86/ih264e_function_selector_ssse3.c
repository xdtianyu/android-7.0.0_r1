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
*  ih264e_function_selector_ssse3.c
*
* @brief
*  Contains functions to initialize function pointers of codec context
*
* @author
*  Ittiam
*
* @par List of Functions:
*  - ih264e_init_function_ptr_ssse3
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
#include "ih264e_platform_macros.h"
#include "ih264e_cabac.h"
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
void ih264e_init_function_ptr_ssse3(codec_t *ps_codec)
{

    /* Init function pointers for intra pred leaf level functions luma
      * Intra 16x16 */
     ps_codec->apf_intra_pred_16_l[0] = ih264_intra_pred_luma_16x16_mode_vert_ssse3;
     ps_codec->apf_intra_pred_16_l[1] = ih264_intra_pred_luma_16x16_mode_horz_ssse3;
     ps_codec->apf_intra_pred_16_l[2] = ih264_intra_pred_luma_16x16_mode_dc_ssse3;
     ps_codec->apf_intra_pred_16_l[3] = ih264_intra_pred_luma_16x16_mode_plane_ssse3;

     /* Init function pointers for intra pred leaf level functions luma
      * Intra 4x4 */
     ps_codec->apf_intra_pred_4_l[0] = ih264_intra_pred_luma_4x4_mode_vert_ssse3;
     ps_codec->apf_intra_pred_4_l[1] = ih264_intra_pred_luma_4x4_mode_horz_ssse3;
     ps_codec->apf_intra_pred_4_l[2] = ih264_intra_pred_luma_4x4_mode_dc_ssse3;
     ps_codec->apf_intra_pred_4_l[3] = ih264_intra_pred_luma_4x4_mode_diag_dl_ssse3;
     ps_codec->apf_intra_pred_4_l[4] = ih264_intra_pred_luma_4x4_mode_diag_dr_ssse3;
     ps_codec->apf_intra_pred_4_l[5] = ih264_intra_pred_luma_4x4_mode_vert_r_ssse3;
     ps_codec->apf_intra_pred_4_l[6] = ih264_intra_pred_luma_4x4_mode_horz_d_ssse3;
     ps_codec->apf_intra_pred_4_l[7] = ih264_intra_pred_luma_4x4_mode_vert_l_ssse3;
     ps_codec->apf_intra_pred_4_l[8] = ih264_intra_pred_luma_4x4_mode_horz_u_ssse3;

     /* Init function pointers for intra pred leaf level functions luma
      * Intra 8x8 */
     ps_codec->apf_intra_pred_8_l[0] = ih264_intra_pred_luma_8x8_mode_vert_ssse3;
     ps_codec->apf_intra_pred_8_l[2] = ih264_intra_pred_luma_8x8_mode_dc_ssse3;
     ps_codec->apf_intra_pred_8_l[3] = ih264_intra_pred_luma_8x8_mode_diag_dl_ssse3;
     ps_codec->apf_intra_pred_8_l[4] = ih264_intra_pred_luma_8x8_mode_diag_dr_ssse3;
     ps_codec->apf_intra_pred_8_l[5] = ih264_intra_pred_luma_8x8_mode_vert_r_ssse3;
     ps_codec->apf_intra_pred_8_l[6] = ih264_intra_pred_luma_8x8_mode_horz_d_ssse3;
     ps_codec->apf_intra_pred_8_l[7] = ih264_intra_pred_luma_8x8_mode_vert_l_ssse3;
     ps_codec->apf_intra_pred_8_l[8] = ih264_intra_pred_luma_8x8_mode_horz_u_ssse3;

     /* Init function pointers for intra pred leaf level functions chroma
      * Intra 8x8 */
     ps_codec->apf_intra_pred_c[1] = ih264_intra_pred_chroma_8x8_mode_horz_ssse3;
     ps_codec->apf_intra_pred_c[2] = ih264_intra_pred_chroma_8x8_mode_vert_ssse3;
     ps_codec->apf_intra_pred_c[3] = ih264_intra_pred_chroma_8x8_mode_plane_ssse3;

    /* Init inverse transform fn ptr */
    ps_codec->pf_iquant_itrans_recon_8x8 = ih264_iquant_itrans_recon_8x8_ssse3;
    ps_codec->pf_iquant_itrans_recon_4x4_dc       = ih264_iquant_itrans_recon_4x4_dc_ssse3;
    ps_codec->pf_iquant_itrans_recon_chroma_4x4_dc = ih264_iquant_itrans_recon_chroma_4x4_dc_ssse3;

    /* Init fn ptr luma deblocking */
    ps_codec->pf_deblk_luma_vert_bs4 = ih264_deblk_luma_vert_bs4_ssse3;
     ps_codec->pf_deblk_luma_vert_bslt4 = ih264_deblk_luma_vert_bslt4_ssse3;
     ps_codec->pf_deblk_luma_horz_bs4 = ih264_deblk_luma_horz_bs4_ssse3;
     ps_codec->pf_deblk_luma_horz_bslt4 = ih264_deblk_luma_horz_bslt4_ssse3;
    /* Init fn ptr chroma deblocking */
     ps_codec->pf_deblk_chroma_vert_bs4 = ih264_deblk_chroma_vert_bs4_ssse3;
     ps_codec->pf_deblk_chroma_vert_bslt4 = ih264_deblk_chroma_vert_bslt4_ssse3;
     ps_codec->pf_deblk_chroma_horz_bs4 = ih264_deblk_chroma_horz_bs4_ssse3;
     ps_codec->pf_deblk_chroma_horz_bslt4 = ih264_deblk_chroma_horz_bslt4_ssse3;

     /* Padding Functions */
     ps_codec->pf_pad_left_luma = ih264_pad_left_luma_ssse3;
     ps_codec->pf_pad_left_chroma = ih264_pad_left_chroma_ssse3;
     ps_codec->pf_pad_right_luma = ih264_pad_right_luma_ssse3;
     ps_codec->pf_pad_right_chroma = ih264_pad_right_chroma_ssse3;

    /* Inter pred leaf level functions */
    ps_codec->pf_inter_pred_luma_copy = ih264_inter_pred_luma_copy_ssse3;
    ps_codec->pf_inter_pred_luma_horz = ih264_inter_pred_luma_horz_ssse3;
    ps_codec->pf_inter_pred_luma_vert = ih264_inter_pred_luma_vert_ssse3;
    ps_codec->pf_inter_pred_chroma = ih264_inter_pred_chroma_ssse3;

    /* memory handling operations */
    ps_codec->pf_mem_cpy_mul8 = ih264_memcpy_mul_8_ssse3;
    ps_codec->pf_mem_set_mul8 = ih264_memset_mul_8_ssse3;

    /*intra mode eval -encoder level function*/
    ps_codec->pf_ih264e_evaluate_intra16x16_modes = ih264e_evaluate_intra16x16_modes_ssse3;
    ps_codec->pf_ih264e_evaluate_intra_4x4_modes = ih264e_evaluate_intra_4x4_modes_ssse3;
    ps_codec->pf_ih264e_evaluate_intra_chroma_modes = ih264e_evaluate_intra_chroma_modes_ssse3;

    /* Halp pel generation function - encoder level*/
    ps_codec->pf_ih264e_sixtapfilter_horz = ih264e_sixtapfilter_horz_ssse3;
    ps_codec->pf_ih264e_sixtap_filter_2dvh_vert = ih264e_sixtap_filter_2dvh_vert_ssse3;
}
