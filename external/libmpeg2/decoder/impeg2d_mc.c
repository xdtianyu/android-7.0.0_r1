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
*  impeg2d_mc.c
*
* @brief
*  Contains MC function definitions for MPEG2 decoder
*
* @author
*  Harish
*
* @par List of Functions:
* - impeg2d_motion_comp()
* - impeg2d_motion_comp_recon_buf()
* - impeg2d_mc_1mv()
* - impeg2d_mc_fw_or_bk_mb()
* - impeg2d_mc_frm_dual_prime()
* - impeg2d_mc_fld_dual_prime()
* - impeg2d_mc_4mv()
* - impeg2d_mc_2mv()
* - impeg2d_dec_intra_mb()
* - impeg2d_dec_skip_p_mb()
* - impeg2d_dec_skip_b_mb()
* - impeg2d_dec_skip_mbs()
* - impeg2d_dec_0mv_coded_mb()
* - impeg2d_mc_halfx_halfy()
* - impeg2d_mc_halfx_fully()
* - impeg2d_mc_fullx_halfy()
* - impeg2d_mc_fullx_fully()
* - impeg2d_set_mc_params()
*
* @remarks
*  None
*
*******************************************************************************
*/
#include <string.h>

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
#include "impeg2d_pic_proc.h"
#include "impeg2d_debug.h"
#include "impeg2d_mv_dec.h"
#include "impeg2d_mc.h"

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_motion_comp                                      */
/*                                                                           */
/*  Description   : Perform motion compensation and store the resulting block*/
/*                  in the buf                                               */
/*                                                                           */
/*  Inputs        : params - Parameters required to do motion compensation   */
/*                                                                           */
/*  Globals       :                                                          */
/*                                                                           */
/*  Processing    : Calls appropriate functions depending on the mode of     */
/*                  compensation                                             */
/*                                                                           */
/*  Outputs       : buf       - Buffer for the motion compensation result    */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         14 09 2005   Hairsh M        First Version                        */
/*                                                                           */
/*****************************************************************************/
void impeg2d_motion_comp(dec_state_t *ps_dec, mb_mc_params_t *ps_params,yuv_buf_t *ps_buf)
{

    PROFILE_DISABLE_MC_RETURN;

    /* Perform motion compensation for Y */
    ps_dec->pf_mc[ps_params->s_luma.u4_mode]((void *)ps_dec, ps_params->s_ref.pu1_y + ps_params->s_luma.u4_src_offset,
                ps_params->s_luma.u4_src_wd,
                ps_buf->pu1_y + ps_params->s_luma.u4_dst_offset_res_buf,
                ps_params->s_luma.u4_dst_wd_res_buf,
                ps_params->s_luma.u4_cols,
                ps_params->s_luma.u4_rows);
    /* Perform motion compensation for U */
    ps_dec->pf_mc[ps_params->s_chroma.u4_mode]((void *)ps_dec, ps_params->s_ref.pu1_u + ps_params->s_chroma.u4_src_offset,
                ps_params->s_chroma.u4_src_wd,
                ps_buf->pu1_u + ps_params->s_chroma.u4_dst_offset_res_buf,
                ps_params->s_chroma.u4_dst_wd_res_buf,
                ps_params->s_chroma.u4_cols,
                ps_params->s_chroma.u4_rows);

    /* Perform motion compensation for V */
    ps_dec->pf_mc[ps_params->s_chroma.u4_mode]((void *)ps_dec, ps_params->s_ref.pu1_v + ps_params->s_chroma.u4_src_offset,
                ps_params->s_chroma.u4_src_wd,
                ps_buf->pu1_v + ps_params->s_chroma.u4_dst_offset_res_buf,
                ps_params->s_chroma.u4_dst_wd_res_buf,
                ps_params->s_chroma.u4_cols,
                ps_params->s_chroma.u4_rows);
}



/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_motion_comp_recon_buf                          */
/*                                                                           */
/*  Description   : Perform motion compensation and store the resulting block*/
/*                  in the buf                                               */
/*                                                                           */
/*  Inputs        : params - Parameters required to do motion compensation   */
/*                                                                           */
/*  Globals       :                                                          */
/*                                                                           */
/*  Processing    : Calls appropriate functions depending on the mode of     */
/*                  compensation                                             */
/*                                                                           */
/*  Outputs       : buf       - Buffer for the motion compensation result    */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         14 09 2005   Harish M        First Version                        */
/*                                                                           */
/*****************************************************************************/
void impeg2d_motion_comp_recon_buf(dec_state_t *ps_dec,
                                     mb_mc_params_t *ps_params,
                                     yuv_buf_t *ps_dest_buf)
{

    PROFILE_DISABLE_MC_RETURN;

    /* Perform motion compensation for Y */
    ps_dec->pf_mc[ps_params->s_luma.u4_mode](ps_dec, ps_params->s_ref.pu1_y + ps_params->s_luma.u4_src_offset,
                                        ps_params->s_luma.u4_src_wd,
                                        ps_dest_buf->pu1_y + ps_params->s_luma.u4_dst_offset_cur_frm,
                                        ps_params->s_luma.u4_dst_wd_cur_frm,
                                        ps_params->s_luma.u4_cols,
                                        ps_params->s_luma.u4_rows);

    /* Perform motion compensation for U */

    ps_dec->pf_mc[ps_params->s_chroma.u4_mode](ps_dec, ps_params->s_ref.pu1_u + ps_params->s_chroma.u4_src_offset,
                                        ps_params->s_chroma.u4_src_wd,
                                        ps_dest_buf->pu1_u + ps_params->s_chroma.u4_dst_offset_cur_frm,
                                        ps_params->s_chroma.u4_dst_wd_cur_frm,
                                        ps_params->s_chroma.u4_cols,
                                        ps_params->s_chroma.u4_rows);

    /* Perform motion compensation for V */
    ps_dec->pf_mc[ps_params->s_chroma.u4_mode](ps_dec, ps_params->s_ref.pu1_v + ps_params->s_chroma.u4_src_offset,
                                        ps_params->s_chroma.u4_src_wd,
                                        ps_dest_buf->pu1_v + ps_params->s_chroma.u4_dst_offset_cur_frm,
                                        ps_params->s_chroma.u4_dst_wd_cur_frm,
                                        ps_params->s_chroma.u4_cols,
                                        ps_params->s_chroma.u4_rows);
}



/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_mc_1mv                                           */
/*                                                                           */
/*  Description   : Perform motion compensation and store the resulting block*/
/*                  in the buf                                               */
/*                                                                           */
/*  Inputs        : params - Parameters required to do motion compensation   */
/*                                                                           */
/*  Globals       :                                                          */
/*                                                                           */
/*  Processing    : Calls appropriate functions depending on the mode of     */
/*                  compensation                                             */
/*                                                                           */
/*  Outputs       : buf       - Buffer for the motion compensation result    */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         14 09 2005   Hairsh M        First Version                        */
/*                                                                           */
/*****************************************************************************/
void impeg2d_mc_1mv(dec_state_t *ps_dec)
{

    impeg2d_motion_comp_recon_buf(ps_dec, &ps_dec->as_mb_mc_params[ps_dec->e_mb_pred][FIRST], &ps_dec->s_dest_buf);
}



/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_mc_fw_or_bk_mb                                   */
/*                                                                           */
/*  Description   : Perform motion compensation and store the resulting block*/
/*                  in the buf                                               */
/*                                                                           */
/*  Inputs        : params - Parameters required to do motion compensation   */
/*                                                                           */
/*  Globals       :                                                          */
/*                                                                           */
/*  Processing    : Calls appropriate functions depending on the mode of     */
/*                  compensation                                             */
/*                                                                           */
/*  Outputs       : buf       - Buffer for the motion compensation result    */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         14 09 2005   Hairsh M        First Version                        */
/*                                                                           */
/*****************************************************************************/
void impeg2d_mc_fw_or_bk_mb(dec_state_t *ps_dec)
{
    impeg2d_motion_comp_recon_buf(ps_dec, &ps_dec->as_mb_mc_params[FORW][FIRST], &ps_dec->s_dest_buf);
    impeg2d_motion_comp_recon_buf(ps_dec, &ps_dec->as_mb_mc_params[FORW][SECOND], &ps_dec->s_dest_buf);
}



/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_mc_frm_dual_prime                                */
/*                                                                           */
/*  Description   : Perform motion compensation and store the resulting block*/
/*                  in the buf                                               */
/*                                                                           */
/*  Inputs        : params - Parameters required to do motion compensation   */
/*                                                                           */
/*  Globals       :                                                          */
/*                                                                           */
/*  Processing    : Calls appropriate functions depending on the mode of     */
/*                  compensation                                             */
/*                                                                           */
/*  Outputs       : buf       - Buffer for the motion compensation result    */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         14 09 2005   Hairsh M        First Version                        */
/*                                                                           */
/*****************************************************************************/
void impeg2d_mc_frm_dual_prime(dec_state_t *ps_dec)
{
    /************************************************************************/
    /* Perform Motion Compensation                                          */
    /************************************************************************/
    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[FORW][FIRST], &ps_dec->s_mc_fw_buf);
    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[BACK][FIRST], &ps_dec->s_mc_bk_buf);

    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[FORW][SECOND], &ps_dec->s_mc_fw_buf);
    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[BACK][SECOND], &ps_dec->s_mc_bk_buf);



    ps_dec->pf_interpolate(&ps_dec->s_mc_fw_buf,&ps_dec->s_mc_bk_buf,&ps_dec->s_dest_buf,ps_dec->u2_picture_width);
}



/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_mc_fld_dual_prime                                */
/*                                                                           */
/*  Description   : Perform motion compensation and store the resulting block*/
/*                  in the buf                                               */
/*                                                                           */
/*  Inputs        : params - Parameters required to do motion compensation   */
/*                                                                           */
/*  Globals       :                                                          */
/*                                                                           */
/*  Processing    : Calls appropriate functions depending on the mode of     */
/*                  compensation                                             */
/*                                                                           */
/*  Outputs       : buf       - Buffer for the motion compensation result    */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         14 09 2005   Hairsh M        First Version                        */
/*                                                                           */
/*****************************************************************************/
void impeg2d_mc_fld_dual_prime(dec_state_t *ps_dec)
{
    /************************************************************************/
    /* Perform Motion Compensation                                          */
    /************************************************************************/
    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[FORW][FIRST], &ps_dec->s_mc_fw_buf);
    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[FORW][SECOND], &ps_dec->s_mc_bk_buf);


    ps_dec->pf_interpolate(&ps_dec->s_mc_fw_buf,&ps_dec->s_mc_bk_buf,&ps_dec->s_dest_buf,ps_dec->u2_picture_width);
}





/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_mc_4mv                                      */
/*                                                                           */
/*  Description   : Perform motion compensation and store the resulting block*/
/*                  in the buf                                               */
/*                                                                           */
/*  Inputs        : params - Parameters required to do motion compensation   */
/*                                                                           */
/*  Globals       :                                                          */
/*                                                                           */
/*  Processing    : Calls appropriate functions depending on the mode of     */
/*                  compensation                                             */
/*                                                                           */
/*  Outputs       : buf       - Buffer for the motion compensation result    */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         14 09 2005   Hairsh M        First Version                        */
/*                                                                           */
/*****************************************************************************/
void impeg2d_mc_4mv(dec_state_t *ps_dec)
{
    /************************************************************************/
    /* Perform Motion Compensation                                          */
    /************************************************************************/
    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[FORW][FIRST], &ps_dec->s_mc_fw_buf);
    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[BACK][FIRST], &ps_dec->s_mc_bk_buf);
    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[FORW][SECOND], &ps_dec->s_mc_fw_buf);
    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[BACK][SECOND], &ps_dec->s_mc_bk_buf);

    ps_dec->pf_interpolate(&ps_dec->s_mc_fw_buf,&ps_dec->s_mc_bk_buf,&ps_dec->s_dest_buf,ps_dec->u2_picture_width);
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_mc_2mv                                         */
/*                                                                           */
/*  Description   : Perform motion compensation and store the resulting block*/
/*                  in the buf                                               */
/*                                                                           */
/*  Inputs        : params - Parameters required to do motion compensation   */
/*                                                                           */
/*  Globals       :                                                          */
/*                                                                           */
/*  Processing    : Calls appropriate functions depending on the mode of     */
/*                  compensation                                             */
/*                                                                           */
/*  Outputs       : buf       - Buffer for the motion compensation result    */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         14 09 2005   Hairsh M        First Version                        */
/*                                                                           */
/*****************************************************************************/
void impeg2d_mc_2mv(dec_state_t *ps_dec)
{
   /************************************************************************/
    /* Perform Motion Compensation                                          */
    /************************************************************************/
    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[FORW][FIRST], &ps_dec->s_mc_fw_buf);
    impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[BACK][FIRST], &ps_dec->s_mc_bk_buf);

    ps_dec->pf_interpolate(&ps_dec->s_mc_fw_buf,&ps_dec->s_mc_bk_buf,&ps_dec->s_dest_buf,ps_dec->u2_picture_width);
}

/*****************************************************************************
*  Function Name   : impeg2d_dec_intra_mb
*
*  Description     : Performs decoding of Intra MB
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*****************************************************************************/
void impeg2d_dec_intra_mb(dec_state_t *ps_dec)
{

    ps_dec->u2_cbp = 0x3F;
    if(ps_dec->u2_concealment_motion_vectors)
    {

        stream_t *ps_stream;

        ps_stream = &ps_dec->s_bit_stream;
        /* Decode the concealment motion vector */
        impeg2d_dec_mv(ps_stream,ps_dec->ai2_pred_mv[FORW][FIRST],ps_dec->ai2_mv[FORW][FIRST],
        ps_dec->au2_f_code[FORW],0,ps_dec->u2_fld_pic);


        /* Set the second motion vector predictor */
        ps_dec->ai2_pred_mv[FORW][SECOND][MV_X] = ps_dec->ai2_pred_mv[FORW][FIRST][MV_X];
        ps_dec->ai2_pred_mv[FORW][SECOND][MV_Y] = ps_dec->ai2_pred_mv[FORW][FIRST][MV_Y];

        /* Flush the marker bit */
        if(0 == (impeg2d_bit_stream_get(ps_stream,1)))
        {
            /* Ignore marker bit error */
        }
    }
    else
    {
        /* Reset the motion vector predictors */
        memset(ps_dec->ai2_pred_mv,0,sizeof(ps_dec->ai2_pred_mv));
    }
}

/*****************************************************************************
*  Function Name   : impeg2d_dec_skip_p_mb
*
*  Description     : Performs decoding needed for Skipped MB encountered in
*                    P Pictures and B Pictures with previous MB not bi-predicted
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*****************************************************************************/
void impeg2d_dec_skip_p_mb(dec_state_t *ps_dec, WORD32 u4_num_of_mbs)
{
    WORD16  *pi2_mv;

    e_mb_type_t e_mb_type;
    mb_mc_params_t *ps_mc;


    WORD32 i4_iter;
    UWORD32 u4_dst_wd;
    UWORD32  u4_dst_offset_x;
    UWORD32  u4_dst_offset_y;
    UWORD32 u4_frm_offset = 0;
    yuv_buf_t s_dst;

    u4_dst_wd = ps_dec->u2_frame_width;

    if(ps_dec->u2_picture_structure != FRAME_PICTURE)
    {
        u4_dst_wd <<= 1;
        if(ps_dec->u2_picture_structure == BOTTOM_FIELD)
        {
            u4_frm_offset = ps_dec->u2_frame_width;
        }
    }

    for (i4_iter = u4_num_of_mbs; i4_iter > 0; i4_iter--)
    {
        if(ps_dec->u2_picture_structure == FRAME_PICTURE)
        {
            e_mb_type = MC_FRM_FW_AND_BK_2MV;
        }
        else
        {
            e_mb_type = MC_FLD_FW_AND_BK_2MV;
        }

        ps_dec->u2_prev_intra_mb = 0;
        pi2_mv               = (WORD16 *)&(ps_dec->ai2_mv[FORW][FIRST]);

        /* Reset the motion vector predictors */
        if(ps_dec->e_pic_type == P_PIC)
        {
            memset(ps_dec->ai2_pred_mv,0,sizeof(ps_dec->ai2_pred_mv));
            pi2_mv[MV_X]    = pi2_mv[MV_Y] = 0;

            ps_dec->u2_cbp     = 0;

            pi2_mv           = (WORD16 *)&ps_dec->ai2_mv[FORW][FIRST];
            ps_mc           = &ps_dec->as_mb_mc_params[FORW][FIRST];
            ps_mc->s_ref      = ps_dec->as_ref_buf[ps_dec->e_mb_pred][ps_dec->u2_fld_parity];

            impeg2d_set_mc_params(&ps_mc->s_luma, &ps_mc->s_chroma, e_mb_type, 0,
                      pi2_mv, ps_dec->u2_mb_x, ps_dec->u2_mb_y, ps_dec->u2_frame_width, ps_dec->u2_frame_height,ps_dec->u2_picture_width);


            u4_dst_offset_x = (ps_dec->u2_mb_x << 4) + u4_frm_offset;
            u4_dst_offset_y = (ps_dec->u2_mb_y << 4) * u4_dst_wd;

            s_dst.pu1_y = ps_dec->s_cur_frm_buf.pu1_y + u4_dst_offset_x + u4_dst_offset_y;

            u4_dst_offset_x = u4_dst_offset_x >> 1;
            u4_dst_offset_y = u4_dst_offset_y >> 2;

            s_dst.pu1_u = ps_dec->s_cur_frm_buf.pu1_u + u4_dst_offset_x + u4_dst_offset_y;
            s_dst.pu1_v = ps_dec->s_cur_frm_buf.pu1_v + u4_dst_offset_x + u4_dst_offset_y;


            ps_mc->s_ref.pu1_y += ps_mc->s_luma.u4_src_offset;
            ps_mc->s_ref.pu1_u += ps_mc->s_chroma.u4_src_offset;
            ps_mc->s_ref.pu1_v += ps_mc->s_chroma.u4_src_offset;

            ps_dec->pf_copy_mb(&ps_mc->s_ref, &s_dst, ps_mc->s_luma.u4_src_wd, u4_dst_wd);
        }

        else
        {
            pi2_mv[MV_X]    = ps_dec->ai2_pred_mv[ps_dec->e_mb_pred][FIRST][MV_X];
            pi2_mv[MV_Y]    = ps_dec->ai2_pred_mv[ps_dec->e_mb_pred][FIRST][MV_Y];

            ps_dec->u2_cbp     = 0;

            pi2_mv           = (WORD16 *)&ps_dec->ai2_mv[FORW][FIRST];
            ps_mc           = &ps_dec->as_mb_mc_params[FORW][FIRST];
            ps_mc->s_ref      = ps_dec->as_ref_buf[ps_dec->e_mb_pred][ps_dec->u2_fld_parity];

            impeg2d_set_mc_params(&ps_mc->s_luma, &ps_mc->s_chroma, e_mb_type, 0,
                      pi2_mv, ps_dec->u2_mb_x, ps_dec->u2_mb_y, ps_dec->u2_frame_width, ps_dec->u2_frame_height,ps_dec->u2_picture_width);

            u4_dst_offset_x = (ps_dec->u2_mb_x << 4) + u4_frm_offset;
            u4_dst_offset_y = (ps_dec->u2_mb_y << 4) * u4_dst_wd;

            ps_mc->s_luma.u4_dst_offset_res_buf = u4_dst_offset_x + u4_dst_offset_y;
            ps_mc->s_luma.u4_dst_wd_res_buf = u4_dst_wd;

            u4_dst_offset_x = u4_dst_offset_x >> 1;
            u4_dst_offset_y = u4_dst_offset_y >> 2;

            ps_mc->s_chroma.u4_dst_offset_res_buf = u4_dst_offset_x + u4_dst_offset_y;
            ps_mc->s_chroma.u4_dst_wd_res_buf = u4_dst_wd >> 1;

            impeg2d_motion_comp(ps_dec, ps_mc, &ps_dec->s_cur_frm_buf);
        }


        /********************************************************************/
        /* Common MB processing tasks                                       */
        /********************************************************************/
        ps_dec->u2_mb_x++;
        ps_dec->u2_num_mbs_left--;

        if (ps_dec->u2_mb_x == ps_dec->u2_num_horiz_mb)
        {
            ps_dec->u2_mb_x = 0;
            ps_dec->u2_mb_y++;
        }
    }

}

/*******************************************************************************
*  Function Name   : impeg2d_dec_skip_b_mb
*
*  Description     : Performs processing needed for Skipped MB encountered in
*                    B Pictures with previous MB bi-predicted.
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*******************************************************************************/
void impeg2d_dec_skip_b_mb(dec_state_t *ps_dec, WORD32 u4_num_of_mbs)
{


    WORD16  *pi2_mv;

    UWORD32 i;
    e_mb_type_t e_mb_type;
    mb_mc_params_t *ps_mc;

    WORD32 i4_iter;
    UWORD32 u4_dst_wd;
    yuv_buf_t s_dst;
    UWORD32  u4_dst_offset_x;
    UWORD32  u4_dst_offset_y;
    UWORD32 u4_frm_offset = 0;

    u4_dst_wd = ps_dec->u2_frame_width;
    s_dst = ps_dec->s_cur_frm_buf;

    if(ps_dec->u2_picture_structure != FRAME_PICTURE)
    {
        u4_dst_wd <<= 1;
        if(ps_dec->u2_picture_structure == BOTTOM_FIELD)
        {
            u4_frm_offset = ps_dec->u2_frame_width;
        }
    }

    for (i4_iter = u4_num_of_mbs; i4_iter > 0; i4_iter--)
    {
        ps_dec->u2_prev_intra_mb = 0;

        if(ps_dec->u2_picture_structure == FRAME_PICTURE)
        {
            e_mb_type = MC_FRM_FW_AND_BK_2MV;
        }
        else
        {
            e_mb_type = MC_FLD_FW_AND_BK_2MV;
        }

        /************************************************************************/
        /* Setting of first motion vector for B MB                              */
        /************************************************************************/
        pi2_mv               = (WORD16 *)&(ps_dec->ai2_mv[FORW][FIRST]);
        {
            pi2_mv[MV_X]         = ps_dec->ai2_pred_mv[FORW][FIRST][MV_X];
            pi2_mv[MV_Y]         = ps_dec->ai2_pred_mv[FORW][FIRST][MV_Y];
        }
        /************************************************************************/
        /* Setting of second motion vector for B MB                             */
        /************************************************************************/
        pi2_mv               = (WORD16 *)&(ps_dec->ai2_mv[BACK][FIRST]);
        {
            pi2_mv[MV_X]         = ps_dec->ai2_pred_mv[BACK][FIRST][MV_X];
            pi2_mv[MV_Y]         = ps_dec->ai2_pred_mv[BACK][FIRST][MV_Y];
        }
        ps_dec->u2_cbp  = 0;

        for(i = 0; i < 2; i++)
        {
            pi2_mv          = (WORD16 *)&ps_dec->ai2_mv[i][FIRST];
            ps_mc          = &ps_dec->as_mb_mc_params[i][FIRST];
            ps_mc->s_ref     = ps_dec->as_ref_buf[i][ps_dec->u2_fld_parity];

            impeg2d_set_mc_params(&ps_mc->s_luma, &ps_mc->s_chroma, e_mb_type, 0, pi2_mv, ps_dec->u2_mb_x,
                          ps_dec->u2_mb_y, ps_dec->u2_frame_width, ps_dec->u2_frame_height,ps_dec->u2_picture_width);
        }

        impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[FORW][FIRST], &ps_dec->s_mc_fw_buf);
        impeg2d_motion_comp(ps_dec, &ps_dec->as_mb_mc_params[BACK][FIRST], &ps_dec->s_mc_bk_buf);

        u4_dst_offset_x = (ps_dec->u2_mb_x << 4) + u4_frm_offset;
        u4_dst_offset_y = (ps_dec->u2_mb_y << 4) * u4_dst_wd;

        s_dst.pu1_y = ps_dec->s_cur_frm_buf.pu1_y + u4_dst_offset_x + u4_dst_offset_y;

        u4_dst_offset_x = u4_dst_offset_x >> 1;
        u4_dst_offset_y = u4_dst_offset_y >> 2;

        s_dst.pu1_u = ps_dec->s_cur_frm_buf.pu1_u + u4_dst_offset_x + u4_dst_offset_y;
        s_dst.pu1_v = ps_dec->s_cur_frm_buf.pu1_v + u4_dst_offset_x + u4_dst_offset_y;

        ps_dec->pf_interpolate(&ps_dec->s_mc_fw_buf,&ps_dec->s_mc_bk_buf,&s_dst, u4_dst_wd);
//        dec->pf_copy_mb(&dec->mc_buf, &dst, MB_SIZE, dst_wd);

        /********************************************************************/
        /* Common MB processing tasks                                       */
        /********************************************************************/
        ps_dec->u2_mb_x++;
        ps_dec->u2_num_mbs_left--;

        if (ps_dec->u2_mb_x == ps_dec->u2_num_horiz_mb)
        {
            ps_dec->u2_mb_x = 0;
            ps_dec->u2_mb_y++;
        }
    }
}
/*******************************************************************************
*  Function Name   : impeg2d_dec_skip_mbs
*
*  Description     : Performs processing needed for Skipped MB encountered in
*                    B Pictures with previous MB bi-predicted.
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*******************************************************************************/
void impeg2d_dec_skip_mbs(dec_state_t *ps_dec, UWORD16 u2_num_skip_mbs)
{
    PROFILE_DISABLE_SKIP_MB();

    if(ps_dec->e_mb_pred == BIDIRECT)
    {
        impeg2d_dec_skip_b_mb(ps_dec, u2_num_skip_mbs);
    }
    else
    {
        impeg2d_dec_skip_p_mb(ps_dec, u2_num_skip_mbs);
    }

    ps_dec->u2_def_dc_pred[Y_LUMA] = 128 << ps_dec->u2_intra_dc_precision;
    ps_dec->u2_def_dc_pred[U_CHROMA] = 128 << ps_dec->u2_intra_dc_precision;
    ps_dec->u2_def_dc_pred[V_CHROMA] = 128 << ps_dec->u2_intra_dc_precision;
}




/*****************************************************************************
*  Function Name   : impeg2d_dec_0mv_coded_mb
*
*  Description     : Decodes the MB with 0 MV but coded. This can occur in P
*                    pictures only
*
*  Arguments       :
*  dec             : Decoder state
*
*  Values Returned : None
*****************************************************************************/
void impeg2d_dec_0mv_coded_mb(dec_state_t *ps_dec)
{


    WORD16   *pi2_mv;
    e_mb_type_t e_mb_type;
    mb_mc_params_t *ps_mc;

    if(ps_dec->u2_picture_structure == FRAME_PICTURE)
    {
        e_mb_type = MC_FRM_FW_AND_BK_2MV;
    }
    else
    {
        e_mb_type = MC_FLD_FW_AND_BK_2MV;
    }




    /* Reset the motion vector predictors */
    memset(ps_dec->ai2_pred_mv,0,sizeof(ps_dec->ai2_pred_mv));

    pi2_mv           = (WORD16 *)&ps_dec->ai2_mv[FORW][FIRST];
    ps_mc           = &ps_dec->as_mb_mc_params[FORW][FIRST];
    ps_mc->s_ref      = ps_dec->as_ref_buf[FORW][ps_dec->u2_fld_parity];

    pi2_mv[MV_X] = 0;
    pi2_mv[MV_Y] = 0;

    impeg2d_set_mc_params(&ps_mc->s_luma, &ps_mc->s_chroma, e_mb_type, 0,
              pi2_mv, ps_dec->u2_mb_x, ps_dec->u2_mb_y, ps_dec->u2_frame_width, ps_dec->u2_frame_height,ps_dec->u2_picture_width);
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_mc_halfx_halfy()                                 */
/*                                                                           */
/*  Description   : Gets the buffer from (0.5,0.5) to (8.5,8.5)              */
/*                  and the above block of size 8 x 8 will be placed as a    */
/*                  block from the current position of out_buf               */
/*                                                                           */
/*  Inputs        : ref - Reference frame from which the block will be       */
/*                        block will be extracted.                           */
/*                  ref_wid - WIdth of reference frame                       */
/*                  out_wid - WIdth of the output frame                      */
/*                  blk_width  - width of the block                          */
/*                  blk_width  - height of the block                         */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Point to the (0,0),(1,0),(0,1),(1,1) position in         */
/*                  the ref frame.Interpolate these four values to get the   */
/*                  value at(0.5,0.5).Repeat this to get an 8 x 8 block      */
/*                  using 9 x 9 block from reference frame                   */
/*                                                                           */
/*  Outputs       : out -  Output containing the extracted block             */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         05 09 2005   Harish M        First Version                        */
/*                                                                           */
/*****************************************************************************/
void impeg2d_mc_halfx_halfy(void *pv_dec,
                           UWORD8 *pu1_ref,
                           UWORD32 u4_ref_wid,
                           UWORD8 *pu1_out,
                           UWORD32 u4_out_wid,
                           UWORD32 u4_blk_width,
                           UWORD32 u4_blk_height)
{
   UWORD8 *pu1_out_ptr,*pu1_ref_ptr;
   dec_state_t *ps_dec = (dec_state_t *)pv_dec;

        pu1_out_ptr = pu1_out;
        pu1_ref_ptr = pu1_ref;

    if((u4_blk_width == MB_SIZE) && (u4_blk_height == MB_SIZE))
    {

        /*luma 16 x 16*/

        /*block 0*/
        ps_dec->pf_halfx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block1*/
        pu1_out_ptr = (pu1_out + BLK_SIZE);
        pu1_ref_ptr = (pu1_ref + BLK_SIZE);
        ps_dec->pf_halfx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 2*/
        pu1_out_ptr = pu1_out + BLK_SIZE * u4_out_wid;
        pu1_ref_ptr = pu1_ref + BLK_SIZE * u4_ref_wid;
        ps_dec->pf_halfx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 3*/
        pu1_out_ptr = pu1_out + BLK_SIZE * u4_out_wid + BLK_SIZE;
        pu1_ref_ptr = pu1_ref + BLK_SIZE * u4_ref_wid + BLK_SIZE;
        ps_dec->pf_halfx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);




    }
    else if ((u4_blk_width == BLK_SIZE) && (u4_blk_height == BLK_SIZE))
    {
        /*chroma 8 x 8*/
        ps_dec->pf_halfx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);
    }
    else if ((u4_blk_width == MB_SIZE) && (u4_blk_height == BLK_SIZE))
    {
        /*block 0*/
        ps_dec->pf_halfx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 1*/
        pu1_out_ptr = (pu1_out + BLK_SIZE);
        pu1_ref_ptr = (pu1_ref + BLK_SIZE);
        ps_dec->pf_halfx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

    }

    else
    {
        UWORD8 *ref_p0,*ref_p1,*ref_p2,*ref_p3;
        UWORD32 i,j;
        /* P0-P3 are the pixels in the reference frame and Q is the value being */
        /* estimated                                                            */
        /*
           P0 P1
             Q
           P2 P3
        */

        ref_p0 = pu1_ref;
        ref_p1 = pu1_ref + 1;
        ref_p2 = pu1_ref + u4_ref_wid;
        ref_p3 = pu1_ref + u4_ref_wid + 1;

        for(i = 0; i < u4_blk_height; i++)
        {
            for(j = 0; j < u4_blk_width; j++)
            {
                *pu1_out++ =   (( (*ref_p0++ )
                            + (*ref_p1++ )
                            + (*ref_p2++ )
                            + (*ref_p3++ ) + 2 ) >> 2);
            }
            ref_p0 += u4_ref_wid - u4_blk_width;
            ref_p1 += u4_ref_wid - u4_blk_width;
            ref_p2 += u4_ref_wid - u4_blk_width;
            ref_p3 += u4_ref_wid - u4_blk_width;

            pu1_out    += u4_out_wid - u4_blk_width;
        }
    }
    return;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_mc_halfx_fully()                                 */
/*                                                                           */
/*  Description   : Gets the buffer from (0.5,0) to (8.5,8)                  */
/*                  and the above block of size 8 x 8 will be placed as a    */
/*                  block from the current position of out_buf               */
/*                                                                           */
/*  Inputs        : ref - Reference frame from which the block will be       */
/*                        block will be extracted.                           */
/*                  ref_wid - WIdth of reference frame                       */
/*                  out_wid - WIdth of the output frame                      */
/*                  blk_width  - width of the block                          */
/*                  blk_width  - height of the block                         */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Point to the (0,0) and (1,0) position in the ref frame   */
/*                  Interpolate these two values to get the value at(0.5,0)  */
/*                  Repeat this to get an 8 x 8 block using 9 x 8 block from */
/*                  reference frame                                          */
/*                                                                           */
/*  Outputs       : out -  Output containing the extracted block             */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         05 09 2005   Harish M        First Version                        */
/*                                                                           */
/*****************************************************************************/

void impeg2d_mc_halfx_fully(void *pv_dec,
                            UWORD8 *pu1_ref,
                            UWORD32 u4_ref_wid,
                            UWORD8 *pu1_out,
                            UWORD32 u4_out_wid,
                            UWORD32 u4_blk_width,
                            UWORD32 u4_blk_height)
{
    UWORD8 *pu1_out_ptr,*pu1_ref_ptr;
    dec_state_t *ps_dec = (dec_state_t *)pv_dec;

        pu1_out_ptr = pu1_out;
        pu1_ref_ptr = pu1_ref;

    if((u4_blk_width == MB_SIZE) && (u4_blk_height == MB_SIZE))
    {

        /*luma 16 x 16*/

        /*block 0*/
        ps_dec->pf_halfx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block1*/
        pu1_out_ptr = (pu1_out + BLK_SIZE);
        pu1_ref_ptr = (pu1_ref + BLK_SIZE);
        ps_dec->pf_halfx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 2*/
        pu1_out_ptr = pu1_out + BLK_SIZE * u4_out_wid;
        pu1_ref_ptr = pu1_ref + BLK_SIZE * u4_ref_wid;
        ps_dec->pf_halfx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 3*/
        pu1_out_ptr = pu1_out + BLK_SIZE * u4_out_wid + BLK_SIZE;
        pu1_ref_ptr = pu1_ref + BLK_SIZE * u4_ref_wid + BLK_SIZE;
        ps_dec->pf_halfx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);




    }
    else if ((u4_blk_width == BLK_SIZE) && (u4_blk_height == BLK_SIZE))
    {
        /*chroma 8 x 8*/
        ps_dec->pf_halfx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);
    }
    else if ((u4_blk_width == MB_SIZE) && (u4_blk_height == BLK_SIZE))
    {
        /*block 0*/
        ps_dec->pf_halfx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 1*/
        pu1_out_ptr = (pu1_out + BLK_SIZE);
        pu1_ref_ptr = (pu1_ref + BLK_SIZE);
        ps_dec->pf_halfx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

    }

    else
    {
        UWORD8 *ref_p0,*ref_p1;
        UWORD32 i,j;

        /* P0-P3 are the pixels in the reference frame and Q is the value being */
        /* estimated                                                            */
        /*
           P0 Q P1
        */

        ref_p0 = pu1_ref;
        ref_p1 = pu1_ref + 1;

        for(i = 0; i < u4_blk_height; i++)
        {
            for(j = 0; j < u4_blk_width; j++)
            {
                *pu1_out++ =   ((( *ref_p0++ )
                            + (*ref_p1++) + 1 ) >> 1);
            }
            ref_p0 += u4_ref_wid - u4_blk_width;
            ref_p1 += u4_ref_wid - u4_blk_width;

            pu1_out    += u4_out_wid - u4_blk_width;
        }
    }
    return;
}


/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_mc_fullx_halfy()                                 */
/*                                                                           */
/*  Description   : Gets the buffer from (0,0.5) to (8,8.5)                  */
/*                  and the above block of size 8 x 8 will be placed as a    */
/*                  block from the current position of out_buf               */
/*                                                                           */
/*  Inputs        : ref - Reference frame from which the block will be       */
/*                        block will be extracted.                           */
/*                  ref_wid - WIdth of reference frame                       */
/*                  out_wid - WIdth of the output frame                      */
/*                  blk_width  - width of the block                          */
/*                  blk_width  - height of the block                         */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Point to the (0,0) and (0,1)   position in the ref frame */
/*                  Interpolate these two values to get the value at(0,0.5)  */
/*                  Repeat this to get an 8 x 8 block using 8 x 9 block from */
/*                  reference frame                                          */
/*                                                                           */
/*  Outputs       : out -  Output containing the extracted block             */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         05 09 2005   Harish M        First Version                        */
/*                                                                           */
/*****************************************************************************/
void impeg2d_mc_fullx_halfy(void *pv_dec,
                            UWORD8 *pu1_ref,
                            UWORD32 u4_ref_wid,
                            UWORD8 *pu1_out,
                            UWORD32 u4_out_wid,
                            UWORD32 u4_blk_width,
                            UWORD32 u4_blk_height)
{

    UWORD8 *pu1_out_ptr,*pu1_ref_ptr;
    dec_state_t *ps_dec = (dec_state_t *)pv_dec;
        pu1_out_ptr = pu1_out;
        pu1_ref_ptr = pu1_ref;

    if((u4_blk_width == MB_SIZE) && (u4_blk_height == MB_SIZE))
    {

        /*luma 16 x 16*/

        /*block 0*/
        ps_dec->pf_fullx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block1*/
        pu1_out_ptr = (pu1_out + BLK_SIZE);
        pu1_ref_ptr = (pu1_ref + BLK_SIZE);
        ps_dec->pf_fullx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 2*/
        pu1_out_ptr = pu1_out + BLK_SIZE * u4_out_wid;
        pu1_ref_ptr = pu1_ref + BLK_SIZE * u4_ref_wid;
        ps_dec->pf_fullx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 3*/
        pu1_out_ptr = pu1_out + BLK_SIZE * u4_out_wid + BLK_SIZE;
        pu1_ref_ptr = pu1_ref + BLK_SIZE * u4_ref_wid + BLK_SIZE;
        ps_dec->pf_fullx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);




    }
    else if ((u4_blk_width == BLK_SIZE) && (u4_blk_height == BLK_SIZE))
    {
        /*chroma 8 x 8*/
        ps_dec->pf_fullx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);
    }
    else if ((u4_blk_width == MB_SIZE) && (u4_blk_height == BLK_SIZE))
    {
        /*block 0*/
        ps_dec->pf_fullx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 1*/
        pu1_out_ptr = (pu1_out + BLK_SIZE);
        pu1_ref_ptr = (pu1_ref + BLK_SIZE);
        ps_dec->pf_fullx_halfy_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

    }

    else if ((u4_blk_width == BLK_SIZE) && (u4_blk_height == (BLK_SIZE / 2)))
    {
        UWORD8 *ref_p0,*ref_p1;
        UWORD32 i,j;
        /* P0-P3 are the pixels in the reference frame and Q is the value being */
        /* estimated                                                            */
        /*
           P0
            x
           P1
        */
        ref_p0 = pu1_ref;
        ref_p1 = pu1_ref + u4_ref_wid;

        for(i = 0; i < u4_blk_height; i++)
        {
            for(j = 0; j < u4_blk_width; j++)
            {
                *pu1_out++ =   ((( *ref_p0++)
                            + (*ref_p1++) + 1 ) >> 1);
            }
            ref_p0 += u4_ref_wid - u4_blk_width;
            ref_p1 += u4_ref_wid - u4_blk_width;

            pu1_out    += u4_out_wid - u4_blk_width;
        }
    }
    return;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2d_mc_fullx_fully()                                 */
/*                                                                           */
/*  Description   : Gets the buffer from (x,y) to (x+8,y+8)                  */
/*                  and the above block of size 8 x 8 will be placed as a    */
/*                  block from the current position of out_buf               */
/*                                                                           */
/*  Inputs        : ref - Reference frame from which the block will be       */
/*                        block will be extracted.                           */
/*                  ref_wid - WIdth of reference frame                       */
/*                  out_wid - WIdth of the output frame                      */
/*                  blk_width  - width of the block                          */
/*                  blk_width  - height of the block                         */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Point to the (0,0) position in the ref frame             */
/*                  Get an 8 x 8 block from reference frame                  */
/*                                                                           */
/*  Outputs       : out -  Output containing the extracted block             */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         05 09 2005   Harish M        First Version                        */
/*                                                                           */
/*****************************************************************************/

void impeg2d_mc_fullx_fully(void *pv_dec,
                            UWORD8 *pu1_ref,
                            UWORD32 u4_ref_wid,
                            UWORD8 *pu1_out,
                            UWORD32 u4_out_wid,
                            UWORD32 u4_blk_width,
                            UWORD32 u4_blk_height)
{

    UWORD8 *pu1_out_ptr,*pu1_ref_ptr;
    dec_state_t *ps_dec = (dec_state_t *)pv_dec;

        pu1_out_ptr = pu1_out;
        pu1_ref_ptr = pu1_ref;

    if((u4_blk_width == MB_SIZE) && (u4_blk_height == MB_SIZE))
    {

        /*luma 16 x 16*/

        /*block 0*/
        ps_dec->pf_fullx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block1*/
        pu1_out_ptr = (pu1_out + BLK_SIZE);
        pu1_ref_ptr = (pu1_ref + BLK_SIZE);
        ps_dec->pf_fullx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 2*/
        pu1_out_ptr = pu1_out + BLK_SIZE * u4_out_wid;
        pu1_ref_ptr = pu1_ref + BLK_SIZE * u4_ref_wid;
        ps_dec->pf_fullx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 3*/
        pu1_out_ptr = pu1_out + BLK_SIZE * u4_out_wid + BLK_SIZE;
        pu1_ref_ptr = pu1_ref + BLK_SIZE * u4_ref_wid + BLK_SIZE;
        ps_dec->pf_fullx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);




    }
    else if ((u4_blk_width == BLK_SIZE) && (u4_blk_height == BLK_SIZE))
    {
        /*chroma 8 x 8*/
        ps_dec->pf_fullx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);
    }
    else if ((u4_blk_width == MB_SIZE) && (u4_blk_height == BLK_SIZE))
    {
        /*block 0*/
        ps_dec->pf_fullx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

        /*block 1*/
        pu1_out_ptr = (pu1_out + BLK_SIZE);
        pu1_ref_ptr = (pu1_ref + BLK_SIZE);
        ps_dec->pf_fullx_fully_8x8(pu1_out_ptr,pu1_ref_ptr,u4_ref_wid,u4_out_wid);

    }
    else
    {
        UWORD32 i;

        for(i = 0; i < u4_blk_height; i++)
        {
            memcpy(pu1_out, pu1_ref, u4_blk_width);
            pu1_ref += u4_ref_wid;
            pu1_out += u4_out_wid;
        }
    }
    return;
}

/*******************************************************************************
*  Function Name   : impeg2d_set_mc_params
*
*  Description     : Sets the parameters for Motion Compensation
*
*  Arguments       :
*  luma            : Parameters for luma blocks
*  chroma          : Parameters for chroma blocks
*  type            : Motion compensation type
*  mv_num          : Number of motion vectors
*  mv              : Motion Vectors
*  mb_x            : X co-ordinate of MB
*  mb_y            : Y co-ordinate of MB
*  frm_wd          : Width of the frame
*
*  Values Returned : None
*******************************************************************************/
void impeg2d_set_mc_params(comp_mc_params_t *ps_luma,
                           comp_mc_params_t *ps_chroma,
                           e_mb_type_t e_type,
                           UWORD16 u2_mv_num,
                           WORD16 ai2_mv[],
                           UWORD16 u2_mb_x,
                           UWORD16 u2_mb_y,
                           UWORD16 u2_frm_wd,
                           UWORD16 u2_frm_ht,
                           UWORD16 u2_picture_width)
{
    WORD16 i2_mvy_round;
    WORD16 i2_mvx_round;
    const mc_type_consts_t *ps_mc_params;
    WORD16 i2_mvx_fullp_round;
    WORD16 i2_mvy_fullp_round;
    UWORD32 u4_frm_chroma_wd;
    WORD16 i2_pix_x, i2_pix_y;

    ps_mc_params = &gas_impeg2d_mc_params_luma[e_type][u2_mv_num];
    /****************************************************************************/
    /* get luma mc params                                                       */
    /****************************************************************************/
    i2_pix_x = MB_SIZE * u2_mb_x + (ai2_mv[MV_X]>>1);
    i2_pix_y = (MB_SIZE * u2_mb_y  +
        (ai2_mv[MV_Y]>>1) * ps_mc_params->mvy_cf + u2_mv_num * ps_mc_params->mv_num_cf) * ps_mc_params->frm_wd_cf;

    // clip pix_x and pix_y so as it falls inside the frame boundary
    CLIP(i2_pix_x, (u2_frm_wd-16), 0);
    CLIP(i2_pix_y, (u2_frm_ht-16), 0);

    ps_luma->u4_src_offset = i2_pix_x +  i2_pix_y * u2_frm_wd;


    /* keep offset  in full pel */
    ps_luma->u4_rows          = ps_mc_params->rows;
    ps_luma->u4_cols          = MB_SIZE;
    ps_luma->u4_dst_wd_res_buf        = ps_mc_params->dst_wd;
    ps_luma->u4_src_wd        = u2_frm_wd * ps_mc_params->src_wd_cf;
    ps_luma->u4_dst_offset_res_buf    = ps_mc_params->dst_offset_scale * MB_SIZE;
    ps_luma->u4_dst_offset_cur_frm    = ps_mc_params->dst_offset_scale * u2_picture_width;
    ps_luma->u4_mode          = ((ai2_mv[MV_X] & 1) << 1) | (ai2_mv[MV_Y] & 1);

    /****************************************************************************/
    /* get chroma mc params                                                     */
    /****************************************************************************/
    ps_mc_params   = &gas_impeg2d_mc_params_chroma[e_type][u2_mv_num];
    i2_mvx_round   = ((ai2_mv[MV_X] + IS_NEG(ai2_mv[MV_X]))>>1);
    i2_mvy_round   = ((ai2_mv[MV_Y] + IS_NEG(ai2_mv[MV_Y]))>>1);

    i2_mvx_fullp_round = (i2_mvx_round>>1);
    i2_mvy_fullp_round = (i2_mvy_round>>1)*ps_mc_params->mvy_cf;

    u4_frm_chroma_wd = (u2_frm_wd>>1);

    i2_pix_x = (MB_SIZE/2) * u2_mb_x + i2_mvx_fullp_round;
    i2_pix_y = ((MB_SIZE/2) * u2_mb_y + i2_mvy_fullp_round + u2_mv_num *
                           ps_mc_params->mv_num_cf)*ps_mc_params->frm_wd_cf;

    CLIP(i2_pix_x, ((u2_frm_wd / 2)-8), 0);
    CLIP(i2_pix_y, ((u2_frm_ht / 2)-8), 0);
    ps_chroma->u4_src_offset = i2_pix_x + i2_pix_y * u4_frm_chroma_wd;


    /* keep offset  in full pel */
    ps_chroma->u4_rows = ps_mc_params->rows;
    ps_chroma->u4_cols        = (MB_SIZE >> 1);
    ps_chroma->u4_dst_wd_res_buf = ps_mc_params->dst_wd;
    ps_chroma->u4_src_wd = (u2_frm_wd>>1) * ps_mc_params->src_wd_cf;
    ps_chroma->u4_dst_offset_res_buf = ps_mc_params->dst_offset_scale * MB_CHROMA_SIZE;
    ps_chroma->u4_dst_offset_cur_frm = ps_mc_params->dst_offset_scale * (u2_picture_width >> 1);
    ps_chroma->u4_mode = ((i2_mvx_round & 1) << 1) | (i2_mvy_round & 1);



    ps_luma->u4_dst_wd_cur_frm = u2_picture_width;
    ps_chroma->u4_dst_wd_cur_frm = u2_picture_width >> 1;

    if(ps_luma->u4_dst_wd_res_buf == MB_SIZE * 2)
    {
        ps_luma->u4_dst_wd_cur_frm = u2_frm_wd << 1;
        ps_chroma->u4_dst_wd_cur_frm = u2_frm_wd;
    }
}


