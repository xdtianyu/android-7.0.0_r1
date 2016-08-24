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
#include <stdio.h>

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
#include "impeg2d_globals.h"
#include "impeg2d_vld_tables.h"
#include "impeg2d_pic_proc.h"
#include "impeg2d_debug.h"
#include "impeg2d_mv_dec.h"
#include "impeg2d_mc.h"

/*******************************************************************************
* Function name : impeg2d_dec_1mv
*
* Description   : Decodes a motion vector and updates the predictors
*
* Arguments     :
* stream        : Bitstream
* predMv        : Prediction for the motion vectors
* mv            : Motion vectors
* fCode         : fcode to the used for the decoding
* shift         : Shift value to be used. This will be equal to
*                 (mv_format == "field") && (picture_structure == "Frame picture")
* i             : 0 - MV_X and 1 - MV_Y
*
* Value Returned: None
*******************************************************************************/
INLINE void impeg2d_dec_1mv(stream_t *ps_stream, WORD16 ai2_pred_mv[], WORD16 ai2_mv[],UWORD16 au2_fCode[],
           UWORD16 u2_mv_y_shift, WORD16 ai2_dmv[])
{
    WORD16  i2_f;
    WORD16  i2_r_size;
    WORD16  i2_high,i2_low,i2_range;
    UWORD32  u4_mv_code;
    WORD16  i2_delta;
    UWORD16 u2_first_bit;
    WORD32 i;
    WORD32 ai2_shifts[2];
    UWORD32 u4_buf;
    UWORD32 u4_buf_nxt;
    UWORD32 u4_offset;
    UWORD32 *pu4_buf_aligned;

    ai2_shifts[0] = 0;
    ai2_shifts[1] = u2_mv_y_shift;


    GET_TEMP_STREAM_DATA(u4_buf,u4_buf_nxt,u4_offset,pu4_buf_aligned,ps_stream)
    for(i = 0; i < 2; i++)
    {
        WORD32 i4_shift = ai2_shifts[i];
        /* Decode the motion_code */
        IBITS_NXT(u4_buf, u4_buf_nxt, u4_offset, u4_mv_code, MV_CODE_LEN)
        u2_first_bit    = (u4_mv_code >> (MV_CODE_LEN - 1)) & 0x01;
        if(u2_first_bit == 1) /* mvCode == 0 */
        {
            i2_delta = 0;
            FLUSH_BITS(u4_offset,u4_buf,u4_buf_nxt,1,pu4_buf_aligned)

            ai2_mv[i] = (ai2_pred_mv[i] >> i4_shift);

            ai2_pred_mv[i] = (ai2_mv[i] << i4_shift);

        }
        else
        {
            UWORD16 u2_index;
            UWORD16 u2_value;
            UWORD16 u2_mv_len;
            UWORD16 u2_abs_mvcode_minus1;
            UWORD16 u2_sign_bit;

            i2_r_size   = au2_fCode[i] - 1;
            i2_f       = 1 << i2_r_size;
            i2_high    = (16 * i2_f) - 1;
            i2_low     = ((-16) * i2_f);
            i2_range   = (32 * i2_f);

            u2_index               = (u4_mv_code >> 1) & 0x1FF;
            u2_value               = gau2_impeg2d_mv_code[u2_index];
            u2_mv_len               = (u2_value & 0x0F);
            u2_abs_mvcode_minus1   = (u2_value >> 8) & 0x0FF;
            u4_mv_code            >>= (MV_CODE_LEN - u2_mv_len - 1);
            u2_sign_bit             = u4_mv_code & 0x1;

            FLUSH_BITS(u4_offset,u4_buf,u4_buf_nxt,(u2_mv_len + 1),pu4_buf_aligned)
            i2_delta = u2_abs_mvcode_minus1 * i2_f + 1;
            if(i2_r_size)
            {
                UWORD32 val;
                IBITS_GET(u4_buf, u4_buf_nxt, u4_offset, val, pu4_buf_aligned, i2_r_size)
                i2_delta += val;
            }

            if(u2_sign_bit)
                i2_delta = -i2_delta;

            ai2_mv[i] = (ai2_pred_mv[i] >> i4_shift) + i2_delta;

            if(ai2_mv[i] < i2_low)
            {
                ai2_mv[i] += i2_range;
            }

            if(ai2_mv[i] > i2_high)
            {
                ai2_mv[i] -= i2_range;
            }
            ai2_pred_mv[i] = (ai2_mv[i] << i4_shift);

        }
        if(ai2_dmv)
        {
            UWORD32 u4_val;
            ai2_dmv[i] = 0;
            IBITS_GET(u4_buf, u4_buf_nxt, u4_offset, u4_val, pu4_buf_aligned, 1)
            if(u4_val)
            {
                IBITS_GET(u4_buf, u4_buf_nxt, u4_offset, u4_val, pu4_buf_aligned, 1)
                ai2_dmv[i] = gai2_impeg2d_dec_mv[u4_val];
            }
        }
    }
    PUT_TEMP_STREAM_DATA(u4_buf, u4_buf_nxt, u4_offset, pu4_buf_aligned, ps_stream)

}
/*******************************************************************************
* Function name : impeg2d_dec_mv
*
* Description   : Decodes a motion vector and updates the predictors
*
* Arguments     :
* stream        : Bitstream
* predMv        : Prediction for the motion vectors
* mv            : Motion vectors
* fCode         : fcode to the used for the decoding
* shift         : Shift value to be used. This will be equal to
*                 (mv_format == "field") && (picture_structure == "Frame picture")
*
* Value Returned: None
*******************************************************************************/
e_field_t impeg2d_dec_mv(stream_t *ps_stream, WORD16 ai2_pred_mv[], WORD16 ai2_mv[],UWORD16 au2_f_code[],
           UWORD16 u2_shift, UWORD16 u2_fld_sel)
{
    e_field_t e_fld;
    if(u2_fld_sel)
    {
        e_fld = (e_field_t)impeg2d_bit_stream_get_bit(ps_stream);
    }
    else
    {
        e_fld = TOP;
    }

    impeg2d_dec_1mv(ps_stream,ai2_pred_mv,ai2_mv,au2_f_code,u2_shift,NULL);

    return(e_fld);
}

/*****************************************************************************
*  Function Name   : impeg2d_dec_1mv_mb
*
*  Description     : Decodes mc params for 1 MV  MB
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*****************************************************************************/
void impeg2d_dec_1mv_mb(dec_state_t *ps_dec)
{
    stream_t         *ps_stream;
    WORD16          *pi2_mv;
    e_field_t         e_fld;
    mb_mc_params_t  *ps_mc;
    e_pred_direction_t   e_ref_pic;


    ps_stream  = &ps_dec->s_bit_stream;
    e_ref_pic = ps_dec->e_mb_pred;
    /************************************************************************/
    /* Decode the motion vector                                             */
    /************************************************************************/
    pi2_mv        = (WORD16 *)&ps_dec->ai2_mv[FORW][FIRST];
    e_fld = impeg2d_dec_mv(ps_stream,ps_dec->ai2_pred_mv[e_ref_pic][FIRST],pi2_mv,
                ps_dec->au2_f_code[e_ref_pic],0, ps_dec->u2_fld_pic);

    ps_dec->ai2_pred_mv[e_ref_pic][SECOND][MV_X] = ps_dec->ai2_pred_mv[e_ref_pic][FIRST][MV_X];
    ps_dec->ai2_pred_mv[e_ref_pic][SECOND][MV_Y] = ps_dec->ai2_pred_mv[e_ref_pic][FIRST][MV_Y];
    /************************************************************************/
    /* Set the motion vector params                                         */
    /************************************************************************/
    ps_mc = &ps_dec->as_mb_mc_params[e_ref_pic][FIRST];
    ps_mc->s_ref = ps_dec->as_ref_buf[e_ref_pic][e_fld];
    impeg2d_set_mc_params(&ps_mc->s_luma, &ps_mc->s_chroma, ps_dec->s_mb_type, 0,
                  pi2_mv, ps_dec->u2_mb_x, ps_dec->u2_mb_y, ps_dec->u2_frame_width, ps_dec->u2_frame_height,ps_dec->u2_picture_width);

}

/*****************************************************************************
*  Function Name   : impeg2d_dec_2mv_fw_or_bk_mb
*
*  Description     : Decodes first part of params for 2 MV Interpolated MB
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*****************************************************************************/
void impeg2d_dec_2mv_fw_or_bk_mb(dec_state_t *ps_dec)
{
    stream_t         *ps_stream;
    WORD16          *pi2_mv;
    e_field_t         e_fld;
    mb_mc_params_t  *ps_mc;
    e_pred_direction_t   e_ref_pic;
    UWORD16 i;

    ps_stream  = &ps_dec->s_bit_stream;
    e_ref_pic = ps_dec->e_mb_pred;
    for(i = 0; i < 2; i++)
    {
        /********************************************************************/
        /* Decode the first motion vector                                   */
        /********************************************************************/
        pi2_mv        = (WORD16 *)&ps_dec->ai2_mv[FORW][i];
        e_fld = impeg2d_dec_mv(ps_stream,ps_dec->ai2_pred_mv[e_ref_pic][i],pi2_mv,
                    ps_dec->au2_f_code[e_ref_pic],ps_dec->u2_frm_pic, 1);

        /********************************************************************/
        /* Set the motion vector params                                     */
        /********************************************************************/
        ps_mc = &ps_dec->as_mb_mc_params[FORW][i];
        ps_mc->s_ref = ps_dec->as_ref_buf[e_ref_pic][e_fld];
        impeg2d_set_mc_params(&ps_mc->s_luma, &ps_mc->s_chroma, ps_dec->s_mb_type, i,
                      pi2_mv, ps_dec->u2_mb_x, ps_dec->u2_mb_y, ps_dec->u2_frame_width, ps_dec->u2_frame_height,ps_dec->u2_picture_width);
    }
}

/*****************************************************************************
*  Function Name   : impeg2d_dec_frm_dual_prime
*
*  Description     : Decodes first part of params for 2 MV Interpolated MB
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*****************************************************************************/
void impeg2d_dec_frm_dual_prime(dec_state_t *ps_dec)
{
    stream_t         *ps_stream;
    WORD16          *pi2_mv;
    mb_mc_params_t  *ps_mc;

    WORD16      ai2_dmv[2];
    WORD16      *pi2_mv1, *pi2_mv2, *pi2_mv3, *pi2_mv4;
    UWORD16 i,j;

    pi2_mv1     = (WORD16 *)&(ps_dec->ai2_mv[FORW][FIRST]);
    pi2_mv2     = (WORD16 *)&(ps_dec->ai2_mv[FORW][SECOND]);
    pi2_mv3     = (WORD16 *)&(ps_dec->ai2_mv[BACK][FIRST]);
    pi2_mv4     = (WORD16 *)&(ps_dec->ai2_mv[BACK][SECOND]);



    ps_stream  = &ps_dec->s_bit_stream;

    /************************************************************************/
    /* Decode the motion vector MV_X, MV_Y and dmv[0], dmv[1]               */
    /************************************************************************/
    impeg2d_dec_1mv(ps_stream,ps_dec->ai2_pred_mv[FORW][FIRST],pi2_mv1,ps_dec->au2_f_code[FORW],ps_dec->u2_frm_pic,ai2_dmv);

    {
        WORD16 ai2_m[2][2];

        if(ps_dec->u2_top_field_first)
        {
            ai2_m[1][0] = 1;
            ai2_m[0][1] = 3;
        }
        else
        {
            ai2_m[1][0] = 3;
            ai2_m[0][1] = 1;
        }

        pi2_mv2[MV_X] = pi2_mv1[MV_X];
        pi2_mv2[MV_Y] = pi2_mv1[MV_Y];

        pi2_mv3[MV_X] = ai2_dmv[0] + DIV_2_RND(pi2_mv1[MV_X] * ai2_m[1][0]);
        pi2_mv4[MV_X] = ai2_dmv[0] + DIV_2_RND(pi2_mv1[MV_X] * ai2_m[0][1]);

        pi2_mv3[MV_Y] = ai2_dmv[1] + DIV_2_RND(pi2_mv1[MV_Y] * ai2_m[1][0]) - 1;
        pi2_mv4[MV_Y] = ai2_dmv[1] + DIV_2_RND(pi2_mv1[MV_Y] * ai2_m[0][1]) + 1;
    }

    ps_dec->ai2_pred_mv[FORW][SECOND][MV_X] = ps_dec->ai2_pred_mv[FORW][FIRST][MV_X];
    ps_dec->ai2_pred_mv[FORW][SECOND][MV_Y] = ps_dec->ai2_pred_mv[FORW][FIRST][MV_Y];

    /************************************************************************/
    /* Set the motion vector params                                         */
    /************************************************************************/
    for(j = 0; j < 2; j++)
    {
        for(i = 0; i < 2; i++)
        {
            pi2_mv        = (WORD16 *)&ps_dec->ai2_mv[j][i];
            ps_mc = &ps_dec->as_mb_mc_params[j][i];
            ps_mc->s_ref = ps_dec->as_ref_buf[FORW][(i ^ j) & 1];
            impeg2d_set_mc_params(&ps_mc->s_luma, &ps_mc->s_chroma, ps_dec->s_mb_type, i,
                      pi2_mv, ps_dec->u2_mb_x, ps_dec->u2_mb_y, ps_dec->u2_frame_width, ps_dec->u2_frame_height,ps_dec->u2_picture_width);
        }
    }

}
/*****************************************************************************
*  Function Name   : impeg2d_dec_fld_dual_prime
*
*  Description     : Decodes first part of params for 2 MV Interpolated MB
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*****************************************************************************/
void impeg2d_dec_fld_dual_prime(dec_state_t *ps_dec)
{
    stream_t         *ps_stream;
    WORD16          *pi2_mv;
    mb_mc_params_t  *ps_mc;

    WORD16      *pi2_mv1, *pi2_mv2;
    WORD16      ai2_dmv[2];


    pi2_mv1     = (WORD16 *)&(ps_dec->ai2_mv[FORW][FIRST]);
    pi2_mv2     = (WORD16 *)&(ps_dec->ai2_mv[FORW][SECOND]);
    ps_stream  = &ps_dec->s_bit_stream;

    /************************************************************************/
    /* Decode the motion vector MV_X, MV_Y and dmv[0], dmv[1]               */
    /************************************************************************/
    impeg2d_dec_1mv(ps_stream,ps_dec->ai2_pred_mv[FORW][FIRST],pi2_mv1,ps_dec->au2_f_code[FORW],0,ai2_dmv);


    pi2_mv2[MV_X] = ai2_dmv[0] + DIV_2_RND(pi2_mv1[MV_X]);
    pi2_mv2[MV_Y] = ai2_dmv[1] + DIV_2_RND(pi2_mv1[MV_Y]);

    if(ps_dec->u2_picture_structure == TOP_FIELD)
        pi2_mv2[MV_Y] -= 1;
    else
        pi2_mv2[MV_Y] += 1;

    ps_dec->ai2_pred_mv[FORW][SECOND][MV_X] = ps_dec->ai2_pred_mv[FORW][FIRST][MV_X];
    ps_dec->ai2_pred_mv[FORW][SECOND][MV_Y] = ps_dec->ai2_pred_mv[FORW][FIRST][MV_Y];

    /************************************************************************/
    /* Set the motion vector params                                         */
    /************************************************************************/
        pi2_mv        = (WORD16 *)&ps_dec->ai2_mv[FORW][0];
        ps_mc = &ps_dec->as_mb_mc_params[FORW][0];
        ps_mc->s_ref = ps_dec->as_ref_buf[FORW][ps_dec->u2_fld_parity];
        impeg2d_set_mc_params(&ps_mc->s_luma, &ps_mc->s_chroma, ps_dec->s_mb_type, 0,
                  pi2_mv, ps_dec->u2_mb_x, ps_dec->u2_mb_y, ps_dec->u2_frame_width, ps_dec->u2_frame_height,ps_dec->u2_picture_width);

        pi2_mv        = (WORD16 *)&ps_dec->ai2_mv[FORW][1];
        ps_mc = &ps_dec->as_mb_mc_params[FORW][1];
        ps_mc->s_ref = ps_dec->as_ref_buf[FORW][!ps_dec->u2_fld_parity];
        impeg2d_set_mc_params(&ps_mc->s_luma, &ps_mc->s_chroma, ps_dec->s_mb_type, 0,
                  pi2_mv, ps_dec->u2_mb_x, ps_dec->u2_mb_y, ps_dec->u2_frame_width, ps_dec->u2_frame_height,ps_dec->u2_picture_width);


}
/*****************************************************************************
*  Function Name   : impeg2d_dec_4mv_mb
*
*  Description     : Decodes first part of params for 2 MV Interpolated MB
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*****************************************************************************/
void impeg2d_dec_4mv_mb(dec_state_t *ps_dec)
{
    stream_t         *ps_stream;
    WORD16          *pi2_mv;
    e_field_t         e_fld;
    mb_mc_params_t  *ps_mc;

    UWORD16 i,j;

    ps_stream  = &ps_dec->s_bit_stream;

    /***********************************************/
    /* loop for FW & BK                            */
    /***********************************************/
    for(j = 0; j < 2; j++)
    {
        /***********************************************/
        /* loop for decoding 2 mvs of same reference frame*/
        /***********************************************/
        for(i = 0; i < 2; i++)
        {
            /****************************************************************/
            /* Decode the first motion vector                               */
            /****************************************************************/
            pi2_mv        = (WORD16 *)&ps_dec->ai2_mv[j][i];
            e_fld = impeg2d_dec_mv(ps_stream,ps_dec->ai2_pred_mv[j][i],pi2_mv,
                        ps_dec->au2_f_code[j],ps_dec->u2_frm_pic, 1);

            /****************************************************************/
            /* Set the motion vector params                                 */
            /****************************************************************/
            ps_mc = &ps_dec->as_mb_mc_params[j][i];
            ps_mc->s_ref = ps_dec->as_ref_buf[j][e_fld];
            impeg2d_set_mc_params(&ps_mc->s_luma, &ps_mc->s_chroma, ps_dec->s_mb_type, i,
                          pi2_mv, ps_dec->u2_mb_x, ps_dec->u2_mb_y, ps_dec->u2_frame_width, ps_dec->u2_frame_height,ps_dec->u2_picture_width);
        }
    }

}
/*******************************************************************************
*  Function Name   : impeg2d_dec_2mv_interp_mb
*
*  Description     : Decodes first part of params for 2 MV Interpolated MB
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*******************************************************************************/
void impeg2d_dec_2mv_interp_mb(dec_state_t *ps_dec)
{
    stream_t         *ps_stream;
    WORD16          *pi2_mv;
    e_field_t         e_fld;
    mb_mc_params_t  *ps_mc;
    UWORD16 i;

    ps_stream  = &ps_dec->s_bit_stream;

    for(i = 0; i < 2; i++)
    {
        /********************************************************************/
        /* Decode the first motion vector                                   */
        /********************************************************************/
        pi2_mv        = (WORD16 *)&ps_dec->ai2_mv[i][FIRST];
        e_fld = impeg2d_dec_mv(ps_stream,ps_dec->ai2_pred_mv[i][FIRST],pi2_mv,
                    ps_dec->au2_f_code[i],0, ps_dec->u2_fld_pic);

        ps_dec->ai2_pred_mv[i][SECOND][MV_X] = ps_dec->ai2_pred_mv[i][FIRST][MV_X];
        ps_dec->ai2_pred_mv[i][SECOND][MV_Y] = ps_dec->ai2_pred_mv[i][FIRST][MV_Y];
        /********************************************************************/
        /* Set the motion vector params                                     */
        /********************************************************************/
        ps_mc = &ps_dec->as_mb_mc_params[i][FIRST];
        ps_mc->s_ref = ps_dec->as_ref_buf[i][e_fld];
        impeg2d_set_mc_params(&ps_mc->s_luma, &ps_mc->s_chroma, ps_dec->s_mb_type,i,
                      pi2_mv, ps_dec->u2_mb_x, ps_dec->u2_mb_y, ps_dec->u2_frame_width, ps_dec->u2_frame_height,ps_dec->u2_picture_width);
    }

}
