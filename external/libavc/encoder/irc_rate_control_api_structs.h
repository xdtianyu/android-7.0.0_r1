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

#ifndef _RATE_CONTROL_API_STRUCTS_H_
#define _RATE_CONTROL_API_STRUCTS_H_

/*
 * The following definitions were present in irc_cntrl_param.h, moved to this
 * file as it is used by irc_rate_control_api.c
 */

/* num_frm_in_period = BIT_ALLOC_PERIOD*intra_frame_interval */
#define VBR_BIT_ALLOC_PERIOD 3
#define CBR_BIT_ALLOC_PERIOD 1

/* Rate control state structure */
typedef struct rate_control_api_t
{
    /* RC Algorithm */
    rc_type_e e_rc_type;

    /* Whether MB level rc is enabled or not */
    UWORD8 u1_is_mb_level_rc_on;

    /* Picture handling struct */
    pic_handling_handle ps_pic_handling;

    /* Model struct for I and P frms */
    rc_rd_model_handle aps_rd_model[MAX_PIC_TYPE];

    /* VBR storage VBV structure */
    vbr_storage_vbv_handle ps_vbr_storage_vbv;

    /* Calculate the estimated SAD */
    est_sad_handle ps_est_sad;

    /* Allocation of bits for each frame */
    bit_allocation_handle ps_bit_allocation;

    /* Init Qp(also used for Const Qp scenarios) */
    UWORD8 au1_init_qp[MAX_PIC_TYPE];

    /* MB Level rate control state structure */
    mb_rate_control_handle ps_mb_rate_control;

    UWORD8 au1_is_first_frm_coded[MAX_PIC_TYPE];

    UWORD8 au1_prev_frm_qp[MAX_PIC_TYPE];

    cbr_buffer_handle ps_cbr_buffer;

    UWORD8 u1_scd_detected;

    UWORD8 u1_frm_qp_after_scd;

    UWORD8 au1_avg_bitrate_changed[MAX_PIC_TYPE];

    UWORD8 u1_is_first_frm;

    UWORD8 au1_min_max_qp[(MAX_PIC_TYPE << 1)];

    WORD32 i4_prev_frm_est_bits;

    vbr_str_prms_t s_vbr_str_prms;

    /* Store the values which are to be impacted after a delay */
    UWORD32 u4_frms_in_delay_prd_for_peak_bit_rate_change;

    UWORD32 au4_new_peak_bit_rate[MAX_NUM_DRAIN_RATES];

    picture_type_e prev_ref_pic_type;

} rate_control_api_t;

#endif/*_RATE_CONTROL_API_STRUCTS_H_*/

