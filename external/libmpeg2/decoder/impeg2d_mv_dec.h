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
#ifndef __IMPEG2D_MV_DEC_H__
#define __IMPEG2D_MV_DEC_H__

e_field_t impeg2d_dec_mv(stream_t *stream, WORD16 predMv[], WORD16 mv[],UWORD16 fCode[],
           UWORD16 shift,UWORD16 fld_sel);
INLINE void impeg2d_dec_1mv(stream_t *stream, WORD16 predMv[], WORD16 mv[],UWORD16 fCode[],
           UWORD16 shift,WORD16 dmv[]);

#endif /* #ifndef __IMPEG2D_MV_DEC_H__  */
