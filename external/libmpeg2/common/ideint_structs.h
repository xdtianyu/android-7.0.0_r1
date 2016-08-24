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
*  ideint_structs.h
*
* @brief
*  Deinterlacer structure definitions
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

#ifndef __IDEINT_STRUCTS_H__
#define __IDEINT_STRUCTS_H__

typedef void ideint_spatial_filter_t(UWORD8 *pu1_src,
                                     UWORD8 *pu1_out,
                                     WORD32 cur_strd,
                                     WORD32 out_strd);

typedef WORD32 ideint_cac_8x8_t(UWORD8 *pu1_top,
                                UWORD8 *pu1_bot,
                                WORD32 i4_top_stride,
                                WORD32 i4_bot_stride);
/** Deinterlacer context */
typedef struct
{
    /** params */
    ideint_params_t s_params;

    /** Adaptive variance used in spatio temporal filtering */
    WORD32 ai4_vrnc_avg_fb[3];

    /** Function pointers */
    icv_sad_8x4_t *pf_sad_8x4;

    icv_variance_8x4_t *pf_variance_8x4;

    ideint_spatial_filter_t *pf_spatial_filter;

    ideint_cac_8x8_t    *pf_cac_8x8;
}ctxt_t;

#endif /* __IDEINT_STRUCTS_H__ */
