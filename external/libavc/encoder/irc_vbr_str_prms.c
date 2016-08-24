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
/* Includes */
/*****************************************************************************/

/* System include files */
#include <stdio.h>

/* User include files */
#include "irc_datatypes.h"
#include "irc_cntrl_param.h"
#include "irc_vbr_str_prms.h"

/******************************************************************************
 Function Name   : irc_init_vbv_str_prms
 Description     : Initializes and calculates the number of I frame and P frames
                   in the delay period
 Return Values   : void
 *****************************************************************************/
void irc_init_vbv_str_prms(vbr_str_prms_t *p_vbr_str_prms,
                           UWORD32 u4_intra_frm_interval,
                           UWORD32 u4_src_ticks,
                           UWORD32 u4_tgt_ticks,
                           UWORD32 u4_frms_in_delay_period)
{

    UWORD32 i4_num_i_frms_in_delay_per, i4_num_p_frms_in_delay_per;

    p_vbr_str_prms->u4_frms_in_delay_prd = u4_frms_in_delay_period;
    p_vbr_str_prms->u4_src_ticks = u4_src_ticks;
    p_vbr_str_prms->u4_tgt_ticks = u4_tgt_ticks;
    p_vbr_str_prms->u4_intra_frame_int = u4_intra_frm_interval;

    /*
     * Finding the number of I frames and P frames in delay period. This
     * value along with the drain rates for the corresponding picture types will
     * be used to calculate the buffer sizes
     */
    i4_num_i_frms_in_delay_per = ((u4_frms_in_delay_period * u4_src_ticks)
                    / (u4_intra_frm_interval * u4_tgt_ticks));

    /* Ceiling the above result*/
    if((i4_num_i_frms_in_delay_per * u4_intra_frm_interval * u4_tgt_ticks)
                    < (u4_frms_in_delay_period * u4_src_ticks))
    {
        i4_num_i_frms_in_delay_per++;

    }
    i4_num_p_frms_in_delay_per = u4_frms_in_delay_period
                    - i4_num_i_frms_in_delay_per;

    p_vbr_str_prms->u4_num_pics_in_delay_prd[I_PIC] =
                    i4_num_i_frms_in_delay_per;
    p_vbr_str_prms->u4_num_pics_in_delay_prd[P_PIC] =
                    i4_num_p_frms_in_delay_per;
    p_vbr_str_prms->u4_intra_prd_pos_in_tgt_ticks = (u4_intra_frm_interval
                    * (p_vbr_str_prms->u4_num_pics_in_delay_prd[I_PIC]))
                    * u4_tgt_ticks;
    p_vbr_str_prms->u4_pic_num = 0;
    p_vbr_str_prms->u4_cur_pos_in_src_ticks = 0;
}

WORD32 irc_get_vsp_num_pics_in_dly_prd(vbr_str_prms_t *p_vbr_str_prms,
                                       UWORD32 *pu4_num_pics_in_delay_prd)
{
    pu4_num_pics_in_delay_prd[I_PIC] =
                    p_vbr_str_prms->u4_num_pics_in_delay_prd[I_PIC];
    pu4_num_pics_in_delay_prd[P_PIC] =
                    p_vbr_str_prms->u4_num_pics_in_delay_prd[P_PIC];
    return (p_vbr_str_prms->u4_frms_in_delay_prd);
}

/******************************************************************************
 Function Name   : irc_update_vbr_str_prms
 Description     : update the number of I frames and P/B frames in the delay period
                   for buffer size calculations
 *****************************************************************************/
void irc_update_vbr_str_prms(vbr_str_prms_t *p_vbr_str_prms,
                             picture_type_e e_pic_type)
{
    /*
     * Updating the number of I frames and P frames after encoding every
     * picture. These values along with the drain rates for the corresponding
     * picture  types will be used to calculate the CBR buffer size every frame
     */

    if(e_pic_type == I_PIC)
    {
        p_vbr_str_prms->u4_num_pics_in_delay_prd[I_PIC]--;
    }
    else
    {
        p_vbr_str_prms->u4_num_pics_in_delay_prd[P_PIC]--;
    }

    /* If the next I frame falls within the delay period, we need to increment
     * the number of I frames in the period, else increment the number of P
     * frames
     */
    if((p_vbr_str_prms->u4_cur_pos_in_src_ticks
                    + (p_vbr_str_prms->u4_frms_in_delay_prd
                                    * p_vbr_str_prms->u4_src_ticks))
                    >= p_vbr_str_prms->u4_intra_prd_pos_in_tgt_ticks)
    {
        p_vbr_str_prms->u4_intra_prd_pos_in_tgt_ticks -=
                        p_vbr_str_prms->u4_cur_pos_in_src_ticks;
        p_vbr_str_prms->u4_intra_prd_pos_in_tgt_ticks +=
                        p_vbr_str_prms->u4_intra_frame_int
                                        * p_vbr_str_prms->u4_tgt_ticks;
        p_vbr_str_prms->u4_num_pics_in_delay_prd[I_PIC]++;
        p_vbr_str_prms->u4_pic_num = 0;
        p_vbr_str_prms->u4_cur_pos_in_src_ticks = 0;
    }
    else
    {
        p_vbr_str_prms->u4_num_pics_in_delay_prd[P_PIC]++;
    }
    p_vbr_str_prms->u4_pic_num++;
    p_vbr_str_prms->u4_cur_pos_in_src_ticks += p_vbr_str_prms->u4_src_ticks;
}

void irc_get_vsp_src_tgt_ticks(vbr_str_prms_t *p_vbr_str_prms,
                               UWORD32 *pu4_src_ticks,
                               UWORD32 *pu4_tgt_ticks)
{
    pu4_src_ticks[0] = p_vbr_str_prms->u4_src_ticks;
    pu4_tgt_ticks[0] = p_vbr_str_prms->u4_tgt_ticks;
}

/*******************************************************************************
 Function Name   : change_vbr_str_prms
 Description     : Takes in changes of Intra frame interval, source and target
                   ticks and recalculates the position of the  next I frame
 ******************************************************************************/
void irc_change_vsp_ifi(vbr_str_prms_t *p_vbr_str_prms,
                        UWORD32 u4_intra_frame_int)
{
    irc_init_vbv_str_prms(p_vbr_str_prms, u4_intra_frame_int,
                          p_vbr_str_prms->u4_src_ticks,
                          p_vbr_str_prms->u4_tgt_ticks,
                          p_vbr_str_prms->u4_frms_in_delay_prd);
}

void irc_change_vsp_tgt_ticks(vbr_str_prms_t *p_vbr_str_prms,
                              UWORD32 u4_tgt_ticks)
{
    UWORD32 u4_rem_intra_per_scaled;
    UWORD32 u4_prev_tgt_ticks = p_vbr_str_prms->u4_tgt_ticks;

    /*
     * If the target frame rate is changed, recalculate the position of the next
     * I frame based on the new target frame rate
     * LIMITATIONS :
     * Currently no support is available for dynamic change in source frame rate
     */

    u4_rem_intra_per_scaled = ((p_vbr_str_prms->u4_intra_prd_pos_in_tgt_ticks
                    - p_vbr_str_prms->u4_cur_pos_in_src_ticks)
                    / u4_prev_tgt_ticks) * u4_tgt_ticks;

    p_vbr_str_prms->u4_intra_prd_pos_in_tgt_ticks = u4_rem_intra_per_scaled
                    + p_vbr_str_prms->u4_cur_pos_in_src_ticks;

}

void irc_change_vsp_src_ticks(vbr_str_prms_t *p_vbr_str_prms,
                              UWORD32 u4_src_ticks)
{
    irc_init_vbv_str_prms(p_vbr_str_prms, p_vbr_str_prms->u4_intra_frame_int,
                          u4_src_ticks, p_vbr_str_prms->u4_tgt_ticks,
                          p_vbr_str_prms->u4_frms_in_delay_prd);
}

void irc_change_vsp_fidp(vbr_str_prms_t *p_vbr_str_prms,
                         UWORD32 u4_frms_in_delay_period)
{
    irc_init_vbv_str_prms(p_vbr_str_prms, p_vbr_str_prms->u4_intra_frame_int,
                          p_vbr_str_prms->u4_src_ticks,
                          p_vbr_str_prms->u4_tgt_ticks,
                          u4_frms_in_delay_period);
}
