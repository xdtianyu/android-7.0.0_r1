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
******************************************************************************
* @file
*  ih264e_deblk.h
*
* @brief
*  This file contains extern declarations of deblocking routines
*
* @author
*  ittiam
*
* @remarks
*  none
******************************************************************************
*/

#ifndef IH264E_DEBLK_H_
#define IH264E_DEBLK_H_

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/

/**
******************************************************************************
 *  @brief  masks to extract csbp
******************************************************************************
 */
#define CSBP_LEFT_BLOCK_MASK  0x1111
#define CSBP_RIGHT_BLOCK_MASK 0x8888


/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief This function computes blocking strength for an mb
*
* @par Description:
*  This function computes blocking strength for an mb
*
* @param[in] ps_proc
*  process context
*
* @returns  none
*
* @remarks In this module it is assumed that their is only single reference
* frame and is always the most recently used anchor frame
*
*******************************************************************************
*/
void ih264e_compute_bs(process_ctxt_t * ps_proc);

/**
*******************************************************************************
*
* @brief This function performs deblocking on an mb
*
* @par Description:
*  This function performs deblocking on an mb
*
* @param[in] ps_proc
*  process context corresponding to the job
*
* @param[in] ps_deblk
*  pointer to deblock context
*
* @returns  none
*
* @remarks none
*
*******************************************************************************
*/
void ih264e_deblock_mb(process_ctxt_t *ps_proc, deblk_ctxt_t * ps_deblk);

#endif /* IH264E_DEBLK_H_ */
