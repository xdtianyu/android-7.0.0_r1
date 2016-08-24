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
/*  File Name         : c_coding_example.h                                   */
/*                                                                           */
/*  Description       : This file contains all the necessary examples to     */
/*                      establish a consistent use of Ittiam C coding        */
/*                      standards (based on Indian Hill C Standards)         */
/*                                                                           */
/*  List of Functions : <List the functions defined in this file>            */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         10 01 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
#ifndef __IMPEG2D_DEBUG_H__
#define __IMPEG2D_DEBUG_H__


/*************************************************************************/
/* DEBUG                                                                 */
/*************************************************************************/
#define DEBUG_MB        0


#if DEBUG_MB
void impeg2d_trace_mb_start(UWORD32 mb_x, UWORD32 mb_y);
void impeg2d_frm_num_set(void);
UWORD32 impeg2d_frm_num_get(void);

#define IMPEG2D_TRACE_MB_START(mb_x, mb_y) void impeg2d_trace_mb_start(UWORD32 mb_x, UWORD32 mb_y);
#define IMPEG2D_FRM_NUM_SET()              void impeg2d_frm_num_set(void);
#define IMPEG2D_FRM_NUM_GET()              UWORD32 impeg2d_frm_num_get(void);
#else
#define IMPEG2D_TRACE_MB_START(mb_x, mb_y)
#define IMPEG2D_FRM_NUM_SET()
#define IMPEG2D_FRM_NUM_GET()
#endif


#define STATISTICS  0

#if STATISTICS
void impeg2d_idct_inp_statistics(WORD16 *pi2_idct_inp, WORD32 non_zero_cols, WORD32 non_zero_rows);
void impeg2d_iqnt_inp_statistics(WORD16 *pi2_iqnt_inp, WORD32 non_zero_cols, WORD32 non_zero_rows);
void impeg2d_print_statistics(void);
#define IMPEG2D_IDCT_INP_STATISTICS(pi2_idct_inp, non_zero_cols, non_zero_rows)  impeg2d_idct_inp_statistics(pi2_idct_inp, non_zero_cols, non_zero_rows)
#define IMPEG2D_IQNT_INP_STATISTICS(pi2_iqnt_inp, non_zero_cols, non_zero_rows)  impeg2d_iqnt_inp_statistics(pi2_iqnt_inp, non_zero_cols, non_zero_rows)
#define IMPEG2D_PRINT_STATISTICS()            impeg2d_print_statistics()
#else
#define IMPEG2D_IDCT_INP_STATISTICS(pi2_idct_inp, non_zero_cols, non_zero_rows)
#define IMPEG2D_IQNT_INP_STATISTICS(pi2_iqnt_inp, non_zero_cols, non_zero_rows)
#define IMPEG2D_PRINT_STATISTICS()
#endif


#if 0
#define PROFILE_DIS_SKIP_MB
#define PROFILE_DIS_MC
#define PROFILE_DIS_INVQUANT
#define PROFILE_DIS_IDCT
#define PROFILE_DIS_MEMSET_RESBUF
#endif


#ifdef PROFILE_DIS_SKIP_MB
#define PROFILE_DISABLE_SKIP_MB() return;
#else
#define PROFILE_DISABLE_SKIP_MB()
#endif

#ifdef PROFILE_DIS_MC
#define PROFILE_DISABLE_MC_IF0 if(0)
#define PROFILE_DISABLE_MC_RETURN return;
#else
#define PROFILE_DISABLE_MC_IF0
#define PROFILE_DISABLE_MC_RETURN
#endif

#ifdef PROFILE_DIS_INVQUANT
#define PROFILE_DISABLE_INVQUANT_IF0 if(0)
#else
#define PROFILE_DISABLE_INVQUANT_IF0
#endif

#ifdef PROFILE_DIS_IDCT
#define PROFILE_DISABLE_IDCT_IF0 if(0)
#else
#define PROFILE_DISABLE_IDCT_IF0
#endif

#ifdef PROFILE_DIS_MEMSET_RESBUF
#define PROFILE_DISABLE_MEMSET_RESBUF_IF0 if(0)
#else
#define PROFILE_DISABLE_MEMSET_RESBUF_IF0
#endif


#endif /* __IMPEG2D_DEBUG_H__ */
