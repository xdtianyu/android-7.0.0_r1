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

#ifndef _RATE_CONTROL_API_H_
#define _RATE_CONTROL_API_H_

#define RC_OK            0
#define RC_FAIL         -1
#define RC_BENIGN_ERR   -2

/* This file should only contain RC API function declarations */

typedef struct rate_control_api_t *rate_control_handle;

WORD32 irc_rate_control_num_fill_use_free_memtab(rate_control_handle *pps_rate_control_api,
                                                 itt_memtab_t *ps_memtab,
                                                 ITT_FUNC_TYPE_E e_func_type);

void irc_initialise_rate_control(rate_control_handle ps_rate_control_api,
                                 rc_type_e e_rate_control_type,
                                 UWORD8 u1_is_mb_level_rc_on,
                                 UWORD32 u4_avg_bit_rate,
                                 UWORD32 *pu4_peak_bit_rate,
                                 UWORD32 u4_min_bit_rate,
                                 UWORD32 u4_frame_rate,
                                 UWORD32 u4_max_delay,
                                 UWORD32 u4_intra_frame_interval,
                                 WORD32 i4_inter_frm_int,
                                 UWORD8 *pu1_init_qp,
                                 UWORD32 u4_max_vbv_buff_size,
                                 WORD32 i4_max_inter_frm_int,
                                 WORD32 i4_is_gop_closed,
                                 UWORD8 *pu1_min_max_qp,
                                 WORD32 i4_use_est_intra_sad,
                                 UWORD32 u4_src_ticks,
                                 UWORD32 u4_tgt_ticks);

/*****************************************************************************
 Process level API fuctions (FRAME LEVEL)
 *****************************************************************************/
void irc_flush_buf_frames(rate_control_handle ps_rate_control_api);

void irc_post_encode_frame_skip(rate_control_handle ps_rate_control_api,
                                picture_type_e e_pic_type);

void irc_add_picture_to_stack(rate_control_handle rate_control_api,
                              WORD32 i4_enc_pic_id);

void irc_add_picture_to_stack_re_enc(rate_control_handle rate_control_api,
                                     WORD32 i4_enc_pic_id,
                                     picture_type_e e_pic_type);

void irc_get_picture_details(rate_control_handle rate_control_api,
                             WORD32 *pi4_pic_id,
                             WORD32 *pi4_pic_disp_order_no,
                             picture_type_e *pe_pic_type);

/* Gets the frame level Qp */
UWORD8 irc_get_frame_level_qp(rate_control_handle rate_control_api,
                              picture_type_e pic_type,
                              WORD32 i4_max_frm_bits);

vbv_buf_status_e irc_get_buffer_status(rate_control_handle rate_control_api,
                                       WORD32 i4_total_frame_bits,
                                       picture_type_e e_pic_type,
                                       WORD32 *pi4_num_bits_to_prevent_vbv_underflow);

WORD32 irc_get_prev_frm_est_bits(rate_control_handle ps_rate_control_api);

void irc_update_pic_handling_state(rate_control_handle ps_rate_control_api,
                                   picture_type_e e_pic_type);

void irc_update_frame_level_info(rate_control_handle ps_rate_control_api,
                                 picture_type_e e_pic_type,
                                 WORD32 *pi4_mb_type_sad,
                                 WORD32 i4_total_frame_bits,
                                 WORD32 i4_model_updation_hdr_bits,
                                 WORD32 *pi4_mb_type_tex_bits,
                                 WORD32 *pi4_tot_mb_type_qp,
                                 WORD32 *pi4_tot_mb_in_type,
                                 WORD32 i4_avg_activity,
                                 UWORD8 u1_is_scd,
                                 WORD32 i4_is_it_a_skip,
                                 WORD32 i4_intra_frm_cost,
                                 WORD32 i4_is_pic_handling_done);

/*****************************************************************************
 MB LEVEL API (just wrapper fucntions)
 *****************************************************************************/

void irc_init_mb_rc_frame_level(rate_control_handle ps_rate_control_api,
                                UWORD8 u1_frame_qp);/* Current frame qp*/

void irc_get_mb_level_qp(rate_control_handle ps_rate_control_api,
                         WORD32 i4_cur_mb_activity,
                         WORD32 *pi4_mb_qp,
                         picture_type_e e_pic_type);

WORD32 irc_get_bits_to_stuff(rate_control_handle ps_rate_control_api,
                             WORD32 i4_tot_consumed_bits,
                             picture_type_e e_pic_type);

/******************************************************************************
 Control Level API functions
 Logic: The control call sets the state structure of the rate control api
 accordingly such that the next process call would implement the same.
 ******************************************************************************/

void irc_change_inter_frm_int_call(rate_control_handle ps_rate_control_api,
                                   WORD32 i4_inter_frm_int);

void irc_change_intra_frm_int_call(rate_control_handle ps_rate_control_api,
                                   WORD32 i4_intra_frm_int);

void irc_change_avg_bit_rate(rate_control_handle ps_rate_control_api,
                             UWORD32 u4_average_bit_rate);

void irc_change_frame_rate(rate_control_handle ps_rate_control_api,
                           UWORD32 u4_frame_rate,
                           UWORD32 u4_src_ticks,
                           UWORD32 u4_target_ticks);

void irc_change_frm_rate_for_bit_alloc(rate_control_handle ps_rate_control_api,
                                       UWORD32 u4_frame_rate);

void irc_change_init_qp(rate_control_handle ps_rate_control_api,
                        UWORD8 *init_qp);

WORD32 irc_change_peak_bit_rate(rate_control_handle ps_rate_control_api,
                                UWORD32 *u4_peak_bit_rate);

void irc_change_buffer_delay(rate_control_handle ps_rate_control_api,
                             UWORD32 u4_buffer_delay);

void irc_force_I_frame(rate_control_handle ps_rate_control_api);

void irc_change_min_max_qp(rate_control_handle ps_rate_control_api,
                           UWORD8 *u1_min_max_qp);

/********************************************************************************
 Getter functions
 For getting the current state of the rate control structures
 ********************************************************************************/

UWORD32 irc_get_frame_rate(rate_control_handle ps_rate_control_api);

UWORD32 irc_get_bit_rate(rate_control_handle ps_rate_control_api);

UWORD32 irc_get_intra_frame_interval(rate_control_handle ps_rate_control_api);

UWORD32 irc_get_inter_frame_interval(rate_control_handle ps_rate_control_api);

rc_type_e irc_get_rc_type(rate_control_handle ps_rate_control_api);

WORD32 irc_get_bits_per_frame(rate_control_handle ps_rate_control_api);

UWORD32 irc_get_peak_bit_rate(rate_control_handle ps_rate_control_api,
                              WORD32 i4_index);

UWORD32 irc_get_max_delay(rate_control_handle ps_rate_control_api);

UWORD32 irc_get_seq_no(rate_control_handle ps_rate_control_api);

WORD32 irc_get_rem_bits_in_period(rate_control_handle ps_rate_control_api);

WORD32 irc_get_vbv_buf_fullness(rate_control_handle ps_rate_control_api);

WORD32 irc_get_vbv_buf_size(rate_control_handle ps_rate_control_api);

WORD32 irc_get_vbv_fulness_with_cur_bits(rate_control_handle ps_rate_control_api,
                                         UWORD32 u4_bits);
#endif
