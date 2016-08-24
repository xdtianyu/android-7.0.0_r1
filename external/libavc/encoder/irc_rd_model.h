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
/* File Name         : irc_rd_model.h                                        */
/*                                                                           */
/* Description       : Implements all the Functions to Model the             */
/*                     Rate Distortion Behaviour of the Codec over the Last  */
/*                     Few Frames.                                           */
/*                                                                           */
/* List of Functions : irc_update_frame_rd_model                             */
/*                     estimate_mpeg2_qp_for_resbits                         */
/*                     update_mb_rd_model                                    */
/*                     find_model_coeffs                                     */
/*                     refine_set_of_points                                  */
/*                     init_mb_rd_model                                      */
/*                     irc_add_frame_to_rd_model                             */
/*                     irc_find_qp_for_target_bits                           */
/*                                                                           */
/*                                                                           */
/* Issues / Problems : None                                                  */
/*                                                                           */
/* Revision History  :                                                       */
/*        DD MM YYYY   Author(s)       Changes (Describe the changes made)   */
/*        21 06 2006   Sarat           Initial Version                       */
/*****************************************************************************/

#ifndef RC_RD_MODEL
#define RC_RD_MODEL

#define MAX_FRAMES_MODELLED 16

typedef float model_coeff;
typedef struct rc_rd_model_t *rc_rd_model_handle;

WORD32 irc_rd_model_num_fill_use_free_memtab(rc_rd_model_handle *pps_rc_rd_model,
                                             itt_memtab_t *ps_memtab,
                                             ITT_FUNC_TYPE_E e_func_type);
/* Interface Functions */
/* Initialise the rate distortion model */
void irc_init_frm_rc_rd_model(rc_rd_model_handle ps_rd_model,
                              UWORD8 u1_max_frames_modelled);

/* Reset the rate distortion model */
void irc_reset_frm_rc_rd_model(rc_rd_model_handle ps_rd_model);

/* Returns the Qp to be used for the given bits and SAD */
UWORD8 irc_find_qp_for_target_bits(rc_rd_model_handle ps_rd_model,
                                   UWORD32 u4_target_res_bits,
                                   UWORD32 u4_estimated_sad,
                                   UWORD8 u1_max_qp,
                                   UWORD8 u1_min_qp);

/* Updates the frame level statistics after encoding a frame */
void irc_add_frame_to_rd_model(rc_rd_model_handle ps_rd_model,
                               UWORD32 i4_res_bits,
                               UWORD8 u1_avg_mp2qp,
                               UWORD32 i4_sad_h264,
                               UWORD8 u1_num_skips);

UWORD32 irc_estimate_bits_for_qp(rc_rd_model_handle ps_rd_model,
                                 UWORD32 u4_estimated_sad,
                                 UWORD8 u1_avg_qp);

/* Get the Linear model coefficient */
model_coeff irc_get_linear_coefficient(rc_rd_model_handle ps_rd_model);

WORD32 irc_calc_per_frm_bits(rc_rd_model_handle ps_rd_model,
                             UWORD16 *pu2_num_pics_of_a_pic_type,
                             UWORD8 *pu1_update_pic_type_model,
                             UWORD8 u1_num_pic_types,
                             UWORD32 *pu4_num_skip_of_a_pic_type,
                             UWORD8 u1_base_pic_type,
                             float *pfl_gamma,
                             float *pfl_eta,
                             UWORD8 u1_curr_pic_type,
                             UWORD32 u4_bits_for_sub_gop,
                             UWORD32 u4_curr_estimated_sad,
                             UWORD8 *pu1_curr_pic_type_qp);
#endif

