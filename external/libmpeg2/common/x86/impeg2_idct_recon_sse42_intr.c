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
 *  impeg2_itrans_recon_x86_intr.c
 *
 * @brief
 *  Contains function definitions for inverse  quantization, inverse
 * transform and reconstruction
 *
 * @author
 *  100470
 *  100592 (edited by)
 *
 * @par List of Functions:
 *  - impeg2_itrans_recon_8x8_sse42()
 *
 * @remarks
 *  None
 *
 *******************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include "iv_datatypedef.h"
#include "impeg2_macros.h"
#include "impeg2_defs.h"
#include "impeg2_globals.h"

#include <immintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>


/**
 *******************************************************************************
 *
 * @brief
 *  This function performs inverse quantization, inverse  transform and
 * reconstruction for 8c8 input block
 *
 * @par Description:
 *  Performs inverse quantization , inverse transform  and adds the
 * prediction data and clips output to 8 bit
 *
 * @param[in] pi2_src
 *  Input 8x8 coefficients
 *
 * @param[in] pi2_tmp
 *  Temporary 8x8 buffer for storing inverse
 *  transform 1st stage output
 *
 * @param[in] pu1_pred
 *  Prediction 8x8 block
 *
 * @param[in] pi2_dequant_coeff
 *  Dequant Coeffs
 *
 * @param[out] pu1_dst
 *  Output 8x8 block
 *
 * @param[in] src_strd
 *  Input stride
 *
 * @param[in] qp_div
 *  Quantization parameter / 6
 *
 * @param[in] qp_rem
 *  Quantization parameter % 6
 *
 * @param[in] pred_strd
 *  Prediction stride
 *
 * @param[in] dst_strd
 *  Output Stride
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


void impeg2_idct_recon_sse42(WORD16 *pi2_src,
                                  WORD16 *pi2_tmp,
                                  UWORD8 *pu1_pred,
                                  UWORD8 *pu1_dst,
                                  WORD32 src_strd,
                                  WORD32 pred_strd,
                                  WORD32 dst_strd,
                                  WORD32 zero_cols,
                                  WORD32 zero_rows)
{
    __m128i m_temp_reg_0;
    __m128i m_temp_reg_1;
    __m128i m_temp_reg_2;
    __m128i m_temp_reg_3;
    __m128i m_temp_reg_5;
    __m128i m_temp_reg_6;
    __m128i m_temp_reg_7;
    __m128i m_temp_reg_4;
    __m128i m_temp_reg_10;
    __m128i m_temp_reg_11;
    __m128i m_temp_reg_12;
    __m128i m_temp_reg_13;
    __m128i m_temp_reg_14;
    __m128i m_temp_reg_15;
    __m128i m_temp_reg_16;
    __m128i m_temp_reg_17;
    __m128i m_temp_reg_20;
    __m128i m_temp_reg_21;
    __m128i m_temp_reg_22;
    __m128i m_temp_reg_23;
    __m128i m_temp_reg_24;
    __m128i m_temp_reg_25;
    __m128i m_temp_reg_26;
    __m128i m_temp_reg_27;
    __m128i m_temp_reg_30;
    __m128i m_temp_reg_31;
    __m128i m_temp_reg_32;
    __m128i m_temp_reg_33;
    __m128i m_temp_reg_34;
    __m128i m_temp_reg_35;
    __m128i m_temp_reg_36;
    __m128i m_temp_reg_37;
    __m128i m_temp_reg_40;
    __m128i m_temp_reg_41;
    __m128i m_temp_reg_42;
    __m128i m_temp_reg_43;
    __m128i m_temp_reg_44;
    __m128i m_temp_reg_45;
    __m128i m_temp_reg_46;
    __m128i m_temp_reg_47;
    __m128i m_temp_reg_50;
    __m128i m_temp_reg_51;
    __m128i m_temp_reg_52;
    __m128i m_temp_reg_53;
    __m128i m_temp_reg_54;
    __m128i m_temp_reg_55;
    __m128i m_temp_reg_56;
    __m128i m_temp_reg_57;
    __m128i m_temp_reg_60;
    __m128i m_temp_reg_61;
    __m128i m_temp_reg_62;
    __m128i m_temp_reg_63;
    __m128i m_temp_reg_64;
    __m128i m_temp_reg_65;
    __m128i m_temp_reg_66;
    __m128i m_temp_reg_67;
    __m128i m_temp_reg_70;
    __m128i m_temp_reg_71;
    __m128i m_temp_reg_72;
    __m128i m_temp_reg_73;
    __m128i m_temp_reg_74;
    __m128i m_temp_reg_75;
    __m128i m_temp_reg_76;
    __m128i m_temp_reg_77;
    __m128i m_coeff1, m_coeff2, m_coeff3, m_coeff4;

    WORD32 check_row_stage_1;   /* Lokesh */
    WORD32 check_row_stage_2;   /* Lokesh */

    __m128i m_rdng_factor;
    WORD32 i4_shift = IDCT_STG1_SHIFT;
    UNUSED(pi2_tmp);
    check_row_stage_1   = ((zero_rows & 0xF0) != 0xF0) ? 1 : 0;
    check_row_stage_2   = ((zero_cols & 0xF0) != 0xF0) ? 1 : 0;

    m_temp_reg_70 = _mm_loadu_si128((__m128i *)pi2_src);
    pi2_src += src_strd;
    m_temp_reg_71 = _mm_loadu_si128((__m128i *)pi2_src);
    pi2_src += src_strd;
    m_temp_reg_72 = _mm_loadu_si128((__m128i *)pi2_src);
    pi2_src += src_strd;
    m_temp_reg_73 = _mm_loadu_si128((__m128i *)pi2_src);
    pi2_src += src_strd;

    m_temp_reg_74 = _mm_loadu_si128((__m128i *)pi2_src);
    pi2_src += src_strd;
    m_temp_reg_75 = _mm_loadu_si128((__m128i *)pi2_src);
    pi2_src += src_strd;
    m_temp_reg_76 = _mm_loadu_si128((__m128i *)pi2_src);
    pi2_src += src_strd;
    m_temp_reg_77 = _mm_loadu_si128((__m128i *)pi2_src);

    if(!check_row_stage_2)
    {
        if(!check_row_stage_1)
        {
            /* ee0 is present in the registers m_temp_reg_10 and m_temp_reg_11 */
            /* ee1 is present in the registers m_temp_reg_12 and m_temp_reg_13 */
            {
                //Interleaving 0,4 row in 0 , 1 Rishab
                /*coef2 for m_temp_reg_12 and m_temp_reg_13 , coef1 for m_temp_reg_10 and m_temp_reg_11*/
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[3][0]);
                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[0][0]);

                m_temp_reg_0 = _mm_unpacklo_epi16(m_temp_reg_70, m_temp_reg_74);

                m_temp_reg_10 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                m_temp_reg_12 = _mm_madd_epi16(m_temp_reg_0, m_coeff2);

            }


            /* eo0 is present in the registers m_temp_reg_14 and m_temp_reg_15 */
            /* eo1 is present in the registers m_temp_reg_16 and m_temp_reg_17 */
            /* as upper 8 bytes are zeros so m_temp_reg_15 and m_temp_reg_17 are not used*/
            {

                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[1][0]); //sub 2B*36-6B*83 ,2T*36-6T*83
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[2][0]); //add 2B*83+6B*36 ,2T*83+6T*36

                /* Combining instructions to eliminate them based on zero_rows : Lokesh */
                //Interleaving 2,6 row in 4, 5 Rishab
                m_temp_reg_4 = _mm_unpacklo_epi16(m_temp_reg_72, m_temp_reg_76);

                m_temp_reg_16 = _mm_madd_epi16(m_temp_reg_4, m_coeff1);
                m_temp_reg_14 = _mm_madd_epi16(m_temp_reg_4, m_coeff2);


                /* Loading coeff for computing o0, o1, o2 and o3 in the next block */

                m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[2][0]);
                m_coeff4 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[3][0]);

                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[0][0]);
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[1][0]);



                /* e */

                /* e0 stored in m_temp_reg_40 and m_temp_reg_41 */
                /* e1 stored in m_temp_reg_42 and m_temp_reg_43 */
                /* e3 stored in m_temp_reg_46 and m_temp_reg_47 */
                /* e2 stored in m_temp_reg_44 and m_temp_reg_45 */
                m_temp_reg_42 = _mm_add_epi32(m_temp_reg_12, m_temp_reg_16);
                m_temp_reg_44 = _mm_sub_epi32(m_temp_reg_12, m_temp_reg_16);

                m_temp_reg_40 = _mm_add_epi32(m_temp_reg_10, m_temp_reg_14);
                m_temp_reg_46 = _mm_sub_epi32(m_temp_reg_10, m_temp_reg_14);

            }

            /* o */
            {

                /* o0 stored in m_temp_reg_30 and m_temp_reg_31 */
                {

                    m_temp_reg_60 = _mm_unpacklo_epi16(m_temp_reg_71, m_temp_reg_73);
                    //o0:1B*89+3B*75,5B*50+7B*18
                    m_temp_reg_30 = _mm_madd_epi16(m_temp_reg_60, m_coeff1);

                    m_rdng_factor = _mm_cvtsi32_si128((1 << (i4_shift - 1)));
                    m_rdng_factor = _mm_shuffle_epi32(m_rdng_factor, 0x0000);



                    /* Column 0 of destination computed here */
                    /* It is stored in m_temp_reg_50 */
                    /* Column 7 of destination computed here */
                    /* It is stored in m_temp_reg_57 */
                    /* Upper 8 bytes of both registers are zero due to zero_cols*/



                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_40, m_temp_reg_30);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_40, m_temp_reg_30);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_63 = _mm_setzero_si128();
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);

                    //o1:1B*75-3B*18,5B*89+7B*50
                    m_temp_reg_32 = _mm_madd_epi16(m_temp_reg_60, m_coeff3);

                    m_temp_reg_50 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_57 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_63);

                    /* Loading coeff for computing o2  in the next block */

                    m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[4][0]);
                    m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[5][0]);

                    /* o1 stored in m_temp_reg_32 and m_temp_reg_33 */



                    /* Column 1 of destination computed here */
                    /* It is stored in m_temp_reg_51 */
                    /* Column 6 of destination computed here */
                    /* It is stored in m_temp_reg_56 */

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_42, m_temp_reg_32);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_42, m_temp_reg_32);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);

                    //o2:1B*50-3B*89,5B*18+7B*75
                    m_temp_reg_34 = _mm_madd_epi16(m_temp_reg_60, m_coeff1);

                    m_temp_reg_51 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_56 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_63);


                    /* o2 stored in m_temp_reg_34 and m_temp_reg_35 */

                    /* Loading coeff for computing o3  in the next block */

                    m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[6][0]);
                    m_coeff4 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[7][0]);



                    /* Column 2 of destination computed here */
                    /* It is stored in m_temp_reg_52 */
                    /* Column 5 of destination computed here */
                    /* It is stored in m_temp_reg_55 */

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_44, m_temp_reg_34);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_44, m_temp_reg_34);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);

                    //o3:1B*18-3B*50,5B*75-7B*89
                    m_temp_reg_36 = _mm_madd_epi16(m_temp_reg_60, m_coeff3);

                    m_temp_reg_52 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_55 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_63);



                    /* o3 stored in m_temp_reg_36 and m_temp_reg_37 */



                    /* Column 3 of destination computed here */
                    /* It is stored in m_temp_reg_53 */
                    /* Column 4 of destination computed here */
                    /* It is stored in m_temp_reg_54 */

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_46, m_temp_reg_36);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_46, m_temp_reg_36);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);


                    m_temp_reg_53 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_54 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_63);
                }
            }

            /* Transpose of the destination 8x8 matrix done here */
            /* and ultimately stored in registers m_temp_reg_50 to m_temp_reg_57 */
            /* respectively */
            {
                m_temp_reg_10 = _mm_unpacklo_epi16(m_temp_reg_50, m_temp_reg_51);
                m_temp_reg_11 = _mm_unpacklo_epi16(m_temp_reg_52, m_temp_reg_53);
                m_temp_reg_0 = _mm_unpacklo_epi32(m_temp_reg_10, m_temp_reg_11);
                m_temp_reg_1 = _mm_unpackhi_epi32(m_temp_reg_10, m_temp_reg_11);

                m_temp_reg_12 = _mm_unpacklo_epi16(m_temp_reg_54, m_temp_reg_55);
                m_temp_reg_13 = _mm_unpacklo_epi16(m_temp_reg_56, m_temp_reg_57);

                m_temp_reg_4 = _mm_unpacklo_epi32(m_temp_reg_12, m_temp_reg_13);
                m_temp_reg_5 = _mm_unpackhi_epi32(m_temp_reg_12, m_temp_reg_13);

                m_temp_reg_50 = _mm_unpacklo_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_51 = _mm_unpackhi_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_52 = _mm_unpacklo_epi64(m_temp_reg_1, m_temp_reg_5);
                m_temp_reg_53 = _mm_unpackhi_epi64(m_temp_reg_1, m_temp_reg_5);

                m_temp_reg_54 = _mm_setzero_si128();
                m_temp_reg_55 = _mm_setzero_si128();
                m_temp_reg_56 = _mm_setzero_si128();
                m_temp_reg_57 = _mm_setzero_si128();
            }
        }
        else
        {
            /* ee0 is present in the registers m_temp_reg_10 and m_temp_reg_11 */
            /* ee1 is present in the registers m_temp_reg_12 and m_temp_reg_13 */
            {
                //Interleaving 0,4 row in 0 , 1 Rishab
                /*coef2 for m_temp_reg_12 and m_temp_reg_13 , coef1 for m_temp_reg_10 and m_temp_reg_11*/
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[3][0]);
                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[0][0]);

                m_temp_reg_0 = _mm_unpacklo_epi16(m_temp_reg_70, m_temp_reg_74);

                m_temp_reg_10 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                m_temp_reg_12 = _mm_madd_epi16(m_temp_reg_0, m_coeff2);

            }


            /* eo0 is present in the registers m_temp_reg_14 and m_temp_reg_15 */
            /* eo1 is present in the registers m_temp_reg_16 and m_temp_reg_17 */
            /* as upper 8 bytes are zeros so m_temp_reg_15 and m_temp_reg_17 are not used*/
            {

                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[1][0]); //sub 2B*36-6B*83 ,2T*36-6T*83
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[2][0]); //add 2B*83+6B*36 ,2T*83+6T*36

                /* Combining instructions to eliminate them based on zero_rows : Lokesh */
                //Interleaving 2,6 row in 4, 5 Rishab
                m_temp_reg_4 = _mm_unpacklo_epi16(m_temp_reg_72, m_temp_reg_76);

                m_temp_reg_16 = _mm_madd_epi16(m_temp_reg_4, m_coeff1);
                m_temp_reg_14 = _mm_madd_epi16(m_temp_reg_4, m_coeff2);


                /* Loading coeff for computing o0, o1, o2 and o3 in the next block */

                m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[2][0]);
                m_coeff4 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[3][0]);

                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[0][0]);
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[1][0]);



                /* e */

                /* e0 stored in m_temp_reg_40 and m_temp_reg_41 */
                /* e1 stored in m_temp_reg_42 and m_temp_reg_43 */
                /* e3 stored in m_temp_reg_46 and m_temp_reg_47 */
                /* e2 stored in m_temp_reg_44 and m_temp_reg_45 */
                m_temp_reg_42 = _mm_add_epi32(m_temp_reg_12, m_temp_reg_16);
                m_temp_reg_44 = _mm_sub_epi32(m_temp_reg_12, m_temp_reg_16);

                m_temp_reg_40 = _mm_add_epi32(m_temp_reg_10, m_temp_reg_14);
                m_temp_reg_46 = _mm_sub_epi32(m_temp_reg_10, m_temp_reg_14);

            }

            /* o */
            {

                /* o0 stored in m_temp_reg_30 and m_temp_reg_31 */
                {

                    m_temp_reg_60 = _mm_unpacklo_epi16(m_temp_reg_71, m_temp_reg_73);
                    m_temp_reg_64 = _mm_unpacklo_epi16(m_temp_reg_75, m_temp_reg_77);
                    //o0:1B*89+3B*75,5B*50+7B*18
                    m_temp_reg_20 = _mm_madd_epi16(m_temp_reg_60, m_coeff1);
                    m_temp_reg_24 = _mm_madd_epi16(m_temp_reg_64, m_coeff2);

                    m_rdng_factor = _mm_cvtsi32_si128((1 << (i4_shift - 1)));
                    m_rdng_factor = _mm_shuffle_epi32(m_rdng_factor, 0x0000);

                    m_temp_reg_30 = _mm_add_epi32(m_temp_reg_20, m_temp_reg_24);



                    /* Column 0 of destination computed here */
                    /* It is stored in m_temp_reg_50 */
                    /* Column 7 of destination computed here */
                    /* It is stored in m_temp_reg_57 */
                    /* Upper 8 bytes of both registers are zero due to zero_cols*/



                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_40, m_temp_reg_30);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_40, m_temp_reg_30);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_63 = _mm_setzero_si128();
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);

                    //o1:1B*75-3B*18,5B*89+7B*50
                    m_temp_reg_22 = _mm_madd_epi16(m_temp_reg_60, m_coeff3);
                    m_temp_reg_26 = _mm_madd_epi16(m_temp_reg_64, m_coeff4);

                    m_temp_reg_50 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_57 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_63);

                    /* Loading coeff for computing o2  in the next block */

                    m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[4][0]);
                    m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[5][0]);

                    /* o1 stored in m_temp_reg_32 and m_temp_reg_33 */
                    m_temp_reg_32 = _mm_sub_epi32(m_temp_reg_22, m_temp_reg_26);



                    /* Column 1 of destination computed here */
                    /* It is stored in m_temp_reg_51 */
                    /* Column 6 of destination computed here */
                    /* It is stored in m_temp_reg_56 */

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_42, m_temp_reg_32);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_42, m_temp_reg_32);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);

                    //o2:1B*50-3B*89,5B*18+7B*75
                    m_temp_reg_20 = _mm_madd_epi16(m_temp_reg_60, m_coeff1);
                    m_temp_reg_24 = _mm_madd_epi16(m_temp_reg_64, m_coeff2);

                    m_temp_reg_51 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_56 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_63);


                    /* o2 stored in m_temp_reg_34 and m_temp_reg_35 */

                    /* Loading coeff for computing o3  in the next block */

                    m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[6][0]);
                    m_coeff4 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[7][0]);

                    m_temp_reg_34 = _mm_add_epi32(m_temp_reg_20, m_temp_reg_24);


                    /* Column 2 of destination computed here */
                    /* It is stored in m_temp_reg_52 */
                    /* Column 5 of destination computed here */
                    /* It is stored in m_temp_reg_55 */

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_44, m_temp_reg_34);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_44, m_temp_reg_34);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);

                    //o3:1B*18-3B*50,5B*75-7B*89
                    m_temp_reg_22 = _mm_madd_epi16(m_temp_reg_60, m_coeff3);
                    m_temp_reg_26 = _mm_madd_epi16(m_temp_reg_64, m_coeff4);

                    m_temp_reg_52 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_55 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_63);



                    /* o3 stored in m_temp_reg_36 and m_temp_reg_37 */

                    m_temp_reg_36 = _mm_add_epi32(m_temp_reg_22, m_temp_reg_26);


                    /* Column 3 of destination computed here */
                    /* It is stored in m_temp_reg_53 */
                    /* Column 4 of destination computed here */
                    /* It is stored in m_temp_reg_54 */

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_46, m_temp_reg_36);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_46, m_temp_reg_36);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);


                    m_temp_reg_53 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_54 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_63);
                }
            }

            /* Transpose of the destination 8x8 matrix done here */
            /* and ultimately stored in registers m_temp_reg_50 to m_temp_reg_57 */
            /* respectively */
            {
                m_temp_reg_10 = _mm_unpacklo_epi16(m_temp_reg_50, m_temp_reg_51);
                m_temp_reg_11 = _mm_unpacklo_epi16(m_temp_reg_52, m_temp_reg_53);
                m_temp_reg_0 = _mm_unpacklo_epi32(m_temp_reg_10, m_temp_reg_11);
                m_temp_reg_1 = _mm_unpackhi_epi32(m_temp_reg_10, m_temp_reg_11);

                m_temp_reg_12 = _mm_unpacklo_epi16(m_temp_reg_54, m_temp_reg_55);
                m_temp_reg_13 = _mm_unpacklo_epi16(m_temp_reg_56, m_temp_reg_57);
                m_temp_reg_4 = _mm_unpacklo_epi32(m_temp_reg_12, m_temp_reg_13);
                m_temp_reg_5 = _mm_unpackhi_epi32(m_temp_reg_12, m_temp_reg_13);

                m_temp_reg_50 = _mm_unpacklo_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_51 = _mm_unpackhi_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_52 = _mm_unpacklo_epi64(m_temp_reg_1, m_temp_reg_5);
                m_temp_reg_53 = _mm_unpackhi_epi64(m_temp_reg_1, m_temp_reg_5);

                m_temp_reg_54 = _mm_setzero_si128();
                m_temp_reg_55 = _mm_setzero_si128();
                m_temp_reg_56 = _mm_setzero_si128();
                m_temp_reg_57 = _mm_setzero_si128();
            }
        }

        /* Stage 2 */
        i4_shift = IDCT_STG2_SHIFT;
        {
            /* ee0 is present in the registers m_temp_reg_10 and m_temp_reg_11 */
            /* ee1 is present in the registers m_temp_reg_12 and m_temp_reg_13 */
            {
                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q11[0][0]); //add
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q11[3][0]); //sub

                m_temp_reg_0 = _mm_unpacklo_epi16(m_temp_reg_50, m_temp_reg_54);
                m_temp_reg_1 = _mm_unpackhi_epi16(m_temp_reg_50, m_temp_reg_54);

                m_temp_reg_10 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                m_temp_reg_12 = _mm_madd_epi16(m_temp_reg_0, m_coeff2);
                m_temp_reg_11 = _mm_madd_epi16(m_temp_reg_1, m_coeff1);
                m_temp_reg_13 = _mm_madd_epi16(m_temp_reg_1, m_coeff2);


                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q11[1][0]);
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q11[2][0]);
            }


            /* eo0 is present in the registers m_temp_reg_14 and m_temp_reg_15 */
            /* eo1 is present in the registers m_temp_reg_16 and m_temp_reg_17 */
            {

                m_temp_reg_0 = _mm_unpacklo_epi16(m_temp_reg_52, m_temp_reg_56);
                m_temp_reg_1 = _mm_unpackhi_epi16(m_temp_reg_52, m_temp_reg_56);


                m_temp_reg_16 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                m_temp_reg_14 = _mm_madd_epi16(m_temp_reg_0, m_coeff2);
                m_temp_reg_17 = _mm_madd_epi16(m_temp_reg_1, m_coeff1);
                m_temp_reg_15 = _mm_madd_epi16(m_temp_reg_1, m_coeff2);

                /* Loading coeff for computing o0 in the next block */
                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[0][0]);


                m_temp_reg_0 = _mm_unpacklo_epi16(m_temp_reg_51, m_temp_reg_53);
                m_temp_reg_1 = _mm_unpackhi_epi16(m_temp_reg_51, m_temp_reg_53);



                /* e */

                /* e0 stored in m_temp_reg_40 and m_temp_reg_41 */
                /* e1 stored in m_temp_reg_42 and m_temp_reg_43 */
                /* e3 stored in m_temp_reg_46 and m_temp_reg_47 */
                /* e2 stored in m_temp_reg_44 and m_temp_reg_45 */
                m_temp_reg_42 = _mm_add_epi32(m_temp_reg_12, m_temp_reg_16);
                m_temp_reg_44 = _mm_sub_epi32(m_temp_reg_12, m_temp_reg_16);

                m_temp_reg_40 = _mm_add_epi32(m_temp_reg_10, m_temp_reg_14);
                m_temp_reg_46 = _mm_sub_epi32(m_temp_reg_10, m_temp_reg_14);

                m_temp_reg_43 = _mm_add_epi32(m_temp_reg_13, m_temp_reg_17);
                m_temp_reg_45 = _mm_sub_epi32(m_temp_reg_13, m_temp_reg_17);

                m_temp_reg_41 = _mm_add_epi32(m_temp_reg_11, m_temp_reg_15);
                m_temp_reg_47 = _mm_sub_epi32(m_temp_reg_11, m_temp_reg_15);

            }

            /* o */
            {

                /* o0 stored in m_temp_reg_30 and m_temp_reg_31 */
                {
                    //o0:1B*89+3B*75,1T*89+3T*75
                    m_temp_reg_30 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                    m_temp_reg_31 = _mm_madd_epi16(m_temp_reg_1, m_coeff1);

                    m_rdng_factor = _mm_cvtsi32_si128((1 << (i4_shift - 1)));
                    m_rdng_factor = _mm_shuffle_epi32(m_rdng_factor, 0x0000);
                    /* Loading coeff for computing o1 in the next block */
                    m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[2][0]);



                    /* Column 0 of destination computed here */
                    /* It is stored in m_temp_reg_50 */
                    /* Column 7 of destination computed here */
                    /* It is stored in m_temp_reg_57 */

                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_40, m_temp_reg_30);
                    m_temp_reg_6 = _mm_sub_epi32(m_temp_reg_40, m_temp_reg_30);

                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_41, m_temp_reg_31);
                    m_temp_reg_7 = _mm_sub_epi32(m_temp_reg_41, m_temp_reg_31);

                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_2, m_rdng_factor);
                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_3, m_rdng_factor);
                    m_temp_reg_6 = _mm_add_epi32(m_temp_reg_6, m_rdng_factor);
                    m_temp_reg_7 = _mm_add_epi32(m_temp_reg_7, m_rdng_factor);

                    //o1:1B*75-3B*18,1T*75-3T*18
                    m_temp_reg_32 = _mm_madd_epi16(m_temp_reg_0, m_coeff3);
                    m_temp_reg_33 = _mm_madd_epi16(m_temp_reg_1, m_coeff3);

                    m_temp_reg_2 = _mm_srai_epi32(m_temp_reg_2, i4_shift);
                    m_temp_reg_3 = _mm_srai_epi32(m_temp_reg_3, i4_shift);
                    m_temp_reg_6 = _mm_srai_epi32(m_temp_reg_6, i4_shift);
                    m_temp_reg_7 = _mm_srai_epi32(m_temp_reg_7, i4_shift);

                    m_temp_reg_50 = _mm_packs_epi32(m_temp_reg_2, m_temp_reg_3);
                    m_temp_reg_57 = _mm_packs_epi32(m_temp_reg_6, m_temp_reg_7);


                    /* o1 stored in m_temp_reg_32 and m_temp_reg_33 */


                    /* Loading coeff for computing o2  in the next block */
                    m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[4][0]);



                    /* Column 1 of destination computed here */
                    /* It is stored in m_temp_reg_51 */
                    /* Column 6 of destination computed here */
                    /* It is stored in m_temp_reg_56 */

                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_42, m_temp_reg_32);
                    m_temp_reg_6 = _mm_sub_epi32(m_temp_reg_42, m_temp_reg_32);

                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_43, m_temp_reg_33);
                    m_temp_reg_7 = _mm_sub_epi32(m_temp_reg_43, m_temp_reg_33);

                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_2, m_rdng_factor);
                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_3, m_rdng_factor);
                    m_temp_reg_6 = _mm_add_epi32(m_temp_reg_6, m_rdng_factor);
                    m_temp_reg_7 = _mm_add_epi32(m_temp_reg_7, m_rdng_factor);

                    //o2:1B*50-3B*89,5T*18+7T*75.
                    m_temp_reg_34 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                    m_temp_reg_35 = _mm_madd_epi16(m_temp_reg_1, m_coeff1);

                    m_temp_reg_2 = _mm_srai_epi32(m_temp_reg_2, i4_shift);
                    m_temp_reg_3 = _mm_srai_epi32(m_temp_reg_3, i4_shift);
                    m_temp_reg_6 = _mm_srai_epi32(m_temp_reg_6, i4_shift);
                    m_temp_reg_7 = _mm_srai_epi32(m_temp_reg_7, i4_shift);

                    m_temp_reg_51 = _mm_packs_epi32(m_temp_reg_2, m_temp_reg_3);
                    m_temp_reg_56 = _mm_packs_epi32(m_temp_reg_6, m_temp_reg_7);


                    /* o2 stored in m_temp_reg_34 and m_temp_reg_35 */

                    /* Loading coeff for computing o3  in the next block */

                    m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[6][0]);


                    /* Column 2 of destination computed here */
                    /* It is stored in m_temp_reg_52 */
                    /* Column 5 of destination computed here */
                    /* It is stored in m_temp_reg_55 */

                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_44, m_temp_reg_34);
                    m_temp_reg_6 = _mm_sub_epi32(m_temp_reg_44, m_temp_reg_34);

                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_45, m_temp_reg_35);
                    m_temp_reg_7 = _mm_sub_epi32(m_temp_reg_45, m_temp_reg_35);

                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_2, m_rdng_factor);
                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_3, m_rdng_factor);
                    m_temp_reg_6 = _mm_add_epi32(m_temp_reg_6, m_rdng_factor);
                    m_temp_reg_7 = _mm_add_epi32(m_temp_reg_7, m_rdng_factor);

                    //o3:1B*18-3B*50,1T*18-3T*50
                    m_temp_reg_36 = _mm_madd_epi16(m_temp_reg_0, m_coeff3);
                    m_temp_reg_37 = _mm_madd_epi16(m_temp_reg_1, m_coeff3);

                    m_temp_reg_2 = _mm_srai_epi32(m_temp_reg_2, i4_shift);
                    m_temp_reg_3 = _mm_srai_epi32(m_temp_reg_3, i4_shift);
                    m_temp_reg_6 = _mm_srai_epi32(m_temp_reg_6, i4_shift);
                    m_temp_reg_7 = _mm_srai_epi32(m_temp_reg_7, i4_shift);


                    m_temp_reg_52 = _mm_packs_epi32(m_temp_reg_2, m_temp_reg_3);
                    m_temp_reg_55 = _mm_packs_epi32(m_temp_reg_6, m_temp_reg_7);



                    /* o3 stored in m_temp_reg_36 and m_temp_reg_37 */


                    /* Column 3 of destination computed here */
                    /* It is stored in m_temp_reg_53 */
                    /* Column 4 of destination computed here */
                    /* It is stored in m_temp_reg_54 */

                    m_temp_reg_20 = _mm_add_epi32(m_temp_reg_46, m_temp_reg_36);
                    m_temp_reg_22 = _mm_sub_epi32(m_temp_reg_46, m_temp_reg_36);

                    m_temp_reg_21 = _mm_add_epi32(m_temp_reg_47, m_temp_reg_37);
                    m_temp_reg_23 = _mm_sub_epi32(m_temp_reg_47, m_temp_reg_37);

                    m_temp_reg_20 = _mm_add_epi32(m_temp_reg_20, m_rdng_factor);
                    m_temp_reg_21 = _mm_add_epi32(m_temp_reg_21, m_rdng_factor);
                    m_temp_reg_22 = _mm_add_epi32(m_temp_reg_22, m_rdng_factor);
                    m_temp_reg_23 = _mm_add_epi32(m_temp_reg_23, m_rdng_factor);

                    m_temp_reg_20 = _mm_srai_epi32(m_temp_reg_20, i4_shift);
                    m_temp_reg_21 = _mm_srai_epi32(m_temp_reg_21, i4_shift);
                    m_temp_reg_22 = _mm_srai_epi32(m_temp_reg_22, i4_shift);
                    m_temp_reg_23 = _mm_srai_epi32(m_temp_reg_23, i4_shift);

                    m_temp_reg_53 = _mm_packs_epi32(m_temp_reg_20, m_temp_reg_21);
                    m_temp_reg_54 = _mm_packs_epi32(m_temp_reg_22, m_temp_reg_23);
                }
            }

            /* Transpose of the destination 8x8 matrix done here */
            /* and ultimately stored in registers m_temp_reg_50 to m_temp_reg_57 */
            /* respectively */
            {
                m_temp_reg_10 = _mm_unpacklo_epi16(m_temp_reg_50, m_temp_reg_51);
                m_temp_reg_11 = _mm_unpacklo_epi16(m_temp_reg_52, m_temp_reg_53);
                m_temp_reg_14 = _mm_unpackhi_epi16(m_temp_reg_50, m_temp_reg_51);
                m_temp_reg_15 = _mm_unpackhi_epi16(m_temp_reg_52, m_temp_reg_53);
                m_temp_reg_0 = _mm_unpacklo_epi32(m_temp_reg_10, m_temp_reg_11);
                m_temp_reg_1 = _mm_unpackhi_epi32(m_temp_reg_10, m_temp_reg_11);
                m_temp_reg_2 = _mm_unpacklo_epi32(m_temp_reg_14, m_temp_reg_15);
                m_temp_reg_3 = _mm_unpackhi_epi32(m_temp_reg_14, m_temp_reg_15);

                m_temp_reg_12 = _mm_unpacklo_epi16(m_temp_reg_54, m_temp_reg_55);
                m_temp_reg_13 = _mm_unpacklo_epi16(m_temp_reg_56, m_temp_reg_57);
                m_temp_reg_16 = _mm_unpackhi_epi16(m_temp_reg_54, m_temp_reg_55);
                m_temp_reg_17 = _mm_unpackhi_epi16(m_temp_reg_56, m_temp_reg_57);
                m_temp_reg_4 = _mm_unpacklo_epi32(m_temp_reg_12, m_temp_reg_13);
                m_temp_reg_5 = _mm_unpackhi_epi32(m_temp_reg_12, m_temp_reg_13);
                m_temp_reg_6 = _mm_unpacklo_epi32(m_temp_reg_16, m_temp_reg_17);
                m_temp_reg_7 = _mm_unpackhi_epi32(m_temp_reg_16, m_temp_reg_17);
                m_temp_reg_10 = _mm_unpacklo_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_11 = _mm_unpackhi_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_12 = _mm_unpacklo_epi64(m_temp_reg_1, m_temp_reg_5);
                m_temp_reg_13 = _mm_unpackhi_epi64(m_temp_reg_1, m_temp_reg_5);

                m_temp_reg_14 = _mm_unpacklo_epi64(m_temp_reg_2, m_temp_reg_6);
                m_temp_reg_15 = _mm_unpackhi_epi64(m_temp_reg_2, m_temp_reg_6);
                m_temp_reg_16 = _mm_unpacklo_epi64(m_temp_reg_3, m_temp_reg_7);
                m_temp_reg_17 = _mm_unpackhi_epi64(m_temp_reg_3, m_temp_reg_7);
            }

            /* Recon and store */
            {
                m_temp_reg_0 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_1 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_2 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_3 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_4 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_5 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_6 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_7 = _mm_loadl_epi64((__m128i *)pu1_pred);

                m_temp_reg_50 = _mm_setzero_si128();
                m_temp_reg_0 = _mm_unpacklo_epi8(m_temp_reg_0, m_temp_reg_50);
                m_temp_reg_1 = _mm_unpacklo_epi8(m_temp_reg_1, m_temp_reg_50);
                m_temp_reg_2 = _mm_unpacklo_epi8(m_temp_reg_2, m_temp_reg_50);
                m_temp_reg_3 = _mm_unpacklo_epi8(m_temp_reg_3, m_temp_reg_50);
                m_temp_reg_4 = _mm_unpacklo_epi8(m_temp_reg_4, m_temp_reg_50);
                m_temp_reg_5 = _mm_unpacklo_epi8(m_temp_reg_5, m_temp_reg_50);
                m_temp_reg_6 = _mm_unpacklo_epi8(m_temp_reg_6, m_temp_reg_50);
                m_temp_reg_7 = _mm_unpacklo_epi8(m_temp_reg_7, m_temp_reg_50);

                m_temp_reg_50 = _mm_add_epi16(m_temp_reg_10, m_temp_reg_0);
                m_temp_reg_51 = _mm_add_epi16(m_temp_reg_11, m_temp_reg_1);
                m_temp_reg_52 = _mm_add_epi16(m_temp_reg_12, m_temp_reg_2);
                m_temp_reg_53 = _mm_add_epi16(m_temp_reg_13, m_temp_reg_3);
                m_temp_reg_54 = _mm_add_epi16(m_temp_reg_14, m_temp_reg_4);
                m_temp_reg_55 = _mm_add_epi16(m_temp_reg_15, m_temp_reg_5);
                m_temp_reg_56 = _mm_add_epi16(m_temp_reg_16, m_temp_reg_6);
                m_temp_reg_57 = _mm_add_epi16(m_temp_reg_17, m_temp_reg_7);

                m_temp_reg_50 = _mm_packus_epi16(m_temp_reg_50, m_temp_reg_50);
                m_temp_reg_51 = _mm_packus_epi16(m_temp_reg_51, m_temp_reg_51);
                m_temp_reg_52 = _mm_packus_epi16(m_temp_reg_52, m_temp_reg_52);
                m_temp_reg_53 = _mm_packus_epi16(m_temp_reg_53, m_temp_reg_53);
                m_temp_reg_54 = _mm_packus_epi16(m_temp_reg_54, m_temp_reg_54);
                m_temp_reg_55 = _mm_packus_epi16(m_temp_reg_55, m_temp_reg_55);
                m_temp_reg_56 = _mm_packus_epi16(m_temp_reg_56, m_temp_reg_56);
                m_temp_reg_57 = _mm_packus_epi16(m_temp_reg_57, m_temp_reg_57);

                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_50);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_51);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_52);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_53);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_54);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_55);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_56);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_57);
                pu1_dst += dst_strd;
            }
        }
    }
    else

    {

        /* ee0 is present in the registers m_temp_reg_10 and m_temp_reg_11 */
        /* ee1 is present in the registers m_temp_reg_12 and m_temp_reg_13 */
        if(!check_row_stage_1)
        {
            /* ee0 is present in the registers m_temp_reg_10 and m_temp_reg_11 */
            /* ee1 is present in the registers m_temp_reg_12 and m_temp_reg_13 */
            {
                //Interleaving 0,4 row in 0 , 1 Rishab
                /*coef2 for m_temp_reg_12 and m_temp_reg_13 , coef1 for m_temp_reg_10 and m_temp_reg_11*/
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[3][0]);
                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[0][0]);

                m_temp_reg_0 = _mm_unpacklo_epi16(m_temp_reg_70, m_temp_reg_74);
                m_temp_reg_1 = _mm_unpackhi_epi16(m_temp_reg_70, m_temp_reg_74);

                m_temp_reg_10 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                m_temp_reg_12 = _mm_madd_epi16(m_temp_reg_0, m_coeff2);


                m_temp_reg_11 = _mm_madd_epi16(m_temp_reg_1, m_coeff1);
                m_temp_reg_13 = _mm_madd_epi16(m_temp_reg_1, m_coeff2);
            }


            /* eo0 is present in the registers m_temp_reg_14 and m_temp_reg_15 */
            /* eo1 is present in the registers m_temp_reg_16 and m_temp_reg_17 */
            {

                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[1][0]); //sub 2B*36-6B*83 ,2T*36-6T*83
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[2][0]); //add 2B*83+6B*36 ,2T*83+6T*36

                /* Combining instructions to eliminate them based on zero_rows : Lokesh */
                //Interleaving 2,6 row in 4, 5 Rishab
                m_temp_reg_4 = _mm_unpacklo_epi16(m_temp_reg_72, m_temp_reg_76);
                m_temp_reg_5 = _mm_unpackhi_epi16(m_temp_reg_72, m_temp_reg_76);

                m_temp_reg_16 = _mm_madd_epi16(m_temp_reg_4, m_coeff1);
                m_temp_reg_14 = _mm_madd_epi16(m_temp_reg_4, m_coeff2);

                m_temp_reg_17 = _mm_madd_epi16(m_temp_reg_5, m_coeff1);
                m_temp_reg_15 = _mm_madd_epi16(m_temp_reg_5, m_coeff2);



                /* Loading coeff for computing o0, o1, o2 and o3 in the next block */

                m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[2][0]);
                //m_coeff4 = _mm_loadu_si128((__m128i *) &gai2_impeg2_idct_odd_8_q15[3][0]);

                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[0][0]);
                //m_coeff2 = _mm_loadu_si128((__m128i *) &gai2_impeg2_idct_odd_8_q15[1][0]);

            }

            /* e */
            {
                /* e0 stored in m_temp_reg_40 and m_temp_reg_41 */
                /* e1 stored in m_temp_reg_42 and m_temp_reg_43 */
                /* e3 stored in m_temp_reg_46 and m_temp_reg_47 */
                /* e2 stored in m_temp_reg_44 and m_temp_reg_45 */
                m_temp_reg_42 = _mm_add_epi32(m_temp_reg_12, m_temp_reg_16);
                m_temp_reg_44 = _mm_sub_epi32(m_temp_reg_12, m_temp_reg_16);

                m_temp_reg_40 = _mm_add_epi32(m_temp_reg_10, m_temp_reg_14);
                m_temp_reg_46 = _mm_sub_epi32(m_temp_reg_10, m_temp_reg_14);

                m_temp_reg_43 = _mm_add_epi32(m_temp_reg_13, m_temp_reg_17);
                m_temp_reg_45 = _mm_sub_epi32(m_temp_reg_13, m_temp_reg_17);

                m_temp_reg_41 = _mm_add_epi32(m_temp_reg_11, m_temp_reg_15);
                m_temp_reg_47 = _mm_sub_epi32(m_temp_reg_11, m_temp_reg_15);

            }

            /* o */
            {

                /* o0 stored in m_temp_reg_30 and m_temp_reg_31 */
                {

                    m_temp_reg_60 = _mm_unpacklo_epi16(m_temp_reg_71, m_temp_reg_73);
                    m_temp_reg_61 = _mm_unpackhi_epi16(m_temp_reg_71, m_temp_reg_73);
                    //o0:1B*89+3B*75,1T*89+3T*75
                    m_temp_reg_30 = _mm_madd_epi16(m_temp_reg_60, m_coeff1);
                    m_temp_reg_31 = _mm_madd_epi16(m_temp_reg_61, m_coeff1);

                    m_rdng_factor = _mm_cvtsi32_si128((1 << (i4_shift - 1)));
                    m_rdng_factor = _mm_shuffle_epi32(m_rdng_factor, 0x0000);

                }

                /* Column 0 of destination computed here */
                /* It is stored in m_temp_reg_50 */
                /* Column 7 of destination computed here */
                /* It is stored in m_temp_reg_57 */
                {


                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_40, m_temp_reg_30);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_40, m_temp_reg_30);

                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_41, m_temp_reg_31);
                    m_temp_reg_67 = _mm_sub_epi32(m_temp_reg_41, m_temp_reg_31);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_63, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);
                    m_temp_reg_67 = _mm_add_epi32(m_temp_reg_67, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_63 = _mm_srai_epi32(m_temp_reg_63, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);
                    m_temp_reg_67 = _mm_srai_epi32(m_temp_reg_67, i4_shift);

                    //o1:1B*75-3B*18,1T*75-3T*18,5B*89+7B*50,5T*89+7T*50
                    m_temp_reg_32 = _mm_madd_epi16(m_temp_reg_60, m_coeff3);
                    m_temp_reg_33 = _mm_madd_epi16(m_temp_reg_61, m_coeff3);

                    m_temp_reg_50 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_57 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_67);

                    /* Loading coeff for computing o2  in the next block */

                    m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[4][0]);

                }

                /* Column 1 of destination computed here */
                /* It is stored in m_temp_reg_51 */
                /* Column 6 of destination computed here */
                /* It is stored in m_temp_reg_56 */
                {
                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_42, m_temp_reg_32);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_42, m_temp_reg_32);

                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_43, m_temp_reg_33);
                    m_temp_reg_67 = _mm_sub_epi32(m_temp_reg_43, m_temp_reg_33);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);
                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_63, m_rdng_factor);
                    m_temp_reg_67 = _mm_add_epi32(m_temp_reg_67, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_63 = _mm_srai_epi32(m_temp_reg_63, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);
                    m_temp_reg_67 = _mm_srai_epi32(m_temp_reg_67, i4_shift);

                    //o2:1B*50-3B*89,1T*50-3T*89
                    m_temp_reg_34 = _mm_madd_epi16(m_temp_reg_60, m_coeff1);
                    m_temp_reg_35 = _mm_madd_epi16(m_temp_reg_61, m_coeff1);

                    m_temp_reg_51 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_56 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_67);


                    /* o2 stored in m_temp_reg_34 and m_temp_reg_35 */


                    /* Loading coeff for computing o3  in the next block */

                    m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[6][0]);

                }

                /* Column 2 of destination computed here */
                /* It is stored in m_temp_reg_52 */
                /* Column 5 of destination computed here */
                /* It is stored in m_temp_reg_55 */
                {
                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_44, m_temp_reg_34);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_44, m_temp_reg_34);

                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_45, m_temp_reg_35);
                    m_temp_reg_67 = _mm_sub_epi32(m_temp_reg_45, m_temp_reg_35);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_63, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);
                    m_temp_reg_67 = _mm_add_epi32(m_temp_reg_67, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_63 = _mm_srai_epi32(m_temp_reg_63, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);
                    m_temp_reg_67 = _mm_srai_epi32(m_temp_reg_67, i4_shift);

                    //o3:1B*18-3B*50,1T*18-3T*50
                    m_temp_reg_36 = _mm_madd_epi16(m_temp_reg_60, m_coeff3);
                    m_temp_reg_37 = _mm_madd_epi16(m_temp_reg_61, m_coeff3);

                    m_temp_reg_52 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_55 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_67);



                    /* o3 stored in m_temp_reg_36 and m_temp_reg_37 */


                }

                /* Column 3 of destination computed here */
                /* It is stored in m_temp_reg_53 */
                /* Column 4 of destination computed here */
                /* It is stored in m_temp_reg_54 */
                {
                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_46, m_temp_reg_36);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_46, m_temp_reg_36);

                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_47, m_temp_reg_37);
                    m_temp_reg_67 = _mm_sub_epi32(m_temp_reg_47, m_temp_reg_37);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_63, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);
                    m_temp_reg_67 = _mm_add_epi32(m_temp_reg_67, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_63 = _mm_srai_epi32(m_temp_reg_63, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);
                    m_temp_reg_67 = _mm_srai_epi32(m_temp_reg_67, i4_shift);

                    m_temp_reg_53 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_54 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_67);
                }
            }

            /* Transpose of the destination 8x8 matrix done here */
            /* and ultimately stored in registers m_temp_reg_50 to m_temp_reg_57 */
            /* respectively */
            {


                m_temp_reg_10 = _mm_unpacklo_epi16(m_temp_reg_50, m_temp_reg_51);
                m_temp_reg_11 = _mm_unpacklo_epi16(m_temp_reg_52, m_temp_reg_53);
                m_temp_reg_14 = _mm_unpackhi_epi16(m_temp_reg_50, m_temp_reg_51);
                m_temp_reg_15 = _mm_unpackhi_epi16(m_temp_reg_52, m_temp_reg_53);
                m_temp_reg_0 = _mm_unpacklo_epi32(m_temp_reg_10, m_temp_reg_11);
                m_temp_reg_1 = _mm_unpackhi_epi32(m_temp_reg_10, m_temp_reg_11);
                m_temp_reg_2 = _mm_unpacklo_epi32(m_temp_reg_14, m_temp_reg_15);
                m_temp_reg_3 = _mm_unpackhi_epi32(m_temp_reg_14, m_temp_reg_15);

                m_temp_reg_12 = _mm_unpacklo_epi16(m_temp_reg_54, m_temp_reg_55);
                m_temp_reg_13 = _mm_unpacklo_epi16(m_temp_reg_56, m_temp_reg_57);
                m_temp_reg_16 = _mm_unpackhi_epi16(m_temp_reg_54, m_temp_reg_55);
                m_temp_reg_17 = _mm_unpackhi_epi16(m_temp_reg_56, m_temp_reg_57);
                m_temp_reg_4 = _mm_unpacklo_epi32(m_temp_reg_12, m_temp_reg_13);
                m_temp_reg_5 = _mm_unpackhi_epi32(m_temp_reg_12, m_temp_reg_13);
                m_temp_reg_6 = _mm_unpacklo_epi32(m_temp_reg_16, m_temp_reg_17);
                m_temp_reg_7 = _mm_unpackhi_epi32(m_temp_reg_16, m_temp_reg_17);

                m_temp_reg_50 = _mm_unpacklo_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_51 = _mm_unpackhi_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_52 = _mm_unpacklo_epi64(m_temp_reg_1, m_temp_reg_5);
                m_temp_reg_53 = _mm_unpackhi_epi64(m_temp_reg_1, m_temp_reg_5);

                m_temp_reg_54 = _mm_unpacklo_epi64(m_temp_reg_2, m_temp_reg_6);
                m_temp_reg_55 = _mm_unpackhi_epi64(m_temp_reg_2, m_temp_reg_6);
                m_temp_reg_56 = _mm_unpacklo_epi64(m_temp_reg_3, m_temp_reg_7);
                m_temp_reg_57 = _mm_unpackhi_epi64(m_temp_reg_3, m_temp_reg_7);
            }
        }
        else
        {

            /* ee0 is present in the registers m_temp_reg_10 and m_temp_reg_11 */
            /* ee1 is present in the registers m_temp_reg_12 and m_temp_reg_13 */
            {
                //Interleaving 0,4 row in 0 , 1 Rishab
                /*coef2 for m_temp_reg_12 and m_temp_reg_13 , coef1 for m_temp_reg_10 and m_temp_reg_11*/
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[3][0]);
                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[0][0]);

                m_temp_reg_0 = _mm_unpacklo_epi16(m_temp_reg_70, m_temp_reg_74);
                m_temp_reg_1 = _mm_unpackhi_epi16(m_temp_reg_70, m_temp_reg_74);

                m_temp_reg_10 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                m_temp_reg_12 = _mm_madd_epi16(m_temp_reg_0, m_coeff2);


                m_temp_reg_11 = _mm_madd_epi16(m_temp_reg_1, m_coeff1);
                m_temp_reg_13 = _mm_madd_epi16(m_temp_reg_1, m_coeff2);
            }


            /* eo0 is present in the registers m_temp_reg_14 and m_temp_reg_15 */
            /* eo1 is present in the registers m_temp_reg_16 and m_temp_reg_17 */
            {

                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[1][0]); //sub 2B*36-6B*83 ,2T*36-6T*83
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q15[2][0]); //add 2B*83+6B*36 ,2T*83+6T*36

                /* Combining instructions to eliminate them based on zero_rows : Lokesh */
                //Interleaving 2,6 row in 4, 5 Rishab
                m_temp_reg_4 = _mm_unpacklo_epi16(m_temp_reg_72, m_temp_reg_76);
                m_temp_reg_5 = _mm_unpackhi_epi16(m_temp_reg_72, m_temp_reg_76);

                m_temp_reg_16 = _mm_madd_epi16(m_temp_reg_4, m_coeff1);
                m_temp_reg_14 = _mm_madd_epi16(m_temp_reg_4, m_coeff2);

                m_temp_reg_17 = _mm_madd_epi16(m_temp_reg_5, m_coeff1);
                m_temp_reg_15 = _mm_madd_epi16(m_temp_reg_5, m_coeff2);



                /* Loading coeff for computing o0, o1, o2 and o3 in the next block */

                m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[2][0]);
                m_coeff4 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[3][0]);

                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[0][0]);
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[1][0]);

            }

            /* e */
            {
                /* e0 stored in m_temp_reg_40 and m_temp_reg_41 */
                /* e1 stored in m_temp_reg_42 and m_temp_reg_43 */
                /* e3 stored in m_temp_reg_46 and m_temp_reg_47 */
                /* e2 stored in m_temp_reg_44 and m_temp_reg_45 */
                m_temp_reg_42 = _mm_add_epi32(m_temp_reg_12, m_temp_reg_16);
                m_temp_reg_44 = _mm_sub_epi32(m_temp_reg_12, m_temp_reg_16);

                m_temp_reg_40 = _mm_add_epi32(m_temp_reg_10, m_temp_reg_14);
                m_temp_reg_46 = _mm_sub_epi32(m_temp_reg_10, m_temp_reg_14);

                m_temp_reg_43 = _mm_add_epi32(m_temp_reg_13, m_temp_reg_17);
                m_temp_reg_45 = _mm_sub_epi32(m_temp_reg_13, m_temp_reg_17);

                m_temp_reg_41 = _mm_add_epi32(m_temp_reg_11, m_temp_reg_15);
                m_temp_reg_47 = _mm_sub_epi32(m_temp_reg_11, m_temp_reg_15);

            }

            /* o */
            {

                /* o0 stored in m_temp_reg_30 and m_temp_reg_31 */
                {

                    m_temp_reg_60 = _mm_unpacklo_epi16(m_temp_reg_71, m_temp_reg_73);
                    m_temp_reg_61 = _mm_unpackhi_epi16(m_temp_reg_71, m_temp_reg_73);
                    m_temp_reg_64 = _mm_unpacklo_epi16(m_temp_reg_75, m_temp_reg_77);
                    m_temp_reg_65 = _mm_unpackhi_epi16(m_temp_reg_75, m_temp_reg_77);
                    //o0:1B*89+3B*75,1T*89+3T*75,5B*50+7B*18,5T*50+7T*18
                    m_temp_reg_20 = _mm_madd_epi16(m_temp_reg_60, m_coeff1);
                    m_temp_reg_21 = _mm_madd_epi16(m_temp_reg_61, m_coeff1);
                    m_temp_reg_24 = _mm_madd_epi16(m_temp_reg_64, m_coeff2);
                    m_temp_reg_25 = _mm_madd_epi16(m_temp_reg_65, m_coeff2);


                    m_rdng_factor = _mm_cvtsi32_si128((1 << (i4_shift - 1)));
                    m_rdng_factor = _mm_shuffle_epi32(m_rdng_factor, 0x0000);

                    m_temp_reg_30 = _mm_add_epi32(m_temp_reg_20, m_temp_reg_24);
                    m_temp_reg_31 = _mm_add_epi32(m_temp_reg_21, m_temp_reg_25);
                }

                /* Column 0 of destination computed here */
                /* It is stored in m_temp_reg_50 */
                /* Column 7 of destination computed here */
                /* It is stored in m_temp_reg_57 */
                {


                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_40, m_temp_reg_30);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_40, m_temp_reg_30);

                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_41, m_temp_reg_31);
                    m_temp_reg_67 = _mm_sub_epi32(m_temp_reg_41, m_temp_reg_31);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_63, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);
                    m_temp_reg_67 = _mm_add_epi32(m_temp_reg_67, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_63 = _mm_srai_epi32(m_temp_reg_63, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);
                    m_temp_reg_67 = _mm_srai_epi32(m_temp_reg_67, i4_shift);

                    //o1:1B*75-3B*18,1T*75-3T*18,5B*89+7B*50,5T*89+7T*50
                    m_temp_reg_22 = _mm_madd_epi16(m_temp_reg_60, m_coeff3);
                    m_temp_reg_26 = _mm_madd_epi16(m_temp_reg_64, m_coeff4);
                    m_temp_reg_23 = _mm_madd_epi16(m_temp_reg_61, m_coeff3);
                    m_temp_reg_27 = _mm_madd_epi16(m_temp_reg_65, m_coeff4);

                    m_temp_reg_50 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_57 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_67);

                    /* Loading coeff for computing o2  in the next block */

                    m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[4][0]);
                    m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[5][0]);

                    /* o1 stored in m_temp_reg_32 and m_temp_reg_33 */
                    m_temp_reg_32 = _mm_sub_epi32(m_temp_reg_22, m_temp_reg_26);
                    m_temp_reg_33 = _mm_sub_epi32(m_temp_reg_23, m_temp_reg_27);
                }

                /* Column 1 of destination computed here */
                /* It is stored in m_temp_reg_51 */
                /* Column 6 of destination computed here */
                /* It is stored in m_temp_reg_56 */
                {
                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_42, m_temp_reg_32);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_42, m_temp_reg_32);

                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_43, m_temp_reg_33);
                    m_temp_reg_67 = _mm_sub_epi32(m_temp_reg_43, m_temp_reg_33);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);
                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_63, m_rdng_factor);
                    m_temp_reg_67 = _mm_add_epi32(m_temp_reg_67, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_63 = _mm_srai_epi32(m_temp_reg_63, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);
                    m_temp_reg_67 = _mm_srai_epi32(m_temp_reg_67, i4_shift);

                    //o2:1B*50-3B*89,1T*50-3T*89,5B*18+7B*75,5T*18+7T*75
                    m_temp_reg_20 = _mm_madd_epi16(m_temp_reg_60, m_coeff1);
                    m_temp_reg_24 = _mm_madd_epi16(m_temp_reg_64, m_coeff2);
                    m_temp_reg_21 = _mm_madd_epi16(m_temp_reg_61, m_coeff1);
                    m_temp_reg_25 = _mm_madd_epi16(m_temp_reg_65, m_coeff2);

                    m_temp_reg_51 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_56 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_67);


                    /* o2 stored in m_temp_reg_34 and m_temp_reg_35 */


                    /* Loading coeff for computing o3  in the next block */

                    m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[6][0]);
                    m_coeff4 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q15[7][0]);

                    m_temp_reg_34 = _mm_add_epi32(m_temp_reg_20, m_temp_reg_24);
                    m_temp_reg_35 = _mm_add_epi32(m_temp_reg_21, m_temp_reg_25);
                }

                /* Column 2 of destination computed here */
                /* It is stored in m_temp_reg_52 */
                /* Column 5 of destination computed here */
                /* It is stored in m_temp_reg_55 */
                {
                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_44, m_temp_reg_34);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_44, m_temp_reg_34);

                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_45, m_temp_reg_35);
                    m_temp_reg_67 = _mm_sub_epi32(m_temp_reg_45, m_temp_reg_35);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_63, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);
                    m_temp_reg_67 = _mm_add_epi32(m_temp_reg_67, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_63 = _mm_srai_epi32(m_temp_reg_63, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);
                    m_temp_reg_67 = _mm_srai_epi32(m_temp_reg_67, i4_shift);

                    //o3:1B*18-3B*50,1T*18-3T*50,5B*75-7B*89,5T*75-7T*89
                    m_temp_reg_22 = _mm_madd_epi16(m_temp_reg_60, m_coeff3);
                    m_temp_reg_26 = _mm_madd_epi16(m_temp_reg_64, m_coeff4);
                    m_temp_reg_23 = _mm_madd_epi16(m_temp_reg_61, m_coeff3);
                    m_temp_reg_27 = _mm_madd_epi16(m_temp_reg_65, m_coeff4);

                    m_temp_reg_52 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_55 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_67);



                    /* o3 stored in m_temp_reg_36 and m_temp_reg_37 */


                    m_temp_reg_36 = _mm_add_epi32(m_temp_reg_22, m_temp_reg_26);
                    m_temp_reg_37 = _mm_add_epi32(m_temp_reg_23, m_temp_reg_27);
                }

                /* Column 3 of destination computed here */
                /* It is stored in m_temp_reg_53 */
                /* Column 4 of destination computed here */
                /* It is stored in m_temp_reg_54 */
                {
                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_46, m_temp_reg_36);
                    m_temp_reg_66 = _mm_sub_epi32(m_temp_reg_46, m_temp_reg_36);

                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_47, m_temp_reg_37);
                    m_temp_reg_67 = _mm_sub_epi32(m_temp_reg_47, m_temp_reg_37);

                    m_temp_reg_62 = _mm_add_epi32(m_temp_reg_62, m_rdng_factor);
                    m_temp_reg_63 = _mm_add_epi32(m_temp_reg_63, m_rdng_factor);
                    m_temp_reg_66 = _mm_add_epi32(m_temp_reg_66, m_rdng_factor);
                    m_temp_reg_67 = _mm_add_epi32(m_temp_reg_67, m_rdng_factor);

                    m_temp_reg_62 = _mm_srai_epi32(m_temp_reg_62, i4_shift);
                    m_temp_reg_63 = _mm_srai_epi32(m_temp_reg_63, i4_shift);
                    m_temp_reg_66 = _mm_srai_epi32(m_temp_reg_66, i4_shift);
                    m_temp_reg_67 = _mm_srai_epi32(m_temp_reg_67, i4_shift);

                    m_temp_reg_53 = _mm_packs_epi32(m_temp_reg_62, m_temp_reg_63);
                    m_temp_reg_54 = _mm_packs_epi32(m_temp_reg_66, m_temp_reg_67);
                }
            }

            /* Transpose of the destination 8x8 matrix done here */
            /* and ultimately stored in registers m_temp_reg_50 to m_temp_reg_57 */
            /* respectively */
            {


                m_temp_reg_10 = _mm_unpacklo_epi16(m_temp_reg_50, m_temp_reg_51);
                m_temp_reg_11 = _mm_unpacklo_epi16(m_temp_reg_52, m_temp_reg_53);
                m_temp_reg_14 = _mm_unpackhi_epi16(m_temp_reg_50, m_temp_reg_51);
                m_temp_reg_15 = _mm_unpackhi_epi16(m_temp_reg_52, m_temp_reg_53);
                m_temp_reg_0 = _mm_unpacklo_epi32(m_temp_reg_10, m_temp_reg_11);
                m_temp_reg_1 = _mm_unpackhi_epi32(m_temp_reg_10, m_temp_reg_11);
                m_temp_reg_2 = _mm_unpacklo_epi32(m_temp_reg_14, m_temp_reg_15);
                m_temp_reg_3 = _mm_unpackhi_epi32(m_temp_reg_14, m_temp_reg_15);

                m_temp_reg_12 = _mm_unpacklo_epi16(m_temp_reg_54, m_temp_reg_55);
                m_temp_reg_13 = _mm_unpacklo_epi16(m_temp_reg_56, m_temp_reg_57);
                m_temp_reg_16 = _mm_unpackhi_epi16(m_temp_reg_54, m_temp_reg_55);
                m_temp_reg_17 = _mm_unpackhi_epi16(m_temp_reg_56, m_temp_reg_57);
                m_temp_reg_4 = _mm_unpacklo_epi32(m_temp_reg_12, m_temp_reg_13);
                m_temp_reg_5 = _mm_unpackhi_epi32(m_temp_reg_12, m_temp_reg_13);
                m_temp_reg_6 = _mm_unpacklo_epi32(m_temp_reg_16, m_temp_reg_17);
                m_temp_reg_7 = _mm_unpackhi_epi32(m_temp_reg_16, m_temp_reg_17);

                m_temp_reg_50 = _mm_unpacklo_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_51 = _mm_unpackhi_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_52 = _mm_unpacklo_epi64(m_temp_reg_1, m_temp_reg_5);
                m_temp_reg_53 = _mm_unpackhi_epi64(m_temp_reg_1, m_temp_reg_5);

                m_temp_reg_54 = _mm_unpacklo_epi64(m_temp_reg_2, m_temp_reg_6);
                m_temp_reg_55 = _mm_unpackhi_epi64(m_temp_reg_2, m_temp_reg_6);
                m_temp_reg_56 = _mm_unpacklo_epi64(m_temp_reg_3, m_temp_reg_7);
                m_temp_reg_57 = _mm_unpackhi_epi64(m_temp_reg_3, m_temp_reg_7);
            }
        }
        /* Stage 2 */

        i4_shift = IDCT_STG2_SHIFT;

        {

            /* ee0 is present in the registers m_temp_reg_10 and m_temp_reg_11 */
            /* ee1 is present in the registers m_temp_reg_12 and m_temp_reg_13 */
            {
                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q11[0][0]); //add
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q11[3][0]); //sub

                m_temp_reg_0 = _mm_unpacklo_epi16(m_temp_reg_50, m_temp_reg_54);
                m_temp_reg_1 = _mm_unpackhi_epi16(m_temp_reg_50, m_temp_reg_54);

                m_temp_reg_10 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                m_temp_reg_12 = _mm_madd_epi16(m_temp_reg_0, m_coeff2);
                m_temp_reg_11 = _mm_madd_epi16(m_temp_reg_1, m_coeff1);
                m_temp_reg_13 = _mm_madd_epi16(m_temp_reg_1, m_coeff2);


                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q11[1][0]);
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_even_8_q11[2][0]);
            }


            /* eo0 is present in the registers m_temp_reg_14 and m_temp_reg_15 */
            /* eo1 is present in the registers m_temp_reg_16 and m_temp_reg_17 */
            {
                m_temp_reg_0 = _mm_unpacklo_epi16(m_temp_reg_52, m_temp_reg_56);
                m_temp_reg_1 = _mm_unpackhi_epi16(m_temp_reg_52, m_temp_reg_56);


                m_temp_reg_16 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                m_temp_reg_14 = _mm_madd_epi16(m_temp_reg_0, m_coeff2);
                m_temp_reg_17 = _mm_madd_epi16(m_temp_reg_1, m_coeff1);
                m_temp_reg_15 = _mm_madd_epi16(m_temp_reg_1, m_coeff2);

                /* Loading coeff for computing o0 in the next block */
                m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[0][0]);
                m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[1][0]);


                m_temp_reg_0 = _mm_unpacklo_epi16(m_temp_reg_51, m_temp_reg_53);
                m_temp_reg_1 = _mm_unpackhi_epi16(m_temp_reg_51, m_temp_reg_53);
            }

            /* e */
            {
                /* e0 stored in m_temp_reg_40 and m_temp_reg_41 */
                /* e1 stored in m_temp_reg_42 and m_temp_reg_43 */
                /* e3 stored in m_temp_reg_46 and m_temp_reg_47 */
                /* e2 stored in m_temp_reg_44 and m_temp_reg_45 */
                m_temp_reg_42 = _mm_add_epi32(m_temp_reg_12, m_temp_reg_16);
                m_temp_reg_44 = _mm_sub_epi32(m_temp_reg_12, m_temp_reg_16);

                m_temp_reg_40 = _mm_add_epi32(m_temp_reg_10, m_temp_reg_14);
                m_temp_reg_46 = _mm_sub_epi32(m_temp_reg_10, m_temp_reg_14);

                m_temp_reg_43 = _mm_add_epi32(m_temp_reg_13, m_temp_reg_17);
                m_temp_reg_45 = _mm_sub_epi32(m_temp_reg_13, m_temp_reg_17);

                m_temp_reg_41 = _mm_add_epi32(m_temp_reg_11, m_temp_reg_15);
                m_temp_reg_47 = _mm_sub_epi32(m_temp_reg_11, m_temp_reg_15);

            }

            /* o */
            {
                m_temp_reg_4 = _mm_unpacklo_epi16(m_temp_reg_55, m_temp_reg_57);
                m_temp_reg_5 = _mm_unpackhi_epi16(m_temp_reg_55, m_temp_reg_57);

                /* o0 stored in m_temp_reg_30 and m_temp_reg_31 */
                {
                    //o0:1B*89+3B*75,1T*89+3T*75,5B*50+7B*18,5T*50+7T*18
                    m_temp_reg_20 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                    m_temp_reg_21 = _mm_madd_epi16(m_temp_reg_1, m_coeff1);
                    m_temp_reg_24 = _mm_madd_epi16(m_temp_reg_4, m_coeff2);
                    m_temp_reg_25 = _mm_madd_epi16(m_temp_reg_5, m_coeff2);

                    m_rdng_factor = _mm_cvtsi32_si128((1 << (i4_shift - 1)));
                    m_rdng_factor = _mm_shuffle_epi32(m_rdng_factor, 0x0000);
                    /* Loading coeff for computing o1 in the next block */
                    m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[2][0]);
                    m_coeff4 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[3][0]);

                    m_temp_reg_30 = _mm_add_epi32(m_temp_reg_20, m_temp_reg_24);
                    m_temp_reg_31 = _mm_add_epi32(m_temp_reg_21, m_temp_reg_25);
                }

                /* Column 0 of destination computed here */
                /* It is stored in m_temp_reg_50 */
                /* Column 7 of destination computed here */
                /* It is stored in m_temp_reg_57 */
                {
                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_40, m_temp_reg_30);
                    m_temp_reg_6 = _mm_sub_epi32(m_temp_reg_40, m_temp_reg_30);

                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_41, m_temp_reg_31);
                    m_temp_reg_7 = _mm_sub_epi32(m_temp_reg_41, m_temp_reg_31);

                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_2, m_rdng_factor);
                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_3, m_rdng_factor);
                    m_temp_reg_6 = _mm_add_epi32(m_temp_reg_6, m_rdng_factor);
                    m_temp_reg_7 = _mm_add_epi32(m_temp_reg_7, m_rdng_factor);

                    m_temp_reg_2 = _mm_srai_epi32(m_temp_reg_2, i4_shift);
                    m_temp_reg_3 = _mm_srai_epi32(m_temp_reg_3, i4_shift);
                    m_temp_reg_6 = _mm_srai_epi32(m_temp_reg_6, i4_shift);
                    m_temp_reg_7 = _mm_srai_epi32(m_temp_reg_7, i4_shift);

                    //o1:1B*75-3B*18,1T*75-3T*18,5B*89+7B*50,5T*89+7T*50
                    m_temp_reg_22 = _mm_madd_epi16(m_temp_reg_0, m_coeff3);
                    m_temp_reg_26 = _mm_madd_epi16(m_temp_reg_4, m_coeff4);
                    m_temp_reg_23 = _mm_madd_epi16(m_temp_reg_1, m_coeff3);
                    m_temp_reg_27 = _mm_madd_epi16(m_temp_reg_5, m_coeff4);

                    m_temp_reg_50 = _mm_packs_epi32(m_temp_reg_2, m_temp_reg_3);
                    m_temp_reg_57 = _mm_packs_epi32(m_temp_reg_6, m_temp_reg_7);


                    /* o1 stored in m_temp_reg_32 and m_temp_reg_33 */


                    /* Loading coeff for computing o2  in the next block */
                    m_coeff1 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[4][0]);
                    m_coeff2 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[5][0]);

                    m_temp_reg_32 = _mm_sub_epi32(m_temp_reg_22, m_temp_reg_26);
                    m_temp_reg_33 = _mm_sub_epi32(m_temp_reg_23, m_temp_reg_27);
                }

                /* Column 1 of destination computed here */
                /* It is stored in m_temp_reg_51 */
                /* Column 6 of destination computed here */
                /* It is stored in m_temp_reg_56 */
                {
                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_42, m_temp_reg_32);
                    m_temp_reg_6 = _mm_sub_epi32(m_temp_reg_42, m_temp_reg_32);

                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_43, m_temp_reg_33);
                    m_temp_reg_7 = _mm_sub_epi32(m_temp_reg_43, m_temp_reg_33);

                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_2, m_rdng_factor);
                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_3, m_rdng_factor);
                    m_temp_reg_6 = _mm_add_epi32(m_temp_reg_6, m_rdng_factor);
                    m_temp_reg_7 = _mm_add_epi32(m_temp_reg_7, m_rdng_factor);

                    m_temp_reg_2 = _mm_srai_epi32(m_temp_reg_2, i4_shift);
                    m_temp_reg_3 = _mm_srai_epi32(m_temp_reg_3, i4_shift);
                    m_temp_reg_6 = _mm_srai_epi32(m_temp_reg_6, i4_shift);
                    m_temp_reg_7 = _mm_srai_epi32(m_temp_reg_7, i4_shift);

                    //o2:1B*50-3B*89,1T*50-3T*89,5B*18+7B*75,5T*18+7T*75
                    m_temp_reg_20 = _mm_madd_epi16(m_temp_reg_0, m_coeff1);
                    m_temp_reg_24 = _mm_madd_epi16(m_temp_reg_4, m_coeff2);
                    m_temp_reg_21 = _mm_madd_epi16(m_temp_reg_1, m_coeff1);
                    m_temp_reg_25 = _mm_madd_epi16(m_temp_reg_5, m_coeff2);

                    m_temp_reg_51 = _mm_packs_epi32(m_temp_reg_2, m_temp_reg_3);
                    m_temp_reg_56 = _mm_packs_epi32(m_temp_reg_6, m_temp_reg_7);


                    /* o2 stored in m_temp_reg_34 and m_temp_reg_35 */

                    /* Loading coeff for computing o3  in the next block */

                    m_coeff3 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[6][0]);
                    m_coeff4 = _mm_loadu_si128((__m128i *)&gai2_impeg2_idct_odd_8_q11[7][0]);

                    m_temp_reg_34 = _mm_add_epi32(m_temp_reg_20, m_temp_reg_24);
                    m_temp_reg_35 = _mm_add_epi32(m_temp_reg_21, m_temp_reg_25);
                }

                /* Column 2 of destination computed here */
                /* It is stored in m_temp_reg_52 */
                /* Column 5 of destination computed here */
                /* It is stored in m_temp_reg_55 */
                {
                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_44, m_temp_reg_34);
                    m_temp_reg_6 = _mm_sub_epi32(m_temp_reg_44, m_temp_reg_34);

                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_45, m_temp_reg_35);
                    m_temp_reg_7 = _mm_sub_epi32(m_temp_reg_45, m_temp_reg_35);

                    m_temp_reg_2 = _mm_add_epi32(m_temp_reg_2, m_rdng_factor);
                    m_temp_reg_3 = _mm_add_epi32(m_temp_reg_3, m_rdng_factor);
                    m_temp_reg_6 = _mm_add_epi32(m_temp_reg_6, m_rdng_factor);
                    m_temp_reg_7 = _mm_add_epi32(m_temp_reg_7, m_rdng_factor);

                    m_temp_reg_2 = _mm_srai_epi32(m_temp_reg_2, i4_shift);
                    m_temp_reg_3 = _mm_srai_epi32(m_temp_reg_3, i4_shift);
                    m_temp_reg_6 = _mm_srai_epi32(m_temp_reg_6, i4_shift);
                    m_temp_reg_7 = _mm_srai_epi32(m_temp_reg_7, i4_shift);

                    //o3:1B*18-3B*50,1T*18-3T*50,5B*75-7B*89,5T*75-7T*89
                    m_temp_reg_22 = _mm_madd_epi16(m_temp_reg_0, m_coeff3);
                    m_temp_reg_26 = _mm_madd_epi16(m_temp_reg_4, m_coeff4);
                    m_temp_reg_23 = _mm_madd_epi16(m_temp_reg_1, m_coeff3);
                    m_temp_reg_27 = _mm_madd_epi16(m_temp_reg_5, m_coeff4);

                    m_temp_reg_52 = _mm_packs_epi32(m_temp_reg_2, m_temp_reg_3);
                    m_temp_reg_55 = _mm_packs_epi32(m_temp_reg_6, m_temp_reg_7);



                    /* o3 stored in m_temp_reg_36 and m_temp_reg_37 */


                    m_temp_reg_36 = _mm_add_epi32(m_temp_reg_22, m_temp_reg_26);
                    m_temp_reg_37 = _mm_add_epi32(m_temp_reg_23, m_temp_reg_27);
                }

                /* Column 3 of destination computed here */
                /* It is stored in m_temp_reg_53 */
                /* Column 4 of destination computed here */
                /* It is stored in m_temp_reg_54 */
                {
                    m_temp_reg_20 = _mm_add_epi32(m_temp_reg_46, m_temp_reg_36);
                    m_temp_reg_22 = _mm_sub_epi32(m_temp_reg_46, m_temp_reg_36);

                    m_temp_reg_21 = _mm_add_epi32(m_temp_reg_47, m_temp_reg_37);
                    m_temp_reg_23 = _mm_sub_epi32(m_temp_reg_47, m_temp_reg_37);

                    m_temp_reg_20 = _mm_add_epi32(m_temp_reg_20, m_rdng_factor);
                    m_temp_reg_21 = _mm_add_epi32(m_temp_reg_21, m_rdng_factor);
                    m_temp_reg_22 = _mm_add_epi32(m_temp_reg_22, m_rdng_factor);
                    m_temp_reg_23 = _mm_add_epi32(m_temp_reg_23, m_rdng_factor);

                    m_temp_reg_20 = _mm_srai_epi32(m_temp_reg_20, i4_shift);
                    m_temp_reg_21 = _mm_srai_epi32(m_temp_reg_21, i4_shift);
                    m_temp_reg_22 = _mm_srai_epi32(m_temp_reg_22, i4_shift);
                    m_temp_reg_23 = _mm_srai_epi32(m_temp_reg_23, i4_shift);

                    m_temp_reg_53 = _mm_packs_epi32(m_temp_reg_20, m_temp_reg_21);
                    m_temp_reg_54 = _mm_packs_epi32(m_temp_reg_22, m_temp_reg_23);
                }
            }

            /* Transpose of the destination 8x8 matrix done here */
            /* and ultimately stored in registers m_temp_reg_50 to m_temp_reg_57 */
            /* respectively */
            {
                m_temp_reg_10 = _mm_unpacklo_epi16(m_temp_reg_50, m_temp_reg_51);
                m_temp_reg_11 = _mm_unpacklo_epi16(m_temp_reg_52, m_temp_reg_53);
                m_temp_reg_14 = _mm_unpackhi_epi16(m_temp_reg_50, m_temp_reg_51);
                m_temp_reg_15 = _mm_unpackhi_epi16(m_temp_reg_52, m_temp_reg_53);
                m_temp_reg_0 = _mm_unpacklo_epi32(m_temp_reg_10, m_temp_reg_11);
                m_temp_reg_1 = _mm_unpackhi_epi32(m_temp_reg_10, m_temp_reg_11);
                m_temp_reg_2 = _mm_unpacklo_epi32(m_temp_reg_14, m_temp_reg_15);
                m_temp_reg_3 = _mm_unpackhi_epi32(m_temp_reg_14, m_temp_reg_15);

                m_temp_reg_12 = _mm_unpacklo_epi16(m_temp_reg_54, m_temp_reg_55);
                m_temp_reg_13 = _mm_unpacklo_epi16(m_temp_reg_56, m_temp_reg_57);
                m_temp_reg_16 = _mm_unpackhi_epi16(m_temp_reg_54, m_temp_reg_55);
                m_temp_reg_17 = _mm_unpackhi_epi16(m_temp_reg_56, m_temp_reg_57);
                m_temp_reg_4 = _mm_unpacklo_epi32(m_temp_reg_12, m_temp_reg_13);
                m_temp_reg_5 = _mm_unpackhi_epi32(m_temp_reg_12, m_temp_reg_13);
                m_temp_reg_6 = _mm_unpacklo_epi32(m_temp_reg_16, m_temp_reg_17);
                m_temp_reg_7 = _mm_unpackhi_epi32(m_temp_reg_16, m_temp_reg_17);
                m_temp_reg_10 = _mm_unpacklo_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_11 = _mm_unpackhi_epi64(m_temp_reg_0, m_temp_reg_4);
                m_temp_reg_12 = _mm_unpacklo_epi64(m_temp_reg_1, m_temp_reg_5);
                m_temp_reg_13 = _mm_unpackhi_epi64(m_temp_reg_1, m_temp_reg_5);

                m_temp_reg_14 = _mm_unpacklo_epi64(m_temp_reg_2, m_temp_reg_6);
                m_temp_reg_15 = _mm_unpackhi_epi64(m_temp_reg_2, m_temp_reg_6);
                m_temp_reg_16 = _mm_unpacklo_epi64(m_temp_reg_3, m_temp_reg_7);
                m_temp_reg_17 = _mm_unpackhi_epi64(m_temp_reg_3, m_temp_reg_7);
            }

            /* Recon and store */
            {
                m_temp_reg_0 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_1 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_2 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_3 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_4 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_5 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_6 = _mm_loadl_epi64((__m128i *)pu1_pred);
                pu1_pred += pred_strd;
                m_temp_reg_7 = _mm_loadl_epi64((__m128i *)pu1_pred);


                m_temp_reg_50 = _mm_setzero_si128();
                m_temp_reg_0 = _mm_unpacklo_epi8(m_temp_reg_0, m_temp_reg_50);
                m_temp_reg_1 = _mm_unpacklo_epi8(m_temp_reg_1, m_temp_reg_50);
                m_temp_reg_2 = _mm_unpacklo_epi8(m_temp_reg_2, m_temp_reg_50);
                m_temp_reg_3 = _mm_unpacklo_epi8(m_temp_reg_3, m_temp_reg_50);
                m_temp_reg_4 = _mm_unpacklo_epi8(m_temp_reg_4, m_temp_reg_50);
                m_temp_reg_5 = _mm_unpacklo_epi8(m_temp_reg_5, m_temp_reg_50);
                m_temp_reg_6 = _mm_unpacklo_epi8(m_temp_reg_6, m_temp_reg_50);
                m_temp_reg_7 = _mm_unpacklo_epi8(m_temp_reg_7, m_temp_reg_50);

                m_temp_reg_50 = _mm_add_epi16(m_temp_reg_10, m_temp_reg_0);
                m_temp_reg_51 = _mm_add_epi16(m_temp_reg_11, m_temp_reg_1);
                m_temp_reg_52 = _mm_add_epi16(m_temp_reg_12, m_temp_reg_2);
                m_temp_reg_53 = _mm_add_epi16(m_temp_reg_13, m_temp_reg_3);
                m_temp_reg_54 = _mm_add_epi16(m_temp_reg_14, m_temp_reg_4);
                m_temp_reg_55 = _mm_add_epi16(m_temp_reg_15, m_temp_reg_5);
                m_temp_reg_56 = _mm_add_epi16(m_temp_reg_16, m_temp_reg_6);
                m_temp_reg_57 = _mm_add_epi16(m_temp_reg_17, m_temp_reg_7);

                m_temp_reg_50 = _mm_packus_epi16(m_temp_reg_50, m_temp_reg_50);
                m_temp_reg_51 = _mm_packus_epi16(m_temp_reg_51, m_temp_reg_51);
                m_temp_reg_52 = _mm_packus_epi16(m_temp_reg_52, m_temp_reg_52);
                m_temp_reg_53 = _mm_packus_epi16(m_temp_reg_53, m_temp_reg_53);
                m_temp_reg_54 = _mm_packus_epi16(m_temp_reg_54, m_temp_reg_54);
                m_temp_reg_55 = _mm_packus_epi16(m_temp_reg_55, m_temp_reg_55);
                m_temp_reg_56 = _mm_packus_epi16(m_temp_reg_56, m_temp_reg_56);
                m_temp_reg_57 = _mm_packus_epi16(m_temp_reg_57, m_temp_reg_57);

                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_50);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_51);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_52);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_53);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_54);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_55);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_56);
                pu1_dst += dst_strd;
                _mm_storel_epi64((__m128i *)pu1_dst, m_temp_reg_57);
                pu1_dst += dst_strd;

            }


        }


    }
}

void impeg2_idct_recon_dc_mismatch_sse42(WORD16 *pi2_src,
                            WORD16 *pi2_tmp,
                            UWORD8 *pu1_pred,
                            UWORD8 *pu1_dst,
                            WORD32 src_strd,
                            WORD32 pred_strd,
                            WORD32 dst_strd,
                            WORD32 zero_cols,
                            WORD32 zero_rows)
{
    WORD32 val;
    __m128i value_4x32b, mismatch_stg2_additive;
    __m128i pred_r, pred_half0, pred_half1;
    __m128i temp0, temp1;
    __m128i round_stg2 = _mm_set1_epi32(IDCT_STG2_ROUND);

    UNUSED(pi2_tmp);
    UNUSED(src_strd);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    val = pi2_src[0] * gai2_impeg2_idct_q15[0];
    val = ((val + IDCT_STG1_ROUND) >> IDCT_STG1_SHIFT);
    val *= gai2_impeg2_idct_q11[0];
    value_4x32b = _mm_set1_epi32(val);

    // Row 0 processing
    mismatch_stg2_additive = _mm_loadu_si128((__m128i *) gai2_impeg2_mismatch_stg2_additive);
    pred_r = _mm_loadl_epi64((__m128i *) pu1_pred);
    pred_r =  _mm_cvtepu8_epi16(pred_r);
    temp0 = _mm_cvtepi16_epi32(mismatch_stg2_additive);
    mismatch_stg2_additive = _mm_srli_si128(mismatch_stg2_additive, 8);
    pred_half0 = _mm_cvtepu16_epi32(pred_r);
    temp1 = _mm_cvtepi16_epi32(mismatch_stg2_additive);

    pred_r = _mm_srli_si128(pred_r, 8);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp0 = _mm_add_epi32(temp0, round_stg2);
    temp1 = _mm_add_epi32(temp1, round_stg2);
    pred_half1 = _mm_cvtepu16_epi32(pred_r);
    temp0 = _mm_srai_epi32(temp0, IDCT_STG2_SHIFT);
    temp1 = _mm_srai_epi32(temp1, IDCT_STG2_SHIFT);
    temp0 = _mm_add_epi32(temp0, pred_half0);
    temp1 = _mm_add_epi32(temp1, pred_half1);

    temp0 = _mm_packus_epi32(temp0, temp1);
    temp0 = _mm_packus_epi16(temp0, temp1);

    _mm_storel_epi64((__m128i *)pu1_dst, temp0);

    // Row 1 processing
    mismatch_stg2_additive = _mm_loadu_si128((__m128i *) (gai2_impeg2_mismatch_stg2_additive + 8));
    pred_r = _mm_loadl_epi64((__m128i *) (pu1_pred + pred_strd));
    pred_r =  _mm_cvtepu8_epi16(pred_r);
    temp0 = _mm_cvtepi16_epi32(mismatch_stg2_additive);
    mismatch_stg2_additive = _mm_srli_si128(mismatch_stg2_additive, 8);
    pred_half0 = _mm_cvtepu16_epi32(pred_r);
    temp1 = _mm_cvtepi16_epi32(mismatch_stg2_additive);

    pred_r = _mm_srli_si128(pred_r, 8);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp0 = _mm_add_epi32(temp0, round_stg2);
    temp1 = _mm_add_epi32(temp1, round_stg2);
    pred_half1 = _mm_cvtepu16_epi32(pred_r);
    temp0 = _mm_srai_epi32(temp0, IDCT_STG2_SHIFT);
    temp1 = _mm_srai_epi32(temp1, IDCT_STG2_SHIFT);
    temp0 = _mm_add_epi32(temp0, pred_half0);
    temp1 = _mm_add_epi32(temp1, pred_half1);

    temp0 = _mm_packus_epi32(temp0, temp1);
    temp0 = _mm_packus_epi16(temp0, temp1);

    _mm_storel_epi64((__m128i *)(pu1_dst + dst_strd), temp0);

    // Row 2 processing
    mismatch_stg2_additive = _mm_loadu_si128((__m128i *) (gai2_impeg2_mismatch_stg2_additive + 16));
    pred_r = _mm_loadl_epi64((__m128i *) (pu1_pred + 2 * pred_strd));
    pred_r =  _mm_cvtepu8_epi16(pred_r);
    temp0 = _mm_cvtepi16_epi32(mismatch_stg2_additive);
    mismatch_stg2_additive = _mm_srli_si128(mismatch_stg2_additive, 8);
    pred_half0 = _mm_cvtepu16_epi32(pred_r);
    temp1 = _mm_cvtepi16_epi32(mismatch_stg2_additive);

    pred_r = _mm_srli_si128(pred_r, 8);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp0 = _mm_add_epi32(temp0, round_stg2);
    temp1 = _mm_add_epi32(temp1, round_stg2);
    pred_half1 = _mm_cvtepu16_epi32(pred_r);
    temp0 = _mm_srai_epi32(temp0, IDCT_STG2_SHIFT);
    temp1 = _mm_srai_epi32(temp1, IDCT_STG2_SHIFT);
    temp0 = _mm_add_epi32(temp0, pred_half0);
    temp1 = _mm_add_epi32(temp1, pred_half1);

    temp0 = _mm_packus_epi32(temp0, temp1);
    temp0 = _mm_packus_epi16(temp0, temp1);

    _mm_storel_epi64((__m128i *)(pu1_dst + 2 * dst_strd), temp0);

    // Row 3 processing
    mismatch_stg2_additive = _mm_loadu_si128((__m128i *) (gai2_impeg2_mismatch_stg2_additive + 24));
    pred_r = _mm_loadl_epi64((__m128i *) (pu1_pred + 3 * pred_strd));
    pred_r =  _mm_cvtepu8_epi16(pred_r);
    temp0 = _mm_cvtepi16_epi32(mismatch_stg2_additive);
    mismatch_stg2_additive = _mm_srli_si128(mismatch_stg2_additive, 8);
    pred_half0 = _mm_cvtepu16_epi32(pred_r);
    temp1 = _mm_cvtepi16_epi32(mismatch_stg2_additive);

    pred_r = _mm_srli_si128(pred_r, 8);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp0 = _mm_add_epi32(temp0, round_stg2);
    temp1 = _mm_add_epi32(temp1, round_stg2);
    pred_half1 = _mm_cvtepu16_epi32(pred_r);
    temp0 = _mm_srai_epi32(temp0, IDCT_STG2_SHIFT);
    temp1 = _mm_srai_epi32(temp1, IDCT_STG2_SHIFT);
    temp0 = _mm_add_epi32(temp0, pred_half0);
    temp1 = _mm_add_epi32(temp1, pred_half1);

    temp0 = _mm_packus_epi32(temp0, temp1);
    temp0 = _mm_packus_epi16(temp0, temp1);

    _mm_storel_epi64((__m128i *)(pu1_dst + 3 * dst_strd), temp0);

    // Row 4 processing
    mismatch_stg2_additive = _mm_loadu_si128((__m128i *) (gai2_impeg2_mismatch_stg2_additive + 32));
    pred_r = _mm_loadl_epi64((__m128i *) (pu1_pred + 4 * pred_strd));
    pred_r =  _mm_cvtepu8_epi16(pred_r);
    temp0 = _mm_cvtepi16_epi32(mismatch_stg2_additive);
    mismatch_stg2_additive = _mm_srli_si128(mismatch_stg2_additive, 8);
    pred_half0 = _mm_cvtepu16_epi32(pred_r);
    temp1 = _mm_cvtepi16_epi32(mismatch_stg2_additive);

    pred_r = _mm_srli_si128(pred_r, 8);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp0 = _mm_add_epi32(temp0, round_stg2);
    temp1 = _mm_add_epi32(temp1, round_stg2);
    pred_half1 = _mm_cvtepu16_epi32(pred_r);
    temp0 = _mm_srai_epi32(temp0, IDCT_STG2_SHIFT);
    temp1 = _mm_srai_epi32(temp1, IDCT_STG2_SHIFT);
    temp0 = _mm_add_epi32(temp0, pred_half0);
    temp1 = _mm_add_epi32(temp1, pred_half1);

    temp0 = _mm_packus_epi32(temp0, temp1);
    temp0 = _mm_packus_epi16(temp0, temp1);

    _mm_storel_epi64((__m128i *)(pu1_dst + 4 * dst_strd), temp0);

    // Row 5 processing
    mismatch_stg2_additive = _mm_loadu_si128((__m128i *) (gai2_impeg2_mismatch_stg2_additive + 40));
    pred_r = _mm_loadl_epi64((__m128i *) (pu1_pred + 5 * pred_strd));
    pred_r =  _mm_cvtepu8_epi16(pred_r);
    temp0 = _mm_cvtepi16_epi32(mismatch_stg2_additive);
    mismatch_stg2_additive = _mm_srli_si128(mismatch_stg2_additive, 8);
    pred_half0 = _mm_cvtepu16_epi32(pred_r);
    temp1 = _mm_cvtepi16_epi32(mismatch_stg2_additive);

    pred_r = _mm_srli_si128(pred_r, 8);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp0 = _mm_add_epi32(temp0, round_stg2);
    temp1 = _mm_add_epi32(temp1, round_stg2);
    pred_half1 = _mm_cvtepu16_epi32(pred_r);
    temp0 = _mm_srai_epi32(temp0, IDCT_STG2_SHIFT);
    temp1 = _mm_srai_epi32(temp1, IDCT_STG2_SHIFT);
    temp0 = _mm_add_epi32(temp0, pred_half0);
    temp1 = _mm_add_epi32(temp1, pred_half1);

    temp0 = _mm_packus_epi32(temp0, temp1);
    temp0 = _mm_packus_epi16(temp0, temp1);

    _mm_storel_epi64((__m128i *)(pu1_dst + 5 * dst_strd), temp0);

    // Row 6 processing
    mismatch_stg2_additive = _mm_loadu_si128((__m128i *) (gai2_impeg2_mismatch_stg2_additive + 48));
    pred_r = _mm_loadl_epi64((__m128i *) (pu1_pred + 6 * pred_strd));
    pred_r =  _mm_cvtepu8_epi16(pred_r);
    temp0 = _mm_cvtepi16_epi32(mismatch_stg2_additive);
    mismatch_stg2_additive = _mm_srli_si128(mismatch_stg2_additive, 8);
    pred_half0 = _mm_cvtepu16_epi32(pred_r);
    temp1 = _mm_cvtepi16_epi32(mismatch_stg2_additive);

    pred_r = _mm_srli_si128(pred_r, 8);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp0 = _mm_add_epi32(temp0, round_stg2);
    temp1 = _mm_add_epi32(temp1, round_stg2);
    pred_half1 = _mm_cvtepu16_epi32(pred_r);
    temp0 = _mm_srai_epi32(temp0, IDCT_STG2_SHIFT);
    temp1 = _mm_srai_epi32(temp1, IDCT_STG2_SHIFT);
    temp0 = _mm_add_epi32(temp0, pred_half0);
    temp1 = _mm_add_epi32(temp1, pred_half1);

    temp0 = _mm_packus_epi32(temp0, temp1);
    temp0 = _mm_packus_epi16(temp0, temp1);

    _mm_storel_epi64((__m128i *)(pu1_dst + 6 * dst_strd), temp0);

    // Row 7 processing
    mismatch_stg2_additive = _mm_loadu_si128((__m128i *) (gai2_impeg2_mismatch_stg2_additive + 56));
    pred_r = _mm_loadl_epi64((__m128i *) (pu1_pred + 7 * pred_strd));
    pred_r =  _mm_cvtepu8_epi16(pred_r);
    temp0 = _mm_cvtepi16_epi32(mismatch_stg2_additive);
    mismatch_stg2_additive = _mm_srli_si128(mismatch_stg2_additive, 8);
    pred_half0 = _mm_cvtepu16_epi32(pred_r);
    temp1 = _mm_cvtepi16_epi32(mismatch_stg2_additive);

    pred_r = _mm_srli_si128(pred_r, 8);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp0 = _mm_add_epi32(temp0, round_stg2);
    temp1 = _mm_add_epi32(temp1, round_stg2);
    pred_half1 = _mm_cvtepu16_epi32(pred_r);
    temp0 = _mm_srai_epi32(temp0, IDCT_STG2_SHIFT);
    temp1 = _mm_srai_epi32(temp1, IDCT_STG2_SHIFT);
    temp0 = _mm_add_epi32(temp0, pred_half0);
    temp1 = _mm_add_epi32(temp1, pred_half1);

    temp0 = _mm_packus_epi32(temp0, temp1);
    temp0 = _mm_packus_epi16(temp0, temp1);

    _mm_storel_epi64((__m128i *)(pu1_dst + 7 * dst_strd), temp0);
}

void impeg2_idct_recon_dc_sse42(WORD16 *pi2_src,
                            WORD16 *pi2_tmp,
                            UWORD8 *pu1_pred,
                            UWORD8 *pu1_dst,
                            WORD32 src_strd,
                            WORD32 pred_strd,
                            WORD32 dst_strd,
                            WORD32 zero_cols,
                            WORD32 zero_rows)
{
    WORD32 val;
    __m128i value_4x32b, pred_r0, pred_r1, temp0, temp1, temp2, temp3;

    UNUSED(pi2_tmp);
    UNUSED(src_strd);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    val = pi2_src[0] * gai2_impeg2_idct_q15[0];
    val = ((val + IDCT_STG1_ROUND) >> IDCT_STG1_SHIFT);
    val = val * gai2_impeg2_idct_q11[0];
    val = ((val + IDCT_STG2_ROUND) >> IDCT_STG2_SHIFT);

    value_4x32b = _mm_set1_epi32(val);

    //Row 0-1 processing
    pred_r0 = _mm_loadl_epi64((__m128i *) pu1_pred);
    pred_r1 = _mm_loadl_epi64((__m128i *) (pu1_pred + pred_strd));
    pred_r0 =  _mm_cvtepu8_epi16(pred_r0);
    pred_r1 =  _mm_cvtepu8_epi16(pred_r1);

    temp0 = _mm_cvtepu16_epi32(pred_r0);
    pred_r0 = _mm_srli_si128(pred_r0, 8);
    temp2 = _mm_cvtepu16_epi32(pred_r1);
    pred_r1 = _mm_srli_si128(pred_r1, 8);
    temp1 = _mm_cvtepu16_epi32(pred_r0);
    temp3 = _mm_cvtepu16_epi32(pred_r1);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp2 = _mm_add_epi32(temp2, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp3 = _mm_add_epi32(temp3, value_4x32b);
    temp0 = _mm_packus_epi32(temp0, temp1);
    temp2 = _mm_packus_epi32(temp2, temp3);
    temp0 = _mm_packus_epi16(temp0, temp1);
    temp2 = _mm_packus_epi16(temp2, temp3);
    _mm_storel_epi64((__m128i *)(pu1_dst), temp0);
    _mm_storel_epi64((__m128i *)(pu1_dst + dst_strd), temp2);

    //Row 2-3 processing
    pu1_pred += 2 * pred_strd;
    pu1_dst += 2 * dst_strd;

    pred_r0 = _mm_loadl_epi64((__m128i *) pu1_pred);
    pred_r1 = _mm_loadl_epi64((__m128i *) (pu1_pred + pred_strd));
    pred_r0 =  _mm_cvtepu8_epi16(pred_r0);
    pred_r1 =  _mm_cvtepu8_epi16(pred_r1);

    temp0 = _mm_cvtepu16_epi32(pred_r0);
    pred_r0 = _mm_srli_si128(pred_r0, 8);
    temp2 = _mm_cvtepu16_epi32(pred_r1);
    pred_r1 = _mm_srli_si128(pred_r1, 8);
    temp1 = _mm_cvtepu16_epi32(pred_r0);
    temp3 = _mm_cvtepu16_epi32(pred_r1);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp2 = _mm_add_epi32(temp2, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp3 = _mm_add_epi32(temp3, value_4x32b);
    temp0 = _mm_packus_epi32(temp0, temp1);
    temp2 = _mm_packus_epi32(temp2, temp3);
    temp0 = _mm_packus_epi16(temp0, temp1);
    temp2 = _mm_packus_epi16(temp2, temp3);
    _mm_storel_epi64((__m128i *)(pu1_dst), temp0);
    _mm_storel_epi64((__m128i *)(pu1_dst + dst_strd), temp2);

    //Row 4-5 processing
    pu1_pred += 2 * pred_strd;
    pu1_dst += 2 * dst_strd;

    pred_r0 = _mm_loadl_epi64((__m128i *) pu1_pred);
    pred_r1 = _mm_loadl_epi64((__m128i *) (pu1_pred + pred_strd));
    pred_r0 =  _mm_cvtepu8_epi16(pred_r0);
    pred_r1 =  _mm_cvtepu8_epi16(pred_r1);

    temp0 = _mm_cvtepu16_epi32(pred_r0);
    pred_r0 = _mm_srli_si128(pred_r0, 8);
    temp2 = _mm_cvtepu16_epi32(pred_r1);
    pred_r1 = _mm_srli_si128(pred_r1, 8);
    temp1 = _mm_cvtepu16_epi32(pred_r0);
    temp3 = _mm_cvtepu16_epi32(pred_r1);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp2 = _mm_add_epi32(temp2, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp3 = _mm_add_epi32(temp3, value_4x32b);
    temp0 = _mm_packus_epi32(temp0, temp1);
    temp2 = _mm_packus_epi32(temp2, temp3);
    temp0 = _mm_packus_epi16(temp0, temp1);
    temp2 = _mm_packus_epi16(temp2, temp3);
    _mm_storel_epi64((__m128i *)(pu1_dst), temp0);
    _mm_storel_epi64((__m128i *)(pu1_dst + dst_strd), temp2);

    //Row 6-7 processing
    pu1_pred += 2 * pred_strd;
    pu1_dst += 2 * dst_strd;

    pred_r0 = _mm_loadl_epi64((__m128i *) pu1_pred);
    pred_r1 = _mm_loadl_epi64((__m128i *) (pu1_pred + pred_strd));
    pred_r0 =  _mm_cvtepu8_epi16(pred_r0);
    pred_r1 =  _mm_cvtepu8_epi16(pred_r1);

    temp0 = _mm_cvtepu16_epi32(pred_r0);
    pred_r0 = _mm_srli_si128(pred_r0, 8);
    temp2 = _mm_cvtepu16_epi32(pred_r1);
    pred_r1 = _mm_srli_si128(pred_r1, 8);
    temp1 = _mm_cvtepu16_epi32(pred_r0);
    temp3 = _mm_cvtepu16_epi32(pred_r1);

    temp0 = _mm_add_epi32(temp0, value_4x32b);
    temp2 = _mm_add_epi32(temp2, value_4x32b);
    temp1 = _mm_add_epi32(temp1, value_4x32b);
    temp3 = _mm_add_epi32(temp3, value_4x32b);
    temp0 = _mm_packus_epi32(temp0, temp1);
    temp2 = _mm_packus_epi32(temp2, temp3);
    temp0 = _mm_packus_epi16(temp0, temp1);
    temp2 = _mm_packus_epi16(temp2, temp3);
    _mm_storel_epi64((__m128i *)(pu1_dst), temp0);
    _mm_storel_epi64((__m128i *)(pu1_dst + dst_strd), temp2);
}
