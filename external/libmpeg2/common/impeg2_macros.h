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
#ifndef __IMPEG2_MACROS_H__
#define __IMPEG2_MACROS_H__

#define ABS(x) ((x) < 0 ? (-1 * (x)) : (x))

#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define MIN(x,y) ((x) < (y) ? (x) : (y))

#define CLIP(Number,Max,Min)    if((Number) > (Max)) (Number) = (Max); \
else if((Number) < (Min)) (Number) = (Min)

#define SIGN(Number)    (((Number) < 0) ? -1 : 1)


#define BITS(val,msb,lsb) (UWORD16)((((val) >> (lsb)) & ((1 << ((msb) - (lsb) + 1)) - 1)))

#define BIT(val,bit)      (UWORD16)(((val) >> (bit)) & 0x1)

#define IS_VAL_IN_RANGE(val,upperLimit,lowerLimit) ((val) >= (lowerLimit) && (val) <= (upperLimit))

#define MSW(dword)        (dword >> 16)
#define LSW(dword)        (dword & 0xFFFF)
#define DIV_2_RND(mv) (((mv) + ((mv) > 0)) >> 1)
#define IS_NEG(Number)    (((Number) < 0) ? 1 : 0)

#define ALIGN128(x) ((((x) + 127) >> 7) << 7)
#define ALIGN64(x)  ((((x) + 63) >> 6) << 6)
#define ALIGN32(x)  ((((x) + 31) >> 5) << 5)
#define ALIGN16(x)  ((((x) + 15) >> 4) << 4)
#define ALIGN8(x)   ((((x) + 7) >> 3) << 3)


#define RETURN_IF(cond, retval) if(cond) {return (retval);}
#define UNUSED(x) ((void)(x))


#define ASSERT(x) assert(x)


#endif  /* __IMPEG2_IT_MACROS_H__ */
