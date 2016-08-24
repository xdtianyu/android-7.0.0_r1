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
#ifndef __IMPEG2D_GLOBALS_H__
#define __IMPEG2D_GLOBALS_H__

typedef struct
{
    UWORD16     mvy_cf;
    UWORD16     mv_num_cf;
    UWORD16     frm_wd_cf;
    UWORD16     src_wd_cf;
    UWORD32      rows;
    UWORD32      dst_wd;
    UWORD32      dst_offset_scale;
}mc_type_consts_t;

extern const mc_type_consts_t gas_impeg2d_mc_params_luma[][2];
extern const mc_type_consts_t gas_impeg2d_mc_params_chroma[][2];

extern const dec_mb_params_t gas_impeg2d_func_frm_fw_or_bk[];
extern const dec_mb_params_t gas_impeg2d_func_fld_fw_or_bk[];

extern const dec_mb_params_t gas_impeg2d_func_frm_bi_direct[];
extern const dec_mb_params_t gas_impeg2d_func_fld_bi_direct[];

#endif /* __IMPEG2D_GLOBALS_H__ */
