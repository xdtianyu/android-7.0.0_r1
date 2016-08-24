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

#include "icv.h"
#include "ideint.h"

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

#include "impeg2d.h"
#include "impeg2d_bitstream.h"
#include "impeg2d_structs.h"
#include "impeg2d_globals.h"
#include "impeg2d_mc.h"
#include "impeg2d_pic_proc.h"
#include "impeg2d_deinterlace.h"

typedef struct
{
    IVD_ARCH_T ivd_arch;
    ICV_ARCH_T icv_arch;
}arch_map_t;

static const arch_map_t gas_impeg2d_arch_mapping[] =
{
    {ARCH_ARM_NONEON,       ICV_ARM_NONEON},
    {ARCH_ARM_A9Q,          ICV_ARM_A9Q},
    {ARCH_ARM_A9A,          ICV_ARM_A9A},
    {ARCH_ARM_A9,           ICV_ARM_A9},
    {ARCH_ARM_A7,           ICV_ARM_A7},
    {ARCH_ARM_A5,           ICV_ARM_A5},
    {ARCH_ARM_A15,          ICV_ARM_A15},
    {ARCH_ARM_NEONINTR,     ICV_ARM_NEONINTR},
    {ARCH_ARMV8_GENERIC,    ICV_ARMV8_GENERIC},
    {ARCH_X86_GENERIC,      ICV_X86_GENERIC},
    {ARCH_X86_SSSE3,        ICV_X86_SSSE3},
    {ARCH_X86_SSE42,        ICV_X86_SSE42},
    {ARCH_X86_AVX2,         ICV_X86_AVX2},
    {ARCH_MIPS_GENERIC,     ICV_MIPS_GENERIC},
    {ARCH_MIPS_32,          ICV_MIPS_32}
};


static void impeg2d_get_pic(icv_pic_t *ps_dst,
                            UWORD8 *pu1_buf_y,
                            UWORD8 *pu1_buf_u,
                            UWORD8 *pu1_buf_v,
                            WORD32 wd,
                            WORD32 ht,
                            WORD32 strd)
{
    ps_dst->ai4_wd[0] = wd;
    ps_dst->ai4_wd[1] = wd / 2;
    ps_dst->ai4_wd[2] = wd / 2;

    ps_dst->ai4_ht[0] = ht;
    ps_dst->ai4_ht[1] = ht / 2;
    ps_dst->ai4_ht[2] = ht / 2;

    ps_dst->ai4_strd[0] = strd;
    ps_dst->ai4_strd[1] = strd / 2;
    ps_dst->ai4_strd[2] = strd / 2;

    ps_dst->apu1_buf[0] = pu1_buf_y;
    ps_dst->apu1_buf[1] = pu1_buf_u;
    ps_dst->apu1_buf[2] = pu1_buf_v;

    ps_dst->e_color_fmt = ICV_YUV420P;
}
static void impeg2d_get_flds(icv_pic_t *ps_frm,
                             icv_pic_t *ps_top_fld,
                             icv_pic_t *ps_bot_fld)
{
    ps_top_fld->ai4_wd[0] = ps_frm->ai4_wd[0];
    ps_top_fld->ai4_wd[1] = ps_frm->ai4_wd[1];
    ps_top_fld->ai4_wd[2] = ps_frm->ai4_wd[2];

    ps_top_fld->ai4_ht[0] = ps_frm->ai4_ht[0] / 2;
    ps_top_fld->ai4_ht[1] = ps_frm->ai4_ht[1] / 2;
    ps_top_fld->ai4_ht[2] = ps_frm->ai4_ht[2] / 2;

    ps_top_fld->ai4_strd[0] = ps_frm->ai4_strd[0] * 2;
    ps_top_fld->ai4_strd[1] = ps_frm->ai4_strd[1] * 2;
    ps_top_fld->ai4_strd[2] = ps_frm->ai4_strd[2] * 2;

    ps_top_fld->e_color_fmt = ps_frm->e_color_fmt;

    /* Copy top field structure to bottom field, since properties of both fields are same */
    *ps_bot_fld = *ps_top_fld;

    /* Initialize the addresses for top field */
    ps_top_fld->apu1_buf[0] = ps_frm->apu1_buf[0];
    ps_top_fld->apu1_buf[1] = ps_frm->apu1_buf[1];
    ps_top_fld->apu1_buf[2] = ps_frm->apu1_buf[2];

    /* Initialize the addresses for bottom field */
    ps_bot_fld->apu1_buf[0] = ps_frm->apu1_buf[0] + ps_frm->ai4_strd[0];
    ps_bot_fld->apu1_buf[1] = ps_frm->apu1_buf[1] + ps_frm->ai4_strd[1];
    ps_bot_fld->apu1_buf[2] = ps_frm->apu1_buf[2] + ps_frm->ai4_strd[2];

    return;
}


static ICV_ARCH_T impeg2d_get_arch(IVD_ARCH_T e_arch)
{
    ICV_ARCH_T ret_arch;
    WORD32 num_entries, i;

    ret_arch = ICV_ARM_A9;
    num_entries = sizeof(gas_impeg2d_arch_mapping) / sizeof(gas_impeg2d_arch_mapping[0]);
    for(i = 0; i < num_entries; i++)
    {
        if(e_arch == gas_impeg2d_arch_mapping[i].ivd_arch)
        {
            ret_arch = gas_impeg2d_arch_mapping[i].icv_arch;
            break;
        }
    }
    return ret_arch;
}

/******************************************************************************
*  Function Name   : impeg2d_deinterlace
*
*  Description     : Deinterlace current picture
*
*  Arguments       :
*  dec             : Decoder Context
*
*  Values Returned : 0 on success, -1 on error
******************************************************************************/
WORD32 impeg2d_deint_ctxt_size(void)
{
    return ideint_ctxt_size();
}

/******************************************************************************
*  Function Name   : impeg2d_deinterlace
*
*  Description     : Deinterlace current picture
*
*  Arguments       :
*  dec             : Decoder Context
*
*  Values Returned : 0 on success, -1 on error
******************************************************************************/
WORD32 impeg2d_deinterlace(dec_state_t *ps_dec,
                           pic_buf_t *ps_src_pic,
                           iv_yuv_buf_t *ps_disp_frm_buf,
                           WORD32 start_row,
                           WORD32 num_rows)
{
    icv_pic_t as_inp_flds[3];
    IDEINT_ERROR_T ret;
    icv_pic_t s_src_frm;
    icv_pic_t s_dst_frm;
    UWORD8 *pu1_dst_y, *pu1_dst_u, *pu1_dst_v;
    ideint_params_t s_params;

    if((NULL == ps_src_pic) || (NULL == ps_src_pic->pu1_y) || (0 == num_rows))
        return -1;

    s_params.e_arch = impeg2d_get_arch(ps_dec->e_processor_arch);
    s_params.e_soc = ICV_SOC_GENERIC;
    s_params.e_mode = IDEINT_MODE_SPATIAL;
    s_params.i4_cur_fld_top = ps_dec->u2_top_field_first;
    s_params.i4_disable_weave = 0;
    s_params.pf_aligned_alloc = NULL;
    s_params.pf_aligned_free = NULL;

    impeg2d_get_pic(&s_src_frm, ps_src_pic->pu1_y, ps_src_pic->pu1_u,
                    ps_src_pic->pu1_v, ps_dec->u2_horizontal_size,
                    ps_dec->u2_vertical_size, ps_dec->u2_frame_width);
    impeg2d_get_flds(&s_src_frm, &as_inp_flds[1], &as_inp_flds[2]);

    if(ps_dec->ps_deint_pic)
    {
        icv_pic_t s_prv_frm;
        icv_pic_t s_fld;
        impeg2d_get_pic(&s_prv_frm, ps_dec->ps_deint_pic->pu1_y,
                        ps_dec->ps_deint_pic->pu1_u,
                        ps_dec->ps_deint_pic->pu1_v, ps_dec->u2_horizontal_size,
                        ps_dec->u2_vertical_size, ps_dec->u2_frame_width);
        impeg2d_get_flds(&s_prv_frm, &s_fld, &as_inp_flds[0]);
    }
    else
    {
        as_inp_flds[0].apu1_buf[0] = NULL;
        as_inp_flds[0].apu1_buf[1] = NULL;
        as_inp_flds[0].apu1_buf[2] = NULL;
    }

    pu1_dst_y = ps_disp_frm_buf->pv_y_buf;
    pu1_dst_u = ps_disp_frm_buf->pv_u_buf;
    pu1_dst_v = ps_disp_frm_buf->pv_v_buf;

    /* Use intermediate buffer as output to deinterlacer,
     * if color format is not 420P
     */
    if(IV_YUV_420P != ps_dec->i4_chromaFormat)
    {
        UWORD8 *pu1_buf_y;
        UWORD8 *pu1_buf_u;
        UWORD8 *pu1_buf_v;
        WORD32 wd = ALIGN16(ps_dec->u2_horizontal_size);
        WORD32 ht = ALIGN16(ps_dec->u2_vertical_size);

        pu1_buf_y = ps_dec->pu1_deint_fmt_buf;
        pu1_buf_u = pu1_buf_y + wd * ht;
        pu1_buf_v = pu1_buf_u + wd * ht / 4;

        pu1_dst_u = pu1_buf_u;
        pu1_dst_v = pu1_buf_v;

        if((ps_dec->i4_chromaFormat != IV_YUV_420SP_UV) &&
           (ps_dec->i4_chromaFormat != IV_YUV_420SP_VU))
        {
            pu1_dst_y = pu1_buf_y;
        }

    }
    impeg2d_get_pic(&s_dst_frm, pu1_dst_y, pu1_dst_u, pu1_dst_v,
                    ps_dec->u2_horizontal_size, ps_dec->u2_vertical_size,
                    ps_dec->u4_frm_buf_stride);


    ret = ideint_process(ps_dec->pv_deinterlacer_ctxt, &as_inp_flds[0],
                         &as_inp_flds[1], &as_inp_flds[2], &s_dst_frm,
                         &s_params, start_row, num_rows);

    if(IDEINT_ERROR_NONE != ret)
    {
        return -1;
    }

    /* Format convert deinterlacer output if required*/
    if(IV_YUV_420P != ps_dec->i4_chromaFormat)
    {
        pic_buf_t s_src_pic;

        s_src_pic = *ps_src_pic;
        s_src_pic.pu1_y = pu1_dst_y;
        s_src_pic.pu1_u = pu1_dst_u;
        s_src_pic.pu1_v = pu1_dst_v;

        impeg2d_format_convert(ps_dec,
                               &s_src_pic,
                               ps_disp_frm_buf,
                               start_row,
                               num_rows);

    }
    return 0;

}
