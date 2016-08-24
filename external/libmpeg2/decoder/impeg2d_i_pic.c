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

#include "iv_datatypedef.h"
#include "iv.h"

#include "impeg2_buf_mgr.h"
#include "impeg2_disp_mgr.h"
#include "impeg2_defs.h"
#include "impeg2_platform_macros.h"
#include "impeg2_inter_pred.h"
#include "impeg2_idct.h"
#include "impeg2_globals.h"
#include "impeg2_mem_func.h"
#include "impeg2_format_conv.h"
#include "impeg2_macros.h"

#include "ivd.h"
#include "impeg2d.h"
#include "impeg2d_bitstream.h"
#include "impeg2d_structs.h"
#include "impeg2d_vld_tables.h"
#include "impeg2d_vld.h"
#include "impeg2d_pic_proc.h"
#include "impeg2d_debug.h"
#include "impeg2d_globals.h"
#include "impeg2d_mv_dec.h"

/*******************************************************************************
*  Function Name   : impeg2d_dec_i_mb_params
*
*  Description     : Decoding I MB parameters.
*
*  Arguments       :
*  dec             : Decoder state
*  stream          : Bitstream
*
*  Values Returned : None
*******************************************************************************/
void impeg2d_dec_i_mb_params(dec_state_t *ps_dec)
{

    UWORD16 u2_next_bits;
    UWORD16 u2_bits_to_flush;
    stream_t *ps_stream = &ps_dec->s_bit_stream;

    /*-----------------------------------------------------------------------*/
    /* Flush the MBAddrIncr Bit                                              */
    /*                                                                       */
    /* Since we are not supporting scalable modes there won't be skipped     */
    /* macroblocks in I-Picture and the MBAddrIncr will always be 1,         */
    /* The MBAddrIncr can never be greater than 1 for the simple and main    */
    /* profile MPEG2.                                                        */
    /*-----------------------------------------------------------------------*/
    if(impeg2d_bit_stream_nxt(ps_stream,1) == 1) //Making sure the increment is one.
    {
        impeg2d_bit_stream_flush(ps_stream,1);
    }
    else if(ps_dec->u2_first_mb && ps_dec->u2_mb_x)
    {
        WORD32 i4_mb_add_inc = impeg2d_get_mb_addr_incr(ps_stream);

            //VOLParams->FirstInSlice = 0;
            /****************************************************************/
            /* Section 6.3.17                                               */
            /* The first MB of a slice cannot be skipped                    */
            /* But the mb_addr_incr can be > 1, because at the beginning of */
            /* a slice, it indicates the offset from the last MB in the     */
            /* previous row. Hence for the first slice in a row, the        */
            /* mb_addr_incr needs to be 1.                                  */
            /****************************************************************/
            /* MB_x is set to zero whenever MB_y changes.                   */

            ps_dec->u2_mb_x = i4_mb_add_inc - 1;
            ps_dec->u2_mb_x = MIN(ps_dec->u2_mb_x, (ps_dec->u2_num_horiz_mb - 1));
    }

    /*-----------------------------------------------------------------------*/
    /* Decode the macroblock_type, dct_type and quantiser_scale_code         */
    /*                                                                       */
    /* macroblock_type      2 bits [can be either 1 or 01]                   */
    /* dct_type             1 bit                                            */
    /* quantiser_scale_code 5 bits                                           */
    /*-----------------------------------------------------------------------*/
    u2_next_bits = impeg2d_bit_stream_nxt(ps_stream,8);
    if(BIT(u2_next_bits,7) == 1)
    {
        /* read the dct_type if needed */
        u2_bits_to_flush = 1;
        if(ps_dec->u2_read_dct_type)
        {
            u2_bits_to_flush++;
            ps_dec->u2_field_dct = BIT(u2_next_bits,6);
        }
    }
    else
    {
        u2_bits_to_flush = 7;
        /*------------------------------------------------------------------*/
        /* read the dct_type if needed                                      */
        /*------------------------------------------------------------------*/
        if(ps_dec->u2_read_dct_type)
        {
            u2_bits_to_flush++;
            ps_dec->u2_field_dct = BIT(u2_next_bits,5);
        }
        else
        {
            u2_next_bits >>= 1;
        }
        /*------------------------------------------------------------------*/
        /* Quant scale code decoding                                        */
        /*------------------------------------------------------------------*/
        {
            UWORD16 quant_scale_code;
            quant_scale_code = u2_next_bits & 0x1F;

            ps_dec->u1_quant_scale = (ps_dec->u2_q_scale_type) ?
                gau1_impeg2_non_linear_quant_scale[quant_scale_code] :
                (quant_scale_code << 1);
        }
    }
    impeg2d_bit_stream_flush(ps_stream,u2_bits_to_flush);
    /*************************************************************************/
    /* Decoding of motion vectors if concealment motion vectors are present  */
    /*************************************************************************/
    if(ps_dec->u2_concealment_motion_vectors)
    {
        if(ps_dec->u2_picture_structure != FRAME_PICTURE)
            impeg2d_bit_stream_flush(ps_stream,1);
        impeg2d_dec_mv(ps_stream,ps_dec->ai2_pred_mv[FORW][FIRST],ps_dec->ai2_mv[FORW][FIRST],
            ps_dec->au2_f_code[FORW],0,0);

        /* Flush the marker bit */
        if(0 == (impeg2d_bit_stream_get(ps_stream,1)))
        {
            /* Ignore marker bit error */
        }

    }
    ps_dec->u2_first_mb = 0;
    return;
}
/*******************************************************************************
*  Function Name   : impeg2d_dec_i_slice
*
*  Description     : Decodes I slice
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*******************************************************************************/
IMPEG2D_ERROR_CODES_T impeg2d_dec_i_slice(dec_state_t *ps_dec)
{
    WORD16 *pi2_vld_out;
    UWORD32 i;
    yuv_buf_t *ps_cur_frm_buf = &ps_dec->s_cur_frm_buf;

    UWORD32 u4_frame_width = ps_dec->u2_frame_width;
    UWORD32 u4_frm_offset = 0;
    UWORD8  *pu1_out_p;
    IMPEG2D_ERROR_CODES_T e_error = (IMPEG2D_ERROR_CODES_T)IVD_ERROR_NONE;


    pi2_vld_out = ps_dec->ai2_vld_buf;


    if(ps_dec->u2_picture_structure != FRAME_PICTURE)
    {
        u4_frame_width <<= 1;
        if(ps_dec->u2_picture_structure == BOTTOM_FIELD)
        {
            u4_frm_offset = ps_dec->u2_frame_width;
        }
    }

    do
    {
        UWORD32 u4_x_offset,u4_y_offset;
        UWORD32 u4_blk_pos;
        UWORD32 u4_x_dst_offset = 0;
        UWORD32 u4_y_dst_offset = 0;


        IMPEG2D_TRACE_MB_START(ps_dec->u2_mb_x, ps_dec->u2_mb_y);

        impeg2d_dec_i_mb_params(ps_dec);

        u4_x_dst_offset = u4_frm_offset + (ps_dec->u2_mb_x << 4);
        u4_y_dst_offset = (ps_dec->u2_mb_y << 4) * u4_frame_width;
        pu1_out_p = ps_cur_frm_buf->pu1_y + u4_x_dst_offset + u4_y_dst_offset;

        for(i = 0; i < NUM_LUMA_BLKS; ++i)
        {

            e_error = ps_dec->pf_vld_inv_quant(ps_dec, pi2_vld_out,
                                            ps_dec->pu1_inv_scan_matrix, 1, Y_LUMA, 0);
            if ((IMPEG2D_ERROR_CODES_T)IVD_ERROR_NONE != e_error)
            {
                return e_error;
            }

            u4_x_offset = gai2_impeg2_blk_x_off[i];

            if(ps_dec->u2_field_dct == 0)
                u4_y_offset = gai2_impeg2_blk_y_off_frm[i] ;
            else
                u4_y_offset = gai2_impeg2_blk_y_off_fld[i] ;

            u4_blk_pos = u4_y_offset * u4_frame_width + u4_x_offset;
            IMPEG2D_IDCT_INP_STATISTICS(pi2_vld_out, ps_dec->u4_non_zero_cols, ps_dec->u4_non_zero_rows);

            PROFILE_DISABLE_IDCT_IF0
            {
                WORD32 i4_idx;
                i4_idx = 1;
                if(1 == (ps_dec->u4_non_zero_cols | ps_dec->u4_non_zero_rows))
                    i4_idx = 0;

                ps_dec->pf_idct_recon[i4_idx * 2 + ps_dec->i4_last_value_one](pi2_vld_out,
                                                        ps_dec->ai2_idct_stg1,
                                                        (UWORD8 *)gau1_impeg2_zerobuf,
                                                        pu1_out_p + u4_blk_pos,
                                                        8,
                                                        8,
                                                        u4_frame_width << ps_dec->u2_field_dct,
                                                        ~ps_dec->u4_non_zero_cols, ~ps_dec->u4_non_zero_rows);

            }

        }

        /* For U and V blocks, divide the x and y offsets by 2. */
        u4_x_dst_offset >>= 1;
        u4_y_dst_offset >>= 2;

        /* In case of chrominance blocks the DCT will be frame DCT */
        /* i = 0, U component and */

        e_error = ps_dec->pf_vld_inv_quant(ps_dec, pi2_vld_out,
                                        ps_dec->pu1_inv_scan_matrix, 1, U_CHROMA, 0);
        if ((IMPEG2D_ERROR_CODES_T)IVD_ERROR_NONE != e_error)
        {
            return e_error;
        }

        pu1_out_p = ps_cur_frm_buf->pu1_u + u4_x_dst_offset + u4_y_dst_offset;
        IMPEG2D_IDCT_INP_STATISTICS(pi2_vld_out, ps_dec->u4_non_zero_cols, ps_dec->u4_non_zero_rows);
        PROFILE_DISABLE_IDCT_IF0
        {
            WORD32 i4_idx;
            i4_idx = 1;
            if(1 == (ps_dec->u4_non_zero_cols | ps_dec->u4_non_zero_rows))
                i4_idx = 0;

            ps_dec->pf_idct_recon[i4_idx * 2 + ps_dec->i4_last_value_one](pi2_vld_out,
                                                    ps_dec->ai2_idct_stg1,
                                                    (UWORD8 *)gau1_impeg2_zerobuf,
                                                    pu1_out_p,
                                                    8,
                                                    8,
                                                    u4_frame_width >> 1,
                                                    ~ps_dec->u4_non_zero_cols, ~ps_dec->u4_non_zero_rows);

        }
        /* Write the idct_out block to the current frame dec->curFrame*/
        /* In case of field DCT type, write to alternate lines */
        e_error = ps_dec->pf_vld_inv_quant(ps_dec, pi2_vld_out,
                                        ps_dec->pu1_inv_scan_matrix, 1, V_CHROMA, 0);
        if ((IMPEG2D_ERROR_CODES_T)IVD_ERROR_NONE != e_error)
        {
            return e_error;
        }

        pu1_out_p = ps_cur_frm_buf->pu1_v + u4_x_dst_offset + u4_y_dst_offset;
        IMPEG2D_IDCT_INP_STATISTICS(pi2_vld_out, ps_dec->u4_non_zero_cols, ps_dec->u4_non_zero_rows);
        PROFILE_DISABLE_IDCT_IF0
        {
            WORD32 i4_idx;
            i4_idx = 1;
            if(1 == (ps_dec->u4_non_zero_cols | ps_dec->u4_non_zero_rows))
                i4_idx = 0;
            ps_dec->pf_idct_recon[i4_idx * 2 + ps_dec->i4_last_value_one](pi2_vld_out,
                                                    ps_dec->ai2_idct_stg1,
                                                    (UWORD8 *)gau1_impeg2_zerobuf,
                                                    pu1_out_p,
                                                    8,
                                                    8,
                                                    u4_frame_width >> 1,
                                                    ~ps_dec->u4_non_zero_cols, ~ps_dec->u4_non_zero_rows);
        }
        ps_dec->u2_num_mbs_left--;


        ps_dec->u2_mb_x++;

        if(ps_dec->s_bit_stream.u4_offset > ps_dec->s_bit_stream.u4_max_offset)
        {
            return IMPEG2D_BITSTREAM_BUFF_EXCEEDED_ERR;
        }
        else if (ps_dec->u2_mb_x == ps_dec->u2_num_horiz_mb)
        {
            ps_dec->u2_mb_x = 0;
            ps_dec->u2_mb_y++;
        }

    }
    while(ps_dec->u2_num_mbs_left != 0 && impeg2d_bit_stream_nxt(&ps_dec->s_bit_stream,23) != 0x0);
    return e_error;
}
