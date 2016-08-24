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
*  ideint.h
*
* @brief
*  Deinterlacer API file
*
* @author
*  Ittiam
*
* @par List of Functions:
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef __IDEINT_H__
#define __IDEINT_H__

/** Error codes */
typedef enum
{
    /** Dummy error code */
    IDEINT_ERROR_NA = 0x7FFFFFFF,

    /** No error */
    IDEINT_ERROR_NONE = 0,

    /** Invalid Context */
    IDEINT_INVALID_CTXT,

    /** Start row not aligned to 8 */
    IDEINT_START_ROW_UNALIGNED,


}IDEINT_ERROR_T;

/** Modes of deinterlacing */
typedef enum
{
    /** Dummy mode */
    IDEINT_MODE_NA = 0x7FFFFFFF,

    /** Weave two fields to get a frame, no filtering */
    IDEINT_MODE_WEAVE = 0,

    /** Weave two fields in static blocks and
     spatial filtering for non-static blocks */
    IDEINT_MODE_SPATIAL,

}IDEINT_MODE_T;

/** Deinterlacer parameters */
typedef struct
{
    /** Mode for deinterlacing */
    IDEINT_MODE_T e_mode;

    /** Flag to indicate if the current field is top field,
     * Prev and Next field are assumed to be of opposite parity
     */
    WORD32  i4_cur_fld_top;

    /** Flag to signal if weave should be disabled.
     * i.e. output already contains weaved fields
     */
    WORD32  i4_disable_weave;

    /** CPU Architecture */
    ICV_ARCH_T e_arch;

    /** SOC */
    ICV_SOC_T e_soc;

    /** Pointer to a function for aligned allocation.
     * If NULL, then malloc will be used internally
     * Module will allocate if any extra memory is needed
     */
    void *(*pf_aligned_alloc)(WORD32 alignment, WORD32 size);

    /** Pointer to a function for aligned free.
     * If NULL, then free will be used internally
     */
    void   (*pf_aligned_free)(void *pv_buf);

}ideint_params_t;

/** Deinterlacer context size */
WORD32 ideint_ctxt_size(void);

/** Deinterlacer process */
IDEINT_ERROR_T ideint_process(void *pv_ctxt,
                              icv_pic_t *ps_prv_fld,
                              icv_pic_t *ps_cur_fld,
                              icv_pic_t *ps_nxt_fld,
                              icv_pic_t *ps_out_frm,
                              ideint_params_t *ps_params,
                              WORD32 start_row,
                              WORD32 num_rows);

#endif /* __IDEINT_H__ */
