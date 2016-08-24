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
#include "irc_common.h"
#include "irc_cntrl_param.h"
#include "irc_mem_req_and_acq.h"
#include "irc_fixed_point_error_bits.h"

typedef struct error_bits_t
{
    /* Max tgt frm rate so that dynamic change in frm rate can be handled */
    WORD32 i4_max_tgt_frm_rate;

    /* Cur frm rate */
    WORD32 i4_cur_tgt_frm_rate;

    /* tgt frame rate*/
    WORD32 i4_tgt_frm_rate;

    /* tgt frm rate increment */
    WORD32 i4_tgt_frm_rate_incr;

    /* flag to indicate 1 second is up */
    UWORD8 u1_compute_error_bits;

    /* Bitrate/frame rate value added over a period */
    WORD32 i4_accum_bitrate;

    /* bitrate */
    WORD32 i4_bitrate;

} error_bits_t;

WORD32 irc_error_bits_num_fill_use_free_memtab(error_bits_t **pps_error_bits,
                                               itt_memtab_t *ps_memtab,
                                               ITT_FUNC_TYPE_E e_func_type)
{
    WORD32 i4_mem_tab_idx = 0;
    error_bits_t s_error_bits_temp;

    /*
     * Hack for all alloc, during which we don't have any state memory.
     * Dereferencing can cause issues
     */
    if(e_func_type == GET_NUM_MEMTAB || e_func_type == FILL_MEMTAB)
        (*pps_error_bits) = &s_error_bits_temp;

    /* For src rate control state structure */
    if(e_func_type != GET_NUM_MEMTAB)
    {
        fill_memtab(&ps_memtab[i4_mem_tab_idx], sizeof(error_bits_t),
                    ALIGN_128_BYTE, PERSISTENT, DDR);
        use_or_fill_base(&ps_memtab[0], (void**)pps_error_bits, e_func_type);
    }
    i4_mem_tab_idx++;

    return (i4_mem_tab_idx);
}

/*******************************************************************************
 * @brief Calculates the error bits due to fixed point divisions
 ******************************************************************************/
void irc_init_error_bits(error_bits_t *ps_error_bits,
                         WORD32 i4_max_tgt_frm_rate,
                         WORD32 i4_bitrate)
{
    /* Initializing the parameters*/
    ps_error_bits->i4_cur_tgt_frm_rate = 0;
    ps_error_bits->i4_max_tgt_frm_rate = i4_max_tgt_frm_rate;

    /* Value by which i4_cur_tgt_frm_rate is incremented every VOP*/
    ps_error_bits->i4_tgt_frm_rate_incr = 1000;

    /*Compute error bits is set to 1 at the end of 1 second*/
    ps_error_bits->u1_compute_error_bits = 0;
    ps_error_bits->i4_tgt_frm_rate = i4_max_tgt_frm_rate;
    ps_error_bits->i4_accum_bitrate = 0;
    ps_error_bits->i4_bitrate = i4_bitrate;
}

/*******************************************************************************
 * @brief Updates the error state
 ******************************************************************************/
void irc_update_error_bits(error_bits_t *ps_error_bits)
{
    WORD32 i4_bits_per_frame;

    X_PROD_Y_DIV_Z(ps_error_bits->i4_bitrate, 1000,
                   ps_error_bits->i4_tgt_frm_rate, i4_bits_per_frame);

    /*
     * This value is incremented every at the end of every VOP by
     * i4_tgt_frm_rate_incr
     */
    ps_error_bits->i4_cur_tgt_frm_rate += ps_error_bits->i4_tgt_frm_rate_incr;
    if(ps_error_bits->u1_compute_error_bits == 1)
    {
        ps_error_bits->i4_accum_bitrate = 0;
    }
    ps_error_bits->i4_accum_bitrate += i4_bits_per_frame;

    /*
     * When current tgt frm rate is equal or greater than max tgt frame rate
     * 1 second is up , compute the error bits
     */
    if(ps_error_bits->i4_cur_tgt_frm_rate >= ps_error_bits->i4_max_tgt_frm_rate)
    {
        ps_error_bits->i4_cur_tgt_frm_rate -=
                        ps_error_bits->i4_max_tgt_frm_rate;
        ps_error_bits->u1_compute_error_bits = 1;
    }
    else
    {
        ps_error_bits->u1_compute_error_bits = 0;
    }
}

/*******************************************************************************
 * @brief Returns the error bits for the current frame if there are any
 *
 ******************************************************************************/
WORD32 irc_get_error_bits(error_bits_t *ps_error_bits)
{
    WORD32 i4_error_bits = 0;

    /*If 1s is up calculate error for the last 1s worth of frames*/
    if(ps_error_bits->u1_compute_error_bits == 1)
    {
        /*Error = Actual bitrate - bits_per_frame * num of frames*/
        i4_error_bits = ps_error_bits->i4_bitrate
                        - ps_error_bits->i4_accum_bitrate;
    }

    return (i4_error_bits);
}

/* *****************************************************************************
 *
 * @brief Change the frame rate parameter for the error bits state
 *
 ******************************************************************************/
void irc_change_frm_rate_in_error_bits(error_bits_t *ps_error_bits,
                                       WORD32 i4_tgt_frm_rate)
{
    /* Value by which i4_cur_tgt_frm_rate is incremented every VOP*/
    ps_error_bits->i4_tgt_frm_rate_incr = (ps_error_bits->i4_max_tgt_frm_rate
                                           * 1000) / i4_tgt_frm_rate;
    ps_error_bits->i4_tgt_frm_rate = i4_tgt_frm_rate;
}

/*******************************************************************************
 * @brief Change the bitrate value for error bits module
 ******************************************************************************/
void irc_change_bitrate_in_error_bits(error_bits_t *ps_error_bits,
                                      WORD32 i4_bitrate)
{
    ps_error_bits->i4_bitrate = i4_bitrate;
}

