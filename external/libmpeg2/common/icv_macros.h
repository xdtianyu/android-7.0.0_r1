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
*  icv_macros.h
*
* @brief
*  This header files contains all the common macros
*
* @author
*  Ittiam
*
* @par List of Functions:
*
* @remarks
*  None
*
*******************************************************************************
*/
#ifndef __ICV_MACROS_H__
#define __ICV_MACROS_H__

#define ABS(x) ((x) < 0 ? (-1 * (x)) : (x))

#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define MIN(x,y) ((x) < (y) ? (x) : (y))

/* Absolute difference */
#define ABS_DIF(x,y) (((x) > (y)) ? ((x) - (y)) : ((y) - (x)))

#define MED3(a,b,c)   (MIN(MAX( MIN((a),(b)), (c)), MAX((a),(b))))

#define AVG(a,b)      (((a) + (b) + 1) >> 1)

#define MEAN(a, b)    AVG(a, b)

#define CLIP3(min, max, x) (((x) > (max)) ? (max) :(((x) < (min))? (min):(x)))
#define SIGN(x)    (((x) < 0) ? -1 : 1)


#define ALIGN128(x) ((((x) + 127) >> 7) << 7)
#define ALIGN64(x)  ((((x) + 63) >> 6) << 6)
#define ALIGN32(x)  ((((x) + 31) >> 5) << 5)
#define ALIGN16(x)  ((((x) + 15) >> 4) << 4)
#define ALIGN8(x)   ((((x) + 7) >> 3) << 3)


#define RETURN_IF(cond, retval) if(cond) {return (retval);}
#define UNUSED(x) ((void)(x))

#define ASSERT(x) assert(x)


#endif  /* __ICV_IT_MACROS_H__ */
