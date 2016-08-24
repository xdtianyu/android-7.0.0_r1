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
*  ih264e_time_stamp.c
*
* @brief
*  This file contains functions used for source and target time stamp management
*
* @author
*  ittiam
*
* @par List of Functions:
*  - gcd()
*  - ih264e_get_range()
*  - ih264e_frame_time_get_init_free_memtab()
*  - ih264e_init_frame_time()
*  - ih264e_should_src_be_skipped()
*  - ih264e_time_stamp_get_init_free_memtab()
*  - ih264e_init_time_stamp()
*  - ih264e_update_time_stamp()
*  - ih264e_frame_time_get_src_frame_rate()
*  - ih264e_frame_time_get_tgt_frame_rate()
*  - ih264e_frame_time_get_src_ticks()
*  - ih264e_frame_time_get_tgt_ticks()
*  - ih264e_frame_time_get_src_time()
*  - ih264e_frame_time_get_tgt_time()
*  - ih264e_frame_time_update_src_frame_rate()
*  - ih264e_frame_time_update_tgt_frame_rate()
*  - ih264_time_stamp_update_frame_rate()
*
* @remarks
*  None
*
*******************************************************************************
*/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* user include files */
#include "irc_datatypes.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "ih264_defs.h"
#include "ih264e_defs.h"
#include "ime_distortion_metrics.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_rc_mem_interface.h"
#include "ih264e_time_stamp.h"
#include "irc_common.h"
#include "irc_rate_control_api.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief Function to compute gcd of two numbers
*
* @par   Description
*  Function to compute gcd of two numbers
*
* @param[in] i4_x
*  value 1
*
* @param[in] i4_y
*  value 2
*
* @returns
*  GCD(value 1, value 2)
*
* @remarks none
*
*******************************************************************************
*/
static WORD32 gcd(WORD32 i4_x, WORD32 i4_y)
{
    if (i4_x > i4_y)
    {
        i4_x = i4_y + i4_x;
        i4_y = i4_x - i4_y;
        i4_x = i4_x - i4_y;
    }
    while (i4_y != 0)
    {
        WORD32 temp;
        i4_x = i4_x % i4_y;
        temp = i4_x;
        i4_x = i4_y;
        i4_y = temp;
    }
    return (i4_x);
}

/**
*******************************************************************************
*
* @brief Function to determine number of bits required to represent a given
*  value
*
* @par   Description
*  This function determines the number of bits required to represent the given
*  value. It is used to find out number of bits to read when the data size is
*  not fixed (e.g. vop_time_increment_resolution).
*
* @param[in] u4_value
*  Value for which the number of bits required to represent is to be determined
*
* @param[in] u1_no_of_bits
*  Represents the value's word type = 8/16/32
*
* @returns
*  The number of bits required to represent the given number
*
* @remarks none
*
*******************************************************************************
*/
static UWORD8 ih264e_get_range(UWORD32 u4_value, UWORD8 u1_no_of_bits)
{
    UWORD8 count;
    UWORD32 temp;

    if (u4_value > (UWORD32) ((1 << (u1_no_of_bits >> 1)) - 1))
    {
        temp = (1 << (u1_no_of_bits - 1));
        for (count = 0; count < (u1_no_of_bits >> 1); count++)
        {
            if ((temp & u4_value) != 0)
            {
                return (UWORD8) (u1_no_of_bits - count);
            }
            else
            {
                temp >>= 1;
            }
        }
        return 0;
    }
    else
    {
        temp = (1 << ((u1_no_of_bits >> 1) - 1));
        for (count = 0; count < ((u1_no_of_bits >> 1) - 1); count++)
        {
            if ((temp & u4_value) != 0)
            {
                return (UWORD8) ((u1_no_of_bits >> 1) - count);
            }
            else
            {
                temp >>= 1;
            }
        }
        return 1;
    }
}

/**
*******************************************************************************
*
* @brief
*  Function to init frame time memtabs
*
* @par Description
*  Function to init frame time memtabs
*
* @param[in] pps_frame_time
*  Pointer to frame time contexts
*
* @param[in] ps_memtab
*  Pointer to memtab
*
* @param[in] e_func_type
*  Function type (get memtabs/init memtabs)
*
* @returns
*  none
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_frame_time_get_init_free_memtab(frame_time_handle *pps_frame_time,
                                              itt_memtab_t *ps_memtab,
                                              ITT_FUNC_TYPE_E e_func_type)
{
    WORD32 i4_mem_tab_idx = 0;
    frame_time_t s_temp_frame_time_t;

    /* Hack for al alloc, during which we dont have any state memory.
     Dereferencing can cause issues */
    if (e_func_type == GET_NUM_MEMTAB || e_func_type == FILL_MEMTAB)
        (*pps_frame_time) = &s_temp_frame_time_t;

    /* for src rate control state structure */
    if (e_func_type != GET_NUM_MEMTAB)
    {
        fill_memtab(&ps_memtab[i4_mem_tab_idx], sizeof(frame_time_t),
                    ALIGN_128_BYTE, PERSISTENT, DDR);
        use_or_fill_base(&ps_memtab[0], (void**) pps_frame_time, e_func_type);
    }
    i4_mem_tab_idx++;

    return (i4_mem_tab_idx);
}

/**
*******************************************************************************
*
* @brief
*  Function to init frame time context
*
* @par Description
*  Frame time structure stores the time of the source and the target frames to
*  be encoded. Based on the time we decide whether or not to encode the source
*  frame
*
* @param[in] ps_frame_time
*  Pointer Frame time context
*
* @param[in] u4_src_frm_rate
*  Source frame rate
*
* @param[in] u4_tgt_frm_rate
*  Target frame rate
*
* @returns
*  none
*
* @remarks
*
*******************************************************************************
*/
void ih264e_init_frame_time(frame_time_t *ps_frame_time,
                            UWORD32 u4_src_frm_rate,
                            UWORD32 u4_tgt_frm_rate)
{
    /* Initialise the common time base based on which the source and target
     * frame times increase */
    WORD32 i4_gcd = gcd(u4_src_frm_rate, u4_tgt_frm_rate);

    /* Avoiding overflow by doing calculations in float */
    number_t s_src_frm_rate, s_tgt_frm_rate, s_gcd, s_common_time_base, s_numerator;

    SET_VAR_Q(s_src_frm_rate, u4_src_frm_rate, 0);
    SET_VAR_Q(s_tgt_frm_rate, u4_tgt_frm_rate, 0);
    SET_VAR_Q(s_gcd, i4_gcd, 0);
    mult32_var_q(s_src_frm_rate, s_tgt_frm_rate, &s_numerator);
    div32_var_q(s_numerator, s_gcd, &s_common_time_base);
    number_t_to_word32(s_common_time_base, &(ps_frame_time->common_time_base));

    /* The source and target increment per vop is initialized */
    ps_frame_time->u4_src_frm_time_incr = ps_frame_time->common_time_base
                    / u4_src_frm_rate;
    ps_frame_time->u4_tgt_frm_time_incr = ps_frame_time->common_time_base
                    / u4_tgt_frm_rate;

    /* Initialise the source and target times to 0 (RESET) */
    ps_frame_time->u4_src_frm_time = 0;
    ps_frame_time->u4_tgt_frm_time = 0;

    /* Initialize the number of frms not to be skipped to 0 */
    ps_frame_time->u4_num_frms_dont_skip = 0;
}

/**
*******************************************************************************
*
* @brief
*  Function to check if frame can be skipped
*
* @par Description
*  Based on the source and target frame time and the delta time stamp
*  we decide whether to code the source or not.
*  This is based on the assumption
*  that the source frame rate is greater that target frame rate.
*  Updates the time_stamp structure
*
* @param[in] ps_frame_time
*  Handle to frame time context
*
* @param[in] u4_delta_time_stamp
*  Time stamp difference between frames
*
* @param[out] pu4_frm_not_skipped_for_dts
*  Flag to indicate if frame is already skipped by application
*
* @returns
*  Flag to skip frame
*
* @remarks
*
*******************************************************************************
*/
UWORD8 ih264e_should_src_be_skipped(frame_time_t *ps_frame_time,
                                    UWORD32 u4_delta_time_stamp,
                                    UWORD32 *pu4_frm_not_skipped_for_dts)
{
    UWORD8 skip_src = 0;

    if (ps_frame_time->u4_tgt_frm_time > ps_frame_time->u4_src_frm_time &&
        ps_frame_time->u4_tgt_frm_time >= (ps_frame_time->u4_src_frm_time +
                        ps_frame_time->u4_src_frm_time_incr))
    {
        skip_src = 1;
    }

    /* source time gets updated every frame */
    ps_frame_time->u4_src_frm_time += ps_frame_time->u4_src_frm_time_incr;

    /* target time gets updated only when the source is coded */
    if (!skip_src)
    {
        ps_frame_time->u4_tgt_frm_time += ps_frame_time->u4_tgt_frm_time_incr;
    }

    /* If the source and target frame times get incremented properly
     both should be equal to the common time base at the same time. If
     that happens we reset the time to zero*/
    if (( ps_frame_time->common_time_base ==(WORD32)ps_frame_time->u4_src_frm_time)
         && (ps_frame_time->common_time_base ==(WORD32) ps_frame_time->u4_tgt_frm_time ))
    {
        ps_frame_time->u4_src_frm_time = 0;
        ps_frame_time->u4_tgt_frm_time = 0;
    }

    /* This keeps a count of how many frames need not be skipped in order
     to take care of the delta time stamp */
    ps_frame_time->u4_num_frms_dont_skip += (u4_delta_time_stamp - 1);

    /** If this frame is to be skipped in order to maintain the tgt_frm_rate
     check if already a frame has been skipped by the application.
     In that case, do not skip this frame **/
    if (ps_frame_time->u4_num_frms_dont_skip && skip_src)
    {
        skip_src = 0;
        *pu4_frm_not_skipped_for_dts = 1;
        ps_frame_time->u4_num_frms_dont_skip -= 1;
    }
    else
    {
        pu4_frm_not_skipped_for_dts[0] = 0;
    }

    return (skip_src);
}

/**
*******************************************************************************
*
* @brief
*  Function to inititialize time stamp memtabs
*
* @par Description
*  Function to initialize time stamp memtabs
*
* @param[in] pps_time_stamp
*  Pointer to time stamp context
*
* @param[in] ps_memtab
*  Pointer to memtab
*
* @param[in] e_func_type
*  Funcion type (Get memtab/ init memtab)
*
* @returns
*   number of memtabs used
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_time_stamp_get_init_free_memtab(time_stamp_handle *pps_time_stamp,
                                              itt_memtab_t *ps_memtab,
                                              ITT_FUNC_TYPE_E e_func_type)
{
    WORD32 i4_mem_tab_idx = 0;
    time_stamp_t s_temp_time_stamp_t;

    /* Hack for al alloc, during which we dont have any state memory.
     Dereferencing can cause issues */
    if (e_func_type == GET_NUM_MEMTAB || e_func_type == FILL_MEMTAB)
        (*pps_time_stamp) = &s_temp_time_stamp_t;

    /* for src rate control state structure */
    if (e_func_type != GET_NUM_MEMTAB)
    {
        fill_memtab(&ps_memtab[i4_mem_tab_idx], sizeof(time_stamp_t),
                    ALIGN_128_BYTE, PERSISTENT, DDR);
        use_or_fill_base(&ps_memtab[0], (void**) pps_time_stamp, e_func_type);
    }
    i4_mem_tab_idx++;

    return (i4_mem_tab_idx);
}

/**
*******************************************************************************
*
* @brief
*  Function to initialize time stamp context
*
* @par Description
*  Time stamp structure stores the time stamp data that
*  needs to be sent in to the header of MPEG4. Based on the
*  max target frame rate the vop_time increment resolution is set
*  so as to support all the frame rates below max frame rate.
*  A support till the third decimal point is assumed.
*
* @param[in] ps_time_stamp
*  Pointer to time stamp structure
*
* @param[in] u4_max_frm_rate
*  Maximum frame rate
*
* @param[in] u4_src_frm_rate
*  Source frame rate
*
* @returns
*  none
*
* @remarks
*
*******************************************************************************
*/
void ih264e_init_time_stamp(time_stamp_t *ps_time_stamp,
                            UWORD32 u4_max_frm_rate,
                            UWORD32 u4_src_frm_rate)
{
    /* We expect the max frame rate to be less than 60000,
     * if not we divide it by zero and work with it */
    if (u4_max_frm_rate > 60000)
    {
        u4_max_frm_rate >>= 1;
        ps_time_stamp->is_max_frame_rate_scaled = 1;
    }
    else
    {
        ps_time_stamp->is_max_frame_rate_scaled = 0;
    }

    ps_time_stamp->u4_vop_time_incr_res = u4_max_frm_rate;
    ps_time_stamp->u4_vop_time_incr_range = ih264e_get_range(u4_max_frm_rate, 32);
    ps_time_stamp->u4_vop_time_incr = (ps_time_stamp->u4_vop_time_incr_res * 1000) / u4_src_frm_rate;/* Since frm rate is in millisec */
    ps_time_stamp->u4_vop_time = 0;
    ps_time_stamp->u4_cur_tgt_vop_time = 0;
    ps_time_stamp->u4_prev_tgt_vop_time = 0;
}

/**
*******************************************************************************
*
* @brief Function to update time stamp context
*
* @par Description
*  Vop time is incremented by increment value. When vop time goes
*  more than the vop time resolution set the modulo time base to
*  1 and reduce the vop time by vop time resolution so that the
*  excess value is present in vop time and get accumulated over time
*  so that the corresponding frame rate is achieved at a average of
*  1000 seconds
*
* @param[in] ps_time_stamp
*  Pointer to time stamp structure
*
* @returns
*  none
*
* @remarks
*
*******************************************************************************
*/
void ih264e_update_time_stamp(time_stamp_t *ps_time_stamp)
{
    /* Since get time stamp is called after the update
     A copy of the vop time and the modulo time is stored */
    ps_time_stamp->u4_cur_tgt_vop_time = ps_time_stamp->u4_vop_time;

    ps_time_stamp->u4_vop_time += ps_time_stamp->u4_vop_time_incr;
    if (ps_time_stamp->u4_vop_time >= ps_time_stamp->u4_vop_time_incr_res)
    {
        ps_time_stamp->u4_vop_time -= ps_time_stamp->u4_vop_time_incr_res;
    }
}

/****************************************************************************
                       Run-Time Modifying functions
****************************************************************************/

/**
*******************************************************************************
*
* @brief Function to get source frame rate
*
* @par Description
*  Function to get source frame rate
*
* @param[in] ps_frame_time
*  Pointer to frame time context
*
* @returns
*  source frame rate
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_frame_time_get_src_frame_rate(frame_time_t *ps_frame_time)
{
    return (ps_frame_time->common_time_base / ps_frame_time->u4_src_frm_time_incr);
}

/**
*******************************************************************************
*
* @brief Function to get target frame rate
*
* @par Description
*  Function to get target frame rate
*
* @param[in] ps_frame_time
*  Pointer to frame time context
*
* @returns
*   target frame rate
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_frame_time_get_tgt_frame_rate(frame_time_t *ps_frame_time)
{
    return (ps_frame_time->common_time_base / ps_frame_time->u4_tgt_frm_time_incr);
}

/**
*******************************************************************************
*
* @brief Function to get source time increment
*
* @par Description
*  Function to get source time increment
*
* @param[in] ps_frame_time
*  Pointer to frame time context
*
* @returns
*  source time increment
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_frame_time_get_src_ticks(frame_time_t *ps_frame_time)
{
    return (ps_frame_time->u4_src_frm_time_incr);
}

/**
*******************************************************************************
*
* @brief Function to get target time increment
*
* @par Description
*  Function to get target time increment
*
* @param[in] ps_frame_time
*  Pointer to frame time context
*
* @returns
*  target time increment
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_frame_time_get_tgt_ticks(frame_time_t *ps_frame_time)
{
    return (ps_frame_time->u4_tgt_frm_time_incr);
}

/**
*******************************************************************************
*
* @brief Function to get src frame time
*
* @par Description
*  Function to get src frame time
*
* @param[in] ps_frame_time
*  Pointer to frame time context
*
* @returns
*  src frame time
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_frame_time_get_src_time(frame_time_t *frame_time)
{
    return (frame_time->u4_src_frm_time);
}

/**
*******************************************************************************
*
* @brief Function to get tgt frame time
*
* @par Description
*  Function to get tgt frame time
*
* @param[in] ps_frame_time
*  Pointer to frame time context
*
* @returns
*  tgt frame time
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_frame_time_get_tgt_time(frame_time_t *frame_time)
{
    return (frame_time->u4_tgt_frm_time);
}

/**
*******************************************************************************
*
* @brief Function to update source frame time with a new source frame rate
*
* @par Description
*  Function to update source frame time with a new source frame rate
*
* @param[in] ps_frame_time
*  Pointer to frame time context
*
* @param[in] src_frm_rate
*  source frame rate
*
* @returns
*  None
*
* @remarks
*
*******************************************************************************
*/
void ih264e_frame_time_update_src_frame_rate(frame_time_t *ps_frame_time,
                                             WORD32 src_frm_rate)
{
    /* Since tgt frame rate does not change deriving the tgt_frm rate from
     * common_time_base */
    WORD32 tgt_frm_rate = ps_frame_time->common_time_base / ps_frame_time->u4_tgt_frm_time_incr;

    /* Re-initialise frame_time based on the new src_frame_rate and
     * old tgt_frame_rate */
    ih264e_init_frame_time(ps_frame_time, src_frm_rate, tgt_frm_rate);
}

/**
*******************************************************************************
*
* @brief Function to update target frame time with a new source frame rate
*
* @par Description
*  Function to update target frame time with a new source frame rate
*
* @param[in] ps_frame_time
*  Pointer to frame time context
*
* @param[in] tgt_frm_rate
*  target frame rate
*
* @returns
*  None
*
* @remarks
*
*******************************************************************************
*/
void ih264e_frame_time_update_tgt_frame_rate(frame_time_t *ps_frame_time,
                                             WORD32 tgt_frm_rate)
{
    /* Since src frame rate does not change deriving the src_frm rate from
     * common_time_base */
    WORD32 src_frm_rate = ps_frame_time->common_time_base / ps_frame_time->u4_src_frm_time_incr;

    /* Re-initialise frame_time based on the new tgt_frame_rate and
     * old src_frame_rate */
    ih264e_init_frame_time(ps_frame_time, src_frm_rate, tgt_frm_rate);
}

/**
*******************************************************************************
*
* @brief Function to update target frame time with a new source frame rate
*
* @par Description
*  When the frame rate changes the time increment is modified by appropriate ticks
*
* @param[in] ps_time_stamp
*  Pointer to time stamp structure
*
* @param[in] src_frm_rate
*  source frame rate
*
* @returns
*  None
*
* @remarks
*
*******************************************************************************
*/
void ih264_time_stamp_update_frame_rate(time_stamp_t *ps_time_stamp,
                                        UWORD32 src_frm_rate)
{
    ps_time_stamp->u4_vop_time_incr = (ps_time_stamp->u4_vop_time_incr_res * 1000) / src_frm_rate;/* Since frm rate is in millisec */
}
