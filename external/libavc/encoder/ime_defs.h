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
*  ihevc_typedefs.h
*
* @brief
*  Type definitions used in the code
*
*
* @remarks
*  None
*
*******************************************************************************
*/
#ifndef _IME_DEFS_H_
#define _IME_DEFS_H_


/* Macros to Label candidates */
#define     SKIP_CAND 0
#define     ZERO_CAND 1
#define     LEFT_CAND 2
#define     TOP_CAND  3
#define     TOPR_CAND 4

#define NONE 0
#define LEFT 1
#define RIGHT 2
#define TOP 3
#define BOTTOM 4

#define MB_SIZE 16

#define FULL_SRCH 0
#define DMND_SRCH 100
#define NSTEP_SRCH 50
#define HEX_SRCH 75

#define MAX_NUM_REFLIST 2
#define SUBPEL_BUFF_CNT 4

#endif /*_IME_DEFS_H_*/

