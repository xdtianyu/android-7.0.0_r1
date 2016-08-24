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
#ifndef __IMPEG2_PLATFORM_MACROS_H__
#define __IMPEG2_PLATFORM_MACROS_H__

#define CONV_LE_TO_BE(u4_temp2,u4_temp1) u4_temp2 =                            \
                                         (u4_temp1 << 24) |                    \
                                         ((u4_temp1 & 0xff00) << 8) |          \
                                         ((u4_temp1 & 0xff0000) >> 8) |        \
                                         (u4_temp1 >> 24);

static __inline  UWORD32 CLZ(UWORD32 u4_word)
{
    if(u4_word)
        return (__builtin_clz(u4_word));
    else
        return 32;
}

#define CLIP_U8(x) ((x) > 255) ? (255) : (((x) < 0) ? (0) : (x))
#define CLIP_S8(x) ((x) > 127) ? (127) : (((x) < -128) ? (-128) : (x))

#define CLIP_U12(x) ((x) > 4095) ? (4095) : (((x) < 0) ? (0) : (x))
#define CLIP_S12(x) ((x) > 2047) ? (2047) : (((x) < -2048) ? (-2048) : (x))

#define CLIP_U16(x) ((x) > 65535) ? (65535) : (((x) < 0) ? (0) : (x))
#define CLIP_S16(x) ((x) > 32767) ? (32767) : (((x) < -32768) ? (-32768) : (x))

#define INLINE
#define PLD(x) __pld(x)

#endif /* __IMPEG2_PLATFORM_MACROS_H__ */
