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
#ifndef __IMPEG2_GLOBALS_H__
#define __IMPEG2_GLOBALS_H__

extern const UWORD8 gau1_impeg2_non_linear_quant_scale[];
extern const UWORD8 gau1_impeg2_intra_quant_matrix_default[];
extern const UWORD8 gau1_impeg2_inter_quant_matrix_default[];
extern const UWORD8  gau1_impeg2_inv_scan_vertical[];
extern const UWORD8  gau1_impeg2_inv_scan_zig_zag[];
extern const UWORD16 gau2_impeg2_frm_rate_code[][2];

extern const UWORD16 gau2_impeg2_chroma_interp_mv[][16];
extern const UWORD16 gau2_impeg2_chroma_interp_inp1[][16];
extern const UWORD16 gau2_impeg2_luma_interp_inp1[];
extern const UWORD16 gau2_impeg2_luma_interp_inp2[];
extern const UWORD16 gau2_impeg2_chroma_interp_inp2[];

extern const WORD16  gai2_impeg2_idct_q15[];
extern const WORD16  gai2_impeg2_idct_q11[];

extern const WORD16 gai2_impeg2_mismatch_stg1_outp[];
extern const WORD16 gai2_impeg2_idct_last_row_q11[];
extern const WORD16 gai2_impeg2_idct_first_col_q15[];
extern const WORD16 gai2_impeg2_idct_first_col_q11[];
extern const WORD16 gai2_impeg2_mismatch_stg2_additive[];

extern const WORD16  gai2_impeg2_blk_y_off_fld[];
extern const WORD16  gai2_impeg2_blk_y_off_frm[];
extern const WORD16  gai2_impeg2_blk_x_off[];

extern const UWORD8 gau1_impeg2_zerobuf[];

extern const WORD16 gai2_impeg2_idct_odd_8_q15[8][8];
extern const WORD16 gai2_impeg2_idct_odd_8_q11[8][8];

extern const WORD16 gai2_impeg2_idct_even_8_q11[4][8];
extern const WORD16 gai2_impeg2_idct_even_8_q15[4][8];

#endif /* __IMPEG2_GLOBALS_H__ */
