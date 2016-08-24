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
/* File Includes                                                             */
/*****************************************************************************/

/* System include files */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* User include files */
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264e.h"
#include "app.h"

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/


/*****************************************************************************/
/*  Macros                                                                   */
/*****************************************************************************/


/*****************************************************************************/
/*  Function Definitions                                                     */
/*****************************************************************************/

IV_STATUS_T read_pic_info(app_ctxt_t *ps_app_ctxt, void *pv_pic_info)
{
    IV_STATUS_T ret = IV_SUCCESS;
    WORD32 size, bytes;

    switch(ps_app_ctxt->u4_pic_info_type)
    {
        case 1:
            size = sizeof(ih264e_pic_info1_t);
            ps_app_ctxt->u4_pic_info_size = sizeof(ih264e_pic_info1_t);
            break;
        case 2:
            size = sizeof(ih264e_pic_info2_t);
            ps_app_ctxt->u4_pic_info_size = sizeof(ih264e_pic_info2_t);
            break;
        default:
            size = 0;
            break;
    }

    bytes = fread(pv_pic_info, 1, size, ps_app_ctxt->fp_pic_info);
    if(bytes != size)
        ret = IV_FAIL;

    return ret;
}

IV_STATUS_T read_mb_info(app_ctxt_t *ps_app_ctxt, void *pv_mb_info)
{
    IV_STATUS_T ret = IV_SUCCESS;
    WORD32 num_mbs;
    WORD32 size;
    WORD32 bytes;

    num_mbs = ALIGN16(ps_app_ctxt->u4_wd) *  ALIGN16(ps_app_ctxt->u4_ht);
    num_mbs /= 256;

    switch(ps_app_ctxt->u4_mb_info_type)
    {
        case 1:
            size = sizeof(ih264e_mb_info1_t) * num_mbs;
            ps_app_ctxt->u4_mb_info_size = sizeof(ih264e_mb_info1_t);
            break;
        case 2:
            size = sizeof(ih264e_mb_info2_t) * num_mbs;
            ps_app_ctxt->u4_mb_info_size = sizeof(ih264e_mb_info2_t);
            break;
        case 3:
            size = sizeof(ih264e_mb_info3_t) * num_mbs;
            ps_app_ctxt->u4_mb_info_size = sizeof(ih264e_mb_info3_t);
            break;
        case 4:
            size = sizeof(ih264e_mb_info4_t) * num_mbs;
            ps_app_ctxt->u4_mb_info_size = sizeof(ih264e_mb_info4_t);
            break;
        default:
            size = 0;
            break;
    }

    bytes = fread(pv_mb_info, 1, size, ps_app_ctxt->fp_mb_info);
    if(bytes != size)
        ret = IV_FAIL;

    return ret;
}

IV_STATUS_T read_input(FILE *fp, iv_raw_buf_t *ps_raw_buf)
{
    WORD32 bytes;
    WORD32 wd, ht, strd;
    UWORD8 *pu1_buf;
    WORD32 i;
    WORD32 comp;
    WORD32 num_comp;

    if (IV_YUV_422ILE == ps_raw_buf->e_color_fmt)
    {
        wd = ps_raw_buf->au4_wd[0];
        ht = ps_raw_buf->au4_ht[0];
        strd = ps_raw_buf->au4_strd[0];
        pu1_buf = ps_raw_buf->apv_bufs[0];

        for(i = 0; i < ht; i++)
        {
            bytes = fread(pu1_buf, sizeof(UWORD8), wd, fp);
            if(bytes != wd )
            {
                return(IV_FAIL);
            }
            pu1_buf += strd;
        }
    }
    else
    {
        num_comp = 2;

        if(IV_YUV_420P == ps_raw_buf->e_color_fmt)
            num_comp = 3;

        for(comp = 0; comp < num_comp; comp++)
        {
            wd = ps_raw_buf->au4_wd[comp];
            ht = ps_raw_buf->au4_ht[comp];
            strd = ps_raw_buf->au4_strd[comp];
            pu1_buf = ps_raw_buf->apv_bufs[comp];

            for(i = 0; i < ht; i++)
            {
                bytes = fread(pu1_buf, sizeof(UWORD8), wd, fp);
                if(bytes != wd)
                {
                    return(IV_FAIL);
                }
                pu1_buf += strd;
            }
        }
    }
    return IV_SUCCESS;
}


IV_STATUS_T dump_input(FILE *fp, iv_raw_buf_t *ps_raw_buf)
{
    WORD32 bytes;
    WORD32 wd, ht, strd;
    UWORD8 *pu1_buf;
    WORD32 i;
    WORD32 comp;
    WORD32 num_comp;

    if (IV_YUV_422ILE == ps_raw_buf->e_color_fmt)
    {
        wd = ps_raw_buf->au4_wd[0];
        ht = ps_raw_buf->au4_ht[0];
        strd = ps_raw_buf->au4_strd[0];
        pu1_buf = ps_raw_buf->apv_bufs[0];

        for(i = 0; i < ht; i++)
        {
            bytes = fwrite(pu1_buf, sizeof(UWORD8), wd, fp);
            if(bytes != wd )
            {
                return(IV_FAIL);
            }
            pu1_buf += strd;
        }
    }
    else
    {
        num_comp = 2;

        if(IV_YUV_420P == ps_raw_buf->e_color_fmt)
            num_comp = 3;

        for(comp = 0; comp < num_comp; comp++)
        {
            wd = ps_raw_buf->au4_wd[comp];
            ht = ps_raw_buf->au4_ht[comp];
            strd = ps_raw_buf->au4_strd[comp];
            pu1_buf = ps_raw_buf->apv_bufs[comp];

            for(i = 0; i < ht; i++)
            {
                bytes = fwrite(pu1_buf, sizeof(UWORD8), wd, fp);
                if(bytes != wd)
                {
                    return(IV_FAIL);
                }
                pu1_buf += strd;
            }
        }
    }
    return IV_SUCCESS;
}

void allocate_input(app_ctxt_t *ps_app_ctxt)
{

    WORD32 num_bufs;
    WORD32 pic_size;
    WORD32 luma_size;
    WORD32 chroma_size;
    WORD32 num_mbs;
    WORD32 i;
    UWORD8 *pu1_buf[3];

    ih264e_ctl_getbufinfo_op_t *ps_get_buf_info_op = &ps_app_ctxt->s_get_buf_info_op;

    num_bufs = MAX(DEFAULT_NUM_INPUT_BUFS, ps_get_buf_info_op->s_ive_op.u4_min_inp_bufs);
    num_bufs = MIN(DEFAULT_MAX_INPUT_BUFS, num_bufs);

    /* Size of buffer */
    luma_size = ps_app_ctxt->u4_wd * ps_app_ctxt->u4_ht;
    chroma_size = luma_size >> 1;
    pic_size = luma_size + chroma_size;

    num_mbs = ALIGN16(ps_app_ctxt->u4_max_wd) *  ALIGN16(ps_app_ctxt->u4_max_ht);
    num_mbs /= 256;

    /* Memset the input buffer array to set is_free to 0 */
    memset(ps_app_ctxt->as_input_buf, 0, sizeof(input_buf_t) * DEFAULT_MAX_INPUT_BUFS);

    for(i = 0; i < num_bufs; i++)
    {
        pu1_buf[0] = (UWORD8 *)ih264a_aligned_malloc(16, pic_size);
        if(NULL == pu1_buf[0])
        {
            CHAR ac_error[STRLENGTH];
            sprintf(ac_error, "Allocation failed for input buffer of size %d\n",
                    pic_size);
            codec_exit(ac_error);
        }
        ps_app_ctxt->as_input_buf[i].pu1_buf = pu1_buf[0];

        pu1_buf[0] = (UWORD8 *)ih264a_aligned_malloc(16, num_mbs * sizeof(ih264e_mb_info_t));
        if(NULL == pu1_buf[0])
        {
            CHAR ac_error[STRLENGTH];
            sprintf(ac_error, "Allocation failed for mb info buffer of size %d\n",
                    (WORD32)(num_mbs * sizeof(ih264e_mb_info_t)));
            codec_exit(ac_error);
        }
        ps_app_ctxt->as_input_buf[i].pv_mb_info = pu1_buf[0];
        pu1_buf[0] = (UWORD8 *)ih264a_aligned_malloc(16, sizeof(ih264e_pic_info2_t));
        if(NULL == pu1_buf[0])
        {
            CHAR ac_error[STRLENGTH];
            sprintf(ac_error, "Allocation failed for pic info buffer of size %d\n",
                   (WORD32) sizeof(ih264e_pic_info2_t));
            codec_exit(ac_error);
        }
        ps_app_ctxt->as_input_buf[i].pv_pic_info = pu1_buf[0];
        ps_app_ctxt->as_input_buf[i].u4_buf_size = pic_size;
        ps_app_ctxt->as_input_buf[i].u4_is_free = 1;
    }
    return;
}


void free_input(app_ctxt_t *ps_app_ctxt)
{

    WORD32 num_bufs;
    WORD32 i;

    num_bufs = MAX(DEFAULT_NUM_INPUT_BUFS, ps_app_ctxt->s_get_buf_info_op.s_ive_op.u4_min_inp_bufs);
    num_bufs = MIN(DEFAULT_MAX_INPUT_BUFS, num_bufs);

    for(i = 0; i < num_bufs; i++)
    {
        ih264a_aligned_free(ps_app_ctxt->as_input_buf[i].pu1_buf);
        ih264a_aligned_free(ps_app_ctxt->as_input_buf[i].pv_mb_info);
        ih264a_aligned_free(ps_app_ctxt->as_input_buf[i].pv_pic_info);
    }
    return;
}

