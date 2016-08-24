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
*  ih264e_statistics.h
*
* @brief
*  Contains macros for generating stats about h264 encoder
*
* @author
*  ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_STATISTICS_H_
#define IH264E_STATISTICS_H_

#if  CAVLC_LEVEL_STATS

/*****************************************************************************/
/* Extern global declarations                                                */
/*****************************************************************************/

/**
 ******************************************************************************
 * @brief  In cavlc encoding, a lut is used for encoding levels. It is not possible
 * to use look up for all possible levels. The extent to which look up is generated
 * is based on the statistics that were collected in the following global variables.
 *
 * gu4_cavlc_level_bin_lt_4 represents the number coefficients with abs(level) < 4
 * gu4_cavlc_level_bin_lt_16 represents the number coefficients with 4 < abs(level) < 16
 * gu4_cavlc_level_bin_lt_32 represents the number coefficients with 16 < abs(level) < 32
 * and so on ...
 * ******************************************************************************
 */
extern UWORD32 gu4_cavlc_level_bin_lt_4;
extern UWORD32 gu4_cavlc_level_bin_lt_16;
extern UWORD32 gu4_cavlc_level_bin_lt_32;
extern UWORD32 gu4_cavlc_level_bin_lt_64;
extern UWORD32 gu4_cavlc_level_bin_lt_128;
extern UWORD32 gu4_cavlc_level_bin_else_where;
extern UWORD32 gu4_cavlc_level_lut_hit_rate;

/*****************************************************************************/
/* Extern function declarations                                              */
/*****************************************************************************/

/**
******************************************************************************
*  @brief print cavlc stats
******************************************************************************
*/
void print_cavlc_level_stats(void);

#define GATHER_CAVLC_STATS1() \
    if (u4_abs_level < 4)\
        gu4_cavlc_level_bin_lt_4 ++; \
    else if  (u4_abs_level < 16) \
        gu4_cavlc_level_bin_lt_16 ++; \
    else if  (u4_abs_level < 32) \
        gu4_cavlc_level_bin_lt_32 ++; \
    else if  (u4_abs_level < 64) \
        gu4_cavlc_level_bin_lt_64 ++; \
    else if  (u4_abs_level < 128) \
        gu4_cavlc_level_bin_lt_128 ++; \
    else \
        gu4_cavlc_level_bin_else_where ++;

#define GATHER_CAVLC_STATS2() \
                gu4_cavlc_level_lut_hit_rate ++;

#else

#define GATHER_CAVLC_STATS1()

#define GATHER_CAVLC_STATS2()

#endif


#if  GATING_STATS

/*****************************************************************************/
/* Extern global declarations                                                */
/*****************************************************************************/

/**
******************************************************************************
* @brief  During encoding at fastest preset, some times if the inter threshold
* is lesser than the predefined threshold, intra analysis is not done. The
* below variable keeps track of the number of mb for which intra analysis is not
* done
* ******************************************************************************
*/
extern UWORD32 gu4_mb_gated_cnt;

/*****************************************************************************/
/* Extern function declarations                                              */
/*****************************************************************************/

/**
******************************************************************************
*  @brief print gating stats
******************************************************************************
*/
void print_gating_stats(void);

#define GATHER_GATING_STATS() \
        gu4_mb_gated_cnt ++;

#else

#define GATHER_GATING_STATS()

#endif


#endif /* IH264E_STATISTICS_H_ */
