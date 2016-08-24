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
*  ih264e_encode.c
*
* @brief
*  This file contains functions for encoding the input yuv frame in synchronous
*  api mode
*
* @author
*  ittiam
*
* List of Functions
*  - ih264e_join_threads()
*  - ih264e_wait_for_thread()
*  - ih264e_encode()
*
******************************************************************************
*/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* System Include files */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
/* User Include files */
#include "ih264e_config.h"
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264e.h"
#include "ithread.h"
#include "ih264_defs.h"
#include "ih264_macros.h"
#include "ih264_debug.h"
#include "ih264_structs.h"
#include "ih264_platform_macros.h"
#include "ih264_error.h"
#include "ime_distortion_metrics.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "ih264_list.h"
#include "ih264e_error.h"
#include "ih264e_defs.h"
#include "ih264e_bitstream.h"
#include "irc_mem_req_and_acq.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_time_stamp.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_master.h"
#include "ih264e_process.h"
#include "ih264_buf_mgr.h"
#include "ih264_dpb_mgr.h"
#include "ih264e_utils.h"
#include "ih264e_fmt_conv.h"
#include "ih264e_statistics.h"
#include "ih264e_trace.h"
#include "ih264e_debug.h"
#ifdef LOGO_EN
#include "ih264e_ittiam_logo.h"
#endif

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
******************************************************************************
*
* @brief
*  This function joins all the spawned threads after successful completion of
*  their tasks
*
* @par   Description
*
* @param[in] ps_codec
*  pointer to codec context
*
* @returns  none
*
******************************************************************************
*/
void ih264e_join_threads(codec_t *ps_codec)
{
    /* temp var */
   WORD32 i = 0;
   WORD32 ret = 0;

   /* join spawned threads */
   while (i < ps_codec->i4_proc_thread_cnt)
   {
       if (ps_codec->ai4_process_thread_created[i])
       {
           ret = ithread_join(ps_codec->apv_proc_thread_handle[i], NULL);
           if (ret != 0)
           {
               printf("pthread Join Failed");
               assert(0);
           }
           ps_codec->ai4_process_thread_created[i] = 0;
           i++;
       }
   }

   ps_codec->i4_proc_thread_cnt = 0;
}

/**
******************************************************************************
*
* @brief This function puts the current thread to sleep for a duration
*  of sleep_us
*
* @par Description
*  ithread_yield() method causes the calling thread to yield execution to another
*  thread that is ready to run on the current processor. The operating system
*  selects the thread to yield to. ithread_usleep blocks the current thread for
*  the specified number of milliseconds. In other words, yield just says,
*  end my timeslice prematurely, look around for other threads to run. If there
*  is nothing better than me, continue. Sleep says I don't want to run for x
*  milliseconds. Even if no other thread wants to run, don't make me run.
*
* @param[in] sleep_us
*  thread sleep duration
*
* @returns error_status
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_wait_for_thread(UWORD32 sleep_us)
{
    /* yield thread */
    ithread_yield();

    /* put thread to sleep */
    ithread_usleep(sleep_us);

    return IH264E_SUCCESS;
}

/**
******************************************************************************
*
* @brief
*  Encodes in synchronous api mode
*
* @par Description
*  This routine processes input yuv, encodes it and outputs bitstream and recon
*
* @param[in] ps_codec_obj
*  Pointer to codec object at API level
*
* @param[in] pv_api_ip
*  Pointer to input argument structure
*
* @param[out] pv_api_op
*  Pointer to output argument structure
*
* @returns  Status
*
******************************************************************************
*/
WORD32 ih264e_encode(iv_obj_t *ps_codec_obj, void *pv_api_ip, void *pv_api_op)
{
    /* error status */
    IH264E_ERROR_T error_status = IH264E_SUCCESS;

    /* codec ctxt */
    codec_t *ps_codec = (codec_t *)ps_codec_obj->pv_codec_handle;

    /* input frame to encode */
    ih264e_video_encode_ip_t *ps_video_encode_ip = pv_api_ip;

    /* output buffer to write stream */
    ih264e_video_encode_op_t *ps_video_encode_op = pv_api_op;

    /* i/o structures */
    inp_buf_t s_inp_buf;
    out_buf_t s_out_buf;

    /* temp var */
    WORD32 ctxt_sel = 0, i, i4_rc_pre_enc_skip;

    /********************************************************************/
    /*                            BEGIN INIT                            */
    /********************************************************************/
    /* reset output structure */
    ps_video_encode_op->s_ive_op.u4_error_code = IV_SUCCESS;
    ps_video_encode_op->s_ive_op.output_present  = 0;
    ps_video_encode_op->s_ive_op.dump_recon = 0;
    ps_video_encode_op->s_ive_op.u4_encoded_frame_type = IV_NA_FRAME;

    /* Check for output memory allocation size */
    if (ps_video_encode_ip->s_ive_ip.s_out_buf.u4_bufsize < MIN_STREAM_SIZE)
    {
        error_status |= IH264E_INSUFFICIENT_OUTPUT_BUFFER;
        SET_ERROR_ON_RETURN(error_status,
                            IVE_UNSUPPORTEDPARAM,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);
    }

    /* copy output info. to internal structure */
    s_out_buf.s_bits_buf = ps_video_encode_ip->s_ive_ip.s_out_buf;
    s_out_buf.u4_is_last = 0;
    s_out_buf.u4_timestamp_low = ps_video_encode_ip->s_ive_ip.u4_timestamp_low;
    s_out_buf.u4_timestamp_high = ps_video_encode_ip->s_ive_ip.u4_timestamp_high;

    /* api call cnt */
    ps_codec->i4_encode_api_call_cnt += 1;

    /* codec context selector */
    ctxt_sel = ps_codec->i4_encode_api_call_cnt % MAX_CTXT_SETS;

    /* reset status flags */
    ps_codec->ai4_pic_cnt[ctxt_sel] = -1;
    ps_codec->s_rate_control.post_encode_skip[ctxt_sel] = 0;
    ps_codec->s_rate_control.pre_encode_skip[ctxt_sel] = 0;

    /* pass output buffer to codec */
    ps_codec->as_out_buf[ctxt_sel] = s_out_buf;

    /* initialize codec ctxt with default params for the first encode api call */
    if (ps_codec->i4_encode_api_call_cnt == 0)
    {
        ih264e_codec_init(ps_codec);
    }

    /* parse configuration params */
    for (i = 0; i < MAX_ACTIVE_CONFIG_PARAMS; i++)
    {
        cfg_params_t *ps_cfg = &ps_codec->as_cfg[i];

        if (1 == ps_cfg->u4_is_valid)
        {
            if ( ((ps_cfg->u4_timestamp_high == ps_video_encode_ip->s_ive_ip.u4_timestamp_high) &&
                            (ps_cfg->u4_timestamp_low == ps_video_encode_ip->s_ive_ip.u4_timestamp_low)) ||
                            ((WORD32)ps_cfg->u4_timestamp_high == -1) ||
                            ((WORD32)ps_cfg->u4_timestamp_low == -1) )
            {
                error_status |= ih264e_codec_update_config(ps_codec, ps_cfg);
                SET_ERROR_ON_RETURN(error_status,
                                    IVE_UNSUPPORTEDPARAM,
                                    ps_video_encode_op->s_ive_op.u4_error_code,
                                    IV_FAIL);

                ps_cfg->u4_is_valid = 0;
            }
        }
    }

    /******************************************************************
     * INSERT LOGO
     *****************************************************************/
#ifdef LOGO_EN
    if (s_inp_buf.s_raw_buf.apv_bufs[0] != NULL &&
                    ps_codec->i4_header_mode != 1)
    {
        ih264e_insert_logo(s_inp_buf.s_raw_buf.apv_bufs[0],
                           s_inp_buf.s_raw_buf.apv_bufs[1],
                           s_inp_buf.s_raw_buf.apv_bufs[2],
                           s_inp_buf.s_raw_buf.au4_strd[0],
                           0,
                           0,
                           ps_codec->s_cfg.e_inp_color_fmt,
                           ps_codec->s_cfg.u4_disp_wd,
                           ps_codec->s_cfg.u4_disp_ht);
    }
#endif /*LOGO_EN*/

    /* In case of alt ref and B pics we will have non reference frame in stream */
    if (ps_codec->s_cfg.u4_enable_alt_ref || ps_codec->s_cfg.u4_num_bframes)
    {
        ps_codec->i4_non_ref_frames_in_stream = 1;
    }

    if (ps_codec->i4_encode_api_call_cnt == 0)
    {
        /********************************************************************/
        /*   number of mv/ref bank buffers used by the codec,               */
        /*      1 to handle curr frame                                      */
        /*      1 to store information of ref frame                         */
        /*      1 more additional because of the codec employs 2 ctxt sets  */
        /*        to assist asynchronous API                                */
        /********************************************************************/

        /* initialize mv bank buffer manager */
        error_status |= ih264e_mv_buf_mgr_add_bufs(ps_codec);
        SET_ERROR_ON_RETURN(error_status,
                            IVE_FATALERROR,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);

        /* initialize ref bank buffer manager */
        error_status |= ih264e_pic_buf_mgr_add_bufs(ps_codec);
        SET_ERROR_ON_RETURN(error_status,
                            IVE_FATALERROR,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);

        /* for the first frame, generate header when not requested explicitly */
        if (ps_codec->i4_header_mode == 0 &&
                        ps_codec->u4_header_generated == 0)
        {
            ps_codec->i4_gen_header = 1;
        }
    }

    /* generate header and return when encoder is operated in header mode */
    if (ps_codec->i4_header_mode == 1)
    {
        /* whenever the header is generated, this implies a start of sequence
         * and a sequence needs to be started with IDR
         */
        ps_codec->force_curr_frame_type = IV_IDR_FRAME;

        /* generate header */
        error_status |= ih264e_generate_sps_pps(ps_codec);

        /* api call cnt */
        ps_codec->i4_encode_api_call_cnt --;

        /* header mode tag is not sticky */
        ps_codec->i4_header_mode = 0;
        ps_codec->i4_gen_header = 0;

        /* send the input to app */
        ps_video_encode_op->s_ive_op.s_inp_buf = ps_video_encode_ip->s_ive_ip.s_inp_buf;
        ps_video_encode_op->s_ive_op.u4_timestamp_low = ps_video_encode_ip->s_ive_ip.u4_timestamp_low;
        ps_video_encode_op->s_ive_op.u4_timestamp_high = ps_video_encode_ip->s_ive_ip.u4_timestamp_high;

        ps_video_encode_op->s_ive_op.u4_is_last = ps_video_encode_ip->s_ive_ip.u4_is_last;

        /* send the output to app */
        ps_video_encode_op->s_ive_op.output_present  = 1;
        ps_video_encode_op->s_ive_op.dump_recon = 0;
        ps_video_encode_op->s_ive_op.s_out_buf = ps_codec->as_out_buf[ctxt_sel].s_bits_buf;

        /* error status */
        SET_ERROR_ON_RETURN(error_status,
                            IVE_FATALERROR,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);

        /* indicates that header has been generated previously */
        ps_codec->u4_header_generated = 1;

        return IV_SUCCESS;
    }

    /* curr pic cnt */
     ps_codec->i4_pic_cnt += 1;

    i4_rc_pre_enc_skip = 0;
    i4_rc_pre_enc_skip = ih264e_input_queue_update(
                    ps_codec, &ps_video_encode_ip->s_ive_ip, &s_inp_buf);

    s_out_buf.u4_is_last = s_inp_buf.u4_is_last;
    ps_video_encode_op->s_ive_op.u4_is_last = s_inp_buf.u4_is_last;

    /* Only encode if the current frame is not pre-encode skip */
    if (!i4_rc_pre_enc_skip && s_inp_buf.s_raw_buf.apv_bufs[0])
    {
        /* proc ctxt base idx */
        WORD32 proc_ctxt_select = ctxt_sel * MAX_PROCESS_THREADS;

        /* proc ctxt */
        process_ctxt_t *ps_proc = &ps_codec->as_process[proc_ctxt_select];

        WORD32 ret = 0;

        /* number of addl. threads to be created */
        WORD32 num_thread_cnt = ps_codec->s_cfg.u4_num_cores - 1;

        /* array giving pic cnt that is being processed in curr context set */
        ps_codec->ai4_pic_cnt[ctxt_sel] = ps_codec->i4_pic_cnt;

        /* initialize all relevant process ctxts */
        error_status |= ih264e_pic_init(ps_codec, &s_inp_buf);
        SET_ERROR_ON_RETURN(error_status,
                            IVE_FATALERROR,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);

        for (i = 0; i < num_thread_cnt; i++)
        {
            ret = ithread_create(ps_codec->apv_proc_thread_handle[i],
                                 NULL,
                                 (void *)ih264e_process_thread,
                                 &ps_codec->as_process[i + 1]);
            if (ret != 0)
            {
                printf("pthread Create Failed");
                assert(0);
            }

            ps_codec->ai4_process_thread_created[i] = 1;

            ps_codec->i4_proc_thread_cnt++;
        }


        /* launch job */
        ih264e_process_thread(ps_proc);

        /* Join threads at the end of encoding a frame */
        ih264e_join_threads(ps_codec);

        ih264_list_reset(ps_codec->pv_proc_jobq);

        ih264_list_reset(ps_codec->pv_entropy_jobq);
    }


   /****************************************************************************
   * RECON
   *    Since we have forward dependent frames, we cannot return recon in encoding
   *    order. It must be in poc order, or input pic order. To achieve this we
   *    introduce a delay of 1 to the recon wrt encode. Now since we have that
   *    delay, at any point minimum of pic_cnt in our ref buffer will be the
   *    correct frame. For ex let our GOP be IBBP [1 2 3 4] . The encode order
   *    will be [1 4 2 3] .Now since we have a delay of 1, when we are done with
   *    encoding 4, the min in the list will be 1. After encoding 2, it will be
   *    2, 3 after 3 and 4 after 4. Hence we can return in sequence. Note
   *    that the 1 delay is critical. Hence if we have post enc skip, we must
   *    skip here too. Note that since post enc skip already frees the recon
   *    buffer we need not do any thing here
   *
   *    We need to return a recon when ever we consume an input buffer. This
   *    comsumption include a pre or post enc skip. Thus dump recon is set for
   *    all cases except when
   *    1) We are waiting -> ps_codec->i4_frame_num > 1
   *    2) When the input buffer is null [ ie we are not consuming any inp]
   *        An exception need to be made for the case when we have the last buffer
   *        since we need to flush out the on remainig recon.
   ****************************************************************************/

    ps_video_encode_op->s_ive_op.dump_recon = 0;

    if (ps_codec->s_cfg.u4_enable_recon && (ps_codec->i4_frame_num > 1 || s_inp_buf.u4_is_last)
                    && (s_inp_buf.s_raw_buf.apv_bufs[0] || s_inp_buf.u4_is_last))
    {
        /* error status */
        IH264_ERROR_T ret = IH264_SUCCESS;
        pic_buf_t *ps_pic_buf = NULL;
        WORD32 i4_buf_status, i4_curr_poc = 32768;

        /* In case of skips we return recon, but indicate that buffer is zero size */
        if (ps_codec->s_rate_control.post_encode_skip[ctxt_sel]
                        || i4_rc_pre_enc_skip)
        {

            ps_video_encode_op->s_ive_op.dump_recon = 1;
            ps_video_encode_op->s_ive_op.s_recon_buf.au4_wd[0] = 0;
            ps_video_encode_op->s_ive_op.s_recon_buf.au4_wd[1] = 0;

        }
        else
        {
            for (i = 0; i < ps_codec->i4_ref_buf_cnt; i++)
            {
                if (ps_codec->as_ref_set[i].i4_pic_cnt == -1)
                    continue;

                i4_buf_status = ih264_buf_mgr_get_status(
                                ps_codec->pv_ref_buf_mgr,
                                ps_codec->as_ref_set[i].ps_pic_buf->i4_buf_id);

                if ((i4_buf_status & BUF_MGR_IO)
                                && (ps_codec->as_ref_set[i].i4_poc < i4_curr_poc))
                {
                    ps_pic_buf = ps_codec->as_ref_set[i].ps_pic_buf;
                    i4_curr_poc = ps_codec->as_ref_set[i].i4_poc;
                }
            }

            ps_video_encode_op->s_ive_op.s_recon_buf =
                            ps_video_encode_ip->s_ive_ip.s_recon_buf;

            /*
             * If we get a valid buffer. output and free recon.
             *
             * we may get an invalid buffer if num_b_frames is 0. This is because
             * We assume that there will be a ref frame in ref list after encoding
             * the last frame. With B frames this is correct since its forward ref
             * pic will be in the ref list. But if num_b_frames is 0, we will not
             * have a forward ref pic
             */

            if (ps_pic_buf)
            {
                /* copy/convert the recon buffer and return */
                ih264e_fmt_conv(ps_codec,
                                ps_pic_buf,
                                ps_video_encode_ip->s_ive_ip.s_recon_buf.apv_bufs[0],
                                ps_video_encode_ip->s_ive_ip.s_recon_buf.apv_bufs[1],
                                ps_video_encode_ip->s_ive_ip.s_recon_buf.apv_bufs[2],
                                ps_video_encode_ip->s_ive_ip.s_recon_buf.au4_wd[0],
                                ps_video_encode_ip->s_ive_ip.s_recon_buf.au4_wd[1],
                                0, ps_codec->s_cfg.u4_disp_ht);

                ps_video_encode_op->s_ive_op.dump_recon = 1;

                ret = ih264_buf_mgr_release(ps_codec->pv_ref_buf_mgr,
                                            ps_pic_buf->i4_buf_id, BUF_MGR_IO);

                if (IH264_SUCCESS != ret)
                {
                    SET_ERROR_ON_RETURN(
                                    (IH264E_ERROR_T)ret, IVE_FATALERROR,
                                    ps_video_encode_op->s_ive_op.u4_error_code,
                                    IV_FAIL);
                }
            }
        }
    }


    /***************************************************************************
     * Free reference buffers:
     * In case of a post enc skip, we have to ensure that those pics will not
     * be used as reference anymore. In all other cases we will not even mark
     * the ref buffers
     ***************************************************************************/
    if (ps_codec->s_rate_control.post_encode_skip[ctxt_sel])
    {
        /* pic info */
        pic_buf_t *ps_cur_pic;

        /* mv info */
        mv_buf_t *ps_cur_mv_buf;

        /* error status */
        IH264_ERROR_T ret = IH264_SUCCESS;

        /* Decrement coded pic count */
        ps_codec->i4_poc--;

        /* loop through to get the min pic cnt among the list of pics stored in ref list */
        /* since the skipped frame may not be on reference list, we may not have an MV bank
         * hence free only if we have allocated */
        for (i = 0; i < ps_codec->i4_ref_buf_cnt; i++)
        {
            if (ps_codec->i4_pic_cnt == ps_codec->as_ref_set[i].i4_pic_cnt)
            {

                ps_cur_pic = ps_codec->as_ref_set[i].ps_pic_buf;

                ps_cur_mv_buf = ps_codec->as_ref_set[i].ps_mv_buf;

                /* release this frame from reference list and recon list */
                ret = ih264_buf_mgr_release(ps_codec->pv_mv_buf_mgr, ps_cur_mv_buf->i4_buf_id , BUF_MGR_REF);
                ret |= ih264_buf_mgr_release(ps_codec->pv_mv_buf_mgr, ps_cur_mv_buf->i4_buf_id , BUF_MGR_IO);
                SET_ERROR_ON_RETURN((IH264E_ERROR_T)ret,
                                    IVE_FATALERROR,
                                    ps_video_encode_op->s_ive_op.u4_error_code,
                                    IV_FAIL);

                ret = ih264_buf_mgr_release(ps_codec->pv_ref_buf_mgr, ps_cur_pic->i4_buf_id , BUF_MGR_REF);
                ret |= ih264_buf_mgr_release(ps_codec->pv_ref_buf_mgr, ps_cur_pic->i4_buf_id , BUF_MGR_IO);
                SET_ERROR_ON_RETURN((IH264E_ERROR_T)ret,
                                    IVE_FATALERROR,
                                    ps_video_encode_op->s_ive_op.u4_error_code,
                                    IV_FAIL);
                break;
            }
        }
    }

    /*
     * Since recon is not in sync with output, ie there can be frame to be
     * given back as recon even after last output. Hence we need to mark that
     * the output is not the last.
     * Hence search through reflist and mark appropriately
     */
    if (ps_codec->s_cfg.u4_enable_recon)
    {
        WORD32 i4_buf_status = 0;

        for (i = 0; i < ps_codec->i4_ref_buf_cnt; i++)
        {
            if (ps_codec->as_ref_set[i].i4_pic_cnt == -1)
                continue;

            i4_buf_status |= ih264_buf_mgr_get_status(
                            ps_codec->pv_ref_buf_mgr,
                            ps_codec->as_ref_set[i].ps_pic_buf->i4_buf_id);
        }

        if (i4_buf_status & BUF_MGR_IO)
        {
            s_out_buf.u4_is_last = 0;
            ps_video_encode_op->s_ive_op.u4_is_last = 0;
        }
    }


    /**************************************************************************
     * Signaling to APP
     *  1) If we valid a valid output mark it so
     *  2) Set the codec output ps_video_encode_op
     *  3) Set the error status
     *  4) Set the return Pic type
     *      Note that we already has marked recon properly
     *  5)Send the consumed input back to app so that it can free it if possible
     *
     *  We will have to return the output and input buffers unconditionally
     *  so that app can release them
     **************************************************************************/
    if (!i4_rc_pre_enc_skip
                    && !ps_codec->s_rate_control.post_encode_skip[ctxt_sel]
                    && s_inp_buf.s_raw_buf.apv_bufs[0])
    {

        /* receive output back from codec */
        s_out_buf = ps_codec->as_out_buf[ctxt_sel];

        /* send the output to app */
        ps_video_encode_op->s_ive_op.output_present  = 1;
        ps_video_encode_op->s_ive_op.u4_error_code = IV_SUCCESS;

        /* Set the time stamps of the encodec input */
        ps_video_encode_op->s_ive_op.u4_timestamp_low = s_inp_buf.u4_timestamp_low;
        ps_video_encode_op->s_ive_op.u4_timestamp_high = s_inp_buf.u4_timestamp_high;


        switch (ps_codec->pic_type)
        {
            case PIC_IDR:
                ps_video_encode_op->s_ive_op.u4_encoded_frame_type =IV_IDR_FRAME;
                break;

            case PIC_I:
                ps_video_encode_op->s_ive_op.u4_encoded_frame_type = IV_I_FRAME;
                break;

            case PIC_P:
                ps_video_encode_op->s_ive_op.u4_encoded_frame_type = IV_P_FRAME;
                break;

            case PIC_B:
                ps_video_encode_op->s_ive_op.u4_encoded_frame_type = IV_B_FRAME;
                break;

            default:
                ps_video_encode_op->s_ive_op.u4_encoded_frame_type = IV_NA_FRAME;
                break;
        }

        for (i = 0; i < (WORD32)ps_codec->s_cfg.u4_num_cores; i++)
        {
            error_status |= ps_codec->as_process[ctxt_sel + i].i4_error_code;
        }
        SET_ERROR_ON_RETURN(error_status,
                            IVE_FATALERROR,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);
    }
    else
    {
        /* proc ctxt base idx */
        WORD32 proc_ctxt_select = ctxt_sel * MAX_PROCESS_THREADS;

        /* proc ctxt */
        process_ctxt_t *ps_proc = &ps_codec->as_process[proc_ctxt_select];

        /* receive output back from codec */
        s_out_buf = ps_codec->as_out_buf[ctxt_sel];

        ps_video_encode_op->s_ive_op.output_present = 0;
        ps_video_encode_op->s_ive_op.u4_error_code = IV_SUCCESS;

        /* Set the time stamps of the encodec input */
        ps_video_encode_op->s_ive_op.u4_timestamp_low = 0;
        ps_video_encode_op->s_ive_op.u4_timestamp_high = 0;

        /* receive input back from codec and send it to app */
        s_inp_buf = ps_proc->s_inp_buf;
        ps_video_encode_op->s_ive_op.s_inp_buf = s_inp_buf.s_raw_buf;

        ps_video_encode_op->s_ive_op.u4_encoded_frame_type =  IV_NA_FRAME;

    }

    /* Send the input to encoder so that it can free it if possible */
    ps_video_encode_op->s_ive_op.s_out_buf = s_out_buf.s_bits_buf;
    ps_video_encode_op->s_ive_op.s_inp_buf = s_inp_buf.s_raw_buf;


    if (1 == s_inp_buf.u4_is_last)
    {
        ps_video_encode_op->s_ive_op.output_present = 0;
        ps_video_encode_op->s_ive_op.dump_recon = 0;
    }

    return IV_SUCCESS;
}
