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
 *  ime.h
 *
 * @brief
 *  Contains declarations of global variables for H264 encoder
 *
 * @author
 *  Ittiam
 *
 * @remarks
 *
 *******************************************************************************
 */

#ifndef IME_H_
#define IME_H_

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/

/**
******************************************************************************
 *  @brief      Number of iterations before exiting during diamond search
******************************************************************************
 */
#define NUM_LAYERS 16

/**
******************************************************************************
 *  @brief     Skip Bias value for P slice
******************************************************************************
 */
#define SKIP_BIAS_P 2

/**
******************************************************************************
 *  @brief     Skip Bias value for B slice
******************************************************************************
 */
#define SKIP_BIAS_B 16

/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/


/**
*******************************************************************************
*
* @brief Diamond Search
*
* @par Description:
*  This function computes the sad at vertices of several layers of diamond grid
*  at a time. The number of layers of diamond grid that would be evaluated is
*  configurable.The function computes the sad at vertices of a diamond grid. If
*  the sad at the center of the diamond grid is lesser than the sad at any other
*  point of the diamond grid, the function marks the candidate Mb partition as
*  mv.
*
* @param[in] ps_mb_part
*  pointer to current mb partition ctxt with respect to ME
*
* @param[in] ps_me_ctxt
*  pointer to me context
*
* @param[in] u4_lambda
*  lambda motion
*
* @param[in] u4_fast_flag
*  enable/disable fast sad computation
*
* @returns  mv pair & corresponding distortion and cost
*
* @remarks This module cannot be part of the final product due to its lack of
* computational feasibility. This is only for quality eval purposes.
*
*******************************************************************************
 */
extern void ime_diamond_search_16x16(me_ctxt_t *ps_me_ctxt, WORD32 i4_reflist);


/**
*******************************************************************************
*
* @brief This function computes the best motion vector among the tentative mv
* candidates chosen.
*
* @par Description:
*  This function determines the position in the search window at which the motion
*  estimation should begin in order to minimise the number of search iterations.
*
* @param[in] ps_mb_part
*  pointer to current mb partition ctxt with respect to ME
*
* @param[in] u4_lambda_motion
*  lambda motion
*
* @param[in] u4_fast_flag
*  enable/disable fast sad computation
*
* @returns  mv pair & corresponding distortion and cost
*
* @remarks none
*
*******************************************************************************
*/
extern void ime_evaluate_init_srchposn_16x16(me_ctxt_t *ps_me_ctxt,
                                             WORD32 i4_reflist);

/**
*******************************************************************************
*
* @brief Searches for the best matching full pixel predictor within the search
* range
*
* @par Description:
*  This function begins by computing the mv predict vector for the current mb.
*  This is used for cost computations. Further basing on the algo. chosen, it
*  looks through a set of candidate vectors that best represent the mb a least
*  cost and returns this information.
*
* @param[in] ps_proc
*  pointer to current proc ctxt
*
* @param[in] ps_me_ctxt
*  pointer to me context
*
* @returns  mv pair & corresponding distortion and cost
*
* @remarks none
*
*******************************************************************************
*/
extern void ime_full_pel_motion_estimation_16x16(me_ctxt_t *ps_me_ctxt,
                                                 WORD32 i4_ref_list);

/**
*******************************************************************************
*
* @brief Searches for the best matching sub pixel predictor within the search
* range
*
* @par Description:
*  This function begins by searching across all sub pixel sample points
*  around the full pel motion vector. The vector with least cost is chosen as
*  the mv for the current mb. If the skip mode is not evaluated while analysing
*  the initial search candidates then analyse it here and update the mv.
*
* @param[in] ps_proc
*  pointer to current proc ctxt
*
* @param[in] ps_me_ctxt
*  pointer to me context
*
* @returns none
*
* @remarks none
*
*******************************************************************************
*/
extern void ime_sub_pel_motion_estimation_16x16(me_ctxt_t *ps_me_ctxt,
                                                WORD32 i4_reflist);

/**
*******************************************************************************
*
* @brief This function computes cost of skip macroblocks
*
* @par Description:
*
* @param[in] ps_me_ctxt
*  pointer to me ctxt
*
* @param[in] ps_skip_mv
*  pointer to skip mv
*
  @param[in] is_slice_type_b
*  Whether slice type is BSLICE or not

* @returns  none
*
* @remarks
* NOTE: while computing the skip cost, do not enable early exit from compute
* sad function because, a negative bias gets added later
*
*******************************************************************************
*/
extern void ime_compute_skip_cost(me_ctxt_t *ps_me_ctxt,
                                  ime_mv_t *ps_skip_mv,
                                  mb_part_ctxt *ps_smb_part_info,
                                  UWORD32 u4_use_stat_sad,
                                  WORD32 i4_reflist,
                                  WORD32 is_slice_type_b);


#endif /* IME_H_ */
