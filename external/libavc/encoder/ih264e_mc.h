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
*  ih264e_mc.h
*
* @brief
*  This file contains declarations of routines that perform motion compensation
*  of luma and chroma macroblocks.
*
* @author
*  ittiam
*
* @remarks
*  none
*
*******************************************************************************
*/

#ifndef IH264E_MC_H_
#define IH264E_MC_H_

/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/

/**
******************************************************************************
*
* @brief
*  performs motion compensation for a luma mb for the given mv.
*
* @par Description
*  This routine performs motion compensation of an inter mb. When the inter
*  mb mode is P16x16, there is no need to copy 16x16 unit from reference buffer
*  to pred buffer. In this case the function returns pointer and stride of the
*  ref. buffer and this info is used in place of pred buffer else where.
*  In other cases, the pred buffer is populated via copy / filtering + copy
*  (q pel cases) and returned.
*
* @param[in] ps_proc
*  pointer to current proc ctxt
*
* @param[out] pu1_pseudo_pred
*  pseudo prediction buffer
*
* @param[out] u4_pseudo_pred_strd
*  pseudo pred buffer stride
*
* @return  none
*
* @remarks Assumes half pel buffers for the entire frame are populated.
*
******************************************************************************
*/
void ih264e_motion_comp_luma(process_ctxt_t *ps_proc,
                             UWORD8 **pu1_pseudo_pred,
                             WORD32 *pi4_pseudo_pred_strd);

/**
******************************************************************************
*
* @brief
*  performs motion compensation for chroma mb
*
* @par   Description
*  Copies a MB of data from the reference buffer (Full pel, half pel or q pel)
*  according to the motion vectors given
*
* @param[in] ps_proc
*  pointer to current proc ctxt
*
* @return  none
*
* @remarks Assumes half pel and quarter pel buffers for the entire frame are
*  populated.
******************************************************************************
*/
void ih264e_motion_comp_chroma
        (
            process_ctxt_t *ps_proc
        );


#endif // IH264E_MC_H_
