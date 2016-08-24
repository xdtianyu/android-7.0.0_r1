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
*  ih264e_cavlc.c
*
* @brief
*  Contains all the routines to code syntax elements and residuals when entropy
*  coding chosen is CAVLC
*
* @author
*  ittiam
*
* @par List of Functions:
*  - ih264e_compute_zeroruns_and_trailingones()
*  - ih264e_write_coeff4x4_cavlc()
*  - ih264e_write_coeff8x8_cavlc()
*  - ih264e_encode_residue()
*  - ih264e_write_islice_mb_cavlc()
*  - ih264e_write_pslice_mb_cavlc()
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
#include "ih264e_config.h"
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264_debug.h"
#include "ih264_macros.h"
#include "ih264_defs.h"
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
*  This function computes run of zero, number of trailing ones and sign of
*  trailing ones basing on the significant coeff map, residual block and
*  total nnz.
*
* @param[in] pi2_res_block
*  Pointer to residual block containing levels in scan order
*
* @param[in] u4_total_coeff
*  Total non-zero coefficients in that sub block
*
* @param[in] pu1_zero_run
*  Pointer to array to store run of zeros
*
* @param[in] u4_sig_coeff_map
*  significant coefficient map
*
* @returns u4_totzero_sign_trailone
*  Bits 0-8 contains number of trailing ones.
*  Bits 8-16 contains bitwise sign information of trailing one
*  Bits 16-24 contains total number of zeros.
*
* @remarks
*  None
*
*******************************************************************************
*/
static UWORD32 ih264e_compute_zeroruns_and_trailingones(WORD16 *pi2_res_block,
                                                        UWORD32 u4_total_coeff,
                                                        UWORD8 *pu1_zero_run,
                                                        UWORD32 u4_sig_coeff_map)
{
    UWORD32 i = 0;
    UWORD32 u4_nnz_coeff = 0;
    WORD32  i4_run = -1;
    UWORD32 u4_sign = 0;
    UWORD32 u4_tot_zero = 0;
    UWORD32 u4_trailing1 = 0;
    WORD32 i4_val;
    UWORD32 u4_totzero_sign_trailone;
    UWORD32 *pu4_zero_run;

    pu4_zero_run = (void *)pu1_zero_run;
    pu4_zero_run[0] = 0;
    pu4_zero_run[1] = 0;
    pu4_zero_run[2] = 0;
    pu4_zero_run[3] = 0;

    /* Compute Runs of zeros for all nnz coefficients except the last 3 */
    if (u4_total_coeff > 3)
    {
        for (i = 0; u4_nnz_coeff < (u4_total_coeff-3); i++)
        {
            i4_run++;

            i4_val = (u4_sig_coeff_map & 0x1);
            u4_sig_coeff_map >>= 1;

            if (i4_val != 0)
            {
                pu1_zero_run[u4_nnz_coeff++] = i4_run;
                i4_run = -1;
            }
        }
    }

    /* Compute T1's, Signof(T1's) and Runs of zeros for the last 3 */
    while (u4_nnz_coeff != u4_total_coeff)
    {
        i4_run++;

        i4_val = (u4_sig_coeff_map & 0x1);
        u4_sig_coeff_map >>= 1;

        if (i4_val != 0)
        {
            if (pi2_res_block[u4_nnz_coeff] == 1)
            {
                pu1_zero_run[u4_nnz_coeff] = i4_run;
                u4_trailing1++;
            }
            else
            {
                if (pi2_res_block[u4_nnz_coeff] == -1)
                {
                    pu1_zero_run[u4_nnz_coeff] = i4_run;
                    u4_sign |= 1 << u4_trailing1;
                    u4_trailing1++;
                }
                else
                {
                    pu1_zero_run[u4_nnz_coeff] = i4_run;
                    u4_trailing1 = 0;
                    u4_sign = 0;
                }
            }
            i4_run = -1;
            u4_nnz_coeff++;
        }
        i++;
    }

    u4_tot_zero = i - u4_total_coeff;
    u4_totzero_sign_trailone = (u4_tot_zero << 16)|(u4_sign << 8)|u4_trailing1;

    return (u4_totzero_sign_trailone);
}

/**
*******************************************************************************
*
* @brief
*  This function generates CAVLC coded bit stream for the given residual block
*
* @param[in] pi2_res_block
*  Pointer to residual block containing levels in scan order
*
* @param[in] u4_total_coeff
*  Total non-zero coefficients in the sub block
*
* @param[in] u4_block_type
*  block type
*
* @param[in] pu1_zero_run
*  Pointer to array to store run of zeros
*
* @param[in] u4_nc
*  average of non zero coeff from top and left blocks (when available)
*
* @param[in, out] ps_bit_stream
*  structure pointing to a buffer holding output bit stream
*
* @param[in] u4_sig_coeff_map
*  significant coefficient map of the residual block
*
* @returns
*  error code
*
* @remarks
*  If the block type is CAVLC_CHROMA_4x4_DC, then u4_nc is non-significant
*
*******************************************************************************
*/
static IH264E_ERROR_T ih264e_write_coeff4x4_cavlc(WORD16 *pi2_res_block,
                                                  UWORD32 u4_total_coeff,
                                                  ENTROPY_BLK_TYPE u4_block_type,
                                                  UWORD8 *pu1_zero_run,
                                                  UWORD32 u4_nc,
                                                  bitstrm_t *ps_bit_stream,
                                                  UWORD32 u4_sig_coeff_map)
{
    IH264E_ERROR_T error_status = IH264E_SUCCESS;
    UWORD32 u4_totzero_sign_trailone = 0;
    UWORD32 u4_trailing_ones = 0;
    UWORD32 u4_tot_zeros = 0;
    UWORD32 u4_remaining_coeff = 0;
    UWORD32 u4_sign1 = 0;
    UWORD32 u4_max_num_coeff = 0;
    const UWORD32 au4_max_num_nnz_coeff[] = {16, 15, 16, 4, 15};

    /* validate inputs */
    ASSERT(u4_block_type <= CAVLC_CHROMA_4x4_AC);

    u4_max_num_coeff = au4_max_num_nnz_coeff[u4_block_type];

    ASSERT(u4_total_coeff <= u4_max_num_coeff);

    if (!u4_total_coeff)
    {
        UWORD32 u4_codeword = 15;
        UWORD32 u4_codesize = 1;
        if (u4_block_type == CAVLC_CHROMA_4x4_DC)
        {
            u4_codeword = 1;
            u4_codesize = 2;
            DEBUG("\n[%d numcoeff, %d numtrailing ones]",u4_total_coeff, 0);
            ENTROPY_TRACE("\tnumber of non zero coeffs ",u4_total_coeff);
            ENTROPY_TRACE("\tnumber of trailing ones ",0);
        }
        else
        {
            UWORD32 u4_vlcnum = u4_nc >> 1;

            /* write coeff_token */
            if (u4_vlcnum > 3)
            {
                /* Num-FLC */
                u4_codeword = 3;
                u4_codesize = 6;
            }
            else
            {
                /* Num-VLC 0, 1, 2 */
                if (u4_vlcnum > 1)
                {
                    u4_vlcnum = 2;
                }
                u4_codesize <<= u4_vlcnum;
                u4_codeword >>= (4 - u4_codesize);
            }

            DEBUG("\n[%d numcoeff, %d numtrailing ones, %d nnz]",u4_total_coeff, 0, u4_nc);
            ENTROPY_TRACE("\tnumber of non zero coeffs ",u4_total_coeff);
            ENTROPY_TRACE("\tnC ",u4_nc);
        }


        DEBUG("\nCOEFF TOKEN 0: %d u4_codeword, %d u4_codesize",u4_codeword, u4_codesize);
        ENTROPY_TRACE("\tcodeword ",u4_codeword);
        ENTROPY_TRACE("\tcodesize ",u4_codesize);

        error_status = ih264e_put_bits(ps_bit_stream, u4_codeword, u4_codesize);

        return error_status;
    }
    else
    {
        /* Compute zero run, number of trailing ones and their sign. */
        u4_totzero_sign_trailone =
                ih264e_compute_zeroruns_and_trailingones(pi2_res_block,
                        u4_total_coeff,
                        pu1_zero_run,
                        u4_sig_coeff_map);
        u4_trailing_ones = u4_totzero_sign_trailone & 0xFF;
        u4_sign1 = (u4_totzero_sign_trailone >> 8)& 0xFF;
        u4_tot_zeros = (u4_totzero_sign_trailone >> 16) & 0xFF;
        u4_remaining_coeff = u4_total_coeff - u4_trailing_ones;

        /* write coeff_token */
        {
            UWORD32 u4_codeword;
            UWORD32 u4_codesize;
            if (u4_block_type == CAVLC_CHROMA_4x4_DC)
            {
                u4_codeword = gu1_code_coeff_token_table_chroma[u4_trailing_ones][u4_total_coeff-1];
                u4_codesize = gu1_size_coeff_token_table_chroma[u4_trailing_ones][u4_total_coeff-1];

                DEBUG("\n[%d numcoeff, %d numtrailing ones]",u4_total_coeff, u4_trailing_ones);
                ENTROPY_TRACE("\tnumber of non zero coeffs ",u4_total_coeff);
                ENTROPY_TRACE("\tnumber of trailing ones ",u4_trailing_ones);
            }
            else
            {
                UWORD32 u4_vlcnum = u4_nc >> 1;

                if (u4_vlcnum > 3)
                {
                    /* Num-FLC */
                    u4_codeword = ((u4_total_coeff-1) << 2 ) + u4_trailing_ones;
                    u4_codesize = 6;
                }
                else
                {
                    /* Num-VLC 0, 1, 2 */
                    if (u4_vlcnum > 1)
                    {
                        u4_vlcnum = 2;
                    }
                    u4_codeword = gu1_code_coeff_token_table[u4_vlcnum][u4_trailing_ones][u4_total_coeff-1];
                    u4_codesize = gu1_size_coeff_token_table[u4_vlcnum][u4_trailing_ones][u4_total_coeff-1];
                }

                DEBUG("\n[%d numcoeff, %d numtrailing ones, %d nnz]",u4_total_coeff, u4_trailing_ones, u4_nc);
                ENTROPY_TRACE("\tnumber of non zero coeffs ",u4_total_coeff);
                ENTROPY_TRACE("\tnumber of trailing ones ",u4_trailing_ones);
                ENTROPY_TRACE("\tnC ",u4_nc);
            }

            DEBUG("\nCOEFF TOKEN 0: %d u4_codeword, %d u4_codesize",u4_codeword, u4_codesize);
            ENTROPY_TRACE("\tcodeword ",u4_codeword);
            ENTROPY_TRACE("\tcodesize ",u4_codesize);

            error_status = ih264e_put_bits(ps_bit_stream, u4_codeword, u4_codesize);
        }

        /* write sign of trailing ones */
        if (u4_trailing_ones)
        {
            DEBUG("\nT1's: %d u4_codeword, %d u4_codesize",u4_sign1, u4_trailing_ones);
            error_status = ih264e_put_bits(ps_bit_stream, u4_sign1, u4_trailing_ones);
            ENTROPY_TRACE("\tnumber of trailing ones ",u4_trailing_ones);
            ENTROPY_TRACE("\tsign of trailing ones ",u4_sign1);
        }

        /* write level codes */
        if (u4_remaining_coeff)
        {
            WORD32 i4_level = pi2_res_block[u4_remaining_coeff-1];
            UWORD32 u4_escape;
            UWORD32 u4_suffix_length = 0; // Level-VLC[N]
            UWORD32 u4_abs_level, u4_abs_level_actual = 0;
            WORD32 i4_sign;
            const UWORD32 u4_rndfactor[] = {0, 0, 1, 3, 7, 15, 31};

            DEBUG("\n \t%d coeff,",i4_level);
            ENTROPY_TRACE("\tcoeff ",i4_level);

            if (u4_trailing_ones < 3)
            {
                /* If there are less than 3 T1s, then the first non-T1 level is incremented if negative (decremented if positive)*/
                if (i4_level < 0)
                {
                    i4_level += 1;
                }
                else
                {
                    i4_level -= 1;
                }

                u4_abs_level_actual = 1;

                /* Initialize VLC table (Suffix Length) to encode the level */
                if (u4_total_coeff > 10)
                {
                    u4_suffix_length = 1;
                }
            }

            i4_sign = (i4_level >> (sizeof(WORD32) * CHAR_BIT - 1));
            u4_abs_level = ((i4_level + i4_sign) ^ i4_sign);

            u4_abs_level_actual += u4_abs_level;

            u4_escape = (u4_abs_level + u4_rndfactor[u4_suffix_length]) >> u4_suffix_length;

            while (1)
            {
                UWORD32 u4_codesize;
                UWORD32 u4_codeword;
                UWORD32 u4_codeval;

                u4_remaining_coeff--;

GATHER_CAVLC_STATS1();

                {
                    u4_codeval = u4_abs_level << 1;
                    u4_codeval = u4_codeval - 2 - i4_sign;

                    if ((!u4_suffix_length) && (u4_escape > 7) && (u4_abs_level < 16))
                    {
                        u4_codeword = (1 << 4) + (u4_codeval - 14);
                        u4_codesize = 19;
                    }
                    else if (u4_escape > 7)
                    {
                        u4_codeword = (1 << 12) + (u4_codeval - (15 << u4_suffix_length));
                        u4_codesize = 28;
                        if (!u4_suffix_length)
                        {
                            u4_codeword -= 15;
                        }
                    }
                    else
                    {
                        u4_codeword = (1 << u4_suffix_length) + (u4_codeval & ((1 << u4_suffix_length)-1));
                        u4_codesize = (u4_codeval >> u4_suffix_length) + 1 + u4_suffix_length;
                    }
                }

                /*put the level code in bitstream*/
                DEBUG("\nLEVEL: %d u4_codeword, %d u4_codesize",u4_codeword, u4_codesize);
                ENTROPY_TRACE("\tcodeword ",u4_codeword);
                ENTROPY_TRACE("\tcodesize ",u4_codesize);
                error_status = ih264e_put_bits(ps_bit_stream, u4_codeword, u4_codesize);

                if (u4_remaining_coeff == 0) break;

                /*update suffix length for next level*/
                if (u4_suffix_length == 0)
                {
                    u4_suffix_length++;
                }
                if (u4_suffix_length < 6)
                {
                    if (u4_abs_level_actual > gu1_threshold_vlc_level[u4_suffix_length])
                    {
                        u4_suffix_length++;
                    }
                }

                /* next level */
                i4_level      = pi2_res_block[u4_remaining_coeff-1];

                DEBUG("\n \t%d coeff,",i4_level);
                ENTROPY_TRACE("\tcoeff ",i4_level);

                i4_sign = (i4_level >> (sizeof(WORD32) * CHAR_BIT - 1));
                u4_abs_level = ((i4_level + i4_sign) ^ i4_sign);

                u4_abs_level_actual = u4_abs_level;

                u4_escape = (u4_abs_level + u4_rndfactor[u4_suffix_length]) >> u4_suffix_length;
            }
        }

        DEBUG("\n \t %d totalzeros",u4_tot_zeros);
        ENTROPY_TRACE("\ttotal zeros ",u4_tot_zeros);

        /* Write Total Zeros */
        if (u4_total_coeff < u4_max_num_coeff)
        {
            WORD32 index;
            UWORD32 u4_codeword;
            UWORD32 u4_codesize;

            if (u4_block_type == CAVLC_CHROMA_4x4_DC)
            {
                UWORD8 gu1_index_zero_table_chroma[] = {0, 4, 7};
                index = gu1_index_zero_table_chroma[u4_total_coeff-1] + u4_tot_zeros;
                u4_codesize = gu1_size_zero_table_chroma[index];
                u4_codeword = gu1_code_zero_table_chroma[index];
            }
            else
            {
                index = gu1_index_zero_table[u4_total_coeff-1] + u4_tot_zeros;
                u4_codesize = gu1_size_zero_table[index];
                u4_codeword = gu1_code_zero_table[index];
            }

            DEBUG("\nTOTAL ZEROS: %d u4_codeword, %d u4_codesize",u4_codeword, u4_codesize);
            ENTROPY_TRACE("\tcodeword ",u4_codeword);
            ENTROPY_TRACE("\tcodesize ",u4_codesize);
            error_status = ih264e_put_bits(ps_bit_stream, u4_codeword, u4_codesize);
        }

        /* Write Run Before */
        if (u4_tot_zeros)
        {
            UWORD32 u4_max_num_coef = u4_total_coeff-1;
            UWORD32 u4_codeword;
            UWORD32 u4_codesize;
            UWORD32 u4_zeros_left = u4_tot_zeros;

            while (u4_max_num_coef)
            {
                UWORD32 u4_run_before = pu1_zero_run[u4_max_num_coef];
                UWORD32 u4_index;

                if (u4_zeros_left > MAX_ZERO_LEFT)
                {
                    u4_index = gu1_index_run_table[MAX_ZERO_LEFT];
                }
                else
                {
                    u4_index = gu1_index_run_table[u4_zeros_left - 1];
                }

                u4_codesize = gu1_size_run_table[u4_index + u4_run_before];
                u4_codeword = gu1_code_run_table[u4_index + u4_run_before];

                DEBUG("\nRUN BEFORE ZEROS: %d u4_codeword, %d u4_codesize",u4_codeword, u4_codesize);
                ENTROPY_TRACE("\tcodeword ",u4_codeword);
                ENTROPY_TRACE("\tcodesize ",u4_codesize);
                error_status = ih264e_put_bits(ps_bit_stream, u4_codeword, u4_codesize);

                u4_zeros_left -= u4_run_before;
                if (!u4_zeros_left)
                {
                    break;
                }
                u4_max_num_coef--;
            }
        }
    }

    return error_status;
}

/**
*******************************************************************************
*
* @brief
*  This function generates CAVLC coded bit stream for the given subblock
*
* @param[in] ps_ent_ctxt
*  Pointer to entropy context
*
* @param[in] pi2_res_block
*  Pointers to residual blocks of all the partitions for the current subblk
*  (containing levels in scan order)
*
* @param[in] pu1_nnz
*  Total non-zero coefficients of all the partitions for the current subblk
*
* @param[in] pu2_sig_coeff_map
*  Significant coefficient map of all the partitions for the current subblk
*
* @param[in] u4_block_type
*  entropy coding block type
*
* @param[in] u4_ngbr_avbl
*  top and left availability of all the partitions for the current subblk
*  (packed)
*
* @param[in] pu1_top_nnz
*  pointer to the buffer containing nnz of all the subblks to the top
*
* @param[in] pu1_left_nnz
*  pointer to the buffer containing nnz of all the subblks to the left
*
* @returns error status
*
* @remarks none
*
*******************************************************************************
*/
static IH264E_ERROR_T ih264e_write_coeff8x8_cavlc(entropy_ctxt_t *ps_ent_ctxt,
                                                  WORD16 **pi2_res_block,
                                                  UWORD8 *pu1_nnz,
                                                  UWORD16 *pu2_sig_coeff_map,
                                                  ENTROPY_BLK_TYPE u4_block_type,
                                                  UWORD32 u4_ngbr_avlb,
                                                  UWORD8 *pu1_top_nnz,
                                                  UWORD8 *pu1_left_nnz)
{
    IH264E_ERROR_T error_status = IH264E_SUCCESS;
    bitstrm_t *ps_bitstream = ps_ent_ctxt->ps_bitstrm;
    UWORD8 *pu1_zero_run = ps_ent_ctxt->au1_zero_run, *pu1_ngbr_avbl;
    UWORD32 u4_nC;
    UWORD8 u1_mb_a, u1_mb_b;

    pu1_ngbr_avbl = (void *)(&u4_ngbr_avlb);

    /* encode ac block index 4x4 = 0*/
    u1_mb_a = pu1_ngbr_avbl[0] & 0x0F;
    u1_mb_b = pu1_ngbr_avbl[0] & 0xF0;
    u4_nC = 0;
    if (u1_mb_a)
        u4_nC += pu1_left_nnz[0];
    if (u1_mb_b)
        u4_nC += pu1_top_nnz[0];
    if (u1_mb_a && u1_mb_b)
        u4_nC = (u4_nC + 1) >> 1;
    pu1_left_nnz[0] = pu1_top_nnz[0] = pu1_nnz[0];
    error_status = ih264e_write_coeff4x4_cavlc(pi2_res_block[0], pu1_nnz[0], u4_block_type, pu1_zero_run, u4_nC, ps_bitstream, pu2_sig_coeff_map[0]);

    /* encode ac block index 4x4 = 1*/
    u1_mb_a = pu1_ngbr_avbl[1] & 0x0F;
    u1_mb_b = pu1_ngbr_avbl[1] & 0xF0;
    u4_nC = 0;
    if (u1_mb_a)
        u4_nC += pu1_left_nnz[0];
    if (u1_mb_b)
        u4_nC += pu1_top_nnz[1];
    if (u1_mb_a && u1_mb_b)
        u4_nC = (u4_nC + 1) >> 1;
    pu1_left_nnz[0] = pu1_top_nnz[1] = pu1_nnz[1];
    error_status = ih264e_write_coeff4x4_cavlc(pi2_res_block[1], pu1_nnz[1], u4_block_type, pu1_zero_run, u4_nC, ps_bitstream, pu2_sig_coeff_map[1]);

    /* encode ac block index 4x4 = 2*/
    u1_mb_a = pu1_ngbr_avbl[2] & 0x0F;
    u1_mb_b = pu1_ngbr_avbl[2] & 0xF0;
    u4_nC = 0;
    if (u1_mb_a)
        u4_nC += pu1_left_nnz[1];
    if (u1_mb_b)
        u4_nC += pu1_top_nnz[0];
    if (u1_mb_a && u1_mb_b)
        u4_nC = (u4_nC + 1) >> 1;
    pu1_left_nnz[1] = pu1_top_nnz[0] = pu1_nnz[2];
    error_status = ih264e_write_coeff4x4_cavlc(pi2_res_block[2], pu1_nnz[2], u4_block_type, pu1_zero_run, u4_nC, ps_bitstream, pu2_sig_coeff_map[2]);

    /* encode ac block index 4x4 = 0*/
    u1_mb_a = pu1_ngbr_avbl[3] & 0x0F;
    u1_mb_b = pu1_ngbr_avbl[3] & 0xF0;
    u4_nC = 0;
    if (u1_mb_a)
        u4_nC += pu1_left_nnz[1];
    if (u1_mb_b)
        u4_nC += pu1_top_nnz[1];
    if (u1_mb_a && u1_mb_b)
        u4_nC = (u4_nC + 1) >> 1;
    pu1_left_nnz[1] = pu1_top_nnz[1] = pu1_nnz[3];
    error_status = ih264e_write_coeff4x4_cavlc(pi2_res_block[3], pu1_nnz[3], u4_block_type, pu1_zero_run, u4_nC, ps_bitstream, pu2_sig_coeff_map[3]);

    return error_status;
}

/**
*******************************************************************************
*
* @brief
*  This function encodes luma and chroma residues of a macro block when
*  the entropy coding mode chosen is cavlc.
*
* @param[in] ps_ent_ctxt
*  Pointer to entropy context
*
* @param[in] u4_mb_type
*  current mb type
*
* @param[in] u4_cbp
*  coded block pattern for the current mb
*
* @returns error code
*
* @remarks none
*
*******************************************************************************
*/
static IH264E_ERROR_T ih264e_encode_residue(entropy_ctxt_t *ps_ent_ctxt,
                                            UWORD32 u4_mb_type,
                                            UWORD32 u4_cbp)
{
    /* error status */
    IH264E_ERROR_T error_status = IH264E_SUCCESS;

    /* packed residue */
    void *pv_mb_coeff_data = ps_ent_ctxt->pv_mb_coeff_data;

    /* bit stream buffer */
    bitstrm_t *ps_bitstream = ps_ent_ctxt->ps_bitstrm;

    /* zero run */
    UWORD8 *pu1_zero_run = ps_ent_ctxt->au1_zero_run;

    /* temp var */
    UWORD32 u4_nC, u4_ngbr_avlb;
    UWORD8 au1_nnz[4], *pu1_ngbr_avlb, *pu1_top_nnz, *pu1_left_nnz;
    UWORD16 au2_sig_coeff_map[4] = {0};
    WORD16 *pi2_res_block[4] = {NULL};
    UWORD8 *pu1_slice_idx = ps_ent_ctxt->pu1_slice_idx;
    tu_sblk_coeff_data_t *ps_mb_coeff_data;
    ENTROPY_BLK_TYPE e_entropy_blk_type = CAVLC_LUMA_4x4;

    /* ngbr availability */
    UWORD8 u1_mb_a, u1_mb_b;

    /* cbp */
    UWORD32 u4_cbp_luma = u4_cbp & 0xF, u4_cbp_chroma = u4_cbp >> 4;

    /* mb indices */
    WORD32 i4_mb_x, i4_mb_y;

    /* derive neighbor availability */
    i4_mb_x = ps_ent_ctxt->i4_mb_x;
    i4_mb_y = ps_ent_ctxt->i4_mb_y;
    pu1_slice_idx += (i4_mb_y * ps_ent_ctxt->i4_wd_mbs);
    /* left macroblock availability */
    u1_mb_a = (i4_mb_x == 0 ||
                    (pu1_slice_idx[i4_mb_x - 1 ] != pu1_slice_idx[i4_mb_x]))? 0 : 1;
    /* top macroblock availability */
    u1_mb_b = (i4_mb_y == 0 ||
                    (pu1_slice_idx[i4_mb_x-ps_ent_ctxt->i4_wd_mbs] != pu1_slice_idx[i4_mb_x]))? 0 : 1;

    pu1_ngbr_avlb = (void *)(&u4_ngbr_avlb);
    pu1_top_nnz = ps_ent_ctxt->pu1_top_nnz_luma[ps_ent_ctxt->i4_mb_x];
    pu1_left_nnz = (UWORD8 *)&ps_ent_ctxt->u4_left_nnz_luma;

    /* encode luma residue */

    /* mb type intra 16x16 */
    if (u4_mb_type == I16x16)
    {
        /* parse packed coeff data structure for residual data */
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[0], au2_sig_coeff_map[0], pi2_res_block[0]);
        /* estimate nnz for the current mb */
        u4_nC = 0;
        if (u1_mb_a)
            u4_nC += pu1_left_nnz[0];
        if (u1_mb_b)
            u4_nC += pu1_top_nnz[0];
        if (u1_mb_a && u1_mb_b)
            u4_nC = (u4_nC + 1) >> 1;

        /* encode dc block */
        ENTROPY_TRACE("Luma DC blk idx %d",0);
        error_status = ih264e_write_coeff4x4_cavlc(pi2_res_block[0], au1_nnz[0], CAVLC_LUMA_4x4_DC, pu1_zero_run, u4_nC, ps_bitstream, au2_sig_coeff_map[0]);

        e_entropy_blk_type = CAVLC_LUMA_4x4_AC;
    }

    if (u4_cbp_luma & 1)
    {
        /* encode ac block index 8x8 = 0*/
        /* parse packed coeff data structure for residual data */
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[0], au2_sig_coeff_map[0], pi2_res_block[0]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[1], au2_sig_coeff_map[1], pi2_res_block[1]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[2], au2_sig_coeff_map[2], pi2_res_block[2]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[3], au2_sig_coeff_map[3], pi2_res_block[3]);
        /* derive sub block neighbor availability */

        pu1_ngbr_avlb[0] = (u1_mb_b << 4) | (u1_mb_a);
        pu1_ngbr_avlb[1] = (u1_mb_b << 4) | 1;
        pu1_ngbr_avlb[2] = (1 << 4) | (u1_mb_a);
        pu1_ngbr_avlb[3] = 0x11;
        /* encode sub blk */
        ENTROPY_TRACE("Luma blk idx %d",0);
        error_status = ih264e_write_coeff8x8_cavlc(ps_ent_ctxt, pi2_res_block, au1_nnz, au2_sig_coeff_map, e_entropy_blk_type, u4_ngbr_avlb, pu1_top_nnz, pu1_left_nnz);
    }
    else
    {
        pu1_top_nnz[0] = pu1_top_nnz[1] = 0;
        pu1_left_nnz[0] = pu1_left_nnz[1] = 0;
    }

    if (u4_cbp_luma & 2)
    {
        /* encode ac block index 8x8 = 1*/
        /* parse packed coeff data structure for residual data */
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[0], au2_sig_coeff_map[0], pi2_res_block[0]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[1], au2_sig_coeff_map[1], pi2_res_block[1]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[2], au2_sig_coeff_map[2], pi2_res_block[2]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[3], au2_sig_coeff_map[3], pi2_res_block[3]);

        /* derive sub block neighbor availability */
        pu1_ngbr_avlb[1] = pu1_ngbr_avlb[0] = (u1_mb_b << 4) | 1;
        pu1_ngbr_avlb[3] = pu1_ngbr_avlb[2] = 0x11;
        /* encode sub blk */
        ENTROPY_TRACE("Luma blk idx %d",1);
        error_status = ih264e_write_coeff8x8_cavlc(ps_ent_ctxt, pi2_res_block, au1_nnz, au2_sig_coeff_map, e_entropy_blk_type, u4_ngbr_avlb, pu1_top_nnz+2, pu1_left_nnz);
    }
    else
    {
        (pu1_top_nnz + 2)[0] = (pu1_top_nnz + 2)[1] = 0;
        pu1_left_nnz[0] = pu1_left_nnz[1] = 0;
    }

    if (u4_cbp_luma & 0x4)
    {
        /* encode ac block index 8x8 = 2*/
        /* parse packed coeff data structure for residual data */
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[0], au2_sig_coeff_map[0], pi2_res_block[0]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[1], au2_sig_coeff_map[1], pi2_res_block[1]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[2], au2_sig_coeff_map[2], pi2_res_block[2]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[3], au2_sig_coeff_map[3], pi2_res_block[3]);

        /* derive sub block neighbor availability */
        pu1_ngbr_avlb[2] = pu1_ngbr_avlb[0] = (1 << 4) | u1_mb_a;
        pu1_ngbr_avlb[1] = pu1_ngbr_avlb[3] = 0x11;
        /* encode sub blk */
        ENTROPY_TRACE("Luma blk idx %d",2);
        error_status = ih264e_write_coeff8x8_cavlc(ps_ent_ctxt, pi2_res_block, au1_nnz, au2_sig_coeff_map, e_entropy_blk_type, u4_ngbr_avlb, pu1_top_nnz, (pu1_left_nnz+2));
    }
    else
    {
        pu1_top_nnz[0] = pu1_top_nnz[1] = 0;
        (pu1_left_nnz + 2)[0] = (pu1_left_nnz + 2)[1] = 0;
    }

    if (u4_cbp_luma & 0x8)
    {
        /* encode ac block index 8x8 = 3*/
        /* parse packed coeff data structure for residual data */
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[0], au2_sig_coeff_map[0], pi2_res_block[0]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[1], au2_sig_coeff_map[1], pi2_res_block[1]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[2], au2_sig_coeff_map[2], pi2_res_block[2]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[3], au2_sig_coeff_map[3], pi2_res_block[3]);

        /* derive sub block neighbor availability */
        u4_ngbr_avlb = 0x11111111;
        /* encode sub blk */
        ENTROPY_TRACE("Luma blk idx %d",3);
        error_status = ih264e_write_coeff8x8_cavlc(ps_ent_ctxt, pi2_res_block, au1_nnz, au2_sig_coeff_map, e_entropy_blk_type, u4_ngbr_avlb, pu1_top_nnz+2, pu1_left_nnz+2);
    }
    else
    {
        (pu1_top_nnz + 2)[0] = (pu1_top_nnz + 2)[1] = 0;
        (pu1_left_nnz + 2)[0] = (pu1_left_nnz + 2)[1] = 0;
    }

    /* encode chroma residue */
    if (u4_cbp_chroma & 3)
    {
        /* parse packed coeff data structure for residual data */
        /* cb, cr */
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[0], au2_sig_coeff_map[0], pi2_res_block[0]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[1], au2_sig_coeff_map[1], pi2_res_block[1]);

        /* encode dc block */
        /* cb, cr */
        ENTROPY_TRACE("Chroma DC blk idx %d",0);
        error_status = ih264e_write_coeff4x4_cavlc(pi2_res_block[0], au1_nnz[0], CAVLC_CHROMA_4x4_DC, pu1_zero_run, 0, ps_bitstream, au2_sig_coeff_map[0]);
        ENTROPY_TRACE("Chroma DC blk idx %d",1);
        error_status = ih264e_write_coeff4x4_cavlc(pi2_res_block[1], au1_nnz[1], CAVLC_CHROMA_4x4_DC, pu1_zero_run, 0, ps_bitstream, au2_sig_coeff_map[1]);
    }

    pu1_top_nnz = ps_ent_ctxt->pu1_top_nnz_cbcr[ps_ent_ctxt->i4_mb_x];
    pu1_left_nnz = (UWORD8 *) &ps_ent_ctxt->u4_left_nnz_cbcr;

    /* encode sub blk */
    if (u4_cbp_chroma & 0x2)
    {
        /* encode ac block index 8x8 = 0*/
        /* derive sub block neighbor availability */
        pu1_ngbr_avlb[0] = (u1_mb_b << 4) | (u1_mb_a);
        pu1_ngbr_avlb[1] = (u1_mb_b << 4) | 1;
        pu1_ngbr_avlb[2] = (1 << 4) | (u1_mb_a);
        pu1_ngbr_avlb[3] = 0x11;

        /* parse packed coeff data structure for residual data */
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[0], au2_sig_coeff_map[0], pi2_res_block[0]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[1], au2_sig_coeff_map[1], pi2_res_block[1]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[2], au2_sig_coeff_map[2], pi2_res_block[2]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[3], au2_sig_coeff_map[3], pi2_res_block[3]);

        ENTROPY_TRACE("Chroma AC blk idx %d",0);
        error_status = ih264e_write_coeff8x8_cavlc(ps_ent_ctxt, pi2_res_block, au1_nnz, au2_sig_coeff_map, CAVLC_CHROMA_4x4_AC, u4_ngbr_avlb, pu1_top_nnz, pu1_left_nnz);
    }
    else
    {
        pu1_top_nnz[0] = pu1_top_nnz[1] = 0;
        pu1_left_nnz[0] = pu1_left_nnz[1] = 0;
    }

    pu1_top_nnz += 2;
    pu1_left_nnz += 2;

    /* encode sub blk */
    if (u4_cbp_chroma & 0x2)
    {
        /* parse packed coeff data structure for residual data */
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[0], au2_sig_coeff_map[0], pi2_res_block[0]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[1], au2_sig_coeff_map[1], pi2_res_block[1]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[2], au2_sig_coeff_map[2], pi2_res_block[2]);
        PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, au1_nnz[3], au2_sig_coeff_map[3], pi2_res_block[3]);

        ENTROPY_TRACE("Chroma AC blk idx %d",1);
        error_status = ih264e_write_coeff8x8_cavlc(ps_ent_ctxt, pi2_res_block, au1_nnz, au2_sig_coeff_map, CAVLC_CHROMA_4x4_AC, u4_ngbr_avlb, pu1_top_nnz, pu1_left_nnz);
    }
    else
    {
        pu1_top_nnz[0] = pu1_top_nnz[1] = 0;
        pu1_left_nnz[0] = pu1_left_nnz[1] = 0;
    }

    /* store the index of the next mb coeff data */
    ps_ent_ctxt->pv_mb_coeff_data = pv_mb_coeff_data;

    return error_status;
}


/**
*******************************************************************************
*
* @brief
*  This function generates CAVLC coded bit stream for an Intra Slice.
*
* @description
*  The mb syntax layer for intra slices constitutes luma mb mode, luma sub modes
*  (if present), mb qp delta, coded block pattern, chroma mb mode and
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
IH264E_ERROR_T ih264e_write_islice_mb_cavlc(entropy_ctxt_t *ps_ent_ctxt)
{
    /* error status */
    IH264E_ERROR_T error_status = IH264E_SUCCESS;

    /* bit stream ptr */
    bitstrm_t *ps_bitstream = ps_ent_ctxt->ps_bitstrm;

    /* packed header data */
    UWORD8 *pu1_byte = ps_ent_ctxt->pv_mb_header_data;

    /* mb header info */
    /*
     * mb_tpm : mb type plus mode
     * mb_type : luma mb type and chroma mb type are packed
     * cbp : coded block pattern
     * mb_qp_delta : mb qp delta
     * chroma_intra_mode : chroma intra mode
     * luma_intra_mode : luma intra mode
     */
    WORD32 mb_tpm, mb_type, cbp, chroma_intra_mode, luma_intra_mode;
    WORD8 mb_qp_delta;

    /* temp var */
    WORD32 i, mb_type_stream;

    WORD32 bitstream_start_offset, bitstream_end_offset;

    /* Starting bitstream offset for header in bits */
    bitstream_start_offset = GET_NUM_BITS(ps_bitstream);


    /********************************************************************/
    /*                    BEGIN HEADER GENERATION                       */
    /********************************************************************/

    /* mb header info */
    mb_tpm = *pu1_byte++;
    cbp = *pu1_byte++;
    mb_qp_delta = *pu1_byte++;

    /* mb type */
    mb_type = mb_tpm & 0xF;
    /* is intra ? */
    if (mb_type == I16x16)
    {
        UWORD32 u4_cbp_l, u4_cbp_c;

        u4_cbp_c = (cbp >> 4);
        u4_cbp_l = (cbp & 0xF);
        luma_intra_mode = (mb_tpm >> 4) & 3;
        chroma_intra_mode = (mb_tpm >> 6);

        mb_type_stream =  luma_intra_mode + 1 + (u4_cbp_c << 2) + (u4_cbp_l == 15) * 12;

        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, mb_type_stream, error_status, "mb type");

        /* intra_chroma_pred_mode */
        PUT_BITS_UEV(ps_bitstream, chroma_intra_mode, error_status, "intra_chroma_pred_mode");
    }
    else if (mb_type == I4x4)
    {
        /* mb sub blk modes */
        WORD32 intra_pred_mode_flag, rem_intra_mode;
        WORD32 byte;

        chroma_intra_mode = (mb_tpm >> 6);

        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, 0, error_status, "mb type");

        for (i = 0; i < 16; i += 2)
        {
            /* sub blk idx 1 */
            byte = *pu1_byte++;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }

            /* sub blk idx 2 */
            byte >>= 4;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }
        }

        /* intra_chroma_pred_mode */
        PUT_BITS_UEV(ps_bitstream, chroma_intra_mode, error_status, "intra_chroma_pred_mode");
    }
    else if (mb_type == I8x8)
    {
        /* transform 8x8 flag */
        UWORD32 u4_transform_size_8x8_flag = ps_ent_ctxt->i1_transform_8x8_mode_flag;

        /* mb sub blk modes */
        WORD32 intra_pred_mode_flag, rem_intra_mode;
        WORD32 byte;

        chroma_intra_mode = (mb_tpm >> 6);

        ASSERT(0);

        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, 0, error_status, "mb type");

        /* u4_transform_size_8x8_flag */
        PUT_BITS(ps_bitstream, u4_transform_size_8x8_flag, 1, error_status, "u4_transform_size_8x8_flag");

        /* write sub block modes */
        for (i = 0; i < 4; i++)
        {
            /* sub blk idx 1 */
            byte = *pu1_byte++;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }

            /* sub blk idx 2 */
            byte >>= 4;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }
        }

        /* intra_chroma_pred_mode */
        PUT_BITS_UEV(ps_bitstream, chroma_intra_mode, error_status, "intra_chroma_pred_mode");
    }
    else
    {
    }

    /* coded_block_pattern */
    if (mb_type != I16x16)
    {
        PUT_BITS_UEV(ps_bitstream, gu1_cbp_map_tables[cbp][0], error_status, "coded_block_pattern");
    }

    if (cbp || mb_type == I16x16)
    {
        /* mb_qp_delta */
        PUT_BITS_SEV(ps_bitstream, mb_qp_delta, error_status, "mb_qp_delta");
    }

    /* Ending bitstream offset for header in bits */
    bitstream_end_offset = GET_NUM_BITS(ps_bitstream);

    ps_ent_ctxt->u4_header_bits[0] += bitstream_end_offset - bitstream_start_offset;

    /* Starting bitstream offset for residue */
    bitstream_start_offset = bitstream_end_offset;

    /* residual */
    error_status = ih264e_encode_residue(ps_ent_ctxt, mb_type, cbp);

    /* Ending bitstream offset for reside in bits */
    bitstream_end_offset = GET_NUM_BITS(ps_bitstream);
    ps_ent_ctxt->u4_residue_bits[0] += bitstream_end_offset - bitstream_start_offset;

    /* store the index of the next mb syntax layer */
    ps_ent_ctxt->pv_mb_header_data = pu1_byte;

    return error_status;
}

/**
*******************************************************************************
*
* @brief
*  This function generates CAVLC coded bit stream for Inter slices
*
* @description
*  The mb syntax layer for inter slices constitutes luma mb mode, luma sub modes
*  (if present), mb qp delta, coded block pattern, chroma mb mode and
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
IH264E_ERROR_T ih264e_write_pslice_mb_cavlc(entropy_ctxt_t *ps_ent_ctxt)
{
    /* error status */
    IH264E_ERROR_T error_status = IH264E_SUCCESS;

    /* bit stream ptr */
    bitstrm_t *ps_bitstream = ps_ent_ctxt->ps_bitstrm;

    /* packed header data */
    UWORD8 *pu1_byte = ps_ent_ctxt->pv_mb_header_data;

    /* mb header info */
    /*
     * mb_tpm : mb type plus mode
     * mb_type : luma mb type and chroma mb type are packed
     * cbp : coded block pattern
     * mb_qp_delta : mb qp delta
     * chroma_intra_mode : chroma intra mode
     * luma_intra_mode : luma intra mode
     * ps_pu :  Pointer to the array of structures having motion vectors, size
     * and position of sub partitions
     */
    WORD32 mb_tpm, mb_type, cbp, chroma_intra_mode, luma_intra_mode;
    WORD8 mb_qp_delta;

    /* temp var */
    WORD32 i, mb_type_stream, cbptable = 1;

    WORD32 is_inter = 0;

    WORD32 bitstream_start_offset, bitstream_end_offset;

    /* Starting bitstream offset for header in bits */
    bitstream_start_offset = GET_NUM_BITS(ps_bitstream);

    /********************************************************************/
    /*                    BEGIN HEADER GENERATION                       */
    /********************************************************************/

    /* mb header info */
    mb_tpm = *pu1_byte++;

    /* mb type */
    mb_type = mb_tpm & 0xF;

    /* check for skip */
    if (mb_type == PSKIP)
    {
        UWORD32 *nnz;

        is_inter = 1;

        /* increment skip counter */
        (*ps_ent_ctxt->pi4_mb_skip_run)++;

        /* store the index of the next mb syntax layer */
        ps_ent_ctxt->pv_mb_header_data = pu1_byte;

        /* set nnz to zero */
        ps_ent_ctxt->u4_left_nnz_luma = 0;
        nnz = (UWORD32 *)ps_ent_ctxt->pu1_top_nnz_luma[ps_ent_ctxt->i4_mb_x];
        *nnz = 0;
        ps_ent_ctxt->u4_left_nnz_cbcr = 0;
        nnz = (UWORD32 *)ps_ent_ctxt->pu1_top_nnz_cbcr[ps_ent_ctxt->i4_mb_x];
        *nnz = 0;

        /* residual */
        error_status = ih264e_encode_residue(ps_ent_ctxt, P16x16, 0);

        bitstream_end_offset = GET_NUM_BITS(ps_bitstream);

        ps_ent_ctxt->u4_header_bits[is_inter] += bitstream_end_offset - bitstream_start_offset;

        return error_status;
    }

    /* remaining mb header info */
    cbp = *pu1_byte++;
    mb_qp_delta = *pu1_byte++;

    /* mb skip run */
    PUT_BITS_UEV(ps_bitstream, *ps_ent_ctxt->pi4_mb_skip_run, error_status, "mb skip run");

    /* reset skip counter */
    *ps_ent_ctxt->pi4_mb_skip_run = 0;

    /* is intra ? */
    if (mb_type == I16x16)
    {
        UWORD32 u4_cbp_l, u4_cbp_c;

        is_inter = 0;

        u4_cbp_c = (cbp >> 4);
        u4_cbp_l = (cbp & 0xF);
        luma_intra_mode = (mb_tpm >> 4) & 3;
        chroma_intra_mode = (mb_tpm >> 6);

        mb_type_stream =  luma_intra_mode + 1 + (u4_cbp_c << 2) + (u4_cbp_l == 15) * 12;

        mb_type_stream += 5;

        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, mb_type_stream, error_status, "mb type");

        /* intra_chroma_pred_mode */
        PUT_BITS_UEV(ps_bitstream, chroma_intra_mode, error_status, "intra_chroma_pred_mode");
    }
    else if (mb_type == I4x4)
    {
        /* mb sub blk modes */
        WORD32 intra_pred_mode_flag, rem_intra_mode;
        WORD32 byte;

        is_inter = 0;

        chroma_intra_mode = (mb_tpm >> 6);
        cbptable = 0;

        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, 5, error_status, "mb type");

        for (i = 0; i < 16; i += 2)
        {
            /* sub blk idx 1 */
            byte = *pu1_byte++;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }

            /* sub blk idx 2 */
            byte >>= 4;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }
        }

        /* intra_chroma_pred_mode */
        PUT_BITS_UEV(ps_bitstream, chroma_intra_mode, error_status, "intra_chroma_pred_mode");
    }
    else if (mb_type == I8x8)
    {
        /* transform 8x8 flag */
        UWORD32 u4_transform_size_8x8_flag = ps_ent_ctxt->i1_transform_8x8_mode_flag;

        /* mb sub blk modes */
        WORD32 intra_pred_mode_flag, rem_intra_mode;
        WORD32 byte;

        is_inter = 0;

        chroma_intra_mode = (mb_tpm >> 6);
        cbptable = 0;

        ASSERT(0);

        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, 5, error_status, "mb type");

        /* u4_transform_size_8x8_flag */
        PUT_BITS(ps_bitstream, u4_transform_size_8x8_flag, 1, error_status, "u4_transform_size_8x8_flag");

        /* write sub block modes */
        for (i = 0; i < 4; i++)
        {
            /* sub blk idx 1 */
            byte = *pu1_byte++;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }

            /* sub blk idx 2 */
            byte >>= 4;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }
        }

        /* intra_chroma_pred_mode */
        PUT_BITS_UEV(ps_bitstream, chroma_intra_mode, error_status, "intra_chroma_pred_mode");
    }
    else
    {
        /* inter macro block partition cnt */
        const UWORD8 au1_part_cnt[] = { 1, 2, 2, 4 };

        /* mv ptr */
        WORD16 *pi2_mv_ptr = (WORD16 *)pu1_byte;

        /* number of partitions for the current mb */
        UWORD32 u4_part_cnt = au1_part_cnt[mb_type - 3];

        is_inter = 1;

        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, mb_type - 3, error_status, "mb type");

        for (i = 0; i < (WORD32)u4_part_cnt; i++)
        {
            PUT_BITS_SEV(ps_bitstream, *pi2_mv_ptr++, error_status, "mv x");
            PUT_BITS_SEV(ps_bitstream, *pi2_mv_ptr++, error_status, "mv y");
        }

        pu1_byte = (UWORD8 *)pi2_mv_ptr;
    }

    /* coded_block_pattern */
    if (mb_type != I16x16)
    {
        PUT_BITS_UEV(ps_bitstream, gu1_cbp_map_tables[cbp][cbptable], error_status, "coded_block_pattern");
    }

    if (cbp || mb_type == I16x16)
    {
        /* mb_qp_delta */
        PUT_BITS_SEV(ps_bitstream, mb_qp_delta, error_status, "mb_qp_delta");
    }

    /* Ending bitstream offset for header in bits */
    bitstream_end_offset = GET_NUM_BITS(ps_bitstream);

    ps_ent_ctxt->u4_header_bits[is_inter] += bitstream_end_offset - bitstream_start_offset;

    /* start bitstream offset for residue in bits */
    bitstream_start_offset = bitstream_end_offset;

    /* residual */
    error_status = ih264e_encode_residue(ps_ent_ctxt, mb_type, cbp);

    /* Ending bitstream offset for residue in bits */
    bitstream_end_offset = GET_NUM_BITS(ps_bitstream);

    ps_ent_ctxt->u4_residue_bits[is_inter] += bitstream_end_offset - bitstream_start_offset;

    /* store the index of the next mb syntax layer */
    ps_ent_ctxt->pv_mb_header_data = pu1_byte;

    return error_status;
}


/**
*******************************************************************************
*
* @brief
*  This function generates CAVLC coded bit stream for B slices
*
* @description
*  The mb syntax layer for inter slices constitutes luma mb mode, luma sub modes
*  (if present), mb qp delta, coded block pattern, chroma mb mode and
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
IH264E_ERROR_T ih264e_write_bslice_mb_cavlc(entropy_ctxt_t *ps_ent_ctxt)
{
    /* error status */
    IH264E_ERROR_T error_status = IH264E_SUCCESS;

    /* bit stream ptr */
    bitstrm_t *ps_bitstream = ps_ent_ctxt->ps_bitstrm;

    /* packed header data */
    UWORD8 *pu1_byte = ps_ent_ctxt->pv_mb_header_data;

    /* mb header info */
    /*
     * mb_tpm : mb type plus mode
     * mb_type : luma mb type and chroma mb type are packed
     * cbp : coded block pattern
     * mb_qp_delta : mb qp delta
     * chroma_intra_mode : chroma intra mode
     * luma_intra_mode : luma intra mode
     * ps_pu :  Pointer to the array of structures having motion vectors, size
     * and position of sub partitions
     */
    WORD32 mb_tpm, mb_type, cbp, chroma_intra_mode, luma_intra_mode;
    WORD8 mb_qp_delta;

    /* temp var */
    WORD32 i, mb_type_stream, cbptable = 1;

    WORD32 is_inter = 0;

    WORD32 bitstream_start_offset, bitstream_end_offset;

    /* Starting bitstream offset for header in bits */
    bitstream_start_offset = GET_NUM_BITS(ps_bitstream);

    /********************************************************************/
    /*                    BEGIN HEADER GENERATION                       */
    /********************************************************************/

    mb_tpm = *pu1_byte++;

    /* mb type */
    mb_type = mb_tpm & 0xF;

    /* check for skip */
    if (mb_type == BSKIP)
    {
        UWORD32 *nnz;

        is_inter = 1;

        /* increment skip counter */
        (*ps_ent_ctxt->pi4_mb_skip_run)++;

        /* store the index of the next mb syntax layer */
        ps_ent_ctxt->pv_mb_header_data = pu1_byte;

        /* set nnz to zero */
        ps_ent_ctxt->u4_left_nnz_luma = 0;
        nnz = (UWORD32 *)ps_ent_ctxt->pu1_top_nnz_luma[ps_ent_ctxt->i4_mb_x];
        *nnz = 0;
        ps_ent_ctxt->u4_left_nnz_cbcr = 0;
        nnz = (UWORD32 *)ps_ent_ctxt->pu1_top_nnz_cbcr[ps_ent_ctxt->i4_mb_x];
        *nnz = 0;

        /* residual */
        error_status = ih264e_encode_residue(ps_ent_ctxt, B16x16, 0);

        bitstream_end_offset = GET_NUM_BITS(ps_bitstream);

        ps_ent_ctxt->u4_header_bits[is_inter] += bitstream_end_offset
                        - bitstream_start_offset;

        return error_status;
    }


    /* remaining mb header info */
    cbp = *pu1_byte++;
    mb_qp_delta = *pu1_byte++;

    /* mb skip run */
    PUT_BITS_UEV(ps_bitstream, *ps_ent_ctxt->pi4_mb_skip_run, error_status, "mb skip run");

    /* reset skip counter */
    *ps_ent_ctxt->pi4_mb_skip_run = 0;

    /* is intra ? */
    if (mb_type == I16x16)
    {
        UWORD32 u4_cbp_l, u4_cbp_c;

        is_inter = 0;

        u4_cbp_c = (cbp >> 4);
        u4_cbp_l = (cbp & 0xF);
        luma_intra_mode = (mb_tpm >> 4) & 3;
        chroma_intra_mode = (mb_tpm >> 6);

        mb_type_stream =  luma_intra_mode + 1 + (u4_cbp_c << 2) + (u4_cbp_l == 15) * 12;

        mb_type_stream += 23;

        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, mb_type_stream, error_status, "mb type");

        /* intra_chroma_pred_mode */
        PUT_BITS_UEV(ps_bitstream, chroma_intra_mode, error_status, "intra_chroma_pred_mode");
    }
    else if (mb_type == I4x4)
    {
        /* mb sub blk modes */
        WORD32 intra_pred_mode_flag, rem_intra_mode;
        WORD32 byte;

        is_inter = 0;

        chroma_intra_mode = (mb_tpm >> 6);
        cbptable = 0;

        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, 23, error_status, "mb type");

        for (i = 0; i < 16; i += 2)
        {
            /* sub blk idx 1 */
            byte = *pu1_byte++;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }

            /* sub blk idx 2 */
            byte >>= 4;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }
        }

        /* intra_chroma_pred_mode */
        PUT_BITS_UEV(ps_bitstream, chroma_intra_mode, error_status, "intra_chroma_pred_mode");
    }
    else if (mb_type == I8x8)
    {
        /* transform 8x8 flag */
        UWORD32 u4_transform_size_8x8_flag = ps_ent_ctxt->i1_transform_8x8_mode_flag;

        /* mb sub blk modes */
        WORD32 intra_pred_mode_flag, rem_intra_mode;
        WORD32 byte;

        is_inter = 0;

        chroma_intra_mode = (mb_tpm >> 6);
        cbptable = 0;

        ASSERT(0);

        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, 23, error_status, "mb type");

        /* u4_transform_size_8x8_flag */
        PUT_BITS(ps_bitstream, u4_transform_size_8x8_flag, 1, error_status, "u4_transform_size_8x8_flag");

        /* write sub block modes */
        for (i = 0; i < 4; i++)
        {
            /* sub blk idx 1 */
            byte = *pu1_byte++;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }

            /* sub blk idx 2 */
            byte >>= 4;

            intra_pred_mode_flag = byte & 0x1;

            /* prev_intra4x4_pred_mode_flag */
            PUT_BITS(ps_bitstream, intra_pred_mode_flag, 1, error_status, "prev_intra4x4_pred_mode_flag");

            /* rem_intra4x4_pred_mode */
            if (!intra_pred_mode_flag)
            {
                rem_intra_mode = (byte & 0xF) >> 1;
                PUT_BITS(ps_bitstream, rem_intra_mode, 3, error_status, "rem_intra4x4_pred_mode");
            }
        }

        /* intra_chroma_pred_mode */
        PUT_BITS_UEV(ps_bitstream, chroma_intra_mode, error_status, "intra_chroma_pred_mode");
    }
    else if(mb_type == BDIRECT)
    {
        is_inter = 1;
        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, B_DIRECT_16x16, error_status, "mb type");
    }
    else /* if mb_type == B16x16 */
    {
        /* inter macro block partition cnt for 16x16 16x8 8x16 8x8 */
        const UWORD8 au1_part_cnt[] = { 1, 2, 2, 4 };

        /* mv ptr */
        WORD16 *pi2_mvd_ptr = (WORD16 *)pu1_byte;

        /* number of partitions for the current mb */
        UWORD32 u4_part_cnt = au1_part_cnt[mb_type - B16x16];

        /* Get the pred modes */
        WORD32 i4_mb_part_pred_mode = (mb_tpm >> 4);

        is_inter = 1;

        mb_type_stream = mb_type - B16x16 + B_L0_16x16 + i4_mb_part_pred_mode;

        /* write mb type */
        PUT_BITS_UEV(ps_bitstream, mb_type_stream, error_status, "mb type");

        for (i = 0; i < (WORD32)u4_part_cnt; i++)
        {
            if (i4_mb_part_pred_mode != PRED_L1)/* || PRED_BI */
            {
                PUT_BITS_SEV(ps_bitstream, *pi2_mvd_ptr, error_status, "mv l0 x");
                pi2_mvd_ptr++;
                PUT_BITS_SEV(ps_bitstream, *pi2_mvd_ptr, error_status, "mv l0 y");
                pi2_mvd_ptr++;
            }
            if (i4_mb_part_pred_mode != PRED_L0)/* || PRED_BI */
            {
                PUT_BITS_SEV(ps_bitstream, *pi2_mvd_ptr, error_status, "mv l1 x");
                pi2_mvd_ptr++;
                PUT_BITS_SEV(ps_bitstream, *pi2_mvd_ptr, error_status, "mv l1 y");
                pi2_mvd_ptr++;
            }
        }

        pu1_byte = (UWORD8 *)pi2_mvd_ptr;
    }

    /* coded_block_pattern */
    if (mb_type != I16x16)
    {
        PUT_BITS_UEV(ps_bitstream, gu1_cbp_map_tables[cbp][cbptable], error_status, "coded_block_pattern");
    }

    if (cbp || mb_type == I16x16)
    {
        /* mb_qp_delta */
        PUT_BITS_SEV(ps_bitstream, mb_qp_delta, error_status, "mb_qp_delta");
    }

    /* Ending bitstream offset for header in bits */
    bitstream_end_offset = GET_NUM_BITS(ps_bitstream);

    ps_ent_ctxt->u4_header_bits[is_inter] += bitstream_end_offset - bitstream_start_offset;

    /* start bitstream offset for residue in bits */
    bitstream_start_offset = bitstream_end_offset;

    /* residual */
    error_status = ih264e_encode_residue(ps_ent_ctxt, mb_type, cbp);

    /* Ending bitstream offset for residue in bits */
    bitstream_end_offset = GET_NUM_BITS(ps_bitstream);

    ps_ent_ctxt->u4_residue_bits[is_inter] += bitstream_end_offset - bitstream_start_offset;

    /* store the index of the next mb syntax layer */
    ps_ent_ctxt->pv_mb_header_data = pu1_byte;

    return error_status;
}
