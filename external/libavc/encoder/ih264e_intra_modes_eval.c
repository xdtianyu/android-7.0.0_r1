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
*  ih264e_intra_modes_eval.c
*
* @brief
*  This file contains definitions of routines that perform rate distortion
*  analysis on a macroblock if they are to be coded as intra.
*
* @author
*  ittiam
*
* @par List of Functions:
*  - ih264e_derive_neighbor_availability_of_mbs()
*  - ih264e_derive_ngbr_avbl_of_mb_partitions()
*  - ih264e_evaluate_intra16x16_modes_for_least_cost_rdoptoff()
*  - ih264e_evaluate_intra8x8_modes_for_least_cost_rdoptoff()
*  - ih264e_evaluate_intra4x4_modes_for_least_cost_rdoptoff()
*  - ih264e_evaluate_intra4x4_modes_for_least_cost_rdopton()
*  - ih264e_evaluate_chroma_intra8x8_modes_for_least_cost_rdoptoff()
*  - ih264e_evaluate_intra16x16_modes()
*  - ih264e_evaluate_intra4x4_modes()
*  - ih264e_evaluate_intra_chroma_modes()
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
#include <string.h>
#include <limits.h>
#include <assert.h>

/* User include files */
#include "ih264e_config.h"
#include "ih264_typedefs.h"
#include "ih264e_defs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264_debug.h"
#include "ih264_defs.h"
#include "ih264_macros.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_structs.h"
#include "ih264_common_tables.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "ime_distortion_metrics.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_intra_modes_eval.h"
#include "ih264e_globals.h"
#include "ime_platform_macros.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
******************************************************************************
*
* @brief
*  derivation process for macroblock availability
*
* @par   Description
*  Calculates the availability of the left, top, topright and topleft macroblocks.
*
* @param[in] ps_proc_ctxt
*  pointer to proc context (handle)
*
* @remarks Based on section 6.4.5 in H264 spec
*
* @return  none
*
******************************************************************************
*/
void ih264e_derive_nghbr_avbl_of_mbs(process_ctxt_t *ps_proc)
{
    UWORD8 *pu1_slice_idx_curr = ps_proc->pu1_slice_idx;
    UWORD8 *pu1_slice_idx_b;
    UWORD8 *pu1_slice_idx_a;
    UWORD8 *pu1_slice_idx_c;
    UWORD8 *pu1_slice_idx_d;
    block_neighbors_t *ps_ngbr_avbl;
    WORD32 i4_mb_x, i4_mb_y;
    WORD32 i4_wd_mbs;

    i4_mb_x = ps_proc->i4_mb_x;
    i4_mb_y = ps_proc->i4_mb_y;

    i4_wd_mbs = ps_proc->i4_wd_mbs;

    pu1_slice_idx_curr += (i4_mb_y * i4_wd_mbs) + i4_mb_x;
    pu1_slice_idx_a = pu1_slice_idx_curr - 1;
    pu1_slice_idx_b = pu1_slice_idx_curr - i4_wd_mbs;
    pu1_slice_idx_c = pu1_slice_idx_b + 1;
    pu1_slice_idx_d = pu1_slice_idx_b - 1;
    ps_ngbr_avbl = ps_proc->ps_ngbr_avbl;

    /**********************************************************************/
    /* The macroblock is marked as available, unless one of the following */
    /* conditions is true in which case the macroblock shall be marked as */
    /* not available.                                                     */
    /* 1. mbAddr < 0                                                      */
    /* 2  mbAddr > CurrMbAddr                                             */
    /* 3. the macroblock with address mbAddr belongs to a different slice */
    /* than the macroblock with address CurrMbAddr                        */
    /**********************************************************************/

    /* left macroblock availability */
    if (i4_mb_x == 0)
    { /* macroblocks along first column */
        ps_ngbr_avbl->u1_mb_a = 0;
    }
    else
    { /* macroblocks belong to same slice? */
        if (*pu1_slice_idx_a != *pu1_slice_idx_curr)
            ps_ngbr_avbl->u1_mb_a = 0;
        else
            ps_ngbr_avbl->u1_mb_a = 1;
    }

    /* top macroblock availability */
    if (i4_mb_y == 0)
    { /* macroblocks along first row */
        ps_ngbr_avbl->u1_mb_b = 0;
    }
    else
    { /* macroblocks belong to same slice? */
        if (*pu1_slice_idx_b != *pu1_slice_idx_curr)
            ps_ngbr_avbl->u1_mb_b = 0;
        else
            ps_ngbr_avbl->u1_mb_b = 1;
    }

    /* top right macroblock availability */
    if (i4_mb_x == i4_wd_mbs-1 || i4_mb_y == 0)
    { /* macroblocks along last column */
        ps_ngbr_avbl->u1_mb_c = 0;
    }
    else
    { /* macroblocks belong to same slice? */
        if (*pu1_slice_idx_c != *pu1_slice_idx_curr)
            ps_ngbr_avbl->u1_mb_c = 0;
        else
            ps_ngbr_avbl->u1_mb_c = 1;
    }

    /* top left macroblock availability */
    if (i4_mb_x == 0 || i4_mb_y == 0)
    { /* macroblocks along first column */
        ps_ngbr_avbl->u1_mb_d = 0;
    }
    else
    { /* macroblocks belong to same slice? */
        if (*pu1_slice_idx_d != *pu1_slice_idx_curr)
            ps_ngbr_avbl->u1_mb_d = 0;
        else
            ps_ngbr_avbl->u1_mb_d = 1;
    }
}

/**
******************************************************************************
*
* @brief
*  derivation process for subblock/partition availability
*
* @par   Description
*  Calculates the availability of the left, top, topright and topleft subblock
*  or partitions.
*
* @param[in]    ps_proc_ctxt
*  pointer to macroblock context (handle)
*
* @param[in]    i1_pel_pos_x
*  column position of the pel wrt the current block
*
* @param[in]    i1_pel_pos_y
*  row position of the pel in wrt current block
*
* @remarks     Assumptions: before calling this function it is assumed that
*   the neighbor availability of the current macroblock is already derived.
*   Based on table 6-3 of H264 specification
*
* @return      availability status (yes or no)
*
******************************************************************************
*/
UWORD8 ih264e_derive_ngbr_avbl_of_mb_partitions(block_neighbors_t *ps_ngbr_avbl,
                                                WORD8 i1_pel_pos_x,
                                                WORD8 i1_pel_pos_y)
{
    UWORD8 u1_neighbor_avail=0;

    /**********************************************************************/
    /* values of i1_pel_pos_x in the range 0-15 inclusive correspond to   */
    /* various columns of a macroblock                                    */
    /*                                                                    */
    /* values of i1_pel_pos_y in the range 0-15 inclusive correspond to   */
    /* various rows of a macroblock                                       */
    /*                                                                    */
    /* other values of i1_pel_pos_x & i1_pel_pos_y represents elements    */
    /* outside the bound of an mb ie., represents its neighbors.          */
    /**********************************************************************/
    if (i1_pel_pos_x < 0)
    { /* column(-1) */
        if (i1_pel_pos_y < 0)
        { /* row(-1) */
            u1_neighbor_avail = ps_ngbr_avbl->u1_mb_d; /* current mb topleft availability */
        }
        else if (i1_pel_pos_y >= 0 && i1_pel_pos_y < 16)
        { /* all rows of a macroblock */
            u1_neighbor_avail = ps_ngbr_avbl->u1_mb_a; /* current mb left availability */
        }
        else /* if (i1_pel_pos_y >= 16) */
        { /* rows(+16) */
            u1_neighbor_avail = 0;  /* current mb bottom left availability */
        }
    }
    else if (i1_pel_pos_x >= 0 && i1_pel_pos_x < 16)
    { /* all columns of a macroblock */
        if (i1_pel_pos_y < 0)
        { /* row(-1) */
            u1_neighbor_avail = ps_ngbr_avbl->u1_mb_b; /* current mb top availability */
        }
        else if (i1_pel_pos_y >= 0 && i1_pel_pos_y < 16)
        { /* all rows of a macroblock */
            u1_neighbor_avail = 1; /* current mb availability */
            /* availability of the partition is dependent on the position of the partition inside the mb */
            /* although the availability is declared as 1 in all cases these needs to be corrected somewhere else and this is not done in here */
        }
        else /* if (i1_pel_pos_y >= 16) */
        { /* rows(+16) */
            u1_neighbor_avail = 0;  /* current mb bottom availability */
        }
    }
    else if (i1_pel_pos_x >= 16)
    { /* column(+16) */
        if (i1_pel_pos_y < 0)
        { /* row(-1) */
            u1_neighbor_avail = ps_ngbr_avbl->u1_mb_c; /* current mb top right availability */
        }
        else /* if (i1_pel_pos_y >= 0) */
        { /* all other rows */
            u1_neighbor_avail = 0;  /* current mb right & bottom right availability */
        }
    }

    return u1_neighbor_avail;
}

/**
******************************************************************************
*
* @brief
*  evaluate best intra 16x16 mode (rate distortion opt off)
*
* @par Description
*  This function evaluates all the possible intra 16x16 modes and finds the mode
*  that best represents the macro-block (least distortion) and occupies fewer
*  bits in the bit-stream.
*
* @param[in]   ps_proc_ctxt
*  pointer to process context (handle)
*
* @remarks
*  Ideally the cost of encoding a macroblock is calculated as
*  (distortion + lambda*rate). Where distortion is SAD/SATD,... between the
*  input block and the reconstructed block and rate is the number of bits taken
*  to place the macroblock in the bit-stream. In this routine the rate does not
*  exactly point to the total number of bits it takes, rather it points to header
*  bits necessary for encoding the macroblock. Assuming the deltaQP, cbp bits
*  and residual bits fall in to texture bits the number of bits taken to encoding
*  mbtype is considered as rate, we compute cost. Further we will approximate
*  the distortion as the deviation b/w input and the predicted block as opposed
*  to input and reconstructed block.
*
*  NOTE: As per the Document JVT-O079, for intra 16x16 macroblock,
*  the SAD and cost are one and the same.
*
* @return     none
*
******************************************************************************
*/

void ih264e_evaluate_intra16x16_modes_for_least_cost_rdoptoff(process_ctxt_t *ps_proc)
{
    /* Codec Context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* SAD(distortion metric) of an 8x8 block */
    WORD32 i4_mb_distortion = INT_MAX, i4_mb_distortion_least = INT_MAX;

    /* lambda */
    UWORD32 u4_lambda = ps_proc->u4_lambda;

    /* cost = distortion + lambda*rate */
    WORD32 i4_mb_cost= INT_MAX, i4_mb_cost_least = INT_MAX;

    /* intra mode */
    UWORD32 u4_intra_mode, u4_best_intra_16x16_mode = DC_I16x16;

    /* neighbor pels for intra prediction */
    UWORD8 *pu1_ngbr_pels_i16 = ps_proc->au1_ngbr_pels;

    /* neighbor availability */
    WORD32 i4_ngbr_avbl;

    /* pointer to src macro block */
    UWORD8 *pu1_curr_mb = ps_proc->pu1_src_buf_luma;
    UWORD8 *pu1_ref_mb = ps_proc->pu1_rec_buf_luma;

    /* pointer to prediction macro block */
    UWORD8 *pu1_pred_mb_intra_16x16 = ps_proc->pu1_pred_mb_intra_16x16;
    UWORD8 *pu1_pred_mb_intra_16x16_plane = ps_proc->pu1_pred_mb_intra_16x16_plane;

    /* strides */
    WORD32 i4_src_strd = ps_proc->i4_src_strd;
    WORD32 i4_pred_strd = ps_proc->i4_pred_strd;
    WORD32 i4_rec_strd = ps_proc->i4_rec_strd;

    /* pointer to neighbors left, top, topleft */
    UWORD8 *pu1_mb_a = pu1_ref_mb - 1;
    UWORD8 *pu1_mb_b = pu1_ref_mb - i4_rec_strd;
    UWORD8 *pu1_mb_d = pu1_mb_b - 1;
    UWORD8 u1_mb_a, u1_mb_b, u1_mb_d;
    /* valid intra modes map */
    UWORD32 u4_valid_intra_modes;

    /* lut for valid intra modes */
    const UWORD8 u1_valid_intra_modes[8] = {4, 6, 4, 6, 5, 7, 5, 15};

    /* temp var */
    UWORD32 i, u4_enable_fast_sad = 0, offset = 0;
    mb_info_t *ps_top_mb_syn_ele = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x;
    UWORD32 u4_constrained_intra_pred = ps_proc->ps_codec->s_cfg.u4_constrained_intra_pred;

    /* init temp var */
    if (ps_proc->i4_slice_type != ISLICE)
    {
        /* Offset for MBtype */
        offset = (ps_proc->i4_slice_type == PSLICE) ? 5 : 23;
        u4_enable_fast_sad = ps_proc->s_me_ctxt.u4_enable_fast_sad;
    }

    /* locating neighbors that are available for prediction */

    /* gather prediction pels from the neighbors, if particular set is not available
     * it is set to zero*/
    /* left pels */
    u1_mb_a = ((ps_proc->ps_ngbr_avbl->u1_mb_a)
                    && (u4_constrained_intra_pred ? ps_proc->s_left_mb_syntax_ele.u2_is_intra : 1));
    if (u1_mb_a)
    {
        for(i = 0; i < 16; i++)
            pu1_ngbr_pels_i16[16-1-i] = pu1_mb_a[i * i4_rec_strd];
    }
    else
    {
        ps_codec->pf_mem_set_mul8(pu1_ngbr_pels_i16,0,MB_SIZE);
    }
    /* top pels */
    u1_mb_b = ((ps_proc->ps_ngbr_avbl->u1_mb_b)
                    && (u4_constrained_intra_pred ? ps_top_mb_syn_ele->u2_is_intra : 1));
    if (u1_mb_b)
    {
        ps_codec->pf_mem_cpy_mul8(pu1_ngbr_pels_i16+16+1,pu1_mb_b,16);
    }
    else
    {
        ps_codec->pf_mem_set_mul8(pu1_ngbr_pels_i16+16+1,0,MB_SIZE);
    }
    /* topleft pels */
    u1_mb_d = ((ps_proc->ps_ngbr_avbl->u1_mb_d)
                    && (u4_constrained_intra_pred ? ps_proc->s_top_left_mb_syntax_ele.u2_is_intra : 1));
    if (u1_mb_d)
    {
        pu1_ngbr_pels_i16[16] = *pu1_mb_d;
    }
    else
    {
        pu1_ngbr_pels_i16[16] = 0;
    }

    i4_ngbr_avbl = (u1_mb_a) + (u1_mb_b << 2) + (u1_mb_d << 1);
    ps_proc->i4_ngbr_avbl_16x16_mb = i4_ngbr_avbl;

    /* set valid intra modes for evaluation */
    u4_valid_intra_modes = u1_valid_intra_modes[i4_ngbr_avbl];

    if (ps_codec->s_cfg.u4_enc_speed_preset == IVE_FAST)
        u4_valid_intra_modes &= ~(1 << PLANE_I16x16);

    /* evaluate b/w HORZ_I16x16, VERT_I16x16 & DC_I16x16 */
    ps_codec->pf_ih264e_evaluate_intra16x16_modes(pu1_curr_mb, pu1_ngbr_pels_i16, pu1_pred_mb_intra_16x16,
                                                  i4_src_strd, i4_pred_strd,
                                                  i4_ngbr_avbl, &u4_intra_mode, &i4_mb_distortion_least,
                                                  u4_valid_intra_modes);

    /* cost = distortion + lambda*rate */
    i4_mb_cost_least = i4_mb_distortion_least;

    if ((( (u4_valid_intra_modes >> 3) & 1) != 0) && (ps_codec->s_cfg.u4_enc_speed_preset != IVE_FASTEST ||
                    ps_proc->i4_slice_type == ISLICE))
    {
        /* intra prediction for PLANE mode*/
        (ps_codec->apf_intra_pred_16_l)[PLANE_I16x16](pu1_ngbr_pels_i16, pu1_pred_mb_intra_16x16_plane, 0, i4_pred_strd, i4_ngbr_avbl);

        /* evaluate distortion between the actual blk and the estimated blk for the given mode */
        ps_codec->apf_compute_sad_16x16[u4_enable_fast_sad](pu1_curr_mb, pu1_pred_mb_intra_16x16_plane, i4_src_strd, i4_pred_strd, i4_mb_cost_least, &i4_mb_distortion);

        /* cost = distortion + lambda*rate */
        i4_mb_cost = i4_mb_distortion;

        /* update the least cost information if necessary */
        if(i4_mb_cost < i4_mb_distortion_least)
        {
            u4_intra_mode = PLANE_I16x16;

            i4_mb_cost_least = i4_mb_cost;
            i4_mb_distortion_least = i4_mb_distortion;
        }
    }

    u4_best_intra_16x16_mode = u4_intra_mode;

    DEBUG("%d partition cost, %d intra mode\n", i4_mb_cost_least * 32, u4_best_intra_16x16_mode);

    ps_proc->u1_l_i16_mode = u4_best_intra_16x16_mode;

    /* cost = distortion + lambda*rate */
    i4_mb_cost_least    = i4_mb_distortion_least + u4_lambda*u1_uev_codelength[offset + u4_best_intra_16x16_mode];


    /* update the type of the mb if necessary */
    if (i4_mb_cost_least < ps_proc->i4_mb_cost)
    {
        ps_proc->i4_mb_cost = i4_mb_cost_least;
        ps_proc->i4_mb_distortion = i4_mb_distortion_least;
        ps_proc->u4_mb_type = I16x16;
    }

    return ;
}


/**
******************************************************************************
*
* @brief
*  evaluate best intra 8x8 mode (rate distortion opt on)
*
* @par Description
*  This function evaluates all the possible intra 8x8 modes and finds the mode
*  that best represents the macro-block (least distortion) and occupies fewer
*  bits in the bit-stream.
*
* @param[in]    ps_proc_ctxt
*  pointer to proc ctxt
*
* @remarks Ideally the cost of encoding a macroblock is calculated as
*  (distortion + lambda*rate). Where distortion is SAD/SATD,... between the
*  input block and the reconstructed block and rate is the number of bits taken
*  to place the macroblock in the bit-stream. In this routine the rate does not
*  exactly point to the total number of bits it takes, rather it points to header
*  bits necessary for encoding the macroblock. Assuming the deltaQP, cbp bits
*  and residual bits fall in to texture bits the number of bits taken to encoding
*  mbtype is considered as rate, we compute cost. Further we will approximate
*  the distortion as the deviation b/w input and the predicted block as opposed
*  to input and reconstructed block.
*
*  NOTE: TODO: This function needs to be tested
*
*  @return      none
*
******************************************************************************
*/
void ih264e_evaluate_intra8x8_modes_for_least_cost_rdoptoff(process_ctxt_t *ps_proc)
{
    /* Codec Context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* SAD(distortion metric) of an 4x4 block */
    WORD32 i4_partition_distortion, i4_partition_distortion_least = INT_MAX, i4_total_distortion = 0;

    /* lambda */
    UWORD32 u4_lambda = ps_proc->u4_lambda;

    /* cost = distortion + lambda*rate */
    WORD32 i4_partition_cost, i4_partition_cost_least, i4_total_cost = u4_lambda;

    /* cost due to mbtype */
    UWORD32 u4_cost_one_bit = u4_lambda, u4_cost_four_bits = 4 * u4_lambda;

    /* intra mode */
    UWORD32 u4_intra_mode, u4_best_intra_8x8_mode = DC_I8x8, u4_estimated_intra_8x8_mode;

    /* neighbor pels for intra prediction */
    UWORD8 *pu1_ngbr_pels_i8 = ps_proc->au1_ngbr_pels;

    /* pointer to curr partition */
    UWORD8 *pu1_mb_curr;

    /* pointer to prediction macro block */
    UWORD8 *pu1_pred_mb = ps_proc->pu1_pred_mb;

    /* strides */
    WORD32 i4_src_strd = ps_proc->i4_src_strd;
    WORD32 i4_pred_strd = ps_proc->i4_pred_strd;

    /* neighbors left, top, top right, top left */
    UWORD8 *pu1_mb_a;
    UWORD8 *pu1_mb_b;
    UWORD8 *pu1_mb_d;

    /* neighbor availability */
    WORD32 i4_ngbr_avbl;
    block_neighbors_t s_ngbr_avbl;

    /* temp vars */
    UWORD32  b8, u4_pix_x, u4_pix_y;
    UWORD32 u4_constrained_intra_pred = ps_proc->ps_codec->s_cfg.u4_constrained_intra_pred;
    block_neighbors_t s_ngbr_avbl_MB;

    /* ngbr mb syntax information */
    UWORD8 *pu1_top_mb_intra_modes = ps_proc->pu1_top_mb_intra_modes + (ps_proc->i4_mb_x << 4);
    mb_info_t *ps_top_mb_syn_ele = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x;
    mb_info_t *ps_top_right_mb_syn_ele = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x;
    /* valid intra modes map */
    UWORD32 u4_valid_intra_modes;

    if (ps_proc->ps_ngbr_avbl->u1_mb_c)
    {
        ps_top_right_mb_syn_ele = ps_proc->ps_top_row_mb_syntax_ele + (ps_proc->i4_mb_x + 1);
    }
    /* left pels */
    s_ngbr_avbl_MB.u1_mb_a = ((ps_proc->ps_ngbr_avbl->u1_mb_a)
                                  && (u4_constrained_intra_pred ? ps_proc->s_left_mb_syntax_ele.u2_is_intra : 1));

    /* top pels */
    s_ngbr_avbl_MB.u1_mb_b = ((ps_proc->ps_ngbr_avbl->u1_mb_b)
                                  && (u4_constrained_intra_pred ? ps_top_mb_syn_ele->u2_is_intra : 1));

    /* topleft pels */
    s_ngbr_avbl_MB.u1_mb_d = ((ps_proc->ps_ngbr_avbl->u1_mb_d)
                                  && (u4_constrained_intra_pred ? ps_proc->s_top_left_mb_syntax_ele.u2_is_intra : 1));

    /* top right */
    s_ngbr_avbl_MB.u1_mb_c = ((ps_proc->ps_ngbr_avbl->u1_mb_c)
                                  && (u4_constrained_intra_pred ? ps_top_right_mb_syn_ele->u2_is_intra : 1));


    for(b8 = 0; b8 < 4; b8++)
    {
        u4_pix_x = (b8 & 0x01) << 3;
        u4_pix_y = (b8 >> 1) << 3;

        pu1_mb_curr = ps_proc->pu1_src_buf_luma + u4_pix_x + (u4_pix_y * i4_src_strd);
        /* when rdopt is off, we use the input as reference for constructing prediction buffer */
        /* as opposed to using the recon pels. (open loop intra prediction) */
        pu1_mb_a = pu1_mb_curr - 1; /* pointer to left macro block */
        pu1_mb_b = pu1_mb_curr - i4_src_strd; /* pointer to top macro block */
        pu1_mb_d = pu1_mb_b - 1; /* pointer to top left macro block */

        /* locating neighbors that are available for prediction */
        /* TODO : update the neighbor availability information basing on constrained intra pred information */
        /* TODO : i4_ngbr_avbl is only being used in DC mode. Can the DC mode be split in to distinct routines */
        /* basing on neighbors available and hence evade the computation of neighbor availability totally. */
        s_ngbr_avbl.u1_mb_a = ih264e_derive_ngbr_avbl_of_mb_partitions(&s_ngbr_avbl_MB, u4_pix_x - 1, u4_pix_y); /* xD = -1, yD = 0 */
        s_ngbr_avbl.u1_mb_b = ih264e_derive_ngbr_avbl_of_mb_partitions(&s_ngbr_avbl_MB, u4_pix_x, u4_pix_y - 1); /* xD = 0, yD = -1 */
        s_ngbr_avbl.u1_mb_c = ih264e_derive_ngbr_avbl_of_mb_partitions(&s_ngbr_avbl_MB, u4_pix_x + 8, u4_pix_y - 1); /* xD = BLK_8x8_SIZE, yD = -1 */
        s_ngbr_avbl.u1_mb_d = ih264e_derive_ngbr_avbl_of_mb_partitions(&s_ngbr_avbl_MB, u4_pix_x - 1, u4_pix_y - 1); /* xD = -1, yD = -1 */

        /* i4_ngbr_avbl = blk_a * LEFT_MB_AVAILABLE_MASK + blk_b * TOP_MB_AVAILABLE_MASK + blk_c * TOP_RIGHT_MB_AVAILABLE_MASK + blk_d * TOP_LEFT_MB_AVAILABLE_MASK */
        i4_ngbr_avbl = (s_ngbr_avbl.u1_mb_a) + (s_ngbr_avbl.u1_mb_d << 1) + (s_ngbr_avbl.u1_mb_b << 2) +  (s_ngbr_avbl.u1_mb_c << 3) +
                        (s_ngbr_avbl.u1_mb_a << 4);
        /* if top partition is available and top right is not available for intra prediction, then */
        /* padd top right samples using top sample and make top right also available */
        /* i4_ngbr_avbl = (s_ngbr_avbl.u1_mb_a) + (s_ngbr_avbl.u1_mb_d << 1) + (s_ngbr_avbl.u1_mb_b << 2) +  ((s_ngbr_avbl.u1_mb_b | s_ngbr_avbl.u1_mb_c) << 3); */
        ps_proc->ai4_neighbor_avail_8x8_subblks[b8] = i4_ngbr_avbl;


        ih264_intra_pred_luma_8x8_mode_ref_filtering(pu1_mb_a, pu1_mb_b, pu1_mb_d, pu1_ngbr_pels_i8,
                                                     i4_src_strd, i4_ngbr_avbl);

        i4_partition_cost_least = INT_MAX;
        /* set valid intra modes for evaluation */
        u4_valid_intra_modes = 0x1ff;

        if (!s_ngbr_avbl.u1_mb_b)
        {
            u4_valid_intra_modes &= ~(1 << VERT_I4x4);
            u4_valid_intra_modes &= ~(1 << DIAG_DL_I4x4);
            u4_valid_intra_modes &= ~(1 << VERT_L_I4x4);
        }
        if (!s_ngbr_avbl.u1_mb_a)
        {
            u4_valid_intra_modes &= ~(1 << HORZ_I4x4);
            u4_valid_intra_modes &= ~(1 << HORZ_U_I4x4);
        }
        if (!s_ngbr_avbl.u1_mb_a || !s_ngbr_avbl.u1_mb_b || !s_ngbr_avbl.u1_mb_d)
        {
            u4_valid_intra_modes &= ~(1 << DIAG_DR_I4x4);
            u4_valid_intra_modes &= ~(1 << VERT_R_I4x4);
            u4_valid_intra_modes &= ~(1 << HORZ_D_I4x4);
        }

        /* estimate the intra 8x8 mode for the current partition (for evaluating cost) */
        if (!s_ngbr_avbl.u1_mb_a || !s_ngbr_avbl.u1_mb_b)
        {
            u4_estimated_intra_8x8_mode = DC_I8x8;
        }
        else
        {
            UWORD32 u4_left_intra_8x8_mode = DC_I8x8;
            UWORD32 u4_top_intra_8x8_mode = DC_I8x8;

            if (u4_pix_x == 0)
            {
                if (ps_proc->s_left_mb_syntax_ele.u2_mb_type == I8x8)
                {
                    u4_left_intra_8x8_mode = ps_proc->au1_left_mb_intra_modes[b8+1];
                }
                else if (ps_proc->s_left_mb_syntax_ele.u2_mb_type == I4x4)
                {
                    u4_left_intra_8x8_mode = ps_proc->au1_left_mb_intra_modes[(b8+1)*4+2];
                }
            }
            else
            {
                u4_left_intra_8x8_mode = ps_proc->au1_intra_luma_mb_8x8_modes[b8-1];
            }

            if (u4_pix_y == 0)
            {
                if (ps_top_mb_syn_ele->u2_mb_type == I8x8)
                {
                    u4_top_intra_8x8_mode = pu1_top_mb_intra_modes[b8+2];
                }
                else if (ps_top_mb_syn_ele->u2_mb_type == I4x4)
                {
                    u4_top_intra_8x8_mode = pu1_top_mb_intra_modes[(b8+2)*4+2];
                }
            }
            else
            {
                u4_top_intra_8x8_mode = ps_proc->au1_intra_luma_mb_8x8_modes[b8-2];
            }

            u4_estimated_intra_8x8_mode = MIN(u4_left_intra_8x8_mode, u4_top_intra_8x8_mode);
        }

        /* perform intra mode 8x8 evaluation */
        for (u4_intra_mode = VERT_I8x8; u4_valid_intra_modes != 0; u4_intra_mode++, u4_valid_intra_modes >>= 1)
        {
            if ( (u4_valid_intra_modes & 1) == 0)
                continue;

            /* intra prediction */
            (ps_codec->apf_intra_pred_8_l)[u4_intra_mode](pu1_ngbr_pels_i8, pu1_pred_mb, 0, i4_pred_strd, i4_ngbr_avbl);

            /* evaluate distortion between the actual blk and the estimated blk for the given mode */
            ime_compute_sad_8x8(pu1_mb_curr, pu1_pred_mb, i4_src_strd, i4_pred_strd, i4_partition_cost_least, &i4_partition_distortion);

            i4_partition_cost = i4_partition_distortion + ((u4_estimated_intra_8x8_mode == u4_intra_mode)?u4_cost_one_bit:u4_cost_four_bits);

            /* update the least cost information if necessary */
            if (i4_partition_cost < i4_partition_cost_least)
            {
                i4_partition_cost_least = i4_partition_cost;
                i4_partition_distortion_least = i4_partition_distortion;
                u4_best_intra_8x8_mode = u4_intra_mode;
            }
        }
        /* macroblock distortion */
        i4_total_cost += i4_partition_cost_least;
        i4_total_distortion += i4_partition_distortion_least;
        /* mb partition mode */
        ps_proc->au1_intra_luma_mb_8x8_modes[b8] = u4_best_intra_8x8_mode;

    }

    /* update the type of the mb if necessary */
    if (i4_total_cost < ps_proc->i4_mb_cost)
    {
        ps_proc->i4_mb_cost = i4_total_cost;
        ps_proc->i4_mb_distortion = i4_total_distortion;
        ps_proc->u4_mb_type = I8x8;
    }

    return ;
}


/**
******************************************************************************
*
* @brief
*  evaluate best intra 4x4 mode (rate distortion opt off)
*
* @par Description
*  This function evaluates all the possible intra 4x4 modes and finds the mode
*  that best represents the macro-block (least distortion) and occupies fewer
*  bits in the bit-stream.
*
* @param[in]    ps_proc_ctxt
*  pointer to proc ctxt
*
* @remarks
*  Ideally the cost of encoding a macroblock is calculated as
*  (distortion + lambda*rate). Where distortion is SAD/SATD,... between the
*  input block and the reconstructed block and rate is the number of bits taken
*  to place the macroblock in the bit-stream. In this routine the rate does not
*  exactly point to the total number of bits it takes, rather it points to header
*  bits necessary for encoding the macroblock. Assuming the deltaQP, cbp bits
*  and residual bits fall in to texture bits the number of bits taken to encoding
*  mbtype is considered as rate, we compute cost. Further we will approximate
*  the distortion as the deviation b/w input and the predicted block as opposed
*  to input and reconstructed block.
*
*  NOTE: As per the Document JVT-O079, for the whole intra 4x4 macroblock,
*  24*lambda is added to the SAD before comparison with the best SAD for
*  inter prediction. This is an empirical value to prevent using too many intra
*  blocks.
*
* @return      none
*
******************************************************************************
*/
void ih264e_evaluate_intra4x4_modes_for_least_cost_rdoptoff(process_ctxt_t *ps_proc)
{
    /* Codec Context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* SAD(distortion metric) of an 4x4 block */
    WORD32 i4_partition_distortion_least = INT_MAX, i4_total_distortion = 0;

    /* lambda */
    UWORD32 u4_lambda = ps_proc->u4_lambda;

    /* cost = distortion + lambda*rate */
    WORD32 i4_partition_cost_least, i4_total_cost = (24 + 1) * u4_lambda;

    /* cost due to mbtype */
    UWORD32 u4_cost_one_bit = u4_lambda, u4_cost_four_bits = 4 * u4_lambda;

    /* intra mode */
    UWORD32 u4_best_intra_4x4_mode = DC_I4x4, u4_estimated_intra_4x4_mode;

    /* neighbor pels for intra prediction */
    UWORD8 *pu1_ngbr_pels_i4 = ps_proc->au1_ngbr_pels;

    /* pointer to curr partition */
    UWORD8 *pu1_mb_curr;

    /* pointer to prediction macro block */
    UWORD8 *pu1_pred_mb = ps_proc->pu1_pred_mb;

    /* strides */
    WORD32 i4_src_strd = ps_proc->i4_src_strd;
    WORD32 i4_pred_strd = ps_proc->i4_pred_strd;

    /* neighbors left, top, top right, top left */
    UWORD8 *pu1_mb_a;
    UWORD8 *pu1_mb_b;
    UWORD8 *pu1_mb_c;
    UWORD8 *pu1_mb_d;

    /* neighbor availability */
    WORD32 i4_ngbr_avbl;
    block_neighbors_t s_ngbr_avbl;

    /* temp vars */
    UWORD32 i, b8, b4, u4_blk_x, u4_blk_y, u4_pix_x, u4_pix_y;

    /* scan order inside 4x4 block */
    const UWORD8 u1_scan_order[16] = {0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15};

    /* ngbr sub mb modes */
    UWORD8 *pu1_top_mb_intra_modes = ps_proc->pu1_top_mb_intra_modes + (ps_proc->i4_mb_x << 4);
    mb_info_t *ps_top_mb_syn_ele = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x;
    mb_info_t *ps_top_right_mb_syn_ele = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x;

    /* valid intra modes map */
    UWORD32 u4_valid_intra_modes;
    UWORD16 u2_valid_modes[8] = {4, 262, 4, 262, 141, 399, 141, 511};

    UWORD32 u4_constrained_intra_pred = ps_proc->ps_codec->s_cfg.u4_constrained_intra_pred;
    UWORD8 u1_mb_a, u1_mb_b, u1_mb_c, u1_mb_d;
    if (ps_proc->ps_ngbr_avbl->u1_mb_c)
    {
        ps_top_right_mb_syn_ele = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x + 1;
    }
    /* left pels */
    u1_mb_a = ((ps_proc->ps_ngbr_avbl->u1_mb_a)
                    && (u4_constrained_intra_pred ? ps_proc->s_left_mb_syntax_ele.u2_is_intra : 1));

    /* top pels */
    u1_mb_b = ((ps_proc->ps_ngbr_avbl->u1_mb_b)
                    && (u4_constrained_intra_pred ? ps_top_mb_syn_ele->u2_is_intra : 1));

    /* topleft pels */
    u1_mb_d = ((ps_proc->ps_ngbr_avbl->u1_mb_d)
                    && (u4_constrained_intra_pred ? ps_proc->s_top_left_mb_syntax_ele.u2_is_intra : 1));

    /* top right */
    u1_mb_c = ((ps_proc->ps_ngbr_avbl->u1_mb_c)
                    && (u4_constrained_intra_pred ? ps_top_right_mb_syn_ele->u2_is_intra : 1));

    i4_ngbr_avbl = (u1_mb_a) + (u1_mb_d << 1) + (u1_mb_b << 2) + (u1_mb_c << 3);
    memcpy(ps_proc->au1_ngbr_avbl_4x4_subblks, gau1_ih264_4x4_ngbr_avbl[i4_ngbr_avbl], 16);

    for (b8 = 0; b8 < 4; b8++)
    {
        u4_blk_x = (b8 & 0x01) << 3;
        u4_blk_y = (b8 >> 1) << 3;
        for (b4 = 0; b4 < 4; b4++)
        {
            u4_pix_x = u4_blk_x + ((b4 & 0x01) << 2);
            u4_pix_y = u4_blk_y + ((b4 >> 1) << 2);

            pu1_mb_curr = ps_proc->pu1_src_buf_luma + u4_pix_x + (u4_pix_y * i4_src_strd);
            /* when rdopt is off, we use the input as reference for constructing prediction buffer */
            /* as opposed to using the recon pels. (open loop intra prediction) */
            pu1_mb_a = pu1_mb_curr - 1; /* pointer to left macro block */
            pu1_mb_b = pu1_mb_curr - i4_src_strd; /* pointer to top macro block */
            pu1_mb_c = pu1_mb_b + 4; /* pointer to top macro block */
            pu1_mb_d = pu1_mb_b - 1; /* pointer to top left macro block */

            /* locating neighbors that are available for prediction */
            /* TODO : update the neighbor availability information basing on constrained intra pred information */
            /* TODO : i4_ngbr_avbl is only being used in DC mode. Can the DC mode be split in to distinct routines */
            /* basing on neighbors available and hence evade the computation of neighbor availability totally. */

            i4_ngbr_avbl = ps_proc->au1_ngbr_avbl_4x4_subblks[(b8 << 2) + b4];
            s_ngbr_avbl.u1_mb_a = (i4_ngbr_avbl & 0x1);
            s_ngbr_avbl.u1_mb_d = (i4_ngbr_avbl & 0x2) >> 1;
            s_ngbr_avbl.u1_mb_b = (i4_ngbr_avbl & 0x4) >> 2;
            s_ngbr_avbl.u1_mb_c = (i4_ngbr_avbl & 0x8) >> 3;
            /* set valid intra modes for evaluation */
            u4_valid_intra_modes = u2_valid_modes[i4_ngbr_avbl & 0x7];

            /* if top partition is available and top right is not available for intra prediction, then */
            /* padd top right samples using top sample and make top right also available */
            /* i4_ngbr_avbl = (s_ngbr_avbl.u1_mb_a) + (s_ngbr_avbl.u1_mb_d << 1) + (s_ngbr_avbl.u1_mb_b << 2) + ((s_ngbr_avbl.u1_mb_b | s_ngbr_avbl.u1_mb_c) << 3); */

            /* gather prediction pels from the neighbors */
            if (s_ngbr_avbl.u1_mb_a)
            {
                for(i = 0; i < 4; i++)
                    pu1_ngbr_pels_i4[4 - 1 -i] = pu1_mb_a[i * i4_src_strd];
            }
            else
            {
                memset(pu1_ngbr_pels_i4, 0, 4);
            }

            if (s_ngbr_avbl.u1_mb_b)
            {
                memcpy(pu1_ngbr_pels_i4 + 4 + 1, pu1_mb_b, 4);
            }
            else
            {
                memset(pu1_ngbr_pels_i4 + 5, 0, 4);
            }

            if (s_ngbr_avbl.u1_mb_d)
                pu1_ngbr_pels_i4[4] = *pu1_mb_d;
            else
                pu1_ngbr_pels_i4[4] = 0;

            if (s_ngbr_avbl.u1_mb_c)
            {
                memcpy(pu1_ngbr_pels_i4 + 8 + 1, pu1_mb_c, 4);
            }
            else if (s_ngbr_avbl.u1_mb_b)
            {
                memset(pu1_ngbr_pels_i4 + 8 + 1, pu1_ngbr_pels_i4[8], 4);
                s_ngbr_avbl.u1_mb_c = s_ngbr_avbl.u1_mb_b;
            }

            i4_partition_cost_least = INT_MAX;

            /* predict the intra 4x4 mode for the current partition (for evaluating cost) */
            if (!s_ngbr_avbl.u1_mb_a || !s_ngbr_avbl.u1_mb_b)
            {
                u4_estimated_intra_4x4_mode = DC_I4x4;
            }
            else
            {
                UWORD32 u4_left_intra_4x4_mode = DC_I4x4;
                UWORD32 u4_top_intra_4x4_mode = DC_I4x4;

                if (u4_pix_x == 0)
                {
                    if (ps_proc->s_left_mb_syntax_ele.u2_mb_type == I4x4)
                    {
                        u4_left_intra_4x4_mode = ps_proc->au1_left_mb_intra_modes[u1_scan_order[3 + u4_pix_y]];
                    }
                    else if (ps_proc->s_left_mb_syntax_ele.u2_mb_type == I8x8)
                    {
                        u4_left_intra_4x4_mode = ps_proc->au1_left_mb_intra_modes[b8 + 1];
                    }
                }
                else
                {
                    u4_left_intra_4x4_mode = ps_proc->au1_intra_luma_mb_4x4_modes[u1_scan_order[(u4_pix_x >> 2) + u4_pix_y - 1]];
                }

                if (u4_pix_y == 0)
                {
                    if (ps_top_mb_syn_ele->u2_mb_type == I4x4)
                    {
                        u4_top_intra_4x4_mode = pu1_top_mb_intra_modes[u1_scan_order[12 + (u4_pix_x >> 2)]];
                    }
                    else if (ps_top_mb_syn_ele->u2_mb_type == I8x8)
                    {
                        u4_top_intra_4x4_mode = pu1_top_mb_intra_modes[b8 + 2];
                    }
                }
                else
                {
                    u4_top_intra_4x4_mode = ps_proc->au1_intra_luma_mb_4x4_modes[u1_scan_order[(u4_pix_x >> 2) + u4_pix_y - 4]];
                }

                u4_estimated_intra_4x4_mode = MIN(u4_left_intra_4x4_mode, u4_top_intra_4x4_mode);
            }

            ps_proc->au1_predicted_intra_luma_mb_4x4_modes[(b8 << 2) + b4] = u4_estimated_intra_4x4_mode;

            /* mode evaluation and prediction */
            ps_codec->pf_ih264e_evaluate_intra_4x4_modes(pu1_mb_curr,
                                                         pu1_ngbr_pels_i4,
                                                         pu1_pred_mb, i4_src_strd,
                                                         i4_pred_strd, i4_ngbr_avbl,
                                                         &u4_best_intra_4x4_mode,
                                                         &i4_partition_cost_least,
                                                         u4_valid_intra_modes,
                                                         u4_lambda,
                                                         u4_estimated_intra_4x4_mode);


            i4_partition_distortion_least = i4_partition_cost_least - ((u4_estimated_intra_4x4_mode == u4_best_intra_4x4_mode) ? u4_cost_one_bit : u4_cost_four_bits);

            DEBUG("%d partition cost, %d intra mode\n", i4_partition_cost_least, u4_best_intra_4x4_mode);
            /* macroblock distortion */
            i4_total_distortion += i4_partition_distortion_least;
            i4_total_cost += i4_partition_cost_least;
            /* mb partition mode */
            ps_proc->au1_intra_luma_mb_4x4_modes[(b8 << 2) + b4] = u4_best_intra_4x4_mode;
        }
    }

    /* update the type of the mb if necessary */
    if (i4_total_cost < ps_proc->i4_mb_cost)
    {
        ps_proc->i4_mb_cost = i4_total_cost;
        ps_proc->i4_mb_distortion = i4_total_distortion;
        ps_proc->u4_mb_type = I4x4;
    }

    return ;
}

/**
******************************************************************************
*
* @brief evaluate best intra 4x4 mode (rate distortion opt on)
*
* @par Description
*  This function evaluates all the possible intra 4x4 modes and finds the mode
*  that best represents the macro-block (least distortion) and occupies fewer
*  bits in the bit-stream.
*
* @param[in]    ps_proc_ctxt
*  pointer to proc ctxt
*
* @remarks
*  Ideally the cost of encoding a macroblock is calculated as
*  (distortion + lambda*rate). Where distortion is SAD/SATD,... between the
*  input block and the reconstructed block and rate is the number of bits taken
*  to place the macroblock in the bit-stream. In this routine the rate does not
*  exactly point to the total number of bits it takes, rather it points to header
*  bits necessary for encoding the macroblock. Assuming the deltaQP, cbp bits
*  and residual bits fall in to texture bits the number of bits taken to encoding
*  mbtype is considered as rate, we compute cost. Further we will approximate
*  the distortion as the deviation b/w input and the predicted block as opposed
*  to input and reconstructed block.
*
*  NOTE: As per the Document JVT-O079, for the whole intra 4x4 macroblock,
*  24*lambda is added to the SAD before comparison with the best SAD for
*  inter prediction. This is an empirical value to prevent using too many intra
*  blocks.
*
* @return      none
*
******************************************************************************
*/
void ih264e_evaluate_intra4x4_modes_for_least_cost_rdopton(process_ctxt_t *ps_proc)
{
    /* Codec Context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* SAD(distortion metric) of an 4x4 block */
    WORD32 i4_partition_distortion_least = INT_MAX, i4_total_distortion = 0;

    /* lambda */
    UWORD32 u4_lambda = ps_proc->u4_lambda;

    /* cost = distortion + lambda*rate */
    WORD32 i4_partition_cost_least, i4_total_cost = (24 + 1) * u4_lambda;

    /* cost due to mbtype */
    UWORD32 u4_cost_one_bit = u4_lambda, u4_cost_four_bits = 4 * u4_lambda;

    /* intra mode */
    UWORD32 u4_best_intra_4x4_mode = DC_I4x4, u4_estimated_intra_4x4_mode;

    /* neighbor pels for intra prediction */
    UWORD8 *pu1_ngbr_pels_i4 = ps_proc->au1_ngbr_pels;

    /* pointer to curr partition */
    UWORD8 *pu1_mb_curr;
    UWORD8 *pu1_mb_ref_left, *pu1_mb_ref_top;
    UWORD8 *pu1_ref_mb_intra_4x4;

    /* pointer to residual macro block */
    WORD16 *pi2_res_mb = ps_proc->pi2_res_buf_intra_4x4;

    /* pointer to prediction macro block */
    UWORD8 *pu1_pred_mb = ps_proc->pu1_pred_mb;

    /* strides */
    WORD32 i4_src_strd = ps_proc->i4_src_strd;
    WORD32 i4_pred_strd = ps_proc->i4_pred_strd;
    WORD32 i4_ref_strd_left, i4_ref_strd_top;

    /* neighbors left, top, top right, top left */
    UWORD8 *pu1_mb_a;
    UWORD8 *pu1_mb_b;
    UWORD8 *pu1_mb_c;
    UWORD8 *pu1_mb_d;

    /* number of non zero coeffs*/
    UWORD8  *pu1_nnz = (UWORD8 *)ps_proc->au4_nnz_intra_4x4;

    /* quantization parameters */
    quant_params_t *ps_qp_params = ps_proc->ps_qp_params[0];

    /* neighbor availability */
    WORD32 i4_ngbr_avbl;
    block_neighbors_t s_ngbr_avbl;

    /* temp vars */
    UWORD32 i, b8, b4, u4_blk_x, u4_blk_y, u4_pix_x, u4_pix_y;

    /* scan order inside 4x4 block */
    const UWORD8 u1_scan_order[16] = {0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15};

    /* ngbr sub mb modes */
    UWORD8 *pu1_top_mb_intra_modes = ps_proc->pu1_top_mb_intra_modes + (ps_proc->i4_mb_x << 4);
    mb_info_t *ps_top_mb_syn_ele = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x;
    mb_info_t *ps_top_right_mb_syn_ele = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x;

    /* valid intra modes map */
    UWORD32 u4_valid_intra_modes;
    UWORD16 u2_valid_modes[8] = {4, 262, 4, 262, 141, 399, 141, 511};

    /* Dummy variable for 4x4 trans function */
    WORD16 i2_dc_dummy;
    UWORD8 u1_mb_a, u1_mb_b, u1_mb_c, u1_mb_d;
    UWORD32 u4_constrained_intra_pred = ps_proc->ps_codec->s_cfg.u4_constrained_intra_pred;

    /* compute ngbr availability for sub blks */
    if (ps_proc->ps_ngbr_avbl->u1_mb_c)
    {
        ps_top_right_mb_syn_ele = ps_proc->ps_top_row_mb_syntax_ele + (ps_proc->i4_mb_x + 1);
    }

    /* left pels */
    u1_mb_a = ((ps_proc->ps_ngbr_avbl->u1_mb_a)
                    && (u4_constrained_intra_pred ? ps_proc->s_left_mb_syntax_ele.u2_is_intra : 1));

       /* top pels */
    u1_mb_b = ((ps_proc->ps_ngbr_avbl->u1_mb_b)
                    && (u4_constrained_intra_pred ? ps_top_mb_syn_ele->u2_is_intra : 1));

       /* topleft pels */
    u1_mb_d = ((ps_proc->ps_ngbr_avbl->u1_mb_d)
                    && (u4_constrained_intra_pred ? ps_proc->s_top_left_mb_syntax_ele.u2_is_intra : 1));

       /* top right pels */
    u1_mb_c = ((ps_proc->ps_ngbr_avbl->u1_mb_c)
                    && (u4_constrained_intra_pred ? ps_top_right_mb_syn_ele->u2_is_intra : 1));

    i4_ngbr_avbl = (u1_mb_a) + (u1_mb_d << 1) + (u1_mb_b << 2) + (u1_mb_c << 3);
    memcpy(ps_proc->au1_ngbr_avbl_4x4_subblks, gau1_ih264_4x4_ngbr_avbl[i4_ngbr_avbl], 16);

    for(b8 = 0; b8 < 4; b8++)
    {
        u4_blk_x = (b8 & 0x01) << 3;
        u4_blk_y = (b8 >> 1) << 3;
        for(b4 = 0; b4 < 4; b4++, pu1_nnz++, pi2_res_mb += MB_SIZE)
        {
            u4_pix_x = u4_blk_x + ((b4 & 0x01) << 2);
            u4_pix_y = u4_blk_y + ((b4 >> 1) << 2);

            pu1_ref_mb_intra_4x4 = ps_proc->pu1_ref_mb_intra_4x4 + u4_pix_x + (u4_pix_y * i4_pred_strd);
            pu1_mb_curr = ps_proc->pu1_src_buf_luma + u4_pix_x + (u4_pix_y * i4_src_strd);
            if (u4_pix_x == 0)
            {
                i4_ref_strd_left = ps_proc->i4_rec_strd;
                pu1_mb_ref_left = ps_proc->pu1_rec_buf_luma + u4_pix_x + (u4_pix_y * i4_ref_strd_left);
            }
            else
            {
                i4_ref_strd_left = i4_pred_strd;
                pu1_mb_ref_left = pu1_ref_mb_intra_4x4;
            }
            if (u4_pix_y == 0)
            {
                i4_ref_strd_top = ps_proc->i4_rec_strd;
                pu1_mb_ref_top = ps_proc->pu1_rec_buf_luma + u4_pix_x + (u4_pix_y * i4_ref_strd_top);
            }
            else
            {
                i4_ref_strd_top = i4_pred_strd;
                pu1_mb_ref_top = pu1_ref_mb_intra_4x4;
            }

            pu1_mb_a = pu1_mb_ref_left - 1; /* pointer to left macro block */
            pu1_mb_b = pu1_mb_ref_top - i4_ref_strd_top; /* pointer to top macro block */
            pu1_mb_c = pu1_mb_b + 4; /* pointer to top right macro block */
            if (u4_pix_y == 0)
                pu1_mb_d = pu1_mb_b - 1;
            else
                pu1_mb_d = pu1_mb_a - i4_ref_strd_left; /* pointer to top left macro block */

            /* locating neighbors that are available for prediction */
            /* TODO : update the neighbor availability information basing on constrained intra pred information */
            /* TODO : i4_ngbr_avbl is only being used in DC mode. Can the DC mode be split in to distinct routines */
            /* basing on neighbors available and hence evade the computation of neighbor availability totally. */

            i4_ngbr_avbl = ps_proc->au1_ngbr_avbl_4x4_subblks[(b8 << 2) + b4];
            s_ngbr_avbl.u1_mb_a = (i4_ngbr_avbl & 0x1);
            s_ngbr_avbl.u1_mb_d = (i4_ngbr_avbl & 0x2) >> 1;
            s_ngbr_avbl.u1_mb_b = (i4_ngbr_avbl & 0x4) >> 2;
            s_ngbr_avbl.u1_mb_c = (i4_ngbr_avbl & 0x8) >> 3;
            /* set valid intra modes for evaluation */
            u4_valid_intra_modes = u2_valid_modes[i4_ngbr_avbl & 0x7];

            /* if top partition is available and top right is not available for intra prediction, then */
            /* padd top right samples using top sample and make top right also available */
            /* i4_ngbr_avbl = (s_ngbr_avbl.u1_mb_a) + (s_ngbr_avbl.u1_mb_d << 1) + (s_ngbr_avbl.u1_mb_b << 2) + ((s_ngbr_avbl.u1_mb_b | s_ngbr_avbl.u1_mb_c) << 3); */

            /* gather prediction pels from the neighbors */
            if (s_ngbr_avbl.u1_mb_a)
            {
                for(i = 0; i < 4; i++)
                    pu1_ngbr_pels_i4[4 - 1 -i] = pu1_mb_a[i * i4_ref_strd_left];
            }
            else
            {
                memset(pu1_ngbr_pels_i4,0,4);
            }
            if(s_ngbr_avbl.u1_mb_b)
            {
                memcpy(pu1_ngbr_pels_i4 + 4 + 1, pu1_mb_b, 4);
            }
            else
            {
                memset(pu1_ngbr_pels_i4 + 4 + 1, 0, 4);
            }
            if (s_ngbr_avbl.u1_mb_d)
                pu1_ngbr_pels_i4[4] = *pu1_mb_d;
            else
                pu1_ngbr_pels_i4[4] = 0;
            if (s_ngbr_avbl.u1_mb_c)
            {
                memcpy(pu1_ngbr_pels_i4 + 8 + 1, pu1_mb_c, 4);
            }
            else if (s_ngbr_avbl.u1_mb_b)
            {
                memset(pu1_ngbr_pels_i4 + 8 + 1, pu1_ngbr_pels_i4[8], 4);
                s_ngbr_avbl.u1_mb_c = s_ngbr_avbl.u1_mb_b;
            }

            i4_partition_cost_least = INT_MAX;

            /* predict the intra 4x4 mode for the current partition (for evaluating cost) */
            if (!s_ngbr_avbl.u1_mb_a || !s_ngbr_avbl.u1_mb_b)
            {
                u4_estimated_intra_4x4_mode = DC_I4x4;
            }
            else
            {
                UWORD32 u4_left_intra_4x4_mode = DC_I4x4;
                UWORD32 u4_top_intra_4x4_mode = DC_I4x4;

                if (u4_pix_x == 0)
                {
                    if (ps_proc->s_left_mb_syntax_ele.u2_mb_type == I4x4)
                    {
                        u4_left_intra_4x4_mode = ps_proc->au1_left_mb_intra_modes[u1_scan_order[3 + u4_pix_y]];
                    }
                    else if (ps_proc->s_left_mb_syntax_ele.u2_mb_type == I8x8)
                    {
                        u4_left_intra_4x4_mode = ps_proc->au1_left_mb_intra_modes[b8 + 1];
                    }
                }
                else
                {
                    u4_left_intra_4x4_mode = ps_proc->au1_intra_luma_mb_4x4_modes[u1_scan_order[(u4_pix_x >> 2) + u4_pix_y - 1]];
                }

                if (u4_pix_y == 0)
                {
                    if (ps_top_mb_syn_ele->u2_mb_type == I4x4)
                    {
                        u4_top_intra_4x4_mode = pu1_top_mb_intra_modes[u1_scan_order[12 + (u4_pix_x >> 2)]];
                    }
                    else if (ps_top_mb_syn_ele->u2_mb_type == I8x8)
                    {
                        u4_top_intra_4x4_mode = pu1_top_mb_intra_modes[b8 + 2];
                    }
                }
                else
                {
                    u4_top_intra_4x4_mode = ps_proc->au1_intra_luma_mb_4x4_modes[u1_scan_order[(u4_pix_x >> 2) + u4_pix_y - 4]];
                }

                u4_estimated_intra_4x4_mode = MIN(u4_left_intra_4x4_mode, u4_top_intra_4x4_mode);
            }

            ps_proc->au1_predicted_intra_luma_mb_4x4_modes[(b8 << 2) + b4] = u4_estimated_intra_4x4_mode;

            /*mode evaluation and prediction*/
            ps_codec->pf_ih264e_evaluate_intra_4x4_modes(pu1_mb_curr,
                                                         pu1_ngbr_pels_i4,
                                                         pu1_pred_mb, i4_src_strd,
                                                         i4_pred_strd, i4_ngbr_avbl,
                                                         &u4_best_intra_4x4_mode,
                                                         &i4_partition_cost_least,
                                                         u4_valid_intra_modes,
                                                         u4_lambda,
                                                         u4_estimated_intra_4x4_mode);


            i4_partition_distortion_least = i4_partition_cost_least - ((u4_estimated_intra_4x4_mode == u4_best_intra_4x4_mode)?u4_cost_one_bit:u4_cost_four_bits);

            DEBUG("%d partition cost, %d intra mode\n", i4_partition_cost_least, u4_best_intra_4x4_mode);

            /* macroblock distortion */
            i4_total_distortion += i4_partition_distortion_least;
            i4_total_cost += i4_partition_cost_least;

            /* mb partition mode */
            ps_proc->au1_intra_luma_mb_4x4_modes[(b8 << 2) + b4] = u4_best_intra_4x4_mode;


            /********************************************************/
            /*  error estimation,                                   */
            /*  transform                                           */
            /*  quantization                                        */
            /********************************************************/
            ps_codec->pf_resi_trans_quant_4x4(pu1_mb_curr, pu1_pred_mb,
                                              pi2_res_mb, i4_src_strd,
                                              i4_pred_strd,
                                              /* No op stride, this implies a buff of lenght 1x16 */
                                              ps_qp_params->pu2_scale_mat,
                                              ps_qp_params->pu2_thres_mat,
                                              ps_qp_params->u1_qbits,
                                              ps_qp_params->u4_dead_zone,
                                              pu1_nnz, &i2_dc_dummy);

            /********************************************************/
            /*  ierror estimation,                                  */
            /*  itransform                                          */
            /*  iquantization                                       */
            /********************************************************/
            ps_codec->pf_iquant_itrans_recon_4x4(pi2_res_mb, pu1_pred_mb,
                                                 pu1_ref_mb_intra_4x4,
                                                 i4_pred_strd, i4_pred_strd,
                                                 ps_qp_params->pu2_iscale_mat,
                                                 ps_qp_params->pu2_weigh_mat,
                                                 ps_qp_params->u1_qp_div,
                                                 ps_proc->pv_scratch_buff, 0,
                                                 NULL);
        }
    }

    /* update the type of the mb if necessary */
    if (i4_total_cost < ps_proc->i4_mb_cost)
    {
        ps_proc->i4_mb_cost = i4_total_cost;
        ps_proc->i4_mb_distortion = i4_total_distortion;
        ps_proc->u4_mb_type = I4x4;
    }

    return ;
}

/**
******************************************************************************
*
* @brief
*  evaluate best chroma intra 8x8 mode (rate distortion opt off)
*
* @par Description
*  This function evaluates all the possible chroma intra 8x8 modes and finds
*  the mode that best represents the macroblock (least distortion) and occupies
*  fewer bits in the bitstream.
*
* @param[in] ps_proc_ctxt
*  pointer to macroblock context (handle)
*
* @remarks
*  For chroma best intra pred mode is calculated based only on SAD
*
* @returns none
*
******************************************************************************
*/

void ih264e_evaluate_chroma_intra8x8_modes_for_least_cost_rdoptoff(process_ctxt_t *ps_proc)
{
    /* Codec Context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* SAD(distortion metric) of an 8x8 block */
    WORD32 i4_mb_distortion, i4_chroma_mb_distortion;

    /* intra mode */
    UWORD32  u4_best_chroma_intra_8x8_mode = DC_CH_I8x8;

    /* neighbor pels for intra prediction */
    UWORD8 *pu1_ngbr_pels_c_i8x8 = ps_proc->au1_ngbr_pels;

    /* pointer to curr macro block */
    UWORD8 *pu1_curr_mb = ps_proc->pu1_src_buf_chroma;
    UWORD8 *pu1_ref_mb = ps_proc->pu1_rec_buf_chroma;

    /* pointer to prediction macro block */
    UWORD8 *pu1_pred_mb = ps_proc->pu1_pred_mb_intra_chroma;
    UWORD8 *pu1_pred_mb_plane = ps_proc->pu1_pred_mb_intra_chroma_plane;

    /* strides */
    WORD32 i4_src_strd_c = ps_proc->i4_src_chroma_strd;
    WORD32 i4_pred_strd = ps_proc->i4_pred_strd;
    WORD32 i4_rec_strd_c = ps_proc->i4_rec_strd;

    /* neighbors left, top, top left */
    UWORD8 *pu1_mb_a = pu1_ref_mb - 2;
    UWORD8 *pu1_mb_b = pu1_ref_mb - i4_rec_strd_c;
    UWORD8 *pu1_mb_d = pu1_mb_b - 2;

    /* neighbor availability */
    const UWORD8  u1_valid_intra_modes[8] = {1, 3, 1, 3, 5, 7, 5, 15};
    WORD32 i4_ngbr_avbl;

    /* valid intra modes map */
    UWORD32 u4_valid_intra_modes;
    mb_info_t *ps_top_mb_syn_ele = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x;

    /* temp var */
    UWORD8 i;
    UWORD32 u4_constrained_intra_pred = ps_proc->ps_codec->s_cfg.u4_constrained_intra_pred;
    UWORD8 u1_mb_a, u1_mb_b, u1_mb_d;
    /* locating neighbors that are available for prediction */

    /* gather prediction pels from the neighbors */
    /* left pels */
    u1_mb_a = ((ps_proc->ps_ngbr_avbl->u1_mb_a)
                    && (u4_constrained_intra_pred ?  ps_proc->s_left_mb_syntax_ele.u2_is_intra : 1));
    if (u1_mb_a)
    {
        for (i = 0; i < 16; i += 2)
        {
            pu1_ngbr_pels_c_i8x8[16 - 2 - i] = pu1_mb_a[(i / 2) * i4_rec_strd_c];
            pu1_ngbr_pels_c_i8x8[16 - 1 - i] = pu1_mb_a[(i / 2) * i4_rec_strd_c + 1];
        }
    }
    else
    {
        ps_codec->pf_mem_set_mul8(pu1_ngbr_pels_c_i8x8, 0, MB_SIZE);
    }

    /* top pels */
    u1_mb_b = ((ps_proc->ps_ngbr_avbl->u1_mb_b)
                    && (u4_constrained_intra_pred ? ps_top_mb_syn_ele->u2_is_intra : 1));
    if (u1_mb_b)
    {
        ps_codec->pf_mem_cpy_mul8(&pu1_ngbr_pels_c_i8x8[18], pu1_mb_b, 16);
    }
    else
    {
        ps_codec->pf_mem_set_mul8((pu1_ngbr_pels_c_i8x8 + 18), 0, MB_SIZE);
    }

    /* top left pels */
    u1_mb_d = ((ps_proc->ps_ngbr_avbl->u1_mb_d)
                    && (u4_constrained_intra_pred ? ps_proc->s_top_left_mb_syntax_ele.u2_is_intra : 1));
    if (u1_mb_d)
    {
        pu1_ngbr_pels_c_i8x8[16] = *pu1_mb_d;
        pu1_ngbr_pels_c_i8x8[17] = *(pu1_mb_d + 1);
    }
    i4_ngbr_avbl = (u1_mb_a) + (u1_mb_b << 2) + (u1_mb_d << 1);
    ps_proc->i4_chroma_neighbor_avail_8x8_mb = i4_ngbr_avbl;

    u4_valid_intra_modes = u1_valid_intra_modes[i4_ngbr_avbl];

    if (ps_codec->s_cfg.u4_enc_speed_preset == IVE_FAST)
        u4_valid_intra_modes &= ~(1 << PLANE_CH_I8x8);

    i4_chroma_mb_distortion = INT_MAX;

    /* perform intra mode chroma  8x8 evaluation */
    /* intra prediction */
    ps_codec->pf_ih264e_evaluate_intra_chroma_modes(pu1_curr_mb,
                                                    pu1_ngbr_pels_c_i8x8,
                                                    pu1_pred_mb,
                                                    i4_src_strd_c,
                                                    i4_pred_strd,
                                                    i4_ngbr_avbl,
                                                    &u4_best_chroma_intra_8x8_mode,
                                                    &i4_chroma_mb_distortion,
                                                    u4_valid_intra_modes);

    if (u4_valid_intra_modes & 8)/* if Chroma PLANE is valid*/
    {
        (ps_codec->apf_intra_pred_c)[PLANE_CH_I8x8](pu1_ngbr_pels_c_i8x8, pu1_pred_mb_plane, 0, i4_pred_strd, i4_ngbr_avbl);

        /* evaluate distortion(sad) */
        ps_codec->pf_compute_sad_16x8(pu1_curr_mb, pu1_pred_mb_plane, i4_src_strd_c, i4_pred_strd, i4_chroma_mb_distortion, &i4_mb_distortion);

        /* update the least distortion information if necessary */
        if(i4_mb_distortion < i4_chroma_mb_distortion)
        {
            i4_chroma_mb_distortion = i4_mb_distortion;
            u4_best_chroma_intra_8x8_mode = PLANE_CH_I8x8;
        }
    }

    DEBUG("%d partition cost, %d intra mode\n", i4_chroma_mb_distortion, u4_best_chroma_intra_8x8_mode);

    ps_proc->u1_c_i8_mode = u4_best_chroma_intra_8x8_mode;

    return ;
}


/**
******************************************************************************
*
* @brief
*  Evaluate best intra 16x16 mode (among VERT, HORZ and DC) and do the
*  prediction.
*
* @par Description
*  This function evaluates first three 16x16 modes and compute corresponding sad
*  and return the buffer predicted with best mode.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[in] pu1_ngbr_pels_i16
*  UWORD8 pointer to neighbouring pels
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
* @param[in] u4_n_avblty
*  availability of neighbouring pixels
*
* @param[in] u4_intra_mode
*  Pointer to the variable in which best mode is returned
*
* @param[in] pu4_sadmin
*  Pointer to the variable in which minimum sad is returned
*
* @param[in] u4_valid_intra_modes
*  Says what all modes are valid
*
* @returns      none
*
******************************************************************************
*/
void ih264e_evaluate_intra16x16_modes(UWORD8 *pu1_src,
                                      UWORD8 *pu1_ngbr_pels_i16,
                                      UWORD8 *pu1_dst,
                                      UWORD32 src_strd,
                                      UWORD32 dst_strd,
                                      WORD32 u4_n_avblty,
                                      UWORD32 *u4_intra_mode,
                                      WORD32 *pu4_sadmin,
                                      UWORD32 u4_valid_intra_modes)
{
    UWORD8 *pu1_neighbour;
    UWORD8 *pu1_src_temp = pu1_src;
    UWORD8 left = 0, top = 0;
    WORD32 u4_dcval = 0;
    WORD32 i, j;
    WORD32 i4_sad_vert = INT_MAX, i4_sad_horz = INT_MAX, i4_sad_dc = INT_MAX,
                    i4_min_sad = INT_MAX;
    UWORD8 val;

    left = (u4_n_avblty & LEFT_MB_AVAILABLE_MASK);
    top = (u4_n_avblty & TOP_MB_AVAILABLE_MASK) >> 2;

    /* left available */
    if (left)
    {
        i4_sad_horz = 0;

        for (i = 0; i < 16; i++)
        {
            val = pu1_ngbr_pels_i16[15 - i];

            u4_dcval += val;

            for (j = 0; j < 16; j++)
            {
                i4_sad_horz += ABS(val - pu1_src_temp[j]);
            }

            pu1_src_temp += src_strd;
        }
        u4_dcval += 8;
    }

    pu1_src_temp = pu1_src;
    /* top available */
    if (top)
    {
        i4_sad_vert = 0;

        for (i = 0; i < 16; i++)
        {
            u4_dcval += pu1_ngbr_pels_i16[17 + i];

            for (j = 0; j < 16; j++)
            {
                i4_sad_vert += ABS(pu1_ngbr_pels_i16[17 + j] - pu1_src_temp[j]);
            }
            pu1_src_temp += src_strd;

        }
        u4_dcval += 8;
    }

    u4_dcval = (u4_dcval) >> (3 + left + top);

    pu1_src_temp = pu1_src;

    /* none available */
    u4_dcval += (left == 0) * (top == 0) * 128;

    i4_sad_dc = 0;

    for (i = 0; i < 16; i++)
    {
        for (j = 0; j < 16; j++)
        {
            i4_sad_dc += ABS(u4_dcval - pu1_src_temp[j]);
        }
        pu1_src_temp += src_strd;
    }

    if ((u4_valid_intra_modes & 04) == 0)/* If DC is disabled */
        i4_sad_dc = INT_MAX;

    if ((u4_valid_intra_modes & 01) == 0)/* If VERT is disabled */
        i4_sad_vert = INT_MAX;

    if ((u4_valid_intra_modes & 02) == 0)/* If HORZ is disabled */
        i4_sad_horz = INT_MAX;

    i4_min_sad = MIN3(i4_sad_horz, i4_sad_dc, i4_sad_vert);

    /* Finding Minimum sad and doing corresponding prediction */
    if (i4_min_sad < *pu4_sadmin)
    {
        *pu4_sadmin = i4_min_sad;
        if (i4_min_sad == i4_sad_vert)
        {
            *u4_intra_mode = VERT_I16x16;
            pu1_neighbour = pu1_ngbr_pels_i16 + 17;
            for (j = 0; j < 16; j++)
            {
                memcpy(pu1_dst, pu1_neighbour, MB_SIZE);
                pu1_dst += dst_strd;
            }
        }
        else if (i4_min_sad == i4_sad_horz)
        {
            *u4_intra_mode = HORZ_I16x16;
            for (j = 0; j < 16; j++)
            {
                val = pu1_ngbr_pels_i16[15 - j];
                memset(pu1_dst, val, MB_SIZE);
                pu1_dst += dst_strd;
            }
        }
        else
        {
            *u4_intra_mode = DC_I16x16;
            for (j = 0; j < 16; j++)
            {
                memset(pu1_dst, u4_dcval, MB_SIZE);
                pu1_dst += dst_strd;
            }
        }
    }
    return;
}

/**
******************************************************************************
*
* @brief
*  Evaluate best intra 4x4 mode and perform prediction.
*
* @par Description
*  This function evaluates  4x4 modes and compute corresponding sad
*  and return the buffer predicted with best mode.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[in] pu1_ngbr_pels
*  UWORD8 pointer to neighbouring pels
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
* @param[in] u4_n_avblty
*  availability of neighbouring pixels
*
* @param[in] u4_intra_mode
*  Pointer to the variable in which best mode is returned
*
* @param[in] pu4_sadmin
*  Pointer to the variable in which minimum cost is returned
*
* @param[in] u4_valid_intra_modes
*  Says what all modes are valid
*
* @param[in] u4_lambda
*  Lamda value for computing cost from SAD
*
* @param[in] u4_predictd_mode
*  Predicted mode for cost computation
*
* @returns      none
*
******************************************************************************
*/
void ih264e_evaluate_intra_4x4_modes(UWORD8 *pu1_src,
                                     UWORD8 *pu1_ngbr_pels,
                                     UWORD8 *pu1_dst,
                                     UWORD32 src_strd,
                                     UWORD32 dst_strd,
                                     WORD32 u4_n_avblty,
                                     UWORD32 *u4_intra_mode,
                                     WORD32 *pu4_sadmin,
                                     UWORD32 u4_valid_intra_modes,
                                     UWORD32  u4_lambda,
                                     UWORD32 u4_predictd_mode)
{
    UWORD8 *pu1_src_temp = pu1_src;
    UWORD8 *pu1_pred = pu1_ngbr_pels;
    UWORD8 left = 0, top = 0;
    UWORD8 u1_pred_val = 0;
    UWORD8 u1_pred_vals[4] = {0};
    UWORD8 *pu1_pred_val = NULL;
    /* To store FILT121 operated values*/
    UWORD8 u1_pred_vals_diag_121[15] = {0};
    /* To store FILT11 operated values*/
    UWORD8 u1_pred_vals_diag_11[15] = {0};
    UWORD8 u1_pred_vals_vert_r[8] = {0};
    UWORD8 u1_pred_vals_horz_d[10] = {0};
    UWORD8 u1_pred_vals_horz_u[10] = {0};
    WORD32 u4_dcval = 0;
    WORD32 i4_sad[MAX_I4x4] = {INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX,
                               INT_MAX, INT_MAX, INT_MAX, INT_MAX};

    WORD32 i4_cost[MAX_I4x4] = {INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX,
                                INT_MAX, INT_MAX, INT_MAX, INT_MAX};
    WORD32 i, i4_min_cost = INT_MAX;

    left = (u4_n_avblty & LEFT_MB_AVAILABLE_MASK);
    top = (u4_n_avblty & TOP_MB_AVAILABLE_MASK) >> 2;

    /* Computing SAD */

    /* VERT mode valid */
    if (u4_valid_intra_modes & 1)
    {
        pu1_pred = pu1_ngbr_pels + 5;
        i4_sad[VERT_I4x4] = 0;
        i4_cost[VERT_I4x4] = 0;

        USADA8(pu1_src_temp, pu1_pred, i4_sad[VERT_I4x4]);
        pu1_src_temp += src_strd;
        USADA8(pu1_src_temp, pu1_pred, i4_sad[VERT_I4x4]);
        pu1_src_temp += src_strd;
        USADA8(pu1_src_temp, pu1_pred, i4_sad[VERT_I4x4]);
        pu1_src_temp += src_strd;
        USADA8(pu1_src_temp, pu1_pred, i4_sad[VERT_I4x4]);

        i4_cost[VERT_I4x4] = i4_sad[VERT_I4x4] + ((u4_predictd_mode == VERT_I4x4) ?
                                        u4_lambda : 4 * u4_lambda);
    }

    /* HORZ mode valid */
    if (u4_valid_intra_modes & 2)
    {
        i4_sad[HORZ_I4x4] = 0;
        i4_cost[HORZ_I4x4] =0;
        pu1_src_temp = pu1_src;

        u1_pred_val = pu1_ngbr_pels[3];

        i4_sad[HORZ_I4x4] += ABS(pu1_src_temp[0] - u1_pred_val)
                        + ABS(pu1_src_temp[1] - u1_pred_val)
                        + ABS(pu1_src_temp[2] - u1_pred_val)
                        + ABS(pu1_src_temp[3] - u1_pred_val);
        pu1_src_temp += src_strd;

        u1_pred_val = pu1_ngbr_pels[2];

        i4_sad[HORZ_I4x4] += ABS(pu1_src_temp[0] - u1_pred_val)
                        + ABS(pu1_src_temp[1] - u1_pred_val)
                        + ABS(pu1_src_temp[2] - u1_pred_val)
                        + ABS(pu1_src_temp[3] - u1_pred_val);
        pu1_src_temp += src_strd;

        u1_pred_val = pu1_ngbr_pels[1];

        i4_sad[HORZ_I4x4] += ABS(pu1_src_temp[0] - u1_pred_val)
                        + ABS(pu1_src_temp[1] - u1_pred_val)
                        + ABS(pu1_src_temp[2] - u1_pred_val)
                        + ABS(pu1_src_temp[3] - u1_pred_val);
        pu1_src_temp += src_strd;

        u1_pred_val = pu1_ngbr_pels[0];

        i4_sad[HORZ_I4x4] += ABS(pu1_src_temp[0] - u1_pred_val)
                        + ABS(pu1_src_temp[1] - u1_pred_val)
                        + ABS(pu1_src_temp[2] - u1_pred_val)
                        + ABS(pu1_src_temp[3] - u1_pred_val);

        i4_cost[HORZ_I4x4] = i4_sad[HORZ_I4x4] + ((u4_predictd_mode == HORZ_I4x4) ?
                                        u4_lambda : 4 * u4_lambda);
    }

    /* DC mode valid */
    if (u4_valid_intra_modes & 4)
    {
        i4_sad[DC_I4x4] = 0;
        i4_cost[DC_I4x4] = 0;
        pu1_src_temp = pu1_src;

        if (left)
            u4_dcval = pu1_ngbr_pels[0] + pu1_ngbr_pels[1] + pu1_ngbr_pels[2]
                            + pu1_ngbr_pels[3] + 2;
        if (top)
            u4_dcval += pu1_ngbr_pels[5] + pu1_ngbr_pels[6] + pu1_ngbr_pels[7]
                            + pu1_ngbr_pels[8] + 2;

        u4_dcval = (u4_dcval) ? (u4_dcval >> (1 + left + top)) : 128;

        /* none available */
        memset(u1_pred_vals, u4_dcval, 4);
        USADA8(pu1_src_temp, u1_pred_vals, i4_sad[DC_I4x4]);
        pu1_src_temp += src_strd;
        USADA8(pu1_src_temp, u1_pred_vals, i4_sad[DC_I4x4]);
        pu1_src_temp += src_strd;
        USADA8(pu1_src_temp, u1_pred_vals, i4_sad[DC_I4x4]);
        pu1_src_temp += src_strd;
        USADA8(pu1_src_temp, u1_pred_vals, i4_sad[DC_I4x4]);
        pu1_src_temp += src_strd;

        i4_cost[DC_I4x4] = i4_sad[DC_I4x4] + ((u4_predictd_mode == DC_I4x4) ?
                                        u4_lambda : 4 * u4_lambda);
    }

    /* if modes other than VERT, HORZ and DC are  valid */
    if (u4_valid_intra_modes > 7)
    {
        pu1_pred = pu1_ngbr_pels;
        pu1_pred[13] = pu1_pred[14] = pu1_pred[12];

        /* Performing FILT121 and FILT11 operation for all neighbour values*/
        for (i = 0; i < 13; i++)
        {
            u1_pred_vals_diag_121[i] = FILT121(pu1_pred[0], pu1_pred[1], pu1_pred[2]);
            u1_pred_vals_diag_11[i] = FILT11(pu1_pred[0], pu1_pred[1]);

            pu1_pred++;
        }

        if (u4_valid_intra_modes & 8)/* DIAG_DL */
        {
            i4_sad[DIAG_DL_I4x4] = 0;
            i4_cost[DIAG_DL_I4x4] = 0;
            pu1_src_temp = pu1_src;
            pu1_pred_val = u1_pred_vals_diag_121 + 5;

            USADA8(pu1_src_temp, pu1_pred_val, i4_sad[DIAG_DL_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val + 1), i4_sad[DIAG_DL_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val + 2), i4_sad[DIAG_DL_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val + 3), i4_sad[DIAG_DL_I4x4]);
            pu1_src_temp += src_strd;
            i4_cost[DIAG_DL_I4x4] = i4_sad[DIAG_DL_I4x4] + ((u4_predictd_mode == DIAG_DL_I4x4) ?
                                            u4_lambda : 4 * u4_lambda);
        }

        if (u4_valid_intra_modes & 16)/* DIAG_DR */
        {
            i4_sad[DIAG_DR_I4x4] = 0;
            i4_cost[DIAG_DR_I4x4] = 0;
            pu1_src_temp = pu1_src;
            pu1_pred_val = u1_pred_vals_diag_121 + 3;

            USADA8(pu1_src_temp, pu1_pred_val, i4_sad[DIAG_DR_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val - 1), i4_sad[DIAG_DR_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val - 2), i4_sad[DIAG_DR_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val - 3), i4_sad[DIAG_DR_I4x4]);
            pu1_src_temp += src_strd;
            i4_cost[DIAG_DR_I4x4] = i4_sad[DIAG_DR_I4x4] + ((u4_predictd_mode == DIAG_DR_I4x4) ?
                                            u4_lambda : 4 * u4_lambda);

        }

        if (u4_valid_intra_modes & 32)/* VERT_R mode valid ????*/
        {
            i4_sad[VERT_R_I4x4] = 0;

            pu1_src_temp = pu1_src;
            u1_pred_vals_vert_r[0] = u1_pred_vals_diag_121[2];
            memcpy((u1_pred_vals_vert_r + 1), (u1_pred_vals_diag_11 + 4), 3);
            u1_pred_vals_vert_r[4] = u1_pred_vals_diag_121[1];
            memcpy((u1_pred_vals_vert_r + 5), (u1_pred_vals_diag_121 + 3), 3);

            pu1_pred_val = u1_pred_vals_diag_11 + 4;
            USADA8(pu1_src_temp, pu1_pred_val, i4_sad[VERT_R_I4x4]);
            pu1_pred_val = u1_pred_vals_diag_121 + 3;
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, pu1_pred_val, i4_sad[VERT_R_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (u1_pred_vals_vert_r), i4_sad[VERT_R_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (u1_pred_vals_vert_r + 4),
                   i4_sad[VERT_R_I4x4]);

            i4_cost[VERT_R_I4x4] = i4_sad[VERT_R_I4x4] + ((u4_predictd_mode == VERT_R_I4x4) ?
                                            u4_lambda : 4 * u4_lambda);
        }

        if (u4_valid_intra_modes & 64)/* HORZ_D mode valid ????*/
        {
            i4_sad[HORZ_D_I4x4] = 0;

            pu1_src_temp = pu1_src;
            u1_pred_vals_horz_d[6] = u1_pred_vals_diag_11[3];
            memcpy((u1_pred_vals_horz_d + 7), (u1_pred_vals_diag_121 + 3), 3);
            u1_pred_vals_horz_d[0] = u1_pred_vals_diag_11[0];
            u1_pred_vals_horz_d[1] = u1_pred_vals_diag_121[0];
            u1_pred_vals_horz_d[2] = u1_pred_vals_diag_11[1];
            u1_pred_vals_horz_d[3] = u1_pred_vals_diag_121[1];
            u1_pred_vals_horz_d[4] = u1_pred_vals_diag_11[2];
            u1_pred_vals_horz_d[5] = u1_pred_vals_diag_121[2];

            pu1_pred_val = u1_pred_vals_horz_d;
            USADA8(pu1_src_temp, (pu1_pred_val + 6), i4_sad[HORZ_D_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val + 4), i4_sad[HORZ_D_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val + 2), i4_sad[HORZ_D_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val), i4_sad[HORZ_D_I4x4]);

            i4_cost[HORZ_D_I4x4] = i4_sad[HORZ_D_I4x4] + ((u4_predictd_mode == HORZ_D_I4x4) ?
                                            u4_lambda : 4 * u4_lambda);
        }

        if (u4_valid_intra_modes & 128)/* VERT_L mode valid ????*/
        {
            i4_sad[VERT_L_I4x4] = 0;
            pu1_src_temp = pu1_src;
            pu1_pred_val = u1_pred_vals_diag_11 + 5;
            USADA8(pu1_src_temp, (pu1_pred_val), i4_sad[VERT_L_I4x4]);
            pu1_src_temp += src_strd;
            pu1_pred_val = u1_pred_vals_diag_121 + 5;
            USADA8(pu1_src_temp, (pu1_pred_val), i4_sad[VERT_L_I4x4]);
            pu1_src_temp += src_strd;
            pu1_pred_val = u1_pred_vals_diag_11 + 6;
            USADA8(pu1_src_temp, (pu1_pred_val), i4_sad[VERT_L_I4x4]);
            pu1_src_temp += src_strd;
            pu1_pred_val = u1_pred_vals_diag_121 + 6;
            USADA8(pu1_src_temp, (pu1_pred_val), i4_sad[VERT_L_I4x4]);

            i4_cost[VERT_L_I4x4] = i4_sad[VERT_L_I4x4] + ((u4_predictd_mode == VERT_L_I4x4) ?
                                            u4_lambda : 4 * u4_lambda);
        }

        if (u4_valid_intra_modes & 256)/* HORZ_U mode valid ????*/
        {
            i4_sad[HORZ_U_I4x4] = 0;
            pu1_src_temp = pu1_src;
            u1_pred_vals_horz_u[0] = u1_pred_vals_diag_11[2];
            u1_pred_vals_horz_u[1] = u1_pred_vals_diag_121[1];
            u1_pred_vals_horz_u[2] = u1_pred_vals_diag_11[1];
            u1_pred_vals_horz_u[3] = u1_pred_vals_diag_121[0];
            u1_pred_vals_horz_u[4] = u1_pred_vals_diag_11[0];
            u1_pred_vals_horz_u[5] = FILT121(pu1_ngbr_pels[0], pu1_ngbr_pels[0], pu1_ngbr_pels[1]);

            memset((u1_pred_vals_horz_u + 6), pu1_ngbr_pels[0], 4);

            pu1_pred_val = u1_pred_vals_horz_u;
            USADA8(pu1_src_temp, (pu1_pred_val), i4_sad[HORZ_U_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val + 2), i4_sad[HORZ_U_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val + 4), i4_sad[HORZ_U_I4x4]);
            pu1_src_temp += src_strd;
            USADA8(pu1_src_temp, (pu1_pred_val + 6), i4_sad[HORZ_U_I4x4]);

            i4_cost[HORZ_U_I4x4] = i4_sad[HORZ_U_I4x4] + ((u4_predictd_mode == HORZ_U_I4x4) ?
                                            u4_lambda : 4 * u4_lambda);
        }

        i4_min_cost = MIN3(MIN3(i4_cost[0], i4_cost[1], i4_cost[2]),
                        MIN3(i4_cost[3], i4_cost[4], i4_cost[5]),
                        MIN3(i4_cost[6], i4_cost[7], i4_cost[8]));

    }
    else
    {
        /* Only first three modes valid */
        i4_min_cost = MIN3(i4_cost[0], i4_cost[1], i4_cost[2]);
    }

    *pu4_sadmin = i4_min_cost;

    if (i4_min_cost == i4_cost[0])
    {
        *u4_intra_mode = VERT_I4x4;
        pu1_pred_val = pu1_ngbr_pels + 5;
        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val), 4);
    }
    else if (i4_min_cost == i4_cost[1])
    {
        *u4_intra_mode = HORZ_I4x4;
        memset(pu1_dst, pu1_ngbr_pels[3], 4);
        pu1_dst += dst_strd;
        memset(pu1_dst, pu1_ngbr_pels[2], 4);
        pu1_dst += dst_strd;
        memset(pu1_dst, pu1_ngbr_pels[1], 4);
        pu1_dst += dst_strd;
        memset(pu1_dst, pu1_ngbr_pels[0], 4);
    }
    else if (i4_min_cost == i4_cost[2])
    {
        *u4_intra_mode = DC_I4x4;
        memset(pu1_dst, u4_dcval, 4);
        pu1_dst += dst_strd;
        memset(pu1_dst, u4_dcval, 4);
        pu1_dst += dst_strd;
        memset(pu1_dst, u4_dcval, 4);
        pu1_dst += dst_strd;
        memset(pu1_dst, u4_dcval, 4);
    }

    else if (i4_min_cost == i4_cost[3])
    {
        *u4_intra_mode = DIAG_DL_I4x4;
        pu1_pred_val = u1_pred_vals_diag_121 + 5;
        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val + 1), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val + 2), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val + 3), 4);
    }
    else if (i4_min_cost == i4_cost[4])
    {
        *u4_intra_mode = DIAG_DR_I4x4;
        pu1_pred_val = u1_pred_vals_diag_121 + 3;

        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val - 1), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val - 2), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val - 3), 4);
    }

    else if (i4_min_cost == i4_cost[5])
    {
        *u4_intra_mode = VERT_R_I4x4;
        pu1_pred_val = u1_pred_vals_diag_11 + 4;
        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
        pu1_pred_val = u1_pred_vals_diag_121 + 3;
        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (u1_pred_vals_vert_r), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (u1_pred_vals_vert_r + 4), 4);
    }
    else if (i4_min_cost == i4_cost[6])
    {
        *u4_intra_mode = HORZ_D_I4x4;
        pu1_pred_val = u1_pred_vals_horz_d;
        memcpy(pu1_dst, (pu1_pred_val + 6), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val + 4), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val + 2), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
    }
    else if (i4_min_cost == i4_cost[7])
    {
        *u4_intra_mode = VERT_L_I4x4;
        pu1_pred_val = u1_pred_vals_diag_11 + 5;
        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
        pu1_pred_val = u1_pred_vals_diag_121 + 5;
        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
        pu1_pred_val = u1_pred_vals_diag_11 + 6;
        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
        pu1_pred_val = u1_pred_vals_diag_121 + 6;
        memcpy(pu1_dst, (pu1_pred_val), 4);
    }
    else if (i4_min_cost == i4_cost[8])
    {
        *u4_intra_mode = HORZ_U_I4x4;
        pu1_pred_val = u1_pred_vals_horz_u;
        memcpy(pu1_dst, (pu1_pred_val), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val + 2), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val + 4), 4);
        pu1_dst += dst_strd;
        memcpy(pu1_dst, (pu1_pred_val + 6), 4);
        pu1_dst += dst_strd;
    }

    return;
}

/**
******************************************************************************
*
* @brief:
*  Evaluate best intr chroma mode (among VERT, HORZ and DC ) and do the prediction.
*
* @par Description
*  This function evaluates  first three intra chroma modes and compute corresponding sad
*  and return the buffer predicted with best mode.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[in] pu1_ngbr_pels
*  UWORD8 pointer to neighbouring pels
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
* @param[in] u4_n_avblty
*  availability of neighbouring pixels
*
* @param[in] u4_intra_mode
*  Pointer to the variable in which best mode is returned
*
* @param[in] pu4_sadmin
*  Pointer to the variable in which minimum sad is returned
*
* @param[in] u4_valid_intra_modes
*  Says what all modes are valid
*
* @return      none
*
******************************************************************************
*/
void ih264e_evaluate_intra_chroma_modes(UWORD8 *pu1_src,
                                        UWORD8 *pu1_ngbr_pels,
                                        UWORD8 *pu1_dst,
                                        UWORD32 src_strd,
                                        UWORD32 dst_strd,
                                        WORD32 u4_n_avblty,
                                        UWORD32 *u4_intra_mode,
                                        WORD32 *pu4_sadmin,
                                        UWORD32 u4_valid_intra_modes)
{
    UWORD8 *pu1_neighbour;
    UWORD8 *pu1_src_temp = pu1_src;
    UWORD8 left = 0, top = 0;
    WORD32 u4_dcval_u_l[2] = { 0, 0 }, /*sum left neighbours for 'U' ,two separate sets - sum of first four from top,and sum of four values from bottom */
           u4_dcval_u_t[2] = { 0, 0 };  /*sum top neighbours for 'U'*/

    WORD32 u4_dcval_v_l[2] = { 0, 0 }, /*sum left neighbours for 'V'*/
           u4_dcval_v_t[2] = { 0, 0 }; /*sum top neighbours for 'V'*/

    WORD32 i, j, row, col, i4_sad_vert = INT_MAX, i4_sad_horz = INT_MAX,
                    i4_sad_dc = INT_MAX, i4_min_sad = INT_MAX;
    UWORD8 val_u, val_v;

    WORD32 u4_dc_val[2][2][2];/*  -----------
                                  |    |    |  Chroma can have four
                                  | 00 | 01 |  separate dc value...
                                  -----------  u4_dc_val corresponds to this dc values
                                  |    |    |  with u4_dc_val[2][2][U] and u4_dc_val[2][2][V]
                                  | 10 | 11 |
                                  -----------                */
    left = (u4_n_avblty & LEFT_MB_AVAILABLE_MASK);
    top = (u4_n_avblty & TOP_MB_AVAILABLE_MASK) >> 2;

    /*Evaluating HORZ*/
    if (left)/* Ifleft available*/
    {
        i4_sad_horz = 0;

        for (i = 0; i < 8; i++)
        {
            val_v = pu1_ngbr_pels[15 - 2 * i];
            val_u = pu1_ngbr_pels[15 - 2 * i - 1];
            row = i / 4;
            u4_dcval_u_l[row] += val_u;
            u4_dcval_v_l[row] += val_v;
            for (j = 0; j < 8; j++)
            {
                i4_sad_horz += ABS(val_u - pu1_src_temp[2 * j]);/* Finding SAD for HORZ mode*/
                i4_sad_horz += ABS(val_v - pu1_src_temp[2 * j + 1]);
            }

            pu1_src_temp += src_strd;
        }
        u4_dcval_u_l[0] += 2;
        u4_dcval_u_l[1] += 2;
        u4_dcval_v_l[0] += 2;
        u4_dcval_v_l[1] += 2;
    }

    /*Evaluating VERT**/
    pu1_src_temp = pu1_src;
    if (top) /* top available*/
    {
        i4_sad_vert = 0;

        for (i = 0; i < 8; i++)
        {
            col = i / 4;

            val_u = pu1_ngbr_pels[18 + i * 2];
            val_v = pu1_ngbr_pels[18 + i * 2 + 1];
            u4_dcval_u_t[col] += val_u;
            u4_dcval_v_t[col] += val_v;

            for (j = 0; j < 16; j++)
            {
                i4_sad_vert += ABS(pu1_ngbr_pels[18 + j] - pu1_src_temp[j]);/* Finding SAD for VERT mode*/
            }
            pu1_src_temp += src_strd;

        }
        u4_dcval_u_t[0] += 2;
        u4_dcval_u_t[1] += 2;
        u4_dcval_v_t[0] += 2;
        u4_dcval_v_t[1] += 2;
    }

    /* computing DC value*/
    /* Equation  8-128 in spec*/
    u4_dc_val[0][0][0] = (u4_dcval_u_l[0] + u4_dcval_u_t[0]) >> (1 + left + top);
    u4_dc_val[0][0][1] = (u4_dcval_v_l[0] + u4_dcval_v_t[0]) >> (1 + left + top);
    u4_dc_val[1][1][0] = (u4_dcval_u_l[1] + u4_dcval_u_t[1]) >> (1 + left + top);
    u4_dc_val[1][1][1] = (u4_dcval_v_l[1] + u4_dcval_v_t[1]) >> (1 + left + top);

    if (top)
    {
        /* Equation  8-132 in spec*/
        u4_dc_val[0][1][0] = (u4_dcval_u_t[1]) >> (1 + top);
        u4_dc_val[0][1][1] = (u4_dcval_v_t[1]) >> (1 + top);
    }
    else
    {
        u4_dc_val[0][1][0] = (u4_dcval_u_l[0]) >> (1 + left);
        u4_dc_val[0][1][1] = (u4_dcval_v_l[0]) >> (1 + left);
    }

    if (left)
    {
        u4_dc_val[1][0][0] = (u4_dcval_u_l[1]) >> (1 + left);
        u4_dc_val[1][0][1] = (u4_dcval_v_l[1]) >> (1 + left);
    }
    else
    {
        u4_dc_val[1][0][0] = (u4_dcval_u_t[0]) >> (1 + top);
        u4_dc_val[1][0][1] = (u4_dcval_v_t[0]) >> (1 + top);
    }

    if (!(left || top))
    {
        /*none available*/
        u4_dc_val[0][0][0] = u4_dc_val[0][0][1] =
        u4_dc_val[0][1][0] = u4_dc_val[0][1][1] =
        u4_dc_val[1][0][0] = u4_dc_val[1][0][1] =
        u4_dc_val[1][1][0] = u4_dc_val[1][1][1] = 128;
    }

    /* Evaluating DC */
    pu1_src_temp = pu1_src;
    i4_sad_dc = 0;
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            col = j / 4;
            row = i / 4;
            val_u = u4_dc_val[row][col][0];
            val_v = u4_dc_val[row][col][1];

            i4_sad_dc += ABS(val_u - pu1_src_temp[2 * j]);/* Finding SAD for DC mode*/
            i4_sad_dc += ABS(val_v - pu1_src_temp[2 * j + 1]);
        }
        pu1_src_temp += src_strd;
    }

    if ((u4_valid_intra_modes & 01) == 0)/* If DC is disabled*/
        i4_sad_dc = INT_MAX;
    if ((u4_valid_intra_modes & 02) == 0)/* If HORZ is disabled*/
        i4_sad_horz = INT_MAX;
    if ((u4_valid_intra_modes & 04) == 0)/* If VERT is disabled*/
        i4_sad_vert = INT_MAX;

    i4_min_sad = MIN3(i4_sad_horz, i4_sad_dc, i4_sad_vert);

    /* Finding Minimum sad and doing corresponding prediction*/
    if (i4_min_sad < *pu4_sadmin)
    {
        *pu4_sadmin = i4_min_sad;

        if (i4_min_sad == i4_sad_dc)
        {
            *u4_intra_mode = DC_CH_I8x8;
            for (i = 0; i < 8; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    col = j / 4;
                    row = i / 4;

                    pu1_dst[2 * j] = u4_dc_val[row][col][0];
                    pu1_dst[2 * j + 1] = u4_dc_val[row][col][1];
                }
                pu1_dst += dst_strd;
            }
        }
        else if (i4_min_sad == i4_sad_horz)
        {
            *u4_intra_mode = HORZ_CH_I8x8;
            for (j = 0; j < 8; j++)
            {
                val_v = pu1_ngbr_pels[15 - 2 * j];
                val_u = pu1_ngbr_pels[15 - 2 * j - 1];

                for (i = 0; i < 8; i++)
                {
                    pu1_dst[2 * i] = val_u;
                    pu1_dst[2 * i + 1] = val_v;

                }
                pu1_dst += dst_strd;
            }
        }
        else
        {
            *u4_intra_mode = VERT_CH_I8x8;
            pu1_neighbour = pu1_ngbr_pels + 18;
            for (j = 0; j < 8; j++)
            {
                memcpy(pu1_dst, pu1_neighbour, MB_SIZE);
                pu1_dst += dst_strd;
            }
        }
    }

    return;
}
