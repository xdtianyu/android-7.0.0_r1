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
#ifndef __IMPEG2D_PIC_PROC_H__
#define __IMPEG2D_PIC_PROC_H__

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/
UWORD16 impeg2d_get_mb_addr_incr(stream_t *stream);
IMPEG2D_ERROR_CODES_T impeg2d_init_video_state(dec_state_t *dec, e_video_type_t videoType);
IMPEG2D_ERROR_CODES_T impeg2d_pre_pic_dec_proc(dec_state_t *dec);
void impeg2d_post_pic_dec_proc(dec_state_t *dec);
IMPEG2D_ERROR_CODES_T impeg2d_dec_i_slice(dec_state_t *dec);
IMPEG2D_ERROR_CODES_T impeg2d_dec_d_slice(dec_state_t *dec);
IMPEG2D_ERROR_CODES_T impeg2d_dec_p_b_slice(dec_state_t *dec);

void impeg2d_format_convert(dec_state_t *ps_dec,
                            pic_buf_t *ps_src_pic,
                            iv_yuv_buf_t    *ps_disp_frm_buf,
                            UWORD32 u4_start_row, UWORD32 u4_num_rows);


#endif /* __IMPEG2D_PIC_PROC_H__  */

