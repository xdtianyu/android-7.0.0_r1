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
 *  ih264e_me.c
 *
 * @brief
 *  Contains definition of functions for motion estimation
 *
 * @author
 *  ittiam
 *
 * @par List of Functions:
 *  - ih264e_init_mv_bits()
 *  - ih264e_skip_analysis_chroma()
 *  - ih264e_skip_analysis_luma()
 *  - ih264e_analyse_skip()
 *  - ih264e_get_search_candidates()
 *  - ih264e_find_skip_motion_vector()
 *  - ih264e_get_mv_predictor()
 *  - ih264e_mv_pred()
 *  - ih264e_mv_pred_me()
 *  - ih264e_init_me()
 *  - ih264e_compute_me()
 *  - ih264e_compute_me_nmb()
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
#include <assert.h>
#include <limits.h>

/* User include files */
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ithread.h"
#include "ih264_platform_macros.h"
#include "ih264_defs.h"
#include "ime_defs.h"
#include "ime_distortion_metrics.h"
#include "ime_structs.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "ih264e_defs.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_globals.h"
#include "ih264_macros.h"
#include "ih264e_me.h"
#include "ime.h"
#include "ih264_debug.h"
#include "ih264e_intra_modes_eval.h"
#include "ih264e_core_coding.h"
#include "ih264e_mc.h"
#include "ih264e_debug.h"
#include "ih264e_half_pel.h"
#include "ime_statistics.h"
#include "ih264e_platform_macros.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief
*  This function populates the length of the codewords for motion vectors in the
*  range (-search range, search range) in pixels
*
* @param[in] ps_me
*  Pointer to me ctxt
*
* @param[out] pu1_mv_bits
*  length of the codeword for all mv's
*
* @remarks The length of the code words are derived from signed exponential
* goloumb codes.
*
*******************************************************************************
*/
void ih264e_init_mv_bits(me_ctxt_t *ps_me_ctxt)
{
    /* temp var */
    WORD32 i, codesize = 3, diff, limit;
    UWORD32 u4_code_num, u4_range;
    UWORD32 u4_uev_min, u4_uev_max, u4_sev_min, u4_sev_max;

    /* max srch range */
    diff = MAX(DEFAULT_MAX_SRCH_RANGE_X, DEFAULT_MAX_SRCH_RANGE_Y);
    /* sub pel */
    diff <<= 2;
    /* delta mv */
    diff <<= 1;

    /* codeNum for positive integer     =  2x-1     : Table9-3  */
    u4_code_num = (diff << 1);

    /* get range of the bit string and put using put_bits()                 */
    GETRANGE(u4_range, u4_code_num);

    limit = 2*u4_range - 1;

    /* init mv bits */
    ps_me_ctxt->pu1_mv_bits[0] = 1;

    while (codesize < limit)
    {
        u4_uev_min = (1 << (codesize >> 1));
        u4_uev_max = 2*u4_uev_min - 1;

        u4_sev_min = u4_uev_min >> 1;
        u4_sev_max = u4_uev_max >> 1;

        DEBUG("\n%d min, %d max %d codesize", u4_sev_min, u4_sev_max, codesize);

        for (i = u4_sev_min; i <= (WORD32)u4_sev_max; i++)
        {
            ps_me_ctxt->pu1_mv_bits[-i] = ps_me_ctxt->pu1_mv_bits[i] = codesize;
        }

        codesize += 2;
    }
}



/**
*******************************************************************************
*
* @brief Determines the valid candidates for which the initial search shall happen.
* The best of these candidates is used to center the diamond pixel search.
*
* @par Description: The function sends the skip, (0,0), left, top and top-right
* neighbouring MBs MVs. The left, top and top-right MBs MVs are used because
* these are the same MVs that are used to form the MV predictor. This initial MV
* search candidates need not take care of slice boundaries and hence neighbor
* availability checks are not made here.
*
* @param[in] ps_left_mb_pu
*  pointer to left mb motion vector info
*
* @param[in] ps_top_mb_pu
*  pointer to top & top right mb motion vector info
*
* @param[in] ps_top_left_mb_pu
*  pointer to top left mb motion vector info
*
* @param[out] ps_skip_mv
*  pointer to skip motion vectors for the curr mb
*
* @param[in] i4_mb_x
*  mb index x
*
* @param[in] i4_mb_y
*  mb index y
*
* @param[in] i4_wd_mbs
*  pic width in mbs
*
* @param[in] ps_motionEst
*  pointer to me context
*
* @returns  The list of MVs to be used of priming the full pel search and the
* number of such MVs
*
* @remarks
*   Assumptions : 1. Assumes Only partition of size 16x16
*
*******************************************************************************
*/
static void ih264e_get_search_candidates(process_ctxt_t *ps_proc,
                                         me_ctxt_t *ps_me_ctxt,
                                         WORD32 i4_reflist)
{
    /* curr mb indices */
    WORD32 i4_mb_x = ps_proc->i4_mb_x;

    /* Motion vector */
    mv_t *ps_left_mv, *ps_top_mv, *ps_top_left_mv, *ps_top_right_mv;

    /* Pred modes */
    WORD32 i4_left_mode, i4_top_mode, i4_top_left_mode, i4_top_right_mode;

    /* mb part info */
    mb_part_ctxt *ps_mb_part = &ps_me_ctxt->as_mb_part[i4_reflist];

    /* mvs */
    WORD32 mvx, mvy;

    /* ngbr availability */
    block_neighbors_t *ps_ngbr_avbl = ps_proc->ps_ngbr_avbl;

    /* Current mode */
    WORD32 i4_cmpl_predmode = (i4_reflist == 0) ? PRED_L1 : PRED_L0;

    /* srch range*/
    WORD32 i4_srch_range_n = ps_me_ctxt->i4_srch_range_n;
    WORD32 i4_srch_range_s = ps_me_ctxt->i4_srch_range_s;
    WORD32 i4_srch_range_e = ps_me_ctxt->i4_srch_range_e;
    WORD32 i4_srch_range_w = ps_me_ctxt->i4_srch_range_w;

    /* num of candidate search candidates */
    UWORD32 u4_num_candidates = 0;

    ps_left_mv = &ps_proc->s_left_mb_pu_ME.s_me_info[i4_reflist].s_mv;
    ps_top_mv = &(ps_proc->ps_top_row_pu_ME + i4_mb_x)->s_me_info[i4_reflist].s_mv;
    ps_top_left_mv = &ps_proc->s_top_left_mb_pu_ME.s_me_info[i4_reflist].s_mv;
    ps_top_right_mv = &(ps_proc->ps_top_row_pu_ME + i4_mb_x + 1)->s_me_info[i4_reflist].s_mv;

    i4_left_mode = ps_proc->s_left_mb_pu_ME.b2_pred_mode != i4_cmpl_predmode;
    i4_top_mode = (ps_proc->ps_top_row_pu_ME + i4_mb_x)->b2_pred_mode != i4_cmpl_predmode;
    i4_top_left_mode = ps_proc->s_top_left_mb_pu_ME.b2_pred_mode != i4_cmpl_predmode;
    i4_top_right_mode = (ps_proc->ps_top_row_pu_ME + i4_mb_x + 1)->b2_pred_mode != i4_cmpl_predmode;

    /* Taking the Zero motion vector as one of the candidates   */
    ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvx = 0;
    ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvy = 0;

    u4_num_candidates++;

    /* Taking the Left MV Predictor as one of the candidates    */
    if (ps_ngbr_avbl->u1_mb_a && i4_left_mode)
    {
        mvx      = (ps_left_mv->i2_mvx + 2) >> 2;
        mvy      = (ps_left_mv->i2_mvy + 2) >> 2;

        mvx = CLIP3(i4_srch_range_w, i4_srch_range_e, mvx);
        mvy = CLIP3(i4_srch_range_n, i4_srch_range_s, mvy);

        ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvx = mvx;
        ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvy = mvy;

        u4_num_candidates ++;
    }

    /* Taking the Top MV Predictor as one of the candidates     */
    if (ps_ngbr_avbl->u1_mb_b && i4_top_mode)
    {
        mvx      = (ps_top_mv->i2_mvx + 2) >> 2;
        mvy      = (ps_top_mv->i2_mvy + 2) >> 2;

        mvx = CLIP3(i4_srch_range_w, i4_srch_range_e, mvx);
        mvy = CLIP3(i4_srch_range_n, i4_srch_range_s, mvy);

        ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvx = mvx;
        ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvy = mvy;

        u4_num_candidates ++;

        /* Taking the TopRt MV Predictor as one of the candidates   */
        if (ps_ngbr_avbl->u1_mb_c && i4_top_right_mode)
        {
            mvx      = (ps_top_right_mv->i2_mvx + 2) >> 2;
            mvy      = (ps_top_right_mv->i2_mvy + 2)>> 2;

            mvx = CLIP3(i4_srch_range_w, i4_srch_range_e, mvx);
            mvy = CLIP3(i4_srch_range_n, i4_srch_range_s, mvy);

            ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvx = mvx;
            ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvy = mvy;

            u4_num_candidates ++;
        }
        /* Taking the TopLt MV Predictor as one of the candidates   */
        else if(ps_ngbr_avbl->u1_mb_d && i4_top_left_mode)
        {
            mvx      = (ps_top_left_mv->i2_mvx + 2) >> 2;
            mvy      = (ps_top_left_mv->i2_mvy + 2) >> 2;

            mvx = CLIP3(i4_srch_range_w, i4_srch_range_e, mvx);
            mvy = CLIP3(i4_srch_range_n, i4_srch_range_s, mvy);

            ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvx = mvx;
            ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvy = mvy;

            u4_num_candidates ++;
        }
    }


    /********************************************************************/
    /*                            MV Prediction                         */
    /********************************************************************/
    ih264e_mv_pred_me(ps_proc, i4_reflist);

    ps_mb_part->s_mv_pred.i2_mvx = ps_proc->ps_pred_mv[i4_reflist].s_mv.i2_mvx;
    ps_mb_part->s_mv_pred.i2_mvy = ps_proc->ps_pred_mv[i4_reflist].s_mv.i2_mvy;

    /* Get the skip motion vector                               */
    {
        ps_me_ctxt->i4_skip_type = ps_proc->ps_codec->apf_find_skip_params_me
                                    [ps_proc->i4_slice_type](ps_proc, i4_reflist);

        /* Taking the Skip motion vector as one of the candidates   */
        mvx = (ps_proc->ps_skip_mv[i4_reflist].s_mv.i2_mvx + 2) >> 2;
        mvy = (ps_proc->ps_skip_mv[i4_reflist].s_mv.i2_mvy + 2) >> 2;

        mvx = CLIP3(i4_srch_range_w, i4_srch_range_e, mvx);
        mvy = CLIP3(i4_srch_range_n, i4_srch_range_s, mvy);

        ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvx = mvx;
        ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvy = mvy;
        u4_num_candidates++;

        if (ps_proc->i4_slice_type == BSLICE)
        {
            /* Taking the temporal Skip motion vector as one of the candidates   */
            mvx = (ps_proc->ps_skip_mv[i4_reflist + 2].s_mv.i2_mvx + 2) >> 2;
            mvy = (ps_proc->ps_skip_mv[i4_reflist + 2].s_mv.i2_mvy + 2) >> 2;

            mvx = CLIP3(i4_srch_range_w, i4_srch_range_e, mvx);
            mvy = CLIP3(i4_srch_range_n, i4_srch_range_s, mvy);

            ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvx = mvx;
            ps_me_ctxt->as_mv_init_search[i4_reflist][u4_num_candidates].i2_mvy = mvy;
            u4_num_candidates++;
        }
    }

    ASSERT(u4_num_candidates <= 6);

    ps_me_ctxt->u4_num_candidates[i4_reflist] = u4_num_candidates;
}

/**
*******************************************************************************
*
* @brief The function computes parameters for a PSKIP MB
*
* @par Description:
*  The function updates the skip motion vector and checks if the current
*  MB can be a skip PSKIP mB or not
*
* @param[in] ps_proc
*  Pointer to process context
*
* @param[in] u4_for_me
*  Flag to indicate function is called for ME or not
*
* @param[out] i4_ref_list
*  Current active refernce list
*
* @returns Flag indicating if the current MB can be marked as skip
*
* @remarks The code implements the logic as described in sec 8.4.1.2.2 in H264
*   specification.
*
*******************************************************************************
*/
WORD32 ih264e_find_pskip_params(process_ctxt_t *ps_proc, WORD32 i4_reflist)
{
    /* left mb motion vector */
    enc_pu_t *ps_left_mb_pu ;

    /* top mb motion vector */
    enc_pu_t *ps_top_mb_pu ;

    /* Skip mv */
    mv_t *ps_skip_mv = &ps_proc->ps_skip_mv[PRED_L0].s_mv;

    UNUSED(i4_reflist);

    ps_left_mb_pu = &ps_proc->s_left_mb_pu ;
    ps_top_mb_pu = ps_proc->ps_top_row_pu + ps_proc->i4_mb_x;

    if ((!ps_proc->ps_ngbr_avbl->u1_mb_a) ||
        (!ps_proc->ps_ngbr_avbl->u1_mb_b) ||
        (
          (ps_left_mb_pu->s_me_info[PRED_L0].i1_ref_idx == -1) &&
          (ps_left_mb_pu->s_me_info[PRED_L0].s_mv.i2_mvx == 0) &&
          (ps_left_mb_pu->s_me_info[PRED_L0].s_mv.i2_mvy == 0)
       ) ||
       (
          (ps_top_mb_pu->s_me_info[PRED_L0].i1_ref_idx == -1) &&
          (ps_top_mb_pu->s_me_info[PRED_L0].s_mv.i2_mvx == 0) &&
          (ps_top_mb_pu->s_me_info[PRED_L0].s_mv.i2_mvy == 0)
       )
     )

    {
        ps_skip_mv->i2_mvx = 0;
        ps_skip_mv->i2_mvy = 0;
    }
    else
    {
        ps_skip_mv->i2_mvx = ps_proc->ps_pred_mv[PRED_L0].s_mv.i2_mvx;
        ps_skip_mv->i2_mvy = ps_proc->ps_pred_mv[PRED_L0].s_mv.i2_mvy;
    }

    if ( (ps_proc->ps_pu->s_me_info[PRED_L0].s_mv.i2_mvx == ps_skip_mv->i2_mvx)
     && (ps_proc->ps_pu->s_me_info[PRED_L0].s_mv.i2_mvy == ps_skip_mv->i2_mvy))
    {
        return 1;
    }

    return 0;
}

/**
*******************************************************************************
*
* @brief The function computes parameters for a PSKIP MB
*
* @par Description:
*  The function updates the skip motion vector and checks if the current
*  MB can be a skip PSKIP mB or not
*
* @param[in] ps_proc
*  Pointer to process context
*
* @param[in] u4_for_me
*  Flag to dincate fucntion is called for ME or not
*
* @param[out] i4_ref_list
*  Current active refernce list
*
* @returns Flag indicating if the current MB can be marked as skip
*
* @remarks The code implements the logic as described in sec 8.4.1.2.2 in H264
*   specification.
*
*******************************************************************************
*/
WORD32 ih264e_find_pskip_params_me(process_ctxt_t *ps_proc, WORD32 i4_reflist)
{
    /* left mb motion vector */
    enc_pu_t *ps_left_mb_pu ;

    /* top mb motion vector */
    enc_pu_t *ps_top_mb_pu ;

    /* Skip mv */
    mv_t *ps_skip_mv = &ps_proc->ps_skip_mv[PRED_L0].s_mv;

    UNUSED(i4_reflist);

    ps_left_mb_pu = &ps_proc->s_left_mb_pu_ME;
    ps_top_mb_pu = ps_proc->ps_top_row_pu_ME + ps_proc->i4_mb_x;

    if ((!ps_proc->ps_ngbr_avbl->u1_mb_a) ||
        (!ps_proc->ps_ngbr_avbl->u1_mb_b) ||
        (
          (ps_left_mb_pu->s_me_info[PRED_L0].i1_ref_idx == -1) &&
          (ps_left_mb_pu->s_me_info[PRED_L0].s_mv.i2_mvx == 0) &&
          (ps_left_mb_pu->s_me_info[PRED_L0].s_mv.i2_mvy == 0)
        ) ||
        (
          (ps_top_mb_pu->s_me_info[PRED_L0].i1_ref_idx == -1) &&
          (ps_top_mb_pu->s_me_info[PRED_L0].s_mv.i2_mvx == 0) &&
          (ps_top_mb_pu->s_me_info[PRED_L0].s_mv.i2_mvy == 0)
        )
     )

    {
        ps_skip_mv->i2_mvx = 0;
        ps_skip_mv->i2_mvy = 0;
    }
    else
    {
        ps_skip_mv->i2_mvx = ps_proc->ps_pred_mv[PRED_L0].s_mv.i2_mvx;
        ps_skip_mv->i2_mvy = ps_proc->ps_pred_mv[PRED_L0].s_mv.i2_mvy;
    }

    return PRED_L0;
}


/**
*******************************************************************************
*
* @brief motion vector predictor
*
* @par Description:
*  The routine calculates the motion vector predictor for a given block,
*  given the candidate MV predictors.
*
* @param[in] ps_left_mb_pu
*  pointer to left mb motion vector info
*
* @param[in] ps_top_row_pu
*  pointer to top & top right mb motion vector info
*
* @param[out] ps_pred_mv
*  pointer to candidate predictors for the current block
*
* @returns  The x & y components of the MV predictor.
*
* @remarks The code implements the logic as described in sec 8.4.1.3 in H264
*   specification.
*   Assumptions : 1. Assumes Single reference frame
*                 2. Assumes Only partition of size 16x16
*
*******************************************************************************
*/
void ih264e_get_mv_predictor(enc_pu_t *ps_left_mb_pu,
                             enc_pu_t *ps_top_row_pu,
                             enc_pu_mv_t *ps_pred_mv,
                             WORD32 i4_ref_list)
{

    /* Indicated the current ref */
    WORD8 i1_ref_idx;

    /* For pred L0 */
    i1_ref_idx = -1;
    {
        /* temp var */
        WORD32 pred_algo = 3, a, b, c;

        /* If only one of the candidate blocks has a reference frame equal to
         * the current block then use the same block as the final predictor */
        a = (ps_left_mb_pu->s_me_info[i4_ref_list].i1_ref_idx == i1_ref_idx) ? 0 : -1;
        b = (ps_top_row_pu[0].s_me_info[i4_ref_list].i1_ref_idx == i1_ref_idx) ? 0 : -1;
        c = (ps_top_row_pu[1].s_me_info[i4_ref_list].i1_ref_idx == i1_ref_idx) ? 0 : -1;

        if (a == 0 && b == -1 && c == -1)
            pred_algo = 0; /* LEFT */
        else if(a == -1 && b == 0 && c == -1)
            pred_algo = 1; /* TOP */
        else if(a == -1 && b == -1 && c == 0)
            pred_algo = 2; /* TOP RIGHT */

        switch (pred_algo)
        {
            case 0:
                /* left */
                ps_pred_mv->s_mv.i2_mvx = ps_left_mb_pu->s_me_info[i4_ref_list].s_mv.i2_mvx;
                ps_pred_mv->s_mv.i2_mvy = ps_left_mb_pu->s_me_info[i4_ref_list].s_mv.i2_mvy;
                break;
            case 1:
                /* top */
                ps_pred_mv->s_mv.i2_mvx = ps_top_row_pu[0].s_me_info[i4_ref_list].s_mv.i2_mvx;
                ps_pred_mv->s_mv.i2_mvy = ps_top_row_pu[0].s_me_info[i4_ref_list].s_mv.i2_mvy;
                break;
            case 2:
                /* top right */
                ps_pred_mv->s_mv.i2_mvx = ps_top_row_pu[1].s_me_info[i4_ref_list].s_mv.i2_mvx;
                ps_pred_mv->s_mv.i2_mvy = ps_top_row_pu[1].s_me_info[i4_ref_list].s_mv.i2_mvy;
                break;
            case 3:
                /* median */
                MEDIAN(ps_left_mb_pu->s_me_info[i4_ref_list].s_mv.i2_mvx,
                       ps_top_row_pu[0].s_me_info[i4_ref_list].s_mv.i2_mvx,
                       ps_top_row_pu[1].s_me_info[i4_ref_list].s_mv.i2_mvx,
                       ps_pred_mv->s_mv.i2_mvx);
                MEDIAN(ps_left_mb_pu->s_me_info[i4_ref_list].s_mv.i2_mvy,
                       ps_top_row_pu[0].s_me_info[i4_ref_list].s_mv.i2_mvy,
                       ps_top_row_pu[1].s_me_info[i4_ref_list].s_mv.i2_mvy,
                       ps_pred_mv->s_mv.i2_mvy);

                break;
            default:
                break;
        }
    }
}

/**
*******************************************************************************
*
* @brief This function performs MV prediction
*
* @par Description:
*
* @param[in] ps_proc
*  Process context corresponding to the job
*
* @returns  none
*
* @remarks none
*  This function will update the MB availability since intra inter decision
*  should be done before the call
*
*******************************************************************************
*/
void ih264e_mv_pred(process_ctxt_t *ps_proc, WORD32 i4_slice_type)
{

    /* left mb motion vector */
    enc_pu_t *ps_left_mb_pu;

    /* top left mb motion vector */
    enc_pu_t *ps_top_left_mb_pu;

    /* top row motion vector info */
    enc_pu_t *ps_top_row_pu;

    /* predicted motion vector */
    enc_pu_mv_t *ps_pred_mv = ps_proc->ps_pred_mv;

    /* zero mv */
    mv_t zero_mv = { 0, 0 };

    /*  mb neighbor availability */
    block_neighbors_t *ps_ngbr_avbl = ps_proc->ps_ngbr_avbl;

    /* mb syntax elements of neighbors */
    mb_info_t *ps_top_syn = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x;
    mb_info_t *ps_top_left_syn;
    UWORD32 u4_left_is_intra;

    /* Temp var */
    WORD32 i4_reflist, max_reflist, i4_cmpl_predmode;

    ps_top_left_syn = &(ps_proc->s_top_left_mb_syntax_ele);
    u4_left_is_intra = ps_proc->s_left_mb_syntax_ele.u2_is_intra;
    ps_left_mb_pu = &ps_proc->s_left_mb_pu;
    ps_top_left_mb_pu = &ps_proc->s_top_left_mb_pu;
    ps_top_row_pu = (ps_proc->ps_top_row_pu + ps_proc->i4_mb_x);

    /* Number of ref lists to process */
    max_reflist = (i4_slice_type == PSLICE) ? 1 : 2;

    for (i4_reflist = 0; i4_reflist < max_reflist; i4_reflist++)
    {
        i4_cmpl_predmode = (i4_reflist == 0) ? PRED_L1 : PRED_L0;

        /* Before performing mv prediction prepare the ngbr information and
         * reset motion vectors basing on their availability */
        if (!ps_ngbr_avbl->u1_mb_a || (u4_left_is_intra == 1)
                        || (ps_left_mb_pu->b2_pred_mode == i4_cmpl_predmode))
        {
            /* left mv */
            ps_left_mb_pu->s_me_info[i4_reflist].i1_ref_idx = 0;
            ps_left_mb_pu->s_me_info[i4_reflist].s_mv = zero_mv;
        }
        if (!ps_ngbr_avbl->u1_mb_b || ps_top_syn->u2_is_intra
                        || (ps_top_row_pu[0].b2_pred_mode == i4_cmpl_predmode))
        {
            /* top mv */
            ps_top_row_pu[0].s_me_info[i4_reflist].i1_ref_idx = 0;
            ps_top_row_pu[0].s_me_info[i4_reflist].s_mv = zero_mv;
        }

        if (!ps_ngbr_avbl->u1_mb_c)
        {
            /* top right mv - When top right partition is not available for
             * prediction if top left is available use it for prediction else
             * set the mv information to -1 and (0, 0)
             * */
            if (!ps_ngbr_avbl->u1_mb_d || ps_top_left_syn->u2_is_intra
                            || (ps_top_left_mb_pu->b2_pred_mode == i4_cmpl_predmode))
            {
                ps_top_row_pu[1].s_me_info[i4_reflist].i1_ref_idx = 0;
                ps_top_row_pu[1].s_me_info[i4_reflist].s_mv = zero_mv;
            }
            else
            {
                ps_top_row_pu[1].s_me_info[i4_reflist].i1_ref_idx = ps_top_left_mb_pu->s_me_info[i4_reflist].i1_ref_idx;
                ps_top_row_pu[1].s_me_info[i4_reflist].s_mv = ps_top_left_mb_pu->s_me_info[i4_reflist].s_mv;
            }
        }
        else if(ps_top_syn[1].u2_is_intra
                        || (ps_top_row_pu[1].b2_pred_mode == i4_cmpl_predmode))
        {
            ps_top_row_pu[1].s_me_info[i4_reflist].i1_ref_idx = 0;
            ps_top_row_pu[1].s_me_info[i4_reflist].s_mv = zero_mv;
        }

        ih264e_get_mv_predictor(ps_left_mb_pu, ps_top_row_pu, &ps_pred_mv[i4_reflist], i4_reflist);
    }

}

/**
*******************************************************************************
*
* @brief This function approximates Pred. MV
*
* @par Description:
*
* @param[in] ps_proc
*  Process context corresponding to the job
*
* @returns  none
*
* @remarks none
*  Motion estimation happens at nmb level. For cost calculations, mv is appro
*  ximated using this function
*
*******************************************************************************
*/
void ih264e_mv_pred_me(process_ctxt_t *ps_proc, WORD32 i4_ref_list)
{
    /* left mb motion vector */
    enc_pu_t *ps_left_mb_pu ;

    /* top left mb motion vector */
    enc_pu_t *ps_top_left_mb_pu ;

    /* top row motion vector info */
    enc_pu_t *ps_top_row_pu;

    enc_pu_t s_top_row_pu[2];

    /* predicted motion vector */
    enc_pu_mv_t *ps_pred_mv = ps_proc->ps_pred_mv;

    /* zero mv */
    mv_t zero_mv = {0, 0};

    /* Complementary pred mode */
    WORD32 i4_cmpl_predmode = (i4_ref_list == 0) ? PRED_L1 : PRED_L0;

    /*  mb neighbor availability */
    block_neighbors_t *ps_ngbr_avbl = ps_proc->ps_ngbr_avbl;

    ps_left_mb_pu = &ps_proc->s_left_mb_pu_ME;
    ps_top_left_mb_pu = &ps_proc->s_top_left_mb_pu_ME;
    ps_top_row_pu = (ps_proc->ps_top_row_pu_ME + ps_proc->i4_mb_x);

    s_top_row_pu[0] = ps_top_row_pu[0];
    s_top_row_pu[1] = ps_top_row_pu[1];

    /*
     * Before performing mv prediction prepare the ngbr information and
     * reset motion vectors basing on their availability
     */

    if (!ps_ngbr_avbl->u1_mb_a || (ps_left_mb_pu->b2_pred_mode == i4_cmpl_predmode))
    {
        /* left mv */
        ps_left_mb_pu->s_me_info[i4_ref_list].i1_ref_idx = 0;
        ps_left_mb_pu->s_me_info[i4_ref_list].s_mv = zero_mv;
    }
    if (!ps_ngbr_avbl->u1_mb_b || (s_top_row_pu[0].b2_pred_mode == i4_cmpl_predmode))
    {
        /* top mv */
        s_top_row_pu[0].s_me_info[i4_ref_list].i1_ref_idx = 0;
        s_top_row_pu[0].s_me_info[i4_ref_list].s_mv = zero_mv;

    }
    if (!ps_ngbr_avbl->u1_mb_c)
    {
        /* top right mv - When top right partition is not available for
         * prediction if top left is available use it for prediction else
         * set the mv information to -1 and (0, 0)
         * */
        if (!ps_ngbr_avbl->u1_mb_d || (ps_top_left_mb_pu->b2_pred_mode == i4_cmpl_predmode))
        {
            s_top_row_pu[1].s_me_info[i4_ref_list].i1_ref_idx = 0;
            s_top_row_pu[1].s_me_info[i4_ref_list].s_mv = zero_mv;

            s_top_row_pu[1].s_me_info[i4_ref_list].i1_ref_idx = 0;
            s_top_row_pu[1].s_me_info[i4_ref_list].s_mv = zero_mv;
        }
        else
        {
            s_top_row_pu[1].s_me_info[i4_ref_list].i1_ref_idx = ps_top_left_mb_pu->s_me_info[0].i1_ref_idx;
            s_top_row_pu[1].s_me_info[i4_ref_list].s_mv = ps_top_left_mb_pu->s_me_info[0].s_mv;
        }
    }
    else if (ps_top_row_pu[1].b2_pred_mode == i4_cmpl_predmode)
    {
        ps_top_row_pu[1].s_me_info[i4_ref_list].i1_ref_idx = 0;
        ps_top_row_pu[1].s_me_info[i4_ref_list].s_mv = zero_mv;
    }

    ih264e_get_mv_predictor(ps_left_mb_pu, &(s_top_row_pu[0]),
                            &ps_pred_mv[i4_ref_list], i4_ref_list);
}

/**
*******************************************************************************
*
* @brief This function initializes me ctxt
*
* @par Description:
*  Before dispatching the current job to me thread, the me context associated
*  with the job is initialized.
*
* @param[in] ps_proc
*  Process context corresponding to the job
*
* @returns  none
*
* @remarks none
*
*******************************************************************************
*/
void ih264e_init_me(process_ctxt_t *ps_proc)
{
    /* me ctxt */
    me_ctxt_t *ps_me_ctxt = &ps_proc->s_me_ctxt;

    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    ps_me_ctxt->i4_skip_bias[BSLICE] = SKIP_BIAS_B;

    if (ps_codec->s_cfg.u4_num_bframes == 0)
    {
       ps_me_ctxt->i4_skip_bias[PSLICE] = 4 * SKIP_BIAS_P;
    }
    else
    {
       ps_me_ctxt->i4_skip_bias[PSLICE] =  SKIP_BIAS_P;
    }

    /* src ptr */
    ps_me_ctxt->pu1_src_buf_luma = ps_proc->pu1_src_buf_luma;
    /* src stride */
    ps_me_ctxt->i4_src_strd = ps_proc->i4_src_strd;

    /* ref ptrs and corresponding lagrange params */
    ps_me_ctxt->apu1_ref_buf_luma[0] = ps_proc->apu1_ref_buf_luma[0];
    ps_me_ctxt->apu1_ref_buf_luma[1] = ps_proc->apu1_ref_buf_luma[1];

    ps_me_ctxt->u4_lambda_motion = gu1_qp0[ps_me_ctxt->u1_mb_qp];


}


/**
*******************************************************************************
*
* @brief This function performs motion estimation for the current mb using
*   single reference list
*
* @par Description:
*  The current mb is compared with a list of mb's in the reference frame for
*  least cost. The mb that offers least cost is chosen as predicted mb and the
*  displacement of the predicted mb from index location of the current mb is
*  signaled as mv. The list of the mb's that are chosen in the reference frame
*  are dependent on the speed of the ME configured.
*
* @param[in] ps_proc
*  Process context corresponding to the job
*
* @returns  motion vector of the pred mb, sad, cost.
*
* @remarks none
*
*******************************************************************************
*/
void ih264e_compute_me_single_reflist(process_ctxt_t *ps_proc)
{
    /* me ctxt */
    me_ctxt_t *ps_me_ctxt = &ps_proc->s_me_ctxt;

    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* recon stride */
    WORD32 i4_rec_strd = ps_proc->i4_rec_strd;

    /* source buffer for halp pel generation functions */
    UWORD8 *pu1_hpel_src;

    /* quantization parameters */
    quant_params_t *ps_qp_params = ps_proc->ps_qp_params[0];

    /* Mb part ctxts for SKIP */
    mb_part_ctxt s_skip_mbpart;

    /* Sad therholds */
    ps_me_ctxt->pu2_sad_thrsh = ps_qp_params->pu2_sad_thrsh;

    {
        WORD32 rows_above, rows_below, columns_left, columns_right;

        /* During evaluation for motion vectors do not search through padded regions */
        /* Obtain number of rows and columns that are effective for computing for me evaluation */
        rows_above = MB_SIZE + ps_proc->i4_mb_y * MB_SIZE;
        rows_below = (ps_proc->i4_ht_mbs - ps_proc->i4_mb_y) * MB_SIZE;
        columns_left = MB_SIZE + ps_proc->i4_mb_x * MB_SIZE;
        columns_right = (ps_proc->i4_wd_mbs - ps_proc->i4_mb_x) * MB_SIZE;

        /* init srch range */
        /* NOTE : For now, lets limit the search range by DEFAULT_MAX_SRCH_RANGE_X / 2
         * on all sides.
         */
        ps_me_ctxt->i4_srch_range_w = -MIN(columns_left, DEFAULT_MAX_SRCH_RANGE_X >> 1);
        ps_me_ctxt->i4_srch_range_e = MIN(columns_right, DEFAULT_MAX_SRCH_RANGE_X >> 1);
        ps_me_ctxt->i4_srch_range_n = -MIN(rows_above, DEFAULT_MAX_SRCH_RANGE_Y >> 1);
        ps_me_ctxt->i4_srch_range_s = MIN(rows_below, DEFAULT_MAX_SRCH_RANGE_Y >> 1);

        /* this is to facilitate fast sub pel computation with minimal loads */
        ps_me_ctxt->i4_srch_range_w += 1;
        ps_me_ctxt->i4_srch_range_e -= 1;
        ps_me_ctxt->i4_srch_range_n += 1;
        ps_me_ctxt->i4_srch_range_s -= 1;
    }

    /* Compute ME and store the MVs */

    /***********************************************************************
     * Compute ME for list L0
     ***********************************************************************/

    /* Init SATQD for the current list */
    ps_me_ctxt->u4_min_sad_reached  = 0;
    ps_me_ctxt->i4_min_sad = ps_proc->ps_cur_mb->u4_min_sad;

    /* Get the seed motion vector candidates                    */
    ih264e_get_search_candidates(ps_proc, ps_me_ctxt, PRED_L0);

    /* ****************************************************************
     *Evaluate the SKIP for current list
     * ****************************************************************/
    s_skip_mbpart.s_mv_curr.i2_mvx = 0;
    s_skip_mbpart.s_mv_curr.i2_mvy = 0;
    s_skip_mbpart.i4_mb_cost = INT_MAX;
    s_skip_mbpart.i4_mb_distortion = INT_MAX;

    ime_compute_skip_cost( ps_me_ctxt,
                           (ime_mv_t *)(&ps_proc->ps_skip_mv[PRED_L0].s_mv),
                           &s_skip_mbpart,
                           ps_proc->ps_codec->s_cfg.u4_enable_satqd,
                           PRED_L0,
                           0 /* Not a Bslice */ );

    s_skip_mbpart.s_mv_curr.i2_mvx <<= 2;
    s_skip_mbpart.s_mv_curr.i2_mvy <<= 2;

    /******************************************************************
     * Evaluate ME For current list
     *****************************************************************/
    ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr.i2_mvx = 0;
    ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr.i2_mvy = 0;
    ps_me_ctxt->as_mb_part[PRED_L0].i4_mb_cost = INT_MAX;
    ps_me_ctxt->as_mb_part[PRED_L0].i4_mb_distortion = INT_MAX;

    /* Init Hpel */
    ps_me_ctxt->as_mb_part[PRED_L0].pu1_best_hpel_buf = NULL;

    /* In case we found out the minimum SAD, exit the ME eval */
    if (!ps_me_ctxt->u4_min_sad_reached)
    {
        /* Evaluate search candidates for initial mv pt */
        ime_evaluate_init_srchposn_16x16(ps_me_ctxt, PRED_L0);

        /********************************************************************/
        /*                  full pel motion estimation                      */
        /********************************************************************/
        ime_full_pel_motion_estimation_16x16(ps_me_ctxt, PRED_L0);

        /* Scale the MV to qpel resolution */
        ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr.i2_mvx <<= 2;
        ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr.i2_mvy <<= 2;

        if (ps_me_ctxt->u4_enable_hpel)
        {
            /* moving src pointer to the converged motion vector location*/
            pu1_hpel_src =   ps_me_ctxt->apu1_ref_buf_luma[PRED_L0]
                             + (ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr.i2_mvx >> 2)
                             + (ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr.i2_mvy >> 2)* i4_rec_strd;

            ps_me_ctxt->apu1_subpel_buffs[0] = ps_proc->apu1_subpel_buffs[0];
            ps_me_ctxt->apu1_subpel_buffs[1] = ps_proc->apu1_subpel_buffs[1];
            ps_me_ctxt->apu1_subpel_buffs[2] = ps_proc->apu1_subpel_buffs[2];

            ps_me_ctxt->u4_subpel_buf_strd = HP_BUFF_WD;

            /* half  pel search is done for both sides of full pel,
             * hence half_x of width x height = 17x16 is created
             * starting from left half_x of converged full pel */
            pu1_hpel_src -= 1;

            /* computing half_x */
            ps_codec->pf_ih264e_sixtapfilter_horz(pu1_hpel_src,
                                                  ps_me_ctxt->apu1_subpel_buffs[0],
                                                  i4_rec_strd,
                                                  ps_me_ctxt->u4_subpel_buf_strd);

            /*
             * Halfpel search is done for both sides of full pel,
             * hence half_y of width x height = 16x17 is created
             * starting from top half_y of converged full pel
             * for half_xy top_left is required
             * hence it starts from pu1_hpel_src = full_pel_converged_point - i4_rec_strd - 1
             */
            pu1_hpel_src -= i4_rec_strd;

            /* computing half_y , and half_xy*/
            ps_codec->pf_ih264e_sixtap_filter_2dvh_vert(
                            pu1_hpel_src, ps_me_ctxt->apu1_subpel_buffs[1],
                            ps_me_ctxt->apu1_subpel_buffs[2], i4_rec_strd,
                            ps_me_ctxt->u4_subpel_buf_strd, ps_proc->ai16_pred1 + 3,
                            ps_me_ctxt->u4_subpel_buf_strd);

            ime_sub_pel_motion_estimation_16x16(ps_me_ctxt, PRED_L0);
        }
    }


    /***********************************************************************
     * If a particular skiip Mv is giving better sad, copy to the corresponding
     * MBPART
     * In B slices this loop should go only to PREDL1: If we found min sad
     * we will go to the skip ref list only
     * Have to find a way to make it without too much change or new vars
     **********************************************************************/
    if (s_skip_mbpart.i4_mb_cost < ps_me_ctxt->as_mb_part[PRED_L0].i4_mb_cost)
    {
        ps_me_ctxt->as_mb_part[PRED_L0].i4_mb_cost = s_skip_mbpart.i4_mb_cost;
        ps_me_ctxt->as_mb_part[PRED_L0].i4_mb_distortion = s_skip_mbpart.i4_mb_distortion;
        ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr = s_skip_mbpart.s_mv_curr;
    }
    else if (ps_me_ctxt->as_mb_part[PRED_L0].pu1_best_hpel_buf)
    {
        /* Now we have to copy the buffers */
        ps_codec->pf_inter_pred_luma_copy(
                        ps_me_ctxt->as_mb_part[PRED_L0].pu1_best_hpel_buf,
                        ps_proc->pu1_best_subpel_buf,
                        ps_me_ctxt->u4_subpel_buf_strd,
                        ps_proc->u4_bst_spel_buf_strd, MB_SIZE, MB_SIZE,
                        NULL, 0);
    }

    /**********************************************************************
     * Now get the minimum of MB part sads by searching over all ref lists
     **********************************************************************/
    ps_proc->ps_pu->s_me_info[PRED_L0].s_mv.i2_mvx = ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr.i2_mvx;
    ps_proc->ps_pu->s_me_info[PRED_L0].s_mv.i2_mvy = ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr.i2_mvy;
    ps_proc->ps_cur_mb->i4_mb_cost = ps_me_ctxt->as_mb_part[PRED_L0].i4_mb_cost;
    ps_proc->ps_cur_mb->i4_mb_distortion = ps_me_ctxt->as_mb_part[PRED_L0].i4_mb_distortion;
    ps_proc->ps_cur_mb->u4_mb_type = P16x16;
    ps_proc->ps_pu->b2_pred_mode = PRED_L0 ;

    /* Mark the reflists */
    ps_proc->ps_pu->s_me_info[0].i1_ref_idx = -1;
    ps_proc->ps_pu->s_me_info[1].i1_ref_idx =  0;

    /* number of partitions */
    ps_proc->u4_num_sub_partitions = 1;
    *(ps_proc->pu4_mb_pu_cnt) = 1;

    /* position in-terms of PU */
    ps_proc->ps_pu->b4_pos_x = 0;
    ps_proc->ps_pu->b4_pos_y = 0;

    /* PU size */
    ps_proc->ps_pu->b4_wd = 3;
    ps_proc->ps_pu->b4_ht = 3;

    /* Update min sad conditions */
    if (ps_me_ctxt->u4_min_sad_reached == 1)
    {
        ps_proc->ps_cur_mb->u4_min_sad_reached = 1;
        ps_proc->ps_cur_mb->u4_min_sad = ps_me_ctxt->i4_min_sad;
    }

}

/**
*******************************************************************************
*
* @brief This function performs motion estimation for the current NMB
*
* @par Description:
* Intializes input and output pointers required by the function ih264e_compute_me
* and calls the function ih264e_compute_me in a loop to process NMBs.
*
* @param[in] ps_proc
*  Process context corresponding to the job
*
* @returns
*
* @remarks none
*
*******************************************************************************
*/
void ih264e_compute_me_nmb(process_ctxt_t *ps_proc, UWORD32 u4_nmb_count)
{
    /* pic pu */
    enc_pu_t *ps_pu_begin = ps_proc->ps_pu;

    /* ME map */
    UWORD8 *pu1_me_map = ps_proc->pu1_me_map + (ps_proc->i4_mb_y * ps_proc->i4_wd_mbs);

    /* temp var */
    UWORD32 u4_i;

    ps_proc->s_me_ctxt.u4_left_is_intra = ps_proc->s_left_mb_syntax_ele.u2_is_intra;
    ps_proc->s_me_ctxt.u4_left_is_skip = (ps_proc->s_left_mb_syntax_ele.u2_mb_type == PSKIP);

    for (u4_i = 0; u4_i < u4_nmb_count; u4_i++)
    {
        /* Wait for ME map */
        if (ps_proc->i4_mb_y > 0)
        {
            /* Wait for top right ME to be done */
            UWORD8 *pu1_me_map_tp_rw = ps_proc->pu1_me_map + (ps_proc->i4_mb_y - 1) * ps_proc->i4_wd_mbs;

            while (1)
            {
                volatile UWORD8 *pu1_buf;
                WORD32 idx = ps_proc->i4_mb_x + u4_i + 1;

                idx = MIN(idx, (ps_proc->i4_wd_mbs - 1));
                pu1_buf =  pu1_me_map_tp_rw + idx;
                if(*pu1_buf)
                    break;
                ithread_yield();
            }
        }

        ps_proc->ps_skip_mv = &(ps_proc->ps_nmb_info[u4_i].as_skip_mv[0]);
        ps_proc->ps_ngbr_avbl = &(ps_proc->ps_nmb_info[u4_i].s_ngbr_avbl);
        ps_proc->ps_pred_mv = &(ps_proc->ps_nmb_info[u4_i].as_pred_mv[0]);

        ps_proc->ps_cur_mb = &(ps_proc->ps_nmb_info[u4_i]);

        ps_proc->ps_cur_mb->u4_min_sad = ps_proc->u4_min_sad;
        ps_proc->ps_cur_mb->u4_min_sad_reached = 0;

        ps_proc->ps_cur_mb->i4_mb_cost = INT_MAX;
        ps_proc->ps_cur_mb->i4_mb_distortion = SHRT_MAX;

        /* Set the best subpel buf to the correct mb so that the buffer can be copied */
        ps_proc->pu1_best_subpel_buf = ps_proc->ps_nmb_info[u4_i].pu1_best_sub_pel_buf;
        ps_proc->u4_bst_spel_buf_strd = ps_proc->ps_nmb_info[u4_i].u4_bst_spel_buf_strd;

        /* Set the min sad conditions */
        ps_proc->ps_cur_mb->u4_min_sad = ps_proc->ps_codec->u4_min_sad;
        ps_proc->ps_cur_mb->u4_min_sad_reached = 0;

        /* Derive neighbor availability for the current macroblock */
        ih264e_derive_nghbr_avbl_of_mbs(ps_proc);

        /* init me */
        ih264e_init_me(ps_proc);

        /* Compute ME according to slice type */
        ps_proc->ps_codec->apf_compute_me[ps_proc->i4_slice_type](ps_proc);

        /* update top and left structs */
        {
            mb_info_t *ps_top_syn = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x;
            mb_info_t *ps_top_left_syn = &(ps_proc->s_top_left_mb_syntax_ME);
            enc_pu_t *ps_left_mb_pu = &ps_proc->s_left_mb_pu_ME;
            enc_pu_t *ps_top_left_mb_pu = &ps_proc->s_top_left_mb_pu_ME;
            enc_pu_t *ps_top_mv = ps_proc->ps_top_row_pu_ME + ps_proc->i4_mb_x;

            *ps_top_left_syn = *ps_top_syn;

            *ps_top_left_mb_pu = *ps_top_mv;
            *ps_left_mb_pu = *ps_proc->ps_pu;
        }

        ps_proc->ps_pu += *ps_proc->pu4_mb_pu_cnt;

        /* Copy the min sad reached info */
        ps_proc->ps_nmb_info[u4_i].u4_min_sad_reached = ps_proc->ps_cur_mb->u4_min_sad_reached;
        ps_proc->ps_nmb_info[u4_i].u4_min_sad   = ps_proc->ps_cur_mb->u4_min_sad;

        /*
         * To make sure that the MV map is properly sync to the
         * cache we need to do a DDB
         */
        {
            DATA_SYNC();

            pu1_me_map[ps_proc->i4_mb_x] = 1;
        }
        ps_proc->i4_mb_x++;

        ps_proc->s_me_ctxt.u4_left_is_intra = 0;
        ps_proc->s_me_ctxt.u4_left_is_skip = (ps_proc->ps_cur_mb->u4_mb_type  == PSKIP);

        /* update buffers pointers */
        ps_proc->pu1_src_buf_luma += MB_SIZE;
        ps_proc->pu1_rec_buf_luma += MB_SIZE;
        ps_proc->apu1_ref_buf_luma[0] += MB_SIZE;
        ps_proc->apu1_ref_buf_luma[1] += MB_SIZE;

        /*
         * Note: Although chroma mb size is 8, as the chroma buffers are interleaved,
         * the stride per MB is MB_SIZE
         */
        ps_proc->pu1_src_buf_chroma += MB_SIZE;
        ps_proc->pu1_rec_buf_chroma += MB_SIZE;
        ps_proc->apu1_ref_buf_chroma[0] += MB_SIZE;
        ps_proc->apu1_ref_buf_chroma[1] += MB_SIZE;


        ps_proc->pu4_mb_pu_cnt += 1;
    }


    ps_proc->ps_pu = ps_pu_begin;
    ps_proc->i4_mb_x = ps_proc->i4_mb_x - u4_nmb_count;

    /* update buffers pointers */
    ps_proc->pu1_src_buf_luma -= MB_SIZE * u4_nmb_count;
    ps_proc->pu1_rec_buf_luma -= MB_SIZE * u4_nmb_count;
    ps_proc->apu1_ref_buf_luma[0] -= MB_SIZE * u4_nmb_count;
    ps_proc->apu1_ref_buf_luma[1] -= MB_SIZE * u4_nmb_count;

    /*
     * Note: Although chroma mb size is 8, as the chroma buffers are interleaved,
     * the stride per MB is MB_SIZE
     */
    ps_proc->pu1_src_buf_chroma -= MB_SIZE * u4_nmb_count;
    ps_proc->pu1_rec_buf_chroma -= MB_SIZE * u4_nmb_count;
    ps_proc->apu1_ref_buf_chroma[0] -= MB_SIZE * u4_nmb_count;
    ps_proc->apu1_ref_buf_chroma[1] -= MB_SIZE * u4_nmb_count;


    ps_proc->pu4_mb_pu_cnt -= u4_nmb_count;
}


/**
*******************************************************************************
*
* @brief The function computes parameters for a BSKIP MB
*
* @par Description:
*  The function updates the skip motion vector for B Mb, check if the Mb can be
*  marked as skip and returns it
*
* @param[in] ps_proc
*  Pointer to process context
*
* @param[in] u4_for_me
*  Dummy
*
* @param[in] i4_reflist
*  Dummy
*
* @returns Flag indicating if the current Mb can be skip or not
*
* @remarks
*   The code implements the logic as described in sec 8.4.1.2.2
*   It also computes co-located MB parmas according to sec 8.4.1.2.1
*
*   Need to add condition for this fucntion to be used in ME
*
*******************************************************************************/
WORD32 ih264e_find_bskip_params_me(process_ctxt_t *ps_proc, WORD32 i4_reflist)
{
    /* Colzero for co-located MB */
    WORD32 i4_colzeroflag;

    /* motion vectors for neighbouring MBs */
    enc_pu_t *ps_a_pu, *ps_c_pu, *ps_b_pu;

    /* Variables to check if a particular mB is available */
    WORD32 i4_a, i4_b, i4_c, i4_c_avail;

    /* Mode availability, init to no modes available     */
    WORD32 i4_mode_avail;

    /*  mb neighbor availability */
    block_neighbors_t *ps_ngbr_avbl = ps_proc->ps_ngbr_avbl;

    /* Temp var */
    WORD32 i, i4_cmpl_mode, i4_skip_type = -1;

    /*
     * Colocated motion vector
     */
    mv_t s_mvcol;

    /*
     * Colocated picture idx
     */
    WORD32 i4_refidxcol;

    UNUSED(i4_reflist);

    /**************************************************************************
     *Find co-located MB parameters
     *      See sec 8.4.1.2.1  for reference
     **************************************************************************/
    {
        /*
         * Find the co-located Mb and update the skip and pred appropriately
         * 1) Default colpic is forward ref : Table 8-6
         * 2) Default mb col is current MB : Table 8-8
         */

        if (ps_proc->ps_colpu->b1_intra_flag)
        {
            s_mvcol.i2_mvx = 0;
            s_mvcol.i2_mvy = 0;
            i4_refidxcol = -1;
        }
        else
        {
            if (ps_proc->ps_colpu->b2_pred_mode != PRED_L1)
            {
                s_mvcol = ps_proc->ps_colpu->s_me_info[PRED_L0].s_mv;
                i4_refidxcol = 0;
            }
            else // if(ps_proc->ps_colpu->b2_pred_mode != PRED_L0)
            {
                s_mvcol = ps_proc->ps_colpu->s_me_info[PRED_L1].s_mv;
                i4_refidxcol = 0;
            }
        }

        /* RefPicList1[ 0 ]  is marked as  "used for short-term reference", as default */
        i4_colzeroflag = (!i4_refidxcol && (ABS(s_mvcol.i2_mvx) <= 1)
                        && (ABS(s_mvcol.i2_mvy) <= 1));

    }

    /***************************************************************************
     * Evaluating skip params : Spatial Skip
     **************************************************************************/
    {
    /* Get the neighbouring MBS according to Section 8.4.1.2.2 */
    ps_a_pu = &ps_proc->s_left_mb_pu_ME;
    ps_b_pu = (ps_proc->ps_top_row_pu_ME + ps_proc->i4_mb_x);

    i4_c_avail = 0;
    if (ps_ngbr_avbl->u1_mb_c)
    {
        ps_c_pu = &((ps_proc->ps_top_row_pu_ME + ps_proc->i4_mb_x)[1]);
        i4_c_avail = 1;
    }
    else
    {
        ps_c_pu = &ps_proc->s_top_left_mb_pu_ME;
        i4_c_avail = ps_ngbr_avbl->u1_mb_d;
    }

    i4_a = ps_ngbr_avbl->u1_mb_a;
    i4_b = ps_ngbr_avbl->u1_mb_b;
    i4_c = i4_c_avail;

    /* Init to no mode avail */
    i4_mode_avail = 0;
    for (i = 0; i < 2; i++)
    {
        i4_cmpl_mode = (i == 0) ? PRED_L1 : PRED_L0;

        i4_mode_avail |= (i4_a && (ps_a_pu->b2_pred_mode != i4_cmpl_mode) && (ps_a_pu->s_me_info[i].i1_ref_idx != 0))<<i;
        i4_mode_avail |= (i4_b && (ps_b_pu->b2_pred_mode != i4_cmpl_mode) && (ps_b_pu->s_me_info[i].i1_ref_idx != 0))<<i;
        i4_mode_avail |= (i4_c && (ps_c_pu->b2_pred_mode != i4_cmpl_mode) && (ps_c_pu->s_me_info[i].i1_ref_idx != 0))<<i;
    }

    if (i4_mode_avail == 0x3 || i4_mode_avail == 0x0)
    {
        i4_skip_type= PRED_BI;
    }
    else if(i4_mode_avail == 0x1)
    {
        i4_skip_type = PRED_L0;
    }
    else if(i4_mode_avail == 0x2)
    {
        i4_skip_type = PRED_L1;
    }

    /* Update skip MV for L0 */
    if ((i4_mode_avail & 0x1) && (!i4_colzeroflag))
    {
        ps_proc->ps_skip_mv[0].s_mv.i2_mvx = ps_proc->ps_pred_mv[0].s_mv.i2_mvx;
        ps_proc->ps_skip_mv[0].s_mv.i2_mvy = ps_proc->ps_pred_mv[0].s_mv.i2_mvy;
    }
    else
    {
        ps_proc->ps_skip_mv[0].s_mv.i2_mvx = 0;
        ps_proc->ps_skip_mv[0].s_mv.i2_mvy = 0;
    }

    /* Update skip MV for L1 */
    if ((i4_mode_avail & 0x2) && (!i4_colzeroflag))
    {
        ps_proc->ps_skip_mv[1].s_mv.i2_mvx = ps_proc->ps_pred_mv[1].s_mv.i2_mvx;
        ps_proc->ps_skip_mv[1].s_mv.i2_mvy = ps_proc->ps_pred_mv[1].s_mv.i2_mvy;
    }
    else
    {
        ps_proc->ps_skip_mv[1].s_mv.i2_mvx = 0;
        ps_proc->ps_skip_mv[1].s_mv.i2_mvy = 0;
    }

    }

    /***************************************************************************
     * Evaluating skip params : Temporal skip
     **************************************************************************/
    {
        pic_buf_t *  ps_ref_pic[MAX_REF_PIC_CNT];
        WORD32 i4_td, i4_tx, i4_tb, i4_dist_scale_factor;
        enc_pu_mv_t *ps_skip_mv = &ps_proc->ps_skip_mv[2];

        ps_ref_pic[PRED_L0] = ps_proc->aps_ref_pic[PRED_L0];
        ps_ref_pic[PRED_L1] = ps_proc->aps_ref_pic[PRED_L1];

        i4_tb = ps_proc->ps_codec->i4_poc - ps_ref_pic[PRED_L0]->i4_abs_poc;
        i4_td = ps_ref_pic[PRED_L1]->i4_abs_poc - ps_ref_pic[PRED_L0]->i4_abs_poc;

        i4_tb = CLIP3(-128, 127, i4_tb);
        i4_td = CLIP3(-128, 127, i4_td);

        i4_tx = ( 16384 + ABS( i4_td / 2 ) ) / i4_td ;
        i4_dist_scale_factor =  CLIP3( -1024, 1023, ( i4_tb * i4_tx + 32 ) >> 6 );

        /* Motion vectors taken in full pel resolution , hence  -> (& 0xfffc) operation */
        ps_skip_mv[PRED_L0].s_mv.i2_mvx = (( i4_dist_scale_factor * s_mvcol.i2_mvx + 128 ) >> 8) & 0xfffc;
        ps_skip_mv[PRED_L0].s_mv.i2_mvy = (( i4_dist_scale_factor * s_mvcol.i2_mvy + 128 ) >> 8) & 0xfffc;

        ps_skip_mv[PRED_L1].s_mv.i2_mvx = (ps_skip_mv[PRED_L0].s_mv.i2_mvx - s_mvcol.i2_mvx) & 0xfffc;
        ps_skip_mv[PRED_L1].s_mv.i2_mvy = (ps_skip_mv[PRED_L0].s_mv.i2_mvy - s_mvcol.i2_mvy) & 0xfffc;

    }

    return i4_skip_type;
}

/**
*******************************************************************************
*
* @brief The function computes the skip motion vectoe for B mb
*
* @par Description:
*  The function gives the skip motion vector for B Mb, check if the Mb can be
*  marked as skip
*
* @param[in] ps_proc
*  Pointer to process context
*
* @param[in] u4_for_me
*  Dummy
*
* @param[in] u4_for_me
*  Dummy
*
* @returns Flag indicating if the current Mb can be skip or not
*
* @remarks The code implements the logic as described in sec 8.4.1.2.2 in H264
*   specification. It also computes co-located MB parmas according to sec 8.4.1.2.1
*
*******************************************************************************/
WORD32 ih264e_find_bskip_params(process_ctxt_t *ps_proc, WORD32 i4_reflist)
{
    WORD32 i4_colzeroflag;

    /* motion vectors */
    enc_pu_t *ps_a_pu, *ps_c_pu, *ps_b_pu;

    /* Syntax elem */
    mb_info_t *ps_a_syn, *ps_b_syn, *ps_c_syn;

    /* Variables to check if a particular mB is available */
    WORD32 i4_a, i4_b, i4_c, i4_c_avail;

    /* Mode availability, init to no modes available     */
    WORD32 i4_mode_avail;

    /*  mb neighbor availability */
    block_neighbors_t *ps_ngbr_avbl = ps_proc->ps_ngbr_avbl;

    /* Temp var */
    WORD32 i, i4_cmpl_mode;

    UNUSED(i4_reflist);

    /**************************************************************************
     *Find co-locates parameters
     *      See sec 8.4.1.2.1  for reference
     **************************************************************************/
    {
        /*
         * Find the co-located Mb and update the skip and pred appropriately
         * 1) Default colpic is forward ref : Table 8-6
         * 2) Default mb col is current MB : Table 8-8
         */

        mv_t s_mvcol;
        WORD32 i4_refidxcol;

        if (ps_proc->ps_colpu->b1_intra_flag)
        {
            s_mvcol.i2_mvx = 0;
            s_mvcol.i2_mvy = 0;
            i4_refidxcol = -1;
        }
        else
        {
            if (ps_proc->ps_colpu->b2_pred_mode != PRED_L1)
            {
                s_mvcol = ps_proc->ps_colpu->s_me_info[PRED_L0].s_mv;
                i4_refidxcol = 0;
            }
            else // if(ps_proc->ps_colpu->b2_pred_mode != PRED_L0)
            {
                s_mvcol = ps_proc->ps_colpu->s_me_info[PRED_L1].s_mv;
                i4_refidxcol = 0;
            }
        }

        /* RefPicList1[ 0 ]  is marked as  "used for short-term reference", as default */
        i4_colzeroflag = (!i4_refidxcol && (ABS(s_mvcol.i2_mvx) <= 1)
                        && (ABS(s_mvcol.i2_mvy) <= 1));

    }

    /***************************************************************************
     * Evaluating skip params
     **************************************************************************/
    /* Section 8.4.1.2.2 */
    ps_a_syn = &ps_proc->s_left_mb_syntax_ele;
    ps_a_pu = &ps_proc->s_left_mb_pu;

    ps_b_syn = ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x;
    ps_b_pu = (ps_proc->ps_top_row_pu + ps_proc->i4_mb_x);

    i4_c_avail = 0;
    if (ps_ngbr_avbl->u1_mb_c)
    {
        ps_c_syn = &((ps_proc->ps_top_row_mb_syntax_ele + ps_proc->i4_mb_x)[1]);
        ps_c_pu = &((ps_proc->ps_top_row_pu + ps_proc->i4_mb_x)[1]);
        i4_c_avail = 1;
    }
    else
    {
        ps_c_syn = &(ps_proc->s_top_left_mb_syntax_ele);
        ps_c_pu = &ps_proc->s_top_left_mb_pu;
        i4_c_avail = ps_ngbr_avbl->u1_mb_d;
    }


    i4_a = ps_ngbr_avbl->u1_mb_a;
    i4_a &= !ps_a_syn->u2_is_intra;

    i4_b = ps_ngbr_avbl->u1_mb_b;
    i4_b &= !ps_b_syn->u2_is_intra;

    i4_c = i4_c_avail;
    i4_c &= !ps_c_syn->u2_is_intra;

    /* Init to no mode avail */
    i4_mode_avail = 0;
    for (i = 0; i < 2; i++)
    {
        i4_cmpl_mode = (i == 0) ? PRED_L1 : PRED_L0;

        i4_mode_avail |= (i4_a && (ps_a_pu->b2_pred_mode != i4_cmpl_mode) && (ps_a_pu->s_me_info[i].i1_ref_idx != 0))<<i;
        i4_mode_avail |= (i4_b && (ps_b_pu->b2_pred_mode != i4_cmpl_mode) && (ps_b_pu->s_me_info[i].i1_ref_idx != 0))<<i;
        i4_mode_avail |= (i4_c && (ps_c_pu->b2_pred_mode != i4_cmpl_mode) && (ps_c_pu->s_me_info[i].i1_ref_idx != 0))<<i;
    }

    /* Update skip MV for L0 */
    if ((i4_mode_avail & 0x1) && (!i4_colzeroflag))
    {
        ps_proc->ps_skip_mv[0].s_mv.i2_mvx = ps_proc->ps_pred_mv[0].s_mv.i2_mvx;
        ps_proc->ps_skip_mv[0].s_mv.i2_mvy = ps_proc->ps_pred_mv[0].s_mv.i2_mvy;
    }
    else
    {
        ps_proc->ps_skip_mv[0].s_mv.i2_mvx = 0;
        ps_proc->ps_skip_mv[0].s_mv.i2_mvy = 0;
    }

    /* Update skip MV for L1 */
    if ((i4_mode_avail & 0x2) && (!i4_colzeroflag))
    {
        ps_proc->ps_skip_mv[1].s_mv.i2_mvx = ps_proc->ps_pred_mv[1].s_mv.i2_mvx;
        ps_proc->ps_skip_mv[1].s_mv.i2_mvy = ps_proc->ps_pred_mv[1].s_mv.i2_mvy;
    }
    else
    {
        ps_proc->ps_skip_mv[1].s_mv.i2_mvx = 0;
        ps_proc->ps_skip_mv[1].s_mv.i2_mvy = 0;
    }

    /* Now see if the ME information matches the SKIP information */
    switch (ps_proc->ps_pu->b2_pred_mode)
    {
        case PRED_BI:
            if (  (ps_proc->ps_pu->s_me_info[0].s_mv.i2_mvx == ps_proc->ps_skip_mv[0].s_mv.i2_mvx)
               && (ps_proc->ps_pu->s_me_info[0].s_mv.i2_mvy == ps_proc->ps_skip_mv[0].s_mv.i2_mvy)
               && (ps_proc->ps_pu->s_me_info[1].s_mv.i2_mvx == ps_proc->ps_skip_mv[1].s_mv.i2_mvx)
               && (ps_proc->ps_pu->s_me_info[1].s_mv.i2_mvy == ps_proc->ps_skip_mv[1].s_mv.i2_mvy)
               && (i4_mode_avail ==  0x3 || i4_mode_avail == 0x0))
            {
                return 1;
            }
            break;

        case PRED_L0:
            if ( (ps_proc->ps_pu->s_me_info[0].s_mv.i2_mvx == ps_proc->ps_skip_mv[0].s_mv.i2_mvx)
              && (ps_proc->ps_pu->s_me_info[0].s_mv.i2_mvy == ps_proc->ps_skip_mv[0].s_mv.i2_mvy)
              && (i4_mode_avail == 0x1))
            {
                return 1;
            }
            break;

        case PRED_L1:
            if (  (ps_proc->ps_pu->s_me_info[1].s_mv.i2_mvx == ps_proc->ps_skip_mv[1].s_mv.i2_mvx)
               && (ps_proc->ps_pu->s_me_info[1].s_mv.i2_mvy == ps_proc->ps_skip_mv[1].s_mv.i2_mvy)
               && (i4_mode_avail == 0x2))
            {
                return 1;
            }
            break;
    }

    return 0;
}


/**
*******************************************************************************
*
* @brief This function computes the best motion vector among the tentative mv
* candidates chosen.
*
* @par Description:
*  This function determines the position in the search window at which the motion
*  estimation should begin in order to minimise the number of search iterations.
*
* @param[in] ps_mb_part
*  pointer to current mb partition ctxt with respect to ME
*
* @param[in] u4_lambda_motion
*  lambda motion
*
* @param[in] u4_fast_flag
*  enable/disable fast sad computation
*
* @returns  mv pair & corresponding distortion and cost
*
* @remarks Currently onyl 4 search candiates are supported
*
*******************************************************************************
*/
void ih264e_evaluate_bipred(me_ctxt_t *ps_me_ctxt,
                            process_ctxt_t *ps_proc,
                            mb_part_ctxt *ps_mb_ctxt_bi)
{

    UWORD32 i, u4_fast_sad;

    WORD32 i4_dest_buff;

    mv_t *ps_l0_pred_mv, *ps_l1_pred_mv, s_l0_mv, s_l1_mv;

    UWORD8 *pu1_ref_mb_l0, *pu1_ref_mb_l1;

    UWORD8 *pu1_dst_buf;

    WORD32 i4_ref_l0_stride, i4_ref_l1_stride;

    WORD32 i4_mb_distortion, i4_mb_cost;

    u4_fast_sad = ps_me_ctxt->u4_enable_fast_sad;

    i4_dest_buff = 0;
    for (i = 0; i < ps_me_ctxt->u4_num_candidates[PRED_BI]; i += 2)
    {
        pu1_dst_buf = ps_me_ctxt->apu1_subpel_buffs[i4_dest_buff];

        s_l0_mv.i2_mvx = ps_me_ctxt->as_mv_init_search[PRED_BI][i].i2_mvx >> 2;
        s_l0_mv.i2_mvy = ps_me_ctxt->as_mv_init_search[PRED_BI][i].i2_mvy >> 2;
        s_l1_mv.i2_mvx = ps_me_ctxt->as_mv_init_search[PRED_BI][i + 1].i2_mvx >> 2;
        s_l1_mv.i2_mvy = ps_me_ctxt->as_mv_init_search[PRED_BI][i + 1].i2_mvy >> 2;

        ps_l0_pred_mv = &ps_proc->ps_pred_mv[PRED_L0].s_mv;
        ps_l1_pred_mv = &ps_proc->ps_pred_mv[PRED_L1].s_mv;

        if ((ps_me_ctxt->as_mv_init_search[PRED_BI][i].i2_mvx & 0x3)||
                        (ps_me_ctxt->as_mv_init_search[PRED_BI][i].i2_mvy & 0x3))
        {
            pu1_ref_mb_l0 = ps_me_ctxt->as_mb_part[PRED_L0].pu1_best_hpel_buf;
            i4_ref_l0_stride = ps_me_ctxt->u4_subpel_buf_strd;
        }
        else
        {
            pu1_ref_mb_l0 = ps_me_ctxt->apu1_ref_buf_luma[PRED_L0] + (s_l0_mv.i2_mvx) + ((s_l0_mv.i2_mvy) * ps_me_ctxt->i4_rec_strd);
            i4_ref_l0_stride = ps_me_ctxt->i4_rec_strd;
        }


        if ((ps_me_ctxt->as_mv_init_search[PRED_BI][i + 1].i2_mvx & 0x3) ||
                        (ps_me_ctxt->as_mv_init_search[PRED_BI][i + 1].i2_mvy & 0x3))
        {
            pu1_ref_mb_l1 = ps_me_ctxt->as_mb_part[PRED_L1].pu1_best_hpel_buf;
            i4_ref_l1_stride = ps_me_ctxt->u4_subpel_buf_strd;
        }
        else
        {
            pu1_ref_mb_l1 = ps_me_ctxt->apu1_ref_buf_luma[PRED_L1] + (s_l1_mv.i2_mvx) + ((s_l1_mv.i2_mvy) * ps_me_ctxt->i4_rec_strd);
            i4_ref_l1_stride = ps_me_ctxt->i4_rec_strd;
        }

        ps_proc->ps_codec->pf_inter_pred_luma_bilinear(
                        pu1_ref_mb_l0, pu1_ref_mb_l1, pu1_dst_buf,
                        i4_ref_l0_stride, i4_ref_l1_stride,
                        ps_me_ctxt->u4_subpel_buf_strd, MB_SIZE, MB_SIZE);

        ps_me_ctxt->pf_ime_compute_sad_16x16[u4_fast_sad](
                        ps_me_ctxt->pu1_src_buf_luma, pu1_dst_buf,
                        ps_me_ctxt->i4_src_strd, ps_me_ctxt->u4_subpel_buf_strd,
                        INT_MAX, &i4_mb_distortion);

        /* compute cost */
        i4_mb_cost =  ps_me_ctxt->pu1_mv_bits[ps_me_ctxt->as_mv_init_search[PRED_BI][i].i2_mvx - ps_l0_pred_mv->i2_mvx];
        i4_mb_cost += ps_me_ctxt->pu1_mv_bits[ps_me_ctxt->as_mv_init_search[PRED_BI][i].i2_mvy - ps_l0_pred_mv->i2_mvy];
        i4_mb_cost += ps_me_ctxt->pu1_mv_bits[ps_me_ctxt->as_mv_init_search[PRED_BI][i + 1].i2_mvx - ps_l1_pred_mv->i2_mvx];
        i4_mb_cost += ps_me_ctxt->pu1_mv_bits[ps_me_ctxt->as_mv_init_search[PRED_BI][i + 1].i2_mvy - ps_l1_pred_mv->i2_mvy];

        i4_mb_cost -= (ps_me_ctxt->i4_skip_bias[BSLICE]) * (ps_me_ctxt->i4_skip_type == PRED_BI) * (i == 0);


        i4_mb_cost *= ps_me_ctxt->u4_lambda_motion;
        i4_mb_cost += i4_mb_distortion;

        if (i4_mb_cost < ps_mb_ctxt_bi->i4_mb_cost)
        {
            ps_mb_ctxt_bi->i4_srch_pos_idx = (i>>1);
            ps_mb_ctxt_bi->i4_mb_cost = i4_mb_cost;
            ps_mb_ctxt_bi->i4_mb_distortion = i4_mb_distortion;
            ps_mb_ctxt_bi->pu1_best_hpel_buf = pu1_dst_buf;
            i4_dest_buff = (i4_dest_buff + 1) % 2;
        }
    }

}

/**
*******************************************************************************
*
* @brief This function performs motion estimation for the current mb
*
* @par Description:
*  The current mb is compared with a list of mb's in the reference frame for
*  least cost. The mb that offers least cost is chosen as predicted mb and the
*  displacement of the predicted mb from index location of the current mb is
*  signaled as mv. The list of the mb's that are chosen in the reference frame
*  are dependent on the speed of the ME configured.
*
* @param[in] ps_proc
*  Process context corresponding to the job
*
* @returns  motion vector of the pred mb, sad, cost.
*
* @remarks none
*
*******************************************************************************
*/
void ih264e_compute_me_multi_reflist(process_ctxt_t *ps_proc)
{
    /* me ctxt */
    me_ctxt_t *ps_me_ctxt = &ps_proc->s_me_ctxt;

    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* Temp variables for looping over ref lists */
    WORD32 i4_reflist, i4_max_reflist;

    /* recon stride */
    WORD32 i4_rec_strd = ps_proc->i4_rec_strd;

    /* source buffer for halp pel generation functions */
    UWORD8 *pu1_hpel_src;

    /* quantization parameters */
    quant_params_t *ps_qp_params = ps_proc->ps_qp_params[0];

    /* Mb part ctxts for SKIP */
    mb_part_ctxt as_skip_mbpart[2];

    /* Sad therholds */
    ps_me_ctxt->pu2_sad_thrsh = ps_qp_params->pu2_sad_thrsh;

    {
        WORD32 rows_above, rows_below, columns_left, columns_right;

        /* During evaluation for motion vectors do not search through padded regions */
        /* Obtain number of rows and columns that are effective for computing for me evaluation */
        rows_above = MB_SIZE + ps_proc->i4_mb_y * MB_SIZE;
        rows_below = (ps_proc->i4_ht_mbs - ps_proc->i4_mb_y) * MB_SIZE;
        columns_left = MB_SIZE + ps_proc->i4_mb_x * MB_SIZE;
        columns_right = (ps_proc->i4_wd_mbs - ps_proc->i4_mb_x) * MB_SIZE;

        /* init srch range */
        /* NOTE : For now, lets limit the search range by DEFAULT_MAX_SRCH_RANGE_X / 2
         * on all sides.
         */
        ps_me_ctxt->i4_srch_range_w = -MIN(columns_left, DEFAULT_MAX_SRCH_RANGE_X >> 1);
        ps_me_ctxt->i4_srch_range_e = MIN(columns_right, DEFAULT_MAX_SRCH_RANGE_X >> 1);
        ps_me_ctxt->i4_srch_range_n = -MIN(rows_above, DEFAULT_MAX_SRCH_RANGE_Y >> 1);
        ps_me_ctxt->i4_srch_range_s = MIN(rows_below, DEFAULT_MAX_SRCH_RANGE_Y >> 1);

        /* this is to facilitate fast sub pel computation with minimal loads */
        if (ps_me_ctxt->u4_enable_hpel)
        {
            ps_me_ctxt->i4_srch_range_w += 1;
            ps_me_ctxt->i4_srch_range_e -= 1;
            ps_me_ctxt->i4_srch_range_n += 1;
            ps_me_ctxt->i4_srch_range_s -= 1;
        }
    }

    /* Compute ME and store the MVs */
    {
        /***********************************************************************
         * Compute ME for lists L0 and L1
         *  For L0 -> L0 skip + L0
         *  for L1 -> L0 skip + L0 + L1 skip + L1
         ***********************************************************************/
        i4_max_reflist = (ps_proc->i4_slice_type == PSLICE) ? PRED_L0 : PRED_L1;

        /* Init SATQD for the current list */
        ps_me_ctxt->u4_min_sad_reached  = 0;
        ps_me_ctxt->i4_min_sad = ps_proc->ps_cur_mb->u4_min_sad;

        for (i4_reflist = PRED_L0; i4_reflist <= i4_max_reflist; i4_reflist++)
        {

            /* Get the seed motion vector candidates                    */
            ih264e_get_search_candidates(ps_proc, ps_me_ctxt, i4_reflist);

            /* ****************************************************************
             *Evaluate the SKIP for current list
             * ****************************************************************/
            as_skip_mbpart[i4_reflist].s_mv_curr.i2_mvx = 0;
            as_skip_mbpart[i4_reflist].s_mv_curr.i2_mvy = 0;
            as_skip_mbpart[i4_reflist].i4_mb_cost = INT_MAX;
            as_skip_mbpart[i4_reflist].i4_mb_distortion = INT_MAX;

            if (ps_me_ctxt->i4_skip_type == i4_reflist)
            {
                ime_compute_skip_cost( ps_me_ctxt,
                                       (ime_mv_t *)(&ps_proc->ps_skip_mv[i4_reflist].s_mv),
                                       &as_skip_mbpart[i4_reflist],
                                       ps_proc->ps_codec->s_cfg.u4_enable_satqd,
                                       i4_reflist,
                                       (ps_proc->i4_slice_type == BSLICE) );
            }

            as_skip_mbpart[i4_reflist].s_mv_curr.i2_mvx <<= 2;
            as_skip_mbpart[i4_reflist].s_mv_curr.i2_mvy <<= 2;

            /******************************************************************
             * Evaluate ME For current list
             *****************************************************************/
            ps_me_ctxt->as_mb_part[i4_reflist].s_mv_curr.i2_mvx = 0;
            ps_me_ctxt->as_mb_part[i4_reflist].s_mv_curr.i2_mvy = 0;
            ps_me_ctxt->as_mb_part[i4_reflist].i4_mb_cost = INT_MAX;
            ps_me_ctxt->as_mb_part[i4_reflist].i4_mb_distortion = INT_MAX;

            /* Init Hpel */
            ps_me_ctxt->as_mb_part[i4_reflist].pu1_best_hpel_buf = NULL;

            /* In case we found out the minimum SAD, exit the ME eval */
            if (ps_me_ctxt->u4_min_sad_reached)
            {
                i4_max_reflist = i4_reflist;
                break;
            }


            /* Evaluate search candidates for initial mv pt */
            ime_evaluate_init_srchposn_16x16(ps_me_ctxt, i4_reflist);

            /********************************************************************/
            /*                  full pel motion estimation                      */
            /********************************************************************/
            ime_full_pel_motion_estimation_16x16(ps_me_ctxt, i4_reflist);

            DEBUG_MV_HISTOGRAM_ADD((ps_me_ctxt->s_mb_part.s_mv_curr.i2_mvx >> 2),
                                   (ps_me_ctxt->s_mb_part.s_mv_curr.i2_mvy >> 2));

            DEBUG_SAD_HISTOGRAM_ADD(ps_me_ctxt->s_mb_part.i4_mb_distortion, 1);

            /* Scale the MV to qpel resolution */
            ps_me_ctxt->as_mb_part[i4_reflist].s_mv_curr.i2_mvx <<= 2;
            ps_me_ctxt->as_mb_part[i4_reflist].s_mv_curr.i2_mvy <<= 2;

            if (ps_me_ctxt->u4_enable_hpel)
            {
                /* moving src pointer to the converged motion vector location */
                pu1_hpel_src =   ps_me_ctxt->apu1_ref_buf_luma[i4_reflist]
                               + (ps_me_ctxt->as_mb_part[i4_reflist].s_mv_curr.i2_mvx >> 2)
                               + ((ps_me_ctxt->as_mb_part[i4_reflist].s_mv_curr.i2_mvy >> 2)* i4_rec_strd);

                ps_me_ctxt->apu1_subpel_buffs[0] = ps_proc->apu1_subpel_buffs[0];
                ps_me_ctxt->apu1_subpel_buffs[1] = ps_proc->apu1_subpel_buffs[1];
                ps_me_ctxt->apu1_subpel_buffs[2] = ps_proc->apu1_subpel_buffs[2];

                /* Init the search position to an invalid number */
                ps_me_ctxt->as_mb_part[i4_reflist].i4_srch_pos_idx = 3;

                /* Incase a buffer is still in use by L0, replace it with spare buff */
                ps_me_ctxt->apu1_subpel_buffs[ps_me_ctxt->as_mb_part[PRED_L0].i4_srch_pos_idx] =
                                ps_proc->apu1_subpel_buffs[3];


                ps_me_ctxt->u4_subpel_buf_strd = HP_BUFF_WD;

                /* half  pel search is done for both sides of full pel,
                 * hence half_x of width x height = 17x16 is created
                 * starting from left half_x of converged full pel */
                pu1_hpel_src -= 1;

                /* computing half_x */
                ps_codec->pf_ih264e_sixtapfilter_horz(pu1_hpel_src,
                                                      ps_me_ctxt->apu1_subpel_buffs[0],
                                                      i4_rec_strd,
                                                      ps_me_ctxt->u4_subpel_buf_strd);

                /*
                 * Halfpel search is done for both sides of full pel,
                 * hence half_y of width x height = 16x17 is created
                 * starting from top half_y of converged full pel
                 * for half_xy top_left is required
                 * hence it starts from pu1_hpel_src = full_pel_converged_point - i4_rec_strd - 1
                 */
                pu1_hpel_src -= i4_rec_strd;

                /* computing half_y and half_xy */
                ps_codec->pf_ih264e_sixtap_filter_2dvh_vert(
                                pu1_hpel_src, ps_me_ctxt->apu1_subpel_buffs[1],
                                ps_me_ctxt->apu1_subpel_buffs[2], i4_rec_strd,
                                ps_me_ctxt->u4_subpel_buf_strd, ps_proc->ai16_pred1 + 3,
                                ps_me_ctxt->u4_subpel_buf_strd);

                ime_sub_pel_motion_estimation_16x16(ps_me_ctxt, i4_reflist);

            }
        }

        /***********************************************************************
         * If a particular skiip Mv is giving better sad, copy to the corresponding
         * MBPART
         * In B slices this loop should go only to PREDL1: If we found min sad
         * we will go to the skip ref list only
         * Have to find a way to make it without too much change or new vars
         **********************************************************************/
        for (i4_reflist = 0; i4_reflist <= i4_max_reflist; i4_reflist++)
        {
            if (as_skip_mbpart[i4_reflist].i4_mb_cost < ps_me_ctxt->as_mb_part[i4_reflist].i4_mb_cost)
            {
                ps_me_ctxt->as_mb_part[i4_reflist].i4_mb_cost = as_skip_mbpart[i4_reflist].i4_mb_cost;
                ps_me_ctxt->as_mb_part[i4_reflist].i4_mb_distortion = as_skip_mbpart[i4_reflist].i4_mb_distortion;
                ps_me_ctxt->as_mb_part[i4_reflist].s_mv_curr = as_skip_mbpart[i4_reflist].s_mv_curr;
            }
        }

        /***********************************************************************
         * Compute ME for BI
         *  In case of BI we do ME for two candidates
         *   1) The best L0 and L1 Mvs
         *   2) Skip L0 and L1 MVs
         *
         *   TODO
         *   one of the search candidates is skip. Hence it may be duplicated
         ***********************************************************************/
        if (i4_max_reflist == PRED_L1 && ps_me_ctxt->u4_min_sad_reached == 0)
        {
            WORD32 i, j = 0;
            WORD32 l0_srch_pos_idx, l1_srch_pos_idx;
            WORD32 i4_l0_skip_mv_idx, i4_l1_skip_mv_idx;

            /* Get the free buffers */
            l0_srch_pos_idx = ps_me_ctxt->as_mb_part[PRED_L0].i4_srch_pos_idx;
            l1_srch_pos_idx = ps_me_ctxt->as_mb_part[PRED_L1].i4_srch_pos_idx;

            /* Search for the two free buffers in subpel list */
            for (i = 0; i < SUBPEL_BUFF_CNT; i++)
            {
                if (i != l0_srch_pos_idx && i != l1_srch_pos_idx)
                {
                    ps_me_ctxt->apu1_subpel_buffs[j] = ps_proc->apu1_subpel_buffs[i];
                    j++;
                }
            }
            ps_me_ctxt->u4_subpel_buf_strd = HP_BUFF_WD;

            /* Copy the statial SKIP MV of each list */
            i4_l0_skip_mv_idx = ps_me_ctxt->u4_num_candidates[PRED_L0] - 2;
            i4_l1_skip_mv_idx = ps_me_ctxt->u4_num_candidates[PRED_L1] - 2;
            ps_me_ctxt->as_mv_init_search[PRED_BI][0].i2_mvx = ps_me_ctxt->as_mv_init_search[PRED_L0][i4_l0_skip_mv_idx].i2_mvx << 2;
            ps_me_ctxt->as_mv_init_search[PRED_BI][0].i2_mvy = ps_me_ctxt->as_mv_init_search[PRED_L0][i4_l0_skip_mv_idx].i2_mvy << 2;
            ps_me_ctxt->as_mv_init_search[PRED_BI][1].i2_mvx = ps_me_ctxt->as_mv_init_search[PRED_L1][i4_l1_skip_mv_idx].i2_mvx << 2;
            ps_me_ctxt->as_mv_init_search[PRED_BI][1].i2_mvy = ps_me_ctxt->as_mv_init_search[PRED_L1][i4_l1_skip_mv_idx].i2_mvy << 2;

            /* Copy the SKIP MV temporal of each list */
            i4_l0_skip_mv_idx++;
            i4_l1_skip_mv_idx++;
            ps_me_ctxt->as_mv_init_search[PRED_BI][2].i2_mvx = ps_me_ctxt->as_mv_init_search[PRED_L0][i4_l0_skip_mv_idx].i2_mvx << 2;
            ps_me_ctxt->as_mv_init_search[PRED_BI][2].i2_mvy = ps_me_ctxt->as_mv_init_search[PRED_L0][i4_l0_skip_mv_idx].i2_mvy << 2;
            ps_me_ctxt->as_mv_init_search[PRED_BI][3].i2_mvx = ps_me_ctxt->as_mv_init_search[PRED_L1][i4_l1_skip_mv_idx].i2_mvx << 2;
            ps_me_ctxt->as_mv_init_search[PRED_BI][3].i2_mvy = ps_me_ctxt->as_mv_init_search[PRED_L1][i4_l1_skip_mv_idx].i2_mvy << 2;

            /* Copy the best MV after ME */
            ps_me_ctxt->as_mv_init_search[PRED_BI][4] = ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr;
            ps_me_ctxt->as_mv_init_search[PRED_BI][5] = ps_me_ctxt->as_mb_part[PRED_L1].s_mv_curr;

            ps_me_ctxt->u4_num_candidates[PRED_BI] = 6;

            ps_me_ctxt->as_mb_part[PRED_BI].i4_mb_cost = INT_MAX;
            ps_me_ctxt->as_mb_part[PRED_BI].i4_mb_distortion = INT_MAX;

            ih264e_evaluate_bipred(ps_me_ctxt, ps_proc,
                                   &ps_me_ctxt->as_mb_part[PRED_BI]);

            i4_max_reflist = PRED_BI;
        }

        /**********************************************************************
         * Now get the minimum of MB part sads by searching over all ref lists
         **********************************************************************/
        ps_proc->ps_pu->b2_pred_mode = 0x3;

        for (i4_reflist = 0; i4_reflist <= i4_max_reflist; i4_reflist++)
        {
            if (ps_me_ctxt->as_mb_part[i4_reflist].i4_mb_cost < ps_proc->ps_cur_mb->i4_mb_cost)
            {
                ps_proc->ps_cur_mb->i4_mb_cost = ps_me_ctxt->as_mb_part[i4_reflist].i4_mb_cost;
                ps_proc->ps_cur_mb->i4_mb_distortion = ps_me_ctxt->as_mb_part[i4_reflist].i4_mb_distortion;
                ps_proc->ps_cur_mb->u4_mb_type = (ps_proc->i4_slice_type == PSLICE) ? P16x16 : B16x16;
                ps_proc->ps_pu->b2_pred_mode = i4_reflist ;
            }
        }

        /**********************************************************************
         * In case we have a BI MB, we have to copy the buffers and set proer MV's
         *  1)In case its BI, we need to get the best MVs given by BI and update
         *    to their corresponding MB part
         *  2)We also need to copy the buffer in which bipred buff is populated
         *
         *  Not that if we have
         **********************************************************************/
        if (ps_proc->ps_pu->b2_pred_mode == PRED_BI)
        {
            WORD32 i4_srch_pos = ps_me_ctxt->as_mb_part[PRED_BI].i4_srch_pos_idx;
            UWORD8 *pu1_bi_buf = ps_me_ctxt->as_mb_part[PRED_BI].pu1_best_hpel_buf;

            ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr = ps_me_ctxt->as_mv_init_search[PRED_BI][i4_srch_pos << 1];
            ps_me_ctxt->as_mb_part[PRED_L1].s_mv_curr = ps_me_ctxt->as_mv_init_search[PRED_BI][(i4_srch_pos << 1) + 1];

            /* Now we have to copy the buffers */
            ps_codec->pf_inter_pred_luma_copy(pu1_bi_buf,
                                              ps_proc->pu1_best_subpel_buf,
                                              ps_me_ctxt->u4_subpel_buf_strd,
                                              ps_proc->u4_bst_spel_buf_strd,
                                              MB_SIZE, MB_SIZE, NULL, 0);

        }
        else if (ps_me_ctxt->as_mb_part[ps_proc->ps_pu->b2_pred_mode].pu1_best_hpel_buf)
        {
            /* Now we have to copy the buffers */
            ps_codec->pf_inter_pred_luma_copy(
                            ps_me_ctxt->as_mb_part[ps_proc->ps_pu->b2_pred_mode].pu1_best_hpel_buf,
                            ps_proc->pu1_best_subpel_buf,
                            ps_me_ctxt->u4_subpel_buf_strd,
                            ps_proc->u4_bst_spel_buf_strd, MB_SIZE, MB_SIZE,
                            NULL, 0);
        }
    }

    /**************************************************************************
     *Now copy the MVs to the current PU with qpel scaling
     ***************************************************************************/
    ps_proc->ps_pu->s_me_info[PRED_L0].s_mv.i2_mvx = (ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr.i2_mvx);
    ps_proc->ps_pu->s_me_info[PRED_L0].s_mv.i2_mvy = (ps_me_ctxt->as_mb_part[PRED_L0].s_mv_curr.i2_mvy);
    ps_proc->ps_pu->s_me_info[PRED_L1].s_mv.i2_mvx = (ps_me_ctxt->as_mb_part[PRED_L1].s_mv_curr.i2_mvx);
    ps_proc->ps_pu->s_me_info[PRED_L1].s_mv.i2_mvy = (ps_me_ctxt->as_mb_part[PRED_L1].s_mv_curr.i2_mvy);


    ps_proc->ps_pu->s_me_info[0].i1_ref_idx = (ps_proc->ps_pu->b2_pred_mode != PRED_L1)? -1:0;
    ps_proc->ps_pu->s_me_info[1].i1_ref_idx = (ps_proc->ps_pu->b2_pred_mode != PRED_L0)? -1:0;

    /* number of partitions */
    ps_proc->u4_num_sub_partitions = 1;
    *(ps_proc->pu4_mb_pu_cnt) = 1;

    /* position in-terms of PU */
    ps_proc->ps_pu->b4_pos_x = 0;
    ps_proc->ps_pu->b4_pos_y = 0;

    /* PU size */
    ps_proc->ps_pu->b4_wd = 3;
    ps_proc->ps_pu->b4_ht = 3;

    /* Update min sad conditions */
    if (ps_me_ctxt->u4_min_sad_reached == 1)
    {
        ps_proc->ps_cur_mb->u4_min_sad_reached = 1;
        ps_proc->ps_cur_mb->u4_min_sad = ps_me_ctxt->i4_min_sad;
    }
}

