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
******************************************************************************
* @file
*  ih264e_debug.h
*
* @brief
*  This file contains extern declarations of routines that could be helpful
*  for debugging purposes.
*
* @author
*  ittiam
*
* @remarks
*  none
******************************************************************************
*/

#ifndef IH264E_DEBUG_H_
#define IH264E_DEBUG_H_

#if DEBUG_RC

#define DEBUG_DUMP_QP(pic_cnt, qp, num_cores) \
    ih264e_debug_dump_qp(pic_cnt, qp, num_cores);

#define DEBUG_DUMP_RC(ps_rc) ih264e_debug_print_rc(ps_rc);

#define DEBUG_DUMP_COST_SAD_PU(ps_proc) ih264e_debug_dump_cost_sad_pu(ps_proc);

#define DEBUG_DUMP_INP_TO_RC_POST_ENC(ps_frame_info, pic_cnt, num_cores) \
                ih264e_debug_dump_inp_to_post_enc(ps_frame_info, pic_cnt, num_cores);

#else

#define DEBUG_DUMP_QP(pic_cnt, qp, num_cores) (void);

#define DEBUG_DUMP_RC(ps_rc) (void);

#define DEBUG_DUMP_COST_SAD_PU(ps_proc) (void);

#define DEBUG_DUMP_INP_TO_RC_POST_ENC(ps_frame_info, pic_cnt, num_cores) (void);

#endif

#endif /* IH264E_DEBUG_H_ */
