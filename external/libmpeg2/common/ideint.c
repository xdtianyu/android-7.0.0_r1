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
*  ideint_api.c
*
* @brief
*  This file contains the definitions of the core  processing of the de-
* interlacer.
*
* @author
*  Ittiam
*
* @par List of Functions:
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
#include "ideint_function_selector.h"

/**
*******************************************************************************
*
* @brief
*  Return deinterlacer context size
*
* @par   Description
*  Return deinterlacer context size, application will allocate this memory
*  and send it as context to process call
*
* @param[in] None
*
* @returns
* Size of deinterlacer context
*
* @remarks
* None
*
*******************************************************************************
*/
WORD32 ideint_ctxt_size(void)
{
    return sizeof(ctxt_t);
}

/**
*******************************************************************************
*
* @brief
* Deinterlace given fields and produce a frame
*
* @par   Description
*  Deinterlacer function that deinterlaces given fields and produces a frame
*
* @param[in] pv_ctxt
*  Deinterlacer context returned by ideint_create()
*
* @param[in] ps_prv_fld
*  Previous field (can be null, in which case spatial filtering is done
*  unconditionally)
*
* @param[in] ps_cur_fld
*  Current field
*
* @param[in] ps_nxt_fld
*  Next field
*
* @param[in] ps_out_frm
*  Output frame
*
* @param[in] ps_params
*  Parameters
*
* @param[in] start_row
*  Start row
*
* @param[in] num_rows
*  Number of rows to be processed
*
* @returns
*  IDEINT_ERROR_T
*
* @remarks
*
*******************************************************************************
*/
IDEINT_ERROR_T ideint_process(void *pv_ctxt,
                              icv_pic_t *ps_prv_fld,
                              icv_pic_t *ps_cur_fld,
                              icv_pic_t *ps_nxt_fld,
                              icv_pic_t *ps_out_frm,
                              ideint_params_t *ps_params,
                              WORD32 start_row,
                              WORD32 num_rows)
{
    ctxt_t *ps_ctxt;
    WORD32 num_blks_x, num_blks_y;
    WORD32 num_comp;
    WORD32 i, row, col;
    WORD32 rows_remaining;

    if(NULL == pv_ctxt)
        return IDEINT_INVALID_CTXT;

    ps_ctxt = (ctxt_t *)pv_ctxt;

    /* Copy the parameters */
    if(ps_params)
    {
        ps_ctxt->s_params = *ps_params;
    }
    else
    {
        /* Use default params if ps_params is NULL */
        ps_ctxt->s_params.i4_cur_fld_top = 1;
        ps_ctxt->s_params.e_mode = IDEINT_MODE_SPATIAL;
        ps_ctxt->s_params.e_arch = ideint_default_arch();
        ps_ctxt->s_params.e_soc = ICV_SOC_GENERIC;
        ps_ctxt->s_params.i4_disable_weave = 0;
        ps_ctxt->s_params.pf_aligned_alloc = NULL;
        ps_ctxt->s_params.pf_aligned_free = NULL;
    }

    /* Start row has to be multiple of 8 */
    if(start_row & 0x7)
    {
        return IDEINT_START_ROW_UNALIGNED;
    }

    /* Initialize variances */
    ps_ctxt->ai4_vrnc_avg_fb[0] = VAR_AVG_LUMA;
    ps_ctxt->ai4_vrnc_avg_fb[1] = VAR_AVG_CHROMA;
    ps_ctxt->ai4_vrnc_avg_fb[2] = VAR_AVG_CHROMA;

    ideint_init_function_ptr(ps_ctxt);

    rows_remaining = ps_out_frm->ai4_ht[0] - start_row;
    num_rows = MIN(num_rows,
                                        rows_remaining);

    IDEINT_CORRUPT_PIC(ps_out_frm, 0xCD);

    //Weave two fields to get a frame
    if(IDEINT_MODE_WEAVE == ps_ctxt->s_params.e_mode)
    {
        if(0 == ps_ctxt->s_params.i4_disable_weave)
        {
            if(ps_ctxt->s_params.i4_cur_fld_top)
                ideint_weave_pic(ps_cur_fld, ps_nxt_fld, ps_out_frm,
                                 start_row,
                                 num_rows);
            else
                ideint_weave_pic(ps_nxt_fld, ps_cur_fld, ps_out_frm,
                                 start_row,
                                 num_rows);
        }
        return IDEINT_ERROR_NONE;
    }

    num_comp = 3;

    for(i = 0; i < num_comp; i++)
    {
        UWORD8 *pu1_prv, *pu1_out;
        UWORD8 *pu1_top, *pu1_bot, *pu1_dst;
        WORD32 cur_strd, out_strd, dst_strd;

        WORD32 st_thresh;
        WORD32 vrnc_avg_st;
        WORD32 disable_cac_sad;
        WORD32 comp_row_start, comp_row_end;
        num_blks_x = ALIGN8(ps_out_frm->ai4_wd[i]) >> 3;
        num_blks_y = ALIGN8(ps_out_frm->ai4_ht[i]) >> 3;
        comp_row_start = start_row;
        comp_row_end = comp_row_start + num_rows;

        if(i)
        {
            comp_row_start >>= 1;
            comp_row_end >>= 1;
        }

        comp_row_end = MIN(comp_row_end, ps_out_frm->ai4_ht[i]);

        comp_row_start =  ALIGN8(comp_row_start) >> 3;
        comp_row_end  = ALIGN8(comp_row_end) >> 3;
        st_thresh        = ST_THRESH;
        vrnc_avg_st      = VAR_AVG_LUMA;

        if(i)
        {
            st_thresh = ST_THRESH >> 1;
            vrnc_avg_st = VAR_AVG_CHROMA;
        }

        out_strd = ps_out_frm->ai4_strd[i];
        if(ps_ctxt->s_params.i4_cur_fld_top)
        {
            cur_strd = ps_cur_fld->ai4_strd[i];
        }
        else
        {
            cur_strd = ps_nxt_fld->ai4_strd[i];
        }


        disable_cac_sad = 0;
        /* If previous field is not provided, then change to SPATIAL mode */
        if(ps_prv_fld->apu1_buf[i] == NULL)
        {
            disable_cac_sad = 1;
        }

        for(row = comp_row_start; row < comp_row_end; row++)
        {
            pu1_out = ps_out_frm->apu1_buf[i];
            pu1_out += (ps_out_frm->ai4_strd[i] * row << 3);

            pu1_prv = ps_prv_fld->apu1_buf[i];
            pu1_prv += (ps_prv_fld->ai4_strd[i] * row << 2);

            if(ps_ctxt->s_params.i4_cur_fld_top)
            {
                pu1_top = ps_cur_fld->apu1_buf[i];
                pu1_bot = ps_nxt_fld->apu1_buf[i];
            }
            else
            {
                pu1_top = ps_nxt_fld->apu1_buf[i];
                pu1_bot = ps_cur_fld->apu1_buf[i];
            }
            pu1_top += (cur_strd * row << 2);
            pu1_bot += (cur_strd * row << 2);

            for(col = 0; col < num_blks_x; col++)
            {
                WORD32 cac, sad, vrnc;
                WORD32 th_num, th_den;
                UWORD8 au1_dst[BLK_WD * BLK_HT];
                WORD32 blk_wd, blk_ht;
                WORD32 input_boundary;
                cac = 0;
                sad = 0;
                th_den = 0;
                th_num = st_thresh;
                vrnc = 0;

                disable_cac_sad = 0;
                /* If previous field is not provided, then change to SPATIAL mode */
                if(ps_prv_fld->apu1_buf[i] == NULL)
                {
                    disable_cac_sad = 1;
                }
                /* For boundary blocks when input dimensions are not multiple of 8,
                 * then change to spatial mode */
                input_boundary = 0;

                blk_wd = BLK_WD;
                blk_ht = BLK_HT;

                if((((num_blks_x - 1) == col) && (ps_out_frm->ai4_wd[i] & 0x7)) ||
                    (((num_blks_y - 1) == row) && (ps_out_frm->ai4_ht[i] & 0x7)))
                {
                    disable_cac_sad = 1;
                    input_boundary = 1;

                    if(((num_blks_x - 1) == col) && (ps_out_frm->ai4_wd[i] & 0x7))
                        blk_wd = (ps_out_frm->ai4_wd[i] & 0x7);

                    if(((num_blks_y - 1) == row) && (ps_out_frm->ai4_ht[i] & 0x7))
                        blk_ht = (ps_out_frm->ai4_ht[i] & 0x7);

                }

                if(0 == disable_cac_sad)
                {
                    /* Compute SAD */
                    PROFILE_DISABLE_SAD
                    sad = ps_ctxt->pf_sad_8x4(pu1_prv, pu1_bot, cur_strd,
                                              cur_strd,
                                              BLK_WD,
                                              BLK_HT >> 1);
                    /* Compute Variance */
                    PROFILE_DISABLE_VARIANCE
                    vrnc = ps_ctxt->pf_variance_8x4(pu1_top, cur_strd, BLK_WD,
                                                    BLK_HT >> 1);

                    th_num = st_thresh;

                    th_num *= vrnc_avg_st +
                              ((MOD_IDX_ST_NUM * vrnc) >> MOD_IDX_ST_SHIFT);

                    th_den = vrnc +
                             ((MOD_IDX_ST_NUM * vrnc_avg_st) >> MOD_IDX_ST_SHIFT);

                    if((sad * th_den) <= th_num)
                    {
                        /* Calculate Combing Artifact if SAD test fails */
                        PROFILE_DISABLE_CAC
                        cac = ps_ctxt->pf_cac_8x8(pu1_top, pu1_bot, cur_strd, cur_strd);
                    }
                }

                pu1_dst = pu1_out;
                dst_strd = out_strd;

                /* In case boundary blocks are not complete (dimensions non-multiple of 8)
                 * Use intermediate buffer as destination and copy required pixels to output
                 * buffer later
                 */
                if(input_boundary)
                {
                    pu1_dst = au1_dst;
                    dst_strd = BLK_WD;
                    ideint_weave_blk(pu1_top, pu1_bot, pu1_dst, dst_strd,
                                     cur_strd, blk_wd, blk_ht);
                }

                /* Weave the two fields unconditionally */
                if(0 == ps_ctxt->s_params.i4_disable_weave)
                {
                    ideint_weave_blk(pu1_top, pu1_bot, pu1_dst, dst_strd,
                                     cur_strd, blk_wd, blk_ht);
                }

                if(disable_cac_sad || cac || (sad * th_den > th_num))
                {
                    /* Pad the input fields in an intermediate buffer if required */
                    if((0 == row) || (0 == col) ||
                       ((num_blks_x - 1) == col) || ((num_blks_y - 1) == row))
                    {
                        UWORD8 *pu1_dst_top;
                        UWORD8 au1_pad[(BLK_HT + 4) * (BLK_WD + 4)];

                        ideint_pad_blk(pu1_top, pu1_bot, au1_pad, cur_strd, row,
                                       col, num_blks_y, num_blks_x, blk_wd, blk_ht);

                        pu1_dst_top = au1_pad + 2 * (BLK_WD + 4) + 2;

                        PROFILE_DISABLE_SPATIAL
                        ps_ctxt->pf_spatial_filter(pu1_dst_top, pu1_dst + dst_strd,
                                                   (BLK_WD + 4) * 2,
                                                   dst_strd * 2);
                    }
                    else
                    {
                        PROFILE_DISABLE_SPATIAL
                        ps_ctxt->pf_spatial_filter(pu1_top, pu1_dst + dst_strd,
                                                   cur_strd, dst_strd * 2);

                    }
                }

                /* copy required pixels to output buffer for boundary blocks
                 * when dimensions are not multiple of 8
                 */
                if(input_boundary)
                {
                    WORD32 j;

                    for(j = 0; j < blk_ht; j++)
                    {
                        memcpy(pu1_out + j * out_strd, au1_dst + j * BLK_WD, blk_wd);
                    }
                }
                pu1_prv += 8;
                pu1_top += 8;
                pu1_bot += 8;
                pu1_out += 8;
            }
        }
    }
    return IDEINT_ERROR_NONE;
}
