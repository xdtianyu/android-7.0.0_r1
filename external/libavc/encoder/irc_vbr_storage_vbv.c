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
/* Includes */
/*****************************************************************************/

/* System include files */
#include <stdio.h>

/* User include files */
#include "irc_datatypes.h"
#include "irc_common.h"
#include "irc_cntrl_param.h"
#include "irc_mem_req_and_acq.h"
#include "irc_fixed_point_error_bits.h"
#include "irc_vbr_storage_vbv.h"
#include "irc_trace_support.h"

#define MAX(x, y)  ((x) > (y) ? (x) : (y))

typedef struct vbr_storage_vbv_t
{
    WORD32 i4_max_buf_size;
    WORD32 i4_cur_buf_size;
    WORD32 i4_max_bits_inflow_per_frm_period;
    WORD32 i4_max_bits_per_tgt_frm;
    /* Storing input variables */
    WORD32 i4_max_bit_rate;
    WORD32 i4_max_frame_rate;
    /* Error bits calculation module */
    error_bits_handle ps_error_bits;

} vbr_storage_vbv_t;

static void overflow_avoided_summation(WORD32 *pi4_accumulator, WORD32 i4_input)
{
    if((pi4_accumulator[0] > 0)
                    && (((int)0x7fffffff - pi4_accumulator[0]) < i4_input))
    {
        pi4_accumulator[0] = 0x7fffffff;
    }
    else if((pi4_accumulator[0] < 0)
                    && (((int)0x80000000 - pi4_accumulator[0]) > i4_input))
    {
        pi4_accumulator[0] = 0x80000000;
    }
    else
    {
        pi4_accumulator[0] += i4_input;
    }
}

WORD32 irc_vbr_vbv_num_fill_use_free_memtab(vbr_storage_vbv_t **pps_vbr_storage_vbv,
                                            itt_memtab_t *ps_memtab,
                                            ITT_FUNC_TYPE_E e_func_type)
{
    WORD32 i4_mem_tab_idx = 0;
    vbr_storage_vbv_t s_vbr_storage_vbv_temp;

    /*
     * Hack for al alloc, during which we don't have any state memory.
     * Dereferencing can cause issues
     */
    if(e_func_type == GET_NUM_MEMTAB || e_func_type == FILL_MEMTAB)
        (*pps_vbr_storage_vbv) = &s_vbr_storage_vbv_temp;

    /*for src rate control state structure*/
    if(e_func_type != GET_NUM_MEMTAB)
    {
        fill_memtab(&ps_memtab[i4_mem_tab_idx], sizeof(vbr_storage_vbv_t),
                    ALIGN_128_BYTE, PERSISTENT, DDR);
        use_or_fill_base(&ps_memtab[0], (void**)pps_vbr_storage_vbv,
                         e_func_type);
    }
    i4_mem_tab_idx++;

    i4_mem_tab_idx += irc_error_bits_num_fill_use_free_memtab(
                    &pps_vbr_storage_vbv[0]->ps_error_bits,
                    &ps_memtab[i4_mem_tab_idx], e_func_type);
    return (i4_mem_tab_idx);
}

void irc_init_vbr_vbv(vbr_storage_vbv_t *ps_vbr_storage_vbv,
                      WORD32 i4_max_bit_rate,
                      WORD32 i4_frm_rate,
                      WORD32 i4_max_vbv_buff_size)
{
    ps_vbr_storage_vbv->i4_max_buf_size = i4_max_vbv_buff_size;
    ps_vbr_storage_vbv->i4_cur_buf_size = i4_max_vbv_buff_size;

    /*
     * Calculate the max number of bits that flow into the decoder
     * in the interval of two frames
     */
    X_PROD_Y_DIV_Z(i4_max_bit_rate, 1000, i4_frm_rate,
                   ps_vbr_storage_vbv->i4_max_bits_inflow_per_frm_period);

    /* init error bits */
    irc_init_error_bits(ps_vbr_storage_vbv->ps_error_bits, i4_frm_rate,
                        i4_max_bit_rate);

    /* Storing the input values */
    ps_vbr_storage_vbv->i4_max_bits_per_tgt_frm =
                    ps_vbr_storage_vbv->i4_max_bits_inflow_per_frm_period;
    ps_vbr_storage_vbv->i4_max_bit_rate = i4_max_bit_rate;
    ps_vbr_storage_vbv->i4_max_frame_rate = i4_frm_rate;
}

void irc_update_vbr_vbv(vbr_storage_vbv_t *ps_vbr_storage_vbv,
                        WORD32 i4_total_bits_decoded)
{
    WORD32 i4_error_bits = irc_get_error_bits(
                    ps_vbr_storage_vbv->ps_error_bits);
    /*
     * In the time interval between two decoded frames the buffer would have been
     * filled up by the max_bits_inflow_per_frm_period.
     */
    overflow_avoided_summation(
                    &ps_vbr_storage_vbv->i4_cur_buf_size,
                    (ps_vbr_storage_vbv->i4_max_bits_inflow_per_frm_period
                                    + i4_error_bits));

    if(ps_vbr_storage_vbv->i4_cur_buf_size
                    > ps_vbr_storage_vbv->i4_max_buf_size)
    {
        ps_vbr_storage_vbv->i4_cur_buf_size =
                        ps_vbr_storage_vbv->i4_max_buf_size;
    }

    ps_vbr_storage_vbv->i4_cur_buf_size -= i4_total_bits_decoded;

    /* Update the error bits state */
    irc_update_error_bits(ps_vbr_storage_vbv->ps_error_bits);

}

WORD32 irc_get_max_target_bits(vbr_storage_vbv_t *ps_vbr_storage_vbv)
{
    WORD32 i4_cur_buf_size = ps_vbr_storage_vbv->i4_cur_buf_size;
    WORD32 i4_error_bits = irc_get_error_bits(
                    ps_vbr_storage_vbv->ps_error_bits);

    /* The buffer size when the next frame is decoded */
    overflow_avoided_summation(
                    &i4_cur_buf_size,
                    (ps_vbr_storage_vbv->i4_max_bits_inflow_per_frm_period
                                    + i4_error_bits));
    if(i4_cur_buf_size > ps_vbr_storage_vbv->i4_max_buf_size)
    {
        i4_cur_buf_size = ps_vbr_storage_vbv->i4_max_buf_size;
    }

    /*
     * Thus for the next frame the maximum number of bits the decoder can consume
     * without underflow is i4_cur_buf_size
     */
    return i4_cur_buf_size;
}

/****************************************************************************
 Function Name : irc_get_buffer_status
 Description   : Gets the state of VBV buffer
 Inputs        : Rate control API , header and texture bits
 Outputs       : 0 = normal, 1 = underflow, 2= overflow
 Returns       : vbv_buf_status_e
 *****************************************************************************/
vbv_buf_status_e irc_get_vbv_buffer_status(vbr_storage_vbv_t *ps_vbr_storage_vbv,
                                           WORD32 i4_total_frame_bits,
                                           WORD32 *pi4_num_bits_to_prevent_vbv_underflow)
{
    vbv_buf_status_e e_buf_status;
    WORD32 i4_cur_buf;
    WORD32 i4_error_bits = irc_get_error_bits(
                    ps_vbr_storage_vbv->ps_error_bits);

    /* error bits due to fixed point computation of drain rate*/
    i4_cur_buf = ps_vbr_storage_vbv->i4_cur_buf_size;
    overflow_avoided_summation(
                    &i4_cur_buf,
                    (ps_vbr_storage_vbv->i4_max_bits_inflow_per_frm_period
                                    + i4_error_bits));

    if(i4_cur_buf > ps_vbr_storage_vbv->i4_max_buf_size)
    {
        i4_cur_buf = ps_vbr_storage_vbv->i4_max_buf_size;
    }

    pi4_num_bits_to_prevent_vbv_underflow[0] = i4_cur_buf;

    i4_cur_buf -= i4_total_frame_bits;
    if(i4_cur_buf < 0)
    {
        e_buf_status = VBV_UNDERFLOW;
    }
    else if(i4_cur_buf > ps_vbr_storage_vbv->i4_max_buf_size)
    {
        e_buf_status = VBV_OVERFLOW;
    }
    else if(i4_cur_buf < (ps_vbr_storage_vbv->i4_max_buf_size >> 2))
    {
        e_buf_status = VBR_CAUTION;
    }
    else
    {
        e_buf_status = VBV_NORMAL;
    }

    return e_buf_status;
}

UWORD8 irc_restrict_swing_dvd_comp(vbr_storage_vbv_t *ps_vbr_storage_vbv)
{
    UWORD8 u1_restrict_swing = 1;

    if(ps_vbr_storage_vbv->i4_cur_buf_size
                    < (ps_vbr_storage_vbv->i4_max_buf_size >> 1))
    {
        u1_restrict_swing = 0;
    }

    return (u1_restrict_swing);
}

WORD32 irc_get_max_vbv_buf_size(vbr_storage_vbv_t *ps_vbr_storage_vbv)
{
    return (ps_vbr_storage_vbv->i4_max_buf_size);
}

WORD32 irc_get_cur_vbv_buf_size(vbr_storage_vbv_t *ps_vbr_storage_vbv)
{
    return (ps_vbr_storage_vbv->i4_cur_buf_size);
}

WORD32 irc_get_max_bits_inflow_per_frm_periode(vbr_storage_vbv_t *ps_vbr_storage_vbv)
{
    return (ps_vbr_storage_vbv->i4_max_bits_inflow_per_frm_period);
}

WORD32 irc_get_max_bits_per_tgt_frm(vbr_storage_vbv_t *ps_vbr_storage_vbv)
{
    return (ps_vbr_storage_vbv->i4_max_bits_per_tgt_frm);
}

WORD32 irc_vbv_get_vbv_buf_fullness(vbr_storage_vbv_t *ps_vbr_storage_vbv,
                                    UWORD32 u4_bits)
{
    WORD32 i4_error_bits = irc_get_error_bits(
                    ps_vbr_storage_vbv->ps_error_bits);
    WORD32 i4_cur_buf_size = ps_vbr_storage_vbv->i4_cur_buf_size;

    overflow_avoided_summation(
                    &i4_cur_buf_size,
                    (ps_vbr_storage_vbv->i4_max_bits_inflow_per_frm_period
                                    + i4_error_bits));

    if(i4_cur_buf_size > ps_vbr_storage_vbv->i4_max_buf_size)
    {
        i4_cur_buf_size = ps_vbr_storage_vbv->i4_max_buf_size;
    }

    i4_cur_buf_size -= u4_bits;

    return (i4_cur_buf_size);
}

WORD32 irc_get_max_tgt_bits_dvd_comp(vbr_storage_vbv_t *ps_vbr_storage_vbv,
                                     WORD32 i4_rem_bits_in_gop,
                                     WORD32 i4_rem_frms_in_gop,
                                     picture_type_e e_pic_type)
{
    WORD32 i4_dbf_max, i4_dbf_min, i4_dbf_prev, i4_vbv_size, i4_dbf_desired;
    WORD32 i4_max_tgt_bits;

    i4_vbv_size = ps_vbr_storage_vbv->i4_max_buf_size;
    i4_dbf_max = 95 * i4_vbv_size / 100;
    i4_dbf_min = 10 * i4_vbv_size / 100;
    i4_dbf_prev = ps_vbr_storage_vbv->i4_cur_buf_size;

    if(i4_rem_bits_in_gop < 0)
        i4_rem_bits_in_gop = 0;
    if(i4_rem_frms_in_gop <= 0)
        i4_rem_frms_in_gop = 1;

    if(e_pic_type == I_PIC)
    {
        i4_dbf_desired = i4_dbf_min;
    }
    else
    {
        i4_dbf_desired = (i4_dbf_max - i4_rem_bits_in_gop / i4_rem_frms_in_gop
                        - i4_dbf_prev) / i4_rem_frms_in_gop;
        i4_dbf_desired += i4_dbf_prev;
    }

    i4_dbf_prev += ps_vbr_storage_vbv->i4_max_bits_inflow_per_frm_period;
    if(i4_dbf_prev > ps_vbr_storage_vbv->i4_max_buf_size)
    {
        i4_dbf_prev = ps_vbr_storage_vbv->i4_max_buf_size;
    }

    i4_max_tgt_bits = MAX(0, (i4_dbf_prev - i4_dbf_desired));
    return (i4_max_tgt_bits);
}

void irc_change_vbr_vbv_frame_rate(vbr_storage_vbv_t *ps_vbr_storage_vbv,
                                   WORD32 i4_frm_rate)
{
    /*
     * Calculate the max number of bits that flow into the decoder
     * in the interval of two frames
     */
    X_PROD_Y_DIV_Z(ps_vbr_storage_vbv->i4_max_bit_rate, 1000, i4_frm_rate,
                   ps_vbr_storage_vbv->i4_max_bits_inflow_per_frm_period);

    /* Update the lower modules */
    irc_change_frm_rate_in_error_bits(ps_vbr_storage_vbv->ps_error_bits,
                                      i4_frm_rate);
    /* Storing the input values */
    ps_vbr_storage_vbv->i4_max_frame_rate = i4_frm_rate;
}

void irc_change_vbr_vbv_bit_rate(vbr_storage_vbv_t *ps_vbr_storage_vbv,
                                 WORD32 i4_max_bit_rate)
{
    /*
     * Calculate the max number of bits that flow into the decoder
     * in the interval of two frames
     */
    X_PROD_Y_DIV_Z(i4_max_bit_rate, 1000, ps_vbr_storage_vbv->i4_max_frame_rate,
                   ps_vbr_storage_vbv->i4_max_bits_inflow_per_frm_period);

    /* update the lower modules */
    irc_change_bitrate_in_error_bits(ps_vbr_storage_vbv->ps_error_bits,
                                     i4_max_bit_rate);

    /* Storing the input values */
    ps_vbr_storage_vbv->i4_max_bit_rate = i4_max_bit_rate;
}

void irc_change_vbr_max_bits_per_tgt_frm(vbr_storage_vbv_t *ps_vbr_storage_vbv,
                                         WORD32 i4_tgt_frm_rate)
{
    /*
     * Calculate the max number of bits that flow into the decoder
     * in the interval of two frames
     */
    X_PROD_Y_DIV_Z(ps_vbr_storage_vbv->i4_max_bit_rate, 1000, i4_tgt_frm_rate,
                   ps_vbr_storage_vbv->i4_max_bits_per_tgt_frm);

}
