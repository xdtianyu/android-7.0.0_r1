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
*  ih264e_trace_support.h
*
* @brief
*  This file contains extern declarations of routines that could be helpful
*  for debugging purposes.
*
* @author
*  Harish
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef TRACE_SUPPORT_H_
#define TRACE_SUPPORT_H_

/*****************************************************************************/
/* Structures                                                                */
/*****************************************************************************/

typedef struct
{
    WORD8 * pu1_buf;
    WORD32 i4_offset;
    WORD32 i4_max_size;
}trace_support_t;

/*****************************************************************************/
/* Extern function declarations                                              */
/*****************************************************************************/

void init_trace_support(WORD8 *pu1_buf, WORD32 i4_size);

int trace_printf(const WORD8 *format, ...);

#endif // TRACE_SUPPORT_H_
