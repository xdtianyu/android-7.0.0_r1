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
*  ih264e_process.c
*
* @brief
*  Contains functions for codec thread
*
* @author
*  Harish
*
* @par List of Functions:
* - ih264e_generate_sps_pps()
* - ih264e_init_entropy_ctxt()
* - ih264e_entropy()
* - ih264e_pack_header_data()
* - ih264e_update_proc_ctxt()
* - ih264e_init_proc_ctxt()
* - ih264e_pad_recon_buffer()
* - ih264e_dblk_pad_hpel_processing_n_mbs()
* - ih264e_process()
* - ih264e_set_rc_pic_params()
* - ih264e_update_rc_post_enc()
* - ih264e_process_thread()
*
* @remarks
*  None
*
*******************************************************************************
*/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* System include files */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

/* User include files */
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264_defs.h"
#include "ih264_debug.h"
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
#include "ih264_platform_macros.h"
#include "ih264_macros.h"
#include "ih264_buf_mgr.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "ih264_common_tables.h"
#include "ih264_list.h"
#include "ih264e_defs.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_cabac.h"
#include "ih264e_process.h"
#include "ithread.h"
#include "ih264e_intra_modes_eval.h"
#include "ih264e_encode_header.h"
#include "ih264e_globals.h"
#include "ih264e_config.h"
#include "ih264e_trace.h"
#include "ih264e_statistics.h"
#include "ih264_cavlc_tables.h"
#include "ih264e_cavlc.h"
#include "ih264e_deblk.h"
#include "ih264e_me.h"
#include "ih264e_debug.h"
#include "ih264e_master.h"
#include "ih264e_utils.h"
#include "irc_mem_req_and_acq.h"
#include "irc_rate_control_api.h"
#include "ih264e_platform_macros.h"
#include "ime_statistics.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
******************************************************************************
*
*  @brief This function generates sps, pps set on request
*
*  @par   Description
*  When the encoder is set in header generation mode, the following function
*  is called. This generates sps and pps headers and returns the control back
*  to caller.
*
*  @param[in]    ps_codec
*  pointer to codec context
*
*  @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_generate_sps_pps(codec_t *ps_codec)
{
    /* choose between ping-pong process buffer set */
    WORD32 ctxt_sel = ps_codec->i4_encode_api_call_cnt % MAX_CTXT_SETS;

    /* entropy ctxt */
    entropy_ctxt_t *ps_entropy = &ps_codec->as_process[ctxt_sel * MAX_PROCESS_THREADS].s_entropy;

    /* Bitstream structure */
    bitstrm_t *ps_bitstrm = ps_entropy->ps_bitstrm;

    /* sps */
    sps_t *ps_sps = NULL;

    /* pps */
    pps_t *ps_pps = NULL;

    /* output buff */
    out_buf_t *ps_out_buf = &ps_codec->as_out_buf[ctxt_sel];


    /********************************************************************/
    /*      initialize the bit stream buffer                            */
    /********************************************************************/
    ih264e_bitstrm_init(ps_bitstrm, ps_out_buf->s_bits_buf.pv_buf, ps_out_buf->s_bits_buf.u4_bufsize);

    /********************************************************************/
    /*                    BEGIN HEADER GENERATION                       */
    /********************************************************************/
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

    ps_entropy->i4_error_code = IH264E_SUCCESS;

    /* generate sps */
    ps_entropy->i4_error_code |= ih264e_generate_sps(ps_bitstrm, ps_sps, &ps_codec->s_vui);

    /* generate pps */
    ps_entropy->i4_error_code |= ih264e_generate_pps(ps_bitstrm, ps_pps, ps_sps);

    /* queue output buffer */
    ps_out_buf->s_bits_buf.u4_bytes = ps_bitstrm->u4_strm_buf_offset;

    return ps_entropy->i4_error_code;
}

/**
*******************************************************************************
*
* @brief   initialize entropy context.
*
* @par Description:
*  Before invoking the call to perform to entropy coding the entropy context
*  associated with the job needs to be initialized. This involves the start
*  mb address, end mb address, slice index and the pointer to location at
*  which the mb residue info and mb header info are packed.
*
* @param[in] ps_proc
*  Pointer to the current process context
*
* @returns error status
*
* @remarks none
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_init_entropy_ctxt(process_ctxt_t *ps_proc)
{
    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* entropy ctxt */
    entropy_ctxt_t *ps_entropy = &ps_proc->s_entropy;

    /* start address */
    ps_entropy->i4_mb_start_add = ps_entropy->i4_mb_y * ps_entropy->i4_wd_mbs + ps_entropy->i4_mb_x;

    /* end address */
    ps_entropy->i4_mb_end_add = ps_entropy->i4_mb_start_add + ps_entropy->i4_mb_cnt;

    /* slice index */
    ps_entropy->i4_cur_slice_idx = ps_proc->pu1_slice_idx[ps_entropy->i4_mb_start_add];

    /* sof */
    /* @ start of frame or start of a new slice, set sof flag */
    if (ps_entropy->i4_mb_start_add == 0)
    {
        ps_entropy->i4_sof = 1;
    }

    if (ps_entropy->i4_mb_x == 0)
    {
        /* packed mb coeff data */
        ps_entropy->pv_mb_coeff_data = ((UWORD8 *)ps_entropy->pv_pic_mb_coeff_data) +
                        ps_entropy->i4_mb_y * ps_codec->u4_size_coeff_data;

        /* packed mb header data */
        ps_entropy->pv_mb_header_data = ((UWORD8 *)ps_entropy->pv_pic_mb_header_data) +
                        ps_entropy->i4_mb_y * ps_codec->u4_size_header_data;
    }

    return IH264E_SUCCESS;
}

/**
*******************************************************************************
*
* @brief entry point for entropy coding
*
* @par Description
*  This function calls lower level functions to perform entropy coding for a
*  group (n rows) of mb's. After encoding 1 row of mb's,  the function takes
*  back the control, updates the ctxt and calls lower level functions again.
*  This process is repeated till all the rows or group of mb's (which ever is
*  minimum) are coded
*
* @param[in] ps_proc
*  process context
*
* @returns  error status
*
* @remarks
*
*******************************************************************************
*/

IH264E_ERROR_T ih264e_entropy(process_ctxt_t *ps_proc)
{
    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* entropy context */
    entropy_ctxt_t *ps_entropy = &ps_proc->s_entropy;

    /* cabac context */
    cabac_ctxt_t *ps_cabac_ctxt = ps_entropy->ps_cabac;

    /* sps */
    sps_t *ps_sps = ps_entropy->ps_sps_base + (ps_entropy->u4_sps_id % MAX_SPS_CNT);

    /* pps */
    pps_t *ps_pps = ps_entropy->ps_pps_base + (ps_entropy->u4_pps_id % MAX_PPS_CNT);

    /* slice header */
    slice_header_t *ps_slice_hdr = ps_entropy->ps_slice_hdr_base + (ps_entropy->i4_cur_slice_idx % MAX_SLICE_HDR_CNT);

    /* slice type */
    WORD32 i4_slice_type = ps_proc->i4_slice_type;

    /* Bitstream structure */
    bitstrm_t *ps_bitstrm = ps_entropy->ps_bitstrm;

    /* output buff */
    out_buf_t s_out_buf;

    /* proc map */
    UWORD8  *pu1_proc_map;

    /* entropy map */
    UWORD8  *pu1_entropy_map_curr;

    /* proc base idx */
    WORD32 ctxt_sel = ps_proc->i4_encode_api_call_cnt % MAX_CTXT_SETS;

    /* temp var */
    WORD32 i4_wd_mbs, i4_ht_mbs;
    UWORD32 u4_mb_cnt, u4_mb_idx, u4_mb_end_idx;
    WORD32 bitstream_start_offset, bitstream_end_offset;
    /********************************************************************/
    /*                            BEGIN INIT                            */
    /********************************************************************/

    /* entropy encode start address */
    u4_mb_idx = ps_entropy->i4_mb_start_add;

    /* entropy encode end address */
    u4_mb_end_idx = ps_entropy->i4_mb_end_add;

    /* width in mbs */
    i4_wd_mbs = ps_entropy->i4_wd_mbs;

    /* height in mbs */
    i4_ht_mbs = ps_entropy->i4_ht_mbs;

    /* total mb cnt */
    u4_mb_cnt = i4_wd_mbs * i4_ht_mbs;

    /* proc map */
    pu1_proc_map = ps_proc->pu1_proc_map + ps_entropy->i4_mb_y * i4_wd_mbs;

    /* entropy map */
    pu1_entropy_map_curr = ps_entropy->pu1_entropy_map + ps_entropy->i4_mb_y * i4_wd_mbs;

    /********************************************************************/
    /* @ start of frame / slice,                                        */
    /*      initialize the output buffer,                               */
    /*      initialize the bit stream buffer,                           */
    /*      check if sps and pps headers have to be generated,          */
    /*      populate and generate slice header                          */
    /********************************************************************/
    if (ps_entropy->i4_sof)
    {
        /********************************************************************/
        /*      initialize the output buffer                                */
        /********************************************************************/
        s_out_buf = ps_codec->as_out_buf[ctxt_sel];

        /* is last frame to encode */
        s_out_buf.u4_is_last = ps_entropy->u4_is_last;

        /* frame idx */
        s_out_buf.u4_timestamp_high = ps_entropy->u4_timestamp_high;
        s_out_buf.u4_timestamp_low = ps_entropy->u4_timestamp_low;

        /********************************************************************/
        /*      initialize the bit stream buffer                            */
        /********************************************************************/
        ih264e_bitstrm_init(ps_bitstrm, s_out_buf.s_bits_buf.pv_buf, s_out_buf.s_bits_buf.u4_bufsize);

        /********************************************************************/
        /*                    BEGIN HEADER GENERATION                       */
        /********************************************************************/
        if (1 == ps_entropy->i4_gen_header)
        {
            /* generate sps */
            ps_entropy->i4_error_code |= ih264e_generate_sps(ps_bitstrm, ps_sps, &ps_codec->s_vui);

            /* generate pps */
            ps_entropy->i4_error_code |= ih264e_generate_pps(ps_bitstrm, ps_pps, ps_sps);

            /* reset i4_gen_header */
            ps_entropy->i4_gen_header = 0;
        }

        /* populate slice header */
        ih264e_populate_slice_header(ps_proc, ps_slice_hdr, ps_pps, ps_sps);

        /* generate slice header */
        ps_entropy->i4_error_code |= ih264e_generate_slice_header(ps_bitstrm, ps_slice_hdr,
                                                                  ps_pps, ps_sps);

        /* once start of frame / slice is done, you can reset it */
        /* it is the responsibility of the caller to set this flag */
        ps_entropy->i4_sof = 0;

        if (CABAC == ps_entropy->u1_entropy_coding_mode_flag)
        {
            BITSTREAM_BYTE_ALIGN(ps_bitstrm);
            BITSTREAM_FLUSH(ps_bitstrm);
            ih264e_init_cabac_ctxt(ps_entropy);
        }
    }

    /* begin entropy coding for the mb set */
    while (u4_mb_idx < u4_mb_end_idx)
    {
        /* init ptrs/indices */
        if (ps_entropy->i4_mb_x == i4_wd_mbs)
        {
            ps_entropy->i4_mb_y++;
            ps_entropy->i4_mb_x = 0;

            /* packed mb coeff data */
            ps_entropy->pv_mb_coeff_data = ((UWORD8 *)ps_entropy->pv_pic_mb_coeff_data) +
                            ps_entropy->i4_mb_y * ps_codec->u4_size_coeff_data;

            /* packed mb header data */
            ps_entropy->pv_mb_header_data = ((UWORD8 *)ps_entropy->pv_pic_mb_header_data) +
                            ps_entropy->i4_mb_y * ps_codec->u4_size_header_data;

            /* proc map */
            pu1_proc_map = ps_proc->pu1_proc_map + ps_entropy->i4_mb_y * i4_wd_mbs;

            /* entropy map */
            pu1_entropy_map_curr = ps_entropy->pu1_entropy_map + ps_entropy->i4_mb_y * i4_wd_mbs;
        }

        DEBUG("\nmb indices x, y %d, %d", ps_entropy->i4_mb_x, ps_entropy->i4_mb_y);
        ENTROPY_TRACE("mb index x %d", ps_entropy->i4_mb_x);
        ENTROPY_TRACE("mb index y %d", ps_entropy->i4_mb_y);

        /* wait until the curr mb is core coded */
        /* The wait for curr mb to be core coded is essential when entropy is launched
         * as a separate job
         */
        while (1)
        {
            volatile UWORD8 *pu1_buf1;
            WORD32 idx = ps_entropy->i4_mb_x;

            pu1_buf1 = pu1_proc_map + idx;
            if (*pu1_buf1)
                break;
            ithread_yield();
        }


        /* write mb layer */
        ps_entropy->i4_error_code |= ps_codec->pf_write_mb_syntax_layer[ps_entropy->u1_entropy_coding_mode_flag][i4_slice_type](ps_entropy);
        /* Starting bitstream offset for header in bits */
        bitstream_start_offset = GET_NUM_BITS(ps_bitstrm);

        /* set entropy map */
        pu1_entropy_map_curr[ps_entropy->i4_mb_x] = 1;

        u4_mb_idx++;
        ps_entropy->i4_mb_x++;
        /* check for eof */
        if (CABAC == ps_entropy->u1_entropy_coding_mode_flag)
        {
            if (ps_entropy->i4_mb_x < i4_wd_mbs)
            {
                ih264e_cabac_encode_terminate(ps_cabac_ctxt, 0);
            }
        }

        if (ps_entropy->i4_mb_x == i4_wd_mbs)
        {
            /* if slices are enabled */
            if (ps_codec->s_cfg.e_slice_mode == IVE_SLICE_MODE_BLOCKS)
            {
                /* current slice index */
                WORD32 i4_curr_slice_idx = ps_entropy->i4_cur_slice_idx;

                /* slice map */
                UWORD8 *pu1_slice_idx = ps_entropy->pu1_slice_idx;

                /* No need to open a slice at end of frame. The current slice can be closed at the time
                 * of signaling eof flag.
                 */
                if ((u4_mb_idx != u4_mb_cnt) && (i4_curr_slice_idx
                                                != pu1_slice_idx[u4_mb_idx]))
                {
                    if (CAVLC == ps_entropy->u1_entropy_coding_mode_flag)
                    { /* mb skip run */
                        if ((i4_slice_type != ISLICE)
                                        && *ps_entropy->pi4_mb_skip_run)
                        {
                            if (*ps_entropy->pi4_mb_skip_run)
                            {
                            PUT_BITS_UEV(ps_bitstrm, *ps_entropy->pi4_mb_skip_run, ps_entropy->i4_error_code, "mb skip run");
                                *ps_entropy->pi4_mb_skip_run = 0;
                            }
                        }
                        /* put rbsp trailing bits for the previous slice */
                                 ps_entropy->i4_error_code |= ih264e_put_rbsp_trailing_bits(ps_bitstrm);
                    }
                    else
                    {
                        ih264e_cabac_encode_terminate(ps_cabac_ctxt, 1);
                    }

                    /* update slice header pointer */
                    i4_curr_slice_idx = pu1_slice_idx[u4_mb_idx];
                    ps_entropy->i4_cur_slice_idx = i4_curr_slice_idx;
                    ps_slice_hdr = ps_entropy->ps_slice_hdr_base+ (i4_curr_slice_idx % MAX_SLICE_HDR_CNT);

                    /* populate slice header */
                    ps_entropy->i4_mb_start_add = u4_mb_idx;
                    ih264e_populate_slice_header(ps_proc, ps_slice_hdr, ps_pps,
                                                 ps_sps);

                    /* generate slice header */
                    ps_entropy->i4_error_code |= ih264e_generate_slice_header(
                                    ps_bitstrm, ps_slice_hdr, ps_pps, ps_sps);
                    if (CABAC == ps_entropy->u1_entropy_coding_mode_flag)
                    {
                        BITSTREAM_BYTE_ALIGN(ps_bitstrm);
                        BITSTREAM_FLUSH(ps_bitstrm);
                        ih264e_init_cabac_ctxt(ps_entropy);
                    }
                }
                else
                {
                    if (CABAC == ps_entropy->u1_entropy_coding_mode_flag
                                    && u4_mb_idx != u4_mb_cnt)
                    {
                        ih264e_cabac_encode_terminate(ps_cabac_ctxt, 0);
                    }
                }
            }
            /* Dont execute any further instructions until store synchronization took place */
            DATA_SYNC();
        }

        /* Ending bitstream offset for header in bits */
        bitstream_end_offset = GET_NUM_BITS(ps_bitstrm);
        ps_entropy->u4_header_bits[i4_slice_type == PSLICE] +=
                        bitstream_end_offset - bitstream_start_offset;
    }

    /* check for eof */
    if (u4_mb_idx == u4_mb_cnt)
    {
        /* set end of frame flag */
        ps_entropy->i4_eof = 1;
    }
    else
    {
        if (CABAC == ps_entropy->u1_entropy_coding_mode_flag
                        && ps_codec->s_cfg.e_slice_mode
                                        != IVE_SLICE_MODE_BLOCKS)
        {
            ih264e_cabac_encode_terminate(ps_cabac_ctxt, 0);
        }
    }

    if (ps_entropy->i4_eof)
    {
        if (CAVLC == ps_entropy->u1_entropy_coding_mode_flag)
        {
            /* mb skip run */
            if ((i4_slice_type != ISLICE) && *ps_entropy->pi4_mb_skip_run)
            {
                if (*ps_entropy->pi4_mb_skip_run)
                {
                    PUT_BITS_UEV(ps_bitstrm, *ps_entropy->pi4_mb_skip_run,
                                 ps_entropy->i4_error_code, "mb skip run");
                    *ps_entropy->pi4_mb_skip_run = 0;
                }
            }
            /* put rbsp trailing bits */
             ps_entropy->i4_error_code |= ih264e_put_rbsp_trailing_bits(ps_bitstrm);
        }
        else
        {
            ih264e_cabac_encode_terminate(ps_cabac_ctxt, 1);
        }

        /* update current frame stats to rc library */
        {
            /* number of bytes to stuff */
            WORD32 i4_stuff_bytes;

            /* update */
            i4_stuff_bytes = ih264e_update_rc_post_enc(
                            ps_codec, ctxt_sel,
                            (ps_proc->ps_codec->i4_poc == 0));

            /* cbr rc - house keeping */
            if (ps_codec->s_rate_control.post_encode_skip[ctxt_sel])
            {
                ps_entropy->ps_bitstrm->u4_strm_buf_offset = 0;
            }
            else if (i4_stuff_bytes)
            {
                /* add filler nal units */
                ps_entropy->i4_error_code |= ih264e_add_filler_nal_unit(ps_bitstrm, i4_stuff_bytes);
            }
        }

        /*
         *Frame number is to be incremented only if the current frame is a
         * reference frame. After each successful frame encode, we increment
         * frame number by 1
         */
        if (!ps_codec->s_rate_control.post_encode_skip[ctxt_sel]
                        && ps_codec->u4_is_curr_frm_ref)
        {
            ps_codec->i4_frame_num++;
        }
        /********************************************************************/
        /*      signal the output                                           */
        /********************************************************************/
        ps_codec->as_out_buf[ctxt_sel].s_bits_buf.u4_bytes =
                        ps_entropy->ps_bitstrm->u4_strm_buf_offset;

        DEBUG("entropy status %x", ps_entropy->i4_error_code);
    }

    /* allow threads to dequeue entropy jobs */
    ps_codec->au4_entropy_thread_active[ctxt_sel] = 0;

    return ps_entropy->i4_error_code;
}

/**
*******************************************************************************
*
* @brief Packs header information of a mb in to a buffer
*
* @par Description:
*  After the deciding the mode info of a macroblock, the syntax elements
*  associated with the mb are packed and stored. The entropy thread unpacks
*  this buffer and generates the end bit stream.
*
* @param[in] ps_proc
*  Pointer to the current process context
*
* @returns error status
*
* @remarks none
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_pack_header_data(process_ctxt_t *ps_proc)
{
    /* curr mb type */
    UWORD32 u4_mb_type = ps_proc->u4_mb_type;

    /* pack mb syntax layer of curr mb (used for entropy coding) */
    if (u4_mb_type == I4x4)
    {
        /* pointer to mb header storage space */
        UWORD8 *pu1_ptr = ps_proc->pv_mb_header_data;

        /* temp var */
        WORD32 i4, byte;

        /* mb type plus mode */
        *pu1_ptr++ = (ps_proc->u1_c_i8_mode << 6) + u4_mb_type;

        /* cbp */
        *pu1_ptr++ = ps_proc->u4_cbp;

        /* mb qp delta */
        *pu1_ptr++ = ps_proc->u4_mb_qp - ps_proc->u4_mb_qp_prev;

        /* sub mb modes */
        for (i4 = 0; i4 < 16; i4 ++)
        {
            byte = 0;

            if (ps_proc->au1_predicted_intra_luma_mb_4x4_modes[i4] ==
                            ps_proc->au1_intra_luma_mb_4x4_modes[i4])
            {
                byte |= 1;
            }
            else
            {

                if (ps_proc->au1_intra_luma_mb_4x4_modes[i4] <
                                ps_proc->au1_predicted_intra_luma_mb_4x4_modes[i4])
                {
                    byte |= (ps_proc->au1_intra_luma_mb_4x4_modes[i4] << 1);
                }
                else
                {
                    byte |= (ps_proc->au1_intra_luma_mb_4x4_modes[i4] - 1) << 1;
                }
            }

            i4++;

            if (ps_proc->au1_predicted_intra_luma_mb_4x4_modes[i4] ==
                            ps_proc->au1_intra_luma_mb_4x4_modes[i4])
            {
                byte |= 16;
            }
            else
            {

                if (ps_proc->au1_intra_luma_mb_4x4_modes[i4] <
                                ps_proc->au1_predicted_intra_luma_mb_4x4_modes[i4])
                {
                    byte |= (ps_proc->au1_intra_luma_mb_4x4_modes[i4] << 5);
                }
                else
                {
                    byte |= (ps_proc->au1_intra_luma_mb_4x4_modes[i4] - 1) << 5;
                }
            }

            *pu1_ptr++ = byte;
        }

        /* end of mb layer */
        ps_proc->pv_mb_header_data = pu1_ptr;
    }
    else if (u4_mb_type == I16x16)
    {
        /* pointer to mb header storage space */
        UWORD8 *pu1_ptr = ps_proc->pv_mb_header_data;

        /* mb type plus mode */
        *pu1_ptr++ = (ps_proc->u1_c_i8_mode << 6) + (ps_proc->u1_l_i16_mode << 4) + u4_mb_type;

        /* cbp */
        *pu1_ptr++ = ps_proc->u4_cbp;

        /* mb qp delta */
        *pu1_ptr++ = ps_proc->u4_mb_qp - ps_proc->u4_mb_qp_prev;

        /* end of mb layer */
        ps_proc->pv_mb_header_data = pu1_ptr;
    }
    else if (u4_mb_type == P16x16)
    {
        /* pointer to mb header storage space */
        UWORD8 *pu1_ptr = ps_proc->pv_mb_header_data;

        WORD16 *i2_mv_ptr;

        /* mb type plus mode */
        *pu1_ptr++ = u4_mb_type;

        /* cbp */
        *pu1_ptr++ = ps_proc->u4_cbp;

        /* mb qp delta */
        *pu1_ptr++ = ps_proc->u4_mb_qp - ps_proc->u4_mb_qp_prev;

        i2_mv_ptr = (WORD16 *)pu1_ptr;

        *i2_mv_ptr++ = ps_proc->ps_pu->s_me_info[0].s_mv.i2_mvx - ps_proc->ps_pred_mv[0].s_mv.i2_mvx;

        *i2_mv_ptr++ = ps_proc->ps_pu->s_me_info[0].s_mv.i2_mvy - ps_proc->ps_pred_mv[0].s_mv.i2_mvy;

        /* end of mb layer */
        ps_proc->pv_mb_header_data = i2_mv_ptr;
    }
    else if (u4_mb_type == PSKIP)
    {
        /* pointer to mb header storage space */
        UWORD8 *pu1_ptr = ps_proc->pv_mb_header_data;

        /* mb type plus mode */
        *pu1_ptr++ = u4_mb_type;

        /* end of mb layer */
        ps_proc->pv_mb_header_data = pu1_ptr;
    }
    else if(u4_mb_type == B16x16)
    {

        /* pointer to mb header storage space */
        UWORD8 *pu1_ptr = ps_proc->pv_mb_header_data;

        WORD16 *i2_mv_ptr;

        UWORD32 u4_pred_mode = ps_proc->ps_pu->b2_pred_mode;

        /* mb type plus mode */
        *pu1_ptr++ = (u4_pred_mode << 4) + u4_mb_type;

        /* cbp */
        *pu1_ptr++ = ps_proc->u4_cbp;

        /* mb qp delta */
        *pu1_ptr++ = ps_proc->u4_mb_qp - ps_proc->u4_mb_qp_prev;

        /* l0 & l1 me data */
        i2_mv_ptr = (WORD16 *)pu1_ptr;

        if (u4_pred_mode != PRED_L1)
        {
            *i2_mv_ptr++ = ps_proc->ps_pu->s_me_info[0].s_mv.i2_mvx
                            - ps_proc->ps_pred_mv[0].s_mv.i2_mvx;

            *i2_mv_ptr++ = ps_proc->ps_pu->s_me_info[0].s_mv.i2_mvy
                            - ps_proc->ps_pred_mv[0].s_mv.i2_mvy;
        }
        if (u4_pred_mode != PRED_L0)
        {
            *i2_mv_ptr++ = ps_proc->ps_pu->s_me_info[1].s_mv.i2_mvx
                            - ps_proc->ps_pred_mv[1].s_mv.i2_mvx;

            *i2_mv_ptr++ = ps_proc->ps_pu->s_me_info[1].s_mv.i2_mvy
                            - ps_proc->ps_pred_mv[1].s_mv.i2_mvy;
        }

        /* end of mb layer */
        ps_proc->pv_mb_header_data = i2_mv_ptr;

    }
    else if(u4_mb_type == BDIRECT)
    {
        /* pointer to mb header storage space */
        UWORD8 *pu1_ptr = ps_proc->pv_mb_header_data;

        /* mb type plus mode */
        *pu1_ptr++ = u4_mb_type;

        /* cbp */
        *pu1_ptr++ = ps_proc->u4_cbp;

        /* mb qp delta */
        *pu1_ptr++ = ps_proc->u4_mb_qp - ps_proc->u4_mb_qp_prev;

        ps_proc->pv_mb_header_data = pu1_ptr;

    }
    else if(u4_mb_type == BSKIP)
    {
        UWORD32 u4_pred_mode = ps_proc->ps_pu->b2_pred_mode;

        /* pointer to mb header storage space */
        UWORD8 *pu1_ptr = ps_proc->pv_mb_header_data;

        /* mb type plus mode */
        *pu1_ptr++ = (u4_pred_mode << 4) + u4_mb_type;

        /* end of mb layer */
        ps_proc->pv_mb_header_data = pu1_ptr;
    }

    return IH264E_SUCCESS;
}

/**
*******************************************************************************
*
* @brief   update process context after encoding an mb. This involves preserving
* the current mb information for later use, initialize the proc ctxt elements to
* encode next mb.
*
* @par Description:
*  This function performs house keeping tasks after encoding an mb.
*  After encoding an mb, various elements of the process context needs to be
*  updated to encode the next mb. For instance, the source, recon and reference
*  pointers, mb indices have to be adjusted to the next mb. The slice index of
*  the current mb needs to be updated. If mb qp modulation is enabled, then if
*  the qp changes the quant param structure needs to be updated. Also to encoding
*  the next mb, the current mb info is used as part of mode prediction or mv
*  prediction. Hence the current mb info has to preserved at top/top left/left
*  locations.
*
* @param[in] ps_proc
*  Pointer to the current process context
*
* @returns none
*
* @remarks none
*
*******************************************************************************
*/
WORD32 ih264e_update_proc_ctxt(process_ctxt_t *ps_proc)
{
    /* error status */
    WORD32 error_status = IH264_SUCCESS;

    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* curr mb indices */
    WORD32 i4_mb_x = ps_proc->i4_mb_x;
    WORD32 i4_mb_y = ps_proc->i4_mb_y;

    /* mb syntax elements of neighbors */
    mb_info_t *ps_left_syn =  &ps_proc->s_left_mb_syntax_ele;
    mb_info_t *ps_top_syn = ps_proc->ps_top_row_mb_syntax_ele + i4_mb_x;
    mb_info_t *ps_top_left_syn = &ps_proc->s_top_left_mb_syntax_ele;

    /* curr mb type */
    UWORD32 u4_mb_type = ps_proc->u4_mb_type;

    /* curr mb type */
    UWORD32 u4_is_intra = ps_proc->u4_is_intra;

    /* width in mbs */
    WORD32 i4_wd_mbs = ps_proc->i4_wd_mbs;

    /*height in mbs*/
    WORD32 i4_ht_mbs = ps_proc->i4_ht_mbs;

    /* proc map */
    UWORD8 *pu1_proc_map = ps_proc->pu1_proc_map + (i4_mb_y * i4_wd_mbs);

    /* deblk context */
    deblk_ctxt_t *ps_deblk = &ps_proc->s_deblk_ctxt;

    /* deblk bs context */
    bs_ctxt_t *ps_bs = &(ps_deblk->s_bs_ctxt);

    /* top row motion vector info */
    enc_pu_t *ps_top_row_pu = ps_proc->ps_top_row_pu + i4_mb_x;

    /* top left mb motion vector */
    enc_pu_t *ps_top_left_mb_pu = &ps_proc->s_top_left_mb_pu;

    /* left mb motion vector */
    enc_pu_t *ps_left_mb_pu = &ps_proc->s_left_mb_pu;

    /* sub mb modes */
    UWORD8 *pu1_top_mb_intra_modes = ps_proc->pu1_top_mb_intra_modes + (i4_mb_x << 4);

    /*************************************************************/
    /* During MV prediction, when top right mb is not available, */
    /* top left mb info. is used for prediction. Hence the curr  */
    /* top, which will be top left for the next mb needs to be   */
    /* preserved before updating it with curr mb info.           */
    /*************************************************************/

    /* mb type, mb class, csbp */
    *ps_top_left_syn = *ps_top_syn;

    if (ps_proc->i4_slice_type != ISLICE)
    {
        /*****************************************/
        /* update top left with top info results */
        /*****************************************/
        /* mv */
        *ps_top_left_mb_pu = *ps_top_row_pu;
    }

    /*************************************************/
    /* update top and left with curr mb info results */
    /*************************************************/

    /* mb type */
    ps_left_syn->u2_mb_type = ps_top_syn->u2_mb_type = u4_mb_type;

    /* mb class */
    ps_left_syn->u2_is_intra = ps_top_syn->u2_is_intra = u4_is_intra;

    /* csbp */
    ps_left_syn->u4_csbp = ps_top_syn->u4_csbp = ps_proc->u4_csbp;

    /* distortion */
    ps_left_syn->i4_mb_distortion = ps_top_syn->i4_mb_distortion = ps_proc->i4_mb_distortion;

    if (u4_is_intra)
    {
        /* mb / sub mb modes */
        if (I16x16 == u4_mb_type)
        {
            pu1_top_mb_intra_modes[0] = ps_proc->au1_left_mb_intra_modes[0] = ps_proc->u1_l_i16_mode;
        }
        else if (I4x4 == u4_mb_type)
        {
            ps_codec->pf_mem_cpy_mul8(ps_proc->au1_left_mb_intra_modes, ps_proc->au1_intra_luma_mb_4x4_modes, 16);
            ps_codec->pf_mem_cpy_mul8(pu1_top_mb_intra_modes, ps_proc->au1_intra_luma_mb_4x4_modes, 16);
        }
        else if (I8x8 == u4_mb_type)
        {
            memcpy(ps_proc->au1_left_mb_intra_modes, ps_proc->au1_intra_luma_mb_8x8_modes, 4);
            memcpy(pu1_top_mb_intra_modes, ps_proc->au1_intra_luma_mb_8x8_modes, 4);
        }

        if ((ps_proc->i4_slice_type == PSLICE) ||(ps_proc->i4_slice_type == BSLICE))
        {
            /* mv */
            *ps_left_mb_pu = *ps_top_row_pu = *(ps_proc->ps_pu);
        }

        *ps_proc->pu4_mb_pu_cnt = 1;
    }
    else
    {
        /* mv */
        *ps_left_mb_pu = *ps_top_row_pu = *(ps_proc->ps_pu);
    }

    /*
     * Mark that the MB has been coded intra
     * So that future AIRs can skip it
     */
    ps_proc->pu1_is_intra_coded[i4_mb_x + (i4_mb_y * i4_wd_mbs)] = u4_is_intra;

    /**************************************************/
    /* pack mb header info. for entropy coding        */
    /**************************************************/
    ih264e_pack_header_data(ps_proc);

    /* update previous mb qp */
    ps_proc->u4_mb_qp_prev = ps_proc->u4_mb_qp;

    /* store qp */
    ps_proc->s_deblk_ctxt.s_bs_ctxt.pu1_pic_qp[(i4_mb_y * i4_wd_mbs) + i4_mb_x] = ps_proc->u4_mb_qp;

    /*
     * We need to sync the cache to make sure that the nmv content of proc
     * is updated to cache properly
     */
    DATA_SYNC();

    /* Just before finishing the row, enqueue the job in to entropy queue.
     * The master thread depending on its convenience shall dequeue it and
     * performs entropy.
     *
     * WARN !! Placing this block post proc map update can cause queuing of
     * entropy jobs in out of order.
     */
    if (i4_mb_x == i4_wd_mbs - 1)
    {
        /* job structures */
        job_t s_job;

        /* job class */
        s_job.i4_cmd = CMD_ENTROPY;

        /* number of mbs to be processed in the current job */
        s_job.i2_mb_cnt = ps_codec->s_cfg.i4_wd_mbs;

        /* job start index x */
        s_job.i2_mb_x = 0;

        /* job start index y */
        s_job.i2_mb_y = ps_proc->i4_mb_y;

        /* proc base idx */
        s_job.i2_proc_base_idx = (ps_codec->i4_encode_api_call_cnt % MAX_CTXT_SETS) ? (MAX_PROCESS_CTXT / 2) : 0;

        /* queue the job */
        error_status |= ih264_list_queue(ps_proc->pv_entropy_jobq, &s_job, 1);

        if(ps_proc->i4_mb_y == (i4_ht_mbs - 1))
            ih264_list_terminate(ps_codec->pv_entropy_jobq);
    }

    /* update proc map */
    pu1_proc_map[i4_mb_x] = 1;

    /**************************************************/
    /* update proc ctxt elements for encoding next mb */
    /**************************************************/
    /* update indices */
    i4_mb_x ++;
    ps_proc->i4_mb_x = i4_mb_x;

    if (ps_proc->i4_mb_x == i4_wd_mbs)
    {
        ps_proc->i4_mb_y++;
        ps_proc->i4_mb_x = 0;
    }

    /* update slice index */
    ps_proc->i4_cur_slice_idx = ps_proc->pu1_slice_idx[ps_proc->i4_mb_y * i4_wd_mbs + ps_proc->i4_mb_x];

    /* update buffers pointers */
    ps_proc->pu1_src_buf_luma += MB_SIZE;
    ps_proc->pu1_rec_buf_luma += MB_SIZE;
    ps_proc->apu1_ref_buf_luma[0] += MB_SIZE;
    ps_proc->apu1_ref_buf_luma[1] += MB_SIZE;

    /*
     * Note: Although chroma mb size is 8, as the chroma buffers are interleaved,
     * the stride per MB is MB_SIZE
     */
    ps_proc->pu1_src_buf_chroma += MB_SIZE;
    ps_proc->pu1_rec_buf_chroma += MB_SIZE;
    ps_proc->apu1_ref_buf_chroma[0] += MB_SIZE;
    ps_proc->apu1_ref_buf_chroma[1] += MB_SIZE;



    /* Reset cost, distortion params */
    ps_proc->i4_mb_cost = INT_MAX;
    ps_proc->i4_mb_distortion = SHRT_MAX;

    ps_proc->ps_pu += *ps_proc->pu4_mb_pu_cnt;

    ps_proc->pu4_mb_pu_cnt += 1;

    /* Update colocated pu */
    if (ps_proc->i4_slice_type == BSLICE)
        ps_proc->ps_colpu += *(ps_proc->aps_mv_buf[1]->pu4_mb_pu_cnt +  (i4_mb_y * ps_proc->i4_wd_mbs) + i4_mb_x);

    /* deblk ctxts */
    if (ps_proc->u4_disable_deblock_level != 1)
    {
        /* indices */
        ps_bs->i4_mb_x = ps_proc->i4_mb_x;
        ps_bs->i4_mb_y = ps_proc->i4_mb_y;

#ifndef N_MB_ENABLE /* For N MB processing update take place inside deblocking function */
        ps_deblk->i4_mb_x ++;

        ps_deblk->pu1_cur_pic_luma += MB_SIZE;
        /*
         * Note: Although chroma mb size is 8, as the chroma buffers are interleaved,
         * the stride per MB is MB_SIZE
         */
        ps_deblk->pu1_cur_pic_chroma += MB_SIZE;
#endif
    }

    return error_status;
}

/**
*******************************************************************************
*
* @brief   initialize process context.
*
* @par Description:
*  Before dispatching the current job to process thread, the process context
*  associated with the job is initialized. Usually every job aims to encode one
*  row of mb's. Basing on the row indices provided by the job, the process
*  context's buffer ptrs, slice indices and other elements that are necessary
*  during core-coding are initialized.
*
* @param[in] ps_proc
*  Pointer to the current process context
*
* @returns error status
*
* @remarks none
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_init_proc_ctxt(process_ctxt_t *ps_proc)
{
    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* nmb processing context*/
    n_mb_process_ctxt_t *ps_n_mb_ctxt = &ps_proc->s_n_mb_ctxt;

    /* indices */
    WORD32 i4_mb_x, i4_mb_y;

    /* strides */
    WORD32 i4_src_strd = ps_proc->i4_src_strd;
    WORD32 i4_src_chroma_strd = ps_proc->i4_src_chroma_strd;
    WORD32 i4_rec_strd = ps_proc->i4_rec_strd;

    /* quant params */
    quant_params_t *ps_qp_params = ps_proc->ps_qp_params[0];

    /* deblk ctxt */
    deblk_ctxt_t *ps_deblk = &ps_proc->s_deblk_ctxt;

    /* deblk bs context */
    bs_ctxt_t *ps_bs = &(ps_deblk->s_bs_ctxt);

    /* Pointer to mv_buffer of current frame */
    mv_buf_t *ps_cur_mv_buf = ps_proc->ps_cur_mv_buf;

    /* Pointers for color space conversion */
    UWORD8 *pu1_y_buf_base, *pu1_u_buf_base, *pu1_v_buf_base;

    /* Pad the MB to support non standard sizes */
    UWORD32 u4_pad_right_sz = ps_codec->s_cfg.u4_wd - ps_codec->s_cfg.u4_disp_wd;
    UWORD32 u4_pad_bottom_sz = ps_codec->s_cfg.u4_ht - ps_codec->s_cfg.u4_disp_ht;
    UWORD16 u2_num_rows = MB_SIZE;
    WORD32 convert_uv_only;

    /********************************************************************/
    /*                            BEGIN INIT                            */
    /********************************************************************/

    i4_mb_x = ps_proc->i4_mb_x;
    i4_mb_y = ps_proc->i4_mb_y;

    /* Number of mbs processed in one loop of process function */
    ps_proc->i4_nmb_ntrpy = ps_proc->i4_wd_mbs;
    ps_proc->u4_nmb_me = ps_proc->i4_wd_mbs;

    /* init buffer pointers */
    convert_uv_only = 1;
    if (u4_pad_bottom_sz || u4_pad_right_sz ||
        ps_codec->s_cfg.e_inp_color_fmt == IV_YUV_422ILE)
    {
        if (ps_proc->i4_mb_y == ps_proc->i4_ht_mbs - 1)
            u2_num_rows = (UWORD16) MB_SIZE - u4_pad_bottom_sz;
        ps_proc->pu1_src_buf_luma_base = ps_codec->pu1_y_csc_buf_base;
        i4_src_strd = ps_proc->i4_src_strd = ps_codec->s_cfg.u4_max_wd;
        ps_proc->pu1_src_buf_luma = ps_proc->pu1_src_buf_luma_base + (i4_mb_x * MB_SIZE) + ps_codec->s_cfg.u4_max_wd * (i4_mb_y * MB_SIZE);
        convert_uv_only = 0;
    }
    else
    {
        i4_src_strd = ps_proc->i4_src_strd = ps_proc->s_inp_buf.s_raw_buf.au4_strd[0];
        ps_proc->pu1_src_buf_luma = ps_proc->pu1_src_buf_luma_base + (i4_mb_x * MB_SIZE) + i4_src_strd * (i4_mb_y * MB_SIZE);
    }


    if (ps_codec->s_cfg.e_inp_color_fmt == IV_YUV_422ILE ||
        ps_codec->s_cfg.e_inp_color_fmt == IV_YUV_420P ||
        ps_proc->i4_mb_y == (ps_proc->i4_ht_mbs - 1) ||
        u4_pad_bottom_sz || u4_pad_right_sz)
    {
        if ((ps_codec->s_cfg.e_inp_color_fmt == IV_YUV_420SP_UV) ||
            (ps_codec->s_cfg.e_inp_color_fmt == IV_YUV_420SP_VU))
            ps_proc->pu1_src_buf_chroma_base = ps_codec->pu1_uv_csc_buf_base;

        ps_proc->pu1_src_buf_chroma = ps_proc->pu1_src_buf_chroma_base + (i4_mb_x * MB_SIZE) + ps_codec->s_cfg.u4_max_wd * (i4_mb_y * BLK8x8SIZE);
        i4_src_chroma_strd = ps_proc->i4_src_chroma_strd = ps_codec->s_cfg.u4_max_wd;
    }
    else
    {
        i4_src_chroma_strd = ps_proc->i4_src_chroma_strd = ps_proc->s_inp_buf.s_raw_buf.au4_strd[1];
        ps_proc->pu1_src_buf_chroma = ps_proc->pu1_src_buf_chroma_base + (i4_mb_x * MB_SIZE) + i4_src_chroma_strd * (i4_mb_y * BLK8x8SIZE);
    }

    ps_proc->pu1_rec_buf_luma = ps_proc->pu1_rec_buf_luma_base + (i4_mb_x * MB_SIZE) + i4_rec_strd * (i4_mb_y * MB_SIZE);
    ps_proc->pu1_rec_buf_chroma = ps_proc->pu1_rec_buf_chroma_base + (i4_mb_x * MB_SIZE) + i4_rec_strd * (i4_mb_y * BLK8x8SIZE);

    /* Tempral back and forward reference buffer */
    ps_proc->apu1_ref_buf_luma[0] = ps_proc->apu1_ref_buf_luma_base[0] + (i4_mb_x * MB_SIZE) + i4_rec_strd * (i4_mb_y * MB_SIZE);
    ps_proc->apu1_ref_buf_chroma[0] = ps_proc->apu1_ref_buf_chroma_base[0] + (i4_mb_x * MB_SIZE) + i4_rec_strd * (i4_mb_y * BLK8x8SIZE);
    ps_proc->apu1_ref_buf_luma[1] = ps_proc->apu1_ref_buf_luma_base[1] + (i4_mb_x * MB_SIZE) + i4_rec_strd * (i4_mb_y * MB_SIZE);
    ps_proc->apu1_ref_buf_chroma[1] = ps_proc->apu1_ref_buf_chroma_base[1] + (i4_mb_x * MB_SIZE) + i4_rec_strd * (i4_mb_y * BLK8x8SIZE);

    /*
     * Do color space conversion
     * NOTE : We assume there that the number of MB's to process will not span multiple rows
     */
    switch (ps_codec->s_cfg.e_inp_color_fmt)
    {
        case IV_YUV_420SP_UV:
        case IV_YUV_420SP_VU:
            /* In case of 420 semi-planar input, copy last few rows to intermediate
               buffer as chroma trans functions access one extra byte due to interleaved input.
               This data will be padded if required */
            if (ps_proc->i4_mb_y == (ps_proc->i4_ht_mbs - 1) || u4_pad_bottom_sz || u4_pad_right_sz)
            {
                WORD32 num_rows = MB_SIZE;
                UWORD8 *pu1_src;
                UWORD8 *pu1_dst;
                WORD32 i;
                pu1_src = (UWORD8 *)ps_proc->s_inp_buf.s_raw_buf.apv_bufs[0] + (i4_mb_x * MB_SIZE) +
                          ps_proc->s_inp_buf.s_raw_buf.au4_strd[0] * (i4_mb_y * MB_SIZE);

                pu1_dst = ps_proc->pu1_src_buf_luma;

                /* If padding is required, we always copy luma, if padding isn't required we never copy luma. */
                if (u4_pad_bottom_sz || u4_pad_right_sz) {
                    if (ps_proc->i4_mb_y == (ps_proc->i4_ht_mbs - 1))
                        num_rows = MB_SIZE - u4_pad_bottom_sz;
                    for (i = 0; i < num_rows; i++)
                    {
                        memcpy(pu1_dst, pu1_src, ps_codec->s_cfg.u4_wd);
                        pu1_src += ps_proc->s_inp_buf.s_raw_buf.au4_strd[0];
                        pu1_dst += ps_proc->i4_src_strd;
                    }
                }
                pu1_src = (UWORD8 *)ps_proc->s_inp_buf.s_raw_buf.apv_bufs[1] + (i4_mb_x * BLK8x8SIZE) +
                          ps_proc->s_inp_buf.s_raw_buf.au4_strd[1] * (i4_mb_y * BLK8x8SIZE);
                pu1_dst = ps_proc->pu1_src_buf_chroma;

                /* Last MB row of chroma is copied unconditionally, since trans functions access an extra byte
                 * due to interleaved input
                 */
                if (ps_proc->i4_mb_y == (ps_proc->i4_ht_mbs - 1))
                    num_rows = (ps_codec->s_cfg.u4_disp_ht >> 1) - (ps_proc->i4_mb_y * BLK8x8SIZE);
                else
                    num_rows = BLK8x8SIZE;
                for (i = 0; i < num_rows; i++)
                {
                    memcpy(pu1_dst, pu1_src, ps_codec->s_cfg.u4_wd);
                    pu1_src += ps_proc->s_inp_buf.s_raw_buf.au4_strd[1];
                    pu1_dst += ps_proc->i4_src_chroma_strd;
                }

            }
            break;

        case IV_YUV_420P :
            pu1_y_buf_base = (UWORD8 *)ps_proc->s_inp_buf.s_raw_buf.apv_bufs[0] + (i4_mb_x * MB_SIZE) +
                            ps_proc->s_inp_buf.s_raw_buf.au4_strd[0] * (i4_mb_y * MB_SIZE);

            pu1_u_buf_base = (UWORD8 *)ps_proc->s_inp_buf.s_raw_buf.apv_bufs[1] + (i4_mb_x * BLK8x8SIZE) +
                            ps_proc->s_inp_buf.s_raw_buf.au4_strd[1] * (i4_mb_y * BLK8x8SIZE);

            pu1_v_buf_base = (UWORD8 *)ps_proc->s_inp_buf.s_raw_buf.apv_bufs[2] + (i4_mb_x * BLK8x8SIZE) +
                            ps_proc->s_inp_buf.s_raw_buf.au4_strd[2] * (i4_mb_y * BLK8x8SIZE);

            ps_codec->pf_ih264e_conv_420p_to_420sp(
                            pu1_y_buf_base, pu1_u_buf_base, pu1_v_buf_base,
                            ps_proc->pu1_src_buf_luma,
                            ps_proc->pu1_src_buf_chroma, u2_num_rows,
                            ps_codec->s_cfg.u4_disp_wd,
                            ps_proc->s_inp_buf.s_raw_buf.au4_strd[0],
                            ps_proc->s_inp_buf.s_raw_buf.au4_strd[1],
                            ps_proc->s_inp_buf.s_raw_buf.au4_strd[2],
                            ps_proc->i4_src_strd, ps_proc->i4_src_chroma_strd,
                            convert_uv_only);
            break;

        case IV_YUV_422ILE :
            pu1_y_buf_base =  (UWORD8 *)ps_proc->s_inp_buf.s_raw_buf.apv_bufs[0] + (i4_mb_x * MB_SIZE * 2)
                              + ps_proc->s_inp_buf.s_raw_buf.au4_strd[0] * (i4_mb_y * MB_SIZE);

            ps_codec->pf_ih264e_fmt_conv_422i_to_420sp(
                            ps_proc->pu1_src_buf_luma,
                            ps_proc->pu1_src_buf_chroma,
                            ps_proc->pu1_src_buf_chroma + 1, pu1_y_buf_base,
                            ps_codec->s_cfg.u4_disp_wd, u2_num_rows,
                            ps_proc->i4_src_strd, ps_proc->i4_src_chroma_strd,
                            ps_proc->i4_src_chroma_strd,
                            ps_proc->s_inp_buf.s_raw_buf.au4_strd[0] >> 1);
            break;

        default:
            break;
    }

    if (u4_pad_right_sz && (ps_proc->i4_mb_x == 0))
    {
        UWORD32 u4_pad_wd, u4_pad_ht;
        u4_pad_wd = (UWORD32)(ps_proc->i4_src_strd - ps_codec->s_cfg.u4_disp_wd);
        u4_pad_wd = MIN(u4_pad_right_sz, u4_pad_wd);
        u4_pad_ht = MB_SIZE;
        if(ps_proc->i4_mb_y == ps_proc->i4_ht_mbs - 1)
            u4_pad_ht = MIN(MB_SIZE, (MB_SIZE - u4_pad_bottom_sz));

        ih264_pad_right_luma(
                        ps_proc->pu1_src_buf_luma + ps_codec->s_cfg.u4_disp_wd,
                        ps_proc->i4_src_strd, u4_pad_ht, u4_pad_wd);

        ih264_pad_right_chroma(
                        ps_proc->pu1_src_buf_chroma + ps_codec->s_cfg.u4_disp_wd,
                        ps_proc->i4_src_chroma_strd, u4_pad_ht / 2, u4_pad_wd);
    }

    /* pad bottom edge */
    if (u4_pad_bottom_sz && (ps_proc->i4_mb_y == ps_proc->i4_ht_mbs - 1) && ps_proc->i4_mb_x == 0)
    {
        ih264_pad_bottom(ps_proc->pu1_src_buf_luma + (MB_SIZE - u4_pad_bottom_sz) * ps_proc->i4_src_strd,
                         ps_proc->i4_src_strd, ps_proc->i4_src_strd, u4_pad_bottom_sz);

        ih264_pad_bottom(ps_proc->pu1_src_buf_chroma + (MB_SIZE - u4_pad_bottom_sz) * ps_proc->i4_src_chroma_strd / 2,
                         ps_proc->i4_src_chroma_strd, ps_proc->i4_src_chroma_strd, (u4_pad_bottom_sz / 2));
    }


    /* packed mb coeff data */
    ps_proc->pv_mb_coeff_data = ((UWORD8 *)ps_proc->pv_pic_mb_coeff_data) + i4_mb_y * ps_codec->u4_size_coeff_data;

    /* packed mb header data */
    ps_proc->pv_mb_header_data = ((UWORD8 *)ps_proc->pv_pic_mb_header_data) + i4_mb_y * ps_codec->u4_size_header_data;

    /* slice index */
    ps_proc->i4_cur_slice_idx = ps_proc->pu1_slice_idx[i4_mb_y * ps_proc->i4_wd_mbs + i4_mb_x];

    /*********************************************************************/
    /* ih264e_init_quant_params() routine is called at the pic init level*/
    /* this would have initialized the qp.                               */
    /* TODO_LATER: currently it is assumed that quant params donot change*/
    /* across mb's. When they do calculate update ps_qp_params accordingly*/
    /*********************************************************************/

    /* init mv buffer ptr */
    ps_proc->ps_pu = ps_cur_mv_buf->ps_pic_pu + (i4_mb_y * ps_proc->i4_wd_mbs *
                     ((MB_SIZE * MB_SIZE) / (ENC_MIN_PU_SIZE * ENC_MIN_PU_SIZE)));

    /* Init co-located mv buffer */
    ps_proc->ps_colpu = ps_proc->aps_mv_buf[1]->ps_pic_pu + (i4_mb_y * ps_proc->i4_wd_mbs *
                        ((MB_SIZE * MB_SIZE) / (ENC_MIN_PU_SIZE * ENC_MIN_PU_SIZE)));

    if (i4_mb_y == 0)
    {
        ps_proc->ps_top_row_pu_ME = ps_cur_mv_buf->ps_pic_pu;
    }
    else
    {
        ps_proc->ps_top_row_pu_ME = ps_cur_mv_buf->ps_pic_pu + ((i4_mb_y - 1) * ps_proc->i4_wd_mbs *
                                    ((MB_SIZE * MB_SIZE) / (ENC_MIN_PU_SIZE * ENC_MIN_PU_SIZE)));
    }

    ps_proc->pu4_mb_pu_cnt = ps_cur_mv_buf->pu4_mb_pu_cnt + (i4_mb_y * ps_proc->i4_wd_mbs);

    /* mb type */
    ps_proc->u4_mb_type = I16x16;

    /* lambda */
    ps_proc->u4_lambda = gu1_qp0[ps_qp_params->u1_mb_qp];

    /* mb distortion */
    ps_proc->i4_mb_distortion = SHRT_MAX;

    if (i4_mb_x == 0)
    {
        ps_proc->s_left_mb_syntax_ele.i4_mb_distortion = 0;

        ps_proc->s_top_left_mb_syntax_ele.i4_mb_distortion = 0;

        ps_proc->s_top_left_mb_syntax_ME.i4_mb_distortion = 0;

        if (i4_mb_y == 0)
        {
            memset(ps_proc->ps_top_row_mb_syntax_ele, 0, (ps_proc->i4_wd_mbs + 1)*sizeof(mb_info_t));
        }
    }

    /* mb cost */
    ps_proc->i4_mb_cost = INT_MAX;

    /**********************/
    /* init deblk context */
    /**********************/
    ps_deblk->i4_mb_x = ps_proc->i4_mb_x;
    /* deblk lags the current mb proc by 1 row */
    /* NOTE: Intra prediction has to happen with non deblocked samples used as reference */
    /* Hence to deblk MB 0 of row 0, you have wait till MB 0 of row 1 is encoded. */
    /* For simplicity, we chose to lag deblking by 1 Row wrt to proc */
    ps_deblk->i4_mb_y = ps_proc->i4_mb_y - 1;

    /* buffer ptrs */
    ps_deblk->pu1_cur_pic_luma = ps_proc->pu1_rec_buf_luma_base + i4_rec_strd * (ps_deblk->i4_mb_y * MB_SIZE);
    ps_deblk->pu1_cur_pic_chroma = ps_proc->pu1_rec_buf_chroma_base + i4_rec_strd * (ps_deblk->i4_mb_y * BLK8x8SIZE);

    /* init deblk bs context */
    /* mb indices */
    ps_bs->i4_mb_x = ps_proc->i4_mb_x;
    ps_bs->i4_mb_y = ps_proc->i4_mb_y;

    /* init n_mb_process  context */
    ps_n_mb_ctxt->i4_mb_x = 0;
    ps_n_mb_ctxt->i4_mb_y = ps_deblk->i4_mb_y;
    ps_n_mb_ctxt->i4_n_mbs = ps_proc->i4_nmb_ntrpy;

    return IH264E_SUCCESS;
}

/**
*******************************************************************************
*
* @brief This function performs luma & chroma padding
*
* @par Description:
*
* @param[in] ps_proc
*  Process context corresponding to the job
*
* @param[in] pu1_curr_pic_luma
*  Pointer to luma buffer
*
* @param[in] pu1_curr_pic_chroma
*  Pointer to chroma buffer
*
* @param[in] i4_mb_x
*  mb index x
*
* @param[in] i4_mb_y
*  mb index y
*
*  @param[in] i4_pad_ht
*  number of rows to be padded
*
* @returns  error status
*
* @remarks none
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_pad_recon_buffer(process_ctxt_t *ps_proc,
                                       UWORD8 *pu1_curr_pic_luma,
                                       UWORD8 *pu1_curr_pic_chroma,
                                       WORD32 i4_mb_x,
                                       WORD32 i4_mb_y,
                                       WORD32 i4_pad_ht)
{
    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* strides */
    WORD32 i4_rec_strd = ps_proc->i4_rec_strd;

    if (i4_mb_x == 0)
    {
        /* padding left luma */
        ps_codec->pf_pad_left_luma(pu1_curr_pic_luma, i4_rec_strd, i4_pad_ht, PAD_LEFT);

        /* padding left chroma */
        ps_codec->pf_pad_left_chroma(pu1_curr_pic_chroma, i4_rec_strd, i4_pad_ht >> 1, PAD_LEFT);
    }
    if (i4_mb_x == ps_proc->i4_wd_mbs - 1)
    {
        /* padding right luma */
        ps_codec->pf_pad_right_luma(pu1_curr_pic_luma + MB_SIZE, i4_rec_strd, i4_pad_ht, PAD_RIGHT);

        /* padding right chroma */
        ps_codec->pf_pad_right_chroma(pu1_curr_pic_chroma + MB_SIZE, i4_rec_strd, i4_pad_ht >> 1, PAD_RIGHT);

        if (i4_mb_y == ps_proc->i4_ht_mbs - 1)
        {
            UWORD8 *pu1_rec_luma = pu1_curr_pic_luma + MB_SIZE + PAD_RIGHT + ((i4_pad_ht - 1) * i4_rec_strd);
            UWORD8 *pu1_rec_chroma = pu1_curr_pic_chroma + MB_SIZE + PAD_RIGHT + (((i4_pad_ht >> 1) - 1) * i4_rec_strd);

            /* padding bottom luma */
            ps_codec->pf_pad_bottom(pu1_rec_luma, i4_rec_strd, i4_rec_strd, PAD_BOT);

            /* padding bottom chroma */
            ps_codec->pf_pad_bottom(pu1_rec_chroma, i4_rec_strd, i4_rec_strd, (PAD_BOT >> 1));
        }
    }

    if (i4_mb_y == 0)
    {
        UWORD8 *pu1_rec_luma = pu1_curr_pic_luma;
        UWORD8 *pu1_rec_chroma = pu1_curr_pic_chroma;
        WORD32 wd = MB_SIZE;

        if (i4_mb_x == 0)
        {
            pu1_rec_luma -= PAD_LEFT;
            pu1_rec_chroma -= PAD_LEFT;

            wd += PAD_LEFT;
        }
        if (i4_mb_x == ps_proc->i4_wd_mbs - 1)
        {
            wd += PAD_RIGHT;
        }

        /* padding top luma */
        ps_codec->pf_pad_top(pu1_rec_luma, i4_rec_strd, wd, PAD_TOP);

        /* padding top chroma */
        ps_codec->pf_pad_top(pu1_rec_chroma, i4_rec_strd, wd, (PAD_TOP >> 1));
    }

    return IH264E_SUCCESS;
}




/**
*******************************************************************************
*
* @brief This function performs deblocking, padding and halfpel generation for
*  'n' MBs
*
* @par Description:
*
* @param[in] ps_proc
*  Process context corresponding to the job
*
* @param[in] pu1_curr_pic_luma
* Current MB being processed(Luma)
*
* @param[in] pu1_curr_pic_chroma
* Current MB being processed(Chroma)
*
* @param[in] i4_mb_x
* Column value of current MB processed
*
* @param[in] i4_mb_y
* Curent row processed
*
* @returns  error status
*
* @remarks none
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_dblk_pad_hpel_processing_n_mbs(process_ctxt_t *ps_proc,
                                                     UWORD8 *pu1_curr_pic_luma,
                                                     UWORD8 *pu1_curr_pic_chroma,
                                                     WORD32 i4_mb_x,
                                                     WORD32 i4_mb_y)
{
    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* n_mb processing context */
    n_mb_process_ctxt_t *ps_n_mb_ctxt = &ps_proc->s_n_mb_ctxt;

    /* deblk context */
    deblk_ctxt_t *ps_deblk = &ps_proc->s_deblk_ctxt;

    /* strides */
    WORD32 i4_rec_strd = ps_proc->i4_rec_strd;

    /* loop variables */
    WORD32 row, i, j, col;

    /* Padding Width */
    UWORD32 u4_pad_wd;

    /* deblk_map of the row being deblocked */
    UWORD8 *pu1_deblk_map = ps_proc->pu1_deblk_map + ps_deblk->i4_mb_y * ps_proc->i4_wd_mbs;

    /* deblk_map_previous row */
    UWORD8 *pu1_deblk_map_prev_row = pu1_deblk_map - ps_proc->i4_wd_mbs;

    WORD32 u4_pad_top = 0;

    WORD32 u4_deblk_prev_row = 0;

    /* Number of mbs to be processed */
    WORD32 i4_n_mbs = ps_n_mb_ctxt->i4_n_mbs;

    /* Number of mbs  actually processed
     * (at the end of a row, when remaining number of MBs are less than i4_n_mbs) */
    WORD32 i4_n_mb_process_count = 0;

    UWORD8 *pu1_pad_bottom_src = NULL;

    UWORD8 *pu1_pad_src_luma = NULL;
    UWORD8 *pu1_pad_src_chroma = NULL;

    if (ps_proc->u4_disable_deblock_level == 1)
    {
        /* If left most MB is processed, then pad left */
        if (i4_mb_x == 0)
        {
            /* padding left luma */
            ps_codec->pf_pad_left_luma(pu1_curr_pic_luma, i4_rec_strd, MB_SIZE, PAD_LEFT);

            /* padding left chroma */
            ps_codec->pf_pad_left_chroma(pu1_curr_pic_chroma, i4_rec_strd, MB_SIZE >> 1, PAD_LEFT);
        }
        /*last col*/
        if (i4_mb_x == (ps_proc->i4_wd_mbs - 1))
        {
            /* padding right luma */
            ps_codec->pf_pad_right_luma(pu1_curr_pic_luma + MB_SIZE, i4_rec_strd, MB_SIZE, PAD_RIGHT);

            /* padding right chroma */
            ps_codec->pf_pad_right_chroma(pu1_curr_pic_chroma + MB_SIZE, i4_rec_strd, MB_SIZE >> 1, PAD_RIGHT);
        }
    }

    if ((i4_mb_y > 0) || (i4_mb_y == (ps_proc->i4_ht_mbs - 1)))
    {
        /* if number of mb's to be processed are less than 'N', go back.
         * exception to the above clause is end of row */
        if ( ((i4_mb_x - (ps_n_mb_ctxt->i4_mb_x - 1)) < i4_n_mbs) && (i4_mb_x < (ps_proc->i4_wd_mbs - 1)) )
        {
            return IH264E_SUCCESS;
        }
        else
        {
            i4_n_mb_process_count = MIN(i4_mb_x - (ps_n_mb_ctxt->i4_mb_x - 1), i4_n_mbs);

            /* performing deblocking for required number of MBs */
            if ((i4_mb_y > 0) && (ps_proc->u4_disable_deblock_level != 1))
            {
                u4_deblk_prev_row = 1;

                /* checking whether the top rows are deblocked */
                for (col = 0; col < i4_n_mb_process_count; col++)
                {
                    u4_deblk_prev_row &= pu1_deblk_map_prev_row[ps_deblk->i4_mb_x + col];
                }

                /* checking whether the top right MB is deblocked */
                if ((ps_deblk->i4_mb_x + i4_n_mb_process_count) != ps_proc->i4_wd_mbs)
                {
                    u4_deblk_prev_row &= pu1_deblk_map_prev_row[ps_deblk->i4_mb_x + i4_n_mb_process_count];
                }

                /* Top or Top right MBs not deblocked */
                if ((u4_deblk_prev_row != 1) && (i4_mb_y > 0))
                {
                    return IH264E_SUCCESS;
                }

                for (row = 0; row < i4_n_mb_process_count; row++)
                {
                    ih264e_deblock_mb(ps_proc, ps_deblk);

                    pu1_deblk_map[ps_deblk->i4_mb_x] = 1;

                    if (ps_deblk->i4_mb_y > 0)
                    {
                        if (ps_deblk->i4_mb_x == 0)/* If left most MB is processed, then pad left*/
                        {
                            /* padding left luma */
                            ps_codec->pf_pad_left_luma(ps_deblk->pu1_cur_pic_luma - i4_rec_strd * MB_SIZE, i4_rec_strd, MB_SIZE, PAD_LEFT);

                            /* padding left chroma */
                            ps_codec->pf_pad_left_chroma(ps_deblk->pu1_cur_pic_chroma - i4_rec_strd * BLK8x8SIZE, i4_rec_strd, MB_SIZE >> 1, PAD_LEFT);
                        }

                        if (ps_deblk->i4_mb_x == (ps_proc->i4_wd_mbs - 1))/*last column*/
                        {
                            /* padding right luma */
                            ps_codec->pf_pad_right_luma(ps_deblk->pu1_cur_pic_luma - i4_rec_strd * MB_SIZE + MB_SIZE, i4_rec_strd, MB_SIZE, PAD_RIGHT);

                            /* padding right chroma */
                            ps_codec->pf_pad_right_chroma(ps_deblk->pu1_cur_pic_chroma - i4_rec_strd * BLK8x8SIZE + MB_SIZE, i4_rec_strd, MB_SIZE >> 1, PAD_RIGHT);
                        }
                    }
                    ps_deblk->i4_mb_x++;

                    ps_deblk->pu1_cur_pic_luma += MB_SIZE;
                    ps_deblk->pu1_cur_pic_chroma += MB_SIZE;

                }
            }
            else if(i4_mb_y > 0)
            {
                ps_deblk->i4_mb_x += i4_n_mb_process_count;

                ps_deblk->pu1_cur_pic_luma += i4_n_mb_process_count * MB_SIZE;
                ps_deblk->pu1_cur_pic_chroma += i4_n_mb_process_count * MB_SIZE;
            }

            if (i4_mb_y == 2)
            {
                u4_pad_wd = i4_n_mb_process_count * MB_SIZE;
                u4_pad_top = ps_n_mb_ctxt->i4_mb_x * MB_SIZE;

                if (ps_n_mb_ctxt->i4_mb_x == 0)
                {
                    u4_pad_wd += PAD_LEFT;
                    u4_pad_top = -PAD_LEFT;
                }

                if (i4_mb_x == ps_proc->i4_wd_mbs - 1)
                {
                    u4_pad_wd += PAD_RIGHT;
                }

                /* padding top luma */
                ps_codec->pf_pad_top(ps_proc->pu1_rec_buf_luma_base + u4_pad_top, i4_rec_strd, u4_pad_wd, PAD_TOP);

                /* padding top chroma */
                ps_codec->pf_pad_top(ps_proc->pu1_rec_buf_chroma_base + u4_pad_top, i4_rec_strd, u4_pad_wd, (PAD_TOP >> 1));
            }

            ps_n_mb_ctxt->i4_mb_x += i4_n_mb_process_count;

            if (i4_mb_x == ps_proc->i4_wd_mbs - 1)
            {
                if (ps_proc->i4_mb_y == ps_proc->i4_ht_mbs - 1)
                {
                    /* Bottom Padding is done in one stretch for the entire width */
                    if (ps_proc->u4_disable_deblock_level != 1)
                    {
                        ps_deblk->pu1_cur_pic_luma = ps_proc->pu1_rec_buf_luma_base + (ps_proc->i4_ht_mbs - 1) * i4_rec_strd * MB_SIZE;

                        ps_deblk->pu1_cur_pic_chroma = ps_proc->pu1_rec_buf_chroma_base + (ps_proc->i4_ht_mbs - 1) * i4_rec_strd * BLK8x8SIZE;

                        ps_n_mb_ctxt->i4_mb_x = 0;
                        ps_n_mb_ctxt->i4_mb_y = ps_proc->i4_mb_y;
                        ps_deblk->i4_mb_x = 0;
                        ps_deblk->i4_mb_y = ps_proc->i4_mb_y;

                        /* update pic qp map (as update_proc_ctxt is still not called for the last MB) */
                        ps_proc->s_deblk_ctxt.s_bs_ctxt.pu1_pic_qp[(i4_mb_y * ps_proc->i4_wd_mbs) + i4_mb_x] = ps_proc->u4_mb_qp;

                        i4_n_mb_process_count = (ps_proc->i4_wd_mbs) % i4_n_mbs;

                        j = (ps_proc->i4_wd_mbs) / i4_n_mbs;

                        for (i = 0; i < j; i++)
                        {
                            for (col = 0; col < i4_n_mbs; col++)
                            {
                                ih264e_deblock_mb(ps_proc, ps_deblk);

                                pu1_deblk_map[ps_deblk->i4_mb_x] = 1;

                                ps_deblk->i4_mb_x++;
                                ps_deblk->pu1_cur_pic_luma += MB_SIZE;
                                ps_deblk->pu1_cur_pic_chroma += MB_SIZE;
                                ps_n_mb_ctxt->i4_mb_x++;
                            }
                        }

                        for (col = 0; col < i4_n_mb_process_count; col++)
                        {
                            ih264e_deblock_mb(ps_proc, ps_deblk);

                            pu1_deblk_map[ps_deblk->i4_mb_x] = 1;

                            ps_deblk->i4_mb_x++;
                            ps_deblk->pu1_cur_pic_luma += MB_SIZE;
                            ps_deblk->pu1_cur_pic_chroma += MB_SIZE;
                            ps_n_mb_ctxt->i4_mb_x++;
                        }

                        pu1_pad_src_luma = ps_proc->pu1_rec_buf_luma_base + (ps_proc->i4_ht_mbs - 2) * MB_SIZE * i4_rec_strd;

                        pu1_pad_src_chroma = ps_proc->pu1_rec_buf_chroma_base + (ps_proc->i4_ht_mbs - 2) * BLK8x8SIZE * i4_rec_strd;

                        /* padding left luma */
                        ps_codec->pf_pad_left_luma(pu1_pad_src_luma, i4_rec_strd, MB_SIZE, PAD_LEFT);

                        /* padding left chroma */
                        ps_codec->pf_pad_left_chroma(pu1_pad_src_chroma, i4_rec_strd, BLK8x8SIZE, PAD_LEFT);

                        pu1_pad_src_luma += i4_rec_strd * MB_SIZE;
                        pu1_pad_src_chroma += i4_rec_strd * BLK8x8SIZE;

                        /* padding left luma */
                        ps_codec->pf_pad_left_luma(pu1_pad_src_luma, i4_rec_strd, MB_SIZE, PAD_LEFT);

                        /* padding left chroma */
                        ps_codec->pf_pad_left_chroma(pu1_pad_src_chroma, i4_rec_strd, BLK8x8SIZE, PAD_LEFT);

                        pu1_pad_src_luma = ps_proc->pu1_rec_buf_luma_base + (ps_proc->i4_ht_mbs - 2) * MB_SIZE * i4_rec_strd + (ps_proc->i4_wd_mbs) * MB_SIZE;

                        pu1_pad_src_chroma = ps_proc->pu1_rec_buf_chroma_base + (ps_proc->i4_ht_mbs - 2) * BLK8x8SIZE * i4_rec_strd + (ps_proc->i4_wd_mbs) * MB_SIZE;

                        /* padding right luma */
                        ps_codec->pf_pad_right_luma(pu1_pad_src_luma, i4_rec_strd, MB_SIZE, PAD_RIGHT);

                        /* padding right chroma */
                        ps_codec->pf_pad_right_chroma(pu1_pad_src_chroma, i4_rec_strd, BLK8x8SIZE, PAD_RIGHT);

                        pu1_pad_src_luma += i4_rec_strd * MB_SIZE;
                        pu1_pad_src_chroma += i4_rec_strd * BLK8x8SIZE;

                        /* padding right luma */
                        ps_codec->pf_pad_right_luma(pu1_pad_src_luma, i4_rec_strd, MB_SIZE, PAD_RIGHT);

                        /* padding right chroma */
                        ps_codec->pf_pad_right_chroma(pu1_pad_src_chroma, i4_rec_strd, BLK8x8SIZE, PAD_RIGHT);

                    }

                    /* In case height is less than 2 MBs pad top */
                    if (ps_proc->i4_ht_mbs <= 2)
                    {
                        UWORD8 *pu1_pad_top_src;
                        /* padding top luma */
                        pu1_pad_top_src = ps_proc->pu1_rec_buf_luma_base - PAD_LEFT;
                        ps_codec->pf_pad_top(pu1_pad_top_src, i4_rec_strd, i4_rec_strd, PAD_TOP);

                        /* padding top chroma */
                        pu1_pad_top_src = ps_proc->pu1_rec_buf_chroma_base - PAD_LEFT;
                        ps_codec->pf_pad_top(pu1_pad_top_src, i4_rec_strd, i4_rec_strd, (PAD_TOP >> 1));
                    }

                    /* padding bottom luma */
                    pu1_pad_bottom_src = ps_proc->pu1_rec_buf_luma_base + ps_proc->i4_ht_mbs * MB_SIZE * i4_rec_strd - PAD_LEFT;
                    ps_codec->pf_pad_bottom(pu1_pad_bottom_src, i4_rec_strd, i4_rec_strd, PAD_BOT);

                    /* padding bottom chroma */
                    pu1_pad_bottom_src = ps_proc->pu1_rec_buf_chroma_base + ps_proc->i4_ht_mbs * (MB_SIZE >> 1) * i4_rec_strd - PAD_LEFT;
                    ps_codec->pf_pad_bottom(pu1_pad_bottom_src, i4_rec_strd, i4_rec_strd, (PAD_BOT >> 1));
                }
            }
        }
    }

    return IH264E_SUCCESS;
}


/**
*******************************************************************************
*
* @brief This function performs luma & chroma core coding for a set of mb's.
*
* @par Description:
*  The mb to be coded is taken and is evaluated over a predefined set of modes
*  (intra (i16, i4, i8)/inter (mv, skip)) for best cost. The mode with least cost
*  is selected and using intra/inter prediction filters, prediction is carried out.
*  The deviation between src and pred signal constitutes error signal. This error
*  signal is transformed (hierarchical transform if necessary) and quantized. The
*  quantized residue is packed in to entropy buffer for entropy coding. This is
*  repeated for all the mb's enlisted under the job.
*
* @param[in] ps_proc
*  Process context corresponding to the job
*
* @returns  error status
*
* @remarks none
*
*******************************************************************************
*/
WORD32 ih264e_process(process_ctxt_t *ps_proc)
{
    /* error status */
    WORD32 error_status = IH264_SUCCESS;

    /* codec context */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* cbp luma, chroma */
    UWORD32 u4_cbp_l, u4_cbp_c;

    /* width in mbs */
    WORD32 i4_wd_mbs = ps_proc->i4_wd_mbs;

    /* loop var */
    WORD32  i4_mb_idx, i4_mb_cnt = ps_proc->i4_mb_cnt;

    /* valid modes */
    UWORD32 u4_valid_modes = 0;

    /* gate threshold */
    WORD32 i4_gate_threshold = 0;

    /* is intra */
    WORD32 luma_idx, chroma_idx, is_intra;

    /* temp variables */
    WORD32 ctxt_sel = ps_proc->i4_encode_api_call_cnt % MAX_CTXT_SETS;

    /*
     * list of modes for evaluation
     * -------------------------------------------------------------------------
     * Note on enabling I4x4 and I16x16
     * At very low QP's the hadamard transform in I16x16 will push up the maximum
     * coeff value very high. CAVLC may not be able to represent the value and
     * hence the stream may not be decodable in some clips.
     * Hence at low QPs, we will enable I4x4 and disable I16x16 irrespective of preset.
     */
    if (ps_proc->i4_slice_type == ISLICE)
    {
        if (ps_proc->u4_frame_qp > 10)
        {
            /* enable intra 16x16 */
            u4_valid_modes |= ps_codec->s_cfg.u4_enable_intra_16x16 ? (1 << I16x16) : 0;

            /* enable intra 8x8 */
            u4_valid_modes |= ps_codec->s_cfg.u4_enable_intra_8x8 ? (1 << I8x8) : 0;
        }

        /* enable intra 4x4 */
        u4_valid_modes |= ps_codec->s_cfg.u4_enable_intra_4x4 ? (1 << I4x4) : 0;
        u4_valid_modes |= (ps_proc->u4_frame_qp <= 10) << I4x4;

    }
    else if (ps_proc->i4_slice_type == PSLICE)
    {
        if (ps_proc->u4_frame_qp > 10)
        {
            /* enable intra 16x16 */
            u4_valid_modes |= ps_codec->s_cfg.u4_enable_intra_16x16 ? (1 << I16x16) : 0;
        }

        /* enable intra 4x4 */
        if (ps_codec->s_cfg.u4_enc_speed_preset == IVE_SLOWEST)
        {
            u4_valid_modes |= ps_codec->s_cfg.u4_enable_intra_4x4 ? (1 << I4x4) : 0;
        }
        u4_valid_modes |= (ps_proc->u4_frame_qp <= 10) << I4x4;

        /* enable inter P16x16 */
        u4_valid_modes |= (1 << P16x16);
    }
    else if (ps_proc->i4_slice_type == BSLICE)
    {
        if (ps_proc->u4_frame_qp > 10)
        {
            /* enable intra 16x16 */
            u4_valid_modes |= ps_codec->s_cfg.u4_enable_intra_16x16 ? (1 << I16x16) : 0;
        }

        /* enable intra 4x4 */
        if (ps_codec->s_cfg.u4_enc_speed_preset == IVE_SLOWEST)
        {
            u4_valid_modes |= ps_codec->s_cfg.u4_enable_intra_4x4 ? (1 << I4x4) : 0;
        }
        u4_valid_modes |= (ps_proc->u4_frame_qp <= 10) << I4x4;

        /* enable inter B16x16 */
        u4_valid_modes |= (1 << B16x16);
    }


    /* init entropy */
    ps_proc->s_entropy.i4_mb_x = ps_proc->i4_mb_x;
    ps_proc->s_entropy.i4_mb_y = ps_proc->i4_mb_y;
    ps_proc->s_entropy.i4_mb_cnt = MIN(ps_proc->i4_nmb_ntrpy, i4_wd_mbs - ps_proc->i4_mb_x);

    /* compute recon when :
     *   1. current frame is to be used as a reference
     *   2. dump recon for bit stream sanity check
     */
    ps_proc->u4_compute_recon = ps_codec->u4_is_curr_frm_ref ||
                                ps_codec->s_cfg.u4_enable_recon;

    /* Encode 'n' macroblocks,
     * 'n' being the number of mbs dictated by current proc ctxt */
    for (i4_mb_idx = 0; i4_mb_idx < i4_mb_cnt; i4_mb_idx ++)
    {
        /* since we have not yet found sad, we have not yet got min sad */
        /* we need to initialize these variables for each MB */
        /* TODO how to get the min sad into the codec */
        ps_proc->u4_min_sad = ps_codec->s_cfg.i4_min_sad;
        ps_proc->u4_min_sad_reached = 0;

        /* mb analysis */
        {
            /* temp var */
            WORD32 i4_mb_id = ps_proc->i4_mb_x + ps_proc->i4_mb_y * i4_wd_mbs;

            /* force intra refresh ? */
            WORD32 i4_air_enable_inter = (ps_codec->s_cfg.e_air_mode == IVE_AIR_MODE_NONE) ||
                            (ps_codec->pu2_intr_rfrsh_map[i4_mb_id] != ps_codec->i4_air_pic_cnt);

            /* evaluate inter 16x16 modes */
            if ((u4_valid_modes & (1 << P16x16)) || (u4_valid_modes & (1 << B16x16)))
            {
                /* compute nmb me */
                if (ps_proc->i4_mb_x % ps_proc->u4_nmb_me == 0)
                {
                    ih264e_compute_me_nmb(ps_proc, MIN((WORD32)ps_proc->u4_nmb_me,
                                                       i4_wd_mbs - ps_proc->i4_mb_x));
                }

                /* set pointers to ME data appropriately for other modules to use */
                {
                    UWORD32 u4_mb_index = ps_proc->i4_mb_x % ps_proc->u4_nmb_me ;

                    /* get the min sad condition for current mb */
                    ps_proc->u4_min_sad_reached = ps_proc->ps_nmb_info[u4_mb_index].u4_min_sad_reached;
                    ps_proc->u4_min_sad = ps_proc->ps_nmb_info[u4_mb_index].u4_min_sad;

                    ps_proc->ps_skip_mv = &(ps_proc->ps_nmb_info[u4_mb_index].as_skip_mv[0]);
                    ps_proc->ps_ngbr_avbl = &(ps_proc->ps_nmb_info[u4_mb_index].s_ngbr_avbl);
                    ps_proc->ps_pred_mv = &(ps_proc->ps_nmb_info[u4_mb_index].as_pred_mv[0]);

                    ps_proc->i4_mb_distortion = ps_proc->ps_nmb_info[u4_mb_index].i4_mb_distortion;
                    ps_proc->i4_mb_cost = ps_proc->ps_nmb_info[u4_mb_index].i4_mb_cost;
                    ps_proc->u4_min_sad = ps_proc->ps_nmb_info[u4_mb_index].u4_min_sad;
                    ps_proc->u4_min_sad_reached = ps_proc->ps_nmb_info[u4_mb_index].u4_min_sad_reached;
                    ps_proc->u4_mb_type = ps_proc->ps_nmb_info[u4_mb_index].u4_mb_type;

                    /* get the best sub pel buffer */
                    ps_proc->pu1_best_subpel_buf = ps_proc->ps_nmb_info[u4_mb_index].pu1_best_sub_pel_buf;
                    ps_proc->u4_bst_spel_buf_strd = ps_proc->ps_nmb_info[u4_mb_index].u4_bst_spel_buf_strd;
                }
                ih264e_derive_nghbr_avbl_of_mbs(ps_proc);
            }
            else
            {
                /* Derive neighbor availability for the current macroblock */
                ps_proc->ps_ngbr_avbl = &ps_proc->s_ngbr_avbl;

                ih264e_derive_nghbr_avbl_of_mbs(ps_proc);
            }

            /*
             * If air says intra, we need to force the following code path to evaluate intra
             * The easy way is just to say that the inter cost is too much
             */
            if (!i4_air_enable_inter)
            {
                ps_proc->u4_min_sad_reached = 0;
                ps_proc->i4_mb_cost = INT_MAX;
                ps_proc->i4_mb_distortion = INT_MAX;
            }
            else if (ps_proc->u4_mb_type == PSKIP)
            {
                goto UPDATE_MB_INFO;
            }

            /* wait until the proc of [top + 1] mb is computed.
             * We wait till the proc dependencies are satisfied */
             if(ps_proc->i4_mb_y > 0)
             {
                /* proc map */
                UWORD8  *pu1_proc_map_top;

                pu1_proc_map_top = ps_proc->pu1_proc_map + ((ps_proc->i4_mb_y - 1) * i4_wd_mbs);

                while (1)
                {
                    volatile UWORD8 *pu1_buf;
                    WORD32 idx = i4_mb_idx + 1;

                    idx = MIN(idx, ((WORD32)ps_codec->s_cfg.i4_wd_mbs - 1));
                    pu1_buf =  pu1_proc_map_top + idx;
                    if(*pu1_buf)
                        break;
                    ithread_yield();
                }
            }

            /* If we already have the minimum sad, there is no point in searching for sad again */
            if (ps_proc->u4_min_sad_reached == 0)
            {
                /* intra gating in inter slices */
                /* No need of gating if we want to force intra, we need to find the threshold only if inter is enabled by AIR*/
                if (i4_air_enable_inter && ps_proc->i4_slice_type != ISLICE && ps_codec->u4_inter_gate)
                {
                    /* distortion of neighboring blocks */
                    WORD32 i4_distortion[4];

                    i4_distortion[0] = ps_proc->s_left_mb_syntax_ele.i4_mb_distortion;

                    i4_distortion[1] = ps_proc->ps_top_row_mb_syntax_ele[ps_proc->i4_mb_x].i4_mb_distortion;

                    i4_distortion[2] = ps_proc->ps_top_row_mb_syntax_ele[ps_proc->i4_mb_x + 1].i4_mb_distortion;

                    i4_distortion[3] = ps_proc->s_top_left_mb_syntax_ele.i4_mb_distortion;

                    i4_gate_threshold = (i4_distortion[0] + i4_distortion[1] + i4_distortion[2] + i4_distortion[3]) >> 2;

                }


                /* If we are going to force intra we need to evaluate intra irrespective of gating */
                if ( (!i4_air_enable_inter) || ((i4_gate_threshold + 16 *((WORD32) ps_proc->u4_lambda)) < ps_proc->i4_mb_distortion))
                {
                    /* evaluate intra 4x4 modes */
                    if (u4_valid_modes & (1 << I4x4))
                    {
                        if (ps_codec->s_cfg.u4_enc_speed_preset == IVE_SLOWEST)
                        {
                            ih264e_evaluate_intra4x4_modes_for_least_cost_rdopton(ps_proc);
                        }
                        else
                        {
                            ih264e_evaluate_intra4x4_modes_for_least_cost_rdoptoff(ps_proc);
                        }
                    }

                    /* evaluate intra 16x16 modes */
                    if (u4_valid_modes & (1 << I16x16))
                    {
                        ih264e_evaluate_intra16x16_modes_for_least_cost_rdoptoff(ps_proc);
                    }

                    /* evaluate intra 8x8 modes */
                    if (u4_valid_modes & (1 << I8x8))
                    {
                        ih264e_evaluate_intra8x8_modes_for_least_cost_rdoptoff(ps_proc);
                    }

                }
        }
     }

        /* is intra */
        if (ps_proc->u4_mb_type == I4x4 || ps_proc->u4_mb_type == I16x16 || ps_proc->u4_mb_type == I8x8)
        {
            luma_idx = ps_proc->u4_mb_type;
            chroma_idx = 0;
            is_intra = 1;

            /* evaluate chroma blocks for intra */
            ih264e_evaluate_chroma_intra8x8_modes_for_least_cost_rdoptoff(ps_proc);
        }
        else
        {
            luma_idx = 3;
            chroma_idx = 1;
            is_intra = 0;
        }
        ps_proc->u4_is_intra = is_intra;
        ps_proc->ps_pu->b1_intra_flag = is_intra;

        /* redo MV pred of neighbors in the case intra mb */
        /* TODO : currently called unconditionally, needs to be called only in the case of intra
         * to modify neighbors */
        if (ps_proc->i4_slice_type != ISLICE)
        {
            ih264e_mv_pred(ps_proc, ps_proc->i4_slice_type);
        }

        /* Perform luma mb core coding */
        u4_cbp_l = (ps_codec->luma_energy_compaction)[luma_idx](ps_proc);

        /* Perform luma mb core coding */
        u4_cbp_c = (ps_codec->chroma_energy_compaction)[chroma_idx](ps_proc);

        /* coded block pattern */
        ps_proc->u4_cbp = (u4_cbp_c << 4) | u4_cbp_l;

        if (!ps_proc->u4_is_intra)
        {
            if (ps_proc->i4_slice_type == BSLICE)
            {
                if (ih264e_find_bskip_params(ps_proc, PRED_L0))
                {
                    ps_proc->u4_mb_type = (ps_proc->u4_cbp) ? BDIRECT : BSKIP;
                }
            }
            else if(!ps_proc->u4_cbp)
            {
                if (ih264e_find_pskip_params(ps_proc, PRED_L0))
                {
                    ps_proc->u4_mb_type = PSKIP;
                }
            }
        }

UPDATE_MB_INFO:

        /* Update mb sad, mb qp and intra mb cost. Will be used by rate control */
        ih264e_update_rc_mb_info(&ps_proc->s_frame_info, ps_proc);

        /**********************************************************************/
        /* if disable deblock level is '0' this implies enable deblocking for */
        /* all edges of all macroblocks with out any restrictions             */
        /*                                                                    */
        /* if disable deblock level is '1' this implies disable deblocking for*/
        /* all edges of all macroblocks with out any restrictions             */
        /*                                                                    */
        /* if disable deblock level is '2' this implies enable deblocking for */
        /* all edges of all macroblocks except edges overlapping with slice   */
        /* boundaries. This option is not currently supported by the encoder  */
        /* hence the slice map should be of no significance to perform debloc */
        /* king                                                               */
        /**********************************************************************/

        if (ps_proc->u4_compute_recon)
        {
            /* deblk context */
            /* src pointers */
            UWORD8 *pu1_cur_pic_luma = ps_proc->pu1_rec_buf_luma;
            UWORD8 *pu1_cur_pic_chroma = ps_proc->pu1_rec_buf_chroma;

            /* src indices */
            UWORD32 i4_mb_x = ps_proc->i4_mb_x;
            UWORD32 i4_mb_y = ps_proc->i4_mb_y;

            /* compute blocking strength */
            if (ps_proc->u4_disable_deblock_level != 1)
            {
                ih264e_compute_bs(ps_proc);
            }

            /* nmb deblocking and hpel and padding */
            ih264e_dblk_pad_hpel_processing_n_mbs(ps_proc, pu1_cur_pic_luma,
                                                  pu1_cur_pic_chroma, i4_mb_x,
                                                  i4_mb_y);
        }

        /* update the context after for coding next mb */
        error_status |= ih264e_update_proc_ctxt(ps_proc);

        /* Once the last row is processed, mark the buffer status appropriately */
        if (ps_proc->i4_ht_mbs == ps_proc->i4_mb_y)
        {
            /* Pointer to current picture buffer structure */
            pic_buf_t *ps_cur_pic = ps_proc->ps_cur_pic;

            /* Pointer to current picture's mv buffer structure */
            mv_buf_t *ps_cur_mv_buf = ps_proc->ps_cur_mv_buf;

            /**********************************************************************/
            /* if disable deblock level is '0' this implies enable deblocking for */
            /* all edges of all macroblocks with out any restrictions             */
            /*                                                                    */
            /* if disable deblock level is '1' this implies disable deblocking for*/
            /* all edges of all macroblocks with out any restrictions             */
            /*                                                                    */
            /* if disable deblock level is '2' this implies enable deblocking for */
            /* all edges of all macroblocks except edges overlapping with slice   */
            /* boundaries. This option is not currently supported by the encoder  */
            /* hence the slice map should be of no significance to perform debloc */
            /* king                                                               */
            /**********************************************************************/
            error_status |= ih264_buf_mgr_release(ps_codec->pv_mv_buf_mgr, ps_cur_mv_buf->i4_buf_id , BUF_MGR_CODEC);

            error_status |= ih264_buf_mgr_release(ps_codec->pv_ref_buf_mgr, ps_cur_pic->i4_buf_id , BUF_MGR_CODEC);

            if (ps_codec->s_cfg.u4_enable_recon)
            {
                /* pic cnt */
                ps_codec->as_rec_buf[ctxt_sel].i4_pic_cnt = ps_proc->i4_pic_cnt;

                /* rec buffers */
                ps_codec->as_rec_buf[ctxt_sel].s_pic_buf  = *ps_proc->ps_cur_pic;

                /* is last? */
                ps_codec->as_rec_buf[ctxt_sel].u4_is_last = ps_proc->s_entropy.u4_is_last;

                /* frame time stamp */
                ps_codec->as_rec_buf[ctxt_sel].u4_timestamp_high = ps_proc->s_entropy.u4_timestamp_high;
                ps_codec->as_rec_buf[ctxt_sel].u4_timestamp_low = ps_proc->s_entropy.u4_timestamp_low;
            }

        }
    }

    DEBUG_HISTOGRAM_DUMP(ps_codec->s_cfg.i4_ht_mbs == ps_proc->i4_mb_y);

    return error_status;
}

/**
*******************************************************************************
*
* @brief
*  Function to update rc context after encoding
*
* @par   Description
*  This function updates the rate control context after the frame is encoded.
*  Number of bits consumed by the current frame, frame distortion, frame cost,
*  number of intra/inter mb's, ... are passed on to rate control context for
*  updating the rc model.
*
* @param[in] ps_codec
*  Handle to codec context
*
* @param[in] ctxt_sel
*  frame context selector
*
* @param[in] pic_cnt
*  pic count
*
* @returns i4_stuffing_byte
*  number of stuffing bytes (if necessary)
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_update_rc_post_enc(codec_t *ps_codec, WORD32 ctxt_sel, WORD32 i4_is_first_frm)
{
    /* proc set base idx */
    WORD32 i4_proc_ctxt_sel_base = ctxt_sel ? (MAX_PROCESS_CTXT / 2) : 0;

    /* proc ctxt */
    process_ctxt_t *ps_proc = &ps_codec->as_process[i4_proc_ctxt_sel_base];

    /* frame qp */
    UWORD8 u1_frame_qp = ps_codec->u4_frame_qp;

    /* cbr rc return status */
    WORD32 i4_stuffing_byte = 0;

    /* current frame stats */
    frame_info_t s_frame_info;
    picture_type_e rc_pic_type;

    /* temp var */
    WORD32 i, j;

    /********************************************************************/
    /*                            BEGIN INIT                            */
    /********************************************************************/

    /* init frame info */
    irc_init_frame_info(&s_frame_info);

    /* get frame info */
    for (i = 0; i < (WORD32)ps_codec->s_cfg.u4_num_cores; i++)
    {
        /*****************************************************************/
        /* One frame can be encoded by max of u4_num_cores threads       */
        /* Accumulating the num mbs, sad, qp and intra_mb_cost from      */
        /* u4_num_cores threads                                          */
        /*****************************************************************/
        for (j = 0; j< MAX_MB_TYPE; j++)
        {
            s_frame_info.num_mbs[j] += ps_proc[i].s_frame_info.num_mbs[j];

            s_frame_info.tot_mb_sad[j] += ps_proc[i].s_frame_info.tot_mb_sad[j];

            s_frame_info.qp_sum[j] += ps_proc[i].s_frame_info.qp_sum[j];
        }

        s_frame_info.intra_mb_cost_sum += ps_proc[i].s_frame_info.intra_mb_cost_sum;

        s_frame_info.activity_sum += ps_proc[i].s_frame_info.activity_sum;

        /*****************************************************************/
        /* gather number of residue and header bits consumed by the frame*/
        /*****************************************************************/
        ih264e_update_rc_bits_info(&s_frame_info, &ps_proc[i].s_entropy);
    }

    /* get pic type */
    switch (ps_codec->pic_type)
    {
        case PIC_I:
        case PIC_IDR:
            rc_pic_type = I_PIC;
            break;
        case PIC_P:
            rc_pic_type = P_PIC;
            break;
        case PIC_B:
            rc_pic_type = B_PIC;
            break;
        default:
            assert(0);
            break;
    }

    /* update rc lib with current frame stats */
    i4_stuffing_byte =  ih264e_rc_post_enc(ps_codec->s_rate_control.pps_rate_control_api,
                                          &(s_frame_info),
                                          ps_codec->s_rate_control.pps_pd_frm_rate,
                                          ps_codec->s_rate_control.pps_time_stamp,
                                          ps_codec->s_rate_control.pps_frame_time,
                                          (ps_proc->i4_wd_mbs * ps_proc->i4_ht_mbs),
                                          &rc_pic_type,
                                          i4_is_first_frm,
                                          &ps_codec->s_rate_control.post_encode_skip[ctxt_sel],
                                          u1_frame_qp,
                                          &ps_codec->s_rate_control.num_intra_in_prev_frame,
                                          &ps_codec->s_rate_control.i4_avg_activity);
    return i4_stuffing_byte;
}

/**
*******************************************************************************
*
* @brief
*  entry point of a spawned encoder thread
*
* @par Description:
*  The encoder thread dequeues a proc/entropy job from the encoder queue and
*  calls necessary routines.
*
* @param[in] pv_proc
*  Process context corresponding to the thread
*
* @returns  error status
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_process_thread(void *pv_proc)
{
    /* error status */
    IH264_ERROR_T ret = IH264_SUCCESS;
    WORD32 error_status = IH264_SUCCESS;

    /* proc ctxt */
    process_ctxt_t *ps_proc = pv_proc;

    /* codec ctxt */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* structure to represent a processing job entry */
    job_t s_job;

    /* blocking call : entropy dequeue is non-blocking till all
     * the proc jobs are processed */
    WORD32 is_blocking = 0;

    /* set affinity */
    ithread_set_affinity(ps_proc->i4_id);

    while(1)
    {
        /* dequeue a job from the entropy queue */
        {
            int error = ithread_mutex_lock(ps_codec->pv_entropy_mutex);

            /* codec context selector */
            WORD32 ctxt_sel = ps_codec->i4_encode_api_call_cnt % MAX_CTXT_SETS;

            volatile UWORD32 *pu4_buf = &ps_codec->au4_entropy_thread_active[ctxt_sel];

            /* have the lock */
            if (error == 0)
            {
                if (*pu4_buf == 0)
                {
                    /* no entropy threads are active, try dequeuing a job from the entropy queue */
                    ret = ih264_list_dequeue(ps_proc->pv_entropy_jobq, &s_job, is_blocking);
                    if (IH264_SUCCESS == ret)
                    {
                        *pu4_buf = 1;
                        ithread_mutex_unlock(ps_codec->pv_entropy_mutex);
                        goto WORKER;
                    }
                    else if(is_blocking)
                    {
                        ithread_mutex_unlock(ps_codec->pv_entropy_mutex);
                        break;
                    }
                }
                ithread_mutex_unlock(ps_codec->pv_entropy_mutex);
            }
        }

        /* dequeue a job from the process queue */
        ret = ih264_list_dequeue(ps_proc->pv_proc_jobq, &s_job, 1);
        if (IH264_SUCCESS != ret)
        {
            if(ps_proc->i4_id)
                break;
            else
            {
                is_blocking = 1;
                continue;
            }
        }

WORKER:
        /* choose appropriate proc context based on proc_base_idx */
        ps_proc = &ps_codec->as_process[ps_proc->i4_id + s_job.i2_proc_base_idx];

        switch (s_job.i4_cmd)
        {
            case CMD_PROCESS:
                ps_proc->i4_mb_cnt = s_job.i2_mb_cnt;
                ps_proc->i4_mb_x = s_job.i2_mb_x;
                ps_proc->i4_mb_y = s_job.i2_mb_y;

                /* init process context */
                ih264e_init_proc_ctxt(ps_proc);

                /* core code all mbs enlisted under the current job */
                error_status |= ih264e_process(ps_proc);
                break;

            case CMD_ENTROPY:
                ps_proc->s_entropy.i4_mb_x = s_job.i2_mb_x;
                ps_proc->s_entropy.i4_mb_y = s_job.i2_mb_y;
                ps_proc->s_entropy.i4_mb_cnt = s_job.i2_mb_cnt;

                /* init entropy */
                ih264e_init_entropy_ctxt(ps_proc);

                /* entropy code all mbs enlisted under the current job */
                error_status |= ih264e_entropy(ps_proc);
                break;

            default:
                error_status |= IH264_FAIL;
                break;
        }
    }

    /* send error code */
    ps_proc->i4_error_code = error_status;
    return ret;
}
