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
*  ih264e_utils.c
*
* @brief
*  Contains miscellaneous utility functions used by the encoder
*
* @author
*  ittiam
*
* @par List of Functions:
*  - ih264e_get_min_level()
*  - ih264e_get_lvl_idx()
*  - ih264e_get_dpb_size()
*  - ih264e_get_total_pic_buf_size()
*  - ih264e_get_pic_mv_bank_size()
*  - ih264e_pic_buf_mgr_add_bufs()
*  - ih264e_mv_buf_mgr_add_bufs()
*  - ih264e_init_quant_params()
*  - ih264e_init_air_map()
*  - ih264e_codec_init()
*  - ih264e_pic_init()
*
* @remarks
*  None
*
*******************************************************************************
*/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* system include files */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* user include files */
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264e.h"
#include "ithread.h"
#include "ih264_defs.h"
#include "ih264_size_defs.h"
#include "ime_distortion_metrics.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "ih264_error.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "ih264_macros.h"
#include "ih264_common_tables.h"
#include "ih264_debug.h"
#include "ih264_trans_data.h"
#include "ih264e_defs.h"
#include "ih264e_globals.h"
#include "ih264_buf_mgr.h"
#include "ih264_dpb_mgr.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_cabac.h"
#include "ih264e_utils.h"
#include "ih264e_config.h"
#include "ih264e_statistics.h"
#include "ih264e_trace.h"
#include "ih264_list.h"
#include "ih264e_encode_header.h"
#include "ih264e_me.h"
#include "ime.h"
#include "ih264e_core_coding.h"
#include "ih264e_rc_mem_interface.h"
#include "ih264e_time_stamp.h"
#include "ih264e_debug.h"
#include "ih264e_process.h"
#include "ih264e_master.h"
#include "irc_rate_control_api.h"
#include "ime_statistics.h"

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 *******************************************************************************
 *
 * @brief
 *  Queues the current buffer, gets back a another buffer for encoding with corrent
 *  picture type
 *
 * @par Description:
 *      This function performs 3 distinct but related functions.
 *      1) Maintains an input queue [Note the the term queue donot imply a
 *         first-in first-out logic here] that queues input and dequeues them so
 *         that input frames can be encoded at any predetermined encoding order
 *      2) Uses RC library to decide which frame must be encoded in current pass
 *         and which picture type it must be encoded to.
 *      3) Uses RC library to decide the QP at which current frame has to be
 *         encoded
 *      4) Determines if the current picture must be encoded or not based on
 *         PRE-ENC skip
 *
 *     Input queue is used for storing input buffers till they are used for
 *     encoding. This queue is maintained at ps_codec->as_inp_list. Whenever a
 *     valid input comes, it is added to the end of queue. This same input is
 *     added to RC queue using the identifier as ps_codec->i4_pic_cnt. Hence any
 *     pic from RC can be located in the input queue easily.
 *
 *     The dequeue operation does not start till we have ps_codec->s_cfg.u4_max_num_bframes
 *     frames in the queue. THis is done in order to ensure that once output starts
 *     we will have a constant stream of output with no gaps.
 *
 *     THe output frame order is governed by RC library. When ever we dequeue a
 *     buffer from RC library, it ensures that we will get them in encoding order
 *     With the output of RC library, we can use the picture id to dequeue the
 *     corresponding buffer from input queue and encode it.
 *
 *     Condition at the end of stream.
 *     -------------------------------
 *      At the last valid buffer from the app, we will get ps_ive_ip->u4_is_last
 *      to be set. This will the given to lib when appropriate input buffer is
 *      given to encoding.
 *
 *      Since we have to output is not in sync with input, we will have frames to
 *      encode even after we recive the last vaild input buffer. Hence we have to
 *      make sure that we donot queue any new buffers once we get the flag [It may
 *      mess up GOP ?]. This is acheived by setting ps_codec->i4_last_inp_buff_received
 *      to act as a permenent marker for last frame recived [This may not be needed,
 *      because in our current app, all buffers after the last are marked as last.
 *      But can we rely on that?] . Hence after this flgag is set no new buffers are
 *      queued.
 *
 * @param[in] ps_codec
 *   Pointer to codec descriptor
 *
 * @param[in] ps_ive_ip
 *   Current input buffer to the encoder
 *
 * @param[out] ps_inp
 *   Buffer to be encoded in the current pass
 *
 * @returns
 *   Flag indicating if we have a pre-enc skip or not
 *
 * @remarks
 * TODO (bpic)
 *  The check for null ans is last is redudent.
 *  Need to see if we can remove it
 *
 *******************************************************************************
 */
WORD32 ih264e_input_queue_update(codec_t *ps_codec,
                                 ive_video_encode_ip_t *ps_ive_ip,
                                 inp_buf_t *ps_enc_buff)
{

    inp_buf_t *ps_inp_buf;
    picture_type_e e_pictype;
    WORD32 i4_skip;
    UWORD32 ctxt_sel, u4_pic_id, u4_pic_disp_id;
    UWORD8 u1_frame_qp;
    UWORD32 max_frame_bits = 0x7FFFFFFF;

    /*  Mark that the last input frame has been received */
    if (ps_ive_ip->u4_is_last == 1)
    {
        ps_codec->i4_last_inp_buff_received = 1;
    }

    if (ps_ive_ip->s_inp_buf.apv_bufs[0] == NULL
                    && !ps_codec->i4_last_inp_buff_received)
    {
        ps_enc_buff->s_raw_buf.apv_bufs[0] = NULL;
        ps_enc_buff->u4_is_last = ps_ive_ip->u4_is_last;
        return 0;
    }

    /***************************************************************************
     * Check for pre enc skip
     *   When src and target frame rates donot match, we skip some frames to
     *   maintain the relation ship between them
     **************************************************************************/
    {
        WORD32 skip_src;

        skip_src = ih264e_update_rc_framerates(
                        ps_codec->s_rate_control.pps_rate_control_api,
                        ps_codec->s_rate_control.pps_pd_frm_rate,
                        ps_codec->s_rate_control.pps_time_stamp,
                        ps_codec->s_rate_control.pps_frame_time);

        if (skip_src)
        {
            ps_enc_buff->u4_is_last = ps_ive_ip->u4_is_last;
            return 1;
        }
    }

    /***************************************************************************
     *Queue the input to the queue
     **************************************************************************/
    ps_inp_buf = &(ps_codec->as_inp_list[ps_codec->i4_pic_cnt
                                         % MAX_NUM_BFRAMES]);

    /* copy input info. to internal structure */
    ps_inp_buf->s_raw_buf = ps_ive_ip->s_inp_buf;
    ps_inp_buf->u4_timestamp_low = ps_ive_ip->u4_timestamp_low;
    ps_inp_buf->u4_timestamp_high = ps_ive_ip->u4_timestamp_high;
    ps_inp_buf->u4_is_last = ps_ive_ip->u4_is_last;
    ps_inp_buf->pv_mb_info = ps_ive_ip->pv_mb_info;
    ps_inp_buf->u4_mb_info_type = ps_ive_ip->u4_mb_info_type;
    ps_inp_buf->pv_pic_info = ps_ive_ip->pv_pic_info;
    ps_inp_buf->u4_pic_info_type = ps_ive_ip->u4_pic_info_type;

    /***************************************************************************
     * Now we should add the picture to RC stack here
     **************************************************************************/
    /*
     * If an I frame has been requested, ask  RC to force it
     * For IDR requests, we have to ask RC to force I and set IDR by our selves
     * since RC Donot know about IDR. For forcing an IDR at dequeue stage we
     * should record that an IDR has been requested some where. Hence we will
     * store it in the u4_idr_inp_list at a position same as that of input frame
     */
    {
        WORD32 i4_force_idr, i4_force_i;

        i4_force_idr = (ps_codec->force_curr_frame_type == IV_IDR_FRAME);
        i4_force_idr |= !(ps_codec->i4_pic_cnt % ps_codec->s_cfg.u4_idr_frm_interval);

        i4_force_i = (ps_codec->force_curr_frame_type == IV_I_FRAME);

        ps_codec->i4_pending_idr_flag |= i4_force_idr;

        if ((ps_codec->i4_pic_cnt > 0) && (i4_force_idr || i4_force_i))
        {
            irc_force_I_frame(ps_codec->s_rate_control.pps_rate_control_api);
        }
        ps_codec->force_curr_frame_type = IV_NA_FRAME;
    }

    irc_add_picture_to_stack(ps_codec->s_rate_control.pps_rate_control_api,
                             ps_codec->i4_pic_cnt);


    /* Delay */
    if (ps_codec->i4_encode_api_call_cnt
                    < (WORD32)(ps_codec->s_cfg.u4_num_bframes))
    {
        ps_enc_buff->s_raw_buf.apv_bufs[0] = NULL;
        ps_enc_buff->u4_is_last = 0;
        return 0;
    }

    /***************************************************************************
     * Get a new pic to encode
     **************************************************************************/
    /* Query the picture_type */
    e_pictype = ih264e_rc_get_picture_details(
                    ps_codec->s_rate_control.pps_rate_control_api, (WORD32 *)(&u4_pic_id),
                    (WORD32 *)(&u4_pic_disp_id));

    switch (e_pictype)
    {
        case I_PIC:
            ps_codec->pic_type = PIC_I;
            break;
        case P_PIC:
            ps_codec->pic_type = PIC_P;
            break;
        case B_PIC:
            ps_codec->pic_type = PIC_B;
            break;
        default:
            ps_codec->pic_type = PIC_NA;
            ps_enc_buff->s_raw_buf.apv_bufs[0] = NULL;
            return 0;
    }

    /* Set IDR if it has been requested */
    if (ps_codec->pic_type == PIC_I)
    {
        ps_codec->pic_type = ps_codec->i4_pending_idr_flag ?
                                    PIC_IDR : ps_codec->pic_type;
        ps_codec->i4_pending_idr_flag = 0;
    }

    /* Get current frame Qp */
    u1_frame_qp = (UWORD8)irc_get_frame_level_qp(
                    ps_codec->s_rate_control.pps_rate_control_api, e_pictype,
                    max_frame_bits);
    ps_codec->u4_frame_qp = gau1_mpeg2_to_h264_qmap[u1_frame_qp];

    /*
     * copy the pic id to poc because the display order is assumed to be same
     * as input order
     */
    ps_codec->i4_poc = u4_pic_id;

    /***************************************************************************
     * Now retrieve the correct picture from the queue
     **************************************************************************/

    /* Mark the skip flag   */
    i4_skip = 0;
    ctxt_sel = ps_codec->i4_encode_api_call_cnt % MAX_CTXT_SETS;
    ps_codec->s_rate_control.pre_encode_skip[ctxt_sel] = i4_skip;

    /* Get a buffer to encode */
    ps_inp_buf = &(ps_codec->as_inp_list[u4_pic_id % MAX_NUM_BFRAMES]);

    /* copy dequeued input to output */
    ps_enc_buff->s_raw_buf = ps_inp_buf->s_raw_buf;
    ps_enc_buff->u4_timestamp_low = ps_inp_buf->u4_timestamp_low;
    ps_enc_buff->u4_timestamp_high = ps_inp_buf->u4_timestamp_high;
    ps_enc_buff->u4_is_last = ps_inp_buf->u4_is_last;
    ps_enc_buff->pv_mb_info = ps_inp_buf->pv_mb_info;
    ps_enc_buff->u4_mb_info_type = ps_inp_buf->u4_mb_info_type;
    ps_enc_buff->pv_pic_info = ps_inp_buf->pv_pic_info;
    ps_enc_buff->u4_pic_info_type = ps_inp_buf->u4_pic_info_type;

    /* Special case for encoding trailing B frames
     *
     * In encoding streams with B frames it may happen that we have a B frame
     * at the end without a P/I frame after it. Hence when we are dequeing from
     * the RC, it will return the P frame [next in display order but before in
     * encoding order] first. Since the dequeue happens for an invalid frame we
     * will get a frame with null buff and set u4_is_last. Hence lib with return
     * last frame flag at this point and will stop encoding.
     *
     * Since for the last B frame, we does not have the forward ref frame
     * it makes sense to force it into P.
     *
     * To solve this, in case the current frame is P and if the last frame flag
     * is set, we need to see if there is and pending B frames. If there are any,
     * we should just encode that picture as the current P frame and set
     * that B frame as the last frame. Hence the encoder will terminate naturally
     * once that B-frame is encoded after all the in between frames.
     *
     * Since we cannot touch RC stack directly, the option of actually swapping
     * frames in RC is ruled out. We have to modify the as_inp_list to simulate
     * such a behavior by RC. We can do that by
     *  1) Search through as_inp_list to locate the largest u4_timestamp_low less
     *     than current u4_timestamp_low. This will give us the last B frame before
     *     the current P frame. Note that this will handle pre encode skip too since
     *     queue happens after pre enc skip.
     *  2) Swap the position in as_inp_list. Hence now the last B frame is
     *     encoded as P frame. And the new last B frame will have u4_is_last
     *     set so that encoder will end naturally once we reached that B frame
     *     or any subsequent frame. Also the current GOP will have 1 less B frame
     *     Since we are swapping, the poc will also be in-order.
     *  3) In case we have an IPP stream, the result of our search will be an
     *     I/P frame which is already encoded. Thus swap and encode will result
     *     in encoding of duplicate frames. Hence to avoid this we will only
     *     have this work around in case of u4_num_bframes > 0.
     *
     *     In case we have forced an I/IDR frame In between this P frame and
     *     the last B frame -> This cannot happen as the current P frame is
     *     supposed to have u4_is_last set. Thus forcing an I/ IDR after this
     *     is illogical.
     *
     *     In cae if we have forced an I such that the frame just before last frame
     *     in is I/P -> This case will never arise. Since we have a closed GOP now,
     *     once we force an I, the gop gets reset, hence there will be a B between
     *     I/P and I/P.
     */
    if (ps_enc_buff->u4_is_last && (ps_codec->pic_type == PIC_P)
                    && ps_codec->s_cfg.u4_num_bframes && (ps_codec->i4_poc > 1))
    {
        UWORD32 u4_cntr, u4_lst_bframe;
        inp_buf_t *ps_swap_buff, *ps_inp_list, *ps_cur_pic;

        u4_cntr = (u4_pic_id + 1) % MAX_NUM_BFRAMES;
        u4_lst_bframe = u4_pic_id ? ((u4_pic_id - 1) % MAX_NUM_BFRAMES) : (MAX_NUM_BFRAMES - 1);

        ps_inp_list = &ps_codec->as_inp_list[0];
        ps_cur_pic = &ps_inp_list[u4_pic_id % MAX_NUM_BFRAMES];

        /* Now search the pic in most recent past to current frame */
        for(; u4_cntr != (u4_pic_id % MAX_NUM_BFRAMES);
                        u4_cntr = ((u4_cntr + 1) % MAX_NUM_BFRAMES))
        {
            if ( (ps_inp_list[u4_cntr].u4_timestamp_low  <= ps_cur_pic->u4_timestamp_low) &&
                 (ps_inp_list[u4_cntr].u4_timestamp_high <= ps_cur_pic->u4_timestamp_high) &&
                 (ps_inp_list[u4_cntr].u4_timestamp_low  >= ps_inp_list[u4_lst_bframe].u4_timestamp_low) &&
                 (ps_inp_list[u4_cntr].u4_timestamp_high >= ps_inp_list[u4_lst_bframe].u4_timestamp_high))
            {
                u4_lst_bframe = u4_cntr;
            }
        }

        ps_swap_buff = &(ps_codec->as_inp_list[u4_lst_bframe]);

        /* copy the last B buffer to output */
        *ps_enc_buff = *ps_swap_buff;

        /* Store the current buf into the queue in place of last B buf */
        *ps_swap_buff = *ps_inp_buf;

    }

    if (ps_enc_buff->u4_is_last)
    {
        ps_codec->pic_type = PIC_NA;
    }

    /* Return the buffer status */
    return (0);
}

/**
*******************************************************************************
*
* @brief
*  Used to get minimum level index for a given picture size
*
* @par Description:
*  Gets the minimum level index and then gets corresponding level.
*  Also used to ignore invalid levels like 2.3, 3.3 etc
*
* @param[in] level
*  Level of the stream
*
* @returns  Level index for a given level
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_get_min_level(WORD32 wd, WORD32 ht)
{
    WORD32 lvl_idx = MAX_LEVEL, i;
    WORD32 pic_size = wd * ht;
    WORD32 max = MAX(wd, ht);
    for (i = 0; i < MAX_LEVEL; i++)
    {
        if ((pic_size <= gai4_ih264_max_luma_pic_size[i]) &&
            (max <= gai4_ih264_max_wd_ht[i]))
        {
            lvl_idx = i;
            break;
        }
    }

    return gai4_ih264_levels[lvl_idx];
}

/**
*******************************************************************************
*
* @brief
*  Used to get level index for a given level
*
* @par Description:
*  Converts from level_idc (which is multiplied by 30) to an index that can be
*  used as a lookup. Also used to ignore invalid levels like 2.2 , 3.2 etc
*
* @param[in] level
*  Level of the stream
*
* @returns  Level index for a given level
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_get_lvl_idx(WORD32 level)
{
    WORD32 lvl_idx = 0;

    if (level < IH264_LEVEL_11)
    {
        lvl_idx = 0;
    }
    else if (level < IH264_LEVEL_12)
    {
        lvl_idx = 1;
    }
    else if (level < IH264_LEVEL_13)
    {
        lvl_idx = 2;
    }
    else if (level < IH264_LEVEL_20)
    {
        lvl_idx = 3;
    }
    else if (level < IH264_LEVEL_21)
    {
        lvl_idx = 4;
    }
    else if (level < IH264_LEVEL_22)
    {
        lvl_idx = 5;
    }
    else if (level < IH264_LEVEL_30)
    {
        lvl_idx = 6;
    }
    else if (level < IH264_LEVEL_31)
    {
        lvl_idx = 7;
    }
    else if (level < IH264_LEVEL_32)
    {
        lvl_idx = 8;
    }
    else if (level < IH264_LEVEL_40)
    {
        lvl_idx = 9;
    }
    else if (level < IH264_LEVEL_41)
    {
        lvl_idx = 10;
    }
    else if (level < IH264_LEVEL_42)
    {
        lvl_idx = 11;
    }
    else if (level < IH264_LEVEL_50)
    {
        lvl_idx = 12;
    }
    else if (level < IH264_LEVEL_51)
    {
        lvl_idx = 13;
    }
    else
    {
        lvl_idx = 14;
    }

    return (lvl_idx);
}

/**
*******************************************************************************
*
* @brief returns maximum number of pictures allowed in dpb for a given level
*
* @par Description:
*  For given width, height and level, number of pictures allowed in decoder
*  picture buffer is computed as per Annex A.3.1
*
* @param[in] level
*  level of the bit-stream
*
* @param[in] pic_size
*  width * height
*
* @returns  Number of buffers in DPB
*
* @remarks
*  From annexure A.3.1 of H264 specification,
*  max_dec_frame_buffering <= MaxDpbSize, where MaxDpbSize is equal to
*  Min( 1024 * MaxDPB / ( PicWidthInMbs * FrameHeightInMbs * 384 ), 16 ) and
*  MaxDPB is given in Table A-1 in units of 1024 bytes. However the MaxDPB size
*  presented in the look up table gas_ih264_lvl_tbl is in units of 512
*  bytes. Hence the expression is modified accordingly.
*
*******************************************************************************
*/
WORD32 ih264e_get_dpb_size(WORD32 level, WORD32 pic_size)
{
    /* dpb size */
    WORD32 max_dpb_size_bytes = 0;

    /* dec frame buffering */
    WORD32 max_dpb_size_frames = 0;

    /* temp var */
    WORD32 i;

    /* determine max luma samples */
    for (i = 0; i < 16; i++)
        if (level == (WORD32)gas_ih264_lvl_tbl[i].u4_level_idc)
            max_dpb_size_bytes = gas_ih264_lvl_tbl[i].u4_max_dpb_size;

    /* from Annexure A.3.1 h264 specification */
    max_dpb_size_frames =
                    MIN( 1024 * max_dpb_size_bytes / ( pic_size * 3 ), MAX_DPB_SIZE );

    return max_dpb_size_frames;
}

/**
*******************************************************************************
*
* @brief
*  Used to get reference picture buffer size for a given level and
*  and padding used
*
* @par Description:
*  Used to get reference picture buffer size for a given level and padding used
*  Each picture is padded on all four sides
*
* @param[in] pic_size
*  Number of luma samples (Width * Height)
*
* @param[in] level
*  Level
*
* @param[in] horz_pad
*  Total padding used in horizontal direction
*
* @param[in] vert_pad
*  Total padding used in vertical direction
*
* @returns  Total picture buffer size
*
* @remarks
*
*
*******************************************************************************
*/
WORD32 ih264e_get_total_pic_buf_size(WORD32 pic_size,
                                     WORD32 level,
                                     WORD32 horz_pad,
                                     WORD32 vert_pad,
                                     WORD32 num_ref_frames,
                                     WORD32 num_reorder_frames)
{
    WORD32 size;
    WORD32 num_luma_samples;
    WORD32 lvl_idx;
    WORD32 max_wd, min_ht;
    WORD32 num_samples;
    WORD32 max_num_bufs;
    WORD32 pad = MAX(horz_pad, vert_pad);

    /*
     * If num_ref_frames and num_reorder_frmaes is specified
     * Use minimum value
     */
    max_num_bufs = (num_ref_frames + num_reorder_frames + MAX_CTXT_SETS);

    /* Get level index */
    lvl_idx = ih264e_get_lvl_idx(level);

    /* Maximum number of luma samples in a picture at given level */
    num_luma_samples = gai4_ih264_max_luma_pic_size[lvl_idx];
    num_luma_samples = MAX(num_luma_samples, pic_size);

    /* Account for chroma */
    num_samples = num_luma_samples * 3 / 2;

    /* Maximum width of luma samples in a picture at given level */
    max_wd = gai4_ih264_max_wd_ht[lvl_idx];

    /* Minimum height of luma samples in a picture at given level */
    min_ht = gai4_ih264_min_wd_ht[lvl_idx];

    /* Allocation is required for
     * (Wd + horz_pad) * (Ht + vert_pad) * (2 * max_dpb_size + 1)
     *
     * Above expanded as
     * ((Wd * Ht) + (horz_pad * vert_pad) + Wd * vert_pad + Ht * horz_pad) * (2 * max_dpb_size + 1)
     * (Wd * Ht) * (2 * max_dpb_size + 1) + ((horz_pad * vert_pad) + Wd * vert_pad + Ht * horz_pad) * (2 * max_dpb_size + 1)
     * Now  max_dpb_size increases with smaller Wd and Ht, but Wd * ht * max_dpb_size will still be lesser or equal to max_wd * max_ht * dpb_size
     *
     * In the above equation (Wd * Ht) * (2 * max_dpb_size + 1) is accounted by using num_samples * (2 * max_dpb_size + 1) below
     *
     * For the padded area use MAX(horz_pad, vert_pad) as pad
     * ((pad * pad) + pad * (Wd + Ht)) * (2 * max_dpb_size + 1) has to accounted from the above for padding
     *
     * Since Width and Height can change worst Wd + Ht is when One of the dimensions is max and other is min
     * So use max_wd and min_ht
     */

    /* Number of bytes in reference pictures */
    size = num_samples * max_num_bufs;

    /* Account for padding area */
    size += ((pad * pad) + pad * (max_wd + min_ht)) * 3 / 2 * max_num_bufs;

    return size;
}

/**
*******************************************************************************
*
* @brief Returns MV bank buffer size for a given number of luma samples
*
* @par Description:
*  For given number of luma samples  one MV bank size is computed.
*  Each MV bank includes pu_map and enc_pu_t for all the min PUs(4x4) in a picture
*
* @param[in] num_luma_samples
*  Max number of luma pixels in the frame
*
* @returns  Total MV Bank size
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_get_pic_mv_bank_size(WORD32 num_luma_samples)
{
    /* mv bank buffer size */
    WORD32 mv_bank_size = 0;

    /* number of sub mb partitions possible */
    WORD32 num_pu = num_luma_samples / (ENC_MIN_PU_SIZE * ENC_MIN_PU_SIZE);

    /* number of mbs */
    WORD32 num_mb = num_luma_samples / (MB_SIZE * MB_SIZE);

    /* Size for storing enc_pu_t start index each MB */
    /* One extra entry is needed to compute number of PUs in the last MB */
    mv_bank_size += num_mb * sizeof(WORD32);

    /* Size for pu_map */
    mv_bank_size += ALIGN4(num_pu);

    /* Size for storing enc_pu_t for each PU */
    mv_bank_size += ALIGN4(num_pu * sizeof(enc_pu_t));

    return mv_bank_size;
}

/**
*******************************************************************************
*
* @brief
*  Function to initialize ps_pic_buf structs add pic buffers to
*  buffer manager in case of non-shared mode
*
* @par Description:
*  Function to initialize ps_pic_buf structs add pic buffers to
*  buffer manager in case of non-shared mode
*  To be called once per stream or for every reset
*
* @param[in] ps_codec
*  Pointer to codec context
*
* @returns  error status
*
* @remarks
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_pic_buf_mgr_add_bufs(codec_t *ps_codec)
{
    /* error status */
    IH264E_ERROR_T ret = IH264E_SUCCESS;

    /* max ref buffer cnt */
    WORD32 max_num_bufs = ps_codec->i4_ref_buf_cnt;

    /* total size for pic buffers */
    WORD32 pic_buf_size_allocated = ps_codec->i4_total_pic_buf_size
                    - BUF_MGR_MAX_CNT * sizeof(pic_buf_t);

    /* temp var */
    UWORD8 *pu1_buf = (UWORD8 *) ps_codec->ps_pic_buf;
    pic_buf_t *ps_pic_buf = (pic_buf_t *) ps_codec->ps_pic_buf;
    WORD32 i;

    pu1_buf += BUF_MGR_MAX_CNT * sizeof(pic_buf_t);

    /* In case of non-shared mode, add picture buffers to buffer manager
     * In case of shared mode, buffers are added in the run-time
     */
    {
        WORD32 buf_ret;

        WORD32 luma_samples = (ps_codec->i4_rec_strd)
                        * (ps_codec->s_cfg.u4_ht + PAD_HT);

        WORD32 chroma_samples = luma_samples >> 1;

        /* Try and add as many buffers as possible for the memory that is allocated */
        /* If the number of buffers that can be added is less than max_num_bufs
         * return with an error */
        for (i = 0; i < max_num_bufs; i++)
        {
            pic_buf_size_allocated -= (luma_samples + chroma_samples);

            if (pic_buf_size_allocated < 0)
            {
                ps_codec->i4_error_code = IH264E_INSUFFICIENT_MEM_PICBUF;
                return IH264E_INSUFFICIENT_MEM_PICBUF;
            }

            ps_pic_buf->pu1_luma = pu1_buf + ps_codec->i4_rec_strd * PAD_TOP
                            + PAD_LEFT;
            pu1_buf += luma_samples;

            ps_pic_buf->pu1_chroma = pu1_buf
                            + ps_codec->i4_rec_strd * (PAD_TOP / 2)+ PAD_LEFT;
            pu1_buf += chroma_samples;

            buf_ret = ih264_buf_mgr_add((buf_mgr_t *) ps_codec->pv_ref_buf_mgr,
                                        ps_pic_buf, i);

            if (0 != buf_ret)
            {
                ps_codec->i4_error_code = IH264E_BUF_MGR_ERROR;
                return IH264E_BUF_MGR_ERROR;
            }
            pu1_buf += (HPEL_PLANES_CNT - 1) * (chroma_samples + luma_samples);
            ps_pic_buf++;
        }
    }

    return ret;
}

/**
*******************************************************************************
*
* @brief Function to add buffers to MV Bank buffer manager
*
* @par Description:
*  Function to add buffers to MV Bank buffer manager.  To be called once per
*  stream or for every reset
*
* @param[in] ps_codec
*  Pointer to codec context
*
* @returns  error status
*
* @remarks
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_mv_buf_mgr_add_bufs(codec_t *ps_codec)
{
    /* error status */
    IH264E_ERROR_T error_status = IH264E_SUCCESS;
    IH264_ERROR_T ret;

    /* max dpb size in frames */
    WORD32 max_dpb_size = 0;

    /* mv bank size for the entire dpb */
    WORD32 mv_bank_size_allocated = 0;

    /* mv bank size per pic */
    WORD32 pic_mv_bank_size = 0;

    /* mv buffer ptr */
    mv_buf_t *ps_mv_buf = NULL;

    /* num of luma samples */
    WORD32 num_luma_samples = ALIGN16(ps_codec->s_cfg.u4_wd)
                            * ALIGN16(ps_codec->s_cfg.u4_ht);

    /* number of mb's & frame partitions */
    WORD32 num_pu, num_mb;

    /* temp var */
    UWORD8 *pu1_buf = NULL;
    WORD32 i;

    /* Compute the number of MB Bank buffers needed */
    max_dpb_size = ps_codec->i4_ref_buf_cnt;

    /* allocate memory for mv buffer array */
    ps_codec->ps_mv_buf = ps_codec->pv_mv_bank_buf_base;
    pu1_buf = ps_codec->pv_mv_bank_buf_base;
    pu1_buf += BUF_MGR_MAX_CNT * sizeof(mv_buf_t);

    /********************************************************************/
    /* allocate memory for individual elements of mv buffer ptr         */
    /********************************************************************/
    mv_bank_size_allocated = ps_codec->i4_total_mv_bank_size
                    - (BUF_MGR_MAX_CNT * sizeof(mv_buf_t));

    /* compute MV bank size per picture */
    pic_mv_bank_size = ih264e_get_pic_mv_bank_size(num_luma_samples);

    num_pu = num_luma_samples / (ENC_MIN_PU_SIZE * ENC_MIN_PU_SIZE);
    num_mb = num_luma_samples / (MB_SIZE * MB_SIZE);
    i = 0;
    ps_mv_buf = ps_codec->pv_mv_bank_buf_base;

    while (i < max_dpb_size)
    {
        mv_bank_size_allocated -= pic_mv_bank_size;

        if (mv_bank_size_allocated < 0)
        {
            ps_codec->i4_error_code = IH264E_INSUFFICIENT_MEM_MVBANK;

            error_status = IH264E_INSUFFICIENT_MEM_MVBANK;

            return error_status;
        }

        ps_mv_buf->pu4_mb_pu_cnt = (UWORD32 *) pu1_buf;
        pu1_buf += num_mb * sizeof(WORD32);

        ps_mv_buf->pu1_pic_pu_map = pu1_buf;
        pu1_buf += ALIGN4(num_pu);

        ps_mv_buf->ps_pic_pu = (enc_pu_t *) (pu1_buf);
        pu1_buf += ALIGN4(num_pu * sizeof(enc_pu_t));

        ret = ih264_buf_mgr_add((buf_mgr_t *) ps_codec->pv_mv_buf_mgr,
                                ps_mv_buf, i);

        if (IH264_SUCCESS != ret)
        {
            ps_codec->i4_error_code = IH264E_BUF_MGR_ERROR;
            error_status = IH264E_BUF_MGR_ERROR;
            return error_status;
        }

        ps_mv_buf++;
        i++;
    }

    return error_status;
}

/**
*******************************************************************************
*
* @brief Function to initialize quant params structure
*
* @par Description:
*  The forward quantization modules depends on qp/6, qp mod 6, forward scale
*  matrix, forward threshold matrix, weight list. The inverse quantization
*  modules depends on qp/6, qp mod 6, inverse scale matrix, weight list.
*  These params are initialized in this function.
*
* @param[in] ps_proc
*  pointer to process context
*
* @param[in] qp
*  quantization parameter
*
* @returns none
*
* @remarks
*
*******************************************************************************
*/
void ih264e_init_quant_params(process_ctxt_t *ps_proc, int qp)
{
    /* quant params */
    quant_params_t *ps_qp_params;

    /* ptr to forward quant threshold matrix */
    const UWORD16 *pu2_thres_mat = NULL;

    /* ptr to forward scale matrix */
    const UWORD16 *pu2_scale_mat = gu2_quant_scale_matrix_4x4;

    /* ptr to inverse scale matrix */
    const UWORD16 *pu2_iscale_mat = gau2_ih264_iquant_scale_matrix_4x4;

    /* temp var */
    UWORD32 u4_qp[3], u4_qp_div6, u4_qp_mod6;
    COMPONENT_TYPE plane;
    WORD32 i;
    UWORD32 u4_satdq_t;
    const UWORD16 *pu2_smat;

    /********************************************************************/
    /* init quant params for all planes Y, U and V                      */
    /********************************************************************/
    /* luma qp */
    u4_qp[Y] = qp;

    /* chroma qp
     * TODO_LATER : just in case if the chroma planes use different qp's this
     * needs to be corrected accordingly.
     */
    u4_qp[U] = gu1_qpc_fqpi[qp];
    u4_qp[V] = gu1_qpc_fqpi[qp];

    plane = Y;
    while (plane <= V)
    {
        u4_qp_div6 = (u4_qp[plane] / 6);
        u4_qp_mod6 = (u4_qp[plane] % 6);

        ps_qp_params = ps_proc->ps_qp_params[plane];

        /* mb qp */
        ps_qp_params->u1_mb_qp = u4_qp[plane];

        /* mb qp / 6 */
        ps_qp_params->u1_qp_div = u4_qp_div6;

        /* mb qp % 6 */
        ps_qp_params->u1_qp_rem = u4_qp_mod6;

        /* QP bits */
        ps_qp_params->u1_qbits = QP_BITS_h264_4x4 + u4_qp_div6;

        /* forward scale matrix */
        ps_qp_params->pu2_scale_mat = pu2_scale_mat + (u4_qp_mod6 * 16);

        /* threshold matrix & weight for quantization */
        pu2_thres_mat = gu2_forward_quant_threshold_4x4 + (u4_qp_mod6 * 16);
        for (i = 0; i < 16; i++)
        {
            ps_qp_params->pu2_thres_mat[i] = pu2_thres_mat[i]
                            >> (8 - u4_qp_div6);
            ps_qp_params->pu2_weigh_mat[i] = 16;
        }

        /* qp dependent rounding constant */
        ps_qp_params->u4_dead_zone =
                        gu4_forward_quant_round_factor_4x4[u4_qp_div6];

        /* slice dependent rounding constant */
        if (ps_proc->i4_slice_type != ISLICE
                        && ps_proc->i4_slice_type != SISLICE)
        {
            ps_qp_params->u4_dead_zone >>= 1;
        }

        /* SATQD threshold for zero block prediction */
        if (ps_proc->ps_codec->s_cfg.u4_enable_satqd)
        {
            pu2_smat = ps_qp_params->pu2_scale_mat;

            u4_satdq_t = ((1 << (ps_qp_params->u1_qbits)) - ps_qp_params->u4_dead_zone);

            ps_qp_params->pu2_sad_thrsh[0] = u4_satdq_t / MAX(pu2_smat[3], pu2_smat[11]);
            ps_qp_params->pu2_sad_thrsh[1] = u4_satdq_t / MAX(pu2_smat[1], pu2_smat[9]);
            ps_qp_params->pu2_sad_thrsh[2] = u4_satdq_t / pu2_smat[15];
            ps_qp_params->pu2_sad_thrsh[3] = u4_satdq_t / pu2_smat[7];
            ps_qp_params->pu2_sad_thrsh[4] = u4_satdq_t / MAX(pu2_smat[12], pu2_smat[14]);
            ps_qp_params->pu2_sad_thrsh[5] = u4_satdq_t / MAX(pu2_smat[4], pu2_smat[6]);
            ps_qp_params->pu2_sad_thrsh[6] = u4_satdq_t / pu2_smat[13];
            ps_qp_params->pu2_sad_thrsh[7] = u4_satdq_t / pu2_smat[5];
            ps_qp_params->pu2_sad_thrsh[8] = u4_satdq_t / MAX(MAX3(pu2_smat[0], pu2_smat[2], pu2_smat[8]), pu2_smat[10]);
        }

        /* inverse scale matrix */
        ps_qp_params->pu2_iscale_mat = pu2_iscale_mat + (u4_qp_mod6 * 16);

        plane += 1;
    }
    return ;
}

/**
*******************************************************************************
*
* @brief
*  Initialize AIR mb frame Map
*
* @par Description:
*  Initialize AIR mb frame map
*  MB frame map indicates which frame an Mb should be coded as intra according to AIR
*
* @param[in] ps_codec
*  Pointer to codec context
*
* @returns  error_status
*
* @remarks
*
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_init_air_map(codec_t *ps_codec)
{
    /* intra refresh map */
    UWORD16 *pu2_intr_rfrsh_map = ps_codec->pu2_intr_rfrsh_map;

    /* air mode */
    IVE_AIR_MODE_T air_mode = ps_codec->s_cfg.e_air_mode;

    /* refresh period */
    UWORD32 air_period = ps_codec->s_cfg.u4_air_refresh_period;

    /* mb cnt */
    UWORD32 u4_mb_cnt = ps_codec->s_cfg.i4_wd_mbs * ps_codec->s_cfg.i4_ht_mbs;

    /* temp var */
    UWORD32 curr_mb, seed_rand = 1;

    switch (air_mode)
    {
        case IVE_AIR_MODE_CYCLIC:

            for (curr_mb = 0; curr_mb < u4_mb_cnt; curr_mb++)
            {
                pu2_intr_rfrsh_map[curr_mb] = curr_mb % air_period;
            }
            break;

        case IVE_AIR_MODE_RANDOM:

            for (curr_mb = 0; curr_mb < u4_mb_cnt; curr_mb++)
            {
                seed_rand = (seed_rand * 32719 + 3) % 32749;
                pu2_intr_rfrsh_map[curr_mb] = seed_rand % air_period;
            }
            break;

        default:

            break;
    }

    return IH264E_SUCCESS;
}

/**
*******************************************************************************
*
* @brief
*  Codec level initializations
*
* @par Description:
*  Initializes the codec with parameters that needs to be set before encoding
*  first frame
*
* @param[in] ps_codec
*  Pointer to codec context
*
* @param[in] ps_inp_buf
*  Pointer to input buffer context
*
* @returns  error_status
*
* @remarks
*
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_codec_init(codec_t *ps_codec)
{
    /********************************************************************
     *                     INITIALIZE CODEC CONTEXT                     *
     ********************************************************************/
    /* encoder presets */
    if (ps_codec->s_cfg.u4_enc_speed_preset != IVE_CONFIG)
    {
        if (ps_codec->s_cfg.u4_enc_speed_preset == IVE_SLOWEST)
        {/* high quality */
            /* enable diamond search */
            ps_codec->s_cfg.u4_me_speed_preset = DMND_SRCH;
            ps_codec->s_cfg.u4_enable_fast_sad = 0;

            /* disable intra 4x4 */
            ps_codec->s_cfg.u4_enable_intra_4x4 = 1;
            ps_codec->luma_energy_compaction[1] =
                            ih264e_code_luma_intra_macroblock_4x4_rdopt_on;

            /* sub pel off */
            ps_codec->s_cfg.u4_enable_hpel = 1;

            /* deblocking off */
            ps_codec->s_cfg.u4_disable_deblock_level = DISABLE_DEBLK_LEVEL_0;

            /* disabled intra inter gating in Inter slices */
            ps_codec->u4_inter_gate = 0;
        }
        else if (ps_codec->s_cfg.u4_enc_speed_preset == IVE_NORMAL)
        {/* normal */
            /* enable diamond search */
            ps_codec->s_cfg.u4_me_speed_preset = DMND_SRCH;
            ps_codec->s_cfg.u4_enable_fast_sad = 0;

            /* disable intra 4x4 */
            ps_codec->s_cfg.u4_enable_intra_4x4 = 1;

            /* sub pel off */
            ps_codec->s_cfg.u4_enable_hpel = 1;

            /* deblocking off */
            ps_codec->s_cfg.u4_disable_deblock_level = DISABLE_DEBLK_LEVEL_0;

            /* disabled intra inter gating in Inter slices */
            ps_codec->u4_inter_gate = 0;
        }
        else if (ps_codec->s_cfg.u4_enc_speed_preset == IVE_FAST)
         {/* normal */
             /* enable diamond search */
             ps_codec->s_cfg.u4_me_speed_preset = DMND_SRCH;
             ps_codec->s_cfg.u4_enable_fast_sad = 0;

             /* disable intra 4x4 */
             ps_codec->s_cfg.u4_enable_intra_4x4 = 0;

             /* sub pel off */
             ps_codec->s_cfg.u4_enable_hpel = 1;

             /* deblocking off */
             ps_codec->s_cfg.u4_disable_deblock_level = DISABLE_DEBLK_LEVEL_0;

             /* disabled intra inter gating in Inter slices */
             ps_codec->u4_inter_gate = 1;
         }
        else if (ps_codec->s_cfg.u4_enc_speed_preset == IVE_HIGH_SPEED)
        {/* fast */
            /* enable diamond search */
            ps_codec->s_cfg.u4_me_speed_preset = DMND_SRCH;
            ps_codec->s_cfg.u4_enable_fast_sad = 0;

            /* disable intra 4x4 */
            ps_codec->s_cfg.u4_enable_intra_4x4 = 0;

            /* sub pel off */
            ps_codec->s_cfg.u4_enable_hpel = 0;

            /* deblocking off */
            ps_codec->s_cfg.u4_disable_deblock_level = DISABLE_DEBLK_LEVEL_4;

            /* disabled intra inter gating in Inter slices */
            ps_codec->u4_inter_gate = 0;
        }
        else if (ps_codec->s_cfg.u4_enc_speed_preset == IVE_FASTEST)
        {/* fastest */
            /* enable diamond search */
            ps_codec->s_cfg.u4_me_speed_preset = DMND_SRCH;

            /* disable intra 4x4 */
            ps_codec->s_cfg.u4_enable_intra_4x4 = 0;

            /* sub pel off */
            ps_codec->s_cfg.u4_enable_hpel = 0;

            /* deblocking off */
            ps_codec->s_cfg.u4_disable_deblock_level = DISABLE_DEBLK_LEVEL_4;

            /* disabled intra inter gating in Inter slices */
            ps_codec->u4_inter_gate = 1;
        }
    }

    /*****************************************************************
     * Initialize AIR inside codec
     *****************************************************************/
    if (IVE_AIR_MODE_NONE != ps_codec->s_cfg.e_air_mode)
    {
        ih264e_init_air_map(ps_codec);

        ps_codec->i4_air_pic_cnt = -1;
    }

    /****************************************************/
    /*           INITIALIZE RATE CONTROL                */
    /****************************************************/
    {
        /* init qp */
        UWORD8 au1_init_qp[MAX_PIC_TYPE];

        /* min max qp */
        UWORD8 au1_min_max_qp[2 * MAX_PIC_TYPE];

        /* init i,p,b qp */
        au1_init_qp[0] = gau1_h264_to_mpeg2_qmap[ps_codec->s_cfg.u4_i_qp];
        au1_init_qp[1] = gau1_h264_to_mpeg2_qmap[ps_codec->s_cfg.u4_p_qp];
        au1_init_qp[2] = gau1_h264_to_mpeg2_qmap[ps_codec->s_cfg.u4_b_qp];

        /* init min max qp */
        au1_min_max_qp[2 * I_PIC] =
                        gau1_h264_to_mpeg2_qmap[ps_codec->s_cfg.u4_i_qp_min];
        au1_min_max_qp[2 * I_PIC + 1] =
                        gau1_h264_to_mpeg2_qmap[ps_codec->s_cfg.u4_i_qp_max];

        au1_min_max_qp[2 * P_PIC] =
                        gau1_h264_to_mpeg2_qmap[ps_codec->s_cfg.u4_p_qp_min];
        au1_min_max_qp[2 * P_PIC + 1] =
                        gau1_h264_to_mpeg2_qmap[ps_codec->s_cfg.u4_p_qp_max];

        au1_min_max_qp[2 * B_PIC] =
                        gau1_h264_to_mpeg2_qmap[ps_codec->s_cfg.u4_b_qp_min];
        au1_min_max_qp[2 * B_PIC + 1] =
                        gau1_h264_to_mpeg2_qmap[ps_codec->s_cfg.u4_b_qp_max];

        /* get rc mode */
        switch (ps_codec->s_cfg.e_rc_mode)
        {
            case IVE_RC_STORAGE:
                ps_codec->s_rate_control.e_rc_type = VBR_STORAGE;
                break;
            case IVE_RC_CBR_NON_LOW_DELAY:
                ps_codec->s_rate_control.e_rc_type = CBR_NLDRC;
                break;
            case IVE_RC_CBR_LOW_DELAY:
                ps_codec->s_rate_control.e_rc_type = CBR_LDRC;
                break;
            case IVE_RC_NONE:
                ps_codec->s_rate_control.e_rc_type = CONST_QP;
                break;
            default:
                break;
        }

        /* init rate control */
        ih264e_rc_init(ps_codec->s_rate_control.pps_rate_control_api,
                       ps_codec->s_rate_control.pps_frame_time,
                       ps_codec->s_rate_control.pps_time_stamp,
                       ps_codec->s_rate_control.pps_pd_frm_rate,
                       ps_codec->s_cfg.u4_max_framerate,
                       ps_codec->s_cfg.u4_src_frame_rate,
                       ps_codec->s_cfg.u4_tgt_frame_rate,
                       ps_codec->s_rate_control.e_rc_type,
                       ps_codec->s_cfg.u4_target_bitrate,
                       ps_codec->s_cfg.u4_max_bitrate,
                       ps_codec->s_cfg.u4_vbv_buffer_delay,
                       ps_codec->s_cfg.u4_i_frm_interval,
                       ps_codec->s_cfg.u4_num_bframes + 1, au1_init_qp,
                       ps_codec->s_cfg.u4_num_bframes + 2 , au1_min_max_qp,
                       MAX(ps_codec->s_cfg.u4_max_level,
                               (UWORD32)ih264e_get_min_level(ps_codec->s_cfg.u4_max_wd, ps_codec->s_cfg.u4_max_ht)));
    }

    /* recon stride */
    ps_codec->i4_rec_strd = ALIGN16(ps_codec->s_cfg.u4_max_wd) + PAD_WD;

    /* max ref and reorder cnt */
    ps_codec->i4_ref_buf_cnt = ps_codec->s_cfg.u4_max_ref_cnt
                    + ps_codec->s_cfg.u4_max_reorder_cnt;
    ps_codec->i4_ref_buf_cnt += MAX_CTXT_SETS;

    DEBUG_HISTOGRAM_INIT();


    /* Init dependecy vars */
    ps_codec->i4_last_inp_buff_received = 0;

    /* At codec start no IDR is pending */
    ps_codec->i4_pending_idr_flag = 0;

    return IH264E_SUCCESS;
}

/**
*******************************************************************************
*
* @brief
*  Picture level initializations
*
* @par Description:
*  Before beginning to encode the frame, the current function initializes all
*  the ctxts (proc, entropy, me, ...) basing on the input configured params.
*  It locates space for storing recon in the encoder picture buffer set, fetches
*  reference frame from encoder picture buffer set. Calls RC pre-enc to get
*  qp and pic type for the current frame. Queues proc jobs so that
*  the other threads can begin encoding. In brief, this function sets up the
*  tone for the entire encoder.
*
* @param[in] ps_codec
*  Pointer to codec context
*
* @param[in] ps_inp_buf
*  Pointer to input buffer context
*
* @returns  error_status
*
* @remarks
*
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_pic_init(codec_t *ps_codec, inp_buf_t *ps_inp_buf)
{
    /* error status */
    IH264E_ERROR_T error_status = IH264E_SUCCESS;
    IH264_ERROR_T ret = IH264_SUCCESS;

    /* mv buff bank */
    mv_buf_t *ps_mv_buf = NULL;
    WORD32 cur_mv_bank_buf_id;

    /* recon buffer set */
    pic_buf_t *ps_cur_pic;
    WORD32 cur_pic_buf_id;
    UWORD8 *pu1_cur_pic_luma, *pu1_cur_pic_chroma;

    /* ref buffer set */
    pic_buf_t *aps_ref_pic[MAX_REF_PIC_CNT] = {NULL, NULL};
    mv_buf_t *aps_mv_buf[MAX_REF_PIC_CNT] = {NULL, NULL};
    WORD32 ref_set_id;

    /* pic time stamp */
    UWORD32 u4_timestamp_high = ps_inp_buf->u4_timestamp_high;
    UWORD32 u4_timestamp_low = ps_inp_buf->u4_timestamp_low;

    /* indices to access curr/prev frame info */
    WORD32 ctxt_sel = ps_codec->i4_encode_api_call_cnt % MAX_CTXT_SETS;

    /* curr pic type */
    PIC_TYPE_T *pic_type = &ps_codec->pic_type;

    /* Diamond search Iteration Max Cnt */
    UWORD32 u4_num_layers =
                    (ps_codec->s_cfg.u4_enc_speed_preset == IVE_FASTEST) ?
                                    (NUM_LAYERS >> 2) : NUM_LAYERS;

    /* enable fast sad */
    UWORD32 u4_enable_fast_sad = ps_codec->s_cfg.u4_enable_fast_sad;

    /********************************************************************/
    /*                     INITIALIZE CODEC CONTEXT                     */
    /********************************************************************/
    /* slice_type */
    if ((PIC_I == *pic_type) || (PIC_IDR == *pic_type))
    {
        ps_codec->i4_slice_type = ISLICE;
    }
    else if (PIC_P == *pic_type)
    {
        ps_codec->i4_slice_type = PSLICE;
    }
    else if(PIC_B == *pic_type)
    {
        ps_codec->i4_slice_type = BSLICE;
    }


    /***************************************************************************
     * Set up variables for sending frame number, poc and reference
     *   a) Set up alt ref too
     **************************************************************************/

    /* Check and set if the current frame is reference or not */
    ps_codec->u4_is_curr_frm_ref = 0;

    /* This frame is reference if its not a B pic, pending approval from alt ref */
    ps_codec->u4_is_curr_frm_ref = (*pic_type != PIC_B);

    /* In case if its a P pic, we will decide according to alt ref also */
    if (ps_codec->s_cfg.u4_enable_alt_ref && (*pic_type == PIC_P)
                    && (ps_codec->i4_pic_cnt
                                    % (ps_codec->s_cfg.u4_enable_alt_ref + 1)))
    {
        ps_codec->u4_is_curr_frm_ref = 0;
    }

    /*
     * Override everything in case of IDR
     * Note that in case of IDR, at this point ps_codec->u4_is_curr_frm_ref must
     * be 1
     */

    /* is this an IDR pic */
    ps_codec->u4_is_idr = 0;

    if (PIC_IDR == *pic_type)
    {
        /* set idr flag */
        ps_codec->u4_is_idr = 1;

        /* reset frame num */
        ps_codec->i4_frame_num = 0;

        /* idr_pic_id */
        ps_codec->i4_idr_pic_id++;
    }

    /***************************************************************************
     * Set up Deblock
     **************************************************************************/

    /* set deblock disable flags based on disable deblock level */
    ps_codec->i4_disable_deblk_pic = 1;

    if (ps_codec->s_cfg.u4_disable_deblock_level == DISABLE_DEBLK_LEVEL_0)
    {
        /* enable deblocking */
        ps_codec->i4_disable_deblk_pic = 0;
    }
    else if (ps_codec->s_cfg.u4_disable_deblock_level == DISABLE_DEBLK_LEVEL_2)
    {
        /* enable deblocking after a period of frames */
        if (ps_codec->i4_disable_deblk_pic_cnt == DISABLE_DEBLOCK_INTERVAL
                        || ps_codec->i4_slice_type == ISLICE)
        {
            ps_codec->i4_disable_deblk_pic = 0;
        }
    }
    else if (ps_codec->s_cfg.u4_disable_deblock_level == DISABLE_DEBLK_LEVEL_3)
    {
        if (ps_codec->i4_slice_type == ISLICE)
        {
            ps_codec->i4_disable_deblk_pic = 0;
        }
    }

    if (ps_codec->i4_disable_deblk_pic)
    {
        ps_codec->i4_disable_deblk_pic_cnt++;
    }
    else
    {
        ps_codec->i4_disable_deblk_pic_cnt = 0;
    }

    /* In slice mode - lets not deblk mb edges that lie along slice boundaries */
    if (ps_codec->i4_disable_deblk_pic == 0)
    {
        if (ps_codec->s_cfg.e_slice_mode != IVE_SLICE_MODE_NONE)
        {
            ps_codec->i4_disable_deblk_pic = 2;
        }
    }

    /* error status */
    ps_codec->i4_error_code = IH264E_SUCCESS;

    /* populate header */
    if (ps_codec->i4_gen_header)
    {
        /* sps */
        sps_t *ps_sps = NULL;

        /* pps */
        pps_t *ps_pps = NULL;

        /*ps_codec->i4_pps_id ++;*/
        ps_codec->i4_pps_id %= MAX_PPS_CNT;

        /*ps_codec->i4_sps_id ++;*/
        ps_codec->i4_sps_id %= MAX_SPS_CNT;

        /* populate sps header */
        ps_sps = ps_codec->ps_sps_base + ps_codec->i4_sps_id;
        ih264e_populate_sps(ps_codec, ps_sps);

        /* populate pps header */
        ps_pps = ps_codec->ps_pps_base + ps_codec->i4_pps_id;
        ih264e_populate_pps(ps_codec, ps_pps);
    }

    /***************************************************************************
     *  Reference and MV bank Buffer Manager
     *  Here we will
     *      1) Find the correct ref pics for the current frame
     *      2) Free the ref pic that is not going to be used anywhere
     *      3) Find a free buff from the list and assign it as the recon of
     *         current frame
     *
     *  1) Finding correct ref pic
     *      All pics needed for future are arranged in a picture list called
     *      ps_codec->as_ref_set. Each picture in this will have a pic buffer and
     *      MV buffer that is marked appropriately as BUF_MGR_REF, BUF_MGR_IO or
     *      BUF_MGR_CODEC. Also the pic_cnt and poc will also be present.
     *      Hence to find the ref pic we will loop through the list and find
     *      2 pictures with maximum i4_pic_cnt .
     *
     *      note that i4_pic_cnt == -1 is used to filter uninit ref pics.
     *      Now since we only have max two ref pics, we will always find max 2
     *      ref pics.

     *
     *  2) 3) Self explanatory
     ***************************************************************************/
    {
        /* Search for buffs with maximum pic cnt */

        WORD32 max_pic_cnt[] = { -1, -1 };

        mv_buf_t *ps_mv_buf_to_free[] = { NULL, NULL };

        /* temp var */
        WORD32 i, buf_status;

        for (i = 0; i < ps_codec->i4_ref_buf_cnt; i++)
        {
            if (ps_codec->as_ref_set[i].i4_pic_cnt == -1)
                continue;

            buf_status = ih264_buf_mgr_get_status(
                            ps_codec->pv_ref_buf_mgr,
                            ps_codec->as_ref_set[i].ps_pic_buf->i4_buf_id);

            /* Ideally we should look for buffer status of MV BUFF also. But since
             * the correponding MV buffs also will be at the same state. It dosent
             * matter as of now. But the check will make the logic better */
            if ((max_pic_cnt[0] < ps_codec->as_ref_set[i].i4_pic_cnt)
                            && (buf_status & BUF_MGR_REF))
            {
                if (max_pic_cnt[1] < ps_codec->as_ref_set[i].i4_pic_cnt)
                {
                    max_pic_cnt[0] = max_pic_cnt[1];
                    aps_ref_pic[0] = aps_ref_pic[1];
                    aps_mv_buf[0] = aps_mv_buf[1];

                    ps_mv_buf_to_free[0] = ps_mv_buf_to_free[1];

                    max_pic_cnt[1] = ps_codec->as_ref_set[i].i4_pic_cnt;
                    aps_ref_pic[1] = ps_codec->as_ref_set[i].ps_pic_buf;
                    aps_mv_buf[1] = ps_codec->as_ref_set[i].ps_mv_buf;
                    ps_mv_buf_to_free[1] = ps_codec->as_ref_set[i].ps_mv_buf;

                }
                else
                {
                    max_pic_cnt[0] = ps_codec->as_ref_set[i].i4_pic_cnt;
                    aps_ref_pic[0] = ps_codec->as_ref_set[i].ps_pic_buf;
                    aps_mv_buf[0] = ps_codec->as_ref_set[i].ps_mv_buf;
                    ps_mv_buf_to_free[0] = ps_codec->as_ref_set[i].ps_mv_buf;
                }
            }
        }

        /*
         * Now if the current picture is I or P, we discard the back ref pic and
         * assign forward ref as backward ref
         */
        if (*pic_type != PIC_B)
        {
            if (ps_mv_buf_to_free[0])
            {
                /* release this frame from reference list */
                ih264_buf_mgr_release(ps_codec->pv_mv_buf_mgr,
                                      ps_mv_buf_to_free[0]->i4_buf_id,
                                      BUF_MGR_REF);

                ih264_buf_mgr_release(ps_codec->pv_ref_buf_mgr,
                                      aps_ref_pic[0]->i4_buf_id, BUF_MGR_REF);
            }

            max_pic_cnt[0] = max_pic_cnt[1];
            aps_ref_pic[0] = aps_ref_pic[1];
            aps_mv_buf[0] = aps_mv_buf[1];

            /* Dummy */
            max_pic_cnt[1] = -1;
        }

        /*
         * Mark all reference pic with unused buffers to be free
         * We need this step since each one, ie ref, recon io etc only unset their
         * respective flags. Hence we need to combine togather and mark the ref set
         * accordingly
         */
        ref_set_id = -1;
        for (i = 0; i < ps_codec->i4_ref_buf_cnt; i++)
        {
            if (ps_codec->as_ref_set[i].i4_pic_cnt == -1)
            {
                ref_set_id = i;
                continue;
            }

            buf_status = ih264_buf_mgr_get_status(
                            ps_codec->pv_ref_buf_mgr,
                            ps_codec->as_ref_set[i].ps_pic_buf->i4_buf_id);

            if ((buf_status & (BUF_MGR_REF | BUF_MGR_CODEC | BUF_MGR_IO)) == 0)
            {
                ps_codec->as_ref_set[i].i4_pic_cnt = -1;
                ps_codec->as_ref_set[i].i4_poc = 32768;

                ref_set_id = i;
            }
        }
        /* An asssert failure here means we donot have any free buffs */
        ASSERT(ref_set_id >= 0);
    }

    {
        /*****************************************************************/
        /* Get free MV Bank to hold current picture's motion vector data */
        /* If there are no free buffers then return with an error code.  */
        /* If the buffer is to be freed by another thread, change the    */
        /* following to call thread yield and wait for buffer to be freed*/
        /*****************************************************************/
        ps_mv_buf = (mv_buf_t *) ih264_buf_mgr_get_next_free(
                        (buf_mgr_t *) ps_codec->pv_mv_buf_mgr,
                        &cur_mv_bank_buf_id);

        if (NULL == ps_mv_buf)
        {
            ps_codec->i4_error_code = IH264E_NO_FREE_MVBANK;
            return IH264E_NO_FREE_MVBANK;
        }

        /* mark the buffer as needed for reference if the curr pic is available for ref */
        if (ps_codec->u4_is_curr_frm_ref)
        {
            ih264_buf_mgr_set_status(ps_codec->pv_mv_buf_mgr,
                                     cur_mv_bank_buf_id, BUF_MGR_REF);
        }

        /* Set current ABS poc to ps_mv_buf, so that while freeing a reference buffer
         * corresponding mv buffer can be found by looping through ps_codec->ps_mv_buf array
         * and getting a buffer id to free
         */
        ps_mv_buf->i4_abs_poc = ps_codec->i4_abs_pic_order_cnt;
        ps_mv_buf->i4_buf_id = cur_mv_bank_buf_id;
    }

    {
        /*****************************************************************/
        /* Get free pic buf to hold current picture's recon data         */
        /* If there are no free buffers then return with an error code.  */
        /* If the buffer is to be freed by another thread, change the    */
        /* following to call thread yield and wait for buffer to be freed*/
        /*****************************************************************/
        ps_cur_pic = (pic_buf_t *) ih264_buf_mgr_get_next_free(
                        (buf_mgr_t *) ps_codec->pv_ref_buf_mgr,
                        &cur_pic_buf_id);

        if (NULL == ps_cur_pic)
        {
            ps_codec->i4_error_code = IH264E_NO_FREE_PICBUF;
            return IH264E_NO_FREE_PICBUF;
        }

        /* mark the buffer as needed for reference if the curr pic is available for ref */
        if (ps_codec->u4_is_curr_frm_ref)
        {
            ih264_buf_mgr_set_status(ps_codec->pv_ref_buf_mgr, cur_pic_buf_id,
                                     BUF_MGR_REF);
        }

        /* Mark the current buffer as needed for IO if recon is enabled */
        if (1 == ps_codec->s_cfg.u4_enable_recon)
        {
            ih264_buf_mgr_set_status(ps_codec->pv_ref_buf_mgr, cur_pic_buf_id,
                                     BUF_MGR_IO);
        }

        /* Associate input timestamp with current buffer */
        ps_cur_pic->u4_timestamp_high = ps_inp_buf->u4_timestamp_high;
        ps_cur_pic->u4_timestamp_low = ps_inp_buf->u4_timestamp_low;

        ps_cur_pic->i4_abs_poc = ps_codec->i4_poc;
        ps_cur_pic->i4_poc_lsb = ps_codec->i4_pic_order_cnt_lsb;

        ps_cur_pic->i4_buf_id = cur_pic_buf_id;

        pu1_cur_pic_luma = ps_cur_pic->pu1_luma;
        pu1_cur_pic_chroma = ps_cur_pic->pu1_chroma;
    }

    /*
     * Add the current picture to ref list independent of the fact that it is used
     * as reference or not. This is because, now recon is not in sync with output
     * hence we may need the current recon after some delay. By adding it to ref list
     * we can retrieve the recon any time we want. The information that it is used
     * for ref can still be found by checking the buffer status of pic buf.
     */
    {
        ps_codec->as_ref_set[ref_set_id].i4_pic_cnt = ps_codec->i4_pic_cnt;
        ps_codec->as_ref_set[ref_set_id].i4_poc = ps_codec->i4_poc;
        ps_codec->as_ref_set[ref_set_id].ps_mv_buf = ps_mv_buf;
        ps_codec->as_ref_set[ref_set_id].ps_pic_buf = ps_cur_pic;
    }

    /********************************************************************/
    /*                     INITIALIZE PROCESS CONTEXT                   */
    /********************************************************************/
    {
        /* temp var */
        WORD32 i, j = 0;

        /* curr proc ctxt */
        process_ctxt_t *ps_proc = NULL;

        j = ctxt_sel * MAX_PROCESS_THREADS;

        /* begin init */
        for (i = j; i < (j + MAX_PROCESS_THREADS); i++)
        {
            ps_proc = &ps_codec->as_process[i];

            /* luma src buffer */
            if (ps_codec->s_cfg.e_inp_color_fmt == IV_YUV_422ILE)
            {
                ps_proc->pu1_src_buf_luma_base = ps_codec->pu1_y_csc_buf_base;
            }
            else
            {
                ps_proc->pu1_src_buf_luma_base =
                                ps_inp_buf->s_raw_buf.apv_bufs[0];
            }

            /* chroma src buffer */
            if (ps_codec->s_cfg.e_inp_color_fmt == IV_YUV_422ILE
                            || ps_codec->s_cfg.e_inp_color_fmt == IV_YUV_420P)
            {
                ps_proc->pu1_src_buf_chroma_base =
                                ps_codec->pu1_uv_csc_buf_base;
            }
            else
            {
                ps_proc->pu1_src_buf_chroma_base =
                                ps_inp_buf->s_raw_buf.apv_bufs[1];
            }

            /* luma rec buffer */
            ps_proc->pu1_rec_buf_luma_base = pu1_cur_pic_luma;

            /* chroma rec buffer */
            ps_proc->pu1_rec_buf_chroma_base = pu1_cur_pic_chroma;

            /* rec stride */
            ps_proc->i4_rec_strd = ps_codec->i4_rec_strd;

            /* frame num */
            ps_proc->i4_frame_num = ps_codec->i4_frame_num;

            /* is idr */
            ps_proc->u4_is_idr = ps_codec->u4_is_idr;

            /* idr pic id */
            ps_proc->u4_idr_pic_id = ps_codec->i4_idr_pic_id;

            /* slice_type */
            ps_proc->i4_slice_type = ps_codec->i4_slice_type;

            /* Input width in mbs */
            ps_proc->i4_wd_mbs = ps_codec->s_cfg.i4_wd_mbs;

            /* Input height in mbs */
            ps_proc->i4_ht_mbs = ps_codec->s_cfg.i4_ht_mbs;

            /* Half x plane offset from pic buf */
            ps_proc->u4_half_x_offset = 0;

            /* Half y plane offset from half x plane */
            ps_proc->u4_half_y_offset = 0;

            /* Half x plane offset from half y plane */
            ps_proc->u4_half_xy_offset = 0;

            /* top row syntax elements */
            ps_proc->ps_top_row_mb_syntax_ele =
                            ps_proc->ps_top_row_mb_syntax_ele_base;

            ps_proc->pu1_top_mb_intra_modes =
                            ps_proc->pu1_top_mb_intra_modes_base;

            ps_proc->ps_top_row_pu = ps_proc->ps_top_row_pu_base;

            /* initialize quant params */
            ps_proc->u4_frame_qp = ps_codec->u4_frame_qp;
            ps_proc->u4_mb_qp = ps_codec->u4_frame_qp;
            ih264e_init_quant_params(ps_proc, ps_proc->u4_frame_qp);

            /* previous mb qp*/
            ps_proc->u4_mb_qp_prev = ps_proc->u4_frame_qp;

            /* Reset frame info */
            memset(&ps_proc->s_frame_info, 0, sizeof(frame_info_t));

            /* initialize proc, deblk and ME map */
            if (i == j)
            {
                /* row '-1' */
                memset(ps_proc->pu1_proc_map - ps_proc->i4_wd_mbs, 1, ps_proc->i4_wd_mbs);
                /* row 0 to ht in mbs */
                memset(ps_proc->pu1_proc_map, 0, ps_proc->i4_wd_mbs * ps_proc->i4_ht_mbs);

                /* row '-1' */
                memset(ps_proc->pu1_deblk_map - ps_proc->i4_wd_mbs, 1, ps_proc->i4_wd_mbs);
                /* row 0 to ht in mbs */
                memset(ps_proc->pu1_deblk_map, 0, ps_proc->i4_wd_mbs * ps_proc->i4_ht_mbs);

                /* row '-1' */
                memset(ps_proc->pu1_me_map - ps_proc->i4_wd_mbs, 1, ps_proc->i4_wd_mbs);
                /* row 0 to ht in mbs */
                memset(ps_proc->pu1_me_map, 0, ps_proc->i4_wd_mbs * ps_proc->i4_ht_mbs);

                /* at the start of air refresh period, reset intra coded map */
                if (IVE_AIR_MODE_NONE != ps_codec->s_cfg.e_air_mode)
                {
                    ps_codec->i4_air_pic_cnt = (ps_codec->i4_air_pic_cnt + 1)
                                    % ps_codec->s_cfg.u4_air_refresh_period;

                    if (!ps_codec->i4_air_pic_cnt)
                    {
                        memset(ps_proc->pu1_is_intra_coded, 0, ps_proc->i4_wd_mbs * ps_proc->i4_ht_mbs);
                    }
                }
            }

            /* deblock level */
            ps_proc->u4_disable_deblock_level = ps_codec->i4_disable_deblk_pic;

            /* slice index map */
            /* no slice */
            if (ps_codec->s_cfg.e_slice_mode == IVE_SLICE_MODE_NONE)
            {
                memset(ps_proc->pu1_slice_idx, 0, ps_proc->i4_wd_mbs * ps_proc->i4_ht_mbs);
            }
            /* generate slices for every 'n' rows, 'n' is given through slice param */
            else if (ps_codec->s_cfg.e_slice_mode == IVE_SLICE_MODE_BLOCKS)
            {
                /* slice idx map */
                UWORD8 *pu1_slice_idx = ps_proc->pu1_slice_idx;

                /* temp var */
                WORD32 i4_mb_y = 0, slice_idx = 0, cnt;

                while (i4_mb_y < ps_proc->i4_ht_mbs)
                {
                    if (i4_mb_y +(WORD32)ps_codec->s_cfg.u4_slice_param < ps_proc->i4_ht_mbs)
                    {
                        cnt = ps_codec->s_cfg.u4_slice_param * ps_proc->i4_wd_mbs;
                        i4_mb_y += ps_codec->s_cfg.u4_slice_param;
                    }
                    else
                    {
                        cnt = (ps_proc->i4_ht_mbs - i4_mb_y) * ps_proc->i4_wd_mbs;
                        i4_mb_y += (ps_proc->i4_ht_mbs - i4_mb_y);
                    }
                    memset(pu1_slice_idx, slice_idx, cnt);
                    slice_idx++;
                    pu1_slice_idx += cnt;
                }
            }

            /* Current MV Bank's buffer ID */
            ps_proc->i4_cur_mv_bank_buf_id = cur_mv_bank_buf_id;

            /* Pointer to current picture buffer structure */
            ps_proc->ps_cur_pic = ps_cur_pic;

            /* Pointer to current pictures mv buffers */
            ps_proc->ps_cur_mv_buf = ps_mv_buf;

            /*
             * pointer to ref picture
             * 0    : Temporal back reference
             * 1    : Temporal forward reference
             */
            ps_proc->aps_ref_pic[PRED_L0] = aps_ref_pic[PRED_L0];
            ps_proc->aps_ref_pic[PRED_L1] = aps_ref_pic[PRED_L1];
            if (ps_codec->pic_type == PIC_B)
            {
                ps_proc->aps_mv_buf[PRED_L0] = aps_mv_buf[PRED_L0];
                ps_proc->aps_mv_buf[PRED_L1] = aps_mv_buf[PRED_L1];
            }
            else
            {
                /*
                 * Else is dummy since for non B pic we does not need this
                 * But an assignment here will help in not having a segfault
                 * when we calcualte colpic in P slices
                 */
                ps_proc->aps_mv_buf[PRED_L0] = ps_mv_buf;
                ps_proc->aps_mv_buf[PRED_L1] = ps_mv_buf;
            }

            if ((*pic_type != PIC_IDR) && (*pic_type != PIC_I))
            {
                /* temporal back an forward  ref pointer luma and chroma */
                ps_proc->apu1_ref_buf_luma_base[PRED_L0] = aps_ref_pic[PRED_L0]->pu1_luma;
                ps_proc->apu1_ref_buf_chroma_base[PRED_L0] = aps_ref_pic[PRED_L0]->pu1_chroma;

                ps_proc->apu1_ref_buf_luma_base[PRED_L1] = aps_ref_pic[PRED_L1]->pu1_luma;
                ps_proc->apu1_ref_buf_chroma_base[PRED_L1] = aps_ref_pic[PRED_L1]->pu1_chroma;
            }

            /* Structure for current input buffer */
            ps_proc->s_inp_buf = *ps_inp_buf;

            /* Number of encode frame API calls made */
            ps_proc->i4_encode_api_call_cnt = ps_codec->i4_encode_api_call_cnt;

            /* Current Picture count */
            ps_proc->i4_pic_cnt = ps_codec->i4_pic_cnt;

            /* error status */
            ps_proc->i4_error_code = 0;

            /********************************************************************/
            /*                     INITIALIZE ENTROPY CONTEXT                   */
            /********************************************************************/
            {
                entropy_ctxt_t *ps_entropy = &ps_proc->s_entropy;

                /* start of frame */
                ps_entropy->i4_sof = 0;

                /* end of frame */
                ps_entropy->i4_eof = 0;

                /* generate header */
                ps_entropy->i4_gen_header = ps_codec->i4_gen_header;

                /* sps ref_set_id */
                ps_entropy->u4_sps_id = ps_codec->i4_sps_id;

                /* sps base */
                ps_entropy->ps_sps_base = ps_codec->ps_sps_base;

                /* sps id */
                ps_entropy->u4_pps_id = ps_codec->i4_pps_id;

                /* sps base */
                ps_entropy->ps_pps_base = ps_codec->ps_pps_base;

                /* slice map */
                ps_entropy->pu1_slice_idx = ps_proc->pu1_slice_idx;

                /* slice hdr base */
                ps_entropy->ps_slice_hdr_base = ps_proc->ps_slice_hdr_base;

                /* Abs poc */
                ps_entropy->i4_abs_pic_order_cnt = ps_proc->ps_codec->i4_poc;

                /* initialize entropy map */
                if (i == j)
                {
                    /* row '-1' */
                    memset(ps_entropy->pu1_entropy_map - ps_proc->i4_wd_mbs, 1, ps_proc->i4_wd_mbs);
                    /* row 0 to ht in mbs */
                    memset(ps_entropy->pu1_entropy_map, 0, ps_proc->i4_wd_mbs * ps_proc->i4_ht_mbs);

                    /* intialize cabac tables */
                    ih264e_init_cabac_table(ps_entropy);
                }

                /* wd in mbs */
                ps_entropy->i4_wd_mbs = ps_proc->i4_wd_mbs;

                /* ht in mbs */
                ps_entropy->i4_ht_mbs = ps_proc->i4_ht_mbs;

                /* transform_8x8_mode_flag */
                ps_entropy->i1_transform_8x8_mode_flag = 0;

                /* entropy_coding_mode_flag */
                ps_entropy->u1_entropy_coding_mode_flag =
                                ps_codec->s_cfg.u4_entropy_coding_mode;

                /* error code */
                ps_entropy->i4_error_code = IH264E_SUCCESS;

                /* mb skip run */
                *(ps_proc->s_entropy.pi4_mb_skip_run) = 0;

                /* last frame to encode */
                ps_proc->s_entropy.u4_is_last = ps_inp_buf->u4_is_last;

                /* Current Picture count */
                ps_proc->s_entropy.i4_pic_cnt = ps_codec->i4_pic_cnt;

                /* time stamps */
                ps_entropy->u4_timestamp_low = u4_timestamp_low;
                ps_entropy->u4_timestamp_high = u4_timestamp_high;

                /* init frame statistics */
                ps_entropy->u4_header_bits[MB_TYPE_INTRA] = 0;
                ps_entropy->u4_header_bits[MB_TYPE_INTER] = 0;
                ps_entropy->u4_residue_bits[MB_TYPE_INTRA] = 0;
                ps_entropy->u4_residue_bits[MB_TYPE_INTER] = 0;
            }

            /********************************************************************/
            /*                     INITIALIZE DEBLOCK CONTEXT                   */
            /********************************************************************/
            {
                /* deblk ctxt */
                deblk_ctxt_t *ps_deblk = &ps_proc->s_deblk_ctxt;

                /* slice idx map */
                ps_deblk->pu1_slice_idx = ps_proc->pu1_slice_idx;
            }

            /********************************************************************/
            /*                     INITIALIZE ME CONTEXT                        */
            /********************************************************************/
            {
                /* me ctxt */
                me_ctxt_t *ps_me_ctxt = &ps_proc->s_me_ctxt;

                /* srch range x */
                ps_me_ctxt->ai2_srch_boundaries[0] =
                                ps_codec->s_cfg.u4_srch_rng_x;

                /* srch range y */
                ps_me_ctxt->ai2_srch_boundaries[1] =
                                ps_codec->s_cfg.u4_srch_rng_y;

                /* rec stride */
                ps_me_ctxt->i4_rec_strd = ps_codec->i4_rec_strd;

                /* Half x plane offset from pic buf */
                ps_me_ctxt->u4_half_x_offset = ps_proc->u4_half_x_offset;

                /* Half y plane offset from half x plane */
                ps_me_ctxt->u4_half_y_offset = ps_proc->u4_half_y_offset;

                /* Half x plane offset from half y plane */
                ps_me_ctxt->u4_half_xy_offset = ps_proc->u4_half_xy_offset;

                /* enable fast sad */
                ps_me_ctxt->u4_enable_fast_sad = u4_enable_fast_sad;

                /* half pel */
                ps_me_ctxt->u4_enable_hpel = ps_codec->s_cfg.u4_enable_hpel;

                /* Diamond search Iteration Max Cnt */
                ps_me_ctxt->u4_num_layers = u4_num_layers;

                /* me speed preset */
                ps_me_ctxt->u4_me_speed_preset =
                                ps_codec->s_cfg.u4_me_speed_preset;

                /* qp */
                ps_me_ctxt->u1_mb_qp = ps_codec->u4_frame_qp;

                if ((i == j) && (0 == ps_codec->i4_poc))
                {
                    /* init mv bits tables */
                    ih264e_init_mv_bits(ps_me_ctxt);
                }
            }

            ps_proc->ps_ngbr_avbl = &(ps_proc->s_ngbr_avbl);

        }

        /* reset encoder header */
        ps_codec->i4_gen_header = 0;
    }

    /********************************************************************/
    /*                       ADD JOBS TO THE QUEUE                      */
    /********************************************************************/
    {
        /* job structures */
        job_t s_job;

        /* temp var */
        WORD32 i;

        /* job class */
        s_job.i4_cmd = CMD_PROCESS;

        /* number of mbs to be processed in the current job */
        s_job.i2_mb_cnt = ps_codec->s_cfg.i4_wd_mbs;

        /* job start index x */
        s_job.i2_mb_x = 0;

        /* proc base idx */
        s_job.i2_proc_base_idx = ctxt_sel ? (MAX_PROCESS_CTXT / 2) : 0;

        for (i = 0; i < (WORD32)ps_codec->s_cfg.i4_ht_mbs; i++)
        {
            /* job start index y */
            s_job.i2_mb_y = i;

            /* queue the job */
            ret = ih264_list_queue(ps_codec->pv_proc_jobq, &s_job, 1);
            if (ret != IH264_SUCCESS)
            {
                ps_codec->i4_error_code = ret;
                return IH264E_FAIL;
            }
        }

        /* Once all the jobs are queued, terminate the queue */
        /* Since the threads are created and deleted in each call, terminating
        here is not an issue */
        ih264_list_terminate(ps_codec->pv_proc_jobq);
    }

    return error_status;
}
