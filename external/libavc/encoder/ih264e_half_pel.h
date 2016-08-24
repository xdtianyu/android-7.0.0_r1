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
 *  ih264e_half_pel.h
 *
 * @brief
 *  Contains extern declarations of subpel functions used by the encoder
 *
 * @author
 *  ittiam
 *
 * @remarks
 *  none
 *
 *******************************************************************************
 */

#ifndef IH264E_HALF_PEL_H_
#define IH264E_HALF_PEL_H_

/*****************************************************************************/
/* Global constants                                                          */
/*****************************************************************************/
/*
 * Dimensions of subpel plane buffers
 */
#define HP_PL_WD  MB_SIZE + 1
#define HP_PL_HT  MB_SIZE + 1

/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief
*  Interprediction luma filter for horizontal input (Filter run for width = 17
*  and height =16)
*
* @par Description:
*  Applies a 6 tap horizontal filter .The output is  clipped to 8 bits
*  sec 8.4.2.2.1 titled "Luma sample interpolation process"
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
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
* @returns
*
* @remarks
*  None
*
*******************************************************************************
*/
typedef void ih264e_sixtapfilter_horz_ft(UWORD8 *pu1_src,
                                         UWORD8 *pu1_dst,
                                         WORD32 src_strd,
                                         WORD32 dst_strd);

ih264e_sixtapfilter_horz_ft ih264e_sixtapfilter_horz;

/* arm assembly */
ih264e_sixtapfilter_horz_ft ih264e_sixtapfilter_horz_a9q;
ih264e_sixtapfilter_horz_ft ih264e_sixtapfilter_horz_av8;

/* x86 intrinsics*/
ih264e_sixtapfilter_horz_ft ih264e_sixtapfilter_horz_ssse3;

/**
*******************************************************************************
*
* @brief
*  This function implements a two stage cascaded six tap filter. It applies
*  the six tap filter in the vertical direction on the predictor values,
*  followed by applying the same filter in the horizontal direction on the
*  output of the first stage. The six tap filtering operation is described in
*  sec 8.4.2.2.1 titled "Luma sample interpolation process" (Filter run for
*  width = 17 and height = 17)
*
* @par Description:
*  The function interpolates the predictors first in the vertical direction and
*  then in the horizontal direction to output the (1/2,1/2). The output of the
*  first stage of the filter is stored in the buffer pointed to by
*  pi16_pred1(only in C) in 16 bit precision.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[out] pu1_dst1
*  UWORD8 pointer to the destination (Horizontal filtered output)
*
* @param[out] pu1_dst2
*  UWORD8 pointer to the destination (output after applying vertical filter to
*  the intermediate horizontal output)
*
* @param[in] src_strd
*  integer source stride

* @param[in] dst_strd
*  integer destination stride of pu1_dst
*
* @param[in] pi4_pred
*  Pointer to 16bit intermediate buffer (used only in c)
*
* @param[in] i4_pred_strd
*  integer destination stride of pi16_pred1
*
* @returns
*
* @remarks
*  None
*
*******************************************************************************
*/
typedef void ih264e_sixtap_filter_2dvh_vert_ft(UWORD8 *pu1_src,
                                               UWORD8 *pu1_dst1,
                                               UWORD8 *pu1_dst2,
                                               WORD32 src_strd,
                                               WORD32 dst_strd,
                                               WORD32 *pi4_pred,
                                               WORD32 i4_pred_strd);

ih264e_sixtap_filter_2dvh_vert_ft ih264e_sixtap_filter_2dvh_vert;

/* assembly */
ih264e_sixtap_filter_2dvh_vert_ft ih264e_sixtap_filter_2dvh_vert_a9q;

ih264e_sixtap_filter_2dvh_vert_ft ih264e_sixtap_filter_2dvh_vert_av8;

/* x86 intrinsics */
ih264e_sixtap_filter_2dvh_vert_ft ih264e_sixtap_filter_2dvh_vert_ssse3;

#endif /* IH264E_HALF_PEL_H_ */
