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
*  ideint_debug.h
*
* @brief
*  Contains debug macros
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

#ifndef __IDEINT_DEBUG_H__
#define __IDEINT_DEBUG_H__


#ifdef CORRUPT_PIC
void ideint_corrupt_pic(icv_pic_t *ps_pic, WORD32 val);
#define IDEINT_CORRUPT_PIC(ps_pic, val) ideint_corrupt_pic(ps_pic, val);
#else
#define IDEINT_CORRUPT_PIC(ps_pic, val)
#endif

#ifdef PROFILE_DIS_SAD
#define PROFILE_DISABLE_SAD if(0)
#else
#define PROFILE_DISABLE_SAD
#endif

#ifdef PROFILE_DIS_VARIANCE
#define PROFILE_DISABLE_VARIANCE if(0)
#else
#define PROFILE_DISABLE_VARIANCE
#endif

#ifdef PROFILE_DIS_CAC
#define PROFILE_DISABLE_CAC if(0)
#else
#define PROFILE_DISABLE_CAC
#endif


#ifdef PROFILE_DIS_SPATIO_TEMPORAL
#define PROFILE_DISABLE_SPATIO_TEMPORAL if(0)
#else
#define PROFILE_DISABLE_SPATIO_TEMPORAL
#endif

#ifdef PROFILE_DIS_SPATIAL
#define PROFILE_DISABLE_SPATIAL if(0)
#else
#define PROFILE_DISABLE_SPATIAL
#endif

#endif /* __IDEINT_DEBUG_H__ */
