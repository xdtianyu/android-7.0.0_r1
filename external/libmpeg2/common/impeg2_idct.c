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
/*****************************************************************************/
/*                                                                           */
/*  File Name         : impeg2_idct.c                                        */
/*                                                                           */
/*  Description       : Contains 2d idct and invese quantization functions   */
/*                                                                           */
/*  List of Functions : impeg2_idct_recon_dc()                               */
/*                      impeg2_idct_recon_dc_mismatch()                      */
/*                      impeg2_idct_recon()                                  */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         10 09 2005   Hairsh M        First Version                        */
/*                                                                           */
/*****************************************************************************/
/*
  IEEE - 1180 results for this IDCT
  L                           256         256         5           5           300         300         384         384         Thresholds
  H                           255         255         5           5           300         300         383         383
  sign                        1           -1          1           -1          1           -1          1           -1
  Peak Error                  1           1           1           1           1           1           1           1           1
  Peak Mean Square Error      0.0191      0.0188      0.0108      0.0111      0.0176      0.0188      0.0165      0.0177      0.06
  Overall Mean Square Error   0.01566406  0.01597656  0.0091875   0.00908906  0.01499063  0.01533281  0.01432344  0.01412344  0.02
  Peak Mean Error             0.0027      0.0026      0.0028      0.002       0.0017      0.0033      0.0031      0.0025      0.015
  Overall Mean Error          0.00002656  -0.00031406 0.00016875  0.00005469  -0.00003125 0.00011406  0.00009219  0.00004219  0.0015
  */
#include <stdio.h>
#include <string.h>

#include "iv_datatypedef.h"
#include "iv.h"
#include "impeg2_defs.h"
#include "impeg2_platform_macros.h"

#include "impeg2_macros.h"
#include "impeg2_globals.h"
#include "impeg2_idct.h"


void impeg2_idct_recon_dc(WORD16 *pi2_src,
                            WORD16 *pi2_tmp,
                            UWORD8 *pu1_pred,
                            UWORD8 *pu1_dst,
                            WORD32 i4_src_strd,
                            WORD32 i4_pred_strd,
                            WORD32 i4_dst_strd,
                            WORD32 i4_zero_cols,
                            WORD32 i4_zero_rows)
{
    WORD32 i4_val, i, j;

    UNUSED(pi2_tmp);
    UNUSED(i4_src_strd);
    UNUSED(i4_zero_cols);
    UNUSED(i4_zero_rows);

    i4_val = pi2_src[0] * gai2_impeg2_idct_q15[0];
    i4_val = ((i4_val + IDCT_STG1_ROUND) >> IDCT_STG1_SHIFT);
    i4_val = i4_val * gai2_impeg2_idct_q11[0];
    i4_val = ((i4_val + IDCT_STG2_ROUND) >> IDCT_STG2_SHIFT);

    for(i = 0; i < TRANS_SIZE_8; i++)
    {
        for(j = 0; j < TRANS_SIZE_8; j++)
        {
            pu1_dst[j] = CLIP_U8(i4_val + pu1_pred[j]);
        }
        pu1_dst  += i4_dst_strd;
        pu1_pred += i4_pred_strd;
    }
}
void impeg2_idct_recon_dc_mismatch(WORD16 *pi2_src,
                            WORD16 *pi2_tmp,
                            UWORD8 *pu1_pred,
                            UWORD8 *pu1_dst,
                            WORD32 i4_src_strd,
                            WORD32 i4_pred_strd,
                            WORD32 i4_dst_strd,
                            WORD32 i4_zero_cols,
                            WORD32 i4_zero_rows)

{
    WORD32 i4_val, i, j;
    WORD32 i4_count = 0;
    WORD32 i4_sum;

    UNUSED(pi2_tmp);
    UNUSED(i4_src_strd);
    UNUSED(i4_zero_cols);
    UNUSED(i4_zero_rows);

    i4_val = pi2_src[0] * gai2_impeg2_idct_q15[0];
    i4_val = ((i4_val + IDCT_STG1_ROUND) >> IDCT_STG1_SHIFT);

    i4_val *= gai2_impeg2_idct_q11[0];
    for(i = 0; i < TRANS_SIZE_8; i++)
    {
        for (j = 0; j < TRANS_SIZE_8; j++)
        {
            i4_sum = i4_val;
            i4_sum += gai2_impeg2_mismatch_stg2_additive[i4_count];
            i4_sum = ((i4_sum + IDCT_STG2_ROUND) >> IDCT_STG2_SHIFT);
            i4_sum += pu1_pred[j];
            pu1_dst[j] = CLIP_U8(i4_sum);
            i4_count++;
        }

        pu1_dst  += i4_dst_strd;
        pu1_pred += i4_pred_strd;
    }

}
/**
 *******************************************************************************
 *
 * @brief
 *  This function performs Inverse transform  and reconstruction for 8x8
 * input block
 *
 * @par Description:
 *  Performs inverse transform and adds the prediction  data and clips output
 * to 8 bit
 *
 * @param[in] pi2_src
 *  Input 8x8 coefficients
 *
 * @param[in] pi2_tmp
 *  Temporary 8x8 buffer for storing inverse
 *
 *  transform
 *  1st stage output
 *
 * @param[in] pu1_pred
 *  Prediction 8x8 block
 *
 * @param[out] pu1_dst
 *  Output 8x8 block
 *
 * @param[in] src_strd
 *  Input stride
 *
 * @param[in] pred_strd
 *  Prediction stride
 *
 * @param[in] dst_strd
 *  Output Stride
 *
 * @param[in] shift
 *  Output shift
 *
 * @param[in] zero_cols
 *  Zero columns in pi2_src
 *
 * @returns  Void
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */

void impeg2_idct_recon(WORD16 *pi2_src,
                        WORD16 *pi2_tmp,
                        UWORD8 *pu1_pred,
                        UWORD8 *pu1_dst,
                        WORD32 i4_src_strd,
                        WORD32 i4_pred_strd,
                        WORD32 i4_dst_strd,
                        WORD32 i4_zero_cols,
                        WORD32 i4_zero_rows)
{
    WORD32 j, k;
    WORD32 ai4_e[4], ai4_o[4];
    WORD32 ai4_ee[2], ai4_eo[2];
    WORD32 i4_add;
    WORD32 i4_shift;
    WORD16 *pi2_tmp_orig;
    WORD32 i4_trans_size;
    WORD32 i4_zero_rows_2nd_stage = i4_zero_cols;
    WORD32 i4_row_limit_2nd_stage;

    i4_trans_size = TRANS_SIZE_8;

    pi2_tmp_orig = pi2_tmp;

    if((i4_zero_cols & 0xF0) == 0xF0)
        i4_row_limit_2nd_stage = 4;
    else
        i4_row_limit_2nd_stage = TRANS_SIZE_8;


    if((i4_zero_rows & 0xF0) == 0xF0) /* First 4 rows of input are non-zero */
    {
        /************************************************************************************************/
        /**********************************START - IT_RECON_8x8******************************************/
        /************************************************************************************************/

        /* Inverse Transform 1st stage */
        i4_shift = IDCT_STG1_SHIFT;
        i4_add = 1 << (i4_shift - 1);

        for(j = 0; j < i4_row_limit_2nd_stage; j++)
        {
            /* Checking for Zero Cols */
            if((i4_zero_cols & 1) == 1)
            {
                memset(pi2_tmp, 0, i4_trans_size * sizeof(WORD16));
            }
            else
            {
                /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
                for(k = 0; k < 4; k++)
                {
                    ai4_o[k] = gai2_impeg2_idct_q15[1 * 8 + k] * pi2_src[i4_src_strd]
                                    + gai2_impeg2_idct_q15[3 * 8 + k]
                                                    * pi2_src[3 * i4_src_strd];
                }
                ai4_eo[0] = gai2_impeg2_idct_q15[2 * 8 + 0] * pi2_src[2 * i4_src_strd];
                ai4_eo[1] = gai2_impeg2_idct_q15[2 * 8 + 1] * pi2_src[2 * i4_src_strd];
                ai4_ee[0] = gai2_impeg2_idct_q15[0 * 8 + 0] * pi2_src[0];
                ai4_ee[1] = gai2_impeg2_idct_q15[0 * 8 + 1] * pi2_src[0];

                /* Combining e and o terms at each hierarchy levels to calculate the final spatial domain vector */
                ai4_e[0] = ai4_ee[0] + ai4_eo[0];
                ai4_e[3] = ai4_ee[0] - ai4_eo[0];
                ai4_e[1] = ai4_ee[1] + ai4_eo[1];
                ai4_e[2] = ai4_ee[1] - ai4_eo[1];
                for(k = 0; k < 4; k++)
                {
                    pi2_tmp[k] =
                                    CLIP_S16(((ai4_e[k] + ai4_o[k] + i4_add) >> i4_shift));
                    pi2_tmp[k + 4] =
                                    CLIP_S16(((ai4_e[3 - k] - ai4_o[3 - k] + i4_add) >> i4_shift));
                }
            }
            pi2_src++;
            pi2_tmp += i4_trans_size;
            i4_zero_cols = i4_zero_cols >> 1;
        }

        pi2_tmp = pi2_tmp_orig;

        /* Inverse Transform 2nd stage */
        i4_shift = IDCT_STG2_SHIFT;
        i4_add = 1 << (i4_shift - 1);
        if((i4_zero_rows_2nd_stage & 0xF0) == 0xF0) /* First 4 rows of output of 1st stage are non-zero */
        {
            for(j = 0; j < i4_trans_size; j++)
            {
                /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
                for(k = 0; k < 4; k++)
                {
                    ai4_o[k] = gai2_impeg2_idct_q11[1 * 8 + k] * pi2_tmp[i4_trans_size]
                                    + gai2_impeg2_idct_q11[3 * 8 + k] * pi2_tmp[3 * i4_trans_size];
                }
                ai4_eo[0] = gai2_impeg2_idct_q11[2 * 8 + 0] * pi2_tmp[2 * i4_trans_size];
                ai4_eo[1] = gai2_impeg2_idct_q11[2 * 8 + 1] * pi2_tmp[2 * i4_trans_size];
                ai4_ee[0] = gai2_impeg2_idct_q11[0 * 8 + 0] * pi2_tmp[0];
                ai4_ee[1] = gai2_impeg2_idct_q11[0 * 8 + 1] * pi2_tmp[0];

                /* Combining e and o terms at each hierarchy levels to calculate the final spatial domain vector */
                ai4_e[0] = ai4_ee[0] + ai4_eo[0];
                ai4_e[3] = ai4_ee[0] - ai4_eo[0];
                ai4_e[1] = ai4_ee[1] + ai4_eo[1];
                ai4_e[2] = ai4_ee[1] - ai4_eo[1];
                for(k = 0; k < 4; k++)
                {
                    WORD32 itrans_out;
                    itrans_out =
                                    CLIP_S16(((ai4_e[k] + ai4_o[k] + i4_add) >> i4_shift));
                    pu1_dst[k] = CLIP_U8((itrans_out + pu1_pred[k]));
                    itrans_out =
                                    CLIP_S16(((ai4_e[3 - k] - ai4_o[3 - k] + i4_add) >> i4_shift));
                    pu1_dst[k + 4] = CLIP_U8((itrans_out + pu1_pred[k + 4]));
                }
                pi2_tmp++;
                pu1_pred += i4_pred_strd;
                pu1_dst += i4_dst_strd;
            }
        }
        else /* All rows of output of 1st stage are non-zero */
        {
            for(j = 0; j < i4_trans_size; j++)
            {
                /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
                for(k = 0; k < 4; k++)
                {
                    ai4_o[k] = gai2_impeg2_idct_q11[1 * 8 + k] * pi2_tmp[i4_trans_size]
                                    + gai2_impeg2_idct_q11[3 * 8 + k]
                                                    * pi2_tmp[3 * i4_trans_size]
                                    + gai2_impeg2_idct_q11[5 * 8 + k]
                                                    * pi2_tmp[5 * i4_trans_size]
                                    + gai2_impeg2_idct_q11[7 * 8 + k]
                                                    * pi2_tmp[7 * i4_trans_size];
                }

                ai4_eo[0] = gai2_impeg2_idct_q11[2 * 8 + 0] * pi2_tmp[2 * i4_trans_size]
                                + gai2_impeg2_idct_q11[6 * 8 + 0] * pi2_tmp[6 * i4_trans_size];
                ai4_eo[1] = gai2_impeg2_idct_q11[2 * 8 + 1] * pi2_tmp[2 * i4_trans_size]
                                + gai2_impeg2_idct_q11[6 * 8 + 1] * pi2_tmp[6 * i4_trans_size];
                ai4_ee[0] = gai2_impeg2_idct_q11[0 * 8 + 0] * pi2_tmp[0]
                                + gai2_impeg2_idct_q11[4 * 8 + 0] * pi2_tmp[4 * i4_trans_size];
                ai4_ee[1] = gai2_impeg2_idct_q11[0 * 8 + 1] * pi2_tmp[0]
                                + gai2_impeg2_idct_q11[4 * 8 + 1] * pi2_tmp[4 * i4_trans_size];

                /* Combining e and o terms at each hierarchy levels to calculate the final spatial domain vector */
                ai4_e[0] = ai4_ee[0] + ai4_eo[0];
                ai4_e[3] = ai4_ee[0] - ai4_eo[0];
                ai4_e[1] = ai4_ee[1] + ai4_eo[1];
                ai4_e[2] = ai4_ee[1] - ai4_eo[1];
                for(k = 0; k < 4; k++)
                {
                    WORD32 itrans_out;
                    itrans_out =
                                    CLIP_S16(((ai4_e[k] + ai4_o[k] + i4_add) >> i4_shift));
                    pu1_dst[k] = CLIP_U8((itrans_out + pu1_pred[k]));
                    itrans_out =
                                    CLIP_S16(((ai4_e[3 - k] - ai4_o[3 - k] + i4_add) >> i4_shift));
                    pu1_dst[k + 4] = CLIP_U8((itrans_out + pu1_pred[k + 4]));
                }
                pi2_tmp++;
                pu1_pred += i4_pred_strd;
                pu1_dst += i4_dst_strd;
            }
        }
        /************************************************************************************************/
        /************************************END - IT_RECON_8x8******************************************/
        /************************************************************************************************/
    }
    else /* All rows of input are non-zero */
    {
        /************************************************************************************************/
        /**********************************START - IT_RECON_8x8******************************************/
        /************************************************************************************************/

        /* Inverse Transform 1st stage */
        i4_shift = IDCT_STG1_SHIFT;
        i4_add = 1 << (i4_shift - 1);

        for(j = 0; j < i4_row_limit_2nd_stage; j++)
        {
            /* Checking for Zero Cols */
            if((i4_zero_cols & 1) == 1)
            {
                memset(pi2_tmp, 0, i4_trans_size * sizeof(WORD16));
            }
            else
            {
                /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
                for(k = 0; k < 4; k++)
                {
                    ai4_o[k] = gai2_impeg2_idct_q15[1 * 8 + k] * pi2_src[i4_src_strd]
                                    + gai2_impeg2_idct_q15[3 * 8 + k]
                                                    * pi2_src[3 * i4_src_strd]
                                    + gai2_impeg2_idct_q15[5 * 8 + k]
                                                    * pi2_src[5 * i4_src_strd]
                                    + gai2_impeg2_idct_q15[7 * 8 + k]
                                                    * pi2_src[7 * i4_src_strd];
                }

                ai4_eo[0] = gai2_impeg2_idct_q15[2 * 8 + 0] * pi2_src[2 * i4_src_strd]
                                + gai2_impeg2_idct_q15[6 * 8 + 0] * pi2_src[6 * i4_src_strd];
                ai4_eo[1] = gai2_impeg2_idct_q15[2 * 8 + 1] * pi2_src[2 * i4_src_strd]
                                + gai2_impeg2_idct_q15[6 * 8 + 1] * pi2_src[6 * i4_src_strd];
                ai4_ee[0] = gai2_impeg2_idct_q15[0 * 8 + 0] * pi2_src[0]
                                + gai2_impeg2_idct_q15[4 * 8 + 0] * pi2_src[4 * i4_src_strd];
                ai4_ee[1] = gai2_impeg2_idct_q15[0 * 8 + 1] * pi2_src[0]
                                + gai2_impeg2_idct_q15[4 * 8 + 1] * pi2_src[4 * i4_src_strd];

                /* Combining e and o terms at each hierarchy levels to calculate the final spatial domain vector */
                ai4_e[0] = ai4_ee[0] + ai4_eo[0];
                ai4_e[3] = ai4_ee[0] - ai4_eo[0];
                ai4_e[1] = ai4_ee[1] + ai4_eo[1];
                ai4_e[2] = ai4_ee[1] - ai4_eo[1];
                for(k = 0; k < 4; k++)
                {
                    pi2_tmp[k] =
                                    CLIP_S16(((ai4_e[k] + ai4_o[k] + i4_add) >> i4_shift));
                    pi2_tmp[k + 4] =
                                    CLIP_S16(((ai4_e[3 - k] - ai4_o[3 - k] + i4_add) >> i4_shift));
                }
            }
            pi2_src++;
            pi2_tmp += i4_trans_size;
            i4_zero_cols = i4_zero_cols >> 1;
        }

        pi2_tmp = pi2_tmp_orig;

        /* Inverse Transform 2nd stage */
        i4_shift = IDCT_STG2_SHIFT;
        i4_add = 1 << (i4_shift - 1);
        if((i4_zero_rows_2nd_stage & 0xF0) == 0xF0) /* First 4 rows of output of 1st stage are non-zero */
        {
            for(j = 0; j < i4_trans_size; j++)
            {
                /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
                for(k = 0; k < 4; k++)
                {
                    ai4_o[k] = gai2_impeg2_idct_q11[1 * 8 + k] * pi2_tmp[i4_trans_size]
                                    + gai2_impeg2_idct_q11[3 * 8 + k] * pi2_tmp[3 * i4_trans_size];
                }
                ai4_eo[0] = gai2_impeg2_idct_q11[2 * 8 + 0] * pi2_tmp[2 * i4_trans_size];
                ai4_eo[1] = gai2_impeg2_idct_q11[2 * 8 + 1] * pi2_tmp[2 * i4_trans_size];
                ai4_ee[0] = gai2_impeg2_idct_q11[0 * 8 + 0] * pi2_tmp[0];
                ai4_ee[1] = gai2_impeg2_idct_q11[0 * 8 + 1] * pi2_tmp[0];

                /* Combining e and o terms at each hierarchy levels to calculate the final spatial domain vector */
                ai4_e[0] = ai4_ee[0] + ai4_eo[0];
                ai4_e[3] = ai4_ee[0] - ai4_eo[0];
                ai4_e[1] = ai4_ee[1] + ai4_eo[1];
                ai4_e[2] = ai4_ee[1] - ai4_eo[1];
                for(k = 0; k < 4; k++)
                {
                    WORD32 itrans_out;
                    itrans_out =
                                    CLIP_S16(((ai4_e[k] + ai4_o[k] + i4_add) >> i4_shift));
                    pu1_dst[k] = CLIP_U8((itrans_out + pu1_pred[k]));
                    itrans_out =
                                    CLIP_S16(((ai4_e[3 - k] - ai4_o[3 - k] + i4_add) >> i4_shift));
                    pu1_dst[k + 4] = CLIP_U8((itrans_out + pu1_pred[k + 4]));
                }
                pi2_tmp++;
                pu1_pred += i4_pred_strd;
                pu1_dst += i4_dst_strd;
            }
        }
        else /* All rows of output of 1st stage are non-zero */
        {
            for(j = 0; j < i4_trans_size; j++)
            {
                /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
                for(k = 0; k < 4; k++)
                {
                    ai4_o[k] = gai2_impeg2_idct_q11[1 * 8 + k] * pi2_tmp[i4_trans_size]
                                    + gai2_impeg2_idct_q11[3 * 8 + k]
                                                    * pi2_tmp[3 * i4_trans_size]
                                    + gai2_impeg2_idct_q11[5 * 8 + k]
                                                    * pi2_tmp[5 * i4_trans_size]
                                    + gai2_impeg2_idct_q11[7 * 8 + k]
                                                    * pi2_tmp[7 * i4_trans_size];
                }

                ai4_eo[0] = gai2_impeg2_idct_q11[2 * 8 + 0] * pi2_tmp[2 * i4_trans_size]
                                + gai2_impeg2_idct_q11[6 * 8 + 0] * pi2_tmp[6 * i4_trans_size];
                ai4_eo[1] = gai2_impeg2_idct_q11[2 * 8 + 1] * pi2_tmp[2 * i4_trans_size]
                                + gai2_impeg2_idct_q11[6 * 8 + 1] * pi2_tmp[6 * i4_trans_size];
                ai4_ee[0] = gai2_impeg2_idct_q11[0 * 8 + 0] * pi2_tmp[0]
                                + gai2_impeg2_idct_q11[4 * 8 + 0] * pi2_tmp[4 * i4_trans_size];
                ai4_ee[1] = gai2_impeg2_idct_q11[0 * 8 + 1] * pi2_tmp[0]
                                + gai2_impeg2_idct_q11[4 * 8 + 1] * pi2_tmp[4 * i4_trans_size];

                /* Combining e and o terms at each hierarchy levels to calculate the final spatial domain vector */
                ai4_e[0] = ai4_ee[0] + ai4_eo[0];
                ai4_e[3] = ai4_ee[0] - ai4_eo[0];
                ai4_e[1] = ai4_ee[1] + ai4_eo[1];
                ai4_e[2] = ai4_ee[1] - ai4_eo[1];
                for(k = 0; k < 4; k++)
                {
                    WORD32 itrans_out;
                    itrans_out =
                                    CLIP_S16(((ai4_e[k] + ai4_o[k] + i4_add) >> i4_shift));
                    pu1_dst[k] = CLIP_U8((itrans_out + pu1_pred[k]));
                    itrans_out =
                                    CLIP_S16(((ai4_e[3 - k] - ai4_o[3 - k] + i4_add) >> i4_shift));
                    pu1_dst[k + 4] = CLIP_U8((itrans_out + pu1_pred[k + 4]));
                }
                pi2_tmp++;
                pu1_pred += i4_pred_strd;
                pu1_dst += i4_dst_strd;
            }
        }
        /************************************************************************************************/
        /************************************END - IT_RECON_8x8******************************************/
        /************************************************************************************************/
    }
}

