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
/* File Includes                                                             */
/*****************************************************************************/

/* System include files */
#include <stdio.h>

/* User include files */
#include "irc_datatypes.h"
#include "irc_cntrl_param.h"
#include "irc_common.h"
#include "irc_mem_req_and_acq.h"
#include "irc_fixed_point_error_bits.h"
#include "irc_cbr_buffer_control.h"
#include "irc_trace_support.h"

typedef struct cbr_buffer_t
{
    /* Buffer size = Delay * Bitrate*/
    WORD32 i4_buffer_size;

    /* Constant drain rate */
    WORD32 i4_drain_bits_per_frame[MAX_NUM_DRAIN_RATES];

    /* Encoder Buffer Fullness */
    WORD32 i4_ebf;

    /* Upper threshold of the Buffer */
    WORD32 i4_upr_thr[MAX_PIC_TYPE];

    /* Lower threshold of the Buffer */
    WORD32 i4_low_thr[MAX_PIC_TYPE];

    /* Stuffing threshold equal to error bits per second in the drain bits
     * fixed point computation */
    WORD32 i4_stuffing_threshold;

    /* For error due to bits per frame calculation */
    error_bits_handle aps_bpf_error_bits[MAX_NUM_DRAIN_RATES];

    /* Whether the buffer model is used for CBR or VBR streaming */
    WORD32 i4_is_cbr_mode;

    /* Input parameters stored for initialization */
    WORD32 ai4_bit_rate[MAX_NUM_DRAIN_RATES];

    WORD32 i4_max_delay;

    WORD32 ai4_num_pics_in_delay_period[MAX_PIC_TYPE];

    WORD32 i4_tgt_frm_rate;

    UWORD32 u4_max_vbv_buf_size;

} cbr_buffer_t;

WORD32 irc_cbr_buffer_num_fill_use_free_memtab(cbr_buffer_t **pps_cbr_buffer,
                                               itt_memtab_t *ps_memtab,
                                               ITT_FUNC_TYPE_E e_func_type)
{
    WORD32 i4_mem_tab_idx = 0, i;
    cbr_buffer_t s_cbr_buffer_temp;

    /*
     * Hack for all alloc, during which we don't have any state memory.
     * Dereferencing can cause issues
     */
    if(e_func_type == GET_NUM_MEMTAB || e_func_type == FILL_MEMTAB)
        (*pps_cbr_buffer) = &s_cbr_buffer_temp;

    if(e_func_type != GET_NUM_MEMTAB)
    {
        fill_memtab(&ps_memtab[i4_mem_tab_idx], sizeof(cbr_buffer_t),
                    ALIGN_128_BYTE, PERSISTENT, DDR);
        use_or_fill_base(&ps_memtab[0], (void**)pps_cbr_buffer, e_func_type);
    }
    i4_mem_tab_idx++;

    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        i4_mem_tab_idx += irc_error_bits_num_fill_use_free_memtab(
                        &pps_cbr_buffer[0]->aps_bpf_error_bits[i],
                        &ps_memtab[i4_mem_tab_idx], e_func_type);
    }
    return (i4_mem_tab_idx);
}

/******************************************************************************
 * @brief Initialize the CBR VBV buffer state.
 * This could however be used for VBR streaming VBV also
 *
 ******************************************************************************/
void irc_init_cbr_buffer(cbr_buffer_t *ps_cbr_buffer,
                         WORD32 i4_buffer_delay,
                         WORD32 i4_tgt_frm_rate,
                         WORD32 *i4_bit_rate,
                         UWORD32 *u4_num_pics_in_delay_prd,
                         UWORD32 u4_vbv_buf_size)
{
    WORD32 i4_i, i4_bits_per_frm[MAX_NUM_DRAIN_RATES];
    int i;

    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        X_PROD_Y_DIV_Z(i4_bit_rate[i], 1000, i4_tgt_frm_rate,
                       i4_bits_per_frm[i]);
        /* Drain rate = bitrate/(framerate/1000) */
        ps_cbr_buffer->i4_drain_bits_per_frame[i] = i4_bits_per_frm[i];
        /* Initialize the bits per frame error bits calculation */
        irc_init_error_bits(ps_cbr_buffer->aps_bpf_error_bits[i],
                            i4_tgt_frm_rate, i4_bit_rate[i]);
    }

    /* Bitrate * delay = buffer size, divide by 1000 as delay is in ms*/
    /* This would mean CBR mode */
    if(i4_bit_rate[0] == i4_bit_rate[1])
    {
        X_PROD_Y_DIV_Z(i4_bit_rate[0], i4_buffer_delay, 1000,
                       ps_cbr_buffer->i4_buffer_size);
        ps_cbr_buffer->i4_is_cbr_mode = 1;
    }
    else
    {
        /* VBR streaming case which has different drain rates for I and P */
        ps_cbr_buffer->i4_buffer_size = u4_num_pics_in_delay_prd[0]
                                        * ps_cbr_buffer->i4_drain_bits_per_frame[0]
                                        + u4_num_pics_in_delay_prd[1]
                                        * ps_cbr_buffer->i4_drain_bits_per_frame[1];

        ps_cbr_buffer->i4_is_cbr_mode = 0;
    }

    if(ps_cbr_buffer->i4_buffer_size > (WORD32)u4_vbv_buf_size)
    {
        ps_cbr_buffer->i4_buffer_size = u4_vbv_buf_size;
    }

    /* Initially Encoder buffer fullness is zero */
    ps_cbr_buffer->i4_ebf = 0;

    /* tgt_frame_rate is divided by 1000 because, an approximate value is fine
     * as this is just a threshold below which stuffing is done to avoid buffer
     * underflow due to fixed point error in drain rate
     */
    ps_cbr_buffer->i4_stuffing_threshold = (i4_bit_rate[0]
                    - (i4_bits_per_frm[0] * (i4_tgt_frm_rate / 1000)));

    for(i4_i = 0; i4_i < MAX_PIC_TYPE; i4_i++)
    {
        /*
         * Upper threshold for
         * I frame = 1 * bits per frame
         * P Frame = 4 * bits per frame.
         * The threshold for I frame is only 1 * bits per frame as the threshold
         * should only account for error in estimated bits.
         * In P frame it should account for difference bets bits consumed by
         * I(Scene change) and P frame I to P complexity is assumed to be 5.
         */
        WORD32 i4_index;
        i4_index = i4_i > 0 ? 1 : 0;
        ps_cbr_buffer->i4_upr_thr[i4_i] = ps_cbr_buffer->i4_buffer_size
                        - (ps_cbr_buffer->i4_buffer_size >> 3);

        /*
         * For both I and P frame Lower threshold is equal to drain rate.Even if
         * the encoder consumes zero bits it should have enough bits to drain
         */
        ps_cbr_buffer->i4_low_thr[i4_i] = i4_bits_per_frm[i4_index];
    }

    /* Storing the input parameters for using it for change functions */
    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        ps_cbr_buffer->ai4_bit_rate[i] = i4_bit_rate[i];
    }

    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        ps_cbr_buffer->ai4_num_pics_in_delay_period[i] =
                        u4_num_pics_in_delay_prd[i];
    }
    ps_cbr_buffer->i4_tgt_frm_rate = i4_tgt_frm_rate;
    ps_cbr_buffer->i4_max_delay = i4_buffer_delay;
    ps_cbr_buffer->u4_max_vbv_buf_size = u4_vbv_buf_size;
}

/******************************************************************************
 * @brief Condition check for constraining the number of bits allocated based on
 * bufer size
 ******************************************************************************/
WORD32 irc_cbr_buffer_constraint_check(cbr_buffer_t *ps_cbr_buffer,
                                       WORD32 i4_tgt_bits,
                                       picture_type_e e_pic_type)
{
    WORD32 i4_max_tgt_bits, i4_min_tgt_bits;
    WORD32 i4_drain_bits_per_frame = (e_pic_type == I_PIC) ?
                                     ps_cbr_buffer->i4_drain_bits_per_frame[0] :
                                     ps_cbr_buffer->i4_drain_bits_per_frame[1];

    /* Max tgt bits = Upper threshold - current encoder buffer fullness */
    i4_max_tgt_bits = ps_cbr_buffer->i4_upr_thr[e_pic_type]
                    - ps_cbr_buffer->i4_ebf;
    /* Max tgt bits cannot be negative */
    if(i4_max_tgt_bits < 0)
        i4_max_tgt_bits = 0;

    /*
     * Min tgt bits , least number of bits in the Encoder after
     * draining such that it is greater than lower threshold
     */
    i4_min_tgt_bits = ps_cbr_buffer->i4_low_thr[e_pic_type]
                    - (ps_cbr_buffer->i4_ebf - i4_drain_bits_per_frame);
    /* Min tgt bits cannot be negative */
    if(i4_min_tgt_bits < 0)
        i4_min_tgt_bits = 0;

    /* Current tgt bits should be between max and min tgt bits */
    CLIP(i4_tgt_bits, i4_max_tgt_bits, i4_min_tgt_bits);
    return i4_tgt_bits;
}

/* *****************************************************************************
 * @brief constaints the bit allocation based on buffer size
 *
 ******************************************************************************/
WORD32 irc_vbr_stream_buffer_constraint_check(cbr_buffer_t *ps_cbr_buffer,
                                              WORD32 i4_tgt_bits,
                                              picture_type_e e_pic_type)
{
    WORD32 i4_max_tgt_bits;

    /* Max tgt bits = Upper threshold - current encoder buffer fullness */
    i4_max_tgt_bits = ps_cbr_buffer->i4_upr_thr[e_pic_type]
                    - ps_cbr_buffer->i4_ebf;

    /* Max tgt bits cannot be negative */
    if(i4_max_tgt_bits < 0)
        i4_max_tgt_bits = 0;

    if(i4_tgt_bits > i4_max_tgt_bits)
        i4_tgt_bits = i4_max_tgt_bits;

    return i4_tgt_bits;
}

/* *****************************************************************************
 * @brief Verifies the buffer state and returns whether it is overflowing,
 * underflowing or normal
 *
 ******************************************************************************/
vbv_buf_status_e irc_get_cbr_buffer_status(cbr_buffer_t *ps_cbr_buffer,
                                           WORD32 i4_tot_consumed_bits,
                                           WORD32 *pi4_num_bits_to_prevent_overflow,
                                           picture_type_e e_pic_type)
{
    vbv_buf_status_e e_buf_status;
    WORD32 i4_cur_enc_buf;
    WORD32 i4_error_bits = (e_pic_type == I_PIC) ?
                            irc_get_error_bits(ps_cbr_buffer
                                               ->aps_bpf_error_bits[0]) :
                            irc_get_error_bits(ps_cbr_buffer
                                               ->aps_bpf_error_bits[1]);

    WORD32 i4_drain_bits_per_frame = (e_pic_type == I_PIC) ?
                                     ps_cbr_buffer->i4_drain_bits_per_frame[0] :
                                     ps_cbr_buffer->i4_drain_bits_per_frame[1];

    /* Add the tot consumed bits to the Encoder Buffer*/
    i4_cur_enc_buf = ps_cbr_buffer->i4_ebf + i4_tot_consumed_bits;

    /* If the Encoder exceeds the Buffer Size signal an Overflow*/
    if(i4_cur_enc_buf > ps_cbr_buffer->i4_buffer_size)
    {
        e_buf_status = VBV_OVERFLOW;
        i4_cur_enc_buf = ps_cbr_buffer->i4_buffer_size;
    }
    else
    {
        /*
         * Subtract the constant drain bits and error bits due to fixed point
         * implementation
         */
        i4_cur_enc_buf -= (i4_drain_bits_per_frame + i4_error_bits);

        /*
         * If the buffer is less than stuffing threshold an Underflow is
         * signaled else its NORMAL
         */
        if(i4_cur_enc_buf < ps_cbr_buffer->i4_stuffing_threshold)
        {
            e_buf_status = VBV_UNDERFLOW;
        }
        else
        {
            e_buf_status = VBV_NORMAL;
        }

        if(i4_cur_enc_buf < 0)
            i4_cur_enc_buf = 0;
    }

    /*
     * The RC lib models the encoder buffer, but the VBV buffer characterizes
     * the decoder buffer
     */
    if(e_buf_status == VBV_OVERFLOW)
    {
        e_buf_status = VBV_UNDERFLOW;
    }
    else if(e_buf_status == VBV_UNDERFLOW)
    {
        e_buf_status = VBV_OVERFLOW;
    }

    pi4_num_bits_to_prevent_overflow[0] = (ps_cbr_buffer->i4_buffer_size
                    - i4_cur_enc_buf);

    return e_buf_status;
}

/*******************************************************************************
 * @brief Based on the bits consumed the buffer model is updated
 ******************************************************************************/
void irc_update_cbr_buffer(cbr_buffer_t *ps_cbr_buffer,
                           WORD32 i4_tot_consumed_bits,
                           picture_type_e e_pic_type)
{
    WORD32 i4_error_bits = (e_pic_type == I_PIC) ?
                           irc_get_error_bits(ps_cbr_buffer->
                                             aps_bpf_error_bits[0]) :
                           irc_get_error_bits( ps_cbr_buffer->
                                              aps_bpf_error_bits[1]);

    WORD32 i4_drain_bits_per_frame = (e_pic_type == I_PIC) ?
                                     ps_cbr_buffer->i4_drain_bits_per_frame[0] :
                                     ps_cbr_buffer->i4_drain_bits_per_frame[1];

    /* Update the Encoder buffer with the total consumed bits*/
    ps_cbr_buffer->i4_ebf += i4_tot_consumed_bits;

    /*
     * Subtract the drain bits and error bits due to fixed point
     * implementation
     */
    ps_cbr_buffer->i4_ebf -= (i4_drain_bits_per_frame + i4_error_bits);

    if(ps_cbr_buffer->i4_ebf < 0)
        ps_cbr_buffer->i4_ebf = 0;

    /*SS - Fix for lack of stuffing*/
    if(ps_cbr_buffer->i4_ebf > ps_cbr_buffer->i4_buffer_size)
    {
        trace_printf(
             (const WORD8*)"Error: Should not be coming here with stuffing\n");
        ps_cbr_buffer->i4_ebf = ps_cbr_buffer->i4_buffer_size;
    }
}

/*******************************************************************************
 * @brief If the buffer underflows then return the number of bits to prevent
 * underflow
 *
 ******************************************************************************/
WORD32 irc_get_cbr_bits_to_stuff(cbr_buffer_t *ps_cbr_buffer,
                                 WORD32 i4_tot_consumed_bits,
                                 picture_type_e e_pic_type)
{
    WORD32 i4_bits_to_stuff;
    WORD32 i4_error_bits = (e_pic_type == I_PIC) ?
                            irc_get_error_bits(ps_cbr_buffer
                                               ->aps_bpf_error_bits[0]) :
                            irc_get_error_bits(ps_cbr_buffer
                                               ->aps_bpf_error_bits[1]);

    WORD32 i4_drain_bits_per_frame = (e_pic_type == I_PIC) ?
                                     ps_cbr_buffer->i4_drain_bits_per_frame[0] :
                                     ps_cbr_buffer->i4_drain_bits_per_frame[1];

    /*
     * Stuffing bits got from the following equation
     * Stuffing_threshold = ebf + tcb - drain bits - error bits + stuff_bits
     */
    i4_bits_to_stuff = i4_drain_bits_per_frame + i4_error_bits
                    + ps_cbr_buffer->i4_stuffing_threshold
                    - (ps_cbr_buffer->i4_ebf + i4_tot_consumed_bits);

    return i4_bits_to_stuff;
}

/*******************************************************************************
 * @brief Update the state for change in number of pics in the delay period
 *
 ******************************************************************************/
void irc_change_cbr_vbv_num_pics_in_delay_period(cbr_buffer_t *ps_cbr_buffer,
                                                 UWORD32 *u4_num_pics_in_delay_prd)
{
    WORD32 i;

    if(!ps_cbr_buffer->i4_is_cbr_mode)
    {
        ps_cbr_buffer->i4_buffer_size =
                        u4_num_pics_in_delay_prd[0]
                        * ps_cbr_buffer->i4_drain_bits_per_frame[0]
                        + u4_num_pics_in_delay_prd[1]
                        * ps_cbr_buffer->i4_drain_bits_per_frame[1];

        if(ps_cbr_buffer->i4_buffer_size
                        > (WORD32)ps_cbr_buffer->u4_max_vbv_buf_size)
        {
            ps_cbr_buffer->i4_buffer_size = ps_cbr_buffer->u4_max_vbv_buf_size;
        }
        for(i = 0; i < MAX_PIC_TYPE; i++)
        {
            ps_cbr_buffer->i4_upr_thr[i] = ps_cbr_buffer->i4_buffer_size
                            - (ps_cbr_buffer->i4_buffer_size >> 3);
        }

        /* Re-initialize the number of pics in delay period */
        for(i = 0; i < MAX_PIC_TYPE; i++)
        {
            ps_cbr_buffer->ai4_num_pics_in_delay_period[i] =
                            u4_num_pics_in_delay_prd[i];
        }
    }
}

/******************************************************************************
 * @brief update the state for change in target frame rate
 *
 ******************************************************************************/
void irc_change_cbr_vbv_tgt_frame_rate(cbr_buffer_t *ps_cbr_buffer,
                                       WORD32 i4_tgt_frm_rate)
{
    WORD32 i4_i, i4_bits_per_frm[MAX_NUM_DRAIN_RATES];
    int i;

    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        X_PROD_Y_DIV_Z(ps_cbr_buffer->ai4_bit_rate[i], 1000, i4_tgt_frm_rate,
                       i4_bits_per_frm[i]);
        /* Drain rate = bitrate/(framerate/1000) */
        ps_cbr_buffer->i4_drain_bits_per_frame[i] = i4_bits_per_frm[i];
        /* Initialize the bits per frame error bits calculation */
        irc_change_frm_rate_in_error_bits(ps_cbr_buffer->aps_bpf_error_bits[i],
                                          i4_tgt_frm_rate);
    }

    /* Bitrate * delay = buffer size, divide by 1000 as delay is in ms*/
    if(!ps_cbr_buffer->i4_is_cbr_mode)
    {
        /* VBR streaming case which has different drain rates for I and P */
        ps_cbr_buffer->i4_buffer_size =
                        ps_cbr_buffer->ai4_num_pics_in_delay_period[0]
                      * ps_cbr_buffer->i4_drain_bits_per_frame[0]
                      + ps_cbr_buffer->ai4_num_pics_in_delay_period[1]
                      * ps_cbr_buffer->i4_drain_bits_per_frame[1];
    }

    if(ps_cbr_buffer->i4_buffer_size
                    > (WORD32)ps_cbr_buffer->u4_max_vbv_buf_size)
    {
        ps_cbr_buffer->i4_buffer_size = ps_cbr_buffer->u4_max_vbv_buf_size;
    }

    /*
     * Tgt_frame_rate is divided by 1000 because an approximate value is fine as
     * this is just a threshold below which stuffing is done to avoid buffer
     * underflow due to fixed point error in drain rate
     */
    ps_cbr_buffer->i4_stuffing_threshold = (ps_cbr_buffer->ai4_bit_rate[0]
                    - (i4_bits_per_frm[0] * (i4_tgt_frm_rate / 1000)));

    for(i4_i = 0; i4_i < MAX_PIC_TYPE; i4_i++)
    {
        /*
         * Upper threshold for
         * I frame = 1 * bits per frame
         * P Frame = 4 * bits per frame.
         * The threshold for I frame is only 1 * bits per frame as the threshold should
         * only account for error in estimated bits.
         * In P frame it should account for difference bets bits consumed by I(Scene change)
         * and P frame I to P complexity is assumed to be 5.
         */
        WORD32 i4_index;
        i4_index = i4_i > 0 ? 1 : 0;
        ps_cbr_buffer->i4_upr_thr[i4_i] = ps_cbr_buffer->i4_buffer_size
                        - (ps_cbr_buffer->i4_buffer_size >> 3);

        /*
         * For both I and P frame Lower threshold is equal to drain rate.
         * Even if the encoder consumes zero bits it should have enough bits to
         * drain
         */
        ps_cbr_buffer->i4_low_thr[i4_i] = i4_bits_per_frm[i4_index];
    }

    /* Storing the input parameters for using it for change functions */
    ps_cbr_buffer->i4_tgt_frm_rate = i4_tgt_frm_rate;
}

/*******************************************************************************
 * @brief Change the state for change in bit rate
 *
 ******************************************************************************/
void irc_change_cbr_vbv_bit_rate(cbr_buffer_t *ps_cbr_buffer,
                                 WORD32 *i4_bit_rate)
{
    WORD32 i4_i, i4_bits_per_frm[MAX_NUM_DRAIN_RATES];
    int i;

    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        X_PROD_Y_DIV_Z(i4_bit_rate[i], 1000, ps_cbr_buffer->i4_tgt_frm_rate,
                       i4_bits_per_frm[i]);
        /* Drain rate = bitrate/(framerate/1000) */
        ps_cbr_buffer->i4_drain_bits_per_frame[i] = i4_bits_per_frm[i];
        /* Initialize the bits per frame error bits calculation */
        irc_change_bitrate_in_error_bits(ps_cbr_buffer->aps_bpf_error_bits[i],
                                         i4_bit_rate[i]);
    }

    /* Bitrate * delay = buffer size, divide by 1000 as delay is in ms*/
    if(i4_bit_rate[0] == i4_bit_rate[1]) /* This would mean CBR mode */
    {
        X_PROD_Y_DIV_Z(i4_bit_rate[0], ps_cbr_buffer->i4_max_delay, 1000,
                       ps_cbr_buffer->i4_buffer_size);
        ps_cbr_buffer->i4_is_cbr_mode = 1;
    }
    else
    {
        /* VBR streaming case which has different drain rates for I and P */
        ps_cbr_buffer->i4_buffer_size =
                        ps_cbr_buffer->ai4_num_pics_in_delay_period[0]
                      * ps_cbr_buffer->i4_drain_bits_per_frame[0]
                      + ps_cbr_buffer->ai4_num_pics_in_delay_period[1]
                      * ps_cbr_buffer->i4_drain_bits_per_frame[1];

        ps_cbr_buffer->i4_is_cbr_mode = 0;
    }

    if(ps_cbr_buffer->i4_buffer_size
                    > (WORD32)ps_cbr_buffer->u4_max_vbv_buf_size)
    {
        ps_cbr_buffer->i4_buffer_size = ps_cbr_buffer->u4_max_vbv_buf_size;
    }

    /*
     * tgt_frame_rate is divided by 1000 because
     * an approximate value is fine as this is just a threshold below which
     * stuffing is done to avoid buffer underflow due to fixed point
     * error in drain rate
     */
    ps_cbr_buffer->i4_stuffing_threshold = (i4_bit_rate[0]
                    - (i4_bits_per_frm[0]
                                    * (ps_cbr_buffer->i4_tgt_frm_rate / 1000)));

    for(i4_i = 0; i4_i < MAX_PIC_TYPE; i4_i++)
    {
        /*
         * Upper threshold for
         * I frame = 1 * bits per frame
         * P Frame = 4 * bits per frame.
         * The threshold for I frame is only 1 * bits per frame as the threshold
         * should only account for error in estimated bits.
         * In P frame it should account for difference bets bits consumed by
         * I(Scene change) and P frame I to P complexity is assumed to be 5.
         */

        WORD32 i4_index;
        i4_index = i4_i > 0 ? 1 : 0;
        ps_cbr_buffer->i4_upr_thr[i4_i] = ps_cbr_buffer->i4_buffer_size
                        - (ps_cbr_buffer->i4_buffer_size >> 3);

        /* For both I and P frame Lower threshold is equal to drain rate.
         * Even if the encoder consumes zero bits it should have enough bits to
         * drain
         */
        ps_cbr_buffer->i4_low_thr[i4_i] = i4_bits_per_frm[i4_index];
    }

    /* Storing the input parameters for using it for change functions */
    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        ps_cbr_buffer->ai4_bit_rate[i] = i4_bit_rate[i];
    }
}

void irc_change_cbr_buffer_delay(cbr_buffer_t *ps_cbr_buffer,
                                 WORD32 i4_buffer_delay)
{
    WORD32 i4_i;

    /* Bitrate * delay = buffer size, divide by 1000 as delay is in ms*/
    if(ps_cbr_buffer->i4_is_cbr_mode)
    {
        X_PROD_Y_DIV_Z(ps_cbr_buffer->ai4_bit_rate[0], i4_buffer_delay, 1000,
                       ps_cbr_buffer->i4_buffer_size);
    }

    if(ps_cbr_buffer->i4_buffer_size
                    > (WORD32)ps_cbr_buffer->u4_max_vbv_buf_size)
    {
        ps_cbr_buffer->i4_buffer_size = ps_cbr_buffer->u4_max_vbv_buf_size;
    }

    for(i4_i = 0; i4_i < MAX_PIC_TYPE; i4_i++)
    {
        /*
         * Upper threshold for
         * I frame = 1 * bits per frame
         * P Frame = 4 * bits per frame.
         * The threshold for I frame is only 1 * bits per frame as the threshold
         * should only account for error in estimated bits.
         * In P frame it should account for difference bets bits consumed by I
         * (Scene change) and P frame I to P complexity is assumed to be 5.
         */
        ps_cbr_buffer->i4_upr_thr[i4_i] = ps_cbr_buffer->i4_buffer_size
                        - (ps_cbr_buffer->i4_buffer_size >> 3);
    }

    /* Storing the input parameters for using it for change functions */
    ps_cbr_buffer->i4_max_delay = i4_buffer_delay;
}

WORD32 irc_get_cbr_buffer_delay(cbr_buffer_t *ps_cbr_buffer)
{
    return (ps_cbr_buffer->i4_max_delay);
}

WORD32 irc_get_cbr_buffer_size(cbr_buffer_t *ps_cbr_buffer)
{
    return (ps_cbr_buffer->i4_buffer_size);
}
