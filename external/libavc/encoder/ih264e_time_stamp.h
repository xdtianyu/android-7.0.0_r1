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
*  ih264e_time_stamp.h
*
* @brief
*  This file contains function declarations used for managing input and output
*  frame time stamps
*
* @author
*  ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_TIME_STAMP_H_
#define IH264E_TIME_STAMP_H_

/*****************************************************************************/
/* Structures                                                                */
/*****************************************************************************/

/**
 * Parameters for Src/Tgt frames that are encoded
 */
typedef struct frame_time_t
{
    /* common time base(=LCM) between source and target frame rate (in ticks)*/
    WORD32 common_time_base;

    /* number of ticks between two source frames */
    UWORD32 u4_src_frm_time_incr;

    /* number of ticks between two target frames */
    UWORD32 u4_tgt_frm_time_incr;

    /* Source frame time - measured as modulo of common time base
     and incremented by src_frm_time_incr */
    UWORD32 u4_src_frm_time;

    /* Target frame time - measured as modulo of common time base
     and incremented by tgt_frm_time_incr */
    UWORD32 u4_tgt_frm_time;

    /* Number of frames not to be skipped while maintaining
     tgt_frm_rate due to delta_time_stamp  */
    UWORD32 u4_num_frms_dont_skip;
}frame_time_t;

typedef struct frame_time_t *frame_time_handle;

/**
 *  Parameters that go in the bitstream based on tgt_frm_rate
 *   1) Initialize the vop_time_incr_res with the max_frame_rate (in frames per 1000 bits)
 *      - To represent all kinds of frame rates
 *   2) Decide the vop_time_incr based on the source frame rate
 *      - The decoder would like to know which source frame is encoded i.e. the source time
 *    id of the target frame encoded and there by adjusting its time of delay
 *   3) vop_time increments every source frame and whenever a frame is encoded (target frame),
 *      the encoder queries the vop time of the source frame and sends it in the bit stream.
 *   4) Since the Source frame skip logic is taken care by the frame_time module, whenever the
 *      encoder queries the time stamp module (which gets updated outside the encoder) the
 *      time stamp module would have the source time
 */
typedef struct time_stamp_t
{
    /*vop_time_incr_res is a integer that indicates
     the number of evenly spaced subintervals, called ticks,
     within one modulo time. */
    UWORD32 u4_vop_time_incr_res;

    /* number of bits to represent vop_time_incr_res */
    UWORD32 u4_vop_time_incr_range;

    /* The number of ticks elapsed between two source vops */
    UWORD32 u4_vop_time_incr;

    /* incremented by vop_time_incr for every source frame.
     Represents the time offset after a modulo_time_base = 1 is sent
     in bit stream*/
    UWORD32 u4_vop_time;

    /* A temporary buffer to copy of vop time and modulo time base
     is stored since update is called before query (get time stamp) and
     so these extra variables cur_tgt_vop_time,  */
    UWORD32 u4_cur_tgt_vop_time;

    UWORD32 u4_prev_tgt_vop_time;

    /* This variable is set to 1 if we scale max frame rate by a factor of 2.
     For mpeg4 standard, we just have 16bits and we can't accommodate more than 60000 as frame rate.
     So we scale it and work with it */
    WORD32 is_max_frame_rate_scaled;
} time_stamp_t;

typedef struct time_stamp_t *time_stamp_handle;

/*****************************************************************************/
/* Extern function declarations                                              */
/*****************************************************************************/

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
                            UWORD32 u4_tgt_frm_rate);

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
                                    UWORD32 *pu4_frm_not_skipped_for_dts);

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
void ih264e_init_time_stamp(time_stamp_handle time_stamp,
                            UWORD32 max_frm_rate,
                            UWORD32 src_frm_rate);

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
void ih264e_update_time_stamp(time_stamp_handle time_stamp);

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
                                              ITT_FUNC_TYPE_E e_func_type);

/**
*******************************************************************************
*
* @brief
*  Function to initialize time stamp memtabs
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
                                              ITT_FUNC_TYPE_E e_func_type);

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
WORD32 ih264e_frame_time_get_src_frame_rate(frame_time_t *ps_frame_time);

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
WORD32 ih264e_frame_time_get_tgt_frame_rate(frame_time_t *ps_frame_time);

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
WORD32 ih264e_frame_time_get_src_ticks(frame_time_t *ps_frame_time);

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
WORD32 ih264e_frame_time_get_tgt_ticks(frame_time_t *ps_frame_time);

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
WORD32 ih264e_frame_time_get_src_time(frame_time_t *frame_time);

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
WORD32 ih264e_frame_time_get_tgt_time(frame_time_t *frame_time);

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
void ih264e_frame_time_update_src_frame_rate(frame_time_t *ps_frame_time, WORD32 src_frm_rate);

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
void ih264e_frame_time_update_tgt_frame_rate(frame_time_t *ps_frame_time, WORD32 tgt_frm_rate);

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
void ih264_time_stamp_update_frame_rate(time_stamp_t *ps_time_stamp, UWORD32 src_frm_rate);

#endif /*IH264E_TIME_STAMP_H_*/

