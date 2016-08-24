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
/*                                                                           */
/*  File Name         : impeg2_format_conv .c                                */
/*                                                                           */
/*  Description       : Contains functions needed to convert the images in   */
/*                      different color spaces to yuv 422i color space       */
/*                                                                           */
/*  List of Functions : YUV420toYUV420()                                      */
/*                      YUV420toYUV422I()                                    */
/*                      YUV420toYUV420SP_VU()                                */
/*                      YUV420toYUV420SP_UU()                                */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         28 08 2007  Naveen Kumar T        Draft                           */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* System include files */

/* User include files */
#include <stdio.h>
#include <string.h>
#include "iv_datatypedef.h"
#include "iv.h"
#include "ithread.h"

#include "iv_datatypedef.h"
#include "impeg2_macros.h"
#include "impeg2_buf_mgr.h"
#include "impeg2_disp_mgr.h"
#include "impeg2_defs.h"
#include "impeg2_platform_macros.h"

#include "impeg2_job_queue.h"
#include "impeg2_format_conv.h"


/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_copy_frm_yuv420p()                                        */
/*                                                                           */
/*  Description   : This function performs conversion from YUV420 to         */
/*                  YUV422I color space.                                     */
/*                                                                           */
/*  Inputs        : pu1_src_y,       -   UWORD8 pointer to source y plane.   */
/*                  pu1_src_u,       -   UWORD8 pointer to source u plane.   */
/*                  pu1_src_v,       -   UWORD8 pointer to source v plane.   */
/*                  pu1_dst_y,       -   UWORD8 pointer to dest y plane.     */
/*                  pu1_dst_u,       -   UWORD8 pointer to dest u plane.     */
/*                  pu1_dst_v,       -   UWORD8 pointer to dest v plane.     */
/*                  u4_width,        -   Width of image.                     */
/*                  u4_height,       -   Height of image.                    */
/*                  u4_src_stride_y  -   Stride in pixels of source Y plane. */
/*                  u4_src_stride_u  -   Stride in pixels of source U plane. */
/*                  u4_src_stride_v  -   Stride in pixels of source V plane. */
/*                  u4_dst_stride_y  -   Stride in pixels of dest Y plane.   */
/*                  u4_dst_stride_u  -   Stride in pixels of dest U plane.   */
/*                  u4_dst_stride_v  -   Stride in pixels of dest V plane.   */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : One row is processed at a time. The one iteration of the */
/*                  code will rearrange pixels into YUV422 interleaved       */
/*                  format.                                                  */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         29 08 2007  Naveen Kumar T        Draft                           */
/*                                                                           */
/*****************************************************************************/
void impeg2_copy_frm_yuv420p(UWORD8 *pu1_src_y,
                             UWORD8 *pu1_src_u,
                             UWORD8 *pu1_src_v,
                             UWORD8 *pu1_dst_y,
                             UWORD8 *pu1_dst_u,
                             UWORD8 *pu1_dst_v,
                             UWORD32 u4_width,
                             UWORD32 u4_height,
                             UWORD32 u4_src_stride_y,
                             UWORD32 u4_src_stride_u,
                             UWORD32 u4_src_stride_v,
                             UWORD32 u4_dst_stride_y,
                             UWORD32 u4_dst_stride_u,
                             UWORD32 u4_dst_stride_v)
{
    WORD32 i4_cnt;
    WORD32  i4_y_height     = (WORD32) u4_height;
    WORD32  i4_uv_height    = u4_height >> 1;
    WORD32  i4_uv_width     = u4_width >> 1;

    for(i4_cnt = 0; i4_cnt < i4_y_height; i4_cnt++)
    {
        memcpy(pu1_dst_y, pu1_src_y, u4_width);
        pu1_dst_y += (u4_dst_stride_y);
        pu1_src_y += (u4_src_stride_y);
    }

    for(i4_cnt = 0; i4_cnt < i4_uv_height; i4_cnt++)
    {
        memcpy(pu1_dst_u, pu1_src_u, i4_uv_width);
        pu1_dst_u += (u4_dst_stride_u);
        pu1_src_u += (u4_src_stride_u);

    }

    for(i4_cnt = 0; i4_cnt < i4_uv_height; i4_cnt++)
    {
        memcpy(pu1_dst_v, pu1_src_v, i4_uv_width);
        pu1_dst_v += (u4_dst_stride_v);
        pu1_src_v += (u4_src_stride_v);

    }

}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : impeg2_fmt_conv_yuv420p_to_yuv422ile()                   */
/*                                                                           */
/*  Description   : This function performs conversion from YUV420 to         */
/*                  YUV422I color space.                                     */
/*                                                                           */
/*  Inputs        : pu1_y            -   UWORD8 pointer to y plane.          */
/*                  pu1_u            -   UWORD8 pointer to u plane.          */
/*                  pu1_v            -   UWORD8 pointer to u plane.          */
/*                  pu2_yuv422i      -   UWORD16 pointer to yuv422iimage.    */
/*                  u4_width         -   Width of the Y plane.               */
/*                  u4_height        -   Height of the Y plane.              */
/*                  u4_stride_y      -   Stride in pixels of Y plane.        */
/*                  u4_stride_u      -   Stride in pixels of U plane.        */
/*                  u4_stride_v      -   Stride in pixels of V plane.        */
/*                  u4_stride_yuv422i-   Stride in pixels of yuv422i image.  */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : One row is processed at a time. The one iteration of the */
/*                  code will rearrange pixels into YUV422 interleaved       */
/*                  format.                                                  */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         29 08 2007  Naveen Kumar T        Draft                           */
/*                                                                           */
/*****************************************************************************/

void impeg2_fmt_conv_yuv420p_to_yuv422ile(register UWORD8 *pu1_y,
                     register UWORD8 *pu1_u,
                     register UWORD8 *pu1_v,
                     void *pv_yuv422i,
                     UWORD32 u4_width,
                     UWORD32 u4_height,
                     UWORD32 u4_stride_y,
                     UWORD32 u4_stride_u,
                     UWORD32 u4_stride_v,
                     UWORD32 u4_stride_yuv422i)
{
    /* Declare local variables */
    register WORD16  i,j;
    register UWORD16 u2_offset1,u2_offset2,u2_offset3,u2_offset_yuv422i;
    register UWORD8  u1_y1,u1_uv;
    register UWORD32 u4_pixel;
    register UWORD16 u2_width_cnt;
    register UWORD32 *pu4_yuv422i;

    UWORD8 u1_flag;             /* This flag is used to indicate wether the row is even or odd */

    u1_flag=0x0;                /* Intialize it with 0 indicating odd row */

    /* Calculate the offsets necessary to make input and output buffers to point next row */
    u2_offset1       = u4_stride_y - u4_width;
    u2_offset2       = u4_stride_u - ((u4_width + 1) >> 1);
    u2_offset3       = u4_stride_v - ((u4_width + 1) >> 1);
    u2_offset_yuv422i = (u4_stride_yuv422i >> 1) -((u4_width + 1) >> 1);

    /* Type cast the output pointer to UWORD32 */
    pu4_yuv422i      = (UWORD32 *)pv_yuv422i;

    /* Calculate the loop counter for inner loop */
    u2_width_cnt     = u4_width >> 1;

    /* Run the loop for height of input buffer */
    for(i = u4_height; i > 0; i--)
    {
        /* Run the loop for width/2 */
        for(j = u2_width_cnt; j > 0; j--)
        {
            /* Store the value in output buffer in the order U0Y0V0Y1U2Y2V2Y3.... */
            /* Load Y0 */
            u1_y1          = *pu1_y++;
            /* Load Y1 */
            u4_pixel       = *pu1_y++;
            /* Load V0 */
            u1_uv          = *pu1_v++;
            u4_pixel       = (u4_pixel << 8) + u1_uv;
            /* Load U0 */
            u1_uv          = *pu1_u++;
            u4_pixel       = (u4_pixel << 8) + u1_y1;
            u4_pixel       = (u4_pixel << 8) + u1_uv;
            *pu4_yuv422i++ = u4_pixel;
        }
        /* Incase of width is odd number take care of last pixel */
        if(u4_width & 0x1)
        {
            /* Store the value in output buffer in the order U0Y0V0Y1U2Y2V2Y3.... */
            /* Load Y0 */
            u1_y1          = *pu1_y++;
            /* Load V0 */
            u1_uv          = *pu1_v++;
            /* Take Y0 as Y1 */
            u4_pixel       = u1_y1;
            u4_pixel       = (u4_pixel << 8) + u1_uv;
            /* Load U0 */
            u1_uv          = *pu1_u++;
            u4_pixel       = (u4_pixel << 8) + u1_y1;
            u4_pixel       = (u4_pixel << 8) + u1_uv;
            *pu4_yuv422i++ = u4_pixel;
        }
        /* Make the pointers to buffer to point to next row */
        pu1_y = pu1_y       + u2_offset1;
        if(!u1_flag)
        {
            /* Restore the pointers of u and v buffer back so that the row of pixels are also  */
            /* Processed with same row of u and values again */
            pu1_u = pu1_u - ((u4_width + 1) >> 1);
            pu1_v = pu1_v - ((u4_width + 1) >> 1);
        }
        else
        {
            /* Adjust the u and v buffer pointers so that they will point to next row */
            pu1_u = pu1_u + u2_offset2;
            pu1_v = pu1_v + u2_offset3;
        }

        /* Adjust the output buffer pointer for next row */
        pu4_yuv422i = pu4_yuv422i + u2_offset_yuv422i;
        /* Toggle the flag to convert between odd and even row */
        u1_flag= u1_flag ^ 0x1;
    }
}




void impeg2_fmt_conv_yuv420p_to_yuv420sp_vu(UWORD8 *pu1_y, UWORD8 *pu1_u, UWORD8 *pu1_v,
                                     UWORD8 *pu1_dest_y, UWORD8 *pu1_dest_uv,
                                     UWORD32 u4_height,  UWORD32 u4_width,UWORD32 u4_stridey,
                                     UWORD32 u4_strideu, UWORD32 u4_stridev,
                                     UWORD32 u4_dest_stride_y, UWORD32 u4_dest_stride_uv,
                                     UWORD32 u4_convert_uv_only
                                     )

{


    UWORD8 *pu1_src,*pu1_dst;
    UWORD8 *pu1_src_u, *pu1_src_v;
    UWORD16 i;
    UWORD32 u2_width_uv;

    UWORD32 u4_dest_inc_y=0, u4_dest_inc_uv=0;


    /* Copy Y buffer */
    pu1_dst = (UWORD8 *)pu1_dest_y;
    pu1_src = (UWORD8 *)pu1_y;

    u4_dest_inc_y =    u4_dest_stride_y;
    u4_dest_inc_uv =   u4_dest_stride_uv;

    if(0 == u4_convert_uv_only)
    {
        for(i = 0; i < u4_height; i++)
        {
            memcpy((void *)pu1_dst,(void *)pu1_src, u4_width);
            pu1_dst += u4_dest_inc_y;
            pu1_src += u4_stridey;
        }
    }

    /* Interleave Cb and Cr buffers */
    pu1_src_u = pu1_u;
    pu1_src_v = pu1_v;
    pu1_dst = pu1_dest_uv ;

    u4_height = (u4_height + 1) >> 1;
    u2_width_uv = (u4_width + 1) >> 1;
    for(i = 0; i < u4_height ; i++)
    {
        UWORD32 j;
        for(j = 0; j < u2_width_uv; j++)
        {
            *pu1_dst++ = *pu1_src_v++;
            *pu1_dst++ = *pu1_src_u++;

        }

        pu1_dst += u4_dest_inc_uv - u4_width;
        pu1_src_u  += u4_strideu - u2_width_uv;
        pu1_src_v  += u4_stridev - u2_width_uv;
    }
}

void impeg2_fmt_conv_yuv420p_to_yuv420sp_uv(UWORD8 *pu1_y, UWORD8 *pu1_u, UWORD8 *pu1_v,
                                     UWORD8 *pu1_dest_y, UWORD8 *pu1_dest_uv,
                                     UWORD32 u4_height,  UWORD32 u4_width,UWORD32 u4_stridey,
                                     UWORD32 u4_strideu, UWORD32 u4_stridev,
                                     UWORD32 u4_dest_stride_y, UWORD32 u4_dest_stride_uv,
                                     UWORD32 u4_convert_uv_only)

{


    UWORD8 *pu1_src,*pu1_dst;
    UWORD8 *pu1_src_u, *pu1_src_v;
    UWORD16 i;
    UWORD32 u2_width_uv;

    UWORD32 u4_dest_inc_y=0, u4_dest_inc_uv=0;


    /* Copy Y buffer */
    pu1_dst = (UWORD8 *)pu1_dest_y;
    pu1_src = (UWORD8 *)pu1_y;

    u4_dest_inc_y =    u4_dest_stride_y;
    u4_dest_inc_uv =   u4_dest_stride_uv;

    if(0 == u4_convert_uv_only)
    {
        for(i = 0; i < u4_height; i++)
        {
            memcpy((void *)pu1_dst,(void *)pu1_src, u4_width);
            pu1_dst += u4_dest_inc_y;
            pu1_src += u4_stridey;
        }
    }

    /* Interleave Cb and Cr buffers */
    pu1_src_u = pu1_u;
    pu1_src_v = pu1_v;
    pu1_dst = pu1_dest_uv ;

    u4_height = (u4_height + 1) >> 1;
    u2_width_uv = (u4_width + 1) >> 1;
    for(i = 0; i < u4_height ; i++)
    {
        UWORD32 j;
        for(j = 0; j < u2_width_uv; j++)
        {
            *pu1_dst++ = *pu1_src_u++;
            *pu1_dst++ = *pu1_src_v++;
        }

        pu1_dst += u4_dest_inc_uv - u4_width;
        pu1_src_u  += u4_strideu - u2_width_uv;
        pu1_src_v  += u4_stridev - u2_width_uv;
    }

}


