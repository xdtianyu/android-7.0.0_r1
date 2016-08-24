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

#ifndef _VBR_STR_PRMS_H_
#define _VBR_STR_PRMS_H_

typedef struct
{
    UWORD32 u4_num_pics_in_delay_prd[MAX_PIC_TYPE];
    UWORD32 u4_pic_num;
    UWORD32 u4_intra_prd_pos_in_tgt_ticks;
    UWORD32 u4_cur_pos_in_src_ticks;
    UWORD32 u4_intra_frame_int;
    UWORD32 u4_src_ticks;
    UWORD32 u4_tgt_ticks;
    UWORD32 u4_frms_in_delay_prd;
} vbr_str_prms_t;

void irc_init_vbv_str_prms(vbr_str_prms_t *p_vbr_str_prms,
                           UWORD32 u4_intra_frm_interval,
                           UWORD32 u4_src_ticks,
                           UWORD32 u4_tgt_ticks,
                           UWORD32 u4_frms_in_delay_period);

WORD32 irc_get_vsp_num_pics_in_dly_prd(vbr_str_prms_t *p_vbr_str_prms,
                                       UWORD32 *pu4_num_pics_in_delay_prd);

void irc_get_vsp_src_tgt_ticks(vbr_str_prms_t *p_vbr_str_prms,
                               UWORD32 *pu4_src_ticks,
                               UWORD32 *pu4_tgt_ticks);

void irc_update_vbr_str_prms(vbr_str_prms_t *p_vbr_str_prms,
                             picture_type_e e_pic_type);

void irc_change_vsp_ifi(vbr_str_prms_t *p_vbr_str_prms,
                        UWORD32 u4_intra_frame_int);

void irc_change_vsp_tgt_ticks(vbr_str_prms_t *p_vbr_str_prms,
                              UWORD32 u4_tgt_ticks);

void irc_change_vsp_src_ticks(vbr_str_prms_t *p_vbr_str_prms,
                              UWORD32 u4_src_ticks);

void irc_change_vsp_fidp(vbr_str_prms_t *p_vbr_str_prms,
                         UWORD32 u4_frms_in_delay_period);

#endif

