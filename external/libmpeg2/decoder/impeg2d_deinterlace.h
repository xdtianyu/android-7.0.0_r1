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
#ifndef __IMPEG2D_DEINTERLACE_H__
#define __IMPEG2D_DEINTERLACE_H__

WORD32 impeg2d_deint_ctxt_size(void);
WORD32 impeg2d_deinterlace(dec_state_t *ps_dec,
                           pic_buf_t *ps_src_pic,
                           iv_yuv_buf_t *ps_disp_frm_buf,
                           WORD32 start_row,
                           WORD32 num_rows);

#endif /* __IMPEG2D_DEINTERLACE_H__ */
