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
*  ih264e_cavlc.h
*
* @brief
*  This file contains enumerations, macros and extern declarations of H264
*  cavlc tables
*
* @author
*  ittiam
*
* @remarks
*  none
******************************************************************************
*/

#ifndef IH264E_CAVLC_H_
#define IH264E_CAVLC_H_

/*****************************************************************************/
/* Function macro definitions                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/

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
IH264E_ERROR_T ih264e_write_islice_mb_cavlc(entropy_ctxt_t *ps_ent_ctxt);

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
IH264E_ERROR_T ih264e_write_pslice_mb_cavlc(entropy_ctxt_t *ps_ent_ctxt);

/**
*******************************************************************************
*
* @brief
*  This function generates CAVLC coded bit stream for Inter(B) slices
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
IH264E_ERROR_T ih264e_write_bslice_mb_cavlc(entropy_ctxt_t *ps_ent_ctxt);

#endif /* IH264E_CAVLC_H_ */
