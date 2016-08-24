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
*  ih264e_rate_control.h
*
* @brief
*  This file contains function declarations of api functions for h264 rate
*  control
*
* @author
*  ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_RATE_CONTROL_H_
#define IH264E_RATE_CONTROL_H_

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief
*  This function initializes rate control context and variables
*
* @par Description
*  This function initializes rate control type, source and target frame rate,
*  average and peak bitrate, intra-inter frame interval and initial
*  quantization parameter
*
* @param[in] pv_rc_api
*  Handle to rate control api
*
* @param[in] pv_frame_time
*  Handle to frame time context
*
* @param[in] pv_time_stamp
*  Handle to time stamp context
*
* @param[in] pv_pd_frm_rate
*  Handle to pull down frame time context
*
* @param[in] u4_max_frm_rate
*  Maximum frame rate
*
* @param[in] u4_src_frm_rate
*  Source frame rate
*
* @param[in] u4_tgt_frm_rate
*  Target frame rate
*
* @param[in] e_rate_control_type
*  Rate control type
*
* @param[in] u4_avg_bit_rate
*  Average bit rate
*
* @param[in] u4_peak_bit_rate
*  Peak bit rate
*
* @param[in] u4_max_delay
*  Maximum delay between frames
*
* @param[in] u4_intra_frame_interval
*  Intra frame interval
*
* @param[in] i4_inter_frm_int
*  Inter frame interval
*
* @param[in] pu1_init_qp
*  Initial qp
*
* @param[in] i4_max_inter_frm_int
*  Maximum inter frame interval
*
* @param[in] pu1_min_max_qp
*  Array of min/max qp
*
* @param[in] u1_profile_level
*  Encoder profile level
*
* @returns none
*
* @remarks
*
*******************************************************************************
*/
void ih264e_rc_init(void *pv_rc_api,
                    void *pv_frame_time,
                    void *pv_time_stamp,
                    void *pv_pd_frm_rate,
                    UWORD32 u4_max_frm_rate,
                    UWORD32 u4_src_frm_rate,
                    UWORD32 u4_tgt_frm_rate,
                    rc_type_e e_rate_control_type,
                    UWORD32 u4_avg_bit_rate,
                    UWORD32 u4_peak_bit_rate,
                    UWORD32 u4_max_delay,
                    UWORD32 u4_intra_frame_interval,
                    WORD32  i4_inter_frm_int,
                    UWORD8 *pu1_init_qp,
                    WORD32 i4_max_inter_frm_int,
                    UWORD8 *pu1_min_max_qp,
                    UWORD8 u1_profile_level);

/**
*******************************************************************************
*
* @brief Function to get picture details
*
* @par   Description
*  This function returns the Picture type(I/P/B)
*
* @param[in] pv_rc_api
*  Handle to Rate control api
*
* @returns
*  Picture type
*
* @remarks none
*
*******************************************************************************
*/
picture_type_e ih264e_rc_get_picture_details(void *pv_rc_api,
                                             WORD32 *pi4_pic_id,
                                             WORD32 *pi4_pic_disp_order_no);


/**
*******************************************************************************
*
* @brief  Function to set frame rate inside RC.
*
* @par Description
*  This function is called before encoding the current frame and gets the qp
*  for the current frame from rate control module
*
* @param[in] ps_rate_control_api
*  Handle to rate control api
*
* @param[in] ps_pd_frm_rate
*  Handle to pull down frm rate context
*
* @param[in] ps_time_stamp
*  Handle to time stamp context
*
* @param[in] ps_frame_time
*  Handle to frame time context
*
* @returns
*  Skip or encode the current frame
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_update_rc_framerates(void *ps_rate_control_api,
                         void *ps_pd_frm_rate,
                         void *ps_time_stamp,
                         void *ps_frame_time
                         );

/**
*******************************************************************************
*
* @brief Function to update mb info for rate control context
*
* @par   Description
*  After encoding a mb, information such as mb type, qp used, mb distortion
*  resulted in encoding the block and so on needs to be preserved for modelling
*  RC. This is preserved via this function call.
*
* @param[in] ps_frame_info
*  Handle Frame info context
*
* @param[in] ps_proc
*  Process context
*
* @returns
*
* @remarks
*
*******************************************************************************
*/
void ih264e_update_rc_mb_info(frame_info_t *ps_frame_info, void *pv_proc);

/**
*******************************************************************************
*
* @brief Function to get rate control buffer status
*
* @par Description
*  This function is used to get buffer status(underflow/overflow) by rate
*  control module
*
* @param[in] pv_rc_api
*  Handle to rate control api context
*
* @param[in] i4_total_frame_bits
*  Total frame bits
*
* @param[in] u1_pic_type
*  Picture type
*
* @param[in] pi4_num_bits_to_prevent_vbv_underflow
*  Number of bits to prevent underflow
*
* @param[out] pu1_is_enc_buf_overflow
*  Buffer overflow indication flag
*
* @param[out] pu1_is_enc_buf_underflow
*  Buffer underflow indication flag
*
* @returns
*
* @remarks
*
*******************************************************************************
*/
void ih264e_rc_get_buffer_status(void *pv_rc_api,
                                 WORD32 i4_total_frame_bits,
                                 picture_type_e e_pic_type,
                                 WORD32 *pi4_num_bits_to_prevent_vbv_underflow,
                                 UWORD8 *pu1_is_enc_buf_overflow,
                                 UWORD8 *pu1_is_enc_buf_underflow);

/**
*******************************************************************************
*
* @brief Function to update rate control module after encoding
*
* @par Description
*  This function is used to update the rate control module after the current
*  frame encoding is done with details such as bits consumed, SAD for I/P/B,
*  intra cost ,mb type and other
*
* @param[in] ps_rate_control_api
*  Handle to rate control api context
*
* @param[in] ps_frame_info
*  Handle to frame info context
*
* @param[in] ps_pd_frm_rate
*  Handle to pull down frame rate context
*
* @param[in] ps_time_stamp
*  Handle to time stamp context
*
* @param[in] ps_frame_time
*  Handle to frame time context
*
* @param[in] i4_total_mb_in_frame
*  Total mb in frame
*
* @param[in] pe_vop_coding_type
*  Picture coding type
*
* @param[in] i4_is_first_frame
*  Is first frame
*
* @param[in] pi4_is_post_encode_skip
*  Post encoding skip flag
*
* @param[in] u1_frame_qp
*  Frame qp
*
* @param[in] pi4_num_intra_in_prev_frame
*  Number of intra mbs in previous frame
*
* @param[in] pi4_avg_activity
*  Average activity
*
* @returns
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_rc_post_enc(void *ps_rate_control_api,
                         frame_info_t *ps_frame_info,
                         void *ps_pd_frm_rate,
                         void *ps_time_stamp,
                         void *ps_frame_time,
                         WORD32 i4_total_mb_in_frame,
                         picture_type_e *pe_vop_coding_type,
                         WORD32 i4_is_first_frame,
                         WORD32 *pi4_is_post_encode_skip,
                         UWORD8 u1_frame_qp,
                         WORD32 *pi4_num_intra_in_prev_frame,
                         WORD32 *pi4_avg_activity);

/**
*******************************************************************************
*
* @brief Function to update bits consumed info to rate control context
*
* @par Description
*  Function to update bits consume info to rate control context
*
* @param[in] ps_frame_info
*  Frame info context
*
* @param[in] ps_entropy
*  Entropy context
*
* @returns
*  total bits consumed by the frame
*
* @remarks
*
*******************************************************************************
*/
void ih264e_update_rc_bits_info(frame_info_t *ps_frame_info, void *pv_entropy);

#endif /* IH264E_RATE_CONTROL_H */

