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

#ifndef _BIT_ALLOCATION_H_
#define _BIT_ALLOCATION_H_

typedef struct bit_allocation_t *bit_allocation_handle;

WORD32 irc_ba_num_fill_use_free_memtab(bit_allocation_handle *pps_bit_allocation,
                                       itt_memtab_t *ps_memtab,
                                       ITT_FUNC_TYPE_E e_func_type);

void irc_ba_init_bit_allocation(bit_allocation_handle ps_bit_allocation,
                                pic_handling_handle ps_pic_handling,
                                WORD32 i4_num_intra_frm_interval,
                                WORD32 i4_bit_rate,
                                WORD32 i4_frm_rate,
                                WORD32 *u4_peak_bit_rate,
                                WORD32 i4_min_bitrate);

/* Estimates the number of texture bits required by the current frame */
WORD32 irc_ba_get_cur_frm_est_texture_bits(bit_allocation_handle ps_bit_allocation,
                                           rc_rd_model_handle *pps_rd_model,
                                           est_sad_handle ps_est_sad,
                                           pic_handling_handle ps_pic_handling,
                                           picture_type_e e_pic_type);

/* Estimate the number of header bits required by the current frame */
WORD32 irc_ba_get_cur_frm_est_header_bits(bit_allocation_handle ps_bit_allocation,
                                          picture_type_e e_pic_type);

/* Get the remaining bits allocated in the period */
WORD32 irc_ba_get_rem_bits_in_period(bit_allocation_handle ps_bit_allocation,
                                     pic_handling_handle ps_pic_handling);

WORD32 irc_ba_get_frame_rate(bit_allocation_handle ps_bit_allocation);

WORD32 irc_ba_get_bit_rate(bit_allocation_handle ps_bit_allocation);
void irc_ba_get_peak_bit_rate(bit_allocation_handle ps_bit_allocation,
                              WORD32 *pi4_peak_bit_rate);

/* Updates the bit allocation module with the actual encoded values */
void irc_ba_update_cur_frm_consumed_bits(bit_allocation_handle ps_bit_allocation,
                                         pic_handling_handle ps_pic_handling,
                                         WORD32 i4_total_frame_bits,
                                         WORD32 i4_model_updation_hdr_bits,
                                         picture_type_e e_pic_type,
                                         UWORD8 u1_is_scd,
                                         WORD32 i4_last_frm_in_gop);

void irc_ba_check_and_update_bit_allocation(bit_allocation_handle ps_bit_allocation,
                                            pic_handling_handle ps_pic_handling,
                                            WORD32 i4_cur_buf_size,
                                            WORD32 i4_max_buf_size,
                                            WORD32 i4_max_bits_inflow_per_frm,
                                            WORD32 i4_tot_frame_bits);

/* Based on the change in frame/bit rate update the remaining bits in period */
void irc_ba_change_remaining_bits_in_period(bit_allocation_handle ps_bit_allocation,
                                            pic_handling_handle ps_pic_handling,
                                            WORD32 i4_bit_rate,
                                            WORD32 i4_frame_rate,
                                            WORD32 *i4_peak_bit_rate);

/* Change the gop size in the middle of a current gop */
void change_gop_size(bit_allocation_handle ps_bit_allocation,
                     WORD32 i4_intra_frm_interval,
                     WORD32 i4_inter_frm_interval,
                     WORD32 i4_num_intra_frm_interval);

void update_rem_frms_in_period(bit_allocation_handle ps_bit_allocation,
                               picture_type_e e_pic_type,
                               UWORD8 u1_is_first_frm,
                               WORD32 i4_intra_frm_interval,
                               WORD32 i4_num_intra_frm_interval);

void irc_ba_change_rem_bits_in_prd_at_force_I_frame(bit_allocation_handle ps_bit_allocation,
                                                    pic_handling_handle ps_pic_handling);

void irc_ba_change_ba_peak_bit_rate(bit_allocation_handle ps_bit_allocation,
                                    WORD32 *ai4_peak_bit_rate);
#endif
