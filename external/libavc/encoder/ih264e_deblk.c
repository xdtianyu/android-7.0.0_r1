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
 *  ih264e_deblk.c
 *
 * @brief
 *  This file contains functions that are associated with deblocking
 *
 * @author
 *  ittiam
 *
 * @par List of Functions:
 *  - ih264e_fill_bs_1mv_1ref_non_mbaff
 *  - ih264e_calculate_csbp
 *  - ih264e_compute_bs
 *  - ih264e_filter_top_edge
 *  - ih264e_filter_left_edge
 *  - ih264e_deblock_mb
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
#include <assert.h>

/* User include files */
#include "ih264e_config.h"
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264_macros.h"
#include "ih264_defs.h"
#include "ih264e_defs.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "ime_distortion_metrics.h"
#include "ime_defs.h"
#include "ime_structs.h"
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
#include "ih264_trans_data.h"
#include "ih264_deblk_tables.h"
#include "ih264e_deblk.h"


/*****************************************************************************/
/* Extern global definitions                                                 */
/*****************************************************************************/

/**
******************************************************************************
* @brief  BS Table Lookup
* input  :
* output :
* @remarks none
******************************************************************************
*/
static const UWORD32 gu4_bs_table[][16] =
{
    {
        0x00000000, 0x02000000, 0x00020000, 0x02020000,
        0x00000200, 0x02000200, 0x00020200, 0x02020200,
        0x00000002, 0x02000002, 0x00020002, 0x02020002,
        0x00000202, 0x02000202, 0x00020202, 0x02020202
    },
    {
        0x01010101, 0x02010101, 0x01020101, 0x02020101,
        0x01010201, 0x02010201, 0x01020201, 0x02020201,
        0x01010102, 0x02010102, 0x01020102, 0x02020102,
        0x01010202, 0x02010202, 0x01020202, 0x02020202
    }
};

/**
******************************************************************************
* @brief  Transpose Matrix used in BS
* input  :
* output :
* @remarks none
******************************************************************************
*/
static const UWORD16  ih264e_gu2_4x4_v2h_reorder[16] =
{
    0x0000, 0x0001, 0x0010, 0x0011,
    0x0100, 0x0101, 0x0110, 0x0111,
    0x1000, 0x1001, 0x1010, 0x1011,
    0x1100, 0x1101, 0x1110, 0x1111
};


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief Fill BS value for all the edges of an mb
*
* @par Description:
*  Fill BS value for all the edges of an mb
*
* @param[in] pu4_horz_bs
*  Base pointer of horizontal BS table
*
* @param[in] pu4_vert_bs
*  Base pointer of vertical BS table
*
* @param[in] u4_left_mb_csbp
*  coded sub block pattern of left mb
*
* @param[in] u4_left_mb_csbp
*  coded sub block pattern of top mb
*
* @param[in] ps_left_pu
*  PU for left MB
*
* @param[in] ps_top_pu
*  PU for top MB
*
* @param[in] ps_curr_pu
*  PU for current MB
*
*
* @returns  none
*
* @remarks  none
*
*******************************************************************************
*/
static void ih264e_fill_bs_1mv_1ref_non_mbaff(UWORD32 *pu4_horz_bs,
                                              UWORD32 *pu4_vert_bs,
                                              UWORD32 u4_left_mb_csbp,
                                              UWORD32 u4_top_mb_csbp,
                                              UWORD32 u4_cur_mb_csbp,
                                              enc_pu_t *ps_left_pu,
                                              enc_pu_t *ps_top_pu,
                                              enc_pu_t *ps_curr_pu)
{
    /* motion vectors of blks p & q */
    WORD16 i16_qMvl0_x, i16_qMvl0_y, i16_pMvl0_x, i16_pMvl0_y;
    WORD16 i16_qMvl1_x, i16_qMvl1_y, i16_pMvl1_x, i16_pMvl1_y;

    /* temp var */
    UWORD32 u4_left_flag, u4_top_flag;
    const UWORD32 *bs_map;
    UWORD32 u4_reordered_vert_bs_enc, u4_temp;

    /* Coded Pattern for Horizontal Edge */
    /*-----------------------------------------------------------------------*/
    /*u4_nbr_horz_csbp=11C|10C|9C|8C|7C|6C|5C|4C|3C|2C|1C|0C|15T|14T|13T|12T */
    /*-----------------------------------------------------------------------*/
    UWORD32 u4_nbr_horz_csbp = (u4_cur_mb_csbp << 4) | (u4_top_mb_csbp >> 12);
    UWORD32 u4_horz_bs_enc = u4_cur_mb_csbp | u4_nbr_horz_csbp;

    /* Coded Pattern for Vertical Edge */
    /*-----------------------------------------------------------------------*/
    /*u4_left_mb_masked_csbp = 15L|0|0|0|11L|0|0|0|7L|0|0|0|3L|0|0|0         */
    /*-----------------------------------------------------------------------*/
    UWORD32 u4_left_mb_masked_csbp = u4_left_mb_csbp & CSBP_RIGHT_BLOCK_MASK;

    /*-----------------------------------------------------------------------*/
    /*u4_cur_mb_masked_csbp =14C|13C|12C|x|10C|9C|8C|x|6C|5C|4C|x|2C|1C|0C|x */
    /*-----------------------------------------------------------------------*/
    UWORD32 u4_cur_mb_masked_csbp = (u4_cur_mb_csbp << 1)
                    & (~CSBP_LEFT_BLOCK_MASK);

    /*-----------------------------------------------------------------------*/
    /*u4_nbr_vert_csbp=14C|13C|12C|15L|10C|9C|8C|11L|6C|5C|4C|7L|2C|1C|0C|3L */
    /*-----------------------------------------------------------------------*/
    UWORD32 u4_nbr_vert_csbp = (u4_cur_mb_masked_csbp)
                    | (u4_left_mb_masked_csbp >> 3);
    UWORD32 u4_vert_bs_enc = u4_cur_mb_csbp | u4_nbr_vert_csbp;

    /* BS Calculation for MB Boundary Edges */

    /* BS calculation for 1 2 3 horizontal boundary */
    bs_map = gu4_bs_table[0];
    pu4_horz_bs[1] = bs_map[(u4_horz_bs_enc >> 4) & 0xF];
    pu4_horz_bs[2] = bs_map[(u4_horz_bs_enc >> 8) & 0xF];
    pu4_horz_bs[3] = bs_map[(u4_horz_bs_enc >> 12) & 0xF];

    /* BS calculation for 5 6 7 vertical boundary */
    /* Do 4x4 tranpose of u4_vert_bs_enc by using look up table for reorder */
    u4_reordered_vert_bs_enc = ih264e_gu2_4x4_v2h_reorder[u4_vert_bs_enc & 0xF];

    u4_temp = ih264e_gu2_4x4_v2h_reorder[(u4_vert_bs_enc >> 4) & 0xF];
    u4_reordered_vert_bs_enc |= (u4_temp << 1);

    u4_temp = ih264e_gu2_4x4_v2h_reorder[(u4_vert_bs_enc >> 8) & 0xF];
    u4_reordered_vert_bs_enc |= (u4_temp << 2);

    u4_temp = ih264e_gu2_4x4_v2h_reorder[(u4_vert_bs_enc >> 12) & 0xF];
    u4_reordered_vert_bs_enc |= (u4_temp << 3);

    pu4_vert_bs[1] = bs_map[(u4_reordered_vert_bs_enc >> 4) & 0xF];
    pu4_vert_bs[2] = bs_map[(u4_reordered_vert_bs_enc >> 8) & 0xF];
    pu4_vert_bs[3] = bs_map[(u4_reordered_vert_bs_enc >> 12) & 0xF];


    /* BS Calculation for MB Boundary Edges */
    if (ps_top_pu->b1_intra_flag)
    {
        pu4_horz_bs[0] = 0x04040404;
    }
    else
    {
        if (ps_curr_pu->b2_pred_mode != ps_top_pu->b2_pred_mode)
        {
            u4_top_flag = 1;
        }
        else if(ps_curr_pu->b2_pred_mode != 2)
        {
            i16_pMvl0_x = ps_top_pu->s_me_info[ps_top_pu->b2_pred_mode].s_mv.i2_mvx;
            i16_pMvl0_y = ps_top_pu->s_me_info[ps_top_pu->b2_pred_mode].s_mv.i2_mvy;

            i16_qMvl0_x = ps_curr_pu->s_me_info[ps_curr_pu->b2_pred_mode].s_mv.i2_mvx;
            i16_qMvl0_y = ps_curr_pu->s_me_info[ps_curr_pu->b2_pred_mode].s_mv.i2_mvy;


            u4_top_flag =  (ABS((i16_pMvl0_x - i16_qMvl0_x)) >= 4)
                         | (ABS((i16_pMvl0_y - i16_qMvl0_y)) >= 4);
        }
        else
        {

            i16_pMvl0_x = ps_top_pu->s_me_info[PRED_L0].s_mv.i2_mvx;
            i16_pMvl0_y = ps_top_pu->s_me_info[PRED_L0].s_mv.i2_mvy;
            i16_pMvl1_x = ps_top_pu->s_me_info[PRED_L1].s_mv.i2_mvx;
            i16_pMvl1_y = ps_top_pu->s_me_info[PRED_L1].s_mv.i2_mvy;

            i16_qMvl0_x = ps_curr_pu->s_me_info[PRED_L0].s_mv.i2_mvx;
            i16_qMvl0_y = ps_curr_pu->s_me_info[PRED_L0].s_mv.i2_mvy;
            i16_qMvl1_x = ps_curr_pu->s_me_info[PRED_L1].s_mv.i2_mvx;
            i16_qMvl1_y = ps_curr_pu->s_me_info[PRED_L1].s_mv.i2_mvy;


            u4_top_flag =  (ABS((i16_pMvl0_x - i16_qMvl0_x)) >= 4)
                         | (ABS((i16_pMvl0_y - i16_qMvl0_y)) >= 4)
                         | (ABS((i16_pMvl1_x - i16_qMvl1_x)) >= 4)
                         | (ABS((i16_pMvl1_y - i16_qMvl1_y)) >= 4);
        }

        bs_map = gu4_bs_table[!!u4_top_flag];
        pu4_horz_bs[0] = bs_map[u4_horz_bs_enc & 0xF];
    }


    if (ps_left_pu->b1_intra_flag)
    {
        pu4_vert_bs[0] = 0x04040404;
    }
    else
    {
        if (ps_curr_pu->b2_pred_mode != ps_left_pu->b2_pred_mode)
        {
            u4_left_flag = 1;
        }
        else if(ps_curr_pu->b2_pred_mode != 2)/* Not bipred */
        {
            i16_pMvl0_x = ps_left_pu->s_me_info[ps_left_pu->b2_pred_mode].s_mv.i2_mvx;
            i16_pMvl0_y = ps_left_pu->s_me_info[ps_left_pu->b2_pred_mode].s_mv.i2_mvy;

            i16_qMvl0_x = ps_curr_pu->s_me_info[ps_curr_pu->b2_pred_mode].s_mv.i2_mvx;
            i16_qMvl0_y = ps_curr_pu->s_me_info[ps_curr_pu->b2_pred_mode].s_mv.i2_mvy;


            u4_left_flag =  (ABS((i16_pMvl0_x - i16_qMvl0_x)) >= 4)
                          | (ABS((i16_pMvl0_y - i16_qMvl0_y)) >= 4);
        }
        else
        {

            i16_pMvl0_x = ps_left_pu->s_me_info[PRED_L0].s_mv.i2_mvx;
            i16_pMvl0_y = ps_left_pu->s_me_info[PRED_L0].s_mv.i2_mvy;
            i16_pMvl1_x = ps_left_pu->s_me_info[PRED_L1].s_mv.i2_mvx;
            i16_pMvl1_y = ps_left_pu->s_me_info[PRED_L1].s_mv.i2_mvy;

            i16_qMvl0_x = ps_curr_pu->s_me_info[PRED_L0].s_mv.i2_mvx;
            i16_qMvl0_y = ps_curr_pu->s_me_info[PRED_L0].s_mv.i2_mvy;
            i16_qMvl1_x = ps_curr_pu->s_me_info[PRED_L1].s_mv.i2_mvx;
            i16_qMvl1_y = ps_curr_pu->s_me_info[PRED_L1].s_mv.i2_mvy;


            u4_left_flag =  (ABS((i16_pMvl0_x - i16_qMvl0_x)) >= 4)
                          | (ABS((i16_pMvl0_y - i16_qMvl0_y)) >= 4)
                          | (ABS((i16_pMvl1_x - i16_qMvl1_x)) >= 4)
                          | (ABS((i16_pMvl1_y - i16_qMvl1_y)) >= 4);
        }

        bs_map = gu4_bs_table[!!u4_left_flag];
        pu4_vert_bs[0] = bs_map[u4_reordered_vert_bs_enc & 0xF];
    }
}

/**
*******************************************************************************
*
* @brief calculate coded subblock pattern from nnz
*
* @par Description:
*  calculate coded subblock pattern from nnz
*
* @param[in] ps_proc
*  process context
*
* @returns  csbp
*
* @remarks  none
*
*******************************************************************************
*/
static UWORD32 ih264e_calculate_csbp(process_ctxt_t *ps_proc)
{
    /* number of non zeros for each tx blk */
    UWORD8 *pu1_curr_nnz = (UWORD8 *)ps_proc->au4_nnz;

    /* csbp */
    UWORD32 u4_csbp = 0;

    /* temp var */
    WORD32  i4_i;

    pu1_curr_nnz += 1;

    /* Creating Subblock pattern for current MB */
    /* 15C|14C|13C|12C|11C|10C|9C|8C|7C|6C|5C|4C|3C|2C|1C|0C  */
    for (i4_i = 0; i4_i < 16; i4_i++ )
    {
        u4_csbp |= ((!!*(pu1_curr_nnz + i4_i))<< i4_i);
    }

    return u4_csbp;
}

/**
*******************************************************************************
*
* @brief This function computes blocking strength for an mb
*
* @par Description:
*  This function computes blocking strength for an mb
*
* @param[in] ps_proc
*  process context
*
* @returns  none
*
* @remarks
*
*******************************************************************************
*/
void ih264e_compute_bs(process_ctxt_t * ps_proc)
{
    /* deblk bs context */
    bs_ctxt_t *ps_bs = &(ps_proc->s_deblk_ctxt.s_bs_ctxt);

    /* vertical blocking strength */
    UWORD32 *pu4_pic_vert_bs;

    /* horizontal blocking strength */
    UWORD32 *pu4_pic_horz_bs;

    /* mb indices */
    WORD32 i4_mb_x, i4_mb_y;

    /* is intra */
    WORD32 i4_intra;

    /* temp var */
    WORD32 i4_wd_mbs = ps_proc->i4_wd_mbs;

    /* init indices */
    i4_mb_x = ps_bs->i4_mb_x;
    i4_mb_y = ps_bs->i4_mb_y;

    /* init pointers */
    pu4_pic_vert_bs = ps_bs->pu4_pic_vert_bs + ((i4_mb_y * i4_wd_mbs) + i4_mb_x) * 4;
    pu4_pic_horz_bs = ps_bs->pu4_pic_horz_bs + ((i4_mb_y * i4_wd_mbs) + i4_mb_x) * 4;

    /* is intra? */
    i4_intra = ps_proc->u4_is_intra;

    /* compute blocking strength */
    if (i4_intra)
    {
        pu4_pic_vert_bs[0] = 0x04040404;
        pu4_pic_vert_bs[1] = pu4_pic_vert_bs[2] = pu4_pic_vert_bs[3] = 0x03030303;

        pu4_pic_horz_bs[0] = 0x04040404;
        pu4_pic_horz_bs[1] = pu4_pic_horz_bs[2] = pu4_pic_horz_bs[3] = 0x03030303;
    }
    else
    {
        /* left mb syntax info */
        mb_info_t *ps_left_mb_syntax_ele = &ps_proc->s_left_mb_syntax_ele;

        /* top mb syntax info */
        mb_info_t *ps_top_mb_syntax_ele = ps_proc->ps_top_row_mb_syntax_ele + i4_mb_x;

        /* top row motion vector info */
        enc_pu_t *ps_top_row_pu = ps_proc->ps_top_row_pu + i4_mb_x;

        /* csbp for curr mb */
        ps_proc->u4_csbp = ih264e_calculate_csbp(ps_proc);

        /* csbp for ngbrs */
        if (i4_mb_x == 0)
        {
            ps_left_mb_syntax_ele->u4_csbp = 0;
            ps_proc->s_left_mb_pu.b1_intra_flag = 0;
            ps_proc->s_left_mb_pu.b2_pred_mode = ps_proc->ps_pu->b2_pred_mode;
            ps_proc->s_left_mb_pu.s_me_info[0].s_mv = ps_proc->ps_pu->s_me_info[0].s_mv;
            ps_proc->s_left_mb_pu.s_me_info[1].s_mv = ps_proc->ps_pu->s_me_info[1].s_mv;
        }
        if (i4_mb_y == 0)
        {
            ps_top_mb_syntax_ele->u4_csbp = 0;
            ps_top_row_pu->b1_intra_flag = 0;
            ps_top_row_pu->b2_pred_mode = ps_proc->ps_pu->b2_pred_mode;
            ps_top_row_pu->s_me_info[0].s_mv = ps_proc->ps_pu->s_me_info[0].s_mv;
            ps_top_row_pu->s_me_info[1].s_mv = ps_proc->ps_pu->s_me_info[1].s_mv;
        }

        ih264e_fill_bs_1mv_1ref_non_mbaff(pu4_pic_horz_bs,
                                          pu4_pic_vert_bs,
                                          ps_left_mb_syntax_ele->u4_csbp,
                                          ps_top_mb_syntax_ele->u4_csbp,
                                          ps_proc->u4_csbp,
                                          &ps_proc->s_left_mb_pu,
                                          ps_top_row_pu,
                                          ps_proc->ps_pu);
    }

    return ;
}

/**
*******************************************************************************
*
* @brief This function performs deblocking of top horizontal edge
*
* @par Description:
*  This function performs deblocking of top horizontal edge
*
* @param[in] ps_codec
*  pointer to codec context
*
* @param[in] ps_proc
*  pointer to proc context
*
* @param[in] pu1_mb_qp
*  pointer to mb quantization param
*
* @param[in] pu1_cur_pic_luma
*  pointer to recon buffer luma
*
* @param[in] pu1_cur_pic_chroma
*  pointer to recon buffer chroma
*
* @param[in] pu4_pic_horz_bs
*  pointer to horizontal blocking strength
*
* @returns  none
*
* @remarks none
*
*******************************************************************************
*/
static void ih264e_filter_top_edge(codec_t *ps_codec,
                                   process_ctxt_t *ps_proc,
                                   UWORD8 *pu1_mb_qp,
                                   UWORD8 *pu1_cur_pic_luma,
                                   UWORD8 *pu1_cur_pic_chroma,
                                   UWORD32 *pu4_pic_horz_bs)
{
    /* strd */
    WORD32 i4_rec_strd = ps_proc->i4_rec_strd;

    /* deblk params */
    UWORD32 u4_alpha_luma, u4_beta_luma, u4_qp_luma, u4_idx_A_luma, u4_idx_B_luma, u4_qp_p, u4_qp_q;
    UWORD32 u4_alpha_chroma, u4_beta_chroma, u4_qp_chroma, u4_idx_A_chroma, u4_idx_B_chroma;

    /* collect qp of left & top mb */
    u4_qp_p = pu1_mb_qp[-ps_proc->i4_wd_mbs];
    u4_qp_q = pu1_mb_qp[0];

    /********/
    /* luma */
    /********/
    u4_qp_luma = (u4_qp_p + u4_qp_q + 1) >> 1;

    /* filter offset A and filter offset B have to be received from slice header */
    /* TODO : for now lets set these offsets as zero */


    u4_idx_A_luma = MIN(51, u4_qp_luma + 0);
    u4_idx_B_luma = MIN(51, u4_qp_luma + 0);

    /* alpha, beta computation */
    u4_alpha_luma = gu1_ih264_alpha_table[u4_idx_A_luma];
    u4_beta_luma = gu1_ih264_beta_table[u4_idx_B_luma];

    /**********/
    /* chroma */
    /**********/
    u4_qp_chroma = (gu1_qpc_fqpi[u4_qp_p] + gu1_qpc_fqpi[u4_qp_q] + 1) >> 1;

    /* filter offset A and filter offset B have to be received from slice header */
    /* TODO : for now lets set these offsets as zero */


    u4_idx_A_chroma = MIN(51, u4_qp_chroma + 0);
    u4_idx_B_chroma = MIN(51, u4_qp_chroma + 0);

    /* alpha, beta computation */
    u4_alpha_chroma = gu1_ih264_alpha_table[u4_idx_A_chroma];
    u4_beta_chroma = gu1_ih264_beta_table[u4_idx_B_chroma];

    /* deblk edge */
    /* top Horizontal edge - allowed to be deblocked ? */
    if (pu4_pic_horz_bs[0] == 0x04040404)
    {
        /* strong filter */
        ps_codec->pf_deblk_luma_horz_bs4(pu1_cur_pic_luma, i4_rec_strd, u4_alpha_luma, u4_beta_luma);
        ps_codec->pf_deblk_chroma_horz_bs4(pu1_cur_pic_chroma, i4_rec_strd, u4_alpha_chroma, u4_beta_chroma, u4_alpha_chroma, u4_beta_chroma);
    }
    else
    {
        /* normal filter */
        ps_codec->pf_deblk_luma_horz_bslt4(pu1_cur_pic_luma, i4_rec_strd, u4_alpha_luma,
                                               u4_beta_luma, pu4_pic_horz_bs[0],
                                               gu1_ih264_clip_table[u4_idx_A_luma]);

        ps_codec->pf_deblk_chroma_horz_bslt4(pu1_cur_pic_chroma, i4_rec_strd, u4_alpha_chroma,
                                             u4_beta_chroma, u4_alpha_chroma, u4_beta_chroma, pu4_pic_horz_bs[0],
                                             gu1_ih264_clip_table[u4_idx_A_chroma], gu1_ih264_clip_table[u4_idx_A_chroma]);
    }
}

/**
*******************************************************************************
*
* @brief This function performs deblocking of left vertical edge
*
* @par Description:
*  This function performs deblocking of top horizontal edge
*
* @param[in] ps_codec
*  pointer to codec context
*
* @param[in] ps_proc
*  pointer to proc context
*
* @param[in] pu1_mb_qp
*  pointer to mb quantization param
*
* @param[in] pu1_cur_pic_luma
*  pointer to recon buffer luma
*
* @param[in] pu1_cur_pic_chroma
*  pointer to recon buffer chroma
*
* @param[in] pu4_pic_vert_bs
*  pointer to vertical blocking strength
*
* @returns  none
*
* @remarks none
*
*******************************************************************************
*/
static void ih264e_filter_left_edge(codec_t *ps_codec,
                                    process_ctxt_t *ps_proc,
                                    UWORD8 *pu1_mb_qp,
                                    UWORD8 *pu1_cur_pic_luma,
                                    UWORD8 *pu1_cur_pic_chroma,
                                    UWORD32 *pu4_pic_vert_bs)
{
    /* strd */
    WORD32 i4_rec_strd = ps_proc->i4_rec_strd;

    /* deblk params */
    UWORD32 u4_alpha_luma, u4_beta_luma, u4_qp_luma, u4_idx_A_luma, u4_idx_B_luma, u4_qp_p, u4_qp_q;
    UWORD32 u4_alpha_chroma, u4_beta_chroma, u4_qp_chroma, u4_idx_A_chroma, u4_idx_B_chroma;

    /* collect qp of left & curr mb */
    u4_qp_p = pu1_mb_qp[-1];
    u4_qp_q = pu1_mb_qp[0];

    /********/
    /* luma */
    /********/
    u4_qp_luma = (u4_qp_p + u4_qp_q + 1) >> 1;

    /* filter offset A and filter offset B have to be received from slice header */
    /* TODO : for now lets set these offsets as zero */


    u4_idx_A_luma = MIN(51, u4_qp_luma + 0);
    u4_idx_B_luma = MIN(51, u4_qp_luma + 0);

    /* alpha, beta computation */
    u4_alpha_luma = gu1_ih264_alpha_table[u4_idx_A_luma];
    u4_beta_luma = gu1_ih264_beta_table[u4_idx_B_luma];

    /**********/
    /* chroma */
    /**********/
    u4_qp_chroma = (gu1_qpc_fqpi[u4_qp_p] + gu1_qpc_fqpi[u4_qp_q] + 1) >> 1;

    /* filter offset A and filter offset B have to be received from slice header */
    /* TODO : for now lets set these offsets as zero */


    u4_idx_A_chroma = MIN(51, u4_qp_chroma + 0);
    u4_idx_B_chroma = MIN(51, u4_qp_chroma + 0);

    /* alpha, beta computation */
    u4_alpha_chroma = gu1_ih264_alpha_table[u4_idx_A_chroma];
    u4_beta_chroma = gu1_ih264_beta_table[u4_idx_B_chroma];

    /* deblk edge */
    if (pu4_pic_vert_bs[0] == 0x04040404)
    {
        /* strong filter */
        ps_codec->pf_deblk_luma_vert_bs4(pu1_cur_pic_luma, i4_rec_strd, u4_alpha_luma, u4_beta_luma);
        ps_codec->pf_deblk_chroma_vert_bs4(pu1_cur_pic_chroma, i4_rec_strd, u4_alpha_chroma, u4_beta_chroma, u4_alpha_chroma, u4_beta_chroma);
    }
    else
    {
        /* normal filter */
        ps_codec->pf_deblk_luma_vert_bslt4(pu1_cur_pic_luma, i4_rec_strd,
                                           u4_alpha_luma, u4_beta_luma,
                                           pu4_pic_vert_bs[0],
                                           gu1_ih264_clip_table[u4_idx_A_luma]);

        ps_codec->pf_deblk_chroma_vert_bslt4(pu1_cur_pic_chroma, i4_rec_strd, u4_alpha_chroma,
                                             u4_beta_chroma, u4_alpha_chroma, u4_beta_chroma, pu4_pic_vert_bs[0],
                                             gu1_ih264_clip_table[u4_idx_A_chroma], gu1_ih264_clip_table[u4_idx_A_chroma]);
    }
}

/**
*******************************************************************************
*
* @brief This function performs deblocking on an mb
*
* @par Description:
*  This function performs deblocking on an mb
*
* @param[in] ps_proc
*  process context corresponding to the job
*
* @param[in] ps_deblk
*  pointer to deblock context
*
* @returns  none
*
* @remarks none
*
*******************************************************************************
*/
void ih264e_deblock_mb(process_ctxt_t *ps_proc, deblk_ctxt_t * ps_deblk)
{
    /* codec ctxt */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* ngbr availability */
    UWORD8  u1_mb_a, u1_mb_b;

    /* mb indices */
    WORD32  i4_mb_x = ps_deblk->i4_mb_x, i4_mb_y = ps_deblk->i4_mb_y;

    /* pic qp ptr */
    UWORD8  *pu1_pic_qp = ps_deblk->s_bs_ctxt.pu1_pic_qp;

    /* vertical blocking strength */
    UWORD32 *pu4_pic_vert_bs = ps_deblk->s_bs_ctxt.pu4_pic_vert_bs;

    /* horizontal blocking strength */
    UWORD32 *pu4_pic_horz_bs = ps_deblk->s_bs_ctxt.pu4_pic_horz_bs;

    /* src buffers luma */
    UWORD8  *pu1_cur_pic_luma = ps_deblk->pu1_cur_pic_luma;

    /* src buffers chroma */
    UWORD8  *pu1_cur_pic_chroma = ps_deblk->pu1_cur_pic_chroma;

    /* strd */
    WORD32 i4_rec_strd = ps_proc->i4_rec_strd;

    /* deblk params */
    UWORD32 u4_alpha_luma, u4_beta_luma, u4_qp_luma, u4_idx_A_luma, u4_idx_B_luma;
    UWORD32 u4_alpha_chroma, u4_beta_chroma, u4_qp_chroma, u4_idx_A_chroma, u4_idx_B_chroma;

    /* temp var */
    UWORD32 push_ptr = (i4_mb_y * ps_proc->i4_wd_mbs) + i4_mb_x;

    /* derive neighbor availability */
    /* In slice mode the edges of mbs that lie on the slice boundary are not deblocked */
    /* deblocking filter idc '2' */
    if (ps_codec->s_cfg.e_slice_mode != IVE_SLICE_MODE_NONE)
    {
        /* slice index */
        UWORD8  *pu1_slice_idx = ps_deblk->pu1_slice_idx;

        pu1_slice_idx += (i4_mb_y * ps_proc->i4_wd_mbs);
        /* left macroblock availability */
        u1_mb_a = (i4_mb_x == 0 ||
                        (pu1_slice_idx[i4_mb_x - 1 ] != pu1_slice_idx[i4_mb_x]))? 0 : 1;
        /* top macroblock availability */
        u1_mb_b = (i4_mb_y == 0 ||
                        (pu1_slice_idx[i4_mb_x-ps_proc->i4_wd_mbs] != pu1_slice_idx[i4_mb_x]))? 0 : 1;
    }
    else
    {
        /* left macroblock availability */
        u1_mb_a = (i4_mb_x == 0)? 0 : 1;
        /* top macroblock availability */
        u1_mb_b = (i4_mb_y == 0)? 0 : 1;
    }

    pu1_pic_qp += push_ptr;
    pu4_pic_vert_bs += push_ptr * 4;
    pu4_pic_horz_bs += push_ptr * 4;

    /********/
    /* luma */
    /********/
    u4_qp_luma = pu1_pic_qp[0];

    /* filter offset A and filter offset B have to be received from slice header */
    /* TODO : for now lets set these offsets as zero */


    u4_idx_A_luma = MIN(51, u4_qp_luma + 0);
    u4_idx_B_luma = MIN(51, u4_qp_luma + 0);

    /* alpha, beta computation */
    u4_alpha_luma = gu1_ih264_alpha_table[u4_idx_A_luma];
    u4_beta_luma = gu1_ih264_beta_table[u4_idx_B_luma];

    /**********/
    /* chroma */
    /**********/
    u4_qp_chroma = gu1_qpc_fqpi[u4_qp_luma];

    /* filter offset A and filter offset B have to be received from slice header */
    /* TODO : for now lets set these offsets as zero */


    u4_idx_A_chroma = MIN(51, u4_qp_chroma + 0);
    u4_idx_B_chroma = MIN(51, u4_qp_chroma + 0);

    /* alpha, beta computation */
    u4_alpha_chroma = gu1_ih264_alpha_table[u4_idx_A_chroma];
    u4_beta_chroma = gu1_ih264_beta_table[u4_idx_B_chroma];

    /* Deblock vertical edges */
    /* left vertical edge 0 - allowed to be deblocked ? */
    if (u1_mb_a)
    {
        ih264e_filter_left_edge(ps_codec, ps_proc, pu1_pic_qp, pu1_cur_pic_luma, pu1_cur_pic_chroma, pu4_pic_vert_bs);
    }

    /* vertical edge 1 */
    if (pu4_pic_vert_bs[1] == 0x04040404)
    {
        /* strong filter */
        ps_codec->pf_deblk_luma_vert_bs4(pu1_cur_pic_luma + 4, i4_rec_strd, u4_alpha_luma, u4_beta_luma);
    }
    else
    {
        /* normal filter */
        ps_codec->pf_deblk_luma_vert_bslt4(pu1_cur_pic_luma + 4, i4_rec_strd,
                                           u4_alpha_luma, u4_beta_luma,
                                           pu4_pic_vert_bs[1],
                                           gu1_ih264_clip_table[u4_idx_A_luma]);
    }

    /* vertical edge 2 */
    if (pu4_pic_vert_bs[2] == 0x04040404)
    {
        /* strong filter */
        ps_codec->pf_deblk_luma_vert_bs4(pu1_cur_pic_luma + 8, i4_rec_strd, u4_alpha_luma, u4_beta_luma);
        ps_codec->pf_deblk_chroma_vert_bs4(pu1_cur_pic_chroma + 8, i4_rec_strd, u4_alpha_chroma, u4_beta_chroma, u4_alpha_chroma, u4_beta_chroma);
    }
    else
    {
        /* normal filter */
        ps_codec->pf_deblk_luma_vert_bslt4(pu1_cur_pic_luma + 8, i4_rec_strd, u4_alpha_luma,
                                           u4_beta_luma, pu4_pic_vert_bs[2],
                                           gu1_ih264_clip_table[u4_idx_A_luma]);

        ps_codec->pf_deblk_chroma_vert_bslt4(pu1_cur_pic_chroma + 8, i4_rec_strd, u4_alpha_chroma,
                                             u4_beta_chroma, u4_alpha_chroma, u4_beta_chroma, pu4_pic_vert_bs[2],
                                             gu1_ih264_clip_table[u4_idx_A_chroma], gu1_ih264_clip_table[u4_idx_A_chroma]);
    }

    /* vertical edge 3 */
    if (pu4_pic_vert_bs[3] == 0x04040404)
    {
        /* strong filter */
        ps_codec->pf_deblk_luma_vert_bs4(pu1_cur_pic_luma + 12, i4_rec_strd, u4_alpha_luma, u4_beta_luma);
    }
    else
    {
        /* normal filter */
        ps_codec->pf_deblk_luma_vert_bslt4(pu1_cur_pic_luma + 12, i4_rec_strd, u4_alpha_luma,
                                           u4_beta_luma, pu4_pic_vert_bs[3],
                                           gu1_ih264_clip_table[u4_idx_A_luma]);
    }

    /* Deblock Horizontal edges */
    /* Horizontal edge 0 */
    if (u1_mb_b)
    {
        ih264e_filter_top_edge(ps_codec, ps_proc, pu1_pic_qp, pu1_cur_pic_luma, pu1_cur_pic_chroma, pu4_pic_horz_bs);
    }

    /* horizontal edge 1 */
    if (pu4_pic_horz_bs[1] == 0x04040404)
    {
        /* strong filter */
        ps_codec->pf_deblk_luma_horz_bs4(pu1_cur_pic_luma + 4 * i4_rec_strd, i4_rec_strd, u4_alpha_luma, u4_beta_luma);
    }
    else
    {
        /* normal filter */
        ps_codec->pf_deblk_luma_horz_bslt4(pu1_cur_pic_luma + 4 * i4_rec_strd, i4_rec_strd, u4_alpha_luma,
                                           u4_beta_luma, pu4_pic_horz_bs[1],
                                           gu1_ih264_clip_table[u4_idx_A_luma]);
    }

    /* horizontal edge 2 */
    if (pu4_pic_horz_bs[2] == 0x04040404)
    {
        /* strong filter */
        ps_codec->pf_deblk_luma_horz_bs4(pu1_cur_pic_luma + 8 * i4_rec_strd, i4_rec_strd, u4_alpha_luma, u4_beta_luma);
        ps_codec->pf_deblk_chroma_horz_bs4(pu1_cur_pic_chroma + 4 * i4_rec_strd, i4_rec_strd, u4_alpha_chroma, u4_beta_chroma, u4_alpha_chroma, u4_beta_chroma);
    }
    else
    {
        /* normal filter */
        ps_codec->pf_deblk_luma_horz_bslt4(pu1_cur_pic_luma + 8 * i4_rec_strd, i4_rec_strd, u4_alpha_luma,
                                           u4_beta_luma, pu4_pic_horz_bs[2],
                                           gu1_ih264_clip_table[u4_idx_A_luma]);

        ps_codec->pf_deblk_chroma_horz_bslt4(pu1_cur_pic_chroma + 4 * i4_rec_strd, i4_rec_strd, u4_alpha_chroma,
                                             u4_beta_chroma, u4_alpha_chroma, u4_beta_chroma, pu4_pic_horz_bs[2],
                                             gu1_ih264_clip_table[u4_idx_A_chroma], gu1_ih264_clip_table[u4_idx_A_chroma]);
    }

    /* horizontal edge 3 */
    if (pu4_pic_horz_bs[3] == 0x04040404)
    {
        /* strong filter */
        ps_codec->pf_deblk_luma_horz_bs4(pu1_cur_pic_luma + 12 * i4_rec_strd, i4_rec_strd, u4_alpha_luma, u4_beta_luma);
    }
    else
    {
        /* normal filter */
        ps_codec->pf_deblk_luma_horz_bslt4(pu1_cur_pic_luma + 12 * i4_rec_strd, i4_rec_strd, u4_alpha_luma,
                                           u4_beta_luma, pu4_pic_horz_bs[3],
                                           gu1_ih264_clip_table[u4_idx_A_luma]);
    }

    return ;
}
