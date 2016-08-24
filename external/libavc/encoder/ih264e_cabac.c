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
*  Contains all leaf level functions for CABAC entropy coding.
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
#include "ih264e_statistics.h"
#include "ih264e_trace.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/


/**
 *******************************************************************************
 *
 * @brief
 *  k-th order Exp-Golomb (UEGk) binarization process: Implements concatenated
 *   unary/ k-th order Exp-Golomb  (UEGk) binarization process,
 *   where k = 0 as defined in 9.3.2.3 of  ITU_T_H264-201402
 *
 * @param[in] i2_sufs
 *  Suffix bit string
 *
 * @param[in] pi1_bins_len
 *  Pointer to length of tthe string
 *
 * @returns Binarized value
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */

UWORD32 ih264e_cabac_UEGk0_binarization(WORD16 i2_sufs, WORD8 *pi1_bins_len)
{
    WORD32 unary_length;
    UWORD32 u4_sufs_shiftk_plus1, u4_egk, u4_unary_bins;

    u4_sufs_shiftk_plus1 = i2_sufs + 1;

    unary_length = (32 - CLZ(u4_sufs_shiftk_plus1) + (0 == u4_sufs_shiftk_plus1));

    /* unary code with (unary_length-1) '1's and terminating '0' bin */
    u4_unary_bins = (1 << unary_length) - 2;

    /* insert the symbol prefix of (unary length - 1)  bins */
    u4_egk = (u4_unary_bins << (unary_length - 1))
                    | (u4_sufs_shiftk_plus1 & ((1 << (unary_length - 1)) - 1));

    /* length of the code = 2 *(unary_length - 1) + 1 + k */
    *pi1_bins_len = (2 * unary_length) - 1;

    return (u4_egk);
}

/**
 *******************************************************************************
 *
 * @brief
 *  Get cabac context for the MB :calculates the pointers to Top and   left
 *          cabac neighbor context depending upon neighbor  availability.
 *
 * @param[in] ps_ent_ctxt
 *  Pointer to entropy context structure
 *
 * @param[in] u4_mb_type
 *  Type of MB
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
void ih264e_get_cabac_context(entropy_ctxt_t *ps_ent_ctxt, WORD32 u4_mb_type)
{

    /* CABAC context */
    cabac_ctxt_t *ps_cabac_ctxt = ps_ent_ctxt->ps_cabac;
    mb_info_ctxt_t *ps_ctx_inc_mb_map;
    cab_csbp_t *ps_lft_csbp;

    WORD32 i4_lft_avail, i4_top_avail, i4_is_intra;
    WORD32 i4_mb_x, i4_mb_y;
    UWORD8 *pu1_slice_idx = ps_ent_ctxt->pu1_slice_idx;

    i4_is_intra = ((u4_mb_type == I16x16) || (u4_mb_type == I8x8)
                    || (u4_mb_type == I4x4));

    /* derive neighbor availability */
    i4_mb_x = ps_ent_ctxt->i4_mb_x;
    i4_mb_y = ps_ent_ctxt->i4_mb_y;
    pu1_slice_idx += (i4_mb_y * ps_ent_ctxt->i4_wd_mbs);
    /* left macroblock availability */
    i4_lft_avail = (i4_mb_x == 0
                    || (pu1_slice_idx[i4_mb_x - 1] != pu1_slice_idx[i4_mb_x])) ?
                    0 : 1;
    /* top macroblock availability */
    i4_top_avail = (i4_mb_y == 0
                    || (pu1_slice_idx[i4_mb_x - ps_ent_ctxt->i4_wd_mbs]
                                    != pu1_slice_idx[i4_mb_x])) ? 0 : 1;
    i4_mb_x = ps_ent_ctxt->i4_mb_x;
    ps_ctx_inc_mb_map = ps_cabac_ctxt->ps_mb_map_ctxt_inc;
    ps_cabac_ctxt->ps_curr_ctxt_mb_info = ps_ctx_inc_mb_map + i4_mb_x;
    ps_cabac_ctxt->ps_left_ctxt_mb_info = ps_cabac_ctxt->ps_def_ctxt_mb_info;
    ps_cabac_ctxt->ps_top_ctxt_mb_info = ps_cabac_ctxt->ps_def_ctxt_mb_info;
    ps_lft_csbp = ps_cabac_ctxt->ps_lft_csbp;
    ps_cabac_ctxt->pu1_left_y_ac_csbp = &ps_lft_csbp->u1_y_ac_csbp_top_mb;
    ps_cabac_ctxt->pu1_left_uv_ac_csbp = &ps_lft_csbp->u1_uv_ac_csbp_top_mb;
    ps_cabac_ctxt->pu1_left_yuv_dc_csbp = &ps_lft_csbp->u1_yuv_dc_csbp_top_mb;
    ps_cabac_ctxt->pi1_left_ref_idx_ctxt_inc =
                    &ps_cabac_ctxt->i1_left_ref_idx_ctx_inc_arr[0][0];
    ps_cabac_ctxt->pu1_left_mv_ctxt_inc =
                    ps_cabac_ctxt->u1_left_mv_ctxt_inc_arr[0];

    if (i4_lft_avail)
        ps_cabac_ctxt->ps_left_ctxt_mb_info =
                        ps_cabac_ctxt->ps_curr_ctxt_mb_info - 1;
    if (i4_top_avail)
        ps_cabac_ctxt->ps_top_ctxt_mb_info =
                        ps_cabac_ctxt->ps_curr_ctxt_mb_info;

    if (!i4_lft_avail)
    {
        UWORD8 u1_def_csbp = i4_is_intra ? 0xf : 0;
        *(ps_cabac_ctxt->pu1_left_y_ac_csbp) = u1_def_csbp;
        *(ps_cabac_ctxt->pu1_left_uv_ac_csbp) = u1_def_csbp;
        *(ps_cabac_ctxt->pu1_left_yuv_dc_csbp) = u1_def_csbp;
        *((UWORD32 *) ps_cabac_ctxt->pi1_left_ref_idx_ctxt_inc) = 0;
        memset(ps_cabac_ctxt->pu1_left_mv_ctxt_inc, 0, 16);
    }
    if (!i4_top_avail)
    {
        UWORD8 u1_def_csbp = i4_is_intra ? 0xff : 0;
        ps_cabac_ctxt->ps_top_ctxt_mb_info->u1_yuv_ac_csbp = u1_def_csbp;
        ps_cabac_ctxt->ps_top_ctxt_mb_info->u1_yuv_dc_csbp = u1_def_csbp;
        ps_cabac_ctxt->ps_curr_ctxt_mb_info->i1_ref_idx[0] =
        ps_cabac_ctxt->ps_curr_ctxt_mb_info->i1_ref_idx[1] =
        ps_cabac_ctxt->ps_curr_ctxt_mb_info->i1_ref_idx[2] =
        ps_cabac_ctxt->ps_curr_ctxt_mb_info->i1_ref_idx[3] = 0;
        memset(ps_cabac_ctxt->ps_curr_ctxt_mb_info->u1_mv, 0, 16);
    }

}



/**
 *******************************************************************************
 * @brief
 *  flushing at termination: Explained in flowchart 9-12(ITU_T_H264-201402).
 *
 *  @param[in]   ps_cabac_ctxt
 *  pointer to cabac context (handle)
 *
 * @returns  none
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
void ih264e_cabac_flush(cabac_ctxt_t *ps_cabac_ctxt)
{

    /* bit stream ptr */
    bitstrm_t *ps_stream = ps_cabac_ctxt->ps_bitstrm;
    encoding_envirnoment_t *ps_cab_enc_env = &(ps_cabac_ctxt->s_cab_enc_env);
    UWORD32 u4_low = ps_cab_enc_env->u4_code_int_low;
    UWORD32 u4_bits_gen = ps_cab_enc_env->u4_bits_gen;
    UWORD8 *pu1_strm_buf = ps_stream->pu1_strm_buffer;
    UWORD32 u4_strm_buf_offset = ps_stream->u4_strm_buf_offset;
    WORD32 zero_run = ps_stream->i4_zero_bytes_run;
    UWORD32 u4_out_standing_bytes = ps_cab_enc_env->u4_out_standing_bytes;

    /************************************************************************/
    /* Insert the carry (propogated in previous byte) along with            */
    /* outstanding bytes (if any) and flush remaining bits                  */
    /************************************************************************/
    {
        /* carry = 1 => putbit(1); carry propogated due to L renorm */
        WORD32 carry = (u4_low >> (u4_bits_gen + CABAC_BITS)) & 0x1;
        WORD32 last_byte;
        WORD32 bits_left;
        WORD32 rem_bits;

        if (carry)
        {
            /* CORNER CASE: if the previous data is 0x000003, then EPB will be inserted
             and the data will become 0x00000303 and if the carry is present, it will
             be added with the last byte and it will become 0x00000304 which is not correct
             as per standard */
            /* so check for previous four bytes and if it is equal to 0x00000303
             then subtract u4_strm_buf_offset by 1 */
            if (pu1_strm_buf[u4_strm_buf_offset - 1] == 0x03
                            && pu1_strm_buf[u4_strm_buf_offset - 2] == 0x03
                            && pu1_strm_buf[u4_strm_buf_offset - 3] == 0x00
                            && pu1_strm_buf[u4_strm_buf_offset - 4] == 0x00)
            {
                u4_strm_buf_offset -= 1;
            }
            /* previous byte carry add will not result in overflow to        */
            /* u4_strm_buf_offset - 2 as we track 0xff as outstanding bytes  */
            pu1_strm_buf[u4_strm_buf_offset - 1] += carry;
            zero_run = 0;
        }

        /*        Insert outstanding bytes (if any)         */
        while (u4_out_standing_bytes)
        {
            UWORD8 u1_0_or_ff = carry ? 0 : 0xFF;

            PUTBYTE_EPB(pu1_strm_buf, u4_strm_buf_offset, u1_0_or_ff, zero_run);
            u4_out_standing_bytes--;
        }

        /*  clear the carry in low */
        u4_low &= ((1 << (u4_bits_gen + CABAC_BITS)) - 1);

        /* extract the remaining bits;                                   */
        /* includes additional msb bit of low as per Figure 9-12      */
        bits_left = u4_bits_gen + 1;
        rem_bits = (u4_low >> (u4_bits_gen + CABAC_BITS - bits_left));

        if (bits_left >= 8)
        {
            last_byte = (rem_bits >> (bits_left - 8)) & 0xFF;
            PUTBYTE_EPB(pu1_strm_buf, u4_strm_buf_offset, last_byte, zero_run);
            bits_left -= 8;
        }

        /* insert last byte along with rbsp stop bit(1) and 0's in the end */
        last_byte = (rem_bits << (8 - bits_left))
                        | (1 << (7 - bits_left) | (1 << (7 - bits_left - 1)));
        last_byte &= 0xFF;
        PUTBYTE_EPB(pu1_strm_buf, u4_strm_buf_offset, last_byte, zero_run);

        /* update the state variables and return success */
        ps_stream->u4_strm_buf_offset = u4_strm_buf_offset;
        ps_stream->i4_zero_bytes_run = 0;
        /* Default init values for scratch variables of bitstream context */
        ps_stream->u4_cur_word = 0;
        ps_stream->i4_bits_left_in_cw = WORD_SIZE;

    }
}

/**
 ******************************************************************************
 *
 *  @brief Puts new byte (and outstanding bytes) into bitstream after cabac
 *         renormalization
 *
 *  @par   Description
 *  1. Extract the leading byte of low(L)
 *  2. If leading byte=0xff increment outstanding bytes and return
 *     (as the actual bits depend on carry propogation later)
 *  3. If leading byte is not 0xff check for any carry propogation
 *  4. Insert the carry (propogated in previous byte) along with outstanding
 *     bytes (if any) and leading byte
 *
 *
 *  @param[in]   ps_cabac_ctxt
 *  pointer to cabac context (handle)
 *
 *  @return
 *
 ******************************************************************************
 */
void ih264e_cabac_put_byte(cabac_ctxt_t *ps_cabac_ctxt)
{

    /* bit stream ptr */
    bitstrm_t *ps_stream = ps_cabac_ctxt->ps_bitstrm;
    encoding_envirnoment_t *ps_cab_enc_env = &(ps_cabac_ctxt->s_cab_enc_env);
    UWORD32 u4_low = ps_cab_enc_env->u4_code_int_low;
    UWORD32 u4_bits_gen = ps_cab_enc_env->u4_bits_gen;
    WORD32 lead_byte = u4_low >> (u4_bits_gen + CABAC_BITS - 8);

    /* Sanity checks */
    ASSERT((ps_cab_enc_env->u4_code_int_range >= 256)
                    && (ps_cab_enc_env->u4_code_int_range < 512));
    ASSERT((u4_bits_gen >= 8));

    /* update bits generated and low after extracting leading byte */
    u4_bits_gen -= 8;
    ps_cab_enc_env->u4_code_int_low &= ((1 << (CABAC_BITS + u4_bits_gen)) - 1);
    ps_cab_enc_env->u4_bits_gen = u4_bits_gen;

    /************************************************************************/
    /* 1. Extract the leading byte of low(L)                                */
    /* 2. If leading byte=0xff increment outstanding bytes and return       */
    /*      (as the actual bits depend on carry propogation later)          */
    /* 3. If leading byte is not 0xff check for any carry propogation       */
    /* 4. Insert the carry (propogated in previous byte) along with         */
    /*    outstanding bytes (if any) and leading byte                       */
    /************************************************************************/
    if (lead_byte == 0xff)
    {
        /* actual bits depend on carry propogration     */
        ps_cab_enc_env->u4_out_standing_bytes++;
        return ;
    }
    else
    {
        /* carry = 1 => putbit(1); carry propogated due to L renorm */
        WORD32 carry = (lead_byte >> 8) & 0x1;
        UWORD8 *pu1_strm_buf = ps_stream->pu1_strm_buffer;
        UWORD32 u4_strm_buf_offset = ps_stream->u4_strm_buf_offset;
        WORD32 zero_run = ps_stream->i4_zero_bytes_run;
        UWORD32 u4_out_standing_bytes = ps_cab_enc_env->u4_out_standing_bytes;


        /*********************************************************************/
        /*        Insert the carry propogated in previous byte               */
        /*                                                                   */
        /* Note : Do not worry about corruption into slice header align byte */
        /*        This is because the first bin cannot result in overflow    */
        /*********************************************************************/
        if (carry)
        {
            /* CORNER CASE: if the previous data is 0x000003, then EPB will be inserted
             and the data will become 0x00000303 and if the carry is present, it will
             be added with the last byte and it will become 0x00000304 which is not correct
             as per standard */
            /* so check for previous four bytes and if it is equal to 0x00000303
             then subtract u4_strm_buf_offset by 1 */
            if (pu1_strm_buf[u4_strm_buf_offset - 1] == 0x03
                            && pu1_strm_buf[u4_strm_buf_offset - 2] == 0x03
                            && pu1_strm_buf[u4_strm_buf_offset - 3] == 0x00
                            && pu1_strm_buf[u4_strm_buf_offset - 4] == 0x00)
            {
                u4_strm_buf_offset -= 1;
            }
            /* previous byte carry add will not result in overflow to        */
            /* u4_strm_buf_offset - 2 as we track 0xff as outstanding bytes  */
            pu1_strm_buf[u4_strm_buf_offset - 1] += carry;
            zero_run = 0;
        }

        /*        Insert outstanding bytes (if any)         */
        while (u4_out_standing_bytes)
        {
            UWORD8 u1_0_or_ff = carry ? 0 : 0xFF;

            PUTBYTE_EPB(pu1_strm_buf, u4_strm_buf_offset, u1_0_or_ff, zero_run);

            u4_out_standing_bytes--;
        }
        ps_cab_enc_env->u4_out_standing_bytes = 0;

        /*        Insert the leading byte                   */
        lead_byte &= 0xFF;
        PUTBYTE_EPB(pu1_strm_buf, u4_strm_buf_offset, lead_byte, zero_run);

        /* update the state variables and return success */
        ps_stream->u4_strm_buf_offset = u4_strm_buf_offset;
        ps_stream->i4_zero_bytes_run = zero_run;

    }
}




 /**
 ******************************************************************************
 *
 *  @brief Codes a bin based on probablilty and mps packed context model
 *
 *  @par   Description
 *  1. Apart from encoding bin, context model is updated as per state transition
 *  2. Range and Low renormalization is done based on bin and original state
 *  3. After renorm bistream is updated (if required)
 *
 *  @param[in]   ps_cabac
 *  pointer to cabac context (handle)
 *
 *  @param[in]   bin
 *  bin(boolean) to be encoded
 *
 *  @param[in]  pu1_bin_ctxts
 *  index of cabac context model containing pState[bits 5-0] | MPS[bit6]
 *
 *  @return
 *
 ******************************************************************************
  */
void ih264e_cabac_encode_bin(cabac_ctxt_t *ps_cabac, WORD32 bin,
                             bin_ctxt_model *pu1_bin_ctxts)
{

    encoding_envirnoment_t *ps_cab_enc_env = &(ps_cabac->s_cab_enc_env);
    UWORD32 u4_range = ps_cab_enc_env->u4_code_int_range;
    UWORD32 u4_low = ps_cab_enc_env->u4_code_int_low;
    UWORD32 u4_rlps;
    UWORD8 state_mps = (*pu1_bin_ctxts) & 0x3F;
    UWORD8 u1_mps = !!((*pu1_bin_ctxts) & (0x40));
    WORD32 shift;
    UWORD32 u4_table_val;
    /* Sanity checks */
    ASSERT((bin == 0) || (bin == 1));
    ASSERT((u4_range >= 256) && (u4_range < 512));

    /* Get the lps range from LUT based on quantized range and state */
    u4_table_val= gau4_ih264_cabac_table[state_mps][(u4_range >> 6) & 0x3];
    u4_rlps = u4_table_val & 0xFF;
    u4_range -= u4_rlps;

    /* check if bin is mps or lps */
    if (u1_mps ^ bin)
    {
        /* lps path;  L= L + R; R = RLPS */
        u4_low += u4_range;
        u4_range = u4_rlps;
        if (state_mps == 0)
        {
            /* MPS(CtxIdx) = 1 - MPS(CtxIdx) */
            u1_mps = 1 - u1_mps;
        } /* update the context model from state transition LUT */

        state_mps =  (u4_table_val >> 15) & 0x3F;
    }
    else
    { /* update the context model from state transition LUT */
        state_mps =  (u4_table_val >> 8) & 0x3F;
    }

    (*pu1_bin_ctxts) = (u1_mps << 6) | state_mps;

        /*****************************************************************/
        /* Renormalization; calculate bits generated based on range(R)   */
        /* Note : 6 <= R < 512; R is 2 only for terminating encode       */
        /*****************************************************************/
        GETRANGE(shift, u4_range);
        shift   = 9 - shift;
        u4_low   <<= shift;
        u4_range <<= shift;

        /* bits to be inserted in the bitstream */
        ps_cab_enc_env->u4_bits_gen += shift;
        ps_cab_enc_env->u4_code_int_range = u4_range;
        ps_cab_enc_env->u4_code_int_low   = u4_low;

        /* generate stream when a byte is ready */
        if (ps_cab_enc_env->u4_bits_gen > CABAC_BITS)
        {
            ih264e_cabac_put_byte(ps_cabac);
        }

}




 /**
 *******************************************************************************
 *
 * @brief
 *  Encoding process for a binary decision :implements encoding process of a decision
 *  as defined in 9.3.4.2 . This function encodes multiple bins, of a symbol. Implements
 *  flowchart Figure 9-7( ITU_T_H264-201402)
 *
 * @param[in] u4_bins
 * array of bin values
 *
 * @param[in] i1_bins_len
 *  Length of bins, maximum 32
 *
 * @param[in] u4_ctx_inc
 *  CtxInc, byte0- bin0, byte1-bin1 ..
 *
 * @param[in] i1_valid_len
 *  valid length of bins, after that CtxInc is constant
 *
 * @param[in] pu1_bin_ctxt_type
 *  Pointer to binary contexts

 * @param[in] ps_cabac
 *  Pointer to cabac_context_structure
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
void ih264e_encode_decision_bins(UWORD32 u4_bins, WORD8 i1_bins_len,
                                 UWORD32 u4_ctx_inc, WORD8 i1_valid_len,
                                 bin_ctxt_model *pu1_bin_ctxt_type,
                                 cabac_ctxt_t *ps_cabac)
{
    WORD8 i;
    UWORD8 u1_ctx_inc, u1_bin;

    for (i = 0; i < i1_bins_len; i++)
    {
        u1_bin = (u4_bins & 0x01);
        u4_bins = u4_bins >> 1;
        u1_ctx_inc = u4_ctx_inc & 0x0f;
        if (i < i1_valid_len)
            u4_ctx_inc = u4_ctx_inc >> 4;
        /* Encode the bin */
        ih264e_cabac_encode_bin(ps_cabac, u1_bin,
                                pu1_bin_ctxt_type + u1_ctx_inc);
    }

}






/**
 *******************************************************************************
 * @brief
 *  Encoding process for a binary decision before termination:Encoding process
 *  of a termination(9.3.4.5 :ITU_T_H264-201402) . Explained in flowchart 9-11.
 *
 * @param[in] ps_cabac
 *  Pointer to cabac structure
 *
 * @param[in] term_bin
 *  Symbol value, end of slice or not, term_bin is binary
 *
 * @returns
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
void ih264e_cabac_encode_terminate(cabac_ctxt_t *ps_cabac, WORD32 term_bin)
{

    encoding_envirnoment_t *ps_cab_enc_env = &(ps_cabac->s_cab_enc_env);

    UWORD32 u4_range = ps_cab_enc_env->u4_code_int_range;
    UWORD32 u4_low = ps_cab_enc_env->u4_code_int_low;
    UWORD32 u4_rlps;
    WORD32 shift;

    /* Sanity checks */
    ASSERT((u4_range >= 256) && (u4_range < 512));
    ASSERT((term_bin == 0) || (term_bin == 1));

    /*  term_bin = 1 has lps range = 2 */
    u4_rlps = 2;
    u4_range -= u4_rlps;

    /* if terminate L is incremented by curR and R=2 */
    if (term_bin)
    {
        /* lps path;  L= L + R; R = RLPS */
        u4_low += u4_range;
        u4_range = u4_rlps;
    }

    /*****************************************************************/
    /* Renormalization; calculate bits generated based on range(R)   */
    /* Note : 6 <= R < 512; R is 2 only for terminating encode       */
    /*****************************************************************/
    GETRANGE(shift, u4_range);
    shift = 9 - shift;
    u4_low <<= shift;
    u4_range <<= shift;

    /* bits to be inserted in the bitstream */
    ps_cab_enc_env->u4_bits_gen += shift;
    ps_cab_enc_env->u4_code_int_range = u4_range;
    ps_cab_enc_env->u4_code_int_low = u4_low;

    /* generate stream when a byte is ready */
    if (ps_cab_enc_env->u4_bits_gen > CABAC_BITS)
    {
        ih264e_cabac_put_byte(ps_cabac);
    }

    if (term_bin)
    {
        ih264e_cabac_flush(ps_cabac);
    }

}


/**
 *******************************************************************************
 * @brief
 * Bypass encoding process for binary decisions:  Explained (9.3.4.4 :ITU_T_H264-201402)
 * , flowchart 9-10.
 *
 *  @param[ino]  ps_cabac : pointer to cabac context (handle)
 *
 *  @param[in]   bin :  bypass bin(0/1) to be encoded
 *
 *  @returns
 *
 *  @remarks
 *  None
 *
 *******************************************************************************
 */

void ih264e_cabac_encode_bypass_bin(cabac_ctxt_t *ps_cabac, WORD32 bin)
{

    encoding_envirnoment_t *ps_cab_enc_env = &(ps_cabac->s_cab_enc_env);

    UWORD32 u4_range = ps_cab_enc_env->u4_code_int_range;
    UWORD32 u4_low = ps_cab_enc_env->u4_code_int_low;

    /* Sanity checks */
    ASSERT((u4_range >= 256) && (u4_range < 512));
    ASSERT((bin == 0) || (bin == 1));

    u4_low <<= 1;
    /* add range if bin is 1 */
    if (bin)
    {
        u4_low += u4_range;
    }

    /* 1 bit to be inserted in the bitstream */
    ps_cab_enc_env->u4_bits_gen++;
    ps_cab_enc_env->u4_code_int_low = u4_low;

    /* generate stream when a byte is ready */
    if (ps_cab_enc_env->u4_bits_gen > CABAC_BITS)
    {
        ih264e_cabac_put_byte(ps_cabac);
    }

}


 /**
 ******************************************************************************
 *
 *  @brief Encodes a series of bypass bins (FLC bypass bins)
 *
 *  @par   Description
 *  This function is more optimal than calling ih264e_cabac_encode_bypass_bin()
 *  in a loop as cabac low, renorm and generating the stream (8bins at a time)
 *  can be done in one operation
 *
 *  @param[inout]ps_cabac
 *   pointer to cabac context (handle)
 *
 *  @param[in]   u4_bins
 *   syntax element to be coded (as FLC bins)
 *
 *  @param[in]   num_bins
 *   This is the FLC length for u4_sym
 *
 *  @return
 *
 ******************************************************************************
 */

void ih264e_cabac_encode_bypass_bins(cabac_ctxt_t *ps_cabac, UWORD32 u4_bins,
                                     WORD32 num_bins)
{

    encoding_envirnoment_t *ps_cab_enc_env = &(ps_cabac->s_cab_enc_env);

    UWORD32 u4_range = ps_cab_enc_env->u4_code_int_range;
    WORD32 next_byte;

    /* Sanity checks */
    ASSERT((num_bins < 33) && (num_bins > 0));
    ASSERT((u4_range >= 256) && (u4_range < 512));

    /* Compute bit always to populate the trace */
    /* increment bits generated by num_bins */

    /* Encode 8bins at a time and put in the bit-stream */
    while (num_bins > 8)
    {
        num_bins -= 8;

        next_byte = (u4_bins >> (num_bins)) & 0xff;

        /*  L = (L << 8) +  (R * next_byte) */
        ps_cab_enc_env->u4_code_int_low <<= 8;
        ps_cab_enc_env->u4_code_int_low += (next_byte * u4_range);
        ps_cab_enc_env->u4_bits_gen += 8;

        if (ps_cab_enc_env->u4_bits_gen > CABAC_BITS)
        {
            /*  insert the leading byte of low into stream */
            ih264e_cabac_put_byte(ps_cabac);
        }
    }

    /* Update low with remaining bins and return */
    next_byte = (u4_bins & ((1 << num_bins) - 1));

    ps_cab_enc_env->u4_code_int_low <<= num_bins;
    ps_cab_enc_env->u4_code_int_low += (next_byte * u4_range);
    ps_cab_enc_env->u4_bits_gen += num_bins;

    if (ps_cab_enc_env->u4_bits_gen > CABAC_BITS)
    {
        /*  insert the leading byte of low into stream */
        ih264e_cabac_put_byte(ps_cabac);
    }

}
