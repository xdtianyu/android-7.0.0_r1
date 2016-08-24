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
*  impeg2d_mc.h
*
* @brief
*  Contains MC function declarations for MPEG2 codec
*
* @author
*  Harish
*
* @remarks
*  None
*
*******************************************************************************
*/
#ifndef __IMPEG2D_MC_H__
#define __IMPEG2D_MC_H__

void impeg2d_dec_2mv_interp_mb(dec_state_t *dec);
void impeg2d_dec_4mv_mb(dec_state_t *dec);


void impeg2d_dec_1mv_mb(dec_state_t *dec);
void impeg2d_dec_2mv_fw_or_bk_mb(dec_state_t *dec);
void impeg2d_dec_fld_dual_prime(dec_state_t *dec);
void impeg2d_dec_frm_dual_prime(dec_state_t *dec);

void impeg2d_mc_1mv(dec_state_t *dec);
void impeg2d_mc_fw_or_bk_mb(dec_state_t *dec);
void impeg2d_mc_fld_dual_prime(dec_state_t *dec);
void impeg2d_mc_frm_dual_prime(dec_state_t *dec);
void impeg2d_mc_4mv(dec_state_t *dec);
void impeg2d_mc_2mv(dec_state_t *dec);

void impeg2d_dec_skip_mbs(dec_state_t *dec, UWORD16 num_skip_mbs);
void impeg2d_dec_0mv_coded_mb(dec_state_t *dec);
void impeg2d_dec_intra_mb(dec_state_t *dec);

void impeg2d_set_mc_params(comp_mc_params_t *luma,
                   comp_mc_params_t *chroma,
                   e_mb_type_t   type,
                   UWORD16 mv_num,
                   WORD16 mv[],
                   UWORD16 mb_x,
                   UWORD16 mb_y,
                   UWORD16 frm_wd,
                   UWORD16 frm_ht,
                   UWORD16 picture_width);

void impeg2d_motion_comp(dec_state_t *dec, mb_mc_params_t *params,yuv_buf_t *buf);

pf_mc_t impeg2d_mc_halfx_halfy;
pf_mc_t impeg2d_mc_halfx_fully;
pf_mc_t impeg2d_mc_fullx_halfy;
pf_mc_t impeg2d_mc_fullx_fully;


#endif /* __IMPEG2D_MC_H__*/
