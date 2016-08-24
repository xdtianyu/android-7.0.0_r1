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
*  ideint_utils.h
*
* @brief
*  Contains various functions needed in deinterlacer
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

#ifndef __IDEINT_UTILS_H__
#define __IDEINT_UTILS_H__

WORD32 ideint_weave_pic(icv_pic_t *ps_src_top,
                        icv_pic_t *ps_src_bot,
                        icv_pic_t *ps_dst_frm,
                        WORD32 start_row,
                        WORD32 num_rows);


void ideint_pad_blk(UWORD8 *pu1_top,
                    UWORD8 *pu1_bot,
                    UWORD8 *pu1_pad,
                    WORD32 cur_strd,
                    WORD32 row,
                    WORD32 col,
                    WORD32 num_blks_y,
                    WORD32 num_blks_x,
                    WORD32 blk_wd,
                    WORD32 blk_ht);

WORD32 ideint_weave_blk(UWORD8 *pu1_top,
                        UWORD8 *pu1_bot,
                        UWORD8 *pu1_dst,
                        WORD32 dst_strd,
                        WORD32 src_strd,
                        WORD32 wd,
                        WORD32 ht);


ideint_spatial_filter_t ideint_spatial_filter;
ideint_spatial_filter_t ideint_spatial_filter_a9;
ideint_spatial_filter_t ideint_spatial_filter_av8;
ideint_spatial_filter_t ideint_spatial_filter_ssse3;

#endif /* __IDEINT_UTILS_H__ */
