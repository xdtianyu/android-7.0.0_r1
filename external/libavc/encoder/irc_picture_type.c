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
/* File Includes                                                             */
/*****************************************************************************/

/* System include files */
#include "stdio.h"
#include "string.h"

/* User include files */
#include "irc_datatypes.h"
#include "irc_cntrl_param.h"
#include "irc_mem_req_and_acq.h"
#include "irc_picture_type.h"
#include "irc_trace_support.h"

#define MAX_INTER_FRM_INT   10

/******************************Pic_details ************************************/
typedef struct
{
    /* The id sent by the codec */
    WORD32 i4_pic_id;

    /* The pics come in, in this order  */
    WORD32 i4_pic_disp_order_no;

    /* I,P,B */
    picture_type_e e_pic_type;

} pic_details_t;

/**************************Pic_handling structure *****************************/
typedef struct pic_handling_t
{
    /***************************************************************************
     * Inputs from the codec
     **************************************************************************/

    /* Number of frames after which an I frame will repeat in display order */
    WORD32 i4_intra_frm_int;

    /* (num_b_pics_in_subgop + 1) */
    WORD32 i4_inter_frm_int;

    /* After these many buffered frames, the pics are encoded */
    WORD32 i4_max_inter_frm_int;

    /* OPEN or CLOSED */
    WORD32 i4_is_gop_closed;

    /* The pic stack */
    /* Stack used to store the input pics in encode order */
    pic_details_t as_pic_stack[MAX_INTER_FRM_INT + 2];

    /***************************************************************************
     * Counters
     **************************************************************************/

    /* Decides whether a B or ref pic */
    WORD32 i4_buf_pic_no;

    /* Current pic's number in displayed, and gets reset after an I-frm */
    WORD32 i4_pic_disp_order_no;

    /* Number of P frms that have come, in the current gop, so far */
    WORD32 i4_p_count_in_gop;

    /* Number of B frms that have come, in the current gop, so far */
    WORD32 i4_b_count_in_gop;

    /* Number of B frms that have come, in the current subgop, so far */
    WORD32 i4_b_count_in_subgop;

    /***************************************************************************
     * Indices to the pic stack (Since we store the pics in the encode order,
     * these vars are modified to meet that)
     **************************************************************************/

    /* B_PIC index */
    WORD32 i4_b_pic_idx;

    /* I,P PIC index */
    WORD32 i4_ref_pic_idx;

    /***************************************************************************
     * Variables operating on the input pics
     **************************************************************************/

    /* Flag denoting whether it's the first gop or not */
    WORD32 i4_is_first_gop;

    /* Number of B_PICs in an incomplete subgop */
    WORD32 i4_b_in_incomp_subgop;

    /* In CLOSED_GOPs, even if inter_frm_int > 1, there can be 2 continous
     * P_PICs at the GOP end. This takes values of 0 or 1 */
    WORD32 i4_extra_p;

    /***************************************************************************
     * Arrays storing the number of frms in the gop
     **************************************************************************/

    /* In the steady state, what's the pic distribution in display order */
    WORD32 i4_frms_in_gop[MAX_PIC_TYPE];

    /*
     * In case of a change in inter frm int call, the pic distribution in
     * that gop in display order
     */
    WORD32 i4_frms_in_cur_gop[MAX_PIC_TYPE];

    /*
     * This is used to denote the number of frms remaining to be encoded in the
     * current gop
     */
    WORD32 i4_rem_frms_in_gop[MAX_PIC_TYPE];

    /***************************************************************************
     * Variables operating on the output pics
     **************************************************************************/

    /* Counts the frms encoded in a gop */
    WORD32 i4_coded_pic_no;

    /* Counts from the start of stack to the end repeatedly */
    WORD32 i4_stack_count;

    /***************************************************************************
     * Tracking a change in the inputs from the codec
     **************************************************************************/

    /* A flag that is set when the codec calls for a change in inter_frm_int */
    WORD32 i4_change_in_inter_frm_int;

    /*
     * When a change_in_inter_frm_int is called, this stores the new
     * inter_frm_int
     */
    WORD32 i4_new_inter_frm_int;

    /*
     * When a change_in_inter_frm_int is called in the middle of a gop,this
     * stores the B_PICs in the incomplete subgop of the mixed gop
     */
    WORD32 i4_b_in_incomp_subgop_mix_gop;

    /*
     * For a CLOSED GOP, when a change_in_inter_frm_int is called in the middle
     * of a gop,this is a flag denoting if there is an extra P_PIC in the mixed
     * gop
     */
    WORD32 i4_extra_p_mix_gop;

    /* A flag that is set when the codec calls for a change in intra_frm_int */
    WORD32 i4_change_in_intra_frm_int;

    /*
     * When a change_in_intra_frm_int is called, this stores the new
     * intra_frm_int
     */
    WORD32 i4_new_intra_frm_int;

    /***************************************************************************
     * Previous pic_stack_indices & details
     **************************************************************************/
    pic_details_t s_prev_pic_details;

    WORD32 i4_prev_b_pic_idx;

    WORD32 i4_last_frm_in_gop;

    WORD32 i4_first_gop_encoded;

    /* NITT TBR */
    picture_type_e e_previous_pic_type;

    WORD32 i4_force_I_frame;

    WORD32 i4_forced_I_frame_cur_frame;

    WORD32 i4_sum_remaining_frm_in_gop;

    WORD32 i4_mod_temp_ref_cnt;

    WORD32 i4_frames_in_fif_gop;

    WORD32 i4_prev_intra_frame_interval;

} pic_handling_t;

static void irc_update_pic_distbn(pic_handling_t *ps_pic_handling,
                                  WORD32 i4_intra_frm_int,
                                  WORD32 i4_inter_frm_int,
                                  WORD32 i4_gop_boundary);

static void find_pic_distbn_in_gop(WORD32 i4_frms_in_gop[MAX_PIC_TYPE],
                                   WORD32 i4_intra_frm_int,
                                   WORD32 i4_inter_frm_int,
                                   WORD32 i4_is_gop_closed,
                                   WORD32 *pi4_b_in_incomp_subgop,
                                   WORD32 *pi4_extra_p);

WORD32 irc_pic_handling_num_fill_use_free_memtab(pic_handling_t **pps_pic_handling,
                                                 itt_memtab_t *ps_memtab,
                                                 ITT_FUNC_TYPE_E e_func_type)
{
    WORD32 i4_mem_tab_idx = 0;
    pic_handling_t s_pic_handling_temp;

    /*
     * Hack for al alloc, during which we dont have any state memory.
     * Dereferencing can cause issues
     */
    if(e_func_type == GET_NUM_MEMTAB || e_func_type == FILL_MEMTAB)
    {
        (*pps_pic_handling) = &s_pic_handling_temp;
    }

    /*for src rate control state structure*/
    if(e_func_type != GET_NUM_MEMTAB)
    {
        fill_memtab(&ps_memtab[i4_mem_tab_idx], sizeof(pic_handling_t),
                    ALIGN_128_BYTE, PERSISTENT, DDR);
        use_or_fill_base(&ps_memtab[0], (void**)pps_pic_handling, e_func_type);
    }
    i4_mem_tab_idx++;

    return (i4_mem_tab_idx);
}

/******************************************************************************
 Description     : initializes the pic handling state struct
 *****************************************************************************/
void irc_init_pic_handling(pic_handling_t *ps_pic_handling,
                           WORD32 i4_intra_frm_int,
                           WORD32 i4_inter_frm_int,
                           WORD32 i4_max_inter_frm_int,
                           WORD32 i4_is_gop_closed)
{
    /* Declarations */
    WORD32 i;

    /* Checks */
    /* Codec Parameters */
    ps_pic_handling->i4_intra_frm_int = i4_intra_frm_int;
    ps_pic_handling->i4_inter_frm_int = i4_inter_frm_int;
    ps_pic_handling->i4_max_inter_frm_int = i4_max_inter_frm_int;
    ps_pic_handling->i4_is_gop_closed = i4_is_gop_closed;

    /* Pic_stack */
    memset(ps_pic_handling->as_pic_stack, 0,
           sizeof(ps_pic_handling->as_pic_stack));
    memset(&ps_pic_handling->s_prev_pic_details, 0,
           sizeof(ps_pic_handling->s_prev_pic_details));

    /* Counters */
    ps_pic_handling->i4_buf_pic_no = 0;
    ps_pic_handling->i4_pic_disp_order_no = 0;

    /* Indices to the pic_stack */
    ps_pic_handling->i4_ref_pic_idx = 0;
    /*
     * B frame index should be ref_frame_num,
     * which is 2 in out case
     */
    ps_pic_handling->i4_b_pic_idx = 2;
    ps_pic_handling->i4_prev_b_pic_idx = 2;

    /* Variables working on the input frames */
    ps_pic_handling->i4_is_first_gop = 1;
    ps_pic_handling->i4_p_count_in_gop = 0;
    ps_pic_handling->i4_b_count_in_gop = 0;
    ps_pic_handling->i4_b_count_in_subgop = 0;

    /* Variables working on the output frames */
    ps_pic_handling->i4_coded_pic_no = -1;
    ps_pic_handling->i4_stack_count = -1;

    /* Tracks the changes in the Codec Parameters */
    ps_pic_handling->i4_change_in_inter_frm_int = 0;
    ps_pic_handling->i4_new_inter_frm_int = i4_max_inter_frm_int;

    /* Tracks the changes in the Codec Parameters */
    ps_pic_handling->i4_change_in_intra_frm_int = 0;
    ps_pic_handling->i4_new_intra_frm_int = i4_intra_frm_int;

    /* Variables on which the bit allocation is dependent  */
    /* Get the pic distribution in the gop */
    find_pic_distbn_in_gop(ps_pic_handling->i4_frms_in_gop, i4_intra_frm_int,
                           i4_inter_frm_int, i4_is_gop_closed,
                           &ps_pic_handling->i4_b_in_incomp_subgop,
                           &ps_pic_handling->i4_extra_p);

    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        ps_pic_handling->i4_frms_in_cur_gop[i] =
                        ps_pic_handling->i4_frms_in_gop[i];
        ps_pic_handling->i4_rem_frms_in_gop[i] =
                        ps_pic_handling->i4_frms_in_gop[i];
    }

    ps_pic_handling->e_previous_pic_type = I_PIC;
    ps_pic_handling->i4_prev_intra_frame_interval = i4_intra_frm_int;
    ps_pic_handling->i4_force_I_frame = 0;
    ps_pic_handling->i4_forced_I_frame_cur_frame = 0;
    ps_pic_handling->i4_sum_remaining_frm_in_gop = 0;
    ps_pic_handling->i4_mod_temp_ref_cnt = 0;

    ps_pic_handling->i4_b_in_incomp_subgop_mix_gop =
                    ps_pic_handling->i4_b_in_incomp_subgop;
    ps_pic_handling->i4_extra_p_mix_gop = ps_pic_handling->i4_extra_p;

    ps_pic_handling->i4_last_frm_in_gop = 0;
    ps_pic_handling->i4_first_gop_encoded = 0;
    ps_pic_handling->i4_frames_in_fif_gop = 0;

}

/*******************************************************************************
 * @brief registers the new intra frame interval value
 ******************************************************************************/
void irc_pic_handling_register_new_int_frm_interval(pic_handling_t *ps_pic_handling,
                                                    WORD32 i4_intra_frm_int)
{
    ps_pic_handling->i4_change_in_intra_frm_int = 1;
    ps_pic_handling->i4_new_intra_frm_int = i4_intra_frm_int;
}

void irc_pic_handling_register_new_inter_frm_interval(pic_handling_t *ps_pic_handling,
                                                      WORD32 i4_inter_frm_int)
{
    /* Update the state structure with the latest values */
    ps_pic_handling->i4_change_in_inter_frm_int = 1;
    ps_pic_handling->i4_new_inter_frm_int = i4_inter_frm_int;
}

static void start_new_gop(pic_handling_t *ps_pic_handling)
{
    WORD32 i;
    WORD32 i4_sum_remaining_frm_in_gop = 0;

    /* Now, the end of gop updates */
    ps_pic_handling->i4_pic_disp_order_no = 0;
    ps_pic_handling->i4_buf_pic_no = 0;
    ps_pic_handling->i4_is_first_gop = 0;
    ps_pic_handling->i4_extra_p_mix_gop = ps_pic_handling->i4_extra_p;

    if(ps_pic_handling->i4_is_gop_closed)
    {
        ps_pic_handling->i4_b_in_incomp_subgop_mix_gop =
                        ps_pic_handling->i4_b_in_incomp_subgop;
    }
    /*
     * Store the number of frames in the gop that is encoded till now
     * just before Force I frame call is made
     */
    ps_pic_handling->i4_frames_in_fif_gop = ps_pic_handling->i4_b_count_in_gop
                    + ps_pic_handling->i4_p_count_in_gop + 1;
    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        i4_sum_remaining_frm_in_gop += ps_pic_handling->i4_rem_frms_in_gop[i];
    }
    ps_pic_handling->i4_sum_remaining_frm_in_gop = i4_sum_remaining_frm_in_gop;
    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        ps_pic_handling->i4_frms_in_cur_gop[i] =
                        ps_pic_handling->i4_frms_in_gop[i];
        ps_pic_handling->i4_rem_frms_in_gop[i] =
                        ps_pic_handling->i4_frms_in_cur_gop[i];
    }
}

/*******************************************************************************
 * @brief Fills the pic_stack with the incoming pics in encode order
 ******************************************************************************/
void irc_add_pic_to_stack(pic_handling_t *ps_pic_handling, WORD32 i4_enc_pic_id)
{
    /* Declarations */
    WORD32 i4_inter_frm_int, i4_max_inter_frm_int,
           i4_intra_frm_int, i4_new_inter_frm_int;
    WORD32 i4_is_gop_closed;
    WORD32 i4_buf_pic_no, i4_pic_disp_order_no;
    WORD32 i4_b_pic_idx, i4_ref_pic_idx;
    WORD32 i4_is_first_gop, i4_b_in_incomp_subgop, i4_p_count_in_gop,
           i4_b_count_in_gop, i4_b_count_in_subgop;
    WORD32 i, i4_p_frms_in_prd, i4_b_frms_in_prd,
           i4_num_b_in_subgop, i4_extra_p;
    WORD32 i4_condn_for_change_in_inter_frm_int;
    picture_type_e e_previous_pic_type, e_cur_pic_type;
    WORD32 i4_force_I_frame;

    /*
     * Initialize the local vars with the state struct values needed by the
     * change calls
     */
    i4_intra_frm_int = ps_pic_handling->i4_intra_frm_int;
    i4_inter_frm_int = ps_pic_handling->i4_inter_frm_int;
    i4_max_inter_frm_int = ps_pic_handling->i4_max_inter_frm_int;
    i4_is_gop_closed = ps_pic_handling->i4_is_gop_closed;

    i4_buf_pic_no = ps_pic_handling->i4_buf_pic_no;
    i4_pic_disp_order_no = ps_pic_handling->i4_pic_disp_order_no;
    i4_b_count_in_gop = ps_pic_handling->i4_b_count_in_gop;
    i4_b_frms_in_prd = ps_pic_handling->i4_frms_in_cur_gop[B_PIC];
    i4_is_first_gop = ps_pic_handling->i4_is_first_gop;
    i4_new_inter_frm_int = ps_pic_handling->i4_new_inter_frm_int;
    e_previous_pic_type = ps_pic_handling->e_previous_pic_type;
    i4_force_I_frame = ps_pic_handling->i4_force_I_frame;

    /*  Force I frame :
     *  Two different cases
     *  1)OPEN_GOP: New GOP is started after number of B pictures in the last
     *              sub gop of a gop to mimic the GOP structure.
     *  2)Closed GOP:Wait till P frame at input and The frame after a P frame
     *               a new GOP is started to mimic the GOP structure.
     */
    if(i4_force_I_frame)
    {
        WORD32 i4_temp_is_gop_closed;
        WORD32 i4_codn = 0;
        /* A special case of Open GOP where the it behaves like Closed GOP*/
        if((i4_intra_frm_int % i4_inter_frm_int) == 1)
        {
            i4_temp_is_gop_closed = 1;
        }
        else
        {
            i4_temp_is_gop_closed = i4_is_gop_closed;
        }
        /* Get the current picture type to aid decision to force an I frame*/
        if((i4_buf_pic_no % i4_inter_frm_int)
           && !(i4_is_gop_closed&& (i4_b_count_in_gop == i4_b_frms_in_prd)))
        {
            e_cur_pic_type = B_PIC;
        }
        else
        {
            if(i4_pic_disp_order_no == 0)
            {
                e_cur_pic_type = I_PIC;
            }
            else
            {
                e_cur_pic_type = P_PIC;
            }
        }
        if((i4_intra_frm_int % i4_inter_frm_int) == 0)
        {
            i4_codn = (e_cur_pic_type == P_PIC);
        }
        else
        {
            i4_codn = (ps_pic_handling->i4_b_count_in_subgop
                            == ps_pic_handling->i4_b_in_incomp_subgop);
        }
        if(e_cur_pic_type == I_PIC)
        {
            /*
             * Don't do anything. Resetting the force I frame flag
             * since the current picture type is already a I frame
             */
            i4_force_I_frame = 0;
        }
        else if(i4_inter_frm_int == 1)
        {
            /*IPP case , Force I frame immediately*/
            start_new_gop(ps_pic_handling);
        }
        else if((!i4_temp_is_gop_closed) && i4_codn)
        {
            start_new_gop(ps_pic_handling);
            if(ps_pic_handling->i4_b_count_in_subgop)
            {
                ps_pic_handling->i4_b_pic_idx += 1;
                ps_pic_handling->i4_b_pic_idx %= (i4_max_inter_frm_int + 1);
            }
        }
        else if(i4_temp_is_gop_closed && (e_previous_pic_type == P_PIC)
                        && (e_cur_pic_type != P_PIC))
        {
            start_new_gop(ps_pic_handling);
            ps_pic_handling->i4_b_pic_idx++;
            ps_pic_handling->i4_b_pic_idx %= (i4_max_inter_frm_int + 1);
        }
        i4_is_first_gop = ps_pic_handling->i4_is_first_gop;
    }


     /***********************CHANGE_INTRA_FRM_INTERVAL**************************
     *
     * Call the irc_update_pic_distbn if
     *      1)Change in intra frm interval flag is set
     *      2)It's the first B_PIC of a gop
     */
    if((ps_pic_handling->i4_change_in_intra_frm_int == 1)
                    && ((i4_pic_disp_order_no == 1)))
    {
        irc_update_pic_distbn(ps_pic_handling,
                              ps_pic_handling->i4_new_intra_frm_int,
                              ps_pic_handling->i4_inter_frm_int, 1);

        ps_pic_handling->i4_change_in_intra_frm_int = 0;

        if(ps_pic_handling->i4_new_intra_frm_int == 1)
        {
            ps_pic_handling->i4_pic_disp_order_no = 0;
        }
    }
    /*********************CHANGE_INTER_FRM_INTERVAL****************************/
    /* Call irc_update_pic_distbn if
     *      1)Change in inter frm interval flag is set
     *      2)It's the first B_PIC after gop/subgop start, and
     *      3)The new inter-frm-interval won't cross the intra_frm_interval
     */
    if((ps_pic_handling->i4_change_in_inter_frm_int == 1)
       && ((i4_buf_pic_no % i4_inter_frm_int == 1)|| (i4_pic_disp_order_no == 1) || (i4_inter_frm_int == 1)))
    {
        /*
         * Condition which checks if the new inter_frm_int will cross the
         * intra_frm_int
         */
        i4_condn_for_change_in_inter_frm_int = ((i4_pic_disp_order_no
                        + i4_new_inter_frm_int - 1) < i4_intra_frm_int);

        if(i4_condn_for_change_in_inter_frm_int)
        {
            /*
             * If there is a change in inter frame interval. We should set the b
             * frame IDX to the (num ref frame - num ref frame in buf)+ i4_ref_pic_idx
             * Since our case we have a structure of I B P or I B...B P only
             * we have three cases
             * 1) current incoming frame is I. Then we have to leave space for
             *    current I and next P hence write b idx as to ref idx + 2
             * 2) Current incoming frame is B. In that case, we have I in buffer.
             *    Only one P needs space hence write b idx as ref idx +1
             * 3) Current incoming frame is P. In that case we are at the end of
             *    gop [sub gop?] and we have to leave space for next gops I and P.
             *    Thus b idx = ref idx + 2
             *
             *  In case of an closed Gop. The last frame has to be forced to be a P.
             *  Hence we may have problems in that case.
             *
             *  Also this has the implicit assumption of only 2 ref frames
             */
            WORD32 i4_is_curr_frm_b =  (i4_buf_pic_no % i4_new_inter_frm_int)&&
                            !(i4_is_gop_closed && (i4_b_count_in_gop == i4_b_frms_in_prd));

            /*If the inter_frm_int = 1, then the b_pic_idx needs to be modified */
            if(i4_inter_frm_int == 1)
            {
                ps_pic_handling->i4_b_pic_idx = ((i4_is_curr_frm_b ? 1 : 2)
                                + ps_pic_handling->i4_ref_pic_idx)
                                % (i4_max_inter_frm_int + 1);
            }

            /*
             * Depending on the gop/subgop boundary, call the change_inter_frm_int
             *
             * TO DO: make a single call, change the name of the fxn to
             * update_state,
             * where state = frms_in_gop + b_incomp_subgop + extra_p
             */

            /* GOP boundary */
            if(i4_pic_disp_order_no == 1)
            {
                irc_update_pic_distbn(ps_pic_handling,
                                      ps_pic_handling->i4_intra_frm_int,
                                      ps_pic_handling->i4_new_inter_frm_int, 1);
            }
            /* Subgop boundary */
            else
            {
                irc_update_pic_distbn(ps_pic_handling,
                                      ps_pic_handling->i4_intra_frm_int,
                                      ps_pic_handling->i4_new_inter_frm_int, 0);
            }

            ps_pic_handling->i4_change_in_inter_frm_int = 0;
            ps_pic_handling->i4_new_inter_frm_int =
                            ps_pic_handling->i4_inter_frm_int;
        }

    }

    /* Initialize the local vars with the state struct values */
    i4_buf_pic_no = ps_pic_handling->i4_buf_pic_no;
    i4_pic_disp_order_no = ps_pic_handling->i4_pic_disp_order_no;
    i4_b_pic_idx = ps_pic_handling->i4_b_pic_idx;
    i4_ref_pic_idx = ps_pic_handling->i4_ref_pic_idx;
    i4_b_in_incomp_subgop = ps_pic_handling->i4_b_in_incomp_subgop_mix_gop;
    i4_p_count_in_gop = ps_pic_handling->i4_p_count_in_gop;
    i4_b_count_in_gop = ps_pic_handling->i4_b_count_in_gop;
    i4_b_count_in_subgop = ps_pic_handling->i4_b_count_in_subgop;
    i4_p_frms_in_prd = ps_pic_handling->i4_frms_in_cur_gop[P_PIC];
    i4_b_frms_in_prd = ps_pic_handling->i4_frms_in_cur_gop[B_PIC];
    i4_extra_p = ps_pic_handling->i4_extra_p_mix_gop;
    i4_inter_frm_int = ps_pic_handling->i4_inter_frm_int;
    i4_intra_frm_int = ps_pic_handling->i4_intra_frm_int;

    /* Initializing the prev_state vars */
    ps_pic_handling->i4_prev_b_pic_idx = ps_pic_handling->i4_b_pic_idx;

    i4_num_b_in_subgop = (i4_inter_frm_int - 1);

    /*********************** Fill the stack ***********************************/
    /* The next part of the code is organized as
     *
     * if(B_PIC conditions satisfied)
     * {
     *  Fill the pic_stack using the b_pic_index
     *  Update the b_pic_index and the other b_pic related vars for the
     *      next B_PIC
     * }
     *  else
     * {
     *  if(I_PIC conditions are satisfied)
     * {
     *  Fill the pic_stack using the ref_pic_index
     *  Update the ref_pic_index and the other ref_pic related vars for the next
     *      I_PIC/P_PIC
     * }
     *  else
     * {
     *  Fill the pic_stack using the ref_pic_index
     *  Update the ref_pic_index and the other ref_pic related vars for the next
     *      I_PIC/P_PIC
     * }
     * }
     */
    /*
     * Condition for a B_PIC -
     * 1) Other than the first I_PIC and the periodically appearing P_PICs, after
     *    every inter_frm_int, rest all pics are B_PICs
     * 2) In case of CLOSED_GOP, the last frame of the gop has to be a P_PIC
     */

    if((i4_buf_pic_no % i4_inter_frm_int)&& !(i4_is_gop_closed
       && (i4_b_count_in_gop == i4_b_frms_in_prd))) /**** B_PIC ****/
    {
        /* Fill the pic_stack */
        ps_pic_handling->as_pic_stack[i4_b_pic_idx].i4_pic_id = i4_enc_pic_id;
        ps_pic_handling->as_pic_stack[i4_b_pic_idx].e_pic_type = B_PIC;
        ps_pic_handling->as_pic_stack[i4_b_pic_idx].i4_pic_disp_order_no =
                        i4_pic_disp_order_no;

        /* Store Pic type*/
        e_previous_pic_type = B_PIC;

        /* Update the prev_pic_details */
        memcpy(&ps_pic_handling->s_prev_pic_details,
               &ps_pic_handling->as_pic_stack[i4_b_pic_idx],
               sizeof(pic_details_t));

        i4_b_count_in_gop++;
        i4_b_count_in_subgop++;

        /* Update the i4_b_pic_idx */
        if(!i4_is_gop_closed)
        {
            /* If this B_PIC features in one of the complete subgops */
            if((i4_b_count_in_subgop < i4_num_b_in_subgop)
                            && !(i4_b_count_in_gop == i4_b_frms_in_prd))
            {
                i4_b_pic_idx++;
            }
            else /* Else if this B_PIC is the last one in a subgop or gop  */
            {
                /*
                 * If this is the last B_PIC of a GOP, depending on the number
                 * of incomp B_pics in the subgop, there can be either only I
                 * or I,P pics between this and the next B_PIC
                 */
                if(i4_b_count_in_gop == i4_b_frms_in_prd)
                {
                    i4_b_pic_idx += (2 + (!i4_b_in_incomp_subgop)); /*Prev*/
                    i4_b_count_in_gop = 0;
                }
                /*
                 * For the last B_PIC of a subgop, there's always a P b/w
                 * this & the next B_PIC
                 */
                else
                {
                    i4_b_pic_idx += 2;
                }
                i4_b_count_in_subgop = 0;
            }
        }
        else
        {
            /* For the last B_PIC of a gop
             * Normally,there will be 3 pics (P,I,P) between this and the next
             * B_PIC for a CLOSED gop, except when
             *  1)Number of P_pics in the gop = 1
             *  2)There is an extra P at the end of the gop
             */
            if(i4_b_count_in_gop == i4_b_frms_in_prd)
            {
                i4_b_pic_idx += (3 + ((i4_b_in_incomp_subgop == 0)
                                 && (i4_p_frms_in_prd> 1)
                                 && (i4_pic_disp_order_no
                                 != (i4_p_frms_in_prd+ i4_b_frms_in_prd- 1))));

                i4_b_count_in_subgop = 0;
            }
            /* For a B_PIC which is not the last one in a subgop */
            else if(i4_b_count_in_subgop < i4_num_b_in_subgop)
            {
                i4_b_pic_idx++;
            }
            else /* For the last B_PIC of a subgop */
            {
                i4_b_pic_idx += 2;
                i4_b_count_in_subgop = 0;
            }
        }
        i4_b_pic_idx %= (i4_max_inter_frm_int + 1);
    }
    /*********** I or P pic *********/
    else
    {
        ps_pic_handling->as_pic_stack[i4_ref_pic_idx].i4_pic_id = i4_enc_pic_id;
        ps_pic_handling->as_pic_stack[i4_ref_pic_idx].i4_pic_disp_order_no =
                        i4_pic_disp_order_no;
        /* Store Pic type*/
        e_previous_pic_type = I_PIC;

        /**** I_PIC ****/
        if(i4_pic_disp_order_no == 0)
        {
            ps_pic_handling->as_pic_stack[i4_ref_pic_idx].e_pic_type = I_PIC;

            /* Update the prev_pic_details */
            memcpy(&ps_pic_handling->s_prev_pic_details,
                   &ps_pic_handling->as_pic_stack[i4_ref_pic_idx],
                   sizeof(pic_details_t));
            /*
             * In case of an I-frame depending on OPEN or CLOSED gop,
             * the ref_pic_idx changes
             */
            if((!i4_is_gop_closed) && (i4_is_first_gop == 0))
            {
                if((i4_p_frms_in_prd <= 1) && (i4_b_in_incomp_subgop == 0))
                {
                    i4_ref_pic_idx++;
                }
                /*
                 * From the 2nd gop onwards, the I and first P frame are
                 * separated by the num_b_in_incomp_subgop
                 */
                else
                {
                    i4_ref_pic_idx += (i4_b_in_incomp_subgop + 1);
                }

                ps_pic_handling->i4_b_in_incomp_subgop_mix_gop =
                                ps_pic_handling->i4_b_in_incomp_subgop;
            }
            else
            {
                i4_ref_pic_idx++;
            }

            i4_b_count_in_gop = 0;
            i4_p_count_in_gop = 0;
            i4_b_count_in_subgop = 0;

        }
        /**** P_PIC ****/
        else
        {
            ps_pic_handling->as_pic_stack[i4_ref_pic_idx].e_pic_type = P_PIC;
            /* Store Pic type*/
            e_previous_pic_type = P_PIC;

            /* Update the prev_pic_details */
            memcpy(&ps_pic_handling->s_prev_pic_details,
                   &ps_pic_handling->as_pic_stack[i4_ref_pic_idx],
                   sizeof(pic_details_t));

            i4_p_count_in_gop++;
            ps_pic_handling->i4_prev_intra_frame_interval = i4_intra_frm_int;

            /*
             * In case of an P-frame depending on OPEN or CLOSED gop, the
             * ref_pic_idx changes
             */
            if(i4_is_gop_closed && (i4_p_count_in_gop == i4_p_frms_in_prd))
            {
                /*
                 * For the last P_PIC in a gop, if extra_p or incomp_b are
                 * present, the number of such pics between this and the next
                 * ref_pic is (i4_b_in_incomp_subgop + 1)
                 */
                if((i4_p_count_in_gop > 1)
                                && (i4_b_in_incomp_subgop || i4_extra_p))
                {
                    i4_ref_pic_idx += (i4_b_in_incomp_subgop + 1);
                }
                else
                {
                    i4_ref_pic_idx += i4_inter_frm_int;
                }
            }
            else
            {
                i4_ref_pic_idx += i4_inter_frm_int;
            }
        }

        i4_ref_pic_idx %= (i4_max_inter_frm_int + 1);
    }

    /* Update those variables working on the input frames  */
    i4_pic_disp_order_no++;
    i4_buf_pic_no++;

#if 0
    /* For any gop */
     /* BUG FIX
      *  This piece of code resets the gop upon I frame(?)
      *  This introduces a problem of GOP getting reset not at I frames as it should be
      *  The reason AFAIK is that
      *  1) This code uses i4_pic_disp_order_no to reset GOP. I assume it computes
      *      if are at GOP boundary and does it, but not sure
      *  2) The frames rmainign in GOP is done in post enc as it should be.
      *
      *  Also ps_pic_handling->i4_pic_disp_order_no is incremented when a pic is added
      *  to stack becuase the additon is in disp order while poping is in encode order
      *
      *  SUppose there is a deay od 1 frame between queue and encode.
      *  then he timing will be. Assume a GOP of IPPIPP
      *
      *      Input buff    Input to qu     Output buf/encode buff      remaining pic in gop
      *    1  I             I                 NA                          rest to 1 2
      *    2  P             P                 I                           0 2
      *    3  P             P                 P                           0 1
      *    4  I             I                 P                           reset to 1 2
      *    5  P             P                 I                           1 1
      *    6  P             P                 P                           1 0
      *    7  NA            NA                P
      *
      *  Hence our gop gets reset at I(1)  and I(4) in the RC.thus the reaming pic in gop
      *  count will be as shown. We can clearly see that the GOP gets reset at I(4) .Hence
      *  for the correpondinng QP for output buf p(4) will be that of an I frame.
      *
      *  By hiding this I hope to fix this problem. But Iam not sure exaclty.
      *  This needs to be investigated further
      *
      *  By hiding this most likely we are in effect disabling the dynanic
      *  update of gop params.
      */

    if(ps_pic_handling->i4_pic_disp_order_no
                    == (i4_max_inter_frm_int - 1- ((!i4_is_gop_closed)
                        * ps_pic_handling->i4_b_in_incomp_subgop_mix_gop)))
    {
        for(i = 0; i < MAX_PIC_TYPE; i++)
        {
            ps_pic_handling->i4_rem_frms_in_gop[i] =
                            ps_pic_handling->i4_frms_in_cur_gop[i];
        }

        if((!i4_is_gop_closed) && (i4_is_first_gop)
                        && (ps_pic_handling->i4_rem_frms_in_gop[B_PIC]
                                        > ps_pic_handling->i4_b_in_incomp_subgop_mix_gop))
        {
            ps_pic_handling->i4_rem_frms_in_gop[B_PIC] =
                            ps_pic_handling->i4_frms_in_cur_gop[B_PIC]
                                            - ps_pic_handling->i4_b_in_incomp_subgop_mix_gop;
        }
    }
#endif

    /* End of GOP updates */
    if(i4_pic_disp_order_no == (i4_p_frms_in_prd + i4_b_frms_in_prd + 1))
    {
        /* Now, the end of gop updates */
        i4_pic_disp_order_no = 0;
        i4_buf_pic_no = 0;
        i4_is_first_gop = 0;
        ps_pic_handling->i4_extra_p_mix_gop = ps_pic_handling->i4_extra_p;

        if(i4_is_gop_closed)
        {
            ps_pic_handling->i4_b_in_incomp_subgop_mix_gop =
                            ps_pic_handling->i4_b_in_incomp_subgop;
        }

        for(i = 0; i < MAX_PIC_TYPE; i++)
        {
            ps_pic_handling->i4_frms_in_cur_gop[i] =
                            ps_pic_handling->i4_frms_in_gop[i];
        }
    }

    /* Updating the vars which work on the encoded pics */
    /* For the first gop */
    if ((ps_pic_handling->i4_is_first_gop)
                    && (ps_pic_handling->i4_pic_disp_order_no == 0))
    {
        ps_pic_handling->i4_coded_pic_no = 0;
        ps_pic_handling->i4_stack_count = 0;
    }

    /* Update the state struct with the modifiable local vars */
    ps_pic_handling->i4_buf_pic_no = i4_buf_pic_no;
    ps_pic_handling->i4_pic_disp_order_no = i4_pic_disp_order_no;
    ps_pic_handling->i4_b_pic_idx = i4_b_pic_idx;
    ps_pic_handling->i4_ref_pic_idx = i4_ref_pic_idx;
    ps_pic_handling->i4_is_first_gop = i4_is_first_gop;
    ps_pic_handling->i4_p_count_in_gop = i4_p_count_in_gop;
    ps_pic_handling->i4_b_count_in_gop = i4_b_count_in_gop;
    ps_pic_handling->i4_b_count_in_subgop = i4_b_count_in_subgop;
    ps_pic_handling->e_previous_pic_type = e_previous_pic_type;
    ps_pic_handling->i4_force_I_frame = i4_force_I_frame;
}

/*******************************************************************************
 * @brief Returns the picture type, ip and display order number for the frame to
 *        be encoded
 ******************************************************************************/
void irc_get_pic_from_stack(pic_handling_t *ps_pic_handling,
                            WORD32 *pi4_pic_id,
                            WORD32 *pi4_pic_disp_order_no,
                            picture_type_e *pe_pic_type)
{
    pic_details_t s_pic_details;
    pic_details_t *ps_pic_details = &s_pic_details;

    if(ps_pic_handling->i4_stack_count < 0)
    {
        ps_pic_details->e_pic_type = BUF_PIC;
        ps_pic_details->i4_pic_disp_order_no = -1;
        ps_pic_details->i4_pic_id = -1;
    }
    else
    {
        memcpy(ps_pic_details,
               &ps_pic_handling->as_pic_stack[ps_pic_handling->i4_stack_count],
               sizeof(pic_details_t));

        /* Force I frame updations */
        if((ps_pic_handling->i4_force_I_frame == 1)
                        && (ps_pic_details->e_pic_type == I_PIC))
        {
            /* Flag to signal change in remaining bits*/
            ps_pic_handling->i4_forced_I_frame_cur_frame = 1;
            ps_pic_handling->i4_force_I_frame = 0;
            /*
             * Indicates count for no. of Pictures whose temporal reference
             * has to be modified
             * in the new GOP
             */
            ps_pic_handling->i4_mod_temp_ref_cnt =
                            ps_pic_handling->i4_b_in_incomp_subgop + 1;
            ps_pic_handling->i4_first_gop_encoded = 1;
        }

        /*
         * In MPEG2, the temporal reference of the first displayed frame in a
         * gop is 0.In case of an OPEN_GOP, the B_PICs of the last subgop in a
         * gop, maybe coded as a part of the next gop. Hence, in such conditions
         * the pic_disp_order needs to be modified so that it gives an
         * indication of the temporal reference
         */
        if((!ps_pic_handling->i4_is_gop_closed)
                        && (ps_pic_handling->i4_first_gop_encoded))
        {
            if(!ps_pic_handling->i4_mod_temp_ref_cnt)
            {
                ps_pic_details->i4_pic_disp_order_no =
                                (ps_pic_handling->as_pic_stack[ps_pic_handling->i4_stack_count].i4_pic_disp_order_no
                                                + ps_pic_handling->i4_b_in_incomp_subgop)
                                                % (ps_pic_handling->i4_prev_intra_frame_interval);

            }
            else
            {
                /*
                 * due to force I frame First frame will have only
                 * ps_pic_handling->i4_frames_in_fif_gop number of frames
                 */
                ps_pic_details->i4_pic_disp_order_no =
                                (ps_pic_handling->as_pic_stack[ps_pic_handling->i4_stack_count].i4_pic_disp_order_no
                                                + ps_pic_handling->i4_b_in_incomp_subgop)
                                                % (ps_pic_handling->i4_frames_in_fif_gop);
                ps_pic_handling->i4_mod_temp_ref_cnt--;
            }
        }
    }

    /* Giving this to the Codec */
    *pi4_pic_id = s_pic_details.i4_pic_id;
    *pi4_pic_disp_order_no = s_pic_details.i4_pic_disp_order_no;
    *pe_pic_type = s_pic_details.e_pic_type;
}

/*******************************************************************************
 * @brief Updates the picture handling state whenever there is changes in input
 *        parameter
 *
 ******************************************************************************/
static void irc_update_pic_distbn(pic_handling_t *ps_pic_handling,
                                  WORD32 i4_intra_frm_int,
                                  WORD32 i4_inter_frm_int,
                                  WORD32 i4_gop_boundary)
{
    /* Declarations */
    WORD32 i4_is_gop_closed;
    WORD32 i, i4_prev_inter_frm_int, i4_max_inter_frm_int, i4_pic_disp_order_no;
    WORD32 i4_b_in_incomp_subgop, i4_extra_p,
           i4_b_in_incomp_subgop_mix_gop,i4_extra_p_mix_gop;
    WORD32 i4_pb_frms_till_prev_p;
    WORD32 ai4_diff_in_frms[MAX_PIC_TYPE];

    /* Initialize the local vars from the state struct */
    i4_is_gop_closed = ps_pic_handling->i4_is_gop_closed;
    i4_prev_inter_frm_int = ps_pic_handling->i4_inter_frm_int;
    i4_max_inter_frm_int = ps_pic_handling->i4_max_inter_frm_int;
    i4_b_in_incomp_subgop = ps_pic_handling->i4_b_in_incomp_subgop;
    i4_extra_p = ps_pic_handling->i4_extra_p;
    i4_b_in_incomp_subgop_mix_gop =
                    ps_pic_handling->i4_b_in_incomp_subgop_mix_gop;
    i4_extra_p_mix_gop = ps_pic_handling->i4_extra_p_mix_gop;
    i4_pic_disp_order_no = ps_pic_handling->i4_pic_disp_order_no;

    i4_pb_frms_till_prev_p = (ps_pic_handling->i4_p_count_in_gop
                              * i4_prev_inter_frm_int);

    /* Check for the validity of the intra_frm_int */
    if(i4_intra_frm_int <= 0)
    {
        i4_intra_frm_int = ps_pic_handling->i4_intra_frm_int;
    }
    /* Check for the validity of the inter_frm_int */
    if((i4_inter_frm_int > i4_max_inter_frm_int) || (i4_inter_frm_int < 0))
    {
        i4_inter_frm_int = ps_pic_handling->i4_inter_frm_int;
    }

    /* Keep a copy of the older frms_in_gop */
    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        ai4_diff_in_frms[i] = ps_pic_handling->i4_frms_in_cur_gop[i];
    }

    /* Update all the variables which are calculated from the inter_frm_int */

    /* Get the new pic distribution in the gop */
    find_pic_distbn_in_gop(ps_pic_handling->i4_frms_in_gop, i4_intra_frm_int,
                           i4_inter_frm_int, i4_is_gop_closed,
                           &i4_b_in_incomp_subgop, &i4_extra_p);

    /* Find the other related variables */
    if(i4_gop_boundary == 0)
    {
        /*
         * Since, the inter frame interval has changed between a gop the
         * current gop will be a mixed gop. So, we need to find the values of
         * the related variables
         */
        find_pic_distbn_in_gop(ps_pic_handling->i4_frms_in_cur_gop,
                               (i4_intra_frm_int - i4_pb_frms_till_prev_p),
                               i4_inter_frm_int, i4_is_gop_closed,
                               &i4_b_in_incomp_subgop_mix_gop,
                               &i4_extra_p_mix_gop);

        ps_pic_handling->i4_frms_in_cur_gop[P_PIC] +=
                        ps_pic_handling->i4_p_count_in_gop;
        ps_pic_handling->i4_frms_in_cur_gop[B_PIC] +=
                        ps_pic_handling->i4_b_count_in_gop;
    }
    else
    {
        /*
         * Since, the inter_frm_interval has changed at a gop boundary, the
         * new gop will have all the subgops with the new inter_frm_interval
         */
        for(i = 0; i < MAX_PIC_TYPE; i++)
        {
            ps_pic_handling->i4_frms_in_cur_gop[i] =
                            ps_pic_handling->i4_frms_in_gop[i];
        }

        i4_b_in_incomp_subgop_mix_gop = i4_b_in_incomp_subgop;
        i4_extra_p_mix_gop = i4_extra_p;
    }

    /* For bit-allocation the rem_frms_in_gop need to be updated */
    /* Checks needed:
     1) If the encoding is happening on the same gop as that of the buffering */
    if(ps_pic_handling->i4_pic_disp_order_no
                    >= (i4_max_inter_frm_int - 1- ((!i4_is_gop_closed)
                       * ps_pic_handling->i4_b_in_incomp_subgop_mix_gop)))
    {
        for(i = 0; i < MAX_PIC_TYPE; i++)
        {
            ps_pic_handling->i4_rem_frms_in_gop[i] +=
                            (ps_pic_handling->i4_frms_in_cur_gop[i]
                                            - ai4_diff_in_frms[i]);
        }
    }

    /* Update the vars which will affect the proper filling of the pic_stack */
    if(i4_pic_disp_order_no == 0) /*Check if redundant*/
    {
        ps_pic_handling->i4_buf_pic_no = 0;
    }
    else
    {
        ps_pic_handling->i4_buf_pic_no = 1;
    }

    ps_pic_handling->i4_b_count_in_subgop = 0;

    /* Update the state struct with the new inter_frm_int */
    ps_pic_handling->i4_inter_frm_int = i4_inter_frm_int;
    ps_pic_handling->i4_intra_frm_int = i4_intra_frm_int;
    ps_pic_handling->i4_b_in_incomp_subgop = i4_b_in_incomp_subgop;
    ps_pic_handling->i4_extra_p = i4_extra_p;
    ps_pic_handling->i4_b_in_incomp_subgop_mix_gop =
                    i4_b_in_incomp_subgop_mix_gop;
    ps_pic_handling->i4_extra_p_mix_gop = i4_extra_p_mix_gop;

}

/* *****************************************************************************
 * @brief Distributes the frames as I, P and B based on intra/inter frame interval.
 *  Along with it it fills the number of frames in sub-gop and extra p frame
 *
 ******************************************************************************/
static void find_pic_distbn_in_gop(WORD32 i4_frms_in_gop[MAX_PIC_TYPE],
                                   WORD32 i4_intra_frm_int,
                                   WORD32 i4_inter_frm_int,
                                   WORD32 i4_is_gop_closed,
                                   WORD32 *pi4_b_in_incomp_subgop,
                                   WORD32 *pi4_extra_p)
{
    /*
     * Find the pic distribution in the gop depending on the inter and intra
     * frm intervals
     */
    i4_frms_in_gop[I_PIC] = 1;

    /* All I frames */
    if(i4_intra_frm_int == 1)
    {
        i4_frms_in_gop[P_PIC] = 0;
        i4_frms_in_gop[B_PIC] = 0;
        *pi4_b_in_incomp_subgop = 0;
        *pi4_extra_p = 0;
    }
    else
    {
        if(i4_is_gop_closed)
        {
            i4_frms_in_gop[P_PIC] = ((i4_intra_frm_int - 2) / i4_inter_frm_int)
                            + 1;

            if((((i4_intra_frm_int - 2) / i4_inter_frm_int) * i4_inter_frm_int)
                            == (i4_intra_frm_int - 2))
            {
                *pi4_extra_p = 1;
            }
            else
            {
                *pi4_extra_p = 0;
            }
        }
        else
        {
            i4_frms_in_gop[P_PIC] = ((i4_intra_frm_int - 1) / i4_inter_frm_int);

            *pi4_extra_p = 0;
        }

        i4_frms_in_gop[B_PIC] = (i4_intra_frm_int - 1 - i4_frms_in_gop[P_PIC]);

        *pi4_b_in_incomp_subgop = (i4_frms_in_gop[B_PIC] - (i4_inter_frm_int - 1)
                                   * ((i4_intra_frm_int - 1)/ i4_inter_frm_int));
    }
}

WORD32 irc_pic_type_get_intra_frame_interval(pic_handling_t *ps_pic_handling)
{

    return (ps_pic_handling->i4_intra_frm_int);
}

WORD32 irc_pic_type_get_inter_frame_interval(pic_handling_t *ps_pic_handling)
{
    return (ps_pic_handling->i4_inter_frm_int);
}

void irc_pic_type_get_rem_frms_in_gop(pic_handling_t *ps_pic_handling,
                                      WORD32 ai4_rem_frms_in_gop[MAX_PIC_TYPE])
{
    memcpy(ai4_rem_frms_in_gop, ps_pic_handling->i4_rem_frms_in_gop,
           sizeof(ps_pic_handling->i4_rem_frms_in_gop));
}

WORD32 irc_pic_type_get_frms_in_gop_force_I_frm(pic_handling_t *ps_pic_handling)
{
    return (ps_pic_handling->i4_frames_in_fif_gop);
}

void irc_pic_type_get_frms_in_gop(pic_handling_t *ps_pic_handling,
                                  WORD32 ai4_frms_in_gop[MAX_PIC_TYPE])
{
    memcpy(ai4_frms_in_gop, ps_pic_handling->i4_frms_in_cur_gop,
           sizeof(ps_pic_handling->i4_frms_in_cur_gop));
}

WORD32 irc_pic_type_get_disp_order_no(pic_handling_t *ps_pic_handling)
{
    return (ps_pic_handling->i4_pic_disp_order_no);
}

void irc_set_force_I_frame_flag(pic_handling_t *ps_pic_handling)
{
    ps_pic_handling->i4_force_I_frame = 1;
}
WORD32 irc_get_forced_I_frame_cur_frm_flag(pic_handling_t *ps_pic_handling)
{
    return (ps_pic_handling->i4_forced_I_frame_cur_frame);
}
void irc_reset_forced_I_frame_cur_frm_flag(pic_handling_t *ps_pic_handling)
{
    ps_pic_handling->i4_forced_I_frame_cur_frame = 0;
}

/******************************************************************************/
/* Functions that work on the encoded frames */
/******************************************************************************/

/******************************************************************************
 Function Name   : irc_update_pic_handling
 Description     : Will be called only for the frames to be encoded
 *****************************************************************************/
void irc_update_pic_handling(pic_handling_t *ps_pic_handling,
                             picture_type_e e_pic_type)
{

    WORD32 i4_max_inter_frm_int;
    WORD32 i;

    /* Initializing the local vars with that of the state struct */
    i4_max_inter_frm_int = ps_pic_handling->i4_max_inter_frm_int;

    /* Update the variables working on the output frames */
    /* Update the stack count */
    ps_pic_handling->i4_stack_count++;

    if(ps_pic_handling->i4_stack_count == (i4_max_inter_frm_int + 1))
    {
        ps_pic_handling->i4_stack_count = 0;
    }

    /* Update the rem_frms_in_gop */
    ps_pic_handling->i4_rem_frms_in_gop[e_pic_type]--;

    /* Assumption : Rem_frms_in_gop needs to be taken care of, for every change in frms */
    ps_pic_handling->i4_last_frm_in_gop = 0;
    if((ps_pic_handling->i4_rem_frms_in_gop[I_PIC] <= 0)
                    && (ps_pic_handling->i4_rem_frms_in_gop[P_PIC] <= 0)
                    && (ps_pic_handling->i4_rem_frms_in_gop[B_PIC] <= 0))
    {
        /* Copy the cur_frms_in_gop to the rem_frm_in_gop */
        for(i = 0; i < MAX_PIC_TYPE; i++)
        {
            ps_pic_handling->i4_rem_frms_in_gop[i] =
                            ps_pic_handling->i4_frms_in_cur_gop[i];
        }

        ps_pic_handling->i4_last_frm_in_gop = 1;
        ps_pic_handling->i4_first_gop_encoded = 1;
    }
}

WORD32 irc_is_last_frame_in_gop(pic_handling_handle ps_pic_handling)
{
    return (ps_pic_handling->i4_last_frm_in_gop);
}

/******************************************************************************
 Function Name   : irc_skip_encoded_frame
 Description     : Needs to go to the current pic in the pic_stack.
                   If it's B_PIC don't do anything
                   If it's a reference picture, push all but the last B_PICs
                   in the current subgop one place down (i.e. just copy their
                   pic_details) and move the last B_PIC in that subgop to the
                   next slot of the skipped picture and convert it's pic_type
                   to that of the reference picture
 *****************************************************************************/
void irc_skip_encoded_frame(pic_handling_t *ps_pic_handling,
                            picture_type_e e_pic_type)
{
    pic_details_t s_pic_details;
    WORD32 i4_stack_count, i4_next_ref_pic_idx, i4_pic_idx;
    WORD32 i4_max_inter_frm_int, i4_last_b_pic_idx, i4_first_b_pic_idx;
    WORD32 i4_next_pic_idx;

    /* State variables used to initialize the local vars (Not to be changed) */
    i4_stack_count = ps_pic_handling->i4_stack_count;
    i4_next_ref_pic_idx = ps_pic_handling->i4_ref_pic_idx;
    i4_max_inter_frm_int = ps_pic_handling->i4_max_inter_frm_int;

    i4_next_pic_idx = ((i4_stack_count + 1) % (i4_max_inter_frm_int + 1));

    /*
     * Check what is the encoded frm_type
     * Changing a B_PIC to a ref_pic is not reqd if
     * there are no B_PICs referring from the skipped ref_pic
     */
    if(((e_pic_type == P_PIC) || (e_pic_type == I_PIC))
                    && (i4_next_pic_idx != i4_next_ref_pic_idx))
    {
        /* Go to the last B_PIC before the next_ref_pic */
        if(i4_next_ref_pic_idx == 0)
        {
            i4_last_b_pic_idx = i4_max_inter_frm_int;
        }
        else
        {
            i4_last_b_pic_idx = (i4_next_ref_pic_idx - 1);
        }

        /* Keep a copy of the last B_PIC pic_details */
        memcpy(&s_pic_details,
               &ps_pic_handling->as_pic_stack[i4_last_b_pic_idx],
               sizeof(pic_details_t));

        i4_pic_idx = i4_last_b_pic_idx;
        i4_first_b_pic_idx = (i4_stack_count + 1) % (i4_max_inter_frm_int + 1);

        /*
         * All the B_PICs other than the last one, need to be shifted one place
         * in the stack
         */
        while((i4_pic_idx != i4_stack_count)
                        && (i4_first_b_pic_idx != i4_last_b_pic_idx))
        {
            if(i4_pic_idx == 0)
            {
                i4_pic_idx = i4_max_inter_frm_int;
            }
            else
            {
                i4_pic_idx--;
            }

            memcpy(&ps_pic_handling->as_pic_stack[(i4_pic_idx + 1)
                                   % (i4_max_inter_frm_int + 1)],
                   &ps_pic_handling->as_pic_stack[i4_pic_idx],
                   sizeof(pic_details_t));

        }

        /*
         * Copy the last B_PIC pic_details to the first B_PIC place and change
         * it's pic type to the ref_PIC
         */
        /*e_ref_pic_type*/
        ps_pic_handling->as_pic_stack[i4_first_b_pic_idx].e_pic_type = P_PIC;

        ps_pic_handling->as_pic_stack[i4_first_b_pic_idx].i4_pic_disp_order_no =
                        s_pic_details.i4_pic_disp_order_no;
        ps_pic_handling->as_pic_stack[i4_first_b_pic_idx].i4_pic_id =
                        s_pic_details.i4_pic_id;

        /* Change the rem_frms_in_prd so that the update works properly */
        if(ps_pic_handling->i4_rem_frms_in_gop[B_PIC] > 0)
        {
            ps_pic_handling->i4_rem_frms_in_gop[B_PIC]--;
            ps_pic_handling->i4_rem_frms_in_gop[P_PIC]++;
        }
    }

}

/******************************************************************************
 Function Name   : flush_frame
 Description     : Since when a flush frame is called, there will be no valid
                   frames after it, the last frame cannot be a B_PIC, as there
                   will be no reference frame for it (Input in display order)

                   So,this fxn needs to go to the last added pic in the pic_stack.
                   If it's reference pic don't do anything
                   If it's a B_PIC, copy it's pic_details and put it in the
                   place of the next reference pic, changing the pic_type to
                   P_PIC
 *****************************************************************************/
void irc_flush_frame_from_pic_stack(pic_handling_t *ps_pic_handling)
{

    pic_details_t s_prev_pic_details;

    /* Get the last entered pic_details (not to be modified here) */
    WORD32 i4_prev_b_pic_idx = ps_pic_handling->i4_prev_b_pic_idx;
    WORD32 i4_ref_pic_idx = ps_pic_handling->i4_ref_pic_idx;
    WORD32 i4_b_pic_idx = ps_pic_handling->i4_b_pic_idx;

    memcpy(&s_prev_pic_details, &ps_pic_handling->s_prev_pic_details,
           sizeof(pic_details_t));

    if(s_prev_pic_details.e_pic_type == B_PIC)
    {
        /* Copy the last B_PIC details to the next reference pic in display order */
        ps_pic_handling->as_pic_stack[i4_ref_pic_idx].i4_pic_disp_order_no =
                        s_prev_pic_details.i4_pic_disp_order_no;
        ps_pic_handling->as_pic_stack[i4_ref_pic_idx].i4_pic_id =
                        s_prev_pic_details.i4_pic_id;
        ps_pic_handling->as_pic_stack[i4_ref_pic_idx].e_pic_type = P_PIC;

        /*
         * Modify the last B_PIC pic_type, so that codec gets to know when
         * all the buffered frames
         * are flushed
         */
        ps_pic_handling->as_pic_stack[i4_prev_b_pic_idx].e_pic_type =
                        MAX_PIC_TYPE;
        ps_pic_handling->as_pic_stack[i4_prev_b_pic_idx].i4_pic_id = -1;
        ps_pic_handling->as_pic_stack[i4_prev_b_pic_idx].i4_pic_disp_order_no =
                        -1;
    }
    else
    {
        /*
         * Modify the next pic_type details in the stack, so that codec gets to
         * know when all the
         * buffered frames are flushed
         */
        ps_pic_handling->as_pic_stack[i4_ref_pic_idx].e_pic_type = MAX_PIC_TYPE;
        ps_pic_handling->as_pic_stack[i4_ref_pic_idx].i4_pic_id = -1;
        ps_pic_handling->as_pic_stack[i4_ref_pic_idx].i4_pic_disp_order_no = -1;

        if(ps_pic_handling->i4_inter_frm_int != 1)
        {
            ps_pic_handling->as_pic_stack[i4_b_pic_idx].e_pic_type =
                            MAX_PIC_TYPE;
            ps_pic_handling->as_pic_stack[i4_b_pic_idx].i4_pic_id = -1;
            ps_pic_handling->as_pic_stack[i4_b_pic_idx].i4_pic_disp_order_no =
                            -1;
        }
    }
}

/******************************************************************************
 Function Name   : irc_add_pic_to_stack_re_enc
 Description     : In case of a re-enc, we can assume the pictures to be coming
                   in the encode order.
                   In case of re-encoder basically, there are 2 problematic cases.
                   1)Inter_frm_int is not known to start with
                   2)Inter_frm_int can keep changing
                   3)Intra_frm_int set by the application and that actually in the
                    decoded bitstream may be different
 *****************************************************************************/
WORD32 irc_add_pic_to_stack_re_enc(pic_handling_t *ps_pic_handling,
                                   WORD32 i4_enc_pic_id,
                                   picture_type_e e_pic_type)
{
    WORD32 i4_b_count_in_subgop;
    WORD32 i4_max_inter_frm_int, i4_inter_frm_int, i4_intra_frm_int;
    WORD32 i4_pic_disp_order_no;
    WORD32 i4_is_gop_closed;
    picture_type_e e_out_pic_type;
    WORD32 i4_b_in_incomp_subgop;

    /* Check if a change in intra_frm_int call has been made */
    if(ps_pic_handling->i4_change_in_intra_frm_int == 1)
    {
        irc_update_pic_distbn(ps_pic_handling,
                              ps_pic_handling->i4_new_intra_frm_int,
                              ps_pic_handling->i4_inter_frm_int, 1);
        ps_pic_handling->i4_change_in_intra_frm_int = 0;
    }

    /* Check if a change in inter_frm_int call has been made */
    if(ps_pic_handling->i4_change_in_inter_frm_int == 1)
    {
        irc_update_pic_distbn(ps_pic_handling,
                              ps_pic_handling->i4_intra_frm_int,
                              ps_pic_handling->i4_new_inter_frm_int, 1);

        ps_pic_handling->i4_change_in_inter_frm_int = 0;
    }

    /* Initialize the local vars with the state vars */
    i4_b_count_in_subgop = ps_pic_handling->i4_b_count_in_subgop;
    i4_max_inter_frm_int = ps_pic_handling->i4_max_inter_frm_int;
    i4_inter_frm_int = ps_pic_handling->i4_inter_frm_int;
    i4_intra_frm_int = ps_pic_handling->i4_intra_frm_int;
    i4_pic_disp_order_no = ps_pic_handling->i4_pic_disp_order_no;
    i4_is_gop_closed = ps_pic_handling->i4_is_gop_closed;
    i4_b_in_incomp_subgop = ps_pic_handling->i4_b_in_incomp_subgop;

    e_out_pic_type = e_pic_type;

    /* Initially the rate_control assumes an IPP sequence */
    if(e_pic_type == B_PIC)
    {
        /* Update the number of B_PICs in a subgop */
        i4_b_count_in_subgop++;

        if(i4_b_count_in_subgop > i4_max_inter_frm_int)
        {
            return (-1);
        }

        /* If the number of B_PICs exceed the set inter_frm_int then
         change the inter_frm_int */
        if(i4_b_count_in_subgop > (i4_inter_frm_int - 1))
        {
            i4_inter_frm_int = (i4_b_count_in_subgop + 1);

            irc_update_pic_distbn(ps_pic_handling, i4_intra_frm_int,
                                  i4_inter_frm_int, 0);
        }
    }
    else if((e_pic_type == I_PIC) || (e_pic_type == P_PIC))
    {
        /* If the B_PICs in the prev subgop were fewer than the current
         * (inter_frm_int-1) and none of these conditions occur, it'll mean the
         *  decrease in the inter_frm_int
         *    1)End of a GOP
         *    2)Beginning of an OPEN_GOP
         */
        if((i4_b_count_in_subgop < (i4_inter_frm_int - 1))
                        && !((!i4_is_gop_closed)
                                        && (i4_b_count_in_subgop
                                                        >= i4_b_in_incomp_subgop))
                        && !((i4_pic_disp_order_no
                                        + (i4_inter_frm_int - 1
                                                        - i4_b_count_in_subgop))
                                        > i4_intra_frm_int))
        {
            i4_inter_frm_int = (i4_b_count_in_subgop + 1);

            irc_update_pic_distbn(ps_pic_handling, i4_intra_frm_int,
                                  i4_inter_frm_int, 0);
        }

        /* Reset the number of B_PICs in a subgop */
        i4_b_count_in_subgop = 0;
    }

    /* Updation of the frame level vars */
    i4_pic_disp_order_no++;

    /* End of gop condition
     *Two cases can arise :
     *1) The intra_frm_int set by the application is greater than the actual
     *   bitstream intra_frm_int (i.e. we will get an I frame before
     *   pic_disp_order_no goes to intra_frm_int)
     *2) The intra_frm_int set by the application is smaller than the actual bitstream intra_frm_int
     *   (i.e. we won't get an I_PIC even if pic_disp_order_no goes to
     *   intra_frm_int) Constraints :
     *    1) I_PIC cannot be changed to B_PIC
     *    2) B_PIC cannot be changed to I_PIC
     */
    if(i4_pic_disp_order_no >= i4_intra_frm_int)
    {
        if(e_pic_type != B_PIC)
        {
            e_out_pic_type = I_PIC;
        }
        else
        {
            e_out_pic_type = B_PIC;
            ps_pic_handling->i4_rem_frms_in_gop[B_PIC]++;
            ps_pic_handling->i4_frms_in_cur_gop[B_PIC]++;
            ps_pic_handling->i4_frms_in_gop[B_PIC]++;
        }
    }
    else
    {
        if((e_pic_type == I_PIC) && (!ps_pic_handling->i4_is_first_gop))
        {
            e_out_pic_type = P_PIC;
            ps_pic_handling->i4_rem_frms_in_gop[P_PIC]++;
            ps_pic_handling->i4_frms_in_cur_gop[P_PIC]++;
            ps_pic_handling->i4_frms_in_gop[P_PIC]++;
        }
        else
        {
            e_out_pic_type = e_pic_type;
        }
    }

    /* Update the frm_vars at the end of the gop */
    if(i4_pic_disp_order_no
                    == (ps_pic_handling->i4_frms_in_cur_gop[P_PIC]
                                    + ps_pic_handling->i4_frms_in_cur_gop[B_PIC]
                                    + 1))
    {
        i4_pic_disp_order_no = 0;
        ps_pic_handling->i4_is_first_gop = 0;
    }

    /* Update the vars working on the encoded pics */
    if((ps_pic_handling->i4_is_first_gop)
                    && (ps_pic_handling->i4_stack_count == -1))
    {
        ps_pic_handling->i4_coded_pic_no = 0;
        ps_pic_handling->i4_stack_count = 0;
    }

    /* Add the pic_details to the pic_stack */
    ps_pic_handling->as_pic_stack[ps_pic_handling->i4_stack_count].e_pic_type =
                    e_out_pic_type;
    ps_pic_handling->as_pic_stack[ps_pic_handling->i4_stack_count].i4_pic_disp_order_no =
                    ps_pic_handling->i4_pic_disp_order_no;
    ps_pic_handling->as_pic_stack[ps_pic_handling->i4_stack_count].i4_pic_id =
                    i4_enc_pic_id;

    /* Writing back those values which need to be updated */
    ps_pic_handling->i4_inter_frm_int = i4_inter_frm_int;
    ps_pic_handling->i4_pic_disp_order_no = i4_pic_disp_order_no;
    ps_pic_handling->i4_b_count_in_subgop = i4_b_count_in_subgop;

    return (0);
}
