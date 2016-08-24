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

#ifndef RC_RD_MODEL_STRUCT
#define RC_RD_MODEL_STRUCT

/*Enable or diable QUAD model*/
#define ENABLE_QUAD_RC_MODEL       0
#define ENABLE_LIN_MODEL_WITH_INTERCEPT  0

/* Number of elements for QP */
#define MPEG2_QP_ELEM       (MAX_MPEG2_QP + 1)


#if ENABLE_QUAD_RC_MODEL
#define QUAD                       1
#define MIN_FRAMES_FOR_QUAD_MODEL  5
#endif

#define MAX_ACTIVE_FRAMES          16
#define MIN_FRAMES_FOR_LIN_MODEL   3
#define INVALID_FRAME_INDEX        255

#define UP_THR_SM           1  /* (1  /pow(2,4) = 0.0625   */
#define UP_THR_E            4

#define LO_THR_SM           368  /* (368.64 / pow(2,14)) = 0.0225 */
#define LO_THR_E            14

#define LIN_DEV_THR_SM     1  /* (1 / pow(1,2)) = .25*/
#define LIN_DEV_THR_E      2

#define PREV_FRAME_MODEL    2

/* Q Factors used for fixed point calculation */
#define Q_FORMAT_GAMMA  8
#define Q_FORMAT_ETA    8

typedef struct rc_rd_model_t
{
    UWORD8 u1_curr_frm_counter;
    UWORD8 u1_num_frms_in_model;
    UWORD8 u1_max_frms_to_model;
    UWORD8 u1_model_used;

    UWORD32 pi4_res_bits[MAX_FRAMES_MODELLED];
    UWORD32 pi4_sad[MAX_FRAMES_MODELLED];

    UWORD8 pu1_num_skips[MAX_FRAMES_MODELLED];
    UWORD8 pu1_avg_qp[MAX_FRAMES_MODELLED];
    UWORD8 au1_num_frames[MPEG2_QP_ELEM];

    model_coeff model_coeff_a_lin_wo_int;
    model_coeff model_coeff_b_lin_wo_int;
    model_coeff model_coeff_c_lin_wo_int;
} rc_rd_model_t;

#endif /* RC_RD_MODEL_STRUCT */
