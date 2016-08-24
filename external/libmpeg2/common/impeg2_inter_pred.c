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
*  impeg2d_mcu.c
*
* @brief
*  Contains MC function definitions for MPEG2 decoder
*
* @author
*  Harish
*
* @par List of Functions:
* - impeg2_copy_mb()
* - impeg2_interpolate()
* - impeg2_mc_halfx_halfy_8x8()
* - impeg2_mc_halfx_fully_8x8()
* - impeg2_mc_fullx_halfy_8x8()
* - impeg2_mc_fullx_fully_8x8()
*
* @remarks
*  None
*
*******************************************************************************
*/

#include <stdio.h>
#include <string.h>
#include "iv_datatypedef.h"
#include "iv.h"
#include "impeg2_buf_mgr.h"
#include "impeg2_disp_mgr.h"
#include "impeg2_defs.h"
#include "impeg2_platform_macros.h"

#include "impeg2_inter_pred.h"
#include "impeg2_globals.h"
#include "impeg2_macros.h"
#include "impeg2_idct.h"

/*******************************************************************************
*  Function Name   : impeg2_copy_mb
*
*  Description     : copies 3 components to the frame from mc_buf
*
*  Arguments       :
*  src_buf         : Source Buffer
*  dst_buf         : Destination Buffer
*  src_offset_x    : X offset for source
*  src_offset_y    : Y offset for source
*  dst_offset_x    : X offset for destination
*  dst_offset_y    : Y offset for destination
*  src_wd          : Source Width
*  dst_wd          : destination Width
*  rows            : Number of rows
*  cols            : Number of columns
*
*  Values Returned : None
*******************************************************************************/
void impeg2_copy_mb(yuv_buf_t *ps_src_buf,
                    yuv_buf_t *ps_dst_buf,
                    UWORD32 u4_src_wd,
                    UWORD32 u4_dst_wd)
{
    UWORD8 *pu1_src;
    UWORD8 *pu1_dst;
    UWORD32 i;
    UWORD32 u4_rows = MB_SIZE;
    UWORD32 u4_cols = MB_SIZE;

    /*******************************************************/
    /* copy Y                                              */
    /*******************************************************/
    pu1_src = ps_src_buf->pu1_y;
    pu1_dst = ps_dst_buf->pu1_y;
    for(i = 0; i < u4_rows; i++)
    {
        memcpy(pu1_dst, pu1_src, u4_cols);
        pu1_src += u4_src_wd;
        pu1_dst += u4_dst_wd;
    }

    u4_src_wd >>= 1;
    u4_dst_wd >>= 1;
    u4_rows >>= 1;
    u4_cols >>= 1;

    /*******************************************************/
    /* copy U                                              */
    /*******************************************************/
    pu1_src = ps_src_buf->pu1_u;
    pu1_dst = ps_dst_buf->pu1_u;
    for(i = 0; i < u4_rows; i++)
    {
        memcpy(pu1_dst, pu1_src, u4_cols);

        pu1_src += u4_src_wd;
        pu1_dst += u4_dst_wd;
    }
    /*******************************************************/
    /* copy V                                              */
    /*******************************************************/
    pu1_src = ps_src_buf->pu1_v;
    pu1_dst = ps_dst_buf->pu1_v;
    for(i = 0; i < u4_rows; i++)
    {
        memcpy(pu1_dst, pu1_src, u4_cols);

        pu1_src += u4_src_wd;
        pu1_dst += u4_dst_wd;
    }

}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_interpolate                                       */
/*                                                                           */
/*  Description   : averages the contents of buf_src1 and buf_src2 and stores*/
/*                  result in buf_dst                                        */
/*                                                                           */
/*  Inputs        : buf_src1 -  First Source                                 */
/*                  buf_src2 -  Second Source                                */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Avg the values from two sources and store the result in  */
/*                  destination buffer                                       */
/*                                                                           */
/*  Outputs       : buf_dst  -  Avg of contents of buf_src1 and buf_src2     */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : Assumes that all 3 buffers are of same size              */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         14 09 2005   Harish M        First Version                        */
/*         15 09 2010   Venkat          Added stride                         */
/*                                                                           */
/*****************************************************************************/
void impeg2_interpolate(yuv_buf_t *ps_buf_src1,
                        yuv_buf_t *ps_buf_src2,
                        yuv_buf_t *ps_buf_dst,
                        UWORD32 u4_stride)
{

    UWORD32 i,j;
    UWORD8 *pu1_src1,*pu1_src2,*pu1_dst;
    pu1_src1 = ps_buf_src1->pu1_y;
    pu1_src2 = ps_buf_src2->pu1_y;
    pu1_dst  = ps_buf_dst->pu1_y;
    for(i = MB_SIZE; i > 0; i--)
    {
        for(j = MB_SIZE; j > 0; j--)
        {
            *pu1_dst++ = ((*pu1_src1++) + (*pu1_src2++) + 1) >> 1;
        }

        pu1_dst += u4_stride - MB_SIZE;

    }

    u4_stride >>= 1;

    pu1_src1 = ps_buf_src1->pu1_u;
    pu1_src2 = ps_buf_src2->pu1_u;
    pu1_dst  = ps_buf_dst->pu1_u;
    for(i = MB_CHROMA_SIZE; i > 0 ; i--)
    {
        for(j = MB_CHROMA_SIZE; j > 0; j--)
        {
            *pu1_dst++ = ((*pu1_src1++) + (*pu1_src2++) + 1) >> 1;
        }

        pu1_dst += u4_stride - MB_CHROMA_SIZE;
    }

    pu1_src1 = ps_buf_src1->pu1_v;
    pu1_src2 = ps_buf_src2->pu1_v;
    pu1_dst  = ps_buf_dst->pu1_v;
    for(i = MB_CHROMA_SIZE; i > 0 ; i--)
    {
        for(j = MB_CHROMA_SIZE; j > 0; j--)
        {
            *pu1_dst++ = ((*pu1_src1++) + (*pu1_src2++) + 1) >> 1;
        }

        pu1_dst += u4_stride - MB_CHROMA_SIZE;
    }

}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_mc_halfx_halfy_8x8()                                 */
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
void impeg2_mc_halfx_halfy_8x8(UWORD8 *pu1_out,
                            UWORD8 *pu1_ref,
                            UWORD32 u4_ref_wid,
                            UWORD32 u4_out_wid)
{
    UWORD8 *pu1_ref_p0,*pu1_ref_p1,*pu1_ref_p2,*pu1_ref_p3;
    UWORD32 i,j;
    /* P0-P3 are the pixels in the reference frame and Q is the value being */
    /* estimated                                                            */
    /*
       P0 P1
         Q
       P2 P3
    */

    pu1_ref_p0 = pu1_ref;
    pu1_ref_p1 = pu1_ref + 1;
    pu1_ref_p2 = pu1_ref + u4_ref_wid;
    pu1_ref_p3 = pu1_ref + u4_ref_wid + 1;

    for(i = 0; i < BLK_SIZE; i++)
    {
        for(j = 0; j < BLK_SIZE; j++)
        {
            *pu1_out++ =   (( (*pu1_ref_p0++ )
                        + (*pu1_ref_p1++ )
                        + (*pu1_ref_p2++ )
                        + (*pu1_ref_p3++ ) + 2 ) >> 2);
        }
        pu1_ref_p0 += u4_ref_wid - BLK_SIZE;
        pu1_ref_p1 += u4_ref_wid - BLK_SIZE;
        pu1_ref_p2 += u4_ref_wid - BLK_SIZE;
        pu1_ref_p3 += u4_ref_wid - BLK_SIZE;

        pu1_out    += u4_out_wid - BLK_SIZE;
    }
    return;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_mc_halfx_fully_8x8()                                 */
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
void impeg2_mc_halfx_fully_8x8(UWORD8 *pu1_out,
                            UWORD8 *pu1_ref,
                            UWORD32 u4_ref_wid,
                            UWORD32 u4_out_wid)
{
    UWORD8 *pu1_ref_p0, *pu1_ref_p1;
    UWORD32 i,j;

    /* P0-P3 are the pixels in the reference frame and Q is the value being */
    /* estimated                                                            */
    /*
       P0 Q P1
    */

    pu1_ref_p0 = pu1_ref;
    pu1_ref_p1 = pu1_ref + 1;

    for(i = 0; i < BLK_SIZE; i++)
    {
        for(j = 0; j < BLK_SIZE; j++)
        {
            *pu1_out++ =   ((( *pu1_ref_p0++ )
                        + (*pu1_ref_p1++) + 1 ) >> 1);
        }
        pu1_ref_p0 += u4_ref_wid - BLK_SIZE;
        pu1_ref_p1 += u4_ref_wid - BLK_SIZE;

        pu1_out    += u4_out_wid - BLK_SIZE;
    }
    return;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_mc_fullx_halfy_8x8()                                 */
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
void impeg2_mc_fullx_halfy_8x8(UWORD8 *pu1_out,
                            UWORD8 *pu1_ref,
                            UWORD32 u4_ref_wid,
                            UWORD32 u4_out_wid)
{

    UWORD8 *pu1_ref_p0, *pu1_ref_p1;
    UWORD32 i,j;
    /* P0-P3 are the pixels in the reference frame and Q is the value being */
    /* estimated                                                            */
    /*
       P0
        x
       P1
    */
    pu1_ref_p0 = pu1_ref;
    pu1_ref_p1 = pu1_ref + u4_ref_wid;

    for(i = 0; i < BLK_SIZE; i++)
    {
        for(j = 0; j < BLK_SIZE; j++)
        {
            *pu1_out++ =   ((( *pu1_ref_p0++)
                        + (*pu1_ref_p1++) + 1 ) >> 1);
        }
        pu1_ref_p0 += u4_ref_wid - BLK_SIZE;
        pu1_ref_p1 += u4_ref_wid - BLK_SIZE;

        pu1_out    += u4_out_wid - BLK_SIZE;
    }

    return;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_mc_fullx_fully_8x8()                                 */
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
void impeg2_mc_fullx_fully_8x8(UWORD8 *pu1_out,
                            UWORD8 *pu1_ref,
                            UWORD32 u4_ref_wid,
                            UWORD32 u4_out_wid)
{

    UWORD32 i;

    for(i = 0; i < BLK_SIZE; i++)
    {
        memcpy(pu1_out, pu1_ref, BLK_SIZE);
        pu1_ref += u4_ref_wid;
        pu1_out += u4_out_wid;
    }
    return;
}
