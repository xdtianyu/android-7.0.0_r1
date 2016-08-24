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
#ifndef _IME_STATISTICS_H_
#define _IME_STATISTICS_H_
#define DEBUG_HISTOGRAM_ENABLE 0
#define SAD_EXIT_STATS 0


#if SAD_EXIT_STATS

/**
******************************************************************************
* @brief  While computing sad, if we want to do a early exit, how often we
* should check if the sad computed till now has exceeded min sad param is
* chosen statistically.
* ******************************************************************************
*/
extern UWORD32 gu4_16x16_sad_ee_stats[16+1];
extern UWORD32 gu4_16x8_sad_ee_stats[8+1];

/**
******************************************************************************
*  @brief print sad early exit stats
******************************************************************************
*/
extern void print_sad_ee_stats(void);

#define GATHER_16x16_SAD_EE_STATS(gu4_16x16_sad_ee_stats, i) \
                gu4_16x16_sad_ee_stats[i]++;
#define GATHER_16x8_SAD_EE_STATS(gu4_16x8_sad_ee_stats, i) \
                gu4_16x8_sad_ee_stats[i]++;

#else

#define GATHER_16x16_SAD_EE_STATS(gu4_16x16_sad_ee_stats, i)
#define GATHER_16x8_SAD_EE_STATS(gu4_16x8_sad_ee_stats, i)

#endif


#if DEBUG_HISTOGRAM_ENABLE
#define DEBUG_HISTOGRAM_INIT() debug_histogram_init()
#define DEBUG_HISTOGRAM_DUMP(condition) if(condition) debug_histogram_dump()
#define DEBUG_MV_HISTOGRAM_ADD(mv_x, mv_y) debug_mv_histogram_add(mv_x, mv_y)
#define DEBUG_SAD_HISTOGRAM_ADD(sad, level) debug_sad_histogram_add(sad, level)
#else
#define DEBUG_HISTOGRAM_INIT()
#define DEBUG_HISTOGRAM_DUMP(condition)
#define DEBUG_MV_HISTOGRAM_ADD(mv_x, mv_y)
#define DEBUG_SAD_HISTOGRAM_ADD(sad, level)
#endif



#endif /*_IME_STATISTICS_H_*/
