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
#ifndef __IMPEG2_INTER_PRED_H__
#define __IMPEG2_INTER_PRED_H__


typedef struct
{
    UWORD8 *pu1_y;
    UWORD8 *pu1_u;
    UWORD8 *pu1_v;
}yuv_buf_t;

typedef struct
{
    WORD16 *pi2_y;
    WORD16 *pi2_u;
    WORD16 *pi2_v;
}yuv_buf16_t;

/**
 * Picture buffer
 */
typedef struct
{
    UWORD8 *pu1_y;
    UWORD8 *pu1_u;
    UWORD8 *pu1_v;

    /** Used to store display Timestamp for current buffer */
    WORD32 u4_ts;
    UWORD8 u1_used_as_ref;

    /**
     * buffer ID from buffer manager
     */
    WORD32 i4_buf_id;

    /* To store the buffer's picture type */
    e_pic_type_t e_pic_type;

}pic_buf_t;

typedef void pf_copy_mb_t (yuv_buf_t *src_buf,
                   yuv_buf_t *dst_buf,
                   UWORD32 src_wd,
                   UWORD32 dst_wd);

typedef void pf_interpred_t(UWORD8 *out,UWORD8 *ref, UWORD32 ref_wid,  UWORD32 out_wid);

typedef void pf_interpolate_t(yuv_buf_t *buf_src1,
                              yuv_buf_t *buf_src2,
                              yuv_buf_t *buf_dst,
                              UWORD32 stride);

pf_interpolate_t impeg2_interpolate;
pf_interpolate_t impeg2_interpolate_a9q;
pf_interpolate_t impeg2_interpolate_av8;

pf_copy_mb_t impeg2_copy_mb;
pf_copy_mb_t impeg2_copy_mb_a9q;
pf_copy_mb_t impeg2_copy_mb_av8;

pf_interpred_t impeg2_mc_halfx_halfy_8x8;
pf_interpred_t impeg2_mc_halfx_fully_8x8;
pf_interpred_t impeg2_mc_fullx_halfy_8x8;
pf_interpred_t impeg2_mc_fullx_fully_8x8;

pf_interpred_t impeg2_mc_halfx_halfy_8x8_a9q;
pf_interpred_t impeg2_mc_halfx_fully_8x8_a9q;
pf_interpred_t impeg2_mc_fullx_halfy_8x8_a9q;
pf_interpred_t impeg2_mc_fullx_fully_8x8_a9q;

/* AV8 Declarations */
pf_interpred_t impeg2_mc_halfx_halfy_8x8_av8;
pf_interpred_t impeg2_mc_halfx_fully_8x8_av8;
pf_interpred_t impeg2_mc_fullx_halfy_8x8_av8;
pf_interpred_t impeg2_mc_fullx_fully_8x8_av8;


/* SSE4.2 Declarations*/
pf_copy_mb_t impeg2_copy_mb_sse42;
pf_interpolate_t impeg2_interpolate_sse42;
pf_interpred_t impeg2_mc_halfx_halfy_8x8_sse42;
pf_interpred_t impeg2_mc_halfx_fully_8x8_sse42;
pf_interpred_t impeg2_mc_fullx_halfy_8x8_sse42;
pf_interpred_t impeg2_mc_fullx_fully_8x8_sse42;

#endif /* #ifndef __IMPEG2_INTER_PRED_H__  */
