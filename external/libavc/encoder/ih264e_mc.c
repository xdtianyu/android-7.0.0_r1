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
 *  ih264e_mc.c
 *
 * @brief
 *  Contains definition of functions for motion compensation
 *
 * @author
 *  ittiam
 *
 * @par List of Functions:
 *  - ih264e_motion_comp_luma()
 *  - ih264e_motion_comp_chroma()
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

/* User include files */
#include "ih264_typedefs.h"
#include "ih264_defs.h"
#include "iv2.h"
#include "ive2.h"
#include "ime_distortion_metrics.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "ih264_structs.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_cabac_tables.h"
#include "ih264e_defs.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_mc.h"
#include "ih264e_half_pel.h"

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 ******************************************************************************
 *
 * @brief
 *  performs motion compensation for a luma mb for the given mv.
 *
 * @par Description
 *  This routine performs motion compensation of an inter mb. When the inter
 *  mb mode is P16x16, there is no need to copy 16x16 unit from reference buffer
 *  to pred buffer. In this case the function returns pointer and stride of the
 *  ref. buffer and this info is used in place of pred buffer else where.
 *  In other cases, the pred buffer is populated via copy / filtering + copy
 *  (q pel cases) and returned.
 *
 * @param[in] ps_proc
 *  pointer to current proc ctxt
 *
 * @param[out] pu1_pseudo_pred
 *  pseudo prediction buffer
 *
 * @param[out] u4_pseudo_pred_strd
 *  pseudo pred buffer stride
 *
 * @return  none
 *
 * @remarks Assumes half pel buffers for the entire frame are populated.
 *
 ******************************************************************************
 */
void ih264e_motion_comp_luma(process_ctxt_t *ps_proc, UWORD8 **pu1_pseudo_pred,
                             WORD32 *pi4_pseudo_pred_strd)
{
    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* me ctxt */
    me_ctxt_t *ps_me_ctxt = &ps_proc->s_me_ctxt;

    /* Pointer to the structure having motion vectors, size and position of curr partitions */
    enc_pu_t *ps_curr_pu;

    /* pointers to full pel, half pel x, half pel y, half pel xy reference buffer */
    UWORD8 *pu1_ref[4];

    /* pred buffer ptr */
    UWORD8 *pu1_pred;

    /* strides of full pel, half pel x, half pel y, half pel xy reference buffer */
    WORD32 i4_ref_strd[4];

    /* pred buffer stride */
    WORD32 i4_pred_strd = ps_proc->i4_pred_strd;

    /* full pel motion vectors */
    WORD32 u4_mv_x_full, u4_mv_y_full;

    /* half pel motion vectors */
    WORD32 u4_mv_x_hpel, u4_mv_y_hpel;

    /* quarter pel motion vectors */
    WORD32 u4_mv_x_qpel, u4_mv_y_qpel;

    /* width & height of the partition */
    UWORD32 wd, ht;

    /* partition idx */
    UWORD32 u4_num_prtn;

    /* half / qpel coefficient */
    UWORD32 u4_subpel_factor;

    /* BIPRED Flag */
    WORD32 i4_bipred_flag;

    /* temp var */
    UWORD32 u4_lkup_idx1;

    /* Init */
    i4_ref_strd[0] = ps_proc->i4_rec_strd;

    i4_ref_strd[1] = i4_ref_strd[2] = i4_ref_strd[3] =
                    ps_me_ctxt->u4_subpel_buf_strd;

    for (u4_num_prtn = 0; u4_num_prtn < ps_proc->u4_num_sub_partitions;
                    u4_num_prtn++)
    {
        mv_t *ps_curr_mv;

        /* update ptr to curr partition */
        ps_curr_pu = ps_proc->ps_pu + u4_num_prtn;

        /* Set no no bipred */
        i4_bipred_flag = 0;

        switch (ps_curr_pu->b2_pred_mode)
        {
            case PRED_L0:
                ps_curr_mv = &ps_curr_pu->s_me_info[0].s_mv;
                pu1_ref[0] = ps_proc->apu1_ref_buf_luma[0];
                break;

            case PRED_L1:
                ps_curr_mv = &ps_curr_pu->s_me_info[1].s_mv;
                pu1_ref[0] = ps_proc->apu1_ref_buf_luma[1];
                break;

            case PRED_BI:
                /*
                 * In case of PRED_BI, we only need to ensure that
                 * the reference buffer that gets selected is
                 * ps_proc->pu1_best_subpel_buf
                 */

                /* Dummy */
                ps_curr_mv = &ps_curr_pu->s_me_info[0].s_mv;
                pu1_ref[0] = ps_proc->apu1_ref_buf_luma[0];

                i4_bipred_flag = 1;
                break;

            default:
                ps_curr_mv = &ps_curr_pu->s_me_info[0].s_mv;
                pu1_ref[0] = ps_proc->apu1_ref_buf_luma[0];
                break;

        }

        /* get full pel mv's (full pel units) */
        u4_mv_x_full = ps_curr_mv->i2_mvx >> 2;
        u4_mv_y_full = ps_curr_mv->i2_mvy >> 2;

        /* get half pel mv's */
        u4_mv_x_hpel = (ps_curr_mv->i2_mvx & 0x2) >> 1;
        u4_mv_y_hpel = (ps_curr_mv->i2_mvy & 0x2) >> 1;

        /* get quarter pel mv's */
        u4_mv_x_qpel = (ps_curr_mv->i2_mvx & 0x1);
        u4_mv_y_qpel = (ps_curr_mv->i2_mvy & 0x1);

        /* width and height of partition */
        wd = (ps_curr_pu->b4_wd + 1) << 2;
        ht = (ps_curr_pu->b4_ht + 1) << 2;

        /* decision ? qpel/hpel, fpel */
        u4_subpel_factor = (u4_mv_y_hpel << 3) + (u4_mv_x_hpel << 2)
                        + (u4_mv_y_qpel << 1) + (u4_mv_x_qpel);

        /* Move ref to position given by MV */
        pu1_ref[0] += ((u4_mv_y_full * i4_ref_strd[0]) + u4_mv_x_full);

        /* Sub pel ptrs/ Biperd pointers init */
        pu1_ref[1] = ps_proc->pu1_best_subpel_buf;
        i4_ref_strd[1] = ps_proc->u4_bst_spel_buf_strd;

        /* update pred buff ptr */
        pu1_pred = ps_proc->pu1_pred_mb
                        + 4 * ps_curr_pu->b4_pos_y * i4_pred_strd
                        + 4 * ps_curr_pu->b4_pos_x;

        /* u4_lkup_idx1 will be non zero for half pel and bipred */
        u4_lkup_idx1 = ((u4_subpel_factor >> 2) != 0) || i4_bipred_flag;

        {
            /********************************************************************/
            /* if the block is P16x16 MB and mv are not quarter pel motion      */
            /* vectors, there is no need to copy 16x16 unit from reference frame*/
            /* to pred buffer. We might as well send the reference frame buffer */
            /* pointer as pred buffer (ofc with updated stride) to fwd transform*/
            /* and inverse transform unit.                                      */
            /********************************************************************/
            if (ps_proc->u4_num_sub_partitions == 1)
            {
                *pu1_pseudo_pred = pu1_ref[u4_lkup_idx1];
                *pi4_pseudo_pred_strd = i4_ref_strd[u4_lkup_idx1];

            }
            /*
             * Copying half pel or full pel to prediction buffer
             * Currently ps_proc->u4_num_sub_partitions will always be 1 as we only support 16x16 in P mbs
             */
            else
            {
                ps_codec->pf_inter_pred_luma_copy(pu1_ref[u4_lkup_idx1],
                                                  pu1_pred,
                                                  i4_ref_strd[u4_lkup_idx1],
                                                  i4_pred_strd, ht, wd, NULL,
                                                  0);
            }

        }
    }
}

/**
 ******************************************************************************
 *
 * @brief
 *  performs motion compensation for chroma mb
 *
 * @par   Description
 *  Copies a MB of data from the reference buffer (Full pel, half pel or q pel)
 *  according to the motion vectors given
 *
 * @param[in] ps_proc
 *  pointer to current proc ctxt
 *
 * @return  none
 *
 * @remarks Assumes half pel and quarter pel buffers for the entire frame are
 *  populated.
 ******************************************************************************
 */
void ih264e_motion_comp_chroma(process_ctxt_t *ps_proc)
{
    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* Pointer to the structure having motion vectors, size and position of curr partitions */
    enc_pu_t *ps_curr_pu;

    /* pointers to full pel, half pel x, half pel y, half pel xy reference buffer */
    UWORD8 *pu1_ref;

    /* pred buffer ptr */
    UWORD8 *pu1_pred;

    /* strides of full pel reference buffer */
    WORD32 i4_ref_strd = ps_proc->i4_rec_strd;

    /* pred buffer stride */
    WORD32 i4_pred_strd = ps_proc->i4_pred_strd;

    /* full pel motion vectors */
    WORD32 u4_mv_x_full, u4_mv_y_full;

    /* half pel motion vectors */
    WORD32 u4_mv_x_hpel, u4_mv_y_hpel;

    /* quarter pel motion vectors */
    WORD32 u4_mv_x_qpel, u4_mv_y_qpel;

    /* width & height of the partition */
    UWORD32 wd, ht;

    /* partition idx */
    UWORD32 u4_num_prtn;

    WORD32 u4_mv_x;
    WORD32 u4_mv_y;
    UWORD8 u1_dx, u1_dy;

    for (u4_num_prtn = 0; u4_num_prtn < ps_proc->u4_num_sub_partitions;
                    u4_num_prtn++)
    {
        mv_t *ps_curr_mv;

        ps_curr_pu = ps_proc->ps_pu + u4_num_prtn;

        if (ps_curr_pu->b2_pred_mode != PRED_BI)
        {
            ps_curr_mv = &ps_curr_pu->s_me_info[ps_curr_pu->b2_pred_mode].s_mv;
            pu1_ref = ps_proc->apu1_ref_buf_chroma[ps_curr_pu->b2_pred_mode];

            u4_mv_x = ps_curr_mv->i2_mvx >> 3;
            u4_mv_y = ps_curr_mv->i2_mvy >> 3;

            /*  corresponds to full pel motion vector in luma, but in chroma corresponds to pel formed wiith dx, dy =4 */
            u4_mv_x_full = (ps_curr_mv->i2_mvx & 0x4) >> 2;
            u4_mv_y_full = (ps_curr_mv->i2_mvy & 0x4) >> 2;

            /* get half pel mv's */
            u4_mv_x_hpel = (ps_curr_mv->i2_mvx & 0x2) >> 1;
            u4_mv_y_hpel = (ps_curr_mv->i2_mvy & 0x2) >> 1;

            /* get quarter pel mv's */
            u4_mv_x_qpel = (ps_curr_mv->i2_mvx & 0x1);
            u4_mv_y_qpel = (ps_curr_mv->i2_mvy & 0x1);

            /* width and height of sub macro block */
            wd = (ps_curr_pu->b4_wd + 1) << 1;
            ht = (ps_curr_pu->b4_ht + 1) << 1;

            /* move the pointers so that they point to the motion compensated locations */
            pu1_ref += ((u4_mv_y * i4_ref_strd) + (u4_mv_x << 1));

            pu1_pred = ps_proc->pu1_pred_mb
                            + 4 * ps_curr_pu->b4_pos_y * i4_pred_strd
                            + 2 * ps_curr_pu->b4_pos_x;

            u1_dx = (u4_mv_x_full << 2) + (u4_mv_x_hpel << 1) + (u4_mv_x_qpel);
            u1_dy = (u4_mv_y_full << 2) + (u4_mv_y_hpel << 1) + (u4_mv_y_qpel);

            /* cases where u1_dx = 0 or u1_dy = 0 are dealt separately in neon with
             * separate functions for better performance
             *
             * ih264_inter_pred_chroma_dx_zero_a9q
             * and
             * ih264_inter_pred_chroma_dy_zero_a9q
             */

            ps_codec->pf_inter_pred_chroma(pu1_ref, pu1_pred, i4_ref_strd,
                                           i4_pred_strd, u1_dx, u1_dy, ht, wd);
        }
        else /* If the pred mode is PRED_BI */
        {
            /*
             * We need to interpolate the L0 and L1 ref pics with the chorma MV
             * then use them to average for bilinrar interpred
             */
            WORD32 i4_predmode;
            UWORD8 *pu1_ref_buf[2];

            /* Temporary buffers to store the interpolated value from L0 and L1 */
            pu1_ref_buf[PRED_L0] = ps_proc->apu1_subpel_buffs[0];
            pu1_ref_buf[PRED_L1] = ps_proc->apu1_subpel_buffs[1];


            for (i4_predmode = 0; i4_predmode < PRED_BI; i4_predmode++)
            {
                ps_curr_mv = &ps_curr_pu->s_me_info[i4_predmode].s_mv;
                pu1_ref = ps_proc->apu1_ref_buf_chroma[i4_predmode];

                u4_mv_x = ps_curr_mv->i2_mvx >> 3;
                u4_mv_y = ps_curr_mv->i2_mvy >> 3;

                /*
                 * corresponds to full pel motion vector in luma, but in chroma
                 * corresponds to pel formed wiith dx, dy =4
                 */
                u4_mv_x_full = (ps_curr_mv->i2_mvx & 0x4) >> 2;
                u4_mv_y_full = (ps_curr_mv->i2_mvy & 0x4) >> 2;

                /* get half pel mv's */
                u4_mv_x_hpel = (ps_curr_mv->i2_mvx & 0x2) >> 1;
                u4_mv_y_hpel = (ps_curr_mv->i2_mvy & 0x2) >> 1;

                /* get quarter pel mv's */
                u4_mv_x_qpel = (ps_curr_mv->i2_mvx & 0x1);
                u4_mv_y_qpel = (ps_curr_mv->i2_mvy & 0x1);

                /* width and height of sub macro block */
                wd = (ps_curr_pu->b4_wd + 1) << 1;
                ht = (ps_curr_pu->b4_ht + 1) << 1;

                /* move the pointers so that they point to the motion compensated locations */
                pu1_ref += ((u4_mv_y * i4_ref_strd) + (u4_mv_x << 1));

                pu1_pred = ps_proc->pu1_pred_mb
                                + 4 * ps_curr_pu->b4_pos_y * i4_pred_strd
                                + 2 * ps_curr_pu->b4_pos_x;

                u1_dx = (u4_mv_x_full << 2) + (u4_mv_x_hpel << 1)
                                + (u4_mv_x_qpel);
                u1_dy = (u4_mv_y_full << 2) + (u4_mv_y_hpel << 1)
                                + (u4_mv_y_qpel);

                ps_codec->pf_inter_pred_chroma(pu1_ref,
                                               pu1_ref_buf[i4_predmode],
                                               i4_ref_strd, MB_SIZE, u1_dx,
                                               u1_dy, ht, wd);
            }

            ps_codec->pf_inter_pred_luma_bilinear(pu1_ref_buf[PRED_L0],
                                                  pu1_ref_buf[PRED_L1], pu1_pred,
                                                  MB_SIZE, MB_SIZE,
                                                  i4_pred_strd, MB_SIZE >> 1,
                                                  MB_SIZE);
        }
    }
}
