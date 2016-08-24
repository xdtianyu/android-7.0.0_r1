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
 *  ih264e_me.h
 *
 * @brief
 *  Contains declarations of global variables for H264 encoder
 *
 * @author
 *  ittiam
 *
 * @remarks
 *
 *******************************************************************************
 */

#ifndef IH264E_ME_H_
#define IH264E_ME_H_

/*****************************************************************************/
/* Function Macros                                                           */
/*****************************************************************************/

/**
 ******************************************************************************
 *  @brief      compute median of 3 elements (a, b, c) and store the output
 *  in to result. This is used for mv prediction
 ******************************************************************************
 */

#define MEDIAN(a, b, c, result) if (a > b){\
                                    if (b > c)\
                                        result = b;\
                                    else {\
                                        if (a > c)\
                                            result = c;\
                                        else \
                                            result = a;\
                                    }\
                                }\
                                else {\
                                    if (c > b)\
                                        result = b;\
                                    else {\
                                        if (c > a)\
                                            result = c;\
                                        else \
                                            result = a;\
                                    }\
                                }

/*****************************************************************************/
/* Extern Function Declarations                                              */
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
void ih264e_init_mv_bits(me_ctxt_t *ps_me);

/**
 *******************************************************************************
 *
 * @brief The function computes the parameters for a P skip MB
 *
 * @par Description:
 *  The function computes the parameters for a P skip MB
 *
 * @param[in] ps_proc
 *  Process context
 *
 * @param[in] u4_for_me
 *  Flag to indicate the purpose of computing skip
 *
 * @param[out] ps_pred_mv
 *  Flag to indicate the current active refernce list
 *
 * @returns
 *       1) Updates skip MV in proc
 *       2) Returns if the current MB can be coded as skip or not
 *
 * @remarks The code implements the logic as described in sec 8.4.1.1 in H264
 *   specification.
 *
 *******************************************************************************
*/
ih264e_skip_params_ft  ih264e_find_pskip_params;

/**
 *******************************************************************************
 *
 * @brief The function computes the parameters for a P skip MB
 *
 * @par Description:
 *  The function computes the parameters for a P skip MB
 *
 * @param[in] ps_proc
 *  Process context
 *
 * @param[in] u4_for_me
 *  Flag to indicate the purpose of computing skip
 *
 * @param[out] ps_pred_mv
 *  Flag to indicate the current active refernce list
 *
 * @returns
 *       1) Updates skip MV in proc
 *       2) Returns if the current MB can be coded as skip or not
 *
 * @remarks The code implements the logic as described in sec 8.4.1.1 in H264
 *   specification.
 *
 *******************************************************************************
*/
ih264e_skip_params_ft  ih264e_find_pskip_params_me;

/**
 *******************************************************************************
 *
 * @brief The function computes the parameters for a B skip MB
 *
 * @par Description:
 *  The function computes the parameters for a B skip MB
 *
 * @param[in] ps_proc
 *  Process context
 *
 * @param[in] u4_for_me
 *  Flag to indicate the purpose of computing skip
 *
 * @param[out] ps_pred_mv
 *  Flag to indicate the current active refernce list
 *
 * @returns
 *       1) Updates skip MV in proc
 *       2) Returns if the current MB can be coded as skip or not
 *
 * @remarks The code implements the logic as described in sec 8.4.1.1 in H264
 *   specification.
 *
 *******************************************************************************
*/
ih264e_skip_params_ft  ih264e_find_bskip_params;

/**
 *******************************************************************************
 *
 * @brief The function computes the parameters for a B skip MB
 *
 * @par Description:
 *  The function computes the parameters for a B skip MB
 *
 * @param[in] ps_proc
 *  Process context
 *
 * @param[in] u4_for_me
 *  Flag to indicate the purpose of computing skip
 *
 * @param[out] ps_pred_mv
 *  Flag to indicate the current active refernce list
 *
 * @returns
 *       1) Updates skip MV in proc
 *       2) The type of SKIP [L0/L1/BI]
 *
 * @remarks
 *******************************************************************************
*/
ih264e_skip_params_ft  ih264e_find_bskip_params_me;

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
 *   Assumptions : 1. Assumes Only partition of size 16x16
 *
 *******************************************************************************
 */
void ih264e_get_mv_predictor(enc_pu_t *ps_left_mb_pu, enc_pu_t *ps_top_row_pu,
                             enc_pu_mv_t *ps_pred_mv, WORD32 i4_ref_list);

/**
 *******************************************************************************
 *
 * @brief This fucntion evalues ME for 2 reference lists
 *
 * @par Description:
 *  It evaluates skip, full-pel an half-pel and assigns the correct MV in proc
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
ih264e_compute_me_ft  ih264e_compute_me_multi_reflist;

/**
 *******************************************************************************
 *
 * @brief This fucntion evalues ME for single reflist [Pred L0]
 *
 * @par Description:
 *  It evaluates skip, full-pel an half-pel and assigns the correct MV in proc
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
ih264e_compute_me_ft  ih264e_compute_me_single_reflist;

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
void ih264e_init_me(process_ctxt_t *ps_proc);

/**
 *******************************************************************************
 *
 * @brief This function performs motion estimation for the current NMB
 *
 * @par Description:
 *  Intializes input and output pointers required by the function ih264e_compute_me
 *  and calls the function ih264e_compute_me in a loop to process NMBs.
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
void ih264e_compute_me_nmb(process_ctxt_t *ps_proc, UWORD32 u4_nmb_count);

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
void ih264e_mv_pred(process_ctxt_t *ps_proc, WORD32 i4_reflist);

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
void ih264e_mv_pred_me(process_ctxt_t *ps_proc, WORD32 i4_ref_list);

#endif /* IH264E_ME_H_ */
