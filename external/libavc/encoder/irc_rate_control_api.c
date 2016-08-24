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
#include "stdio.h"

/* User include files */
#include "irc_datatypes.h"
#include "irc_common.h"
#include "irc_cntrl_param.h"
#include "irc_mem_req_and_acq.h"
#include "irc_rd_model.h"
#include "irc_est_sad.h"
#include "irc_fixed_point_error_bits.h"
#include "irc_vbr_storage_vbv.h"
#include "irc_picture_type.h"
#include "irc_bit_allocation.h"
#include "irc_mb_model_based.h"
#include "irc_cbr_buffer_control.h"
#include "irc_vbr_str_prms.h"
#include "irc_rate_control_api.h"
#include "irc_rate_control_api_structs.h"
#include "irc_trace_support.h"


#define MIN(a,b)   (((a) < (b)) ? (a) : (b))
#define MAX(a,b)   (((a) > (b)) ? (a) : (b))

#define DEV_Q   4       /*Q format(Shift) for Deviation range factor */
#define HI_DEV_FCTR     22  /* 1.4*16 */
#define LO_DEV_FCTR     12  /* 0.75*16 */
#define GET_HI_DEV_QP(Qprev) (( ((WORD32) Qprev)*HI_DEV_FCTR + (1<<(DEV_Q-1)))>>DEV_Q)
#define GET_LO_DEV_QP(Qprev) (( ((WORD32) Qprev)*LO_DEV_FCTR + (1<<(DEV_Q-1)))>>DEV_Q)
#define CLIP_QP(Qc, hi_d, lo_d) (((Qc) < (lo_d))?((lo_d)):(((Qc) > (hi_d))?(hi_d):(Qc)))

/*****************************************************************************/
/* Restricts the quantization parameter variation within delta */
/*****************************************************************************/
/* static WORD32 restrict_swing(WORD32 cur_qp, WORD32 prev_qp, WORD32 delta_qp)
 {
 if((cur_qp) - (prev_qp) > (delta_qp)) (cur_qp) = (prev_qp) + (delta_qp) ;
 if((prev_qp) - (cur_qp) > (delta_qp)) (cur_qp) = (prev_qp) - (delta_qp) ;
 return cur_qp;
 }*/

/*****************************************************************************
 Function Name : rate_control_get_init_free_memtab
 Description   : Takes or gives memtab
 Inputs        : pps_rate_control_api -  pointer to RC api pointer
 ps_memtab            -  Memtab pointer
 i4_use_base          -  Set during init, else 0
 i4_fill_base         -  Set during free, else 0
 *****************************************************************************/
WORD32 irc_rate_control_num_fill_use_free_memtab(rate_control_handle *pps_rate_control_api,
                                                 itt_memtab_t *ps_memtab,
                                                 ITT_FUNC_TYPE_E e_func_type)
{
    WORD32 i4_mem_tab_idx = 0, i;
    rate_control_api_t s_temp_rc_api;

    /*
     * Hack for al alloc, during which we dont have any state memory.
     * Dereferencing can cause issues
     */
    if(e_func_type == GET_NUM_MEMTAB || e_func_type == FILL_MEMTAB)
        (*pps_rate_control_api) = &s_temp_rc_api;

    /*for src rate control state structure*/
    if(e_func_type != GET_NUM_MEMTAB)
    {
        fill_memtab(&ps_memtab[i4_mem_tab_idx], sizeof(rate_control_api_t),
                    ALIGN_128_BYTE, PERSISTENT, DDR);
        use_or_fill_base(&ps_memtab[0], (void**)pps_rate_control_api,
                         e_func_type);
    }
    i4_mem_tab_idx++;

    /* Get the memory requirement of lower modules */
    i4_mem_tab_idx += irc_ba_num_fill_use_free_memtab(
                    &pps_rate_control_api[0]->ps_bit_allocation,
                    &ps_memtab[i4_mem_tab_idx], e_func_type);

    i4_mem_tab_idx += irc_cbr_buffer_num_fill_use_free_memtab(
                    &pps_rate_control_api[0]->ps_cbr_buffer,
                    &ps_memtab[i4_mem_tab_idx], e_func_type);

    i4_mem_tab_idx += irc_est_sad_num_fill_use_free_memtab(
                    &pps_rate_control_api[0]->ps_est_sad,
                    &ps_memtab[i4_mem_tab_idx], e_func_type);

    i4_mem_tab_idx += irc_mbrc_num_fill_use_free_memtab(
                    &pps_rate_control_api[0]->ps_mb_rate_control,
                    &ps_memtab[i4_mem_tab_idx], e_func_type);

    i4_mem_tab_idx += irc_vbr_vbv_num_fill_use_free_memtab(
                    &pps_rate_control_api[0]->ps_vbr_storage_vbv,
                    &ps_memtab[i4_mem_tab_idx], e_func_type);

    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        i4_mem_tab_idx += irc_rd_model_num_fill_use_free_memtab(
                        &pps_rate_control_api[0]->aps_rd_model[i],
                        &ps_memtab[i4_mem_tab_idx], e_func_type);
    }
    i4_mem_tab_idx += irc_pic_handling_num_fill_use_free_memtab(
                    &pps_rate_control_api[0]->ps_pic_handling,
                    &ps_memtab[i4_mem_tab_idx], e_func_type);

    return (i4_mem_tab_idx);
}

/*****************************************************************************
 Function Name : irc_initialise_rate_control
 Description   : Initialise the rate control structure
 Inputs        : ps_rate_control_api   - api struct
                 e_rate_control_type   - VBR, CBR (NLDRC/LDRC), VBR_STREAMING
                 u1_is_mb_level_rc_on  - enabling mb level RC
                 u4_avg_bit_rate       - bit rate to achieved across the entire
                                         file size
                 u4_peak_bit_rate      - max possible drain rate
                 u4_frame_rate         - number of frames in 1000 seconds
                 u4_intra_frame_interval - num frames between two I frames
                 *au1_init_qp          - init_qp for I,P,B
 *****************************************************************************/
void irc_initialise_rate_control(rate_control_api_t *ps_rate_control_api,
                                 rc_type_e e_rate_control_type,
                                 UWORD8 u1_is_mb_level_rc_on,
                                 UWORD32 u4_avg_bit_rate,
                                 UWORD32 *pu4_peak_bit_rate,
                                 UWORD32 u4_min_bit_rate,
                                 UWORD32 u4_frame_rate,
                                 UWORD32 u4_max_delay,
                                 UWORD32 u4_intra_frame_interval,
                                 WORD32  i4_inter_frm_int,
                                 UWORD8 *pu1_init_qp,
                                 UWORD32 u4_max_vbv_buff_size,
                                 WORD32 i4_max_inter_frm_int,
                                 WORD32 i4_is_gop_closed,
                                 UWORD8 *pu1_min_max_qp,
                                 WORD32 i4_use_est_intra_sad,
                                 UWORD32 u4_src_ticks,
                                 UWORD32 u4_tgt_ticks)
{
    WORD32 i;
    UWORD32 u4_frms_in_delay_prd = (u4_frame_rate * u4_max_delay) / 1000000;
    ps_rate_control_api->e_rc_type = e_rate_control_type;
    ps_rate_control_api->u1_is_mb_level_rc_on = u1_is_mb_level_rc_on;

    trace_printf((const WORD8*)"RC type = %d\n", e_rate_control_type);

    /* Set the avg_bitrate_changed flag for each pic_type to 0 */
    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        ps_rate_control_api->au1_avg_bitrate_changed[i] = 0;
    }

    /* Initialize the pic_handling module */
    irc_init_pic_handling(ps_rate_control_api->ps_pic_handling,
                          (WORD32)u4_intra_frame_interval,
                          i4_inter_frm_int, i4_max_inter_frm_int,
                          i4_is_gop_closed);

    /*** Initialize the rate control modules  ***/
    if(ps_rate_control_api->e_rc_type != CONST_QP)
    {
        UWORD32 au4_num_pics_in_delay_prd[MAX_PIC_TYPE];

        /* Initialize the model parameter structures */
        for(i = 0; i < MAX_PIC_TYPE; i++)
        {
            irc_init_frm_rc_rd_model(ps_rate_control_api->aps_rd_model[i],
                                     MAX_FRAMES_MODELLED);
        }

        /* Initialize the buffer mechanism */
        if((ps_rate_control_api->e_rc_type == VBR_STORAGE)
                        || (ps_rate_control_api->e_rc_type
                                        == VBR_STORAGE_DVD_COMP))
        {
            /* Assuming both the peak bit rates are same for a VBR_STORAGE and
             VBR_STORAGE_DVD_COMP */
            if(pu4_peak_bit_rate[0] != pu4_peak_bit_rate[1])
            {
                trace_printf((const WORD8*)"For VBR_STORAGE and VBR_STORAGE_DVD_COMP the peak bit rates should be same\n");
            }
            irc_init_vbr_vbv(ps_rate_control_api->ps_vbr_storage_vbv,
                             (WORD32)pu4_peak_bit_rate[0],
                             (WORD32)u4_frame_rate,
                             (WORD32)u4_max_vbv_buff_size);
        }
        else if(ps_rate_control_api->e_rc_type == CBR_NLDRC)
        {
            UWORD32 u4_avg_bit_rate_copy[MAX_NUM_DRAIN_RATES];
            for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
            {
                u4_avg_bit_rate_copy[i] = u4_avg_bit_rate;
            }
            /* In case of CBR the num pics in delay is ignored */
            for(i = 0; i < MAX_PIC_TYPE; i++)
                au4_num_pics_in_delay_prd[i] = 0;

            irc_init_cbr_buffer(ps_rate_control_api->ps_cbr_buffer,
                                u4_max_delay, u4_frame_rate,
                                (WORD32 *)u4_avg_bit_rate_copy,
                                au4_num_pics_in_delay_prd,
                                u4_max_vbv_buff_size);
        }
        else if(ps_rate_control_api->e_rc_type == VBR_STREAMING)
        {
            irc_init_vbv_str_prms(&ps_rate_control_api->s_vbr_str_prms,
                                  u4_intra_frame_interval, u4_src_ticks,
                                  u4_tgt_ticks, u4_frms_in_delay_prd);

            /* Get the number of pics of each type in delay period */
            irc_get_vsp_num_pics_in_dly_prd(
                            &ps_rate_control_api->s_vbr_str_prms,
                            au4_num_pics_in_delay_prd);

            irc_init_cbr_buffer(ps_rate_control_api->ps_cbr_buffer,
                                u4_max_delay, u4_frame_rate,
                                (WORD32 *)pu4_peak_bit_rate,
                                au4_num_pics_in_delay_prd,
                                u4_max_vbv_buff_size);
        }

        /* Initialize the SAD estimation module */
        irc_init_est_sad(ps_rate_control_api->ps_est_sad, i4_use_est_intra_sad);

        /* Initialize the bit allocation module according to VBR or CBR */
        if((ps_rate_control_api->e_rc_type == VBR_STORAGE)
                        || (ps_rate_control_api->e_rc_type == VBR_STREAMING)
                        || (ps_rate_control_api->e_rc_type
                                        == VBR_STORAGE_DVD_COMP))
        {
            irc_ba_init_bit_allocation(ps_rate_control_api->ps_bit_allocation,
                                       ps_rate_control_api->ps_pic_handling,
                                       VBR_BIT_ALLOC_PERIOD, u4_avg_bit_rate,
                                       u4_frame_rate,
                                       (WORD32 *)pu4_peak_bit_rate,
                                       u4_min_bit_rate);
        }
        else if(ps_rate_control_api->e_rc_type == CBR_NLDRC)
        {
            irc_ba_init_bit_allocation(ps_rate_control_api->ps_bit_allocation,
                                       ps_rate_control_api->ps_pic_handling,
                                       CBR_BIT_ALLOC_PERIOD, u4_avg_bit_rate,
                                       u4_frame_rate,
                                       (WORD32 *)pu4_peak_bit_rate,
                                       u4_min_bit_rate);
        }

        /*
         * u1_scd_detected will be initialized to 1 when a Scene change is
         * detected
         */
        ps_rate_control_api->u1_scd_detected = 0;
    }

    /* Initialize the init_qp */
    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        ps_rate_control_api->au1_init_qp[i] = pu1_init_qp[i];
        ps_rate_control_api->au1_prev_frm_qp[i] = pu1_init_qp[i];
        ps_rate_control_api->au1_min_max_qp[(i << 1)] =
                        pu1_min_max_qp[(i << 1)];
        ps_rate_control_api->au1_min_max_qp[(i << 1) + 1] = pu1_min_max_qp[(i
                        << 1) + 1];
    }

    /* Initialize the is_first_frm_encoded */
    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        ps_rate_control_api->au1_is_first_frm_coded[i] = 0;
    }
    ps_rate_control_api->u1_is_first_frm = 1;

    /*
     * Control flag for delayed impact after a change in peak bitrate has been
     * made
     */
    ps_rate_control_api->u4_frms_in_delay_prd_for_peak_bit_rate_change = 0;
    for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
    {
        ps_rate_control_api->au4_new_peak_bit_rate[i] = pu4_peak_bit_rate[i];
    }

    /* Initialize the mb level rate control module */
    irc_init_mb_level_rc(ps_rate_control_api->ps_mb_rate_control);
    ps_rate_control_api->i4_prev_frm_est_bits = u4_avg_bit_rate * 1000
                    / u4_frame_rate;

    ps_rate_control_api->prev_ref_pic_type = I_PIC;
}

/******************************************************************************
 *Description   : calls irc_add_pic_to_stack
 ******************************************************************************/
void irc_add_picture_to_stack(rate_control_api_t *rate_control_api,
                              WORD32 i4_enc_pic_id)
{
    /* Call the routine to add the pic to stack in encode order */
    irc_add_pic_to_stack(rate_control_api->ps_pic_handling, i4_enc_pic_id);
}

void irc_add_picture_to_stack_re_enc(rate_control_api_t *rate_control_api,
                                     WORD32 i4_enc_pic_id,
                                     picture_type_e e_pic_type)
{
    /*
     * In case of a re-encoder, the pics will come in the encode order itself.
     * So, there is no need to buffer the pics up
     */
    irc_add_pic_to_stack_re_enc(rate_control_api->ps_pic_handling,
                                i4_enc_pic_id, e_pic_type);
}

/*******************************************************************************
 Description   : Decides the picture type based on the state
 ******************************************************************************/
void irc_get_picture_details(rate_control_handle rate_control_api,
                             WORD32 *pi4_pic_id,
                             WORD32 *pi4_pic_disp_order_no,
                             picture_type_e *pe_pic_type)
{
    /* Call to get the pic_details */
    irc_get_pic_from_stack(rate_control_api->ps_pic_handling, pi4_pic_id,
                           pi4_pic_disp_order_no, pe_pic_type);
}

/*******************************************************************************
 *  Description   : Gets the frame level qp for the given picture type
 ******************************************************************************/
UWORD8 irc_get_frame_level_qp(rate_control_api_t *ps_rate_control_api,
                              picture_type_e e_pic_type,
                              WORD32 i4_ud_max_bits)
{
    UWORD8 u1_frame_qp, i;

    if((ps_rate_control_api->e_rc_type != VBR_STORAGE)
                    && (ps_rate_control_api->e_rc_type != VBR_STORAGE_DVD_COMP)
                    && (ps_rate_control_api->e_rc_type != CBR_NLDRC)
                    && (ps_rate_control_api->e_rc_type != CONST_QP)
                    && (ps_rate_control_api->e_rc_type != VBR_STREAMING))
    {
        trace_printf((const WORD8*)(const WORD8*)" Only VBR,NLDRC and CONST QP supported for now \n");
        return (0);
    }

    if(ps_rate_control_api->e_rc_type != CONST_QP)
    {
        UWORD8 u1_is_first_frm_coded = 1;

        /* Check whether at least one frame of a each picture type gets encoded*/
        /* Check whether it is an IPP or IPB kind of encoding */
        if((ps_rate_control_api->au1_is_first_frm_coded[I_PIC]
                        && ps_rate_control_api->au1_is_first_frm_coded[P_PIC])
                        || ((irc_pic_type_get_intra_frame_interval(
                                        ps_rate_control_api->ps_pic_handling)
                                        == 1)
                                        && (ps_rate_control_api->au1_is_first_frm_coded[I_PIC])))
        {
            if(e_pic_type != B_PIC)
                u1_is_first_frm_coded = 1;
            else
            {
                for(i = 0; i < MAX_PIC_TYPE; i++)
                {
                    u1_is_first_frm_coded &=
                                    ps_rate_control_api->au1_is_first_frm_coded[i];
                }
            }
        }
        else
        {
            u1_is_first_frm_coded = 0;
        }

        if(u1_is_first_frm_coded)
        {
            WORD32 i4_cur_est_texture_bits, i4_cur_est_header_bits;
            WORD32 i4_cur_est_bits;
            UWORD32 u4_estimated_sad;

            /* Force I frame updation of rem_bits_in_frame*/
            if(irc_get_forced_I_frame_cur_frm_flag(
                            ps_rate_control_api->ps_pic_handling) == 1)
            {
                irc_ba_change_rem_bits_in_prd_at_force_I_frame(
                                ps_rate_control_api->ps_bit_allocation,
                                ps_rate_control_api->ps_pic_handling);
                irc_reset_forced_I_frame_cur_frm_flag(
                                ps_rate_control_api->ps_pic_handling);
            }

            /* Get the estimated texture bits allocated for the current frame*/
            i4_cur_est_texture_bits = irc_ba_get_cur_frm_est_texture_bits(
                            ps_rate_control_api->ps_bit_allocation,
                            ps_rate_control_api->aps_rd_model,
                            ps_rate_control_api->ps_est_sad,
                            ps_rate_control_api->ps_pic_handling, e_pic_type);

            /* Get the estimated header bits*/
            i4_cur_est_header_bits = irc_ba_get_cur_frm_est_header_bits(
                            ps_rate_control_api->ps_bit_allocation, e_pic_type);

            /* Total estimated bits */
            i4_cur_est_bits = i4_cur_est_header_bits + i4_cur_est_texture_bits;

            trace_printf((const WORD8*)"ft %d, etb = %d, eb %d, ", e_pic_type,
                         i4_cur_est_texture_bits, i4_cur_est_bits);

            /* Threshold the estimated bits based on the buffer fullness*/
            if(ps_rate_control_api->e_rc_type == VBR_STORAGE)
            {
                WORD32 i4_cur_frm_max_bit_possible;
                i4_cur_frm_max_bit_possible = irc_get_max_target_bits(
                                ps_rate_control_api->ps_vbr_storage_vbv);

                if(i4_cur_est_bits > i4_cur_frm_max_bit_possible)
                {
                    /* Assuming header would consume the same amount of bits */
                    i4_cur_est_texture_bits = i4_cur_frm_max_bit_possible
                                    - i4_cur_est_header_bits;
                }
            }
            else if(ps_rate_control_api->e_rc_type == VBR_STORAGE_DVD_COMP)
            {
                WORD32 i4_rem_bits_in_gop, i4_rem_frms_in_gop, i;
                WORD32 i4_cur_frm_max_bit_possible,
                                ai4_rem_frms_in_gop[MAX_PIC_TYPE];
                irc_pic_type_get_rem_frms_in_gop(
                                ps_rate_control_api->ps_pic_handling,
                                ai4_rem_frms_in_gop);
                i4_rem_bits_in_gop = irc_get_rem_bits_in_period(
                                ps_rate_control_api);
                i4_rem_frms_in_gop = 0;
                for(i = 0; i < MAX_PIC_TYPE; i++)
                    i4_rem_frms_in_gop += ai4_rem_frms_in_gop[i];

                /* Threshold the bits based on estimated buffer fullness */
                i4_cur_frm_max_bit_possible = irc_get_max_tgt_bits_dvd_comp(
                                ps_rate_control_api->ps_vbr_storage_vbv,
                                i4_rem_bits_in_gop, i4_rem_frms_in_gop,
                                e_pic_type);

                if(i4_cur_est_bits > i4_cur_frm_max_bit_possible)
                {
                    /* Assuming header would consume the same amount of bits */
                    i4_cur_est_texture_bits = i4_cur_frm_max_bit_possible
                                    - i4_cur_est_header_bits;

                }
            }
            else if(ps_rate_control_api->e_rc_type == CBR_NLDRC)
            {
                WORD32 i4_cur_frm_bits_acc_buffer =
                                irc_cbr_buffer_constraint_check(
                                                ps_rate_control_api->ps_cbr_buffer,
                                                i4_cur_est_bits, e_pic_type);

                /* Assuming the header would consume the same amount of bits */
                i4_cur_est_texture_bits = i4_cur_frm_bits_acc_buffer
                                - i4_cur_est_header_bits;

            }
            else if(ps_rate_control_api->e_rc_type == VBR_STREAMING)
            {
                WORD32 i4_cur_frm_bits_acc_buffer =
                                irc_vbr_stream_buffer_constraint_check(
                                                ps_rate_control_api->ps_cbr_buffer,
                                                i4_cur_est_bits, e_pic_type);

                /* Assuming the header would consume the same amount of bits */
                i4_cur_est_texture_bits = i4_cur_frm_bits_acc_buffer
                                - i4_cur_est_header_bits;
            }

            trace_printf((const WORD8*)"emtb = %d, ", i4_cur_est_texture_bits);

            /*
             * If the estimated texture bits go to values less than zero
             * due to buffer underflow, make the estimated target bits to go
             * to zero
             */
            if(i4_cur_est_texture_bits < 0)
                i4_cur_est_texture_bits = 0;

            ps_rate_control_api->i4_prev_frm_est_bits = (i4_cur_est_texture_bits
                            + i4_cur_est_header_bits);

            /* Clip est_texture_bits according to the user-defined max value */
            if((i4_cur_est_texture_bits
                            > (i4_ud_max_bits - i4_cur_est_header_bits))
                            && (e_pic_type != I_PIC))
            {
                i4_cur_est_texture_bits = (i4_ud_max_bits
                                - i4_cur_est_header_bits);
                trace_printf((const WORD8*)"udcb = %d, ",
                             i4_ud_max_bits - i4_cur_est_header_bits);
            }

            /* Calculate the estimated SAD for corresponding frame*/
            u4_estimated_sad = irc_get_est_sad(ps_rate_control_api->ps_est_sad,
                                               e_pic_type);

            /* Query the model for the Qp for the corresponding frame*/

            /*
             * The check is because the model gives a negative QP when the
             * i4_cur_est_texture_bits is less than or equal to 0
             * [This is a bug in the model]. As a temporary fix, the frame QP
             * is being set to the max QP allowed
             */
            if(i4_cur_est_texture_bits > 0)
            {
                u1_frame_qp = irc_find_qp_for_target_bits(
                                ps_rate_control_api->aps_rd_model[e_pic_type],
                                i4_cur_est_texture_bits,
                                u4_estimated_sad,
                                ps_rate_control_api->au1_min_max_qp[(e_pic_type
                                                << 1)],
                                ps_rate_control_api->au1_min_max_qp[(e_pic_type
                                                << 1) + 1]);
            }
            else
            {
                u1_frame_qp = ps_rate_control_api->au1_min_max_qp[(e_pic_type
                                << 1) + 1];
            }

            trace_printf((const WORD8*)"ehb %d, etb %d, fqp %d, es %d, eb %d, ",
                         i4_cur_est_header_bits, i4_cur_est_texture_bits,
                         u1_frame_qp, u4_estimated_sad, i4_cur_est_bits);

            /* Restricting the QP swing if the average bit rate has changed */
            if(ps_rate_control_api->au1_avg_bitrate_changed[e_pic_type] == 0)
            {
                WORD32 prev_qp;
                WORD32 hi_dev_qp, lo_dev_qp;
                /* Restricting the qp swing */
                prev_qp = ps_rate_control_api->au1_prev_frm_qp[ps_rate_control_api->prev_ref_pic_type];

                if(ps_rate_control_api->prev_ref_pic_type != e_pic_type)
                {
                    if(e_pic_type == I_PIC)
                    {
                        /*
                         * Constrain I-frame QP to be within specified limit of
                         * prev_ref_qp/Kp
                         */
                        prev_qp = (P_TO_I_RATIO * prev_qp + (1 << (K_Q - 1)))
                                        >> (K_Q);
                    }
                    else if(e_pic_type == P_PIC)
                    {
                        /*
                         * Constrain P-frame QP to be within specified limit of
                         * Kp*prev_ref_qp
                         */
                        prev_qp = (I_TO_P_RATIO * prev_qp + (1 << (K_Q - 1)))
                                        >> (K_Q);
                    }
                    else if(ps_rate_control_api->prev_ref_pic_type == P_PIC)
                    {
                        /* current frame is B-pic */
                        /* Constrain B-frame QP to be within specified limit of
                         * prev_ref_qp/Kb
                         */
                        prev_qp = (P_TO_B_RATIO * prev_qp + (1 << (K_Q - 1)))
                                        >> (K_Q);
                    }
                    else /* if(ps_rate_control_api->prev_ref_pic_type == I_PIC*/
                    {
                        /* current frame is B-pic */
                        /*
                         * Constrain B-frame QP to be within specified limit of
                         * prev_ref_qp/Kb
                         */
                        prev_qp = (P_TO_B_RATIO * I_TO_P_RATIO * prev_qp
                                        + (1 << (K_Q + K_Q - 1)))
                                        >> (K_Q + K_Q);
                    }
                }

                /*
                 * Due to the inexact nature of translation tables, QP may
                 * get locked at some values. This is because of the inexactness of
                 * the tables causing a change of +-1 in back and forth translations.
                 * In that case, if we restrict the QP swing to +-1, we will get
                 * the lock up condition. Hence we make it such that we will have
                 * a swing of atleast +- 2 from prev_qp
                 */

                lo_dev_qp = GET_LO_DEV_QP(prev_qp);
                lo_dev_qp = MIN(lo_dev_qp, prev_qp - 2);
                lo_dev_qp = MAX(lo_dev_qp, ps_rate_control_api->au1_min_max_qp[(e_pic_type << 1)]);

                hi_dev_qp = GET_HI_DEV_QP(prev_qp);
                hi_dev_qp = MAX(hi_dev_qp, prev_qp + 2);
                hi_dev_qp = MIN(hi_dev_qp, ps_rate_control_api->au1_min_max_qp[(e_pic_type << 1) + 1]);

                u1_frame_qp = (UWORD8)CLIP_QP((WORD32)u1_frame_qp, hi_dev_qp , lo_dev_qp);

            }
            else
            {
                ps_rate_control_api->au1_avg_bitrate_changed[e_pic_type] = 0;
            }
        }
        else
        {
            /*
             * The u1_is_first_frm_coded gets reset
             *  a) at start of sequence
             *  b) whenever there is a scene change.
             *     In both cases since we do not have any estimate about the
             *     current frame, we just send in the previous frame qp value.IN
             *     Scene change case the previous QP is incremented by 4 , This is
             *     done because the Scene changed VOP will have over consumed and
             *     chances of future frames skipping is very high. For the init
             *     case, the previous frame QP is initialized with the init qp
             */
            if((ps_rate_control_api->u1_scd_detected)
                            && (ps_rate_control_api->e_rc_type != CONST_QP))
            {
                /*
                 * If scene change is detected, I frame Qp would have been
                 * updated
                 */
                 /* Use a QP calculated in the prev update fxn */
                u1_frame_qp = ps_rate_control_api->u1_frm_qp_after_scd;
            }
            else
            {
                u1_frame_qp = ps_rate_control_api->au1_prev_frm_qp[e_pic_type];
            }
        }
    }
    else
    {
        u1_frame_qp = ps_rate_control_api->au1_init_qp[e_pic_type];
    }

    trace_printf((const WORD8*)"fqp %d\n", u1_frame_qp);

    return (u1_frame_qp);
}

/*******************************************************************************
 *Function Name : irc_get_buffer_status
 *Description   : Gets the state of VBV buffer
 *Outputs       : 0 = normal, 1 = underflow, 2= overflow
 *Returns       : vbv_buf_status_e
 ******************************************************************************/
vbv_buf_status_e irc_get_buffer_status(rate_control_api_t *ps_rate_control_api,
                                       WORD32 i4_total_frame_bits,
                                       picture_type_e e_pic_type,
                                       WORD32 *pi4_num_bits_to_prevent_vbv_underflow)
{
    vbv_buf_status_e e_buf_status = VBV_NORMAL;

    /* Get the buffer status for the current total consumed bits and error bits*/
    if(ps_rate_control_api->e_rc_type == VBR_STORAGE_DVD_COMP)
    {
        e_buf_status = irc_get_vbv_buffer_status(
                        ps_rate_control_api->ps_vbr_storage_vbv,
                        i4_total_frame_bits,
                        pi4_num_bits_to_prevent_vbv_underflow);

        trace_printf((const WORD8*)"e_buf_status = %d\n", e_buf_status);
    }
    else if(ps_rate_control_api->e_rc_type == VBR_STORAGE)
    {
        /* For VBR case since there is not underflow returning the max value */
        pi4_num_bits_to_prevent_vbv_underflow[0] = irc_get_max_vbv_buf_size(
                        ps_rate_control_api->ps_vbr_storage_vbv);
        e_buf_status = VBV_NORMAL;
    }
    else if(ps_rate_control_api->e_rc_type == CBR_NLDRC)
    {
        e_buf_status = irc_get_cbr_buffer_status(
                        ps_rate_control_api->ps_cbr_buffer, i4_total_frame_bits,
                        pi4_num_bits_to_prevent_vbv_underflow, e_pic_type);

    }
    else if(ps_rate_control_api->e_rc_type == VBR_STREAMING)
    {
        /* For VBR_streaming, error bits are computed according to peak bitrate*/
        e_buf_status = irc_get_cbr_buffer_status(
                        ps_rate_control_api->ps_cbr_buffer, i4_total_frame_bits,
                        pi4_num_bits_to_prevent_vbv_underflow, e_pic_type);
    }
    return e_buf_status;
}

/*******************************************************************************
 Function Name : irc_update_pic_handling_state
 Description   : If the forward path and the backward path of rate control
 ******************************************************************************/
void irc_update_pic_handling_state(rate_control_api_t *ps_rate_control_api,
                                   picture_type_e e_pic_type)
{
    irc_update_pic_handling(ps_rate_control_api->ps_pic_handling, e_pic_type);
}

/******************************************************************************
 Function Name : irc_update_frame_level_info
 Description   : Updates the frame level information into the rate control
                 structure
 ******************************************************************************/
void irc_update_frame_level_info(rate_control_api_t *ps_rate_control_api,
                                 picture_type_e e_pic_type,
                                 WORD32 *pi4_mb_type_sad,
                                 WORD32 i4_total_frame_bits,
                                 WORD32 i4_model_updation_hdr_bits,
                                 WORD32 *pi4_mb_type_tex_bits,
                                 WORD32 *pi4_tot_mb_type_qp,
                                 WORD32 *pi4_tot_mb_in_type,
                                 WORD32 i4_avg_activity,
                                 UWORD8 u1_is_scd,
                                 WORD32 i4_is_it_a_skip,
                                 WORD32 i4_intra_frm_cost,
                                 WORD32 i4_is_pic_handling_done)
{
    UWORD8 u1_num_skips = 0;
    WORD32 i;
    UWORD32 u4_frame_sad = 0;
    WORD32 i4_tot_texture_bits = 0;
    WORD32 i4_tot_mbs = 0;
    WORD32 i4_avg_qp = 0;

    /* SCD not supported in case of IPB encoder */
    if(u1_is_scd && (irc_pic_type_get_inter_frame_interval(
                                    ps_rate_control_api->ps_pic_handling) > 1))
    {
        u1_is_scd = 0;
    }
    trace_printf((const WORD8*)"i4_total_frame_bits %d\n", i4_total_frame_bits);

    if(!i4_is_it_a_skip && !i4_is_pic_handling_done)
    {
        /* Update the pic_handling struct */
        irc_update_pic_handling(ps_rate_control_api->ps_pic_handling,
                                e_pic_type);
    }

    if(ps_rate_control_api->e_rc_type != CONST_QP)
    {
        if(!i4_is_it_a_skip)
        {
            WORD32 i4_new_period_flag;
            /******************************************************************
             Calculate the total values from the individual values
             ******************************************************************/
            for(i = 0; i < MAX_MB_TYPE; i++)
                u4_frame_sad += pi4_mb_type_sad[i];
            for(i = 0; i < MAX_MB_TYPE; i++)
                i4_tot_texture_bits += pi4_mb_type_tex_bits[i];
            for(i = 0; i < MAX_MB_TYPE; i++)
                i4_avg_qp += pi4_tot_mb_type_qp[i];
            for(i = 0; i < MAX_MB_TYPE; i++)
                i4_tot_mbs += pi4_tot_mb_in_type[i];
            i4_avg_qp /= i4_tot_mbs; /* Calculate the average QP */

            if(ps_rate_control_api->u1_is_mb_level_rc_on)
            {
                /*
                 * The model needs to take into consideration the average
                 * activity of the entire frame while estimating the QP. Thus
                 * the frame sad values are scaled by the average activity
                 * before updating it into the model.
                 */
                if(!i4_avg_activity)
                    i4_avg_activity = 1;
                i4_intra_frm_cost *= i4_avg_activity;
                u4_frame_sad *= i4_avg_activity;
            }

            /******************************************************************
             Update the bit allocation module
             NOTE: For bit allocation module, the pic_type should not be
             modified to that of 'I', in case of a SCD.
             ******************************************************************/
            i4_new_period_flag = irc_is_last_frame_in_gop(
                            ps_rate_control_api->ps_pic_handling);
            irc_ba_update_cur_frm_consumed_bits(
                            ps_rate_control_api->ps_bit_allocation,
                            ps_rate_control_api->ps_pic_handling,
                            i4_total_frame_bits, i4_model_updation_hdr_bits,
                            e_pic_type, u1_is_scd, i4_new_period_flag);

            if(1 == i4_new_period_flag
                            && ((ps_rate_control_api->e_rc_type == VBR_STORAGE)
                                            || (ps_rate_control_api->e_rc_type
                                                            == VBR_STORAGE_DVD_COMP)))
            {
                irc_ba_check_and_update_bit_allocation(
                                ps_rate_control_api->ps_bit_allocation,
                                ps_rate_control_api->ps_pic_handling,
                                irc_get_cur_vbv_buf_size(
                                                ps_rate_control_api->ps_vbr_storage_vbv),
                                irc_get_max_vbv_buf_size(
                                                ps_rate_control_api->ps_vbr_storage_vbv),
                                irc_get_max_bits_per_tgt_frm(
                                                ps_rate_control_api->ps_vbr_storage_vbv),
                                i4_total_frame_bits);
            }
        }

        /**********************************************************************
         Update the buffer status
         *********************************************************************/
        /*
         * This update is done after overflow and underflow handling to
         *  account for the actual bits dumped
         */
        if((ps_rate_control_api->e_rc_type == VBR_STORAGE)
                        || (ps_rate_control_api->e_rc_type
                                        == VBR_STORAGE_DVD_COMP))
        {
            irc_update_vbr_vbv(ps_rate_control_api->ps_vbr_storage_vbv,
                               i4_total_frame_bits);
        }
        else if(ps_rate_control_api->e_rc_type == CBR_NLDRC)
        {
            irc_update_cbr_buffer(ps_rate_control_api->ps_cbr_buffer,
                                  i4_total_frame_bits, e_pic_type);
        }
        else if(ps_rate_control_api->e_rc_type == VBR_STREAMING)
        {
            UWORD32 au4_num_pics_in_delay_prd[MAX_PIC_TYPE];

            irc_get_vsp_num_pics_in_dly_prd(
                            &ps_rate_control_api->s_vbr_str_prms,
                            au4_num_pics_in_delay_prd);

            irc_update_cbr_buffer(ps_rate_control_api->ps_cbr_buffer,
                                  i4_total_frame_bits, e_pic_type);

            irc_update_vbr_str_prms(&ps_rate_control_api->s_vbr_str_prms,
                                    e_pic_type);

            irc_change_cbr_vbv_num_pics_in_delay_period(
                            ps_rate_control_api->ps_cbr_buffer,
                            au4_num_pics_in_delay_prd);

            /*
             * If the change_in_peak_bitrate flag is set, after the delay period
             * update the peak_bitrate and the buffer parameters
             */
            if(!ps_rate_control_api->u4_frms_in_delay_prd_for_peak_bit_rate_change)
            {
                irc_ba_change_ba_peak_bit_rate(
                                ps_rate_control_api->ps_bit_allocation,
                                (WORD32 *)&ps_rate_control_api->au4_new_peak_bit_rate[0]);
                irc_change_cbr_vbv_bit_rate(
                                ps_rate_control_api->ps_cbr_buffer,
                                (WORD32 *)&ps_rate_control_api->au4_new_peak_bit_rate[0]);
            }
            if(ps_rate_control_api->u4_frms_in_delay_prd_for_peak_bit_rate_change)
                ps_rate_control_api->u4_frms_in_delay_prd_for_peak_bit_rate_change--;
        }

        if(!i4_is_it_a_skip)
        {
            /*******************************************************************
             Handle the SCENE CHANGE DETECTED
             1) Make the picture type as I, so that updation happens as if it is
                an I frame
             2) Reset model, SAD and flag to restart the estimation process
             ******************************************************************/
            if(u1_is_scd)
            {
                WORD32 i4_frm_qp_after_scd;
                UWORD32 u4_prev_I_frm_sad;

                e_pic_type = I_PIC;

                /* Scale scd qp based on SCD Frm sad and previous I Frm sad */
                /* frm_qp_after_scd = (avg_qp * cur_frm_sad)/prev_I_frm_sad */

                /*
                 * QP for the next frame should take care of
                 * 1) due to scene change, the current picture has consumed more
                 *      bits
                 * 2) relative complexity of the previous scene and the current
                 *     scene
                 */

                /* Get the intra SAD for the previous scene */
                u4_prev_I_frm_sad = irc_get_est_sad(
                                ps_rate_control_api->ps_est_sad, I_PIC);

                /*
                 * Scale the QP based on the SAD ratio of the current pic and
                 * previous scene intra SAD
                 */
                X_PROD_Y_DIV_Z(i4_avg_qp, u4_frame_sad, u4_prev_I_frm_sad,
                               i4_frm_qp_after_scd);

                /* Limit the next frame qp by 50% across both the sides */
                if(i4_frm_qp_after_scd > ((i4_avg_qp * 3) >> 1))
                {
                    i4_frm_qp_after_scd = (i4_avg_qp * 3) >> 1;
                }
                else if(i4_frm_qp_after_scd < (i4_avg_qp >> 1))
                {
                    i4_frm_qp_after_scd = (i4_avg_qp >> 1);
                }

                /*
                 * Ensure that the next frame QP is within the min_max limit of
                 * QP allowed
                 */
                if(i4_frm_qp_after_scd
                                > ps_rate_control_api->au1_min_max_qp[(e_pic_type
                                                << 1) + 1])
                {
                    i4_frm_qp_after_scd =
                                    ps_rate_control_api->au1_min_max_qp[(e_pic_type
                                                    << 1) + 1];
                }
                else if(i4_frm_qp_after_scd
                                < ps_rate_control_api->au1_min_max_qp[(e_pic_type
                                                << 1)])
                {
                    i4_frm_qp_after_scd =
                                    ps_rate_control_api->au1_min_max_qp[(e_pic_type
                                                    << 1)];
                }

                /* Update the state var */
                ps_rate_control_api->u1_frm_qp_after_scd =
                                (UWORD8)i4_frm_qp_after_scd;

                /* re-set model */
                for(i = 0; i < MAX_PIC_TYPE; i++)
                {
                    irc_reset_frm_rc_rd_model(
                                    ps_rate_control_api->aps_rd_model[i]);
                }

                /* Reset the SAD estimation module */
                irc_reset_est_sad(ps_rate_control_api->ps_est_sad);

                /* Reset flag */
                for(i = 0; i < MAX_PIC_TYPE; i++)
                {
                    ps_rate_control_api->au1_is_first_frm_coded[i] = 0;
                }

                /* Reset the MB Rate control */
                irc_init_mb_level_rc(ps_rate_control_api->ps_mb_rate_control);

                /*Set u1_scd_detected flag*/
                ps_rate_control_api->u1_scd_detected = 1;

                /*
                 * Adjust the average QP for the frame based on bits
                 * consumption
                 */
                /*
                 *  Initialize the QP for each picture type according to the
                 * average QP of the SCD pic
                 */
                ps_rate_control_api->au1_prev_frm_qp[I_PIC] = (UWORD8)i4_avg_qp;

                trace_printf((const WORD8*)"SCD DETECTED\n");
            }
            else
            {
                ps_rate_control_api->u1_scd_detected = 0;
                /**************************************************************
                 Update the Qp used by the current frame
                 **************************************************************/
                ps_rate_control_api->au1_prev_frm_qp[e_pic_type] =
                                (UWORD8)i4_avg_qp;
            }

            /********************************************************************
             Update the model of the correponding picture type
             NOTE: For SCD, we force the frame type from 'P' to that of a 'I'
             ******************************************************************/
            /*
             * For very simple sequences no bits are consumed by texture. These
             * frames do not add any information to the model and so not added
             */
            if(i4_tot_texture_bits && u4_frame_sad)
            {
                irc_add_frame_to_rd_model(
                                ps_rate_control_api->aps_rd_model[e_pic_type],
                                i4_tot_texture_bits, (UWORD8)i4_avg_qp,
                                u4_frame_sad, u1_num_skips);

                /*
                 * At least one proper frame in added into the model. Until that
                 * keep using the initial QP
                 */
                ps_rate_control_api->au1_is_first_frm_coded[e_pic_type] = 1;
            }

            if(i4_avg_activity)
            {
                /* Update the mb_level model */
                irc_mb_update_frame_level(
                                ps_rate_control_api->ps_mb_rate_control,
                                i4_avg_activity);
            }

            /******************************************************************
             Update the sad estimation module
             NOTE: For SCD, we force the frame type from 'P' to that of a 'I'
             ******************************************************************/
            if(u4_frame_sad)
            {
                irc_update_actual_sad(ps_rate_control_api->ps_est_sad,
                                      u4_frame_sad, e_pic_type);

                irc_update_actual_sad_for_intra(ps_rate_control_api->ps_est_sad,
                                                i4_intra_frm_cost);
            }

            /*
             * Update the variable which denotes that a frame has been
             * encountered
             */
            ps_rate_control_api->u1_is_first_frm = 0;

        }
    }

    /* Store the prev encoded picture type for restricting Qp swing */
    if((e_pic_type == I_PIC) || (e_pic_type == P_PIC))
    {
        ps_rate_control_api->prev_ref_pic_type = e_pic_type;
    }

    trace_printf((const WORD8*)"ft %d,hb %d,tb %d,qp %d,fs %d\n", e_pic_type,
                 i4_model_updation_hdr_bits, i4_tot_texture_bits, i4_avg_qp,
                 u4_frame_sad);

    return;
}

/*******************************************************************************
 MB Level API functions
 ******************************************************************************/

/******************************************************************************
 Function Name : irc_init_mb_rc_frame_level
 Description   : Initialise the frame level details required for a mb level
 ******************************************************************************/

void irc_init_mb_rc_frame_level(rate_control_api_t *ps_rate_control_api,
                                UWORD8 u1_frame_qp)
{
    irc_mb_init_frame_level(ps_rate_control_api->ps_mb_rate_control,
                            u1_frame_qp);
}

/******************************************************************************
 Function Name : irc_get_mb_level_qp
 Description   : Get the mb level qp
 *****************************************************************************/
void irc_get_mb_level_qp(rate_control_api_t *ps_rate_control_api,
                         WORD32 i4_cur_mb_activity,
                         WORD32 *pi4_mb_qp,
                         picture_type_e e_pic_type)
{
    if(ps_rate_control_api->u1_is_mb_level_rc_on)
    {
        irc_get_mb_qp(ps_rate_control_api->ps_mb_rate_control,
                      i4_cur_mb_activity, pi4_mb_qp);

        /* Truncating the QP to the Max and Min Qp values possible */
        if(pi4_mb_qp[1] < ps_rate_control_api->au1_min_max_qp[e_pic_type << 1])
        {
            pi4_mb_qp[1] = ps_rate_control_api->au1_min_max_qp[e_pic_type << 1];
        }
        if(pi4_mb_qp[1]
                        > ps_rate_control_api->au1_min_max_qp[(e_pic_type << 1)
                                        + 1])
        {
            pi4_mb_qp[1] = ps_rate_control_api->au1_min_max_qp[(e_pic_type << 1)
                            + 1];
        }
    }
    else
    {
        WORD32 i4_qp;
        i4_qp = irc_get_frm_level_qp(ps_rate_control_api->ps_mb_rate_control);
        /* Both the qp are used for */
        pi4_mb_qp[0] = i4_qp; /* Used as feedback for the rate control */
        pi4_mb_qp[1] = i4_qp; /* Used for quantising the MB*/
    }
}

/****************************************************************************
 Function Name : irc_get_bits_to_stuff
 Description   : Gets the bits to stuff to prevent Underflow of Encoder Buffer
 *****************************************************************************/
WORD32 irc_get_bits_to_stuff(rate_control_api_t *ps_rate_control_api,
                             WORD32 i4_tot_consumed_bits,
                             picture_type_e e_pic_type)
{
    WORD32 i4_bits_to_stuff;
    /* Get the CBR bits to stuff*/
    i4_bits_to_stuff = irc_get_cbr_bits_to_stuff(
                    ps_rate_control_api->ps_cbr_buffer, i4_tot_consumed_bits,
                    e_pic_type);
    return i4_bits_to_stuff;
}

/****************************************************************************
 Function Name : irc_get_prev_frm_est_bits
 Description   : Returns previous frame estimated bits
 *****************************************************************************/
WORD32 irc_get_prev_frm_est_bits(rate_control_api_t *ps_rate_control_api)
{
    return (ps_rate_control_api->i4_prev_frm_est_bits);
}

/******************************************************************************
 Control Level API functions
 Logic: The control call sets the state structure of the rate control api
         accordingly such that the next process call would implement the same.
 ******************************************************************************/

void irc_change_inter_frm_int_call(rate_control_api_t *ps_rate_control_api,
                                   WORD32 i4_inter_frm_int)
{
    irc_pic_handling_register_new_inter_frm_interval(
                    ps_rate_control_api->ps_pic_handling, i4_inter_frm_int);
}

void irc_change_intra_frm_int_call(rate_control_api_t *ps_rate_control_api,
                                   WORD32 i4_intra_frm_int)
{
    irc_pic_handling_register_new_int_frm_interval(
                    ps_rate_control_api->ps_pic_handling, i4_intra_frm_int);

    if(ps_rate_control_api->e_rc_type == VBR_STREAMING)
    {
        irc_change_vsp_ifi(&ps_rate_control_api->s_vbr_str_prms,
                           i4_intra_frm_int);
    }
}

/****************************************************************************
 Function Name : irc_change_avg_bit_rate
 Description   : Whenever the average bit rate changes, the excess bits is
                 between the changed bit rate and the old one is re-distributed
                 in the bit allocation module
 *****************************************************************************/
void irc_change_avg_bit_rate(rate_control_api_t *ps_rate_control_api,
                             UWORD32 u4_average_bit_rate)
{
    int i;
    if(ps_rate_control_api->e_rc_type != CONST_QP)
    {
        /*
         * Bit Allocation Module: distribute the excess/deficit bits between the
         * old and the new frame rate to all the remaining frames
         */
        irc_ba_change_remaining_bits_in_period(
                        ps_rate_control_api->ps_bit_allocation,
                        ps_rate_control_api->ps_pic_handling,
                        u4_average_bit_rate,
                        irc_ba_get_frame_rate(
                                        ps_rate_control_api->ps_bit_allocation),
                        (WORD32 *)(ps_rate_control_api->au4_new_peak_bit_rate));
    }
    if(ps_rate_control_api->e_rc_type == CBR_NLDRC)
    {
        UWORD32 u4_average_bit_rate_copy[MAX_NUM_DRAIN_RATES];
        for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
        {
            u4_average_bit_rate_copy[i] = u4_average_bit_rate;
        }
        irc_change_cbr_vbv_bit_rate(ps_rate_control_api->ps_cbr_buffer,
                                    (WORD32 *)(u4_average_bit_rate_copy));
    }

    /*
     * This is done only for average bitrate changing somewhere after the model
     * stabilizes.Here it is assumed that user will not do this call after
     * first few frames. If we dont have this check, what would happen is since
     * the model has not stabilized, also bitrate has changed before the first
     * frame, we dont restrict the qp. Qp can go to very bad values after init
     * qp since if swing is disabled.
     * This check will become buggy if change bitrate is called say somewhere
     * after first two frames.Bottom line - RC init is done during create and
     * this call is done just before first process.And we want to differentiate
     * between this call done before first process and the call which is done
     * during run time
     */
    if(ps_rate_control_api->u1_is_first_frm == 0)
    {
        for(i = 0; i < MAX_PIC_TYPE; i++)
        {
            ps_rate_control_api->au1_avg_bitrate_changed[i] = 1;
        }
    }
}

/****************************************************************************
 Function Name : irc_change_frame_rate
 Description   : Does the necessary changes whenever there is a change in
                 frame rate
 *****************************************************************************/
void irc_change_frame_rate(rate_control_api_t *ps_rate_control_api,
                           UWORD32 u4_frame_rate,
                           UWORD32 u4_src_ticks,
                           UWORD32 u4_tgt_ticks)
{

    if(ps_rate_control_api->e_rc_type != CONST_QP)
    {
        UWORD32 u4_frms_in_delay_prd = ((u4_frame_rate
                        * irc_get_cbr_buffer_delay(
                                        ps_rate_control_api->ps_cbr_buffer))
                        / 1000000);
        if((ps_rate_control_api->e_rc_type == VBR_STORAGE)
                        || (ps_rate_control_api->e_rc_type
                                        == VBR_STORAGE_DVD_COMP))
        {
            irc_change_vbr_vbv_frame_rate(
                            ps_rate_control_api->ps_vbr_storage_vbv,
                            u4_frame_rate);
        }
        else if(ps_rate_control_api->e_rc_type == CBR_NLDRC)
        {
            irc_change_cbr_vbv_tgt_frame_rate(
                            ps_rate_control_api->ps_cbr_buffer, u4_frame_rate);
        }
        else if(ps_rate_control_api->e_rc_type == VBR_STREAMING)
        {
            UWORD32 au4_num_pics_in_delay_prd[MAX_PIC_TYPE];
            irc_change_vsp_tgt_ticks(&ps_rate_control_api->s_vbr_str_prms,
                                     u4_tgt_ticks);
            irc_change_vsp_src_ticks(&ps_rate_control_api->s_vbr_str_prms,
                                     u4_src_ticks);
            irc_change_vsp_fidp(&ps_rate_control_api->s_vbr_str_prms,
                                u4_frms_in_delay_prd);

            irc_get_vsp_num_pics_in_dly_prd(
                            &ps_rate_control_api->s_vbr_str_prms,
                            au4_num_pics_in_delay_prd);
            irc_change_cbr_vbv_tgt_frame_rate(
                            ps_rate_control_api->ps_cbr_buffer, u4_frame_rate);
            irc_change_cbr_vbv_num_pics_in_delay_period(
                            ps_rate_control_api->ps_cbr_buffer,
                            au4_num_pics_in_delay_prd);
        }

        /*
         * Bit Allocation Module: distribute the excess/deficit bits between the
         * old and the new frame rate to all the remaining frames
         */
        irc_ba_change_remaining_bits_in_period(
                        ps_rate_control_api->ps_bit_allocation,
                        ps_rate_control_api->ps_pic_handling,
                        irc_ba_get_bit_rate(
                                        ps_rate_control_api->ps_bit_allocation),
                        u4_frame_rate,
                        (WORD32 *)(ps_rate_control_api->au4_new_peak_bit_rate));
    }
}

/****************************************************************************
 Function Name : irc_change_frm_rate_for_bit_alloc
 Description   : Does the necessary changes only in the bit_allocation module
                 there is a change in frame rate
 *****************************************************************************/
void irc_change_frm_rate_for_bit_alloc(rate_control_api_t *ps_rate_control_api,
                                       UWORD32 u4_frame_rate)
{

    if(ps_rate_control_api->e_rc_type != CONST_QP)
    {
        /*
         * Bit Allocation Module: distribute the excess/deficit bits between the
         * old and the new frame rate to all the remaining frames
         */
        irc_ba_change_remaining_bits_in_period(
                        ps_rate_control_api->ps_bit_allocation,
                        ps_rate_control_api->ps_pic_handling,
                        irc_ba_get_bit_rate(
                                        ps_rate_control_api->ps_bit_allocation),
                        u4_frame_rate,
                        (WORD32 *)(ps_rate_control_api->au4_new_peak_bit_rate));

        if(ps_rate_control_api->e_rc_type == VBR_STORAGE
                        || ps_rate_control_api->e_rc_type
                                        == VBR_STORAGE_DVD_COMP)
        {
            irc_change_vbr_max_bits_per_tgt_frm(
                            ps_rate_control_api->ps_vbr_storage_vbv,
                            u4_frame_rate);
        }
    }
}

void irc_change_init_qp(rate_control_api_t *ps_rate_control_api,
                        UWORD8 *pu1_init_qp)
{
    WORD32 i;
    /* Initialize the init_qp */
    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        ps_rate_control_api->au1_init_qp[i] = pu1_init_qp[i];
        ps_rate_control_api->au1_prev_frm_qp[i] = pu1_init_qp[i];
    }
}

void irc_change_min_max_qp(rate_control_api_t *ps_rate_control_api,
                           UWORD8 *pu1_min_max_qp)
{
    WORD32 i;
    for(i = 0; i < MAX_PIC_TYPE; i++)
    {
        ps_rate_control_api->au1_min_max_qp[(i << 1)] =
                        pu1_min_max_qp[(i << 1)];
        ps_rate_control_api->au1_min_max_qp[(i << 1) + 1] = pu1_min_max_qp[(i
                        << 1) + 1];
    }
}

/****************************************************************************
 Function Name : irc_change_peak_bit_rate
 Description   : Does the necessary changes whenever there is a change in
                 peak bit rate
 *****************************************************************************/
WORD32 irc_change_peak_bit_rate(rate_control_api_t *ps_rate_control_api,
                                UWORD32 *pu4_peak_bit_rate)
{
    WORD32 i4_ret_val = RC_OK;
    int i;

    /*
     * Buffer Mechanism Module: Re-initialize the number of bits consumed per
     * frame
     */
    if(ps_rate_control_api->e_rc_type == VBR_STORAGE
                    || ps_rate_control_api->e_rc_type == VBR_STORAGE_DVD_COMP)
    {
        /* Send the new peak bit rate and the old frame rate */
        irc_change_vbr_vbv_bit_rate(ps_rate_control_api->ps_vbr_storage_vbv,
                                    pu4_peak_bit_rate[0]);
        irc_ba_change_ba_peak_bit_rate(ps_rate_control_api->ps_bit_allocation,
                                       (WORD32 *)pu4_peak_bit_rate);

        for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
        {
            ps_rate_control_api->au4_new_peak_bit_rate[i] =
                            pu4_peak_bit_rate[i];
        }
    }
    else if(ps_rate_control_api->e_rc_type == VBR_STREAMING)
    {
        if(ps_rate_control_api->u4_frms_in_delay_prd_for_peak_bit_rate_change)
        {
            /*
             * Means that change in peak bit rate has been made twice before the
             * previous change could take effect
             */
            i4_ret_val = RC_BENIGN_ERR;
        }
        /*
         * If the change happens before encoding the first frame make the
         * effect immediately else delay the effect
         */
        if(ps_rate_control_api->u1_is_first_frm)
        {
            for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
            {
                ps_rate_control_api->au4_new_peak_bit_rate[i] =
                                pu4_peak_bit_rate[i];
            }
            irc_ba_change_ba_peak_bit_rate(
                            ps_rate_control_api->ps_bit_allocation,
                            (WORD32 *)pu4_peak_bit_rate);
            irc_change_cbr_vbv_bit_rate(ps_rate_control_api->ps_cbr_buffer,
                                        (WORD32 *)pu4_peak_bit_rate);
        }
        else
        {
            UWORD32 au4_num_pics_in_delay_prd[MAX_NUM_DRAIN_RATES];
            /*
             * Else store the number of frames after which the effect should
             * happen and then update the peak bitrate
             */
            ps_rate_control_api->u4_frms_in_delay_prd_for_peak_bit_rate_change =
                            irc_get_vsp_num_pics_in_dly_prd(
                                            &ps_rate_control_api->s_vbr_str_prms,
                                            au4_num_pics_in_delay_prd);
            for(i = 0; i < MAX_NUM_DRAIN_RATES; i++)
            {
                ps_rate_control_api->au4_new_peak_bit_rate[i] =
                                pu4_peak_bit_rate[i];
            }
        }
    }

    return (i4_ret_val);
}

void irc_change_buffer_delay(rate_control_api_t *ps_rate_control_api,
                             UWORD32 u4_buffer_delay)
{
    UWORD32 u4_frms_in_delay_prd = ((irc_ba_get_frame_rate(
                    ps_rate_control_api->ps_bit_allocation) * u4_buffer_delay)
                    / 1000000);

    /* Initialize the rate control modules */
    if(ps_rate_control_api->e_rc_type == CBR_NLDRC)
    {
        irc_change_cbr_buffer_delay(ps_rate_control_api->ps_cbr_buffer,
                                    u4_buffer_delay);
    }
    else if(ps_rate_control_api->e_rc_type == VBR_STORAGE
                    || ps_rate_control_api->e_rc_type == VBR_STORAGE_DVD_COMP)
    {
        UWORD32 au4_num_pics_in_delay_prd[MAX_PIC_TYPE];

        irc_change_vsp_fidp(&ps_rate_control_api->s_vbr_str_prms,
                            u4_frms_in_delay_prd);

        /* Get the number of pics of each type in delay period */
        irc_get_vsp_num_pics_in_dly_prd(&ps_rate_control_api->s_vbr_str_prms,
                                        au4_num_pics_in_delay_prd);

        irc_change_cbr_vbv_num_pics_in_delay_period(
                        ps_rate_control_api->ps_cbr_buffer,
                        au4_num_pics_in_delay_prd);
    }
}

/* Getter functions to get the current rate control parameters */
UWORD32 irc_get_frame_rate(rate_control_api_t *ps_rate_control_api)
{
    return (irc_ba_get_frame_rate(ps_rate_control_api->ps_bit_allocation));
}

UWORD32 irc_get_bit_rate(rate_control_api_t *ps_rate_control_api)
{
    return (irc_ba_get_bit_rate(ps_rate_control_api->ps_bit_allocation));
}

UWORD32 irc_get_peak_bit_rate(rate_control_api_t *ps_rate_control_api,
                              WORD32 i4_index)
{
    return (ps_rate_control_api->au4_new_peak_bit_rate[i4_index]);
}

UWORD32 irc_get_intra_frame_interval(rate_control_api_t *ps_rate_control_api)
{
    return (irc_pic_type_get_intra_frame_interval(
                    ps_rate_control_api->ps_pic_handling));
}

UWORD32 irc_get_inter_frame_interval(rate_control_api_t *ps_rate_control_api)
{
    return (irc_pic_type_get_inter_frame_interval(
                    ps_rate_control_api->ps_pic_handling));
}

rc_type_e irc_get_rc_type(rate_control_api_t *ps_rate_control_api)
{
    return (ps_rate_control_api->e_rc_type);
}

WORD32 irc_get_bits_per_frame(rate_control_api_t *ps_rate_control_api)
{
    WORD32 i4_bits_per_frm;

    X_PROD_Y_DIV_Z(irc_ba_get_bit_rate(ps_rate_control_api->ps_bit_allocation),
                   (UWORD32)1000,
                   irc_ba_get_frame_rate(ps_rate_control_api->ps_bit_allocation),
                   i4_bits_per_frm);

    return (i4_bits_per_frm);
}

UWORD32 irc_get_max_delay(rate_control_api_t *ps_rate_control_api)
{
    return (irc_get_cbr_buffer_delay(ps_rate_control_api->ps_cbr_buffer));
}

UWORD32 irc_get_seq_no(rate_control_api_t *ps_rate_control_api)
{
    return (irc_pic_type_get_disp_order_no(ps_rate_control_api->ps_pic_handling));
}

UWORD32 irc_get_rem_frames_in_gop(rate_control_api_t *ps_rate_control_api)
{
    WORD32 ai4_rem_frms_in_period[MAX_PIC_TYPE];
    WORD32 j;
    UWORD32 u4_rem_frms_in_period = 0;

    /* Get the rem_frms_in_gop & the frms_in_gop from the pic_type state struct */
    irc_pic_type_get_rem_frms_in_gop(ps_rate_control_api->ps_pic_handling,
                                     ai4_rem_frms_in_period);

    /* Depending on the number of gops in a period, find the num_frms_in_prd */
    for(j = 0; j < MAX_PIC_TYPE; j++)
    {
        u4_rem_frms_in_period += ai4_rem_frms_in_period[j];
    }

    return (u4_rem_frms_in_period);
}

/****************************************************************************
 Function Name : irc_flush_buf_frames
 Description   : API call to flush the buffered up frames
 *****************************************************************************/
void irc_flush_buf_frames(rate_control_api_t *ps_rate_control_api)
{
    irc_flush_frame_from_pic_stack(ps_rate_control_api->ps_pic_handling);
}

/****************************************************************************
 Function Name : irc_flush_buf_frames
 Description   : API call to flush the buffered up frames
 *****************************************************************************/

void irc_post_encode_frame_skip(rate_control_api_t *ps_rate_control_api,
                                picture_type_e e_pic_type)
{
    irc_skip_encoded_frame(ps_rate_control_api->ps_pic_handling, e_pic_type);
}

/****************************************************************************
 Function Name : irc_force_I_frame
 Description   : API call to force an I frame
 *****************************************************************************/
void irc_force_I_frame(rate_control_api_t *ps_rate_control_api)
{
    irc_set_force_I_frame_flag(ps_rate_control_api->ps_pic_handling);
}

/****************************************************************************
 * Function Name : rc_get_rem_bits_in_gop
 * Description   : API call to get remaining bits in GOP
 * *****************************************************************************/
WORD32 irc_get_rem_bits_in_period(rate_control_api_t *ps_rate_control_api)
{
    return (irc_ba_get_rem_bits_in_period(
                    ps_rate_control_api->ps_bit_allocation,
                    ps_rate_control_api->ps_pic_handling));
}

/****************************************************************************
 * Function Name : irc_get_vbv_buf_fullness
 * Description   : API call to get VBV buffer fullness
 ******************************************************************************/
WORD32 irc_get_vbv_buf_fullness(rate_control_api_t *ps_rate_control_api)
{
    return (irc_get_cur_vbv_buf_size(ps_rate_control_api->ps_vbr_storage_vbv));
}

WORD32 irc_get_vbv_buf_size(rate_control_api_t *ps_rate_control_api)
{
    if(ps_rate_control_api->e_rc_type == CBR_NLDRC
                    || ps_rate_control_api->e_rc_type == VBR_STREAMING)
    {
        return (irc_get_cbr_buffer_size(ps_rate_control_api->ps_cbr_buffer));
    }
    else
    {
        return (irc_get_max_vbv_buf_size(
                        ps_rate_control_api->ps_vbr_storage_vbv));
    }
}

WORD32 irc_get_vbv_fulness_with_cur_bits(rate_control_api_t *ps_rate_control_api,
                                         UWORD32 u4_bits)
{
    return (irc_vbv_get_vbv_buf_fullness(
                    ps_rate_control_api->ps_vbr_storage_vbv, u4_bits));
}

void irc_set_avg_mb_act(rate_control_api_t *ps_rate_control_api,
                        WORD32 i4_avg_activity)
{
    irc_mb_update_frame_level(ps_rate_control_api->ps_mb_rate_control,
                              i4_avg_activity);
    return;
}
