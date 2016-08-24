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
*  ih264e_modify_frm_rate.c
*
* @brief
*  Functions used to modify frame rate
*
* @author
*  ittiam
*
* @par List of Functions:
*  - ih264e_pd_frm_rate_get_init_free_memtab()
*  - ih264e_init_pd_frm_rate()
*  - ih264e_update_pd_frm_rate()
*  - ih264e_get_pd_avg_frm_rate()
*
* @remarks
*  None
*
*******************************************************************************
*/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* User include files */
#include "irc_datatypes.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264_defs.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "ih264e_defs.h"
#include "ime_distortion_metrics.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_rc_mem_interface.h"
#include "ih264e_time_stamp.h"
#include "ih264e_modify_frm_rate.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief Function to init pd frame rate memtab
*
* @par Description
*  Function to init pull down frame rate memtab
*
* @param[in] pps_pd_frm_rate
*  pull down frame rate context
*
* @param[in] ps_memtab
*  Handle to memtab
*
* @param[in] e_func_type
*  Function type (get memtab/ update memtab)
*
* @returns  Number of memtabs used
*
* @remarks  None
*
*******************************************************************************
*/
WORD32 ih264e_pd_frm_rate_get_init_free_memtab(pd_frm_rate_handle *pps_pd_frm_rate,
                                               itt_memtab_t *ps_memtab,
                                               ITT_FUNC_TYPE_E e_func_type)
{
    WORD32 i4_mem_tab_idx = 0;
    pd_frm_rate_t s_temp_pd_frm_rate_t;

    /* Hack for al alloc, during which we dont have any state memory.
     Dereferencing can cause issues */
    if (e_func_type == GET_NUM_MEMTAB || e_func_type == FILL_MEMTAB)
        (*pps_pd_frm_rate) = &s_temp_pd_frm_rate_t;

    /* for src rate control state structure */
    if (e_func_type != GET_NUM_MEMTAB)
    {
        fill_memtab(&ps_memtab[i4_mem_tab_idx], sizeof(pd_frm_rate_t),
                    ALIGN_128_BYTE, PERSISTENT, DDR);
        use_or_fill_base(&ps_memtab[0], (void**) pps_pd_frm_rate, e_func_type);
    }
    i4_mem_tab_idx++;

    return (i4_mem_tab_idx);
}

/**
*******************************************************************************
*
* @brief Initializes the pull down frame rate state structure based on input
*  frame rate
*
* @par Description
*  Initializes the pull down frame rate state structure based on input frame rate
*
* @param[in] ps_pd_frm_rate
*  Pull down frame rate context
*
* @param[in] u4_input_frm_rate
*  Input frame rate in frame per 1000sec
*
* @returns none
*
* @remarks
*
*******************************************************************************
*/
void ih264e_init_pd_frm_rate(pd_frm_rate_t *ps_pd_frm_rate,
                             UWORD32 u4_input_frm_rate)
{
    WORD32 i;

    ps_pd_frm_rate->u4_input_frm_rate = u4_input_frm_rate;

    for (i = 0; i < (WORD32) (u4_input_frm_rate / 1000); i++)
    {
        ps_pd_frm_rate->u4_cur_frm_rate[i] = u4_input_frm_rate;
    }

    ps_pd_frm_rate->u4_frm_num = 0;

    ps_pd_frm_rate->u4_tot_frm_encoded = 0;
}

/**
*******************************************************************************
*
* @brief Function to update pull down frame rate
*
* @par   Description
*  For each frame a run time frame rate value is sent based on whether a frame
*  is skipped or not. If it is skipped for pull down then the current frame
*  rate for the pull down period is signaled as 4/5th of the original frame
*  rate. Thus when this is averaged the frame rate gradually switches from the
*  input frame rate to 4/5th of input frame rate as and when more 3:2 pull
*  down patterns are detected
*
* @param[in] ps_pd_frm_rate
*  Pull down frame rate context
*
* @param[in] u4_input_frm_rate
*  Input frame rate in frame per 1000sec
*
* @returns none
*
* @remarks
*
*******************************************************************************
*/
void ih264e_update_pd_frm_rate(pd_frm_rate_t *ps_pd_frm_rate,
                               UWORD32 u4_cur_frm_rate)
{
    ps_pd_frm_rate->u4_cur_frm_rate[ps_pd_frm_rate->u4_frm_num] = u4_cur_frm_rate;

    ps_pd_frm_rate->u4_frm_num++;

    /* Increment the frame number */
    if (ps_pd_frm_rate->u4_tot_frm_encoded < (ps_pd_frm_rate->u4_input_frm_rate / 1000))
    {
        ps_pd_frm_rate->u4_tot_frm_encoded++;
    }

    /* Reset frm_num to zero  */
    if (ps_pd_frm_rate->u4_frm_num >= (ps_pd_frm_rate->u4_input_frm_rate / 1000))
    {
        ps_pd_frm_rate->u4_frm_num = 0;
    }
}

/**
*******************************************************************************
*
* @brief returns average frame rate in 1 sec duration
*
* @par Description
*  Averages the last N frame in period(1 sec) and then gives that
*  as the current frames frame rate. Thus this averages out the sudden
*  variation in frame rate
*
* @param[in] ps_pd_frm_rate
*  Handle to pull down frame rate context
*
* @returns average frame rate
*
* @remarks
*
*******************************************************************************
*/
UWORD32 ih264e_get_pd_avg_frm_rate(pd_frm_rate_t *ps_pd_frm_rate)
{
    WORD32 i;
    WORD32 i4_avg_frm_rate = 0;

    for (i = 0; i < (WORD32) ps_pd_frm_rate->u4_tot_frm_encoded; i++)
    {
        i4_avg_frm_rate += ps_pd_frm_rate->u4_cur_frm_rate[i];
    }

    i4_avg_frm_rate = i4_avg_frm_rate / ps_pd_frm_rate->u4_tot_frm_encoded;

    return i4_avg_frm_rate;
}
