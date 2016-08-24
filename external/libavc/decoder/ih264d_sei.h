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
/*                                                                           */
/*  File Name         : ih264d_sei.h                                                */
/*                                                                           */
/*  Description       : This file contains routines to parse SEI NAL's       */
/*                                                                           */
/*  List of Functions : <List the functions defined in this file>            */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         25 05 2005   NS              Draft                                */
/*                                                                           */
/*****************************************************************************/

#ifndef _IH264D_SEI_H_
#define _IH264D_SEI_H_

#include "ih264_typedefs.h"
#include "ih264_macros.h"
#include "ih264_platform_macros.h"
#include "ih264d_bitstrm.h"
#include "ih264d_structs.h"

#define SEI_BUF_PERIOD      0
#define SEI_PIC_TIMING      1
#define SEI_PAN_SCAN_RECT   2
#define SEI_FILLER          3
#define SEI_UD_REG_T35      4
#define SEI_UD_UN_REG       5
#define SEI_RECOVERY_PT     6
#define SEI_DEC_REF_MARK    7
#define SEI_SPARE_PIC       8
#define SEI_SCENE_INFO      9
#define SEI_SUB_SEQN_INFO   10
#define SEI_SUB_SEQN_LAY_CHAR       11
#define SEI_SUB_SEQN_CHAR   12
#define SEI_FULL_FRAME_FREEZE       13
#define SEI_FULL_FRAME_FREEZE_REL   14
#define SEI_FULL_FRAME_SNAP_SHOT    15
#define SEI_PROG_REF_SEGMENT_START  16
#define SEI_PROG_REF_SEGMENT_END    17
#define SEI_MOT_CON_SLICE_GRP_SET   18
/* Declaration of dec_struct_t to avoid CCS compilation Error */
struct _DecStruct;
WORD32 ih264d_parse_sei_message(struct _DecStruct *ps_dec,
                                dec_bit_stream_t *ps_bitstrm);
typedef struct
{
    UWORD8 u1_seq_parameter_set_id;
    UWORD32 u4_initial_cpb_removal_delay;
    UWORD32 u4_nitial_cpb_removal_delay_offset;

} buf_period_t;

struct _sei
{
    UWORD8 u1_seq_param_set_id;
    buf_period_t s_buf_period;
    UWORD8 u1_pic_struct;
    UWORD16 u2_recovery_frame_cnt;
    UWORD8 u1_exact_match_flag;
    UWORD8 u1_broken_link_flag;
    UWORD8 u1_changing_slice_grp_idc;
    UWORD8 u1_is_valid;
};
typedef struct _sei sei;
#endif /* _IH264D_SEI_H_ */

