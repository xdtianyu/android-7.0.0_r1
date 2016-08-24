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
*  ime_platform_macros.h
*
* @brief
*  Platform specific Macro definitions used in the codec
*
* @author
*  Ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/


#ifndef _IME_PLATFORM_MACROS_H_
#define _IME_PLATFORM_MACROS_H_

/*****************************************************************************/
/* Function macro definitions                                                */
/*****************************************************************************/

#define USADA8(src,est,sad) \
                sad +=  ABS(src[0]-est[0]) + \
                ABS(src[1]-est[1]) + \
                ABS(src[2]-est[2]) + \
                ABS(src[3]-est[3])


#endif /* _IH264_PLATFORM_MACROS_H_ */
