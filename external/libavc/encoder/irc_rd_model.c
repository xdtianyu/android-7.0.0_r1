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

/****************************************************************************/
/* File Name         : irc_rd_model.c                                       */
/*                                                                          */
/* Description       : Implall the Functions to Model the                   */
/*                     Rate Distortion Behaviour of the Codec over the Last */
/*                     Few Frames.                                          */
/*                                                                          */
/* List of Functions : irc_update_frame_rd_model                            */
/*                     estimate_mpeg2_qp_for_resbits                        */
/*                                                                          */
/* Issues / Problems : None                                                 */
/*                                                                          */
/* Revision History  :                                                      */
/*        DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*        21 06 2006   Sarat           Initial Version                      */
/****************************************************************************/

/* System include files */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "math.h"

/* User include files */
#include "irc_datatypes.h"
#include "irc_common.h"
#include "irc_mem_req_and_acq.h"
#include "irc_rd_model.h"
#include "irc_rd_model_struct.h"


WORD32 irc_rd_model_num_fill_use_free_memtab(rc_rd_model_t **pps_rc_rd_model,
                                             itt_memtab_t *ps_memtab,
                                             ITT_FUNC_TYPE_E e_func_type)
{
    WORD32 i4_mem_tab_idx = 0;
    rc_rd_model_t s_rc_rd_model_temp;

    /*
     * Hack for al alloc, during which we don't have any state memory.
     * Dereferencing can cause issues
     */
    if(e_func_type == GET_NUM_MEMTAB || e_func_type == FILL_MEMTAB)
        (*pps_rc_rd_model) = &s_rc_rd_model_temp;

    /*for src rate control state structure*/
    if(e_func_type != GET_NUM_MEMTAB)
    {
        fill_memtab(&ps_memtab[i4_mem_tab_idx], sizeof(rc_rd_model_t),
                    ALIGN_128_BYTE, PERSISTENT, DDR);
        use_or_fill_base(&ps_memtab[0], (void**)pps_rc_rd_model, e_func_type);
    }
    i4_mem_tab_idx++;

    return (i4_mem_tab_idx);
}

void irc_init_frm_rc_rd_model(rc_rd_model_t *ps_rd_model,
                              UWORD8 u1_max_frames_modelled)
{

    ps_rd_model->u1_num_frms_in_model = 0;
    ps_rd_model->u1_curr_frm_counter = 0;
    ps_rd_model->u1_max_frms_to_model = u1_max_frames_modelled;

    ps_rd_model->model_coeff_a_lin_wo_int = 0;
    ps_rd_model->model_coeff_b_lin_wo_int = 0;
    ps_rd_model->model_coeff_c_lin_wo_int = 0;
}

void irc_reset_frm_rc_rd_model(rc_rd_model_t *ps_rd_model)
{
    ps_rd_model->u1_num_frms_in_model = 0;
    ps_rd_model->u1_curr_frm_counter = 0;

    ps_rd_model->model_coeff_a_lin_wo_int = 0;
    ps_rd_model->model_coeff_b_lin_wo_int = 0;
    ps_rd_model->model_coeff_c_lin_wo_int = 0;
}

static UWORD8 find_model_coeffs(UWORD32 *pi4_res_bits,
                                UWORD32 *pi4_sad_h264,
                                UWORD8 *pu1_num_skips,
                                UWORD8 *pui_avg_mpeg2_qp,
                                UWORD8 u1_num_frms,
                                UWORD8 u1_model_used,
                                WORD8 *pi1_frame_index,
                                model_coeff *pmc_model_coeff,
                                model_coeff *pmc_model_coeff_lin,
                                model_coeff *pmc_model_coeff_lin_wo_int,
                                rc_rd_model_t *ps_rd_model)
{
    UWORD32 i;
    UWORD8 u1_num_frms_used = 0;
    UWORD8 u1_frm_indx;

    float sum_y = 0;
    float sum_x_y = 0;
    float sum_x2_y = 0;
    float sum_x = 0;
    float sum_x2 = 0;
    float sum_x3 = 0;
    float sum_x4 = 0;

    float x0, y0;
    float model_coeff_a = 0.0, model_coeff_b = 0.0, model_coeff_c = 0.0;

#if !(ENABLE_QUAD_RC_MODEL||ENABLE_LIN_MODEL_WITH_INTERCEPT)
    UNUSED(pu1_num_skips);
    UNUSED(pmc_model_coeff);
    UNUSED(pmc_model_coeff_lin);
#endif

    for(i = 0; i < u1_num_frms; i++)
    {
        if(-1 == pi1_frame_index[i])
            continue;

        u1_frm_indx = (UWORD8)pi1_frame_index[i];

        y0 = (float)(pi4_res_bits[u1_frm_indx]);
        x0 = (float)(pi4_sad_h264[u1_frm_indx]
                        / (float)pui_avg_mpeg2_qp[u1_frm_indx]);

        sum_y += y0;
        sum_x_y += x0 * y0;
        sum_x2_y += x0 * x0 * y0;
        sum_x += x0;
        sum_x2 += x0 * x0;
        sum_x3 += x0 * x0 * x0;
        sum_x4 += x0 * x0 * x0 * x0;
        u1_num_frms_used++;
    }

    sum_y /= u1_num_frms_used;
    sum_x_y /= u1_num_frms_used;
    sum_x2_y /= u1_num_frms_used;
    sum_x /= u1_num_frms_used;
    sum_x2 /= u1_num_frms_used;
    sum_x3 /= u1_num_frms_used;
    sum_x4 /= u1_num_frms_used;

    {
        UWORD8 u1_curr_frame_index;
        UWORD8 u1_avgqp_prvfrm;
        UWORD32 u4_prevfrm_bits, u4_prevfrm_sad;

        u1_curr_frame_index = ps_rd_model->u1_curr_frm_counter;
        if(0 == u1_curr_frame_index)
            u1_curr_frame_index = (MAX_FRAMES_MODELLED - 1);
        else
            u1_curr_frame_index--;

        u1_avgqp_prvfrm = ps_rd_model->pu1_avg_qp[u1_curr_frame_index];
        u4_prevfrm_bits = ps_rd_model->pi4_res_bits[u1_curr_frame_index];
        u4_prevfrm_sad = ps_rd_model->pi4_sad[u1_curr_frame_index];

        if(0 != u4_prevfrm_sad)
            model_coeff_a = (float)(u4_prevfrm_bits * u1_avgqp_prvfrm)
                            / u4_prevfrm_sad;
        else
            model_coeff_a = 0;

        model_coeff_b = 0;
        model_coeff_c = 0;

        pmc_model_coeff_lin_wo_int[0] = model_coeff_b;
        pmc_model_coeff_lin_wo_int[1] = model_coeff_a;
        pmc_model_coeff_lin_wo_int[2] = model_coeff_c;
    }

    return u1_model_used;
}

static void irc_update_frame_rd_model(rc_rd_model_t *ps_rd_model)
{
    WORD8 pi1_frame_index[MAX_FRAMES_MODELLED],
                    pi1_frame_index_initial[MAX_FRAMES_MODELLED];

    UWORD8 u1_num_skips_temp;
    UWORD8 u1_avg_mpeg2_qp_temp, u1_min_mpeg2_qp, u1_max_mpeg2_qp;
    UWORD8 u1_num_frms_input, u1_num_active_frames, u1_reject_frame;
    UWORD32 u4_num_skips;

    UWORD8 u1_min2_mpeg2_qp, u1_max2_mpeg2_qp;
    UWORD8 u1_min_qp_frame_indx, u1_max_qp_frame_indx;
    UWORD8 pu1_num_frames[MPEG2_QP_ELEM];
    model_coeff model_coeff_array[3], model_coeff_array_lin[3],
                    model_coeff_array_lin_wo_int[3];
    UWORD32 i;
    UWORD8 u1_curr_frame_index;

    u1_curr_frame_index = ps_rd_model->u1_curr_frm_counter;

    ps_rd_model->u1_model_used = PREV_FRAME_MODEL;

    if(0 == u1_curr_frame_index)
        u1_curr_frame_index = (MAX_FRAMES_MODELLED - 1);
    else
        u1_curr_frame_index--;

    /************************************************************************/
    /* Rearrange data to be fed into a Linear Regression Module             */
    /* Module finds a,b,c such that                                         */
    /*      y = ax + bx^2 + c                                               */
    /************************************************************************/
    u4_num_skips = 0;
    u1_num_frms_input = 0;
    memset(pu1_num_frames, 0, MPEG2_QP_ELEM);
    memset(pi1_frame_index, -1, MAX_FRAMES_MODELLED);
    u1_min_mpeg2_qp = MAX_MPEG2_QP;
    u1_max_mpeg2_qp = 0;

    u1_num_active_frames = ps_rd_model->u1_num_frms_in_model;
    if(u1_num_active_frames > MAX_ACTIVE_FRAMES)
    {
        u1_num_active_frames = MAX_ACTIVE_FRAMES;
    }

    /************************************************************************/
    /* Choose the set of Points to be used for MSE fit of Quadratic model   */
    /* Points chosen are spread across the Qp range. Max of 2 points are    */
    /* chosen for a Qp.                                                     */
    /************************************************************************/
    for(i = 0; i < u1_num_active_frames; i++)
    {
        u1_reject_frame = 0;
        u1_num_skips_temp = ps_rd_model->pu1_num_skips[u1_curr_frame_index];
        u1_avg_mpeg2_qp_temp = ps_rd_model->pu1_avg_qp[u1_curr_frame_index];

        if((0 == u4_num_skips) && (0 != u1_num_skips_temp))
            u1_reject_frame = 1;
        if((1 == u4_num_skips) && (u1_num_skips_temp > 1))
            u1_reject_frame = 1;
        if(pu1_num_frames[u1_avg_mpeg2_qp_temp] >= 2)
            u1_reject_frame = 1;

        if(0 == i)
            u1_reject_frame = 0;

        if(0 == u1_reject_frame)
        {
            pi1_frame_index[u1_num_frms_input] = (WORD8)u1_curr_frame_index;
            pu1_num_frames[u1_avg_mpeg2_qp_temp] += 1;

            if(u1_min_mpeg2_qp > u1_avg_mpeg2_qp_temp)
                u1_min_mpeg2_qp = u1_avg_mpeg2_qp_temp;
            if(u1_max_mpeg2_qp < u1_avg_mpeg2_qp_temp)
                u1_max_mpeg2_qp = u1_avg_mpeg2_qp_temp;

            u1_num_frms_input++;
        }

        if(0 == u1_curr_frame_index)
            u1_curr_frame_index = (MAX_FRAMES_MODELLED - 1);
        else
            u1_curr_frame_index--;
    }

    /************************************************************************/
    /* Add Pivot Points to the Data set to be used for finding Quadratic    */
    /* Model Coeffs. These will help in constraining the shape of  Quadratic*/
    /* to adapt too much to the Local deviations.                           */
    /************************************************************************/
    u1_min2_mpeg2_qp = u1_min_mpeg2_qp;
    u1_max2_mpeg2_qp = u1_max_mpeg2_qp;
    u1_min_qp_frame_indx = INVALID_FRAME_INDEX;
    u1_max_qp_frame_indx = INVALID_FRAME_INDEX;

    /* Loop runnning over the Stored Frame Level Data
     to find frames of MinQp and MaxQp */
    for(; i < ps_rd_model->u1_num_frms_in_model; i++)
    {
        u1_num_skips_temp = ps_rd_model->pu1_num_skips[u1_curr_frame_index];
        u1_avg_mpeg2_qp_temp = ps_rd_model->pu1_avg_qp[u1_curr_frame_index];

        if(((0 == u4_num_skips) && (0 != u1_num_skips_temp))
                        || ((1 == u4_num_skips) && (u1_num_skips_temp > 1)))
            continue;

        if(u1_min2_mpeg2_qp > u1_avg_mpeg2_qp_temp)
        {
            u1_min2_mpeg2_qp = u1_avg_mpeg2_qp_temp;
            u1_min_qp_frame_indx = u1_curr_frame_index;
        }
        if(u1_max2_mpeg2_qp < u1_avg_mpeg2_qp_temp)
        {
            u1_max2_mpeg2_qp = u1_avg_mpeg2_qp_temp;
            u1_max_qp_frame_indx = u1_curr_frame_index;
        }
        if(0 == u1_curr_frame_index)
            u1_curr_frame_index = (MAX_FRAMES_MODELLED - 1);
        else
            u1_curr_frame_index--;
    }

    /* Add the Chosen Points to the regression data set */
    if(INVALID_FRAME_INDEX != u1_min_qp_frame_indx)
    {
        pi1_frame_index[u1_num_frms_input] = (WORD8)u1_min_qp_frame_indx;
        u1_num_frms_input++;
    }
    if(INVALID_FRAME_INDEX != u1_max_qp_frame_indx)
    {
        pi1_frame_index[u1_num_frms_input] = (WORD8)u1_max_qp_frame_indx;
        u1_num_frms_input++;
    }
    memcpy(pi1_frame_index_initial, pi1_frame_index, MAX_FRAMES_MODELLED);

    /***** Call the Module to Return the Coeffs for the Fed Data *****/
    ps_rd_model->u1_model_used = find_model_coeffs(ps_rd_model->pi4_res_bits,
                                                   ps_rd_model->pi4_sad,
                                                   ps_rd_model->pu1_num_skips,
                                                   ps_rd_model->pu1_avg_qp,
                                                   u1_num_frms_input,
                                                   ps_rd_model->u1_model_used,
                                                   pi1_frame_index,
                                                   model_coeff_array,
                                                   model_coeff_array_lin,
                                                   model_coeff_array_lin_wo_int,
                                                   ps_rd_model);

    ps_rd_model->model_coeff_b_lin_wo_int = model_coeff_array_lin_wo_int[0];
    ps_rd_model->model_coeff_a_lin_wo_int = model_coeff_array_lin_wo_int[1];
    ps_rd_model->model_coeff_c_lin_wo_int = model_coeff_array_lin_wo_int[2];
}

UWORD32 irc_estimate_bits_for_qp(rc_rd_model_t *ps_rd_model,
                                 UWORD32 u4_estimated_sad,
                                 UWORD8 u1_avg_qp)
{
  float fl_num_bits = 0;

  fl_num_bits = ps_rd_model->model_coeff_a_lin_wo_int
      * ((float)(u4_estimated_sad / u1_avg_qp));

  return ((UWORD32)fl_num_bits);
}

UWORD8 irc_find_qp_for_target_bits(rc_rd_model_t *ps_rd_model,
                                   UWORD32 u4_target_res_bits,
                                   UWORD32 u4_estimated_sad,
                                   UWORD8 u1_min_qp,
                                   UWORD8 u1_max_qp)
{
    UWORD8 u1_qp;
    float x_value = 1.0, f_qp;

    ps_rd_model->u1_model_used = PREV_FRAME_MODEL;

    {
        x_value = (float)u4_target_res_bits
                        / ps_rd_model->model_coeff_a_lin_wo_int;
    }

    if(0 != x_value)
        f_qp = u4_estimated_sad / x_value;
    else
        f_qp = 255;

    if(f_qp > 255)
        f_qp = 255;

    /* Truncating the QP to the Max and Min Qp values possible */
    if(f_qp < u1_min_qp)
        f_qp = u1_min_qp;
    if(f_qp > u1_max_qp)
        f_qp = u1_max_qp;

    u1_qp = (UWORD8)(f_qp + 0.5);

    return u1_qp;
}

void irc_add_frame_to_rd_model(rc_rd_model_t *ps_rd_model,
                               UWORD32 i4_res_bits,
                               UWORD8 u1_avg_mp2qp,
                               UWORD32 i4_sad_h264,
                               UWORD8 u1_num_skips)
{
    UWORD8 u1_curr_frame_index;
    u1_curr_frame_index = ps_rd_model->u1_curr_frm_counter;

    /*Insert the Present Frame Data into the RD Model State Memory*/
    ps_rd_model->pi4_res_bits[u1_curr_frame_index] = i4_res_bits;
    ps_rd_model->pi4_sad[u1_curr_frame_index] = i4_sad_h264;
    ps_rd_model->pu1_num_skips[u1_curr_frame_index] = u1_num_skips;
    ps_rd_model->pu1_avg_qp[u1_curr_frame_index] = u1_avg_mp2qp;

    ps_rd_model->u1_curr_frm_counter++;
    if(MAX_FRAMES_MODELLED == ps_rd_model->u1_curr_frm_counter)
        ps_rd_model->u1_curr_frm_counter = 0;

    if(ps_rd_model->u1_num_frms_in_model < ps_rd_model->u1_max_frms_to_model)
    {
        ps_rd_model->u1_num_frms_in_model++;
    }
    irc_update_frame_rd_model(ps_rd_model);
}

/*****************************************************************************
 *Function Name : irc_calc_per_frm_bits
 *Description   :
 *Inputs        : pu2_num_pics_of_a_pic_type
 *                  -  pointer to RC api pointer
 *                pu2_num_pics_of_a_pic_type
 *                  -  N1, N2,...Nk
 *                pu1_update_pic_type_model
 *                  -  flag which tells whether or not to update model
 *                     coefficients of a particular pic-type
 *                u1_num_pic_types
 *                  - value of k
 *                pu4_num_skip_of_a_pic_type
 *                  - the number of skips of that pic-type. It "may" be used to
 *                    update the model coefficients at a later point. Right now
 *                    it is not being used at all.
 *                u1_base_pic_type
 *                  - base pic type index wrt which alpha & beta are calculated
 *                pfl_gamma
 *                  - gamma_i = beta_i / alpha_i
 *                pfl_eta
 *                  -
 *                u1_curr_pic_type
 *                  - the current pic-type for which the targetted bits need to
 *                    be computed
 *                u4_bits_for_sub_gop
 *                 - the number of bits to be consumed for the remaining part of
 *                   sub-gop
 *                u4_curr_estimated_sad
 *                 -
 *                pu1_curr_pic_type_qp
 *                  -  output of this function
 *****************************************************************************/

WORD32 irc_calc_per_frm_bits(rc_rd_model_t *ps_rd_model,
                             UWORD16 *pu2_num_pics_of_a_pic_type,
                             UWORD8 *pu1_update_pic_type_model,
                             UWORD8 u1_num_pic_types,
                             UWORD32 *pu4_num_skip_of_a_pic_type,
                             UWORD8 u1_base_pic_type,
                             float *pfl_gamma,
                             float *pfl_eta,
                             UWORD8 u1_curr_pic_type,
                             UWORD32 u4_bits_for_sub_gop,
                             UWORD32 u4_curr_estimated_sad,
                             UWORD8 *pu1_curr_pic_type_qp)
{
    WORD32 i4_per_frm_bits_Ti;
    UWORD8 u1_i;
    rc_rd_model_t *ps_rd_model_of_pic_type;

    UNUSED(pu4_num_skip_of_a_pic_type);
    UNUSED(u1_base_pic_type);

    /* First part of this function updates all the model coefficients */
    /*for all the pic-types */
    {
        for(u1_i = 0; u1_i < u1_num_pic_types; u1_i++)
        {
            if((0 != pu2_num_pics_of_a_pic_type[u1_i])
                            && (1 == pu1_update_pic_type_model[u1_i]))
            {
                irc_update_frame_rd_model(&ps_rd_model[u1_i]);
            }
        }
    }

    /*
     * The second part of this function deals with solving the
     * equation using all the pic-types models
     */
    {
        UWORD8 u1_combined_model_used;

        /* solve the equation */
        {
            model_coeff eff_A;
            float fl_sad_by_qp_base;
            float fl_sad_by_qp_curr_frm = 1.0;
            float fl_qp_curr_frm;
            float fl_bits_for_curr_frm = 0;



            /* If the combined chosen model is linear model without an intercept */

            u1_combined_model_used = PREV_FRAME_MODEL;
            {
                eff_A = 0.0;

                for(u1_i = 0; u1_i < u1_num_pic_types; u1_i++)
                {
                    ps_rd_model_of_pic_type = ps_rd_model + u1_i;

                    eff_A += ((pfl_eta[u1_i]
                               + pu2_num_pics_of_a_pic_type[u1_i]- 1)
                               * ps_rd_model_of_pic_type->model_coeff_a_lin_wo_int
                               * pfl_gamma[u1_i]);
                }

                fl_sad_by_qp_base = u4_bits_for_sub_gop / eff_A;

                fl_sad_by_qp_curr_frm = fl_sad_by_qp_base
                                * pfl_gamma[u1_curr_pic_type]
                                * pfl_eta[u1_curr_pic_type];

                ps_rd_model_of_pic_type = ps_rd_model + u1_curr_pic_type;

                fl_bits_for_curr_frm =
                                ps_rd_model_of_pic_type->model_coeff_a_lin_wo_int
                                                * fl_sad_by_qp_curr_frm;
            }

            /*
             * Store the model that was finally used to calculate Qp.
             * This is so that the same model is used in further calculations
             * for this picture.
             */
            ps_rd_model_of_pic_type = ps_rd_model + u1_curr_pic_type;
            ps_rd_model_of_pic_type->u1_model_used = u1_combined_model_used;

            i4_per_frm_bits_Ti = (WORD32)(fl_bits_for_curr_frm + 0.5);

            if(fl_sad_by_qp_curr_frm > 0)
                fl_qp_curr_frm = (float)u4_curr_estimated_sad
                                / fl_sad_by_qp_curr_frm;
            else
                fl_qp_curr_frm = 255;

            if(fl_qp_curr_frm > 255)
                fl_qp_curr_frm = 255;

            *pu1_curr_pic_type_qp = (fl_qp_curr_frm + 0.5);

        }
    }
    return (i4_per_frm_bits_Ti);
}

model_coeff irc_get_linear_coefficient(rc_rd_model_t *ps_rd_model)
{
    return (ps_rd_model->model_coeff_a_lin_wo_int);
}


