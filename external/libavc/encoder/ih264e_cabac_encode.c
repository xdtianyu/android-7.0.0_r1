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
*  ih264e_cabac.c
*
* @brief
*  Contains all functions to encode in CABAC entropy mode
*
*
* @author
* Doney Alex
*
* @par List of Functions:
*
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
#include <string.h>

/* User include files */
#include "ih264e_config.h"
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264_debug.h"
#include "ih264_defs.h"
#include "ih264e_defs.h"
#include "ih264_macros.h"
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
#include "ih264_platform_macros.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_cabac.h"
#include "ih264e_encode_header.h"
#include "ih264_cavlc_tables.h"
#include "ih264e_cavlc.h"
#include "ih264e_statistics.h"
#include "ih264e_trace.h"

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/




/**
 *******************************************************************************
 *
 * @brief
 *  Encodes mb_skip_flag  using CABAC entropy coding mode.
 *
 * @param[in] u1_mb_skip_flag
 *  mb_skip_flag
 *
 * @param[in] ps_cabac_ctxt
 *  Pointer to cabac context structure
 *
 * @param[in] u4_ctxidx_offset
 *  ctxIdxOffset for mb_skip_flag context
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_enc_mb_skip(UWORD8 u1_mb_skip_flag,
                                     cabac_ctxt_t *ps_cabac_ctxt,
                                     UWORD32 u4_ctxidx_offset)
{

    UWORD8 u4_ctx_inc;
    WORD8 a, b;
    a = ((ps_cabac_ctxt->ps_left_ctxt_mb_info->u1_mb_type & CAB_SKIP_MASK) ?
                    0 : 1);
    b = ((ps_cabac_ctxt->ps_top_ctxt_mb_info->u1_mb_type & CAB_SKIP_MASK) ?
                    0 : 1);

    u4_ctx_inc = a + b;
    /* Encode the bin */
    ih264e_cabac_encode_bin(ps_cabac_ctxt,
                            (UWORD32) u1_mb_skip_flag,
                            ps_cabac_ctxt->au1_cabac_ctxt_table + u4_ctxidx_offset
                                    + u4_ctx_inc);

}


/* ! < Table 9-36 – Binarization for macroblock types in I slices  in ITU_T_H264-201402
 * Bits 0-7 : binarised value
 * Bits 8-15: length of binary sequence
 */
static const UWORD32 u4_mb_type_intra[26] =
    { 0x0100, 0x0620, 0x0621, 0x0622, 0x0623, 0x0748, 0x0749, 0x074a, 0x074b,
      0x074c, 0x074d, 0x074e, 0x074f, 0x0628, 0x0629, 0x062a, 0x062b, 0x0758,
      0x0759, 0x075a, 0x075b, 0x075c, 0x075d, 0x075e, 0x075f, 0x0203 };


/* CtxInc for mb types */
static const UWORD32 u4_mb_ctxinc[2][26] =
{
    /* Intra CtxInc's */
    {   0x00,
        0x03467, 0x03467, 0x03467, 0x03467, 0x034567, 0x034567, 0x034567,
        0x034567, 0x034567, 0x034567, 0x034567, 0x034567, 0x03467, 0x03467,
        0x03467, 0x03467, 0x034567, 0x034567, 0x034567, 0x034567, 0x034567,
        0x034567, 0x034567, 0x034567, 0x00},
    /* Inter CtxInc's */
    {   0x00,
        0x001233, 0x001233, 0x001233, 0x001233, 0x0012233, 0x0012233, 0x0012233,
        0x0012233, 0x0012233, 0x0012233, 0x0012233, 0x0012233, 0x001233, 0x001233,
        0x001233, 0x001233, 0x0012233, 0x0012233, 0x0012233, 0x0012233, 0x0012233,
        0x0012233, 0x0012233, 0x0012233, 0x00}
};


/**
 *******************************************************************************
 *
 * @brief
 *  Encodes mb_type for an intra MB.
 *
 * @param[in] u4_slice_type
 *  slice type
 *
 * @param[in] u4_intra_mb_type
 *  MB type (Table 7-11)
 *
 * @param[in] ps_cabac_ctxt
 *  Pointer to cabac context structure
 *
 ** @param[in] u4_ctxidx_offset
 *  ctxIdxOffset for mb_type context
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */

static void ih264e_cabac_enc_intra_mb_type(UWORD32 u4_slice_type,
                                           UWORD32 u4_intra_mb_type,
                                           cabac_ctxt_t *ps_cabac_ctxt,
                                           UWORD32 u4_ctx_idx_offset)
{

    encoding_envirnoment_t *ps_cab_enc_env = &(ps_cabac_ctxt->s_cab_enc_env);
    bin_ctxt_model *pu1_mb_bin_ctxt, *pu1_bin_ctxt;
    UWORD8 u1_bin;
    mb_info_ctxt_t *ps_left_ctxt = ps_cabac_ctxt->ps_left_ctxt_mb_info;
    mb_info_ctxt_t *ps_top_ctxt = ps_cabac_ctxt->ps_top_ctxt_mb_info;
    UWORD32 u4_bins;
    UWORD32 u4_ctx_inc;
    WORD8 i1_bins_len;
    UWORD32 u4_code_int_range;
    UWORD32 u4_code_int_low;
    UWORD16 u2_quant_code_int_range;
    UWORD16 u4_code_int_range_lps;
    WORD8 i;
    UWORD8 u1_ctx_inc;
    UWORD32 u4_table_val;

    pu1_mb_bin_ctxt = ps_cabac_ctxt->au1_cabac_ctxt_table + u4_ctx_idx_offset;

    u4_bins = u4_mb_type_intra[u4_intra_mb_type];
    i1_bins_len = (WORD8) ((u4_bins >> 8) & 0x0f);
    u4_ctx_inc = u4_mb_ctxinc[(u4_slice_type != ISLICE)][u4_intra_mb_type];
    u1_ctx_inc = 0;
    if (u4_slice_type == ISLICE)
    {
        if (ps_left_ctxt != ps_cabac_ctxt->ps_def_ctxt_mb_info)
            u1_ctx_inc += ((ps_left_ctxt->u1_mb_type != CAB_I4x4) ? 1 : 0);
        if (ps_top_ctxt != ps_cabac_ctxt->ps_def_ctxt_mb_info)
            u1_ctx_inc += ((ps_top_ctxt->u1_mb_type != CAB_I4x4) ? 1 : 0);

        u4_ctx_inc = (u4_ctx_inc | (u1_ctx_inc << ((i1_bins_len - 1) << 2)));
    }
    else
    {
        pu1_mb_bin_ctxt += 3;
        if (u4_slice_type == BSLICE)
            pu1_mb_bin_ctxt += 2;

    }

    u4_code_int_range = ps_cab_enc_env->u4_code_int_range;
    u4_code_int_low = ps_cab_enc_env->u4_code_int_low;

    for (i = (i1_bins_len - 1); i >= 0; i--)
    {
        WORD32 shift;

        u1_ctx_inc = ((u4_ctx_inc >> (i << 2)) & 0x0f);
        u1_bin = ((u4_bins >> i) & 0x01);
        /* Encode the bin */
        pu1_bin_ctxt = pu1_mb_bin_ctxt + u1_ctx_inc;
        if (i != (i1_bins_len - 2))
        {
            WORD8 i1_mps = !!((*pu1_bin_ctxt) & (0x40));
            WORD8 i1_state = (*pu1_bin_ctxt) & 0x3F;

            u2_quant_code_int_range = ((u4_code_int_range >> 6) & 0x03);
            u4_table_val =
                            gau4_ih264_cabac_table[i1_state][u2_quant_code_int_range];
            u4_code_int_range_lps = u4_table_val & 0xFF;

            u4_code_int_range -= u4_code_int_range_lps;
            if (u1_bin != i1_mps)
            {
                u4_code_int_low += u4_code_int_range;
                u4_code_int_range = u4_code_int_range_lps;
                if (i1_state == 0)
                {
                    /* MPS(CtxIdx) = 1 - MPS(CtxIdx) */
                    i1_mps = 1 - i1_mps;
                }

                i1_state = (u4_table_val >> 15) & 0x3F;
            }
            else
            {
                i1_state = (u4_table_val >> 8) & 0x3F;

            }

            (*pu1_bin_ctxt) = (i1_mps << 6) | i1_state;
        }
        else
        {
            u4_code_int_range -= 2;
        }

        /* Renormalize */
        /*****************************************************************/
        /* Renormalization; calculate bits generated based on range(R)   */
        /* Note : 6 <= R < 512; R is 2 only for terminating encode       */
        /*****************************************************************/
        GETRANGE(shift, u4_code_int_range);
        shift = 9 - shift;
        u4_code_int_low <<= shift;
        u4_code_int_range <<= shift;

        /* bits to be inserted in the bitstream */
        ps_cab_enc_env->u4_bits_gen += shift;
        ps_cab_enc_env->u4_code_int_range = u4_code_int_range;
        ps_cab_enc_env->u4_code_int_low = u4_code_int_low;

        /* generate stream when a byte is ready */
        if (ps_cab_enc_env->u4_bits_gen > CABAC_BITS)
        {
            ih264e_cabac_put_byte(ps_cabac_ctxt);
            u4_code_int_range = ps_cab_enc_env->u4_code_int_range;
            u4_code_int_low = ps_cab_enc_env->u4_code_int_low;

        }
    }
}



/**
 *******************************************************************************
 *
 * @brief
 *  Encodes prev_intra4x4_pred_mode_flag and
 *  rem_intra4x4_pred_mode using CABAC entropy coding mode
 *
 * @param[in] ps_cabac_ctxt
 *  Pointer to cabac context structure
 *
 *  @param[in] pu1_intra_4x4_modes
 *  Pointer to array containing prev_intra4x4_pred_mode_flag and
 *  rem_intra4x4_pred_mode
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_enc_4x4mb_modes(cabac_ctxt_t *ps_cabac_ctxt,
                                         UWORD8 *pu1_intra_4x4_modes)
{
    WORD32 i;
    WORD8 byte;
    for (i = 0; i < 16; i += 2)
    {
        /* sub blk idx 1 */
        byte = *pu1_intra_4x4_modes++;
        if (byte & 0x1)
        {
            ih264e_cabac_encode_bin(ps_cabac_ctxt,
                                    1,
                                    ps_cabac_ctxt->au1_cabac_ctxt_table
                                            + PREV_INTRA4X4_PRED_MODE_FLAG);
        }
        else
        {
            /* Binarization is FL and Cmax=7 */
            ih264e_encode_decision_bins(byte & 0xF,
                                        4,
                                        0x05554,
                                        4,
                                        ps_cabac_ctxt->au1_cabac_ctxt_table
                                            + REM_INTRA4X4_PRED_MODE - 5,
                                        ps_cabac_ctxt);
        }
        /* sub blk idx 2 */
        byte >>= 4;
        if (byte & 0x1)
        {
            ih264e_cabac_encode_bin(ps_cabac_ctxt,
                                    1,
                                    ps_cabac_ctxt->au1_cabac_ctxt_table
                                            + PREV_INTRA4X4_PRED_MODE_FLAG);
        }
        else
        {
            ih264e_encode_decision_bins(byte & 0xF,
                                        4,
                                        0x05554,
                                        4,
                                        ps_cabac_ctxt->au1_cabac_ctxt_table
                                            + REM_INTRA4X4_PRED_MODE - 5,
                                        ps_cabac_ctxt);
        }
    }
}



/**
 *******************************************************************************
 *
 * @brief
 *  Encodes chroma  intrapred mode for the MB.
 *
 * @param[in] u1_chroma_pred_mode
 *  Chroma intr prediction mode
 *
 * @param[in] ps_cabac_ctxt
 *  Pointer to cabac context structure
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_enc_chroma_predmode(UWORD8 u1_chroma_pred_mode,
                                             cabac_ctxt_t *ps_cabac_ctxt)
{

    WORD8 i1_temp;
    mb_info_ctxt_t *ps_curr_ctxt = ps_cabac_ctxt->ps_curr_ctxt_mb_info;
    mb_info_ctxt_t *ps_left_ctxt = ps_cabac_ctxt->ps_left_ctxt_mb_info;
    mb_info_ctxt_t *ps_top_ctxt = ps_cabac_ctxt->ps_top_ctxt_mb_info;
    UWORD32 u4_bins = 0;
    WORD8 i1_bins_len = 1;
    UWORD32 u4_ctx_inc = 0;
    UWORD8 a, b;
    a = ((ps_left_ctxt->u1_intrapred_chroma_mode != 0) ? 1 : 0);
    b = ((ps_top_ctxt->u1_intrapred_chroma_mode != 0) ? 1 : 0);

    /* Binarization is TU and Cmax=3 */
    ps_curr_ctxt->u1_intrapred_chroma_mode = u1_chroma_pred_mode;

    u4_ctx_inc = a + b;
    u4_ctx_inc = (u4_ctx_inc | 0x330);
    if (u1_chroma_pred_mode)
    {
        u4_bins = 1;
        i1_temp = u1_chroma_pred_mode;
        i1_temp--;
        /* Put a stream of 1's of length Chromaps_pred_mode_ctxt value */
        while (i1_temp)
        {
            u4_bins = (u4_bins | (1 << i1_bins_len));
            i1_bins_len++;
            i1_temp--;
        }
        /* If Chromaps_pred_mode_ctxt < Cmax i.e 3. Terminate put a zero */
        if (u1_chroma_pred_mode < 3)
        {
            i1_bins_len++;
        }
    }

    ih264e_encode_decision_bins(u4_bins,
                                i1_bins_len,
                                u4_ctx_inc,
                                3,
                                ps_cabac_ctxt->au1_cabac_ctxt_table
                                    + INTRA_CHROMA_PRED_MODE,
                                ps_cabac_ctxt);

}


/**
 *******************************************************************************
 *
 * @brief
 *  Encodes CBP for the MB.
 *
 * @param[in] u1_cbp
 *  CBP for the MB
 *
 * @param[in] ps_cabac_ctxt
 *  Pointer to cabac context structure
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_enc_cbp(UWORD32 u4_cbp, cabac_ctxt_t *ps_cabac_ctxt)
{
    mb_info_ctxt_t *ps_left_ctxt = ps_cabac_ctxt->ps_left_ctxt_mb_info;
    mb_info_ctxt_t *ps_top_ctxt = ps_cabac_ctxt->ps_top_ctxt_mb_info;
    WORD8 i2_cbp_chroma, i, j;
    UWORD8 u1_ctxt_inc, u1_bin;
    UWORD8 a, b;
    UWORD32 u4_ctx_inc;
    UWORD32 u4_bins;
    WORD8 i1_bins_len;

    /* CBP Luma, FL, Cmax = 15, L = 4 */
    u4_ctx_inc = 0;
    u4_bins = 0;
    i1_bins_len = 5;
    for (i = 0; i < 4; i++)
    {
        /* calulate ctxtInc, depending on neighbour availability */
        /* u1_ctxt_inc = CondTerm(A) + 2 * CondTerm(B);
         A: Left block and B: Top block */

        /* Check for Top availability */
        if (i >> 1)
        {
            j = i - 2;
            /* Top is available always and it's current MB */
            b = (((u4_cbp >> j) & 0x01) != 0 ? 0 : 1);
        }
        else
        {
            /* for blocks whose top reference is in another MB */
            {
                j = i + 2;
                b = ((ps_top_ctxt->u1_cbp >> j) & 0x01) ? 0 : 1;
            }
        }

        /* Check for Left availability */
        if (i & 0x01)
        {
            /* Left is available always and it's current MB */
            j = i - 1;
            a = (((u4_cbp >> j) & 0x01) != 0 ? 0 : 1);
        }
        else
        {
            {
                j = i + 1;
                a = ((ps_left_ctxt->u1_cbp >> j) & 0x01) ? 0 : 1;
            }
        }
        u1_ctxt_inc = a + 2 * b;
        u1_bin = ((u4_cbp >> i) & 0x01);
        u4_ctx_inc = (u4_ctx_inc | (u1_ctxt_inc << (i << 2)));
        u4_bins = (u4_bins | (u1_bin << i));
    }

    /* CBP Chroma, TU, Cmax = 2 */
    i2_cbp_chroma = u4_cbp >> 4;
    /* calulate ctxtInc, depending on neighbour availability */
    a = (ps_left_ctxt->u1_cbp > 15) ? 1 : 0;
    b = (ps_top_ctxt->u1_cbp > 15) ? 1 : 0;

    u1_ctxt_inc = a + 2 * b;
    if (i2_cbp_chroma)
    {
        u4_ctx_inc = u4_ctx_inc | ((4 + u1_ctxt_inc) << 16);
        u4_bins = (u4_bins | 0x10);
        /* calulate ctxtInc, depending on neighbour availability */
        a = (ps_left_ctxt->u1_cbp > 31) ? 1 : 0;
        b = (ps_top_ctxt->u1_cbp > 31) ? 1 : 0;
        u1_ctxt_inc = a + 2 * b;
        u4_ctx_inc = u4_ctx_inc | ((8 + u1_ctxt_inc) << 20);
        u4_bins = (u4_bins | (((i2_cbp_chroma >> 1) & 0x01) << i1_bins_len));
        i1_bins_len++;
    }
    else
    {
        u4_ctx_inc = (u4_ctx_inc | ((4 + u1_ctxt_inc) << 16));
    }
    ih264e_encode_decision_bins(u4_bins, i1_bins_len, u4_ctx_inc, 8,
                                ps_cabac_ctxt->au1_cabac_ctxt_table + CBP_LUMA,
                                ps_cabac_ctxt);
}


/**
 *******************************************************************************
 *
 * @brief
 *  Encodes mb_qp_delta for the MB.
 *
 * @param[in] i1_mb_qp_delta
 *  mb_qp_delta
 *
 * @param[in] ps_cabac_ctxt
 *  Pointer to cabac context structure
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_enc_mb_qp_delta(WORD8 i1_mb_qp_delta,
                                         cabac_ctxt_t *ps_cabac_ctxt)
{
    UWORD8 u1_code_num;
    UWORD8 u1_ctxt_inc;

    UWORD32 u4_ctx_inc;
    UWORD32 u4_bins;
    WORD8 i1_bins_len;
    UWORD8 u1_ctx_inc, u1_bin;
    /* Range of ps_mb_qp_delta_ctxt= -26 to +25 inclusive */
        ASSERT((i1_mb_qp_delta < 26) && (i1_mb_qp_delta > -27));
    /* if ps_mb_qp_delta_ctxt=0, then codeNum=0 */
    u1_code_num = 0;
    if (i1_mb_qp_delta > 0)
        u1_code_num = (i1_mb_qp_delta << 1) - 1;
    else if (i1_mb_qp_delta < 0)
        u1_code_num = (ABS(i1_mb_qp_delta)) << 1;

    u4_ctx_inc = 0;
    u4_bins = 0;
    i1_bins_len = 1;
    /* calculate ctxtInc, depending on neighbour availability */
    u1_ctxt_inc = (!(!(ps_cabac_ctxt->i1_prevps_mb_qp_delta_ctxt)));
    ps_cabac_ctxt->i1_prevps_mb_qp_delta_ctxt = i1_mb_qp_delta;

    if (u1_code_num == 0)
    {
        /* b0 */
        u1_bin = (UWORD8) (u4_bins);
        u1_ctx_inc = u1_ctxt_inc & 0x0f;
        /* Encode the bin */
        ih264e_cabac_encode_bin(ps_cabac_ctxt,
                                u1_bin,
                                ps_cabac_ctxt->au1_cabac_ctxt_table + MB_QP_DELTA
                                        + u1_ctx_inc);

    }
    else
    {
        /* b0 */
        u4_ctx_inc = u1_ctxt_inc;
        u4_bins = 1;
        u1_code_num--;
        if (u1_code_num == 0)
        {
            /* b1 */
            u4_ctx_inc = (u4_ctx_inc | 0x20);
            i1_bins_len++;
            ih264e_encode_decision_bins(u4_bins, i1_bins_len, u4_ctx_inc, 3,
                                        ps_cabac_ctxt->au1_cabac_ctxt_table + MB_QP_DELTA,
                                        ps_cabac_ctxt);
        }
        else
        {
            /* b1 */
            u4_ctx_inc = (u4_ctx_inc | 0x20);
            u4_bins = (u4_bins | (1 << i1_bins_len));
            i1_bins_len++;
            u1_code_num--;
            /* BinIdx from b2 onwards */
            if (u1_code_num < 30)
            { /* maximum i1_bins_len = 31 */
                while (u1_code_num)
                {
                    u4_bins = (u4_bins | (1 << i1_bins_len));
                    i1_bins_len++;
                    u1_code_num--;
                };
                u4_ctx_inc = (u4_ctx_inc | 0x300);
                i1_bins_len++;
                ih264e_encode_decision_bins(u4_bins,
                                            i1_bins_len,
                                            u4_ctx_inc,
                                            2,
                                            ps_cabac_ctxt->au1_cabac_ctxt_table
                                                + MB_QP_DELTA,
                                            ps_cabac_ctxt);
            }
            else
            {
                /* maximum i1_bins_len = 53 */
                u4_bins = 0xffffffff;
                i1_bins_len = 32;
                u4_ctx_inc = (u4_ctx_inc | 0x300);
                u1_code_num -= 30;
                ih264e_encode_decision_bins(u4_bins,
                                            i1_bins_len,
                                            u4_ctx_inc,
                                            2,
                                            ps_cabac_ctxt->au1_cabac_ctxt_table
                                                + MB_QP_DELTA,
                                            ps_cabac_ctxt);
                u4_bins = 0;
                i1_bins_len = 0;
                u4_ctx_inc = 0x033;
                while (u1_code_num)
                {
                    u4_bins = (u4_bins | (1 << i1_bins_len));
                    i1_bins_len++;
                    u1_code_num--;
                };

                u4_ctx_inc = (u4_ctx_inc | 0x300);
                i1_bins_len++;
                ih264e_encode_decision_bins(u4_bins,
                                            i1_bins_len,
                                            u4_ctx_inc,
                                            1,
                                            ps_cabac_ctxt->au1_cabac_ctxt_table
                                                + MB_QP_DELTA,
                                            ps_cabac_ctxt);
            }
        }
    }
}




/**
 *******************************************************************************
 * @brief
 *  Encodes 4residual_block_cabac as defined in 7.3.5.3.3.
 *
 * @param[in] pi2_res_block
 *  pointer to the array of residues
 *
 * @param[in]  u1_nnz
 *  Number of non zero coeffs in the block
 *
 * @param[in] u1_max_num_coeffs
 *  Max number of coeffs that can be there in the block
 *
 * @param[in] u2_sig_coeff_map
 *  Significant coeff map
 *
 * @param[in] u4_ctx_cat_offset
 *  ctxIdxOffset for  absolute value contexts
 *
 * @param[in]  pu1_ctxt_sig_coeff
 *  Pointer to residual state variables
 *
 * @param[in] ps_cabac_ctxt
 *  Pointer to cabac context structure
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_write_coeff4x4(WORD16 *pi2_res_block, UWORD8 u1_nnz,
                                        UWORD8 u1_max_num_coeffs,
                                        UWORD16 u2_sig_coeff_map,
                                        UWORD32 u4_ctx_cat_offset,
                                        bin_ctxt_model *pu1_ctxt_sig_coeff,
                                        cabac_ctxt_t *ps_cabac_ctxt)
{

    WORD8 i;
    WORD16 *pi16_coeffs;
    UWORD32 u4_sig_coeff, u4_bins;
    UWORD32 u4_ctx_inc;
    UWORD8 u1_last_sig_coef_index = (31 - CLZ(u2_sig_coeff_map));

    /* Always put Coded Block Flag as 1 */

        pi16_coeffs = pi2_res_block;
        {
            bin_ctxt_model *pu1_bin_ctxt;
            UWORD8 u1_bin, uc_last;

            i = 0;
            pu1_bin_ctxt = pu1_ctxt_sig_coeff;
            u4_sig_coeff = 0;
            u1_bin = 1;
            if ((u1_last_sig_coef_index))
            {
                u1_bin = !!(u2_sig_coeff_map & 01);
            }
            uc_last = 1;

            do
            {
                /* Encode Decision */
                ih264e_cabac_encode_bin(ps_cabac_ctxt, u1_bin, pu1_bin_ctxt);

                if (u1_bin & uc_last)
                {
                    u4_sig_coeff = (u4_sig_coeff | (1 << i));
                    pu1_bin_ctxt = pu1_ctxt_sig_coeff + i
                                    + LAST_SIGNIFICANT_COEFF_FLAG_FRAME
                                    - SIGNIFICANT_COEFF_FLAG_FRAME;
                    u1_bin = (i == u1_last_sig_coef_index);
                    uc_last = 0;
                }
                else
                {
                    i = i + 1;
                    pu1_bin_ctxt = pu1_ctxt_sig_coeff + i;
                    u1_bin = (i == u1_last_sig_coef_index);
                    uc_last = 1;
                    if ((i != u1_last_sig_coef_index))
                    {
                        u1_bin = !!((u2_sig_coeff_map >> i) & 01);
                    }
                }
            }while (!((i > u1_last_sig_coef_index)
                            || (i > (u1_max_num_coeffs - 1))));
        }

        /* Encode coeff_abs_level_minus1 and coeff_sign_flag */
        {
            UWORD8 u1_sign;
            UWORD16 u2_abs_level;
            UWORD8 u1_abs_level_equal1 = 1, u1_abs_level_gt1 = 0;
            UWORD8 u1_ctx_inc;
            UWORD8 u1_coff;
            WORD16 i2_sufs;
            WORD8 i1_bins_len;
            i = u1_last_sig_coef_index;
            pi16_coeffs = pi2_res_block + u1_nnz - 1;
            do
            {
                {
                    u4_sig_coeff = u4_sig_coeff & ((1 << i) - 1);
                    u4_bins = 0;
                    u4_ctx_inc = 0;
                    i1_bins_len = 1;
                    /* Encode the AbsLevelMinus1 */
                    u2_abs_level = ABS(*(pi16_coeffs)) - 1;
                    /* CtxInc for bin0 */
                    u4_ctx_inc = MIN(u1_abs_level_equal1, 4);
                    /* CtxInc for remaining */
                    u1_ctx_inc = 5 + MIN(u1_abs_level_gt1, 4);
                    u4_ctx_inc = u4_ctx_inc + (u1_ctx_inc << 4);
                    if (u2_abs_level)
                    {
                        u1_abs_level_gt1++;
                        u1_abs_level_equal1 = 0;
                    }
                    if (!u1_abs_level_gt1)
                        u1_abs_level_equal1++;

                    u1_coff = 14;
                    if (u2_abs_level >= u1_coff)
                    {
                        /* Prefix TU i.e string of 14 1's */
                        u4_bins = 0x3fff;
                        i1_bins_len = 14;
                        ih264e_encode_decision_bins(u4_bins, i1_bins_len,
                                                    u4_ctx_inc, 1, ps_cabac_ctxt->au1_cabac_ctxt_table
                                                    + u4_ctx_cat_offset,
                                                    ps_cabac_ctxt);

                        /* Suffix, uses EncodeBypass */
                        i2_sufs = u2_abs_level - u1_coff;

                        u4_bins = ih264e_cabac_UEGk0_binarization(i2_sufs,
                                                                  &i1_bins_len);

                        ih264e_cabac_encode_bypass_bins(ps_cabac_ctxt, u4_bins,
                                                        i1_bins_len);

                    }
                    else
                    {
                        /* Prefix only */
                        u4_bins = (1 << u2_abs_level) - 1;
                        i1_bins_len = u2_abs_level + 1;
                        /* Encode Terminating bit */
                        ih264e_encode_decision_bins(u4_bins, i1_bins_len,
                                                    u4_ctx_inc, 1, ps_cabac_ctxt->au1_cabac_ctxt_table
                                                    + u4_ctx_cat_offset,
                                                    ps_cabac_ctxt);
                    }
                }
                /* encode coeff_sign_flag[i] */
                u1_sign = ((*pi16_coeffs) < 0) ? 1 : 0;
                ih264e_cabac_encode_bypass_bin(ps_cabac_ctxt, u1_sign);
                i = CLZ(u4_sig_coeff);
                i = 31 - i;
                pi16_coeffs--;
            }while (u4_sig_coeff);
        }

}


/**
 *******************************************************************************
 * @brief
 * Write DC coeffs for intra predicted luma block
 *
 * @param[in] ps_ent_ctxt
 *  Pointer to entropy context structure
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_encode_residue_luma_dc(entropy_ctxt_t *ps_ent_ctxt)
{

    /* CABAC context */
    cabac_ctxt_t *ps_cabac_ctxt = ps_ent_ctxt->ps_cabac;
    tu_sblk_coeff_data_t *ps_mb_coeff_data;

    /* packed residue */
    void *pv_mb_coeff_data = ps_ent_ctxt->pv_mb_coeff_data;
    UWORD16 u2_sig_coeff_map;
    WORD16 *pi2_res_block;
    UWORD8 u1_nnz;
    UWORD8 u1_cbf;
    mb_info_ctxt_t *ps_top_ctxt = ps_cabac_ctxt->ps_top_ctxt_mb_info;
    mb_info_ctxt_t *p_CurCtxt = ps_cabac_ctxt->ps_curr_ctxt_mb_info;

    PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, u1_nnz,
                               u2_sig_coeff_map, pi2_res_block);

    u1_cbf = !!(u1_nnz);

    {
        UWORD32 u4_ctx_inc;
        UWORD8 u1_a, u1_b;

        u1_a = ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0] & 0x1;
        u1_b = ps_top_ctxt->u1_yuv_dc_csbp & 0x1;
        u4_ctx_inc = u1_a + (u1_b << 1);

        ih264e_cabac_encode_bin(ps_cabac_ctxt,
                                u1_cbf,
                                ps_cabac_ctxt->au1_cabac_ctxt_table + CBF
                                        + (LUMA_DC_CTXCAT << 2) + u4_ctx_inc);
    }

    /* Write coded_block_flag */
    if (u1_cbf)
    {
        ih264e_cabac_write_coeff4x4(pi2_res_block,
                                   u1_nnz,
                                   15,
                                   u2_sig_coeff_map,
                                   COEFF_ABS_LEVEL_MINUS1 + COEFF_ABS_LEVEL_CAT_0_OFFSET,
                                   ps_cabac_ctxt->au1_cabac_ctxt_table
                                        + SIGNIFICANT_COEFF_FLAG_FRAME
                                        + SIG_COEFF_CTXT_CAT_0_OFFSET,
                                   ps_cabac_ctxt);

        ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0] |= 0x1;
        p_CurCtxt->u1_yuv_dc_csbp |= 0x1;
    }
    else
    {
        ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0] &= 0x6;
        p_CurCtxt->u1_yuv_dc_csbp &= 0x6;
    }

    ps_ent_ctxt->pv_mb_coeff_data = pv_mb_coeff_data;
}




/**
 *******************************************************************************
 * @brief
 * Write chroma residues to the bitstream
 *
 * @param[in] ps_ent_ctxt
 *  Pointer to entropy context structure
 *
 * @param[in] u1_chroma_cbp
 * coded block pattern, chroma
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_write_chroma_residue(entropy_ctxt_t *ps_ent_ctxt,
                                              UWORD8 u1_chroma_cbp)
{
    /* CABAC context */
    cabac_ctxt_t *ps_cabac_ctxt = ps_ent_ctxt->ps_cabac;
    tu_sblk_coeff_data_t *ps_mb_coeff_data;
    /* packed residue */
    void *pv_mb_coeff_data = ps_ent_ctxt->pv_mb_coeff_data;
    UWORD16 u2_sig_coeff_map;
    UWORD8 u1_nnz;
    mb_info_ctxt_t *ps_top_ctxt_mb_info, *ps_curr_ctxt;

    ps_top_ctxt_mb_info = ps_cabac_ctxt->ps_top_ctxt_mb_info;
    ps_curr_ctxt = ps_cabac_ctxt->ps_curr_ctxt_mb_info;

    /********************/
    /* Write Chroma DC */
    /********************/
    {
        WORD16 *pi2_res_block;
        UWORD8 u1_left_dc_csbp, u1_top_dc_csbp, u1_uv, u1_cbf;

        u1_left_dc_csbp = (ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0]) >> 1;
        u1_top_dc_csbp = (ps_top_ctxt_mb_info->u1_yuv_dc_csbp) >> 1;

        for (u1_uv = 0; u1_uv < 2; u1_uv++)
        {
            PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data,
                                       u1_nnz, u2_sig_coeff_map, pi2_res_block);
            u1_cbf = !!(u1_nnz);
            {
                UWORD8 u1_a, u1_b;
                UWORD32 u4_ctx_inc;
                u1_a = (u1_left_dc_csbp >> u1_uv) & 0x01;
                u1_b = (u1_top_dc_csbp >> u1_uv) & 0x01;
                u4_ctx_inc = (u1_a + (u1_b << 1));

                ih264e_cabac_encode_bin(ps_cabac_ctxt,
                                        u1_cbf,
                                        ps_cabac_ctxt->au1_cabac_ctxt_table + CBF
                                                + (CHROMA_DC_CTXCAT << 2)
                                                + u4_ctx_inc);
            }

            if (u1_cbf)
            {
                ih264e_cabac_write_coeff4x4(pi2_res_block,
                                            u1_nnz,
                                            3,
                                            u2_sig_coeff_map,
                                            COEFF_ABS_LEVEL_MINUS1
                                                + COEFF_ABS_LEVEL_CAT_3_OFFSET,
                                             ps_cabac_ctxt->au1_cabac_ctxt_table
                                                + SIGNIFICANT_COEFF_FLAG_FRAME
                                                + SIG_COEFF_CTXT_CAT_3_OFFSET,
                                              ps_cabac_ctxt);

                SETBIT(u1_top_dc_csbp, u1_uv);
                SETBIT(u1_left_dc_csbp, u1_uv);
            }
            else
            {
                CLEARBIT(u1_top_dc_csbp, u1_uv);
                CLEARBIT(u1_left_dc_csbp, u1_uv);
            }
        }
        /*************************************************************/
        /*      Update the DC csbp                                   */
        /*************************************************************/
        ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0] &= 0x1;
        ps_curr_ctxt->u1_yuv_dc_csbp &= 0x1;
        ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0] |= (u1_left_dc_csbp << 1);
        ps_curr_ctxt->u1_yuv_dc_csbp |= (u1_top_dc_csbp << 1);
    }
    /*******************/
    /* Write Chroma AC */
    /*******************/
    {
        if (u1_chroma_cbp == 2)
        {
            UWORD8 u1_uv_blkno, u1_left_ac_csbp, u1_top_ac_csbp;
            WORD16 *pi2_res_block;
            u1_left_ac_csbp = ps_cabac_ctxt->pu1_left_uv_ac_csbp[0];
            u1_top_ac_csbp = ps_top_ctxt_mb_info->u1_yuv_ac_csbp >> 4;

            for (u1_uv_blkno = 0; u1_uv_blkno < 8; u1_uv_blkno++)
            {
                UWORD8 u1_cbf;
                UWORD8 u1_b2b0, u1_b2b1;
                PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data,
                                           u1_nnz, u2_sig_coeff_map,
                                           pi2_res_block);

                u1_cbf = !!(u1_nnz);
                u1_b2b0 = ((u1_uv_blkno & 0x4) >> 1) | (u1_uv_blkno & 0x1);
                u1_b2b1 = ((u1_uv_blkno & 0x4) >> 1)
                                | ((u1_uv_blkno & 0x2) >> 1);

                {
                    UWORD8 u1_a, u1_b;
                    UWORD32 u4_ctx_inc;
                    /* write coded_block_flag */
                    u1_a = (u1_left_ac_csbp >> u1_b2b1) & 0x1;
                    u1_b = (u1_top_ac_csbp >> u1_b2b0) & 0x1;
                    u4_ctx_inc = u1_a + (u1_b << 1);

                    ih264e_cabac_encode_bin(ps_cabac_ctxt,
                                            u1_cbf,
                                            ps_cabac_ctxt->au1_cabac_ctxt_table + CBF
                                                    + (CHROMA_AC_CTXCAT << 2)
                                                    + u4_ctx_inc);

                }
                if (u1_cbf)
                {
                    ih264e_cabac_write_coeff4x4(pi2_res_block,
                                                u1_nnz,
                                                14,
                                                u2_sig_coeff_map,
                                                COEFF_ABS_LEVEL_MINUS1
                                                    + COEFF_ABS_LEVEL_CAT_4_OFFSET,
                                                ps_cabac_ctxt->au1_cabac_ctxt_table
                                                    + +SIGNIFICANT_COEFF_FLAG_FRAME
                                                    + SIG_COEFF_CTXT_CAT_4_OFFSET,
                                                ps_cabac_ctxt);

                    SETBIT(u1_left_ac_csbp, u1_b2b1);
                    SETBIT(u1_top_ac_csbp, u1_b2b0);
                }
                else
                {
                    CLEARBIT(u1_left_ac_csbp, u1_b2b1);
                    CLEARBIT(u1_top_ac_csbp, u1_b2b0);

                }
            }
            /*************************************************************/
            /*      Update the AC csbp                                   */
            /*************************************************************/
            ps_cabac_ctxt->pu1_left_uv_ac_csbp[0] = u1_left_ac_csbp;
            ps_curr_ctxt->u1_yuv_ac_csbp &= 0x0f;
            ps_curr_ctxt->u1_yuv_ac_csbp |= (u1_top_ac_csbp << 4);
        }
        else
        {
            ps_cabac_ctxt->pu1_left_uv_ac_csbp[0] = 0;
            ps_curr_ctxt->u1_yuv_ac_csbp &= 0xf;
        }
    }
    ps_ent_ctxt->pv_mb_coeff_data = pv_mb_coeff_data;
}




/**
 *******************************************************************************
 * @brief
 * Encodes Residues for the MB as defined in 7.3.5.3
 *
 * @param[in] ps_ent_ctxt
 *  Pointer to entropy context structure
 *
 * @param[in] u1_cbp
 * coded block pattern
 *
 * @param[in] u1_ctx_cat
 * Context category, LUMA_AC_CTXCAT or LUMA_4x4_CTXCAT
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_encode_residue(entropy_ctxt_t *ps_ent_ctxt,
                                        UWORD32 u4_cbp, UWORD8 u1_ctx_cat)
{
    /* CABAC context */
    cabac_ctxt_t *ps_cabac_ctxt = ps_ent_ctxt->ps_cabac;

    tu_sblk_coeff_data_t *ps_mb_coeff_data;
    /* packed residue */
    void *pv_mb_coeff_data = ps_ent_ctxt->pv_mb_coeff_data;
    UWORD16 u2_sig_coeff_map;
    UWORD8 u1_nnz;
    mb_info_ctxt_t *ps_curr_ctxt;
    mb_info_ctxt_t *ps_top_ctxt;
    UWORD8 u1_left_ac_csbp;
    UWORD8 u1_top_ac_csbp;
    UWORD32 u4_ctx_idx_offset_sig_coef, u4_ctx_idx_offset_abs_lvl;
    ps_curr_ctxt = ps_cabac_ctxt->ps_curr_ctxt_mb_info;
    ps_top_ctxt = ps_cabac_ctxt->ps_top_ctxt_mb_info;
    u1_left_ac_csbp = ps_cabac_ctxt->pu1_left_y_ac_csbp[0];
    u1_top_ac_csbp = ps_top_ctxt->u1_yuv_ac_csbp;

    if (u4_cbp & 0xf)
    {
        /*  Write luma residue  */
        UWORD8 u1_offset;
        WORD16 *pi2_res_block;
        UWORD8 u1_subblk_num;
        if (u1_ctx_cat == LUMA_AC_CTXCAT)
        {
            u1_offset = 1;
            u4_ctx_idx_offset_sig_coef = SIG_COEFF_CTXT_CAT_1_OFFSET;
            u4_ctx_idx_offset_abs_lvl = COEFF_ABS_LEVEL_MINUS1
                                      + COEFF_ABS_LEVEL_CAT_1_OFFSET;
        }
        else
        {
            u1_offset = 0;
            u4_ctx_idx_offset_sig_coef = SIG_COEFF_CTXT_CAT_2_OFFSET;
            u4_ctx_idx_offset_abs_lvl = COEFF_ABS_LEVEL_MINUS1
                                        + COEFF_ABS_LEVEL_CAT_2_OFFSET;
        }

        for (u1_subblk_num = 0; u1_subblk_num < 16; u1_subblk_num++)
        {
            UWORD8 u1_b0, u1_b1, u1_b2, u1_b3, u1_b2b0, u1_b3b1, u1_b3b2;
            u1_b0 = (u1_subblk_num & 0x1);
            u1_b1 = (u1_subblk_num & 0x2) >> 1;
            u1_b2 = (u1_subblk_num & 0x4) >> 2;
            u1_b3 = (u1_subblk_num & 0x8) >> 3;
            u1_b2b0 = (u1_b2 << 1) | (u1_b0);
            u1_b3b1 = (u1_b3 << 1) | (u1_b1);
            u1_b3b2 = (u1_b3 << 1) | (u1_b2);

            if (!((u4_cbp >> u1_b3b2) & 0x1))
            {
                /* ---------------------------------------------------------- */
                /* The current block is not coded so skip all the sub block */
                /* and set the pointer of scan level, csbp accrodingly      */
                /* ---------------------------------------------------------- */
                CLEARBIT(u1_top_ac_csbp, u1_b2b0);
                CLEARBIT(u1_top_ac_csbp, (u1_b2b0 + 1));
                CLEARBIT(u1_left_ac_csbp, u1_b3b1);
                CLEARBIT(u1_left_ac_csbp, (u1_b3b1 + 1));

                u1_subblk_num += 3;
            }
            else
            {
                UWORD8 u1_csbf;

                PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data,
                                           u1_nnz, u2_sig_coeff_map,
                                           pi2_res_block);

                u1_csbf = !!(u1_nnz);
                {
                    UWORD8 u1_a, u1_b;
                    UWORD32 u4_ctx_inc;
                    u1_b = (u1_top_ac_csbp >> u1_b2b0) & 0x01;
                    u1_a = (u1_left_ac_csbp >> u1_b3b1) & 0x01;
                    u4_ctx_inc = u1_a + (u1_b << 1);

                    /* Encode the bin */
                    ih264e_cabac_encode_bin(ps_cabac_ctxt,
                                            u1_csbf,
                                            ps_cabac_ctxt->au1_cabac_ctxt_table + CBF
                                                + (u1_ctx_cat << 2) + u4_ctx_inc);

                }
                /**************************/
                /* Write coded_block_flag */
                /**************************/
                if (u1_csbf)
                {
                    ih264e_cabac_write_coeff4x4(pi2_res_block,
                                                u1_nnz,
                                                (UWORD8) (15 - u1_offset),
                                                u2_sig_coeff_map,
                                                u4_ctx_idx_offset_abs_lvl,
                                                ps_cabac_ctxt->au1_cabac_ctxt_table
                                                    + SIGNIFICANT_COEFF_FLAG_FRAME
                                                        + u4_ctx_idx_offset_sig_coef,
                                                ps_cabac_ctxt);

                    SETBIT(u1_top_ac_csbp, u1_b2b0);
                    SETBIT(u1_left_ac_csbp, u1_b3b1);
                }
                else
                {
                    CLEARBIT(u1_top_ac_csbp, u1_b2b0);
                    CLEARBIT(u1_left_ac_csbp, u1_b3b1);
                }
            }
        }
        /**************************************************************************/
        /*                   Update the AC csbp                                   */
        /**************************************************************************/
        ps_cabac_ctxt->pu1_left_y_ac_csbp[0] = u1_left_ac_csbp & 0xf;
        u1_top_ac_csbp &= 0x0f;
        ps_curr_ctxt->u1_yuv_ac_csbp &= 0xf0;
        ps_curr_ctxt->u1_yuv_ac_csbp |= u1_top_ac_csbp;
    }
    else
    {
        ps_cabac_ctxt->pu1_left_y_ac_csbp[0] = 0;
        ps_curr_ctxt->u1_yuv_ac_csbp &= 0xf0;
    }

    /*     Write chroma residue */

    ps_ent_ctxt->pv_mb_coeff_data = pv_mb_coeff_data;
    {
        UWORD8 u1_cbp_chroma;
        u1_cbp_chroma = u4_cbp >> 4;
        if (u1_cbp_chroma)
        {
            ih264e_cabac_write_chroma_residue(ps_ent_ctxt, u1_cbp_chroma);
        }
        else
        {
            ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0] &= 0x1;
            ps_curr_ctxt->u1_yuv_dc_csbp &= 0x1;
            ps_cabac_ctxt->pu1_left_uv_ac_csbp[0] = 0;
            ps_curr_ctxt->u1_yuv_ac_csbp &= 0xf;
        }
    }
}

/**
 *******************************************************************************
 * @brief
 * Encodes a Motion vector (9.3.3.1.1.7 )
 *
 * @param[in] u1_mvd
 *  Motion vector to be encoded
 *
 * @param[in] u4_ctx_idx_offset
 * *  ctxIdxOffset for MV_X or MV_Ycontext
 *
 * @param[in]  ui2_abs_mvd
 * sum of absolute value of corresponding neighboring motion vectors
 *
 * @param[in] ps_cabac_ctxt
 *  Pointer to cabac context structure
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_enc_ctx_mvd(WORD16 u1_mvd, UWORD32 u4_ctx_idx_offset,
                                     UWORD16 ui2_abs_mvd,
                                     cabac_ctxt_t *ps_cabac_ctxt)
{

    UWORD8  u1_bin, u1_ctxt_inc;
    WORD8 k = 3, u1_coff = 9;
    WORD16 i2_abs_mvd, i2_sufs;
    UWORD32 u4_ctx_inc;
    UWORD32 u4_bins;
    WORD8 i1_bins_len;

    /* if mvd < u1_coff
     only Prefix
     else
     Prefix + Suffix

     encode sign bit

     Prefix TU encoding Cmax =u1_coff and Suffix 3rd order Exp-Golomb
     */

    if (ui2_abs_mvd < 3)
        u4_ctx_inc = 0;
    else if (ui2_abs_mvd > 32)
        u4_ctx_inc = 2;
    else
        u4_ctx_inc = 1;

    u4_bins = 0;
    i1_bins_len = 1;

    if (u1_mvd == 0)
    {
        ih264e_cabac_encode_bin(ps_cabac_ctxt,
                                0,
                                ps_cabac_ctxt->au1_cabac_ctxt_table + u4_ctx_idx_offset
                                        + u4_ctx_inc);
    }
    else
    {
        i2_abs_mvd = ABS(u1_mvd);
        if (i2_abs_mvd >= u1_coff)
        {
            /* Prefix TU i.e string of 9 1's */
            u4_bins = 0x1ff;
            i1_bins_len = 9;
            u4_ctx_inc = (u4_ctx_inc | 0x065430);

            ih264e_encode_decision_bins(u4_bins,
                                        i1_bins_len,
                                        u4_ctx_inc,
                                        4,
                                        ps_cabac_ctxt->au1_cabac_ctxt_table
                                            + u4_ctx_idx_offset,
                                        ps_cabac_ctxt);

            /* Suffix, uses EncodeBypass */
            u4_bins = 0;
            i1_bins_len = 0;
            i2_sufs = i2_abs_mvd - u1_coff;
            while (1)
            {
                if (i2_sufs >= (1 << k))
                {
                    u4_bins = (u4_bins | (1 << (31 - i1_bins_len)));
                    i1_bins_len++;
                    i2_sufs = i2_sufs - (1 << k);
                    k++;
                }
                else
                {
                    i1_bins_len++;
                    while (k--)
                    {
                        u1_bin = ((i2_sufs >> k) & 0x01);
                        u4_bins = (u4_bins | (u1_bin << (31 - i1_bins_len)));
                        i1_bins_len++;
                    }
                    break;
                }
            }
            u4_bins >>= (32 - i1_bins_len);
            ih264e_cabac_encode_bypass_bins(ps_cabac_ctxt, u4_bins,
                                            i1_bins_len);
        }
        else
        {
            /* Prefix only */
            /* b0 */
            u4_bins = 1;
            i2_abs_mvd--;
            u1_ctxt_inc = 3;
            while (i2_abs_mvd)
            {
                i2_abs_mvd--;
                u4_bins = (u4_bins | (1 << i1_bins_len));
                if (u1_ctxt_inc <= 6)
                {
                    u4_ctx_inc = (u4_ctx_inc
                                    | (u1_ctxt_inc << (i1_bins_len << 2)));
                    u1_ctxt_inc++;
                }
                i1_bins_len++;
            }
            /* Encode Terminating bit */
            if (i1_bins_len <= 4)
                u4_ctx_inc = (u4_ctx_inc | (u1_ctxt_inc << (i1_bins_len << 2)));
            i1_bins_len++;
            ih264e_encode_decision_bins(u4_bins,
                                        i1_bins_len,
                                        u4_ctx_inc,
                                        4,
                                        ps_cabac_ctxt->au1_cabac_ctxt_table
                                            + u4_ctx_idx_offset,
                                        ps_cabac_ctxt);
        }
        /* sign bit, uses EncodeBypass */
        if (u1_mvd > 0)
            ih264e_cabac_encode_bypass_bin(ps_cabac_ctxt, 0);
        else
            ih264e_cabac_encode_bypass_bin(ps_cabac_ctxt, 1);
    }
}

/**
 *******************************************************************************
 * @brief
 * Encodes all motion vectors for a P16x16 MB
 *
 * @param[in] ps_cabac_ctxt
 *  Pointer to cabac context structure
 *
 * @param[in] pi2_mv_ptr
 * Pointer to array of motion vectors
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_enc_mvds_p16x16(cabac_ctxt_t *ps_cabac_ctxt,
                                         WORD16 *pi2_mv_ptr)
{


    /* Encode the differential component of the motion vectors */

    {
        UWORD8 u1_abs_mvd_x, u1_abs_mvd_y;
        UWORD8 *pu1_top_mv_ctxt, *pu1_lft_mv_ctxt;
        WORD16 u2_mv;
        u1_abs_mvd_x = 0;
        u1_abs_mvd_y = 0;
        pu1_top_mv_ctxt = ps_cabac_ctxt->ps_curr_ctxt_mb_info->u1_mv[0];
        pu1_lft_mv_ctxt = ps_cabac_ctxt->pu1_left_mv_ctxt_inc[0];
        {
            UWORD16 u2_abs_mvd_x_a, u2_abs_mvd_x_b, u2_abs_mvd_y_a,
                            u2_abs_mvd_y_b;
            u2_abs_mvd_x_b = (UWORD16) pu1_top_mv_ctxt[0];
            u2_abs_mvd_y_b = (UWORD16) pu1_top_mv_ctxt[1];
            u2_abs_mvd_x_a = (UWORD16) pu1_lft_mv_ctxt[0];
            u2_abs_mvd_y_a = (UWORD16) pu1_lft_mv_ctxt[1];
            u2_mv = *(pi2_mv_ptr++);

            ih264e_cabac_enc_ctx_mvd(u2_mv, MVD_X,
                                    (UWORD16) (u2_abs_mvd_x_a + u2_abs_mvd_x_b),
                                    ps_cabac_ctxt);

            u1_abs_mvd_x = CLIP3(0, 127, ABS(u2_mv));
            u2_mv = *(pi2_mv_ptr++);

            ih264e_cabac_enc_ctx_mvd(u2_mv, MVD_Y,
                                    (UWORD16) (u2_abs_mvd_y_a + u2_abs_mvd_y_b),
                                    ps_cabac_ctxt);

            u1_abs_mvd_y = CLIP3(0, 127, ABS(u2_mv));
        }
        /***************************************************************/
        /* Store abs_mvd_values cabac contexts                         */
        /***************************************************************/
        pu1_top_mv_ctxt[0] = pu1_lft_mv_ctxt[0] = u1_abs_mvd_x;
        pu1_top_mv_ctxt[1] = pu1_lft_mv_ctxt[1] = u1_abs_mvd_y;
    }
}


/**
 *******************************************************************************
 * @brief
 * Encodes all motion vectors for a B MB (Assues that mbype is B_L0_16x16, B_L1_16x16 or B_Bi_16x16
 *
 * @param[in] ps_cabac_ctxt
 *  Pointer to cabac context structure
 *
 * @param[in] pi2_mv_ptr
 * Pointer to array of motion vectors
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
static void ih264e_cabac_enc_mvds_b16x16(cabac_ctxt_t *ps_cabac_ctxt,
                                         WORD16 *pi2_mv_ptr,
                                         WORD32 i4_mb_part_pred_mode )
{

    /* Encode the differential component of the motion vectors */

    {
        UWORD8 u1_abs_mvd_x, u1_abs_mvd_y;
        UWORD8 *pu1_top_mv_ctxt, *pu1_lft_mv_ctxt;
        WORD16 u2_mv;
        u1_abs_mvd_x = 0;
        u1_abs_mvd_y = 0;
        pu1_top_mv_ctxt = ps_cabac_ctxt->ps_curr_ctxt_mb_info->u1_mv[0];
        pu1_lft_mv_ctxt = ps_cabac_ctxt->pu1_left_mv_ctxt_inc[0];
        if (i4_mb_part_pred_mode != PRED_L1)/* || PRED_BI */
        {
            UWORD16 u2_abs_mvd_x_a, u2_abs_mvd_x_b, u2_abs_mvd_y_a,
                            u2_abs_mvd_y_b;
            u2_abs_mvd_x_b = (UWORD16) pu1_top_mv_ctxt[0];
            u2_abs_mvd_y_b = (UWORD16) pu1_top_mv_ctxt[1];
            u2_abs_mvd_x_a = (UWORD16) pu1_lft_mv_ctxt[0];
            u2_abs_mvd_y_a = (UWORD16) pu1_lft_mv_ctxt[1];
            u2_mv = *(pi2_mv_ptr++);

            ih264e_cabac_enc_ctx_mvd(u2_mv, MVD_X,
                                    (UWORD16) (u2_abs_mvd_x_a + u2_abs_mvd_x_b),
                                    ps_cabac_ctxt);

            u1_abs_mvd_x = CLIP3(0, 127, ABS(u2_mv));
            u2_mv = *(pi2_mv_ptr++);

            ih264e_cabac_enc_ctx_mvd(u2_mv, MVD_Y,
                                    (UWORD16) (u2_abs_mvd_y_a + u2_abs_mvd_y_b),
                                    ps_cabac_ctxt);

            u1_abs_mvd_y = CLIP3(0, 127, ABS(u2_mv));
        }
        /***************************************************************/
        /* Store abs_mvd_values cabac contexts                         */
        /***************************************************************/
        pu1_top_mv_ctxt[0] = pu1_lft_mv_ctxt[0] = u1_abs_mvd_x;
        pu1_top_mv_ctxt[1] = pu1_lft_mv_ctxt[1] = u1_abs_mvd_y;

        u1_abs_mvd_x = 0;
        u1_abs_mvd_y = 0;
        if (i4_mb_part_pred_mode != PRED_L0)/* || PRED_BI */
        {
            UWORD16 u2_abs_mvd_x_a, u2_abs_mvd_x_b, u2_abs_mvd_y_a,
                            u2_abs_mvd_y_b;
            u2_abs_mvd_x_b = (UWORD16) pu1_top_mv_ctxt[2];
            u2_abs_mvd_y_b = (UWORD16) pu1_top_mv_ctxt[3];
            u2_abs_mvd_x_a = (UWORD16) pu1_lft_mv_ctxt[2];
            u2_abs_mvd_y_a = (UWORD16) pu1_lft_mv_ctxt[3];
            u2_mv = *(pi2_mv_ptr++);

            ih264e_cabac_enc_ctx_mvd(u2_mv, MVD_X,
                                    (UWORD16) (u2_abs_mvd_x_a + u2_abs_mvd_x_b),
                                    ps_cabac_ctxt);

            u1_abs_mvd_x = CLIP3(0, 127, ABS(u2_mv));
            u2_mv = *(pi2_mv_ptr++);

            ih264e_cabac_enc_ctx_mvd(u2_mv, MVD_Y,
                                    (UWORD16) (u2_abs_mvd_y_a + u2_abs_mvd_y_b),
                                    ps_cabac_ctxt);

            u1_abs_mvd_y = CLIP3(0, 127, ABS(u2_mv));
        }
        /***************************************************************/
        /* Store abs_mvd_values cabac contexts                         */
        /***************************************************************/
        pu1_top_mv_ctxt[2] = pu1_lft_mv_ctxt[2] = u1_abs_mvd_x;
        pu1_top_mv_ctxt[3] = pu1_lft_mv_ctxt[3] = u1_abs_mvd_y;
    }
}



/**
 *******************************************************************************
 *
 * @brief
 *  This function generates CABAC coded bit stream for an Intra Slice.
 *
 * @description
 *  The mb syntax layer for intra slices constitutes luma mb mode, mb qp delta, coded block pattern, chroma mb mode and
 *  luma/chroma residue. These syntax elements are written as directed by table
 *  7.3.5 of h264 specification.
 *
 * @param[in] ps_ent_ctxt
 *  pointer to entropy context
 *
 * @returns error code
 *
 * @remarks none
 *
 *******************************************************************************
 */
IH264E_ERROR_T ih264e_write_islice_mb_cabac(entropy_ctxt_t *ps_ent_ctxt)
{
    /* bit stream ptr */
    bitstrm_t *ps_bitstream = ps_ent_ctxt->ps_bitstrm;
    /* CABAC context */
    cabac_ctxt_t *ps_cabac_ctxt = ps_ent_ctxt->ps_cabac;
    /* packed header data */
    UWORD8 *pu1_byte = ps_ent_ctxt->pv_mb_header_data;
    mb_info_ctxt_t *ps_curr_ctxt;
    WORD32 mb_tpm, mb_type, cbp, chroma_intra_mode, luma_intra_mode;
    WORD8 mb_qp_delta;
    UWORD32 u4_cbp_l, u4_cbp_c;
    WORD32 byte_count = 0;
    WORD32 bitstream_start_offset, bitstream_end_offset;

    if ((ps_bitstream->u4_strm_buf_offset + MIN_STREAM_SIZE_MB)
                    >= ps_bitstream->u4_max_strm_size)
    {
        /* return without corrupting the buffer beyond its size */
        return (IH264E_BITSTREAM_BUFFER_OVERFLOW);
    }
    /* mb header info */
    mb_tpm = *pu1_byte++;
    byte_count++;
    cbp = *pu1_byte++;
    byte_count++;
    mb_qp_delta = *pu1_byte++;
    byte_count++;
    /* mb type */
    mb_type = mb_tpm & 0xF;

    ih264e_get_cabac_context(ps_ent_ctxt, mb_type);
    ps_curr_ctxt = ps_cabac_ctxt->ps_curr_ctxt_mb_info;

    /* Starting bitstream offset for header in bits */
    bitstream_start_offset = GET_NUM_BITS(ps_bitstream);
    u4_cbp_c = (cbp >> 4);
    u4_cbp_l = (cbp & 0xF);
    if (mb_type == I16x16)
    {
        luma_intra_mode = ((mb_tpm >> 4) & 3) + 1 + (u4_cbp_c << 2)
                        + (u4_cbp_l == 15) * 12;
    }
    else
    {
        luma_intra_mode = 0;
    }

    chroma_intra_mode = (mb_tpm >> 6);

    /* Encode Intra pred mode, Luma */
    ih264e_cabac_enc_intra_mb_type(ISLICE, luma_intra_mode, ps_cabac_ctxt,
                                   MB_TYPE_I_SLICE);

    if (mb_type == I4x4)
    {   /* Encode 4x4 MB modes */
        ih264e_cabac_enc_4x4mb_modes(ps_cabac_ctxt, pu1_byte);
        byte_count += 8;
    }
    /* Encode chroma mode */
    ih264e_cabac_enc_chroma_predmode(chroma_intra_mode, ps_cabac_ctxt);

    if (mb_type != I16x16)
    { /* Encode MB cbp */
        ih264e_cabac_enc_cbp(cbp, ps_cabac_ctxt);
    }

    if ((cbp > 0) || (mb_type == I16x16))
    {
        /* Encode mb_qp_delta */
        ih264e_cabac_enc_mb_qp_delta(mb_qp_delta, ps_cabac_ctxt);
        /* Ending bitstream offset for header in bits */
        bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
        ps_ent_ctxt->u4_header_bits[0] += bitstream_end_offset
                        - bitstream_start_offset;
        /* Starting bitstream offset for residue */
        bitstream_start_offset = bitstream_end_offset;
        if (mb_type == I16x16)
        {
            ps_curr_ctxt->u1_mb_type = CAB_I16x16;
            ps_curr_ctxt->u1_cbp = cbp;
            ih264e_cabac_encode_residue_luma_dc(ps_ent_ctxt);
            ih264e_cabac_encode_residue(ps_ent_ctxt, cbp, LUMA_AC_CTXCAT);
        }
        else
        {
            ps_curr_ctxt->u1_cbp = cbp;
            ps_curr_ctxt->u1_mb_type = I4x4;
            ps_curr_ctxt->u1_mb_type = CAB_I4x4;
            ih264e_cabac_encode_residue(ps_ent_ctxt, cbp, LUMA_4X4_CTXCAT);
            ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0] &= 0x6;
            ps_cabac_ctxt->ps_curr_ctxt_mb_info->u1_yuv_dc_csbp &= 0x6;
        }
        /* Ending bitstream offset for reside in bits */
        bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
        ps_ent_ctxt->u4_residue_bits[0] += bitstream_end_offset
                        - bitstream_start_offset;
    }
    else
    {
        ps_curr_ctxt->u1_yuv_ac_csbp = 0;
        ps_curr_ctxt->u1_yuv_dc_csbp = 0;
        *(ps_cabac_ctxt->pu1_left_uv_ac_csbp) = 0;
        *(ps_cabac_ctxt->pu1_left_y_ac_csbp) = 0;
        *(ps_cabac_ctxt->pu1_left_yuv_dc_csbp) = 0;
        /* Ending bitstream offset for header in bits */
        bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
        ps_ent_ctxt->u4_header_bits[0] += bitstream_end_offset
                        - bitstream_start_offset;

        /* Computing the number of used used for encoding the MB syntax */
    }
    memset(ps_curr_ctxt->u1_mv, 0, 16);
    memset(ps_cabac_ctxt->pu1_left_mv_ctxt_inc, 0, 16);
    ps_cabac_ctxt->ps_curr_ctxt_mb_info->u1_cbp = cbp;
    ps_ent_ctxt->pv_mb_header_data = ((WORD8 *)ps_ent_ctxt->pv_mb_header_data) + byte_count;
    if (mb_type == I16x16)
    {
        ps_curr_ctxt->u1_mb_type = CAB_I16x16;

    }
    else
    {
        ps_curr_ctxt->u1_mb_type = CAB_I4x4;

    }
    return IH264E_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @brief
 *  This function generates CABAC coded bit stream for Inter slices
 *
 * @description
 *  The mb syntax layer for inter slices constitutes luma mb mode, mb qp delta, coded block pattern, chroma mb mode and
 *  luma/chroma residue. These syntax elements are written as directed by table
 *  7.3.5 of h264 specification
 *
 * @param[in] ps_ent_ctxt
 *  pointer to entropy context
 *
 * @returns error code
 *
 * @remarks none
 *
 *******************************************************************************
 */
IH264E_ERROR_T ih264e_write_pslice_mb_cabac(entropy_ctxt_t *ps_ent_ctxt)
{
    /* bit stream ptr */
    bitstrm_t *ps_bitstream = ps_ent_ctxt->ps_bitstrm;
    /* CABAC context */
    cabac_ctxt_t *ps_cabac_ctxt = ps_ent_ctxt->ps_cabac;

    mb_info_ctxt_t *ps_curr_ctxt;

    WORD32 bitstream_start_offset, bitstream_end_offset;
    WORD32 mb_tpm, mb_type, cbp, chroma_intra_mode, luma_intra_mode;
    WORD8 mb_qp_delta;
    UWORD32 u4_cbp_l, u4_cbp_c;
    WORD32 byte_count = 0;
    UWORD8 *pu1_byte = ps_ent_ctxt->pv_mb_header_data;

    if ((ps_bitstream->u4_strm_buf_offset + MIN_STREAM_SIZE_MB)
                    >= ps_bitstream->u4_max_strm_size)
    {
        /* return without corrupting the buffer beyond its size */
        return (IH264E_BITSTREAM_BUFFER_OVERFLOW);
    }
    /* mb header info */
    mb_tpm = *pu1_byte++;
    byte_count++;

    /* mb type */
    mb_type = mb_tpm & 0xF;
    /* CABAC contexts for the MB */
    ih264e_get_cabac_context(ps_ent_ctxt, mb_type);
    ps_curr_ctxt = ps_cabac_ctxt->ps_curr_ctxt_mb_info;

    /* if Intra MB */
    if (mb_type == I16x16 || mb_type == I4x4)
    {
        cbp = *pu1_byte++;
        byte_count++;
        mb_qp_delta = *pu1_byte++;
        byte_count++;

        /* Starting bitstream offset for header in bits */
        bitstream_start_offset = GET_NUM_BITS(ps_bitstream);

        /* Encode mb_skip_flag */
        ih264e_cabac_enc_mb_skip(0, ps_cabac_ctxt, MB_SKIP_FLAG_P_SLICE);
        u4_cbp_c = (cbp >> 4);
        u4_cbp_l = (cbp & 0xF);
        if (mb_type == I16x16)
        {
            luma_intra_mode = ((mb_tpm >> 4) & 3) + 1 + (u4_cbp_c << 2)
                            + (u4_cbp_l == 15) * 12;
        }
        else
        {
            luma_intra_mode = 0;
        }
        /* Encode intra mb type */
        {
            ih264e_cabac_encode_bin(ps_cabac_ctxt,
                                    1,
                                    ps_cabac_ctxt->au1_cabac_ctxt_table
                                        + MB_TYPE_P_SLICE);

            ih264e_cabac_enc_intra_mb_type(PSLICE, (UWORD8) luma_intra_mode,
                                           ps_cabac_ctxt, MB_TYPE_P_SLICE);
        }

        if (mb_type == I4x4)
        {   /* Intra 4x4 modes */
            ih264e_cabac_enc_4x4mb_modes(ps_cabac_ctxt, pu1_byte);
            byte_count += 8;
        }
        chroma_intra_mode = (mb_tpm >> 6);

        ih264e_cabac_enc_chroma_predmode(chroma_intra_mode, ps_cabac_ctxt);

        if (mb_type != I16x16)
        {
            /* encode CBP */
            ih264e_cabac_enc_cbp(cbp, ps_cabac_ctxt);
        }

        if ((cbp > 0) || (mb_type == I16x16))
        {
            ih264e_cabac_enc_mb_qp_delta(mb_qp_delta, ps_cabac_ctxt);

            /* Ending bitstream offset for header in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_header_bits[0] += bitstream_end_offset
                            - bitstream_start_offset;
            /* Starting bitstream offset for residue */
            bitstream_start_offset = bitstream_end_offset;

            /* Encoding Residue */
            if (mb_type == I16x16)
            {
                ps_curr_ctxt->u1_mb_type = CAB_I16x16;
                ps_curr_ctxt->u1_cbp = (UWORD8) cbp;
                ih264e_cabac_encode_residue_luma_dc(ps_ent_ctxt);
                ih264e_cabac_encode_residue(ps_ent_ctxt, cbp, LUMA_AC_CTXCAT);
            }
            else
            {
                ps_curr_ctxt->u1_cbp = (UWORD8) cbp;
                ps_curr_ctxt->u1_mb_type = I4x4;
                ps_curr_ctxt->u1_mb_type = CAB_I4x4;
                ih264e_cabac_encode_residue(ps_ent_ctxt, cbp, LUMA_4X4_CTXCAT);
                ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0] &= 0x6;
                ps_cabac_ctxt->ps_curr_ctxt_mb_info->u1_yuv_dc_csbp &= 0x6;
            }

            /* Ending bitstream offset for reside in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_residue_bits[0] += bitstream_end_offset
                            - bitstream_start_offset;
        }
        else
        {
            ps_curr_ctxt->u1_yuv_ac_csbp = 0;
            ps_curr_ctxt->u1_yuv_dc_csbp = 0;
            *(ps_cabac_ctxt->pu1_left_uv_ac_csbp) = 0;
            *(ps_cabac_ctxt->pu1_left_y_ac_csbp) = 0;
            *(ps_cabac_ctxt->pu1_left_yuv_dc_csbp) = 0;
            /* Ending bitstream offset for header in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_header_bits[0] += bitstream_end_offset
                            - bitstream_start_offset;
        }

        memset(ps_curr_ctxt->u1_mv, 0, 16);
        memset(ps_cabac_ctxt->pu1_left_mv_ctxt_inc, 0, 16);
        ps_cabac_ctxt->ps_curr_ctxt_mb_info->u1_cbp = (UWORD8) cbp;

        if (mb_type == I16x16)
        {
            ps_curr_ctxt->u1_mb_type = CAB_I16x16;
        }
        else
        {
            ps_curr_ctxt->u1_mb_type = CAB_I4x4;
        }

        ps_ent_ctxt->pv_mb_header_data = ((WORD8 *)ps_ent_ctxt->pv_mb_header_data) + byte_count;

        return IH264E_SUCCESS;
    }
    else /* Inter MB */
    {
        /* Starting bitstream offset for header in bits */
        bitstream_start_offset = GET_NUM_BITS(ps_bitstream);
        /* Encoding P16x16 */
        if (mb_type != PSKIP)
        {
            cbp = *pu1_byte++;
            byte_count++;
            mb_qp_delta = *pu1_byte++;
            byte_count++;

            /* Encoding mb_skip */
            ih264e_cabac_enc_mb_skip(0, ps_cabac_ctxt, MB_SKIP_FLAG_P_SLICE);

            /* Encoding mb_type as P16x16 */
            {
                UWORD32 u4_ctx_inc_p;
                u4_ctx_inc_p = (0x010 + ((2) << 8));

                ih264e_encode_decision_bins(0, 3, u4_ctx_inc_p, 3,
                                            &(ps_cabac_ctxt->au1_cabac_ctxt_table[MB_TYPE_P_SLICE]),
                                            ps_cabac_ctxt);
            }
            ps_curr_ctxt->u1_mb_type = CAB_P;
            {
                WORD16 *pi2_mv_ptr = (WORD16 *) pu1_byte;
                byte_count += 4;
                ps_curr_ctxt->u1_mb_type = (ps_curr_ctxt->u1_mb_type
                                            | CAB_NON_BD16x16);
                 /* Encoding motion vector for P16x16 */
                ih264e_cabac_enc_mvds_p16x16(ps_cabac_ctxt, pi2_mv_ptr);
            }
            /* Encode CBP */
            ih264e_cabac_enc_cbp(cbp, ps_cabac_ctxt);

            if (cbp)
            {
                /* encode mb_qp_delta */
                ih264e_cabac_enc_mb_qp_delta(mb_qp_delta, ps_cabac_ctxt);
            }

            /* Ending bitstream offset for header in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_header_bits[1] += bitstream_end_offset
                            - bitstream_start_offset;
            /* Starting bitstream offset for residue */
            bitstream_start_offset = bitstream_end_offset;

        }
        else/* MB = PSKIP */
        {
            ih264e_cabac_enc_mb_skip(1, ps_cabac_ctxt, MB_SKIP_FLAG_P_SLICE);

            ps_curr_ctxt->u1_mb_type = CAB_P_SKIP;
            (*ps_ent_ctxt->pi4_mb_skip_run)++;

            memset(ps_curr_ctxt->u1_mv, 0, 16);
            memset(ps_cabac_ctxt->pu1_left_mv_ctxt_inc, 0, 16);
            cbp = 0;

            /* Ending bitstream offset for header in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_header_bits[1] += bitstream_end_offset
                            - bitstream_start_offset;
            /* Starting bitstream offset for residue */

        }

        if (cbp > 0)
        {
            /* Encode residue */
            ih264e_cabac_encode_residue(ps_ent_ctxt, cbp, LUMA_4X4_CTXCAT);
            /* Ending bitstream offset for reside in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_residue_bits[1] += bitstream_end_offset
                            - bitstream_start_offset;

            ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0] &= 0x6;
            ps_curr_ctxt->u1_yuv_dc_csbp &= 0x6;
        }
        else
        {
            ps_curr_ctxt->u1_yuv_ac_csbp = 0;
            ps_curr_ctxt->u1_yuv_dc_csbp = 0;
            *(ps_cabac_ctxt->pu1_left_uv_ac_csbp) = 0;
            *(ps_cabac_ctxt->pu1_left_y_ac_csbp) = 0;
            *(ps_cabac_ctxt->pu1_left_yuv_dc_csbp) = 0;
        }
        ps_curr_ctxt->u1_intrapred_chroma_mode = 0;
        ps_curr_ctxt->u1_cbp = cbp;
        ps_ent_ctxt->pv_mb_header_data = ((WORD8 *)ps_ent_ctxt->pv_mb_header_data) + byte_count;
        return IH264E_SUCCESS;
    }
}


/* ! < Table 9-37 – Binarization for macroblock types in B slices  in ITU_T_H264-201402
 * Bits 0-7 : binarised value
 * Bits 8-15: length of binary sequence */


static const UWORD32 u4_b_mb_type[27] = { 0x0100, 0x0301, 0x0305, 0x0603,
                                          0x0623, 0x0613, 0x0633, 0x060b,
                                          0x062b, 0x061b, 0x063b, 0x061f,
                                          0x0707, 0x0747, 0x0727, 0x0767,
                                          0x0717, 0x0757, 0x0737, 0x0777,
                                          0x070f, 0x074f, 0x063f };
/* CtxInc for mb types in B slices */
static const UWORD32 ui_b_mb_type_ctx_inc[27] = { 0x00, 0x0530, 0x0530,
                                                  0x0555430, 0x0555430,
                                                  0x0555430, 0x0555430,
                                                  0x0555430, 0x0555430,
                                                  0x0555430, 0x0555430,
                                                  0x0555430, 0x05555430,
                                                  0x05555430, 0x05555430,
                                                  0x05555430, 0x05555430,
                                                  0x05555430, 0x05555430,
                                                  0x05555430, 0x05555430,
                                                  0x05555430, 0x0555430 };

/**
 *******************************************************************************
 *
 * @brief
 *  This function generates CABAC coded bit stream for B slices
 *
 * @description
 *  The mb syntax layer for inter slices constitutes luma mb mode,
 *  mb qp delta, coded block pattern, chroma mb mode and
 *  luma/chroma residue. These syntax elements are written as directed by table
 *  7.3.5 of h264 specification
 *
 * @param[in] ps_ent_ctxt
 *  pointer to entropy context
 *
 * @returns error code
 *
 * @remarks none
 *
 *******************************************************************************
 */
IH264E_ERROR_T ih264e_write_bslice_mb_cabac(entropy_ctxt_t *ps_ent_ctxt)
{
    /* bit stream ptr */
    bitstrm_t *ps_bitstream = ps_ent_ctxt->ps_bitstrm;
    /* CABAC context */
    cabac_ctxt_t *ps_cabac_ctxt = ps_ent_ctxt->ps_cabac;

    mb_info_ctxt_t *ps_curr_ctxt;

    WORD32 bitstream_start_offset, bitstream_end_offset;
    WORD32 mb_tpm, mb_type, cbp, chroma_intra_mode, luma_intra_mode;
    WORD8 mb_qp_delta;
    UWORD32 u4_cbp_l, u4_cbp_c;
    WORD32 byte_count = 0;
    UWORD8 *pu1_byte = ps_ent_ctxt->pv_mb_header_data;

    if ((ps_bitstream->u4_strm_buf_offset + MIN_STREAM_SIZE_MB)
                    >= ps_bitstream->u4_max_strm_size)
    {
        /* return without corrupting the buffer beyond its size */
        return (IH264E_BITSTREAM_BUFFER_OVERFLOW);
    }
    /* mb header info */
    mb_tpm = *pu1_byte++;
    byte_count++;

    /* mb type */
    mb_type = mb_tpm & 0xF;
    /* CABAC contexts for the MB */
    ih264e_get_cabac_context(ps_ent_ctxt, mb_type);
    ps_curr_ctxt = ps_cabac_ctxt->ps_curr_ctxt_mb_info;

    /* if Intra MB */
    if (mb_type == I16x16 || mb_type == I4x4)
    {
        cbp = *pu1_byte++;
        byte_count++;
        mb_qp_delta = *pu1_byte++;
        byte_count++;

        /* Starting bitstream offset for header in bits */
        bitstream_start_offset = GET_NUM_BITS(ps_bitstream);

        /* Encode mb_skip_flag */
        ih264e_cabac_enc_mb_skip(0, ps_cabac_ctxt, MB_SKIP_FLAG_B_SLICE);
        u4_cbp_c = (cbp >> 4);
        u4_cbp_l = (cbp & 0xF);
        if (mb_type == I16x16)
        {
            luma_intra_mode = ((mb_tpm >> 4) & 3) + 1 + (u4_cbp_c << 2)
                            + (u4_cbp_l == 15) * 12;
        }
        else
        {
            luma_intra_mode = 0;
        }
        /* Encode intra mb type */
        {
            mb_info_ctxt_t *ps_left_ctxt = ps_cabac_ctxt->ps_left_ctxt_mb_info;
            mb_info_ctxt_t *ps_top_ctxt = ps_cabac_ctxt->ps_top_ctxt_mb_info;
            UWORD32 u4_ctx_inc = 0;

            if (ps_left_ctxt != ps_cabac_ctxt->ps_def_ctxt_mb_info)
                u4_ctx_inc += ((ps_left_ctxt->u1_mb_type & CAB_BD16x16_MASK)
                                != CAB_BD16x16) ? 1 : 0;
            if (ps_top_ctxt != ps_cabac_ctxt->ps_def_ctxt_mb_info)
                u4_ctx_inc += ((ps_top_ctxt->u1_mb_type & CAB_BD16x16_MASK)
                                != CAB_BD16x16) ? 1 : 0;

            /* Intra Prefix Only "111101" */
            u4_ctx_inc = (u4_ctx_inc | 0x05555430);
            ih264e_encode_decision_bins(0x2f,
                                        6,
                                        u4_ctx_inc,
                                        3,
                                        ps_cabac_ctxt->au1_cabac_ctxt_table
                                            + MB_TYPE_B_SLICE,
                                        ps_cabac_ctxt);

            ih264e_cabac_enc_intra_mb_type(BSLICE, (UWORD8) luma_intra_mode,
                                           ps_cabac_ctxt, MB_TYPE_B_SLICE);

        }

        if (mb_type == I4x4)
        { /* Intra 4x4 modes */
            ih264e_cabac_enc_4x4mb_modes(ps_cabac_ctxt, pu1_byte);
            byte_count += 8;
        }
        chroma_intra_mode = (mb_tpm >> 6);

        ih264e_cabac_enc_chroma_predmode(chroma_intra_mode, ps_cabac_ctxt);

        if (mb_type != I16x16)
        {
            /* encode CBP */
            ih264e_cabac_enc_cbp(cbp, ps_cabac_ctxt);
        }

        if ((cbp > 0) || (mb_type == I16x16))
        {
            ih264e_cabac_enc_mb_qp_delta(mb_qp_delta, ps_cabac_ctxt);

            /* Ending bitstream offset for header in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_header_bits[0] += bitstream_end_offset
                            - bitstream_start_offset;
            /* Starting bitstream offset for residue */
            bitstream_start_offset = bitstream_end_offset;

            /* Encoding Residue */
            if (mb_type == I16x16)
            {
                ps_curr_ctxt->u1_mb_type = CAB_I16x16;
                ps_curr_ctxt->u1_cbp = (UWORD8) cbp;
                ih264e_cabac_encode_residue_luma_dc(ps_ent_ctxt);
                ih264e_cabac_encode_residue(ps_ent_ctxt, cbp, LUMA_AC_CTXCAT);
            }
            else
            {
                ps_curr_ctxt->u1_cbp = (UWORD8) cbp;
                ps_curr_ctxt->u1_mb_type = I4x4;
                ps_curr_ctxt->u1_mb_type = CAB_I4x4;
                ih264e_cabac_encode_residue(ps_ent_ctxt, cbp, LUMA_4X4_CTXCAT);
                ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0] &= 0x6;
                ps_cabac_ctxt->ps_curr_ctxt_mb_info->u1_yuv_dc_csbp &= 0x6;
            }

            /* Ending bitstream offset for reside in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_residue_bits[0] += bitstream_end_offset
                            - bitstream_start_offset;
        }
        else
        {
            ps_curr_ctxt->u1_yuv_ac_csbp = 0;
            ps_curr_ctxt->u1_yuv_dc_csbp = 0;
            *(ps_cabac_ctxt->pu1_left_uv_ac_csbp) = 0;
            *(ps_cabac_ctxt->pu1_left_y_ac_csbp) = 0;
            *(ps_cabac_ctxt->pu1_left_yuv_dc_csbp) = 0;
            /* Ending bitstream offset for header in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_header_bits[0] += bitstream_end_offset
                            - bitstream_start_offset;
        }

        memset(ps_curr_ctxt->u1_mv, 0, 16);
        memset(ps_cabac_ctxt->pu1_left_mv_ctxt_inc, 0, 16);
        ps_cabac_ctxt->ps_curr_ctxt_mb_info->u1_cbp = (UWORD8) cbp;

        if (mb_type == I16x16)
        {
            ps_curr_ctxt->u1_mb_type = CAB_I16x16;
        }
        else
        {
            ps_curr_ctxt->u1_mb_type = CAB_I4x4;
        }

        ps_ent_ctxt->pv_mb_header_data = ((WORD8 *)ps_ent_ctxt->pv_mb_header_data) + byte_count;

        return IH264E_SUCCESS;
    }

    else /* Inter MB */
    {
        /* Starting bitstream offset for header in bits */
        bitstream_start_offset = GET_NUM_BITS(ps_bitstream);
        /* Encoding B_Direct_16x16 */
        if (mb_type == BDIRECT)
        {
            cbp = *pu1_byte++;
            byte_count++;
            mb_qp_delta = *pu1_byte++;
            byte_count++;

            /* Encoding mb_skip */
            ih264e_cabac_enc_mb_skip(0, ps_cabac_ctxt, MB_SKIP_FLAG_B_SLICE);

            /* Encoding mb_type as B_Direct_16x16 */
            {

                mb_info_ctxt_t *ps_left_ctxt =
                                ps_cabac_ctxt->ps_left_ctxt_mb_info;
                mb_info_ctxt_t *ps_top_ctxt = ps_cabac_ctxt->ps_top_ctxt_mb_info;
                UWORD32 u4_ctx_inc = 0;

                if (ps_left_ctxt != ps_cabac_ctxt->ps_def_ctxt_mb_info)
                    u4_ctx_inc += ((ps_left_ctxt->u1_mb_type & CAB_BD16x16_MASK)
                                    != CAB_BD16x16) ? 1 : 0;
                if (ps_top_ctxt != ps_cabac_ctxt->ps_def_ctxt_mb_info)
                    u4_ctx_inc += ((ps_top_ctxt->u1_mb_type & CAB_BD16x16_MASK)
                                    != CAB_BD16x16) ? 1 : 0;
                /* Encode the bin */
                ih264e_cabac_encode_bin(
                                ps_cabac_ctxt,
                                0,
                                ps_cabac_ctxt->au1_cabac_ctxt_table
                                                + MB_TYPE_B_SLICE + u4_ctx_inc);

            }
            ps_curr_ctxt->u1_mb_type = CAB_BD16x16;
            memset(ps_curr_ctxt->u1_mv, 0, 16);
            memset(ps_cabac_ctxt->pu1_left_mv_ctxt_inc, 0, 16);

            /* Encode CBP */
            ih264e_cabac_enc_cbp(cbp, ps_cabac_ctxt);

            if (cbp)
            {
                /* encode mb_qp_delta */
                ih264e_cabac_enc_mb_qp_delta(mb_qp_delta, ps_cabac_ctxt);
            }

            /* Ending bitstream offset for header in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_header_bits[1] += bitstream_end_offset
                            - bitstream_start_offset;
            /* Starting bitstream offset for residue */
            bitstream_start_offset = bitstream_end_offset;
            /* Starting bitstream offset for residue */

        }

        else if (mb_type == BSKIP)/* MB = BSKIP */
        {
            ih264e_cabac_enc_mb_skip(1, ps_cabac_ctxt, MB_SKIP_FLAG_B_SLICE);

            ps_curr_ctxt->u1_mb_type = CAB_B_SKIP;

            memset(ps_curr_ctxt->u1_mv, 0, 16);
            memset(ps_cabac_ctxt->pu1_left_mv_ctxt_inc, 0, 16);
            cbp = 0;

            /* Ending bitstream offset for header in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_header_bits[1] += bitstream_end_offset
                            - bitstream_start_offset;
            /* Starting bitstream offset for residue */

        }

        else /* mbype is B_L0_16x16, B_L1_16x16 or B_Bi_16x16 */
        {
            WORD32 i4_mb_part_pred_mode = (mb_tpm >> 4);
            UWORD32 u4_mb_type = mb_type - B16x16 + B_L0_16x16
                            + i4_mb_part_pred_mode;
            cbp = *pu1_byte++;
            byte_count++;
            mb_qp_delta = *pu1_byte++;
            byte_count++;

            /* Encoding mb_skip */
            ih264e_cabac_enc_mb_skip(0, ps_cabac_ctxt, MB_SKIP_FLAG_B_SLICE);

            /* Encoding mb_type as B16x16 */
            {
                mb_info_ctxt_t *ps_left_ctxt =
                                ps_cabac_ctxt->ps_left_ctxt_mb_info;
                mb_info_ctxt_t *ps_top_ctxt = ps_cabac_ctxt->ps_top_ctxt_mb_info;
                UWORD32 u4_ctx_inc = 0;

                UWORD32 u4_mb_type_bins = u4_b_mb_type[u4_mb_type];
                UWORD32 u4_bin_len = (u4_mb_type_bins >> 8) & 0x0F;
                u4_mb_type_bins = u4_mb_type_bins & 0xFF;

                if (ps_left_ctxt != ps_cabac_ctxt->ps_def_ctxt_mb_info)
                    u4_ctx_inc += ((ps_left_ctxt->u1_mb_type & CAB_BD16x16_MASK)
                                    != CAB_BD16x16) ? 1 : 0;
                if (ps_top_ctxt != ps_cabac_ctxt->ps_def_ctxt_mb_info)
                    u4_ctx_inc += ((ps_top_ctxt->u1_mb_type & CAB_BD16x16_MASK)
                                    != CAB_BD16x16) ? 1 : 0;

                u4_ctx_inc = u4_ctx_inc | ui_b_mb_type_ctx_inc[u4_mb_type];

                ih264e_encode_decision_bins(u4_mb_type_bins,
                                            u4_bin_len,
                                            u4_ctx_inc,
                                            u4_bin_len,
                                            &(ps_cabac_ctxt->au1_cabac_ctxt_table[MB_TYPE_B_SLICE]),
                                            ps_cabac_ctxt);
            }

            ps_curr_ctxt->u1_mb_type = CAB_NON_BD16x16;
            {
                WORD16 *pi2_mv_ptr = (WORD16 *) pu1_byte;
                /* Get the pred modes */

                byte_count += 4 * (1 + (i4_mb_part_pred_mode == PRED_BI));

                ps_curr_ctxt->u1_mb_type = (ps_curr_ctxt->u1_mb_type
                                | CAB_NON_BD16x16);
                /* Encoding motion vector for B16x16 */
                ih264e_cabac_enc_mvds_b16x16(ps_cabac_ctxt, pi2_mv_ptr,
                                             i4_mb_part_pred_mode);
            }
            /* Encode CBP */
            ih264e_cabac_enc_cbp(cbp, ps_cabac_ctxt);

            if (cbp)
            {
                /* encode mb_qp_delta */
                ih264e_cabac_enc_mb_qp_delta(mb_qp_delta, ps_cabac_ctxt);
            }

            /* Ending bitstream offset for header in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_header_bits[1] += bitstream_end_offset
                            - bitstream_start_offset;
            /* Starting bitstream offset for residue */
            bitstream_start_offset = bitstream_end_offset;
        }

        if (cbp > 0)
        {
            /* Encode residue */
            ih264e_cabac_encode_residue(ps_ent_ctxt, cbp, LUMA_4X4_CTXCAT);
            /* Ending bitstream offset for reside in bits */
            bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
            ps_ent_ctxt->u4_residue_bits[1] += bitstream_end_offset
                            - bitstream_start_offset;

            ps_cabac_ctxt->pu1_left_yuv_dc_csbp[0] &= 0x6;
            ps_curr_ctxt->u1_yuv_dc_csbp &= 0x6;
        }
        else
        {
            ps_curr_ctxt->u1_yuv_ac_csbp = 0;
            ps_curr_ctxt->u1_yuv_dc_csbp = 0;
            *(ps_cabac_ctxt->pu1_left_uv_ac_csbp) = 0;
            *(ps_cabac_ctxt->pu1_left_y_ac_csbp) = 0;
            *(ps_cabac_ctxt->pu1_left_yuv_dc_csbp) = 0;
        }
        ps_curr_ctxt->u1_intrapred_chroma_mode = 0;
        ps_curr_ctxt->u1_cbp = cbp;
        ps_ent_ctxt->pv_mb_header_data = ((WORD8 *)ps_ent_ctxt->pv_mb_header_data) + byte_count;
        return IH264E_SUCCESS;
    }
}
