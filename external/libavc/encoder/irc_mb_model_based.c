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

/* User include files */
#include "irc_datatypes.h"
#include "irc_cntrl_param.h"
#include "irc_mem_req_and_acq.h"
#include "irc_mb_model_based.h"

typedef struct mb_rate_control_t
{
    /* Frame Qp */
    UWORD8 u1_frm_qp;

    /*
     * Estimated average activity for the current frame (updated with the
     * previous frame activity since it is independent of picture type whether
     * it is I or P)
     */
    WORD32 i4_avg_activity;

} mb_rate_control_t;

WORD32 irc_mbrc_num_fill_use_free_memtab(mb_rate_control_t **pps_mb_rate_control,
                                         itt_memtab_t *ps_memtab,
                                         ITT_FUNC_TYPE_E e_func_type)
{
    WORD32 i4_mem_tab_idx = 0;
    mb_rate_control_t s_mb_rate_control_temp;

    /*
     * Hack for al alloc, during which we don't have any state memory.
     * Dereferencing can cause issues
     */
    if(e_func_type == GET_NUM_MEMTAB || e_func_type == FILL_MEMTAB)
    {
        (*pps_mb_rate_control) = &s_mb_rate_control_temp;
    }

    /*For src rate control state structure*/
    if(e_func_type != GET_NUM_MEMTAB)
    {
        fill_memtab(&ps_memtab[i4_mem_tab_idx], sizeof(mb_rate_control_t),
                    ALIGN_128_BYTE, PERSISTENT, DDR);
        use_or_fill_base(&ps_memtab[0], (void**)pps_mb_rate_control,
                         e_func_type);
    }
    i4_mem_tab_idx++;

    return (i4_mem_tab_idx);
}

/*******************************************************************************
 MB LEVEL API FUNCTIONS
 ******************************************************************************/

/******************************************************************************
 Description     : Initialize the mb model and the average activity to default
                   values
 ******************************************************************************/
void irc_init_mb_level_rc(mb_rate_control_t *ps_mb_rate_control)
{
    /* Set values to default */
    ps_mb_rate_control->i4_avg_activity = 0;
}

/******************************************************************************
 Description     : Initialize the mb state with frame level decisions
 *********************************************************************************/
void irc_mb_init_frame_level(mb_rate_control_t *ps_mb_rate_control,
                             UWORD8 u1_frame_qp)
{
    /* Update frame level QP */
    ps_mb_rate_control->u1_frm_qp = u1_frame_qp;
}

/******************************************************************************
 Description     : Reset the mb activity - Whenever there is SCD
                   the mb activity is reset
 *********************************************************************************/
void irc_reset_mb_activity(mb_rate_control_t *ps_mb_rate_control)
{
    ps_mb_rate_control->i4_avg_activity = 0;
}

/******************************************************************************
 Description     : Calculates the mb level qp
 *********************************************************************************/
void irc_get_mb_qp(mb_rate_control_t *ps_mb_rate_control,
                   WORD32 i4_cur_mb_activity,
                   WORD32 *pi4_mb_qp)
{
    WORD32 i4_qp;
    /* Initialize the mb level qp with the frame level qp */
    i4_qp = ps_mb_rate_control->u1_frm_qp;

    /*
     * Store the model based QP - This is used for updating the rate control model
     */
    pi4_mb_qp[0] = i4_qp;

    /* Modulate the Qp based on the activity */
    if((ps_mb_rate_control->i4_avg_activity) && (i4_qp < 100))
    {
        i4_qp =((((2 * i4_cur_mb_activity))
               + ps_mb_rate_control->i4_avg_activity)* i4_qp
               + ((i4_cur_mb_activity + 2 * ps_mb_rate_control->i4_avg_activity)
               >> 1))/ (i4_cur_mb_activity + 2 * ps_mb_rate_control->i4_avg_activity);

        if(i4_qp > ((3 * ps_mb_rate_control->u1_frm_qp) >> 1))
        {
            i4_qp = ((3 * ps_mb_rate_control->u1_frm_qp) >> 1);
        }
    }

    /* Store the qp modulated by mb activity - This is used for encoding the MB */
    pi4_mb_qp[1] = i4_qp;
}

/*******************************************************************************
 Description     : Returns the stored frame level QP
 ******************************************************************************/
UWORD8 irc_get_frm_level_qp(mb_rate_control_t *ps_mb_rate_control)
{
    return (ps_mb_rate_control->u1_frm_qp);
}

/*******************************************************************************
 Description     : Update the frame level info collected
 ******************************************************************************/
void irc_mb_update_frame_level(mb_rate_control_t *ps_mb_rate_control,
                               WORD32 i4_avg_activity)
{
     /* Update the Average Activity */
     ps_mb_rate_control->i4_avg_activity = i4_avg_activity;
}
