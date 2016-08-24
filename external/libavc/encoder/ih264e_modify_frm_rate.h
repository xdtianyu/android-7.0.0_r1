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
*  ih264e_modify_frm_rate.h
*
* @brief
*  Functions declarations used to modify frame rate
*
* @author
*  ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_MODIFY_FRM_RATE_H_
#define IH264E_MODIFY_FRM_RATE_H_

/*****************************************************************************/
/* Constant Definitions                                                      */
/*****************************************************************************/

#define MAX_NUM_FRAME   120


/*****************************************************************************/
/* Structures                                                                */
/*****************************************************************************/
typedef struct pd_frm_rate_t
{
    /*
     * The input frame rate set in the encoder (per 1000 sec)
     */
    UWORD32 u4_input_frm_rate;

    /*
     * Frame rate of current frame due to pull down
     */
    UWORD32 u4_cur_frm_rate[MAX_NUM_FRAME];

    /*
     * current frame num in the above buffer
     */
    UWORD32 u4_frm_num;

    /*
     * Total number of frames encoded.
     * if greater than input frame rate stays at input frame rate
     */
    UWORD32 u4_tot_frm_encoded;

}pd_frm_rate_t;

typedef struct pd_frm_rate_t *pd_frm_rate_handle;


/*****************************************************************************/
/* Function Declarations                                                     */
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
                                               ITT_FUNC_TYPE_E e_func_type);
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
void ih264e_init_pd_frm_rate(pd_frm_rate_handle ps_pd_frm_rate,
                             UWORD32 u4_input_frm_rate);

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
void ih264e_update_pd_frm_rate(pd_frm_rate_handle ps_pd_frm_rate,
                               UWORD32 u4_cur_frm_rate);

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
UWORD32 ih264e_get_pd_avg_frm_rate(pd_frm_rate_handle ps_pd_frm_rate);

#endif /* IH264E_MODIFY_FRM_RATE_H_ */
