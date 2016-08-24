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
*  ih264e_rate_control.c
*
* @brief
*  Contains api function definitions for h264 rate control
*
* @author
*  ittiam
*
* @par List of Functions:
*  - ih264e_rc_init()
*  - ih264e_rc_get_picture_details()
*  - ih264e_rc_pre_enc()
*  - ih264e_update_rc_mb_info()
*  - ih264e_rc_get_buffer_status()
*  - ih264e_rc_post_enc()
*  - ih264e_update_rc_bits_info()
*
* @remarks
*  None
*
*******************************************************************************
*/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* User include files */
#include "irc_datatypes.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264e.h"
#include "ih264_defs.h"
#include "ih264_macros.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_common_tables.h"
#include "ih264_cabac_tables.h"
#include "ih264e_defs.h"
#include "ih264e_globals.h"
#include "irc_mem_req_and_acq.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "irc_rate_control_api.h"
#include "ih264e_time_stamp.h"
#include "ih264e_modify_frm_rate.h"
#include "ih264e_rate_control.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "ime_distortion_metrics.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_utils.h"
#include "irc_trace_support.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief This function does nothing
*
* @par Description
*  This function does nothing
*
* @param[in] variadic function

* @returns none
*
* @remarks This function is used by the rc library for debugging purposes.
*  However this function was not part of rc library. So this is defined here
*  to resolve link issues.
*
*******************************************************************************
*/
int trace_printf(const WORD8 *format, ...)
{
    UNUSED(format);
    return(0);
};

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
                    UWORD8 u1_profile_level)
{
//    UWORD8  u1_is_mb_level_rc_on = 0;
    UWORD32 au4_peak_bit_rate[2] = {0,0};
    UWORD32 u4_min_bit_rate      = 0;
    WORD32  i4_is_gop_closed     = 1;
//    WORD32  i4_use_est_intra_sad = 1;
    UWORD32 u4_src_ticks         = 0;
    UWORD32 u4_tgt_ticks         = 0;
    UWORD8  u1_level_idx         = ih264e_get_lvl_idx(u1_profile_level);
    UWORD32 u4_max_cpb_size      = 1200 * gas_ih264_lvl_tbl[u1_level_idx].u4_max_cpb_size;

    /* Fill the params needed for the RC init */
    if (e_rate_control_type == CBR_NLDRC)
    {
        au4_peak_bit_rate[0] = u4_avg_bit_rate;
        au4_peak_bit_rate[1] = u4_avg_bit_rate;
    }
    else
    {
        au4_peak_bit_rate[0] = u4_peak_bit_rate;
        au4_peak_bit_rate[1] = u4_peak_bit_rate;
    }

    /* Initialize frame time computation module*/
    ih264e_init_frame_time(pv_frame_time,
                           u4_src_frm_rate,  /* u4_src_frm_rate */
                           u4_tgt_frm_rate); /* u4_tgt_frm_rate */

    /* Initialize the pull_down frame rate */
    ih264e_init_pd_frm_rate(pv_pd_frm_rate,
                            u4_src_frm_rate);  /* u4_input_frm_rate */

    /* Initialize time stamp structure */
    ih264e_init_time_stamp(pv_time_stamp,
                           u4_max_frm_rate,    /* u4_max_frm_rate */
                           u4_src_frm_rate);   /* u4_src_frm_rate */

    u4_src_ticks = ih264e_frame_time_get_src_ticks(pv_frame_time);
    u4_tgt_ticks = ih264e_frame_time_get_tgt_ticks(pv_frame_time);

    /* Init max_inter_frame int */
    i4_max_inter_frm_int = (i4_inter_frm_int == 1) ? 2 : (i4_inter_frm_int + 2);

    /* Initialize the rate control */
    irc_initialise_rate_control(pv_rc_api,                  /* RC handle */
                                e_rate_control_type,        /* RC algo type */
                                0,                          /* MB activity on/off */
                                u4_avg_bit_rate,            /* Avg Bitrate */
                                au4_peak_bit_rate,          /* Peak bitrate array[2]:[I][P] */
                                u4_min_bit_rate,            /* Min Bitrate */
                                u4_src_frm_rate,            /* Src frame_rate */
                                u4_max_delay,               /* Max buffer delay */
                                u4_intra_frame_interval,    /* Intra frm_interval */
                                i4_inter_frm_int,           /* Inter frame interval */
                                pu1_init_qp,                /* Init QP array[3]:[I][P][B] */
                                u4_max_cpb_size,            /* Max VBV/CPB Buffer Size */
                                i4_max_inter_frm_int,       /* Max inter frm_interval */
                                i4_is_gop_closed,           /* Open/Closed GOP */
                                pu1_min_max_qp,             /* Min-max QP array[6]:[Imax][Imin][Pmax][Pmin][Bmax][Bmin] */
                                0,                          /* How to calc the I-frame estimated_sad */
                                u4_src_ticks,               /* Src_ticks = LCM(src_frm_rate,tgt_frm_rate)/src_frm_rate */
                                u4_tgt_ticks);              /* Tgt_ticks = LCM(src_frm_rate,tgt_frm_rate)/tgt_frm_rate */
}

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
                                             WORD32 *pi4_pic_disp_order_no)
{
    picture_type_e e_rc_pic_type = P_PIC;

    irc_get_picture_details(pv_rc_api, pi4_pic_id, pi4_pic_disp_order_no,
                            &e_rc_pic_type);

    return (e_rc_pic_type);
}

/**
*******************************************************************************
*
* @brief  Function to get rate control output before encoding
*
* @par Description
*  This function is called before queing the current frame. It decides if we should
*  skip the current iput buffer due to frame rate mismatch. It also updates RC about
*  the acehivble frame rate
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
* @param[in] i4_delta_time_stamp
*  Time stamp difference between frames
*
* @param[in] i4_total_mb_in_frame
*  Total Macro Blocks in frame
*
* @param[in/out] pe_vop_coding_type
*  Picture coding type(I/P/B)
*
* @param[in/out] pu1_frame_qp
*  QP for current frame
*
* @returns
*  Skip or queue the current frame
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_update_rc_framerates(void *ps_rate_control_api,
                                   void *ps_pd_frm_rate,
                                   void *ps_time_stamp,
                                   void *ps_frame_time)
{
    WORD8 i4_skip_src = 0;
    UWORD32 u4_src_not_skipped_for_dts = 0;

    /* Update the time stamp for the current frame */
    ih264e_update_time_stamp(ps_time_stamp);

    /* Check if a src not needs to be skipped */
    i4_skip_src = ih264e_should_src_be_skipped(ps_frame_time,
                                               1,
                                               &u4_src_not_skipped_for_dts);

    if (i4_skip_src)
    {
        /***********************************************************************
         *Based on difference in source and target frame rate frames are skipped
         ***********************************************************************/
        /*update the missing frames frm_rate with 0 */
        ih264e_update_pd_frm_rate(ps_pd_frm_rate, 0);
    }
    else
    {
        WORD32 i4_avg_frm_rate, i4_source_frame_rate;

        i4_source_frame_rate = ih264e_frame_time_get_src_frame_rate(
                        ps_frame_time);

        /* Update the frame rate of the frame present with the tgt_frm_rate */
        /* If the frm was not skipped due to delta_time_stamp, update the
         frame_rate with double the tgt_frame_rate value, so that it makes
         up for one of the frames skipped by the application */
        ih264e_update_pd_frm_rate(ps_pd_frm_rate, i4_source_frame_rate);

        /* Based on the update get the average frame rate */
        i4_avg_frm_rate = ih264e_get_pd_avg_frm_rate(ps_pd_frm_rate);

        /* Call the RC library function to change the frame_rate to the
         actually achieved frm_rate */
        irc_change_frm_rate_for_bit_alloc(ps_rate_control_api, i4_avg_frm_rate);
    }

    return (i4_skip_src);
}

/**
*******************************************************************************
*
* @brief Function to update mb info for rate control context
*
* @par   Description
*  After encoding a mb, information such as mb type, qp used, mb distortion
*  resulted in encoding the block and so on needs to be preserved for modeling
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
void ih264e_update_rc_mb_info(frame_info_t *ps_frame_info, void *pv_proc)
{
    /* proc ctxt */
    process_ctxt_t *ps_proc = pv_proc;

    /* is intra or inter */
    WORD32 mb_type = !ps_proc->u4_is_intra;

    /* distortion */
    ps_frame_info->tot_mb_sad[mb_type] += ps_proc->i4_mb_distortion;

    /* qp */
    ps_frame_info->qp_sum[mb_type] += gau1_h264_to_mpeg2_qmap[ps_proc->u4_mb_qp];

    /* mb cnt */
    ps_frame_info->num_mbs[mb_type]++;

    /* cost */
    if (ps_proc->u4_is_intra)
    {
        ps_frame_info->intra_mb_cost_sum += ps_proc->i4_mb_cost;
    }
}

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
                                 UWORD8 *pu1_is_enc_buf_underflow)
{
    vbv_buf_status_e e_vbv_buf_status = VBV_NORMAL;

    e_vbv_buf_status = irc_get_buffer_status(pv_rc_api,
                                             i4_total_frame_bits,
                                             e_pic_type,
                                             pi4_num_bits_to_prevent_vbv_underflow);

    if (e_vbv_buf_status == VBV_OVERFLOW)
    {
        *pu1_is_enc_buf_underflow = 1;
        *pu1_is_enc_buf_overflow = 0;
    }
    else if (e_vbv_buf_status == VBV_UNDERFLOW)
    {
        *pu1_is_enc_buf_underflow = 0;
        *pu1_is_enc_buf_overflow = 1;
    }
    else
    {
        *pu1_is_enc_buf_underflow = 0;
        *pu1_is_enc_buf_overflow = 0;
    }
}

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
*  Numberf of intra mbs in previous frame
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
WORD32 ih264e_rc_post_enc(void * ps_rate_control_api,
                          frame_info_t *ps_frame_info,
                          void * ps_pd_frm_rate,
                          void * ps_time_stamp,
                          void * ps_frame_time,
                          WORD32   i4_total_mb_in_frame,
                          picture_type_e *pe_vop_coding_type,
                          WORD32 i4_is_first_frame,
                          WORD32 *pi4_is_post_encode_skip,
                          UWORD8 u1_frame_qp,
                          WORD32 *pi4_num_intra_in_prev_frame,
                          WORD32 *pi4_avg_activity)
{
    /* Variables for the update_frm_level_info */
    WORD32  ai4_tot_mb_in_type[MAX_MB_TYPE];
    WORD32  ai4_tot_mb_type_qp[MAX_MB_TYPE]    = {0, 0};
    WORD32  ai4_mb_type_sad[MAX_MB_TYPE]       = {0, 0};
    WORD32  ai4_mb_type_tex_bits[MAX_MB_TYPE]  = {0, 0};
    WORD32   i4_total_frame_bits               = 0;
    WORD32   i4_total_hdr_bits                 = 0;
    WORD32   i4_total_texturebits;
    WORD32   i4_avg_mb_activity                = 0;
    WORD32   i4_intra_frm_cost                 = 0;
    UWORD8   u1_is_scd                         = 0;
    WORD32  i4_cbr_bits_to_stuff               = 0;
    UWORD32   u4_num_intra_in_prev_frame        = *pi4_num_intra_in_prev_frame;
    UNUSED(ps_pd_frm_rate);
    UNUSED(ps_time_stamp);
    UNUSED(ps_frame_time);
    UNUSED(u1_frame_qp);
    /* Accumulate RC stats */
    ai4_tot_mb_in_type[MB_TYPE_INTRA]    = irc_fi_get_total_mb(ps_frame_info,MB_TYPE_INTRA);
    ai4_tot_mb_in_type[MB_TYPE_INTER]    = irc_fi_get_total_mb(ps_frame_info,MB_TYPE_INTER);
    /* ai4_tot_mb_type_qp[MB_TYPE_INTRA]    = 0;
    ai4_tot_mb_type_qp[MB_TYPE_INTER]    = ps_enc->pu1_h264_mpg2quant[u1_frame_qp] * i4_total_mb_in_frame;*/
    ai4_tot_mb_type_qp[MB_TYPE_INTRA]    = irc_fi_get_total_mb_qp(ps_frame_info,MB_TYPE_INTRA);
    ai4_tot_mb_type_qp[MB_TYPE_INTER]    = irc_fi_get_total_mb_qp(ps_frame_info,MB_TYPE_INTER);
    ai4_mb_type_sad[MB_TYPE_INTRA]       = irc_fi_get_total_mb_sad(ps_frame_info,MB_TYPE_INTRA);
    ai4_mb_type_sad[MB_TYPE_INTER]       = irc_fi_get_total_mb_sad(ps_frame_info,MB_TYPE_INTER);
    i4_intra_frm_cost                    = irc_fi_get_total_intra_mb_cost(ps_frame_info);
    i4_avg_mb_activity                   = irc_fi_get_avg_activity(ps_frame_info);
    i4_total_hdr_bits                    = irc_fi_get_total_header_bits(ps_frame_info);
    i4_total_texturebits                 = irc_fi_get_total_mb_texture_bits(ps_frame_info,MB_TYPE_INTRA);
    i4_total_texturebits                 += irc_fi_get_total_mb_texture_bits(ps_frame_info,MB_TYPE_INTER);
    i4_total_frame_bits                  = i4_total_hdr_bits + i4_total_texturebits ;

    *pi4_avg_activity = i4_avg_mb_activity;


    /* Texture bits are not accumulated. Hence subtracting hdr bits from total bits */
    ai4_mb_type_tex_bits[MB_TYPE_INTRA]  = 0;
    ai4_mb_type_tex_bits[MB_TYPE_INTER]  = i4_total_frame_bits - i4_total_hdr_bits;

    /* Set post encode skip to zero */
    pi4_is_post_encode_skip[0]= 0;

    /* For NLDRC, get the buffer status for stuffing or skipping */
    if (irc_get_rc_type(ps_rate_control_api) == CBR_NLDRC)
    {
        WORD32 i4_get_num_bit_to_prevent_vbv_overflow;
        UWORD8 u1_enc_buf_overflow,u1_enc_buf_underflow;

        /* Getting the buffer status */
        ih264e_rc_get_buffer_status(ps_rate_control_api, i4_total_frame_bits,
            pe_vop_coding_type[0],  &i4_get_num_bit_to_prevent_vbv_overflow,
            &u1_enc_buf_overflow,&u1_enc_buf_underflow);

        /* We skip the frame if decoder buffer is underflowing. But we never skip first I frame */
        if ((u1_enc_buf_overflow == 1) && (i4_is_first_frame != 1))
        // if ((u1_enc_buf_overflow == 1) && (i4_is_first_frame != 0))
        {
            irc_post_encode_frame_skip(ps_rate_control_api, (picture_type_e)pe_vop_coding_type[0]);
            // i4_total_frame_bits = imp4_write_skip_frame_header(ps_enc);
            i4_total_frame_bits = 0;

            *pi4_is_post_encode_skip = 1;

            /* Adjust the GOP if in case we skipped an I-frame */
            if (*pe_vop_coding_type == I_PIC)
                irc_force_I_frame(ps_rate_control_api);

            /* Since this frame is skipped by writing 7 bytes header, we say this is a P frame */
            // *pe_vop_coding_type = P;

            /* Getting the buffer status again,to check if it underflows  */
            irc_get_buffer_status(ps_rate_control_api, i4_total_frame_bits,
                (picture_type_e)pe_vop_coding_type[0], &i4_get_num_bit_to_prevent_vbv_overflow);

        }

        /* In this case we stuff bytes as buffer is overflowing */
        if (u1_enc_buf_underflow == 1)
        {
            /* The stuffing function is directly pulled out from split controller workspace.
               encode_vop_data() function makes sure alignment data is dumped at the end of a
               frame. Split controller was identifying this alignment byte, overwriting it with
               the stuff data and then finally aligning the buffer. Here every thing is inside
               the DSP. So, ideally encode_vop_data needn't align, and we can start stuffing directly.
               But in that case, it'll break the logic for a normal frame.
               Hence for simplicity, not changing this part since it is ok to align and
               then overwrite since stuffing is not done for every frame */
            i4_cbr_bits_to_stuff = irc_get_bits_to_stuff(ps_rate_control_api, i4_total_frame_bits, pe_vop_coding_type[0]);

            /* Just add extra 32 bits to make sure we don't stuff lesser */
            i4_cbr_bits_to_stuff += 32;

            /* We can not stuff more than the outbuf size. So have a check here */
            /* Add stuffed bits to total bits */
            i4_total_frame_bits += i4_cbr_bits_to_stuff;
        }
    }

#define ENABLE_SCD 1
#if ENABLE_SCD
    /* If number of intra MBs are more than 2/3rd of total MBs, assume it as a scene change */
    if ((ai4_tot_mb_in_type[MB_TYPE_INTRA] > ((2 * i4_total_mb_in_frame) / 3)) &&
       (*pe_vop_coding_type == P_PIC) &&
       (ai4_tot_mb_in_type[MB_TYPE_INTRA] > ((11 * (WORD32)u4_num_intra_in_prev_frame) / 10)))
    {
        u1_is_scd = 1;
    }
#endif

    /* Update num intra mbs of this frame */
    if (pi4_is_post_encode_skip[0] == 0)
    {
        *pi4_num_intra_in_prev_frame = ai4_tot_mb_in_type[MB_TYPE_INTRA];
    }

    /* Reset intra count to zero, if u encounter an I frame */
    if (*pe_vop_coding_type == I_PIC)
    {
        *pi4_num_intra_in_prev_frame = 0;
    }

    /* Do an update of rate control after post encode */
    irc_update_frame_level_info(ps_rate_control_api,        /* RC state */
                                pe_vop_coding_type[0],      /* PIC type */
                                ai4_mb_type_sad,            /* SAD for [Intra/Inter] */
                                i4_total_frame_bits,        /* Total frame bits */
                                i4_total_hdr_bits,          /* header bits for */
                                ai4_mb_type_tex_bits,       /* for MB[Intra/Inter] */
                                ai4_tot_mb_type_qp,         /* for MB[Intra/Inter] */
                                ai4_tot_mb_in_type,         /* for MB[Intra/Inter] */
                                i4_avg_mb_activity,         /* Average mb activity in frame */
                                u1_is_scd,                  /* Is a scene change detected */
                                0,                          /* Pre encode skip  */
                                (WORD32)i4_intra_frm_cost,  /* Intra cost for frame */
                                0);                         /* Not done outside */

    return (i4_cbr_bits_to_stuff >> 3);
}

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
void ih264e_update_rc_bits_info(frame_info_t *ps_frame_info, void *pv_entropy)
{
    entropy_ctxt_t *ps_entropy = pv_entropy;

    ps_frame_info->mb_header_bits[MB_TYPE_INTRA] += ps_entropy->u4_header_bits[MB_TYPE_INTRA];

    ps_frame_info->mb_texture_bits[MB_TYPE_INTRA] += ps_entropy->u4_residue_bits[MB_TYPE_INTRA];

    ps_frame_info->mb_header_bits[MB_TYPE_INTER] += ps_entropy->u4_header_bits[MB_TYPE_INTER];

    ps_frame_info->mb_texture_bits[MB_TYPE_INTER] += ps_entropy->u4_residue_bits[MB_TYPE_INTER];

    return;
}

