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

/** Includes */
#include <stdio.h>
#include <string.h>
#include "irc_datatypes.h"
#include "irc_mem_req_and_acq.h"
#include "irc_common.h"
#include "irc_cntrl_param.h"
#include "irc_fixed_point_error_bits.h"
#include "irc_rd_model.h"
#include "irc_est_sad.h"
#include "irc_picture_type.h"
#include "irc_bit_allocation.h"
#include "irc_trace_support.h"

/** Macros **/
#define MIN(x,y)  ((x) < (y))? (x) : (y)

/* State structure for bit allocation */
typedef struct
{
    /* using var_q number as it can cross 31 bits for large intra frameinterval */
    number_t vq_rem_bits_in_period;

    /* Storing inputs */
    WORD32 i4_tot_frms_in_gop;

    WORD32 i4_num_intra_frm_interval;

    WORD32 i4_bits_per_frm;

} rem_bit_in_prd_t;

typedef struct bit_allocation_t
{
    rem_bit_in_prd_t s_rbip;

    /* A universal constant giving the relative complexity between pictures */
    WORD32 i2_K[MAX_PIC_TYPE];

    /* To get a estimate of the header bits consumed */
    WORD32 i4_prev_frm_header_bits[MAX_PIC_TYPE];

    WORD32 i4_bits_per_frm;

    WORD32 i4_num_gops_in_period;

    /* Num gops as set by rate control module */
    WORD32 i4_actual_num_gops_in_period;

    number_t vq_saved_bits;

    WORD32 i4_max_bits_per_frm[MAX_NUM_DRAIN_RATES];

    WORD32 i4_min_bits_per_frm;

    /* Error bits module */
    error_bits_handle ps_error_bits;

    /* Storing frame rate */
    WORD32 i4_frame_rate;

    WORD32 i4_bit_rate;

    WORD32 ai4_peak_bit_rate[MAX_NUM_DRAIN_RATES];

} bit_allocation_t;

static WORD32 get_number_of_frms_in_a_gop(pic_handling_handle ps_pic_handling)
{
    WORD32 i4_tot_frms_in_gop = 0, i;
    WORD32 ai4_frms_in_gop[MAX_PIC_TYPE];

    /* Query the pic_handling struct for the rem frames in the period */
    irc_pic_type_get_frms_in_gop(ps_pic_handling, ai4_frms_in_gop);

    /* Get the total frms in the gop */
    i4_tot_frms_in_gop = 0;
    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        i4_tot_frms_in_gop += ai4_frms_in_gop[i];
    }
    return (i4_tot_frms_in_gop);
}

static void init_rbip(rem_bit_in_prd_t *ps_rbip,
                      pic_handling_handle ps_pic_handling,
                      WORD32 i4_bits_per_frm,
                      WORD32 i4_num_intra_frm_interval)
{
    WORD32 i4_tot_frms_in_gop = get_number_of_frms_in_a_gop(ps_pic_handling);

    /* rem_bits_in_period = bits_per_frm * tot_frms_in_gop * num_intra_frm_interval */
    {
        number_t vq_bits_per_frm, vq_tot_frms_in_gop, vq_num_intra_frm_interval;
        number_t *pvq_rem_bits_in_period = &ps_rbip->vq_rem_bits_in_period;

        SET_VAR_Q(vq_bits_per_frm, i4_bits_per_frm, 0);
        SET_VAR_Q(vq_tot_frms_in_gop, i4_tot_frms_in_gop, 0);
        SET_VAR_Q(vq_num_intra_frm_interval, i4_num_intra_frm_interval, 0);

        /* rem_bits_in_period = bits_per_frm * tot_frms_in_gop */
        mult32_var_q(vq_bits_per_frm, vq_tot_frms_in_gop,
                     pvq_rem_bits_in_period);

        /* rem_bits_in_period *= num_intra_frm_interval */
        mult32_var_q(vq_num_intra_frm_interval, pvq_rem_bits_in_period[0],
                     pvq_rem_bits_in_period);
    }

    /*
     * Store the total number of frames in GOP value which is
     * used from module A
     */
    ps_rbip->i4_tot_frms_in_gop = i4_tot_frms_in_gop;
    ps_rbip->i4_num_intra_frm_interval = i4_num_intra_frm_interval;
    ps_rbip->i4_bits_per_frm = i4_bits_per_frm;
}

static void check_update_rbip(rem_bit_in_prd_t *ps_rbip,
                              pic_handling_handle ps_pic_handling)
{
    /*
     * NOTE: Intra frame interval changes after the first I frame that is
     * encoded in a GOP
     */
    WORD32 i4_new_tot_frms_in_gop = get_number_of_frms_in_a_gop(
                    ps_pic_handling);

    if(i4_new_tot_frms_in_gop != ps_rbip->i4_tot_frms_in_gop)
    {
        WORD32 i4_rem_frames_in_period =
                        ps_rbip->i4_num_intra_frm_interval
                                        * (i4_new_tot_frms_in_gop
                                                        - ps_rbip->i4_tot_frms_in_gop);

        number_t vq_rem_frms_in_period, s_bits_per_frm, vq_delta_bits_in_period;

        SET_VAR_Q(vq_rem_frms_in_period, i4_rem_frames_in_period, 0);
        SET_VAR_Q(s_bits_per_frm, ps_rbip->i4_bits_per_frm, 0);

        /* delta_bits_in_period = bits_per_frm * rem_frms_in_period */
        mult32_var_q(s_bits_per_frm, vq_rem_frms_in_period,
                     &vq_delta_bits_in_period);

        /* rem_bits_in_period += delta_bits_in_period */
        add32_var_q(vq_delta_bits_in_period, ps_rbip->vq_rem_bits_in_period,
                    &ps_rbip->vq_rem_bits_in_period);
    }
    /* Updated the new values */
    ps_rbip->i4_tot_frms_in_gop = i4_new_tot_frms_in_gop;
}

static void irc_ba_update_rbip(rem_bit_in_prd_t *ps_rbip,
                               pic_handling_handle ps_pic_handling,
                               WORD32 i4_num_of_bits)
{
    number_t vq_num_bits;

    check_update_rbip(ps_rbip, ps_pic_handling);

    /* rem_bits_in_period += num_of_bits */
    SET_VAR_Q(vq_num_bits, i4_num_of_bits, 0);
    add32_var_q(vq_num_bits, ps_rbip->vq_rem_bits_in_period,
                &ps_rbip->vq_rem_bits_in_period);
}

static void irc_ba_change_rbip(rem_bit_in_prd_t *ps_rbip,
                               pic_handling_handle ps_pic_handling,
                               WORD32 i4_new_bits_per_frm,
                               WORD32 i4_new_num_intra_frm_interval)
{
    WORD32 ai4_rem_frms_in_period[MAX_PIC_TYPE], i4_rem_frms_in_gop, i;
    irc_pic_type_get_rem_frms_in_gop(ps_pic_handling, ai4_rem_frms_in_period);

    i4_rem_frms_in_gop = 0;
    for(i = 0; i < MAX_PIC_TYPE; i++)
        i4_rem_frms_in_gop += ai4_rem_frms_in_period[i];

    if(i4_new_bits_per_frm != ps_rbip->i4_bits_per_frm)
    {
        WORD32 i4_rem_frms_in_period = (ps_rbip->i4_num_intra_frm_interval - 1)
                        * ps_rbip->i4_tot_frms_in_gop + i4_rem_frms_in_gop;

        number_t vq_rem_frms_in_period, vq_delta_bits_per_frm,
                        vq_delta_bits_in_period;

        /* delta_bits_per_frm = new_bits_per_frm - old_bits_per_frm */
        SET_VAR_Q(vq_delta_bits_per_frm,
                  (i4_new_bits_per_frm - ps_rbip->i4_bits_per_frm), 0);

        SET_VAR_Q(vq_rem_frms_in_period, i4_rem_frms_in_period, 0);

        /* delta_bits_in_period = delta_bits_per_frm * rem_frms_in_period */
        mult32_var_q(vq_delta_bits_per_frm, vq_rem_frms_in_period,
                     &vq_delta_bits_in_period);

        /* ps_rbip->rem_bits_in_period += delta_bits_in_period */
        add32_var_q(vq_delta_bits_in_period, ps_rbip->vq_rem_bits_in_period,
                    &ps_rbip->vq_rem_bits_in_period);
    }

    if(i4_new_num_intra_frm_interval != ps_rbip->i4_num_intra_frm_interval)
    {
        WORD32 i4_rem_frms_in_period = ps_rbip->i4_tot_frms_in_gop
                        * (i4_new_num_intra_frm_interval
                                        - ps_rbip->i4_num_intra_frm_interval);

        number_t vq_rem_frms_in_period, vq_new_bits_per_frm,
                        vq_delta_bits_in_period;

        /* new_bits_per_frm = new_new_bits_per_frm - old_new_bits_per_frm */
        SET_VAR_Q(vq_new_bits_per_frm, i4_new_bits_per_frm, 0);

        SET_VAR_Q(vq_rem_frms_in_period, i4_rem_frms_in_period, 0);

        /* delta_bits_in_period = new_bits_per_frm * rem_frms_in_period */
        mult32_var_q(vq_new_bits_per_frm, vq_rem_frms_in_period,
                     &vq_delta_bits_in_period);

        /* ps_rbip->rem_bits_in_period += delta_bits_in_period */
        add32_var_q(vq_delta_bits_in_period, ps_rbip->vq_rem_bits_in_period,
                    &ps_rbip->vq_rem_bits_in_period);
    }
    /* Update the new value */
    ps_rbip->i4_num_intra_frm_interval = i4_new_num_intra_frm_interval;
    ps_rbip->i4_bits_per_frm = i4_new_bits_per_frm;
}

WORD32 irc_ba_num_fill_use_free_memtab(bit_allocation_t **pps_bit_allocation,
                                       itt_memtab_t *ps_memtab,
                                       ITT_FUNC_TYPE_E e_func_type)
{
    WORD32 i4_mem_tab_idx = 0;
    bit_allocation_t s_bit_allocation_temp;

    /*
     * Hack for all alloc, during which we don't have any state memory.
     * Dereferencing can cause issues
     */
    if(e_func_type == GET_NUM_MEMTAB || e_func_type == FILL_MEMTAB)
        (*pps_bit_allocation) = &s_bit_allocation_temp;

    /*for src rate control state structure*/
    if(e_func_type != GET_NUM_MEMTAB)
    {
        fill_memtab(&ps_memtab[i4_mem_tab_idx], sizeof(bit_allocation_t),
                    ALIGN_128_BYTE, PERSISTENT, DDR);
        use_or_fill_base(&ps_memtab[0], (void**)pps_bit_allocation,
                         e_func_type);
    }
    i4_mem_tab_idx++;

    i4_mem_tab_idx += irc_error_bits_num_fill_use_free_memtab(
                    &pps_bit_allocation[0]->ps_error_bits,
                    &ps_memtab[i4_mem_tab_idx], e_func_type);

    return (i4_mem_tab_idx);
}

/*******************************************************************************
 Function Name : irc_ba_init_bit_allocation
 Description   : Initialize the bit_allocation structure.
 ******************************************************************************/
void irc_ba_init_bit_allocation(bit_allocation_t *ps_bit_allocation,
                                pic_handling_handle ps_pic_handling,
                                WORD32 i4_num_intra_frm_interval,
                                WORD32 i4_bit_rate,
                                WORD32 i4_frm_rate,
                                WORD32 *i4_peak_bit_rate,
                                WORD32 i4_min_bitrate)
{
    WORD32 i;
    WORD32 i4_bits_per_frm, i4_max_bits_per_frm[MAX_NUM_DRAIN_RATES];

    /* Calculate the bits per frame */
    X_PROD_Y_DIV_Z(i4_bit_rate, 1000, i4_frm_rate, i4_bits_per_frm);
    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        X_PROD_Y_DIV_Z(i4_peak_bit_rate[i], 1000, i4_frm_rate,
                       i4_max_bits_per_frm[i]);
    }
    /* Initialize the bits_per_frame */
    ps_bit_allocation->i4_bits_per_frm = i4_bits_per_frm;
    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        ps_bit_allocation->i4_max_bits_per_frm[i] = i4_max_bits_per_frm[i];
    }
    X_PROD_Y_DIV_Z(i4_min_bitrate, 1000, i4_frm_rate,
                   ps_bit_allocation->i4_min_bits_per_frm);

    /*
     * Initialize the rem_bits in period
     * The first gop in case of an OPEN GOP may have fewer B_PICs,
     * That condition is not taken care of
     */
    init_rbip(&ps_bit_allocation->s_rbip, ps_pic_handling, i4_bits_per_frm,
              i4_num_intra_frm_interval);

    /* Initialize the num_gops_in_period */
    ps_bit_allocation->i4_num_gops_in_period = i4_num_intra_frm_interval;
    ps_bit_allocation->i4_actual_num_gops_in_period = i4_num_intra_frm_interval;

    /* Relative complexity between I and P frames */
    ps_bit_allocation->i2_K[I_PIC] = (1 << K_Q);
    ps_bit_allocation->i2_K[P_PIC] = I_TO_P_RATIO;
    ps_bit_allocation->i2_K[B_PIC] = (P_TO_B_RATIO * I_TO_P_RATIO) >> K_Q;

    /* Initialize the saved bits to 0*/
    SET_VAR_Q(ps_bit_allocation->vq_saved_bits, 0, 0);

    /* Update the error bits module with average bits */
    irc_init_error_bits(ps_bit_allocation->ps_error_bits, i4_frm_rate,
                        i4_bit_rate);
    /* Store the input for implementing change in values */
    ps_bit_allocation->i4_frame_rate = i4_frm_rate;
    ps_bit_allocation->i4_bit_rate = i4_bit_rate;

    memset(ps_bit_allocation->i4_prev_frm_header_bits, 0, sizeof(ps_bit_allocation->i4_prev_frm_header_bits));
    for(i=0;i<MAX_NUM_DRAIN_RATES;i++)
        ps_bit_allocation->ai4_peak_bit_rate[i] = i4_peak_bit_rate[i];
}

/*******************************************************************************
 Function Name : get_cur_frm_est_bits
 Description   : Based on remaining bits in period and rd_model
 the number of bits required for the current frame is estimated.
 ******************************************************************************/
WORD32 irc_ba_get_cur_frm_est_texture_bits(bit_allocation_t *ps_bit_allocation,
                                           rc_rd_model_handle *pps_rd_model,
                                           est_sad_handle ps_est_sad,
                                           pic_handling_handle ps_pic_handling,
                                           picture_type_e e_pic_type)
{
    WORD32 i, j;
    WORD32 i4_est_texture_bits_for_frm;
    number_t vq_rem_texture_bits;
    number_t vq_complexity_estimate[MAX_PIC_TYPE];
    WORD32 i4_rem_frms_in_period[MAX_PIC_TYPE], i4_frms_in_period[MAX_PIC_TYPE];
    number_t vq_max_consumable_bits;
    number_t vq_rem_frms_in_period[MAX_PIC_TYPE], vq_est_texture_bits_for_frm;
    number_t vq_prev_hdr_bits[MAX_PIC_TYPE];

    WORD32 complexity_est = 0;

    /* Get the rem_frms_in_gop & the frms_in_gop from the pic_type state struct */
    irc_pic_type_get_rem_frms_in_gop(ps_pic_handling, i4_rem_frms_in_period);
    irc_pic_type_get_frms_in_gop(ps_pic_handling, i4_frms_in_period);

    /* Depending on the number of gops in a period, find the num_frms_in_prd */
    for(j = 0; j < MAX_PIC_TYPE; j++)
    {
        i4_rem_frms_in_period[j] += (i4_frms_in_period[j]
                        * (ps_bit_allocation->i4_num_gops_in_period - 1));
        i4_frms_in_period[j] *= ps_bit_allocation->i4_num_gops_in_period;
    }

    /* Remove the header bits from the remaining bits to find how many bits you
     can transfer.*/
    irc_ba_update_rbip(&ps_bit_allocation->s_rbip, ps_pic_handling, 0);
    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        SET_VAR_Q(vq_rem_frms_in_period[i], i4_rem_frms_in_period[i], 0);
        SET_VAR_Q(vq_prev_hdr_bits[i],
                  ps_bit_allocation->i4_prev_frm_header_bits[i], 0);
    }
    {
        /*
         *rem_texture_bits = rem_bits_in_period -
         *(rem_frms_in_period[I_PIC] * prev_frm_header_bits[I_PIC]) -
         *(rem_frms_in_period[P_PIC] * prev_frm_header_bits[P_PIC]) -
         *(rem_frms_in_period[B_PIC] * prev_frm_header_bits[B_PIC]);
         */
        number_t vq_rem_hdr_bits;
        vq_rem_texture_bits = ps_bit_allocation->s_rbip.vq_rem_bits_in_period;

        mult32_var_q(vq_prev_hdr_bits[I_PIC], vq_rem_frms_in_period[I_PIC],
                     &vq_rem_hdr_bits);
        sub32_var_q(vq_rem_texture_bits, vq_rem_hdr_bits, &vq_rem_texture_bits);

        mult32_var_q(vq_prev_hdr_bits[P_PIC], vq_rem_frms_in_period[P_PIC],
                     &vq_rem_hdr_bits);
        sub32_var_q(vq_rem_texture_bits, vq_rem_hdr_bits, &vq_rem_texture_bits);

        mult32_var_q(vq_prev_hdr_bits[B_PIC], vq_rem_frms_in_period[B_PIC],
                     &vq_rem_hdr_bits);
        sub32_var_q(vq_rem_texture_bits, vq_rem_hdr_bits, &vq_rem_texture_bits);
    }
    {
        /* max_consumable_bits =
         *(frms_in_period[I_PIC] * max_bits_per_frm[0] ) +
         *(frms_in_period[P_PIC] + frms_in_period[B_PIC] ) * max_bits_per_frm[1];
         */
        number_t vq_max_bits, vq_max_bits_per_frm[2];

        SET_VAR_Q(vq_max_bits_per_frm[0],
                  ps_bit_allocation->i4_max_bits_per_frm[0], 0);
        SET_VAR_Q(vq_max_bits_per_frm[1],
                  ps_bit_allocation->i4_max_bits_per_frm[1], 0);

        mult32_var_q(vq_rem_frms_in_period[I_PIC], vq_max_bits_per_frm[0],
                     &vq_max_bits);
        vq_max_consumable_bits = vq_max_bits;

        mult32_var_q(vq_rem_frms_in_period[P_PIC], vq_max_bits_per_frm[1],
                     &vq_max_bits);
        add32_var_q(vq_max_bits, vq_max_consumable_bits,
                    &vq_max_consumable_bits);

        mult32_var_q(vq_rem_frms_in_period[B_PIC], vq_max_bits_per_frm[1],
                     &vq_max_bits);
        add32_var_q(vq_max_bits, vq_max_consumable_bits,
                    &vq_max_consumable_bits);
    }

    /* rem_texture_bits = MIN(rem_texture_bits, max_consumable_bits) */
    MIN_VARQ(vq_max_consumable_bits, vq_rem_texture_bits, vq_rem_texture_bits);

    /* The bits are then allocated based on the relative complexity of the
     current frame with respect to that of the rest of the frames in period */
    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        number_t vq_lin_mod_coeff, vq_est_sad, vq_K;

        /* Getting the linear model coefficient */
        vq_lin_mod_coeff = irc_get_linear_coefficient(pps_rd_model[i]);

        /* Getting the estimated SAD */
        SET_VAR_Q(vq_est_sad, irc_get_est_sad(ps_est_sad,i), 0);

        /* Making K factor a var Q format */
        SET_VAR_Q(vq_K, ps_bit_allocation->i2_K[i], K_Q);

        /* Complexity_estimate = [ (lin_mod_coeff * estimated_sad) / K factor ]  */
        mult32_var_q(vq_lin_mod_coeff, vq_est_sad, &vq_lin_mod_coeff);
        div32_var_q(vq_lin_mod_coeff, vq_K, &vq_complexity_estimate[i]);
    }

    /*
     * For simple cases, one of the complexities go to zero and in those cases
     * distribute the bits evenly among frames based on I_TO_P_RATIO
     */

    /* Also check the B-pictures complexity only in case they are present*/
    if(i4_frms_in_period[B_PIC] == 0)
    {
        complexity_est = (vq_complexity_estimate[I_PIC]
                        && vq_complexity_estimate[P_PIC]);
    }
    else
    {
        complexity_est = (vq_complexity_estimate[I_PIC]
                        && vq_complexity_estimate[P_PIC]
                        && vq_complexity_estimate[B_PIC]);
    }

    if(complexity_est)
    {
        /*
         * Estimated texture bits =
         * (remaining bits) * (cur frm complexity)
         * ---------------------------------------
         * (num_i_frm*i_frm_complexity) + (num_p_frm*pfrm_complexity)
         *  + (b_frm * b_frm_cm)
         */
        mult32_var_q(vq_rem_texture_bits, vq_complexity_estimate[e_pic_type],
                     &vq_rem_texture_bits);

        for(i = 0; i < MAX_PIC_TYPE; i++)
        {
            mult32_var_q(vq_rem_frms_in_period[i], vq_complexity_estimate[i],
                         &vq_rem_frms_in_period[i]);
        }

        add32_var_q(vq_rem_frms_in_period[I_PIC], vq_rem_frms_in_period[P_PIC],
                    &vq_rem_frms_in_period[I_PIC]);

        add32_var_q(vq_rem_frms_in_period[I_PIC], vq_rem_frms_in_period[B_PIC],
                    &vq_rem_frms_in_period[I_PIC]);

        div32_var_q(vq_rem_texture_bits, vq_rem_frms_in_period[I_PIC],
                    &vq_est_texture_bits_for_frm);

        number_t_to_word32(vq_est_texture_bits_for_frm,
                           &i4_est_texture_bits_for_frm);
    }
    else
    {
        number_t vq_i_to_p_bit_ratio, vq_rem_frms;

        SET_VAR_Q(vq_i_to_p_bit_ratio, I_TO_P_BIT_RATIO, 0);

        /* rem_frms = ((I_TO_P_BIT_RATIO * rem_frms_in_period[I_PIC]) +
         * rem_frms_in_period[P_PIC]  +  rem_frms_in_period[B_PIC]);
         */
        mult32_var_q(vq_rem_frms_in_period[I_PIC], vq_i_to_p_bit_ratio,
                     &vq_rem_frms);
        add32_var_q(vq_rem_frms_in_period[P_PIC], vq_rem_frms, &vq_rem_frms);
        add32_var_q(vq_rem_frms_in_period[B_PIC], vq_rem_frms, &vq_rem_frms);

        /* est_texture_bits_for_frm = rem_texture_bits / rem_frms */
        div32_var_q(vq_rem_texture_bits, vq_rem_frms,
                    &vq_est_texture_bits_for_frm);
        number_t_to_word32(vq_est_texture_bits_for_frm,
                           &i4_est_texture_bits_for_frm);

        i4_est_texture_bits_for_frm =
                        (I_PIC == e_pic_type) ?
                                        (i4_est_texture_bits_for_frm
                                                        * I_TO_P_BIT_RATIO) :
                                        i4_est_texture_bits_for_frm;
    }

    /*
     * If the remaining bits in the period becomes negative then the estimated
     * texture bits would also become negative. This would send a feedback to
     * the model which may go for a toss. Thus sending the minimum possible
     * value = 0
     */
    if(i4_est_texture_bits_for_frm < 0)
    {
        i4_est_texture_bits_for_frm = 0;
    }

    return (i4_est_texture_bits_for_frm);
}

/******************************************************************************
 Function Name : irc_ba_get_cur_frm_est_header_bits
 Description   : Based on remaining bits in period and rd_model
                 the number of bits required for the current frame is estimated.
 ******************************************************************************/
WORD32 irc_ba_get_cur_frm_est_header_bits(bit_allocation_t *ps_bit_allocation,
                                          picture_type_e e_pic_type)
{
    return (ps_bit_allocation->i4_prev_frm_header_bits[e_pic_type]);
}

WORD32 irc_ba_get_rem_bits_in_period(bit_allocation_t *ps_bit_allocation,
                                     pic_handling_handle ps_pic_handling)
{
    WORD32 i4_rem_bits_in_gop = 0;
    irc_ba_update_rbip(&ps_bit_allocation->s_rbip, ps_pic_handling, 0);
    number_t_to_word32(ps_bit_allocation->s_rbip.vq_rem_bits_in_period,
                       &i4_rem_bits_in_gop);
    return (i4_rem_bits_in_gop);
}

/*******************************************************************************
 Function Name : irc_ba_update_cur_frm_consumed_bits
 Description   : Based on remaining bits in period and rd_model
                 the number of bits required for the current frame is estimated.
 ******************************************************************************/
void irc_ba_update_cur_frm_consumed_bits(bit_allocation_t *ps_bit_allocation,
                                         pic_handling_handle ps_pic_handling,
                                         WORD32 i4_total_frame_bits,
                                         WORD32 i4_model_updation_hdr_bits,
                                         picture_type_e e_pic_type,
                                         UWORD8 u1_is_scd,
                                         WORD32 i4_last_frm_in_gop)
{
    WORD32 i4_error_bits = irc_get_error_bits(ps_bit_allocation->ps_error_bits);

    /* Update the remaining bits in period */
    irc_ba_update_rbip(&ps_bit_allocation->s_rbip, ps_pic_handling,
                       (-i4_total_frame_bits + i4_error_bits));

    /*
     * Update the header bits so that it can be used as an estimate to the next
     * frame
     */
    if(u1_is_scd)
    {
        /*
         * In case of SCD, even though the frame type is P, it is equivalent to
         * a I frame and so the corresponding header bits is updated
         */
        ps_bit_allocation->i4_prev_frm_header_bits[I_PIC] =
                        i4_model_updation_hdr_bits;

#define MAX_NUM_GOPS_IN_PERIOD (3)
        if(ps_bit_allocation->i4_num_gops_in_period < MAX_NUM_GOPS_IN_PERIOD)
        {
            /*
             * Whenever there is a scene change increase the number of gops by
             * 2 so that the number of bits allocated is not very constrained
             */
            ps_bit_allocation->i4_num_gops_in_period += 2;
            /* Add the extra bits in GOP to remaining bits in period */
            irc_ba_change_rbip(&ps_bit_allocation->s_rbip, ps_pic_handling,
                               ps_bit_allocation->i4_bits_per_frm,
                               ps_bit_allocation->i4_num_gops_in_period);
        }
    }
    else
    {
        ps_bit_allocation->i4_prev_frm_header_bits[e_pic_type] =
                        i4_model_updation_hdr_bits;
    }

    if(i4_last_frm_in_gop)
    {
        WORD32 i4_num_bits_in_a_gop = get_number_of_frms_in_a_gop(
                        ps_pic_handling) * ps_bit_allocation->i4_bits_per_frm;
        /*
         * If the number of gops in period has been increased due to scene
         * change, slowly bring in down across the gops
         */
        if(ps_bit_allocation->i4_num_gops_in_period
                        > ps_bit_allocation->i4_actual_num_gops_in_period)
        {
            ps_bit_allocation->i4_num_gops_in_period--;
            irc_ba_change_rbip(&ps_bit_allocation->s_rbip, ps_pic_handling,
                               ps_bit_allocation->i4_bits_per_frm,
                               ps_bit_allocation->i4_num_gops_in_period);
        }
        /*
         * If rem_bits_in_period < 0 decrease the number of bits allocated for
         * the next period else increase it
         */
        irc_ba_update_rbip(&ps_bit_allocation->s_rbip, ps_pic_handling,
                           i4_num_bits_in_a_gop);
    }
    /* Update the lower modules */
    irc_update_error_bits(ps_bit_allocation->ps_error_bits);
}

void irc_ba_change_remaining_bits_in_period(bit_allocation_t *ps_bit_allocation,
                                            pic_handling_handle ps_pic_handling,
                                            WORD32 i4_bit_rate,
                                            WORD32 i4_frame_rate,
                                            WORD32 *i4_peak_bit_rate)
{
    WORD32 i4_new_avg_bits_per_frm;
    WORD32 i4_new_peak_bits_per_frm[MAX_NUM_DRAIN_RATES];
    WORD32 i4_rem_frms_in_period[MAX_PIC_TYPE];
    int i;

    /* Calculate the new per frame bits */
    X_PROD_Y_DIV_Z(i4_bit_rate, 1000, i4_frame_rate, i4_new_avg_bits_per_frm);
    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        X_PROD_Y_DIV_Z(i4_peak_bit_rate[i], 1000, i4_frame_rate,
                       i4_new_peak_bits_per_frm[i]);
    }

    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        ps_bit_allocation->i4_max_bits_per_frm[i] = i4_new_peak_bits_per_frm[i];
    }

    /*
     * Get the rem_frms_in_prd & the frms_in_prd from the pic_type state
     * struct
     */
    irc_pic_type_get_rem_frms_in_gop(ps_pic_handling, i4_rem_frms_in_period);

    /*
     * If the difference > 0(/ <0), the remaining bits in period needs to be
     * increased(/decreased) based on the remaining number of frames
     */
    irc_ba_change_rbip(&ps_bit_allocation->s_rbip, ps_pic_handling,
                       i4_new_avg_bits_per_frm,
                       ps_bit_allocation->i4_num_gops_in_period);

    /* Update the new average bits per frame */
    ps_bit_allocation->i4_bits_per_frm = i4_new_avg_bits_per_frm;
    /* change the lower modules state */
    irc_change_bitrate_in_error_bits(ps_bit_allocation->ps_error_bits,
                                     i4_bit_rate);
    irc_change_frm_rate_in_error_bits(ps_bit_allocation->ps_error_bits,
                                      i4_frame_rate);

    /* Store the modified frame_rate */
    ps_bit_allocation->i4_frame_rate = i4_frame_rate;
    ps_bit_allocation->i4_bit_rate = i4_bit_rate;
    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
        ps_bit_allocation->ai4_peak_bit_rate[i] = i4_peak_bit_rate[i];
}

void irc_ba_change_ba_peak_bit_rate(bit_allocation_t *ps_bit_allocation,
                                    WORD32 *ai4_peak_bit_rate)
{
    WORD32 i;

    /* Calculate the bits per frame */
    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        X_PROD_Y_DIV_Z(ai4_peak_bit_rate[i], 1000,
                       ps_bit_allocation->i4_frame_rate,
                       ps_bit_allocation->i4_max_bits_per_frm[i]);
        ps_bit_allocation->ai4_peak_bit_rate[i] = ai4_peak_bit_rate[i];
    }
}

/******************************************************************************
 * @brief Modifies the remaining bit in period for the gop which has fif.
 *      since fif would cause a new gop to be created, we need to add the number
 *      of encoded frames in the fif GOP worth of bits to remaining bits in
 *      period
 ******************************************************************************/
void irc_ba_change_rem_bits_in_prd_at_force_I_frame(bit_allocation_t *ps_bit_allocation,
                                                    pic_handling_handle ps_pic_handling)
{
    WORD32 i4_frms_in_period;
    i4_frms_in_period = irc_pic_type_get_frms_in_gop_force_I_frm(
                    ps_pic_handling);
    irc_ba_update_rbip(&ps_bit_allocation->s_rbip, ps_pic_handling,
                       ps_bit_allocation->i4_bits_per_frm * i4_frms_in_period);
}

void irc_ba_check_and_update_bit_allocation(bit_allocation_t *ps_bit_allocation,
                                            pic_handling_handle ps_pic_handling,
                                            WORD32 i4_cur_buf_size,
                                            WORD32 i4_max_buf_size,
                                            WORD32 i4_max_bits_inflow_per_frm,
                                            WORD32 i4_tot_frame_bits)
{

    number_t vq_max_drain_bits, vq_extra_bits, vq_less_bits,
                    vq_allocated_saved_bits, vq_min_bits_for_period;
    WORD32 i4_num_frms_in_period = get_number_of_frms_in_a_gop(ps_pic_handling);
    number_t vq_rem_bits_in_period, vq_num_frms_in_period, vq_zero;
    WORD32 b_rem_bits_gt_max_drain, b_rem_bits_lt_min_bits,
                    b_saved_bits_gt_zero;
    rem_bit_in_prd_t *ps_rbip = &ps_bit_allocation->s_rbip;

    UNUSED(i4_cur_buf_size);
    UNUSED(i4_max_buf_size);
    UNUSED(i4_tot_frame_bits);

    /*
     * If the remaining bits is greater than what can be drained in that period
     * Clip the remaining bits in period to the maximum it can drain in that
     * period with the error of current buffer size.Accumulate the saved bits
     * if any. else if the remaining bits is lesser than the minimum bit rate
     * promised in that period Add the excess bits to remaining bits in period
     * and reduce it from the saved bits Else Provide the extra bits from the
     * "saved bits pool".
     */
    /*
     * max_drain_bits = num_gops_in_period * num_frms_in_period *
     * * max_bits_inflow_per_frm
     */
    SET_VAR_Q(vq_num_frms_in_period,
              (ps_bit_allocation->i4_num_gops_in_period * i4_num_frms_in_period),
              0);
    SET_VAR_Q(vq_max_drain_bits, i4_max_bits_inflow_per_frm, 0);
    SET_VAR_Q(vq_zero, 0, 0);
    mult32_var_q(vq_max_drain_bits, vq_num_frms_in_period, &vq_max_drain_bits);

    /*
     * min_bits_for_period = num_gops_in_period * num_frms_in_period *
     * min_bits_per_frm
     */
    SET_VAR_Q(vq_min_bits_for_period, ps_bit_allocation->i4_min_bits_per_frm,
              0);
    mult32_var_q(vq_min_bits_for_period, vq_num_frms_in_period,
                 &vq_min_bits_for_period);

    vq_rem_bits_in_period = ps_rbip->vq_rem_bits_in_period;

    /* Evaluate rem_bits_in_period  > max_drain_bits      */
    VQ_A_GT_VQ_B(ps_rbip->vq_rem_bits_in_period, vq_max_drain_bits,
                 b_rem_bits_gt_max_drain);

    /* Evaluate rem_bits_in_period  < min_bits_for_period */
    VQ_A_LT_VQ_B(ps_rbip->vq_rem_bits_in_period, vq_min_bits_for_period,
                 b_rem_bits_lt_min_bits);

    /* Evaluate saved_bits  > 0 */
    VQ_A_LT_VQ_B(ps_bit_allocation->vq_saved_bits, vq_zero,
                 b_saved_bits_gt_zero);

    /* (i4_rem_bits_in_period > i4_max_drain_bits) */
    if(b_rem_bits_gt_max_drain)
    {
        /* extra_bits = rem_bits_in_period - max_drain_bits */
        sub32_var_q(ps_rbip->vq_rem_bits_in_period, vq_max_drain_bits,
                    &vq_extra_bits);

        /* saved_bits += extra_bits */
        add32_var_q(ps_bit_allocation->vq_saved_bits, vq_extra_bits,
                    &ps_bit_allocation->vq_saved_bits);

        /* rem_bits_in_period = vq_max_drain_bits */
        ps_rbip->vq_rem_bits_in_period = vq_max_drain_bits;
    }
    else if(b_rem_bits_lt_min_bits)
    {
        /* extra_bits(-ve) =  rem_bits_in_period - i4_min_bits_for_period */
        sub32_var_q(ps_rbip->vq_rem_bits_in_period, vq_min_bits_for_period,
                    &vq_extra_bits);

        /* saved_bits += extra_bits(-ve) */
        add32_var_q(ps_bit_allocation->vq_saved_bits, vq_extra_bits,
                    &ps_bit_allocation->vq_saved_bits);

        /* rem_bits_in_period = min_bits_for_period */
        ps_rbip->vq_rem_bits_in_period = vq_min_bits_for_period;
    }
    else if(b_saved_bits_gt_zero)
    {
        /* less_bits = max_drain_bits - _rem_bits_in_period */
        sub32_var_q(vq_max_drain_bits, vq_rem_bits_in_period, &vq_less_bits);

        /* allocated_saved_bits = MIN (less_bits, saved_bits) */
        MIN_VARQ(ps_bit_allocation->vq_saved_bits, vq_less_bits,
                 vq_allocated_saved_bits);

        /* rem_bits_in_period += allocted_save_bits */
        add32_var_q(ps_rbip->vq_rem_bits_in_period, vq_allocated_saved_bits,
                    &ps_rbip->vq_rem_bits_in_period);

        /* saved_bits -= allocted_save_bits */
        sub32_var_q(ps_bit_allocation->vq_saved_bits, vq_allocated_saved_bits,
                    &ps_bit_allocation->vq_saved_bits);
    }
    return;
}

WORD32 irc_ba_get_frame_rate(bit_allocation_t *ps_bit_allocation)
{
    return (ps_bit_allocation->i4_frame_rate);
}

WORD32 irc_ba_get_bit_rate(bit_allocation_t *ps_bit_allocation)
{
    return (ps_bit_allocation->i4_bit_rate);
}

void irc_ba_get_peak_bit_rate(bit_allocation_t *ps_bit_allocation,
                              WORD32 *pi4_peak_bit_rate)
{
    WORD32 i;
    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        pi4_peak_bit_rate[i] = ps_bit_allocation->ai4_peak_bit_rate[i];
    }
}
