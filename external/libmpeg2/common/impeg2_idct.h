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
#ifndef __IMPEG2_IDCT_H__
#define __IMPEG2_IDCT_H__


/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

typedef void  pf_idct_recon_t(WORD16 *pi2_src,
                            WORD16 *pi2_tmp,
                            UWORD8 *pu1_pred,
                            UWORD8 *pu1_dst,
                            WORD32 src_strd,
                            WORD32 pred_strd,
                            WORD32 dst_strd,
                            WORD32 zero_cols,
                            WORD32 zero_rows);

/* ARM assembly modules curently ignore non_zero_cols argument */
pf_idct_recon_t impeg2_idct_recon_dc;

pf_idct_recon_t impeg2_idct_recon_dc_mismatch;

pf_idct_recon_t impeg2_idct_recon;


pf_idct_recon_t impeg2_idct_recon_dc_a9q;

pf_idct_recon_t impeg2_idct_recon_dc_mismatch_a9q;

pf_idct_recon_t impeg2_idct_recon_a9q;


pf_idct_recon_t impeg2_idct_recon_dc_av8;

pf_idct_recon_t impeg2_idct_recon_dc_mismatch_av8;

pf_idct_recon_t impeg2_idct_recon_av8;

pf_idct_recon_t impeg2_idct_recon_sse42;

pf_idct_recon_t impeg2_idct_recon_dc_mismatch_sse42;

pf_idct_recon_t impeg2_idct_recon_dc_sse42;

#endif /* #ifndef __IMPEG2_IDCT_H__ */

