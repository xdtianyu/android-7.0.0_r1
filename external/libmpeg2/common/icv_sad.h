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
*  icv_sad.h
*
* @brief
*  This file contains the functions to compute SAD
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
#ifndef __ICV_SAD_H__
#define __ICV_SAD_H__

typedef WORD32 icv_sad_8x4_t(UWORD8 *pu1_src,
                             UWORD8 *pu1_ref,
                             WORD32 src_strd,
                             WORD32 ref_strd,
                             WORD32 wd,
                             WORD32 ht);

icv_sad_8x4_t icv_sad_8x4;
icv_sad_8x4_t icv_sad_8x4_a9;
icv_sad_8x4_t icv_sad_8x4_av8;
icv_sad_8x4_t icv_sad_8x4_ssse3;
#endif  /* __ICV_SAD_H__ */
