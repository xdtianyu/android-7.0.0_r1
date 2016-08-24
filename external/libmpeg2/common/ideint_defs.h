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
*  ideint_defs.h
*
* @brief
*  Contains deinterlacer definitions
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

#ifndef __IDEINT_DEFS_H__
#define __IDEINT_DEFS_H__

#define ADJ_SAD_THRESH (6 * (FLD_BLK_SIZE * 2)) // *2 : 2 way collapsing (col+row)

#define RSUM_CSUM_THRESH_PER_PEL  5//0

/* Block dimensions. All the decisions (which method to be used) are     */
/* made on block basis. The blk level decisions help us in               */
/* reducing the time-complexity of the algorithm.                        */
#define BLK_WD_SHIFT 3
#define BLK_HT_SHIFT 3

#define BLK_WD     (1 << BLK_WD_SHIFT )
#define BLK_HT     (1 << BLK_HT_SHIFT)

#define FLD_BLK_SIZE   (BLK_WD * (BLK_HT >> 1))


/* Inside the algorithm, the block itself is divided amongst further     */
/* smaller blocks.                                                       */
#define SUB_BLK_WD  (BLK_WD  >> 1)
#define SUB_BLK_HT (BLK_HT >> 1) /* field dimensions. */

#define FLD_SUB_BLK_SIZE (SUB_BLK_WD * SUB_BLK_HT)


/*****************************************************************************/
/* Stationarity check threshold, used in deciding when to weave.             */
/*****************************************************************************/
#define ST_THRESH        ((15 * FLD_BLK_SIZE) >> 1)

#define MOD_IDX_ST_NUM   3
#define MOD_IDX_ST_SHIFT 1

#define VAR_AVG_LUMA   735
#define VAR_AVG_CHROMA  38

/*****************************************************************************/
/* Threshold to choose the fallback method out of Bob and 3-field Kernel     */
/* method.                                                                   */
/*****************************************************************************/
#define FB_THRESH  (32 * FLD_BLK_SIZE)

#define MOD_IDX_FB    4


#define EDGE_BIAS_0  5
#define EDGE_BIAS_1  7

/*****************************************************************************/
/* Adjacent correlation bias, used in biasing the adjacent correlation over  */
/* the alternate one, while comparing the two; in the combing-artifact-check */
/* function.                                                                 */
/*****************************************************************************/
#define SAD_BIAS_ADDITIVE   (FLD_SUB_BLK_SIZE >> 1)

/*****************************************************************************/
/* Mult bias is 1.125 = 9/8. Multiplication by this number is done in two    */
/* stpes, first multiplication by 9 and then shift by 3.                     */
/*****************************************************************************/
#define SAD_BIAS_MULT_SHIFT   3

/*****************************************************************************/
/* row_sum threshold, used for making the combing artifact check more robust */
/* against the noise (e.g. ringing) by rejecting insignificant pixel         */
/* difference across two adjacent rows; in the combing artifact check        */
/* function.                                                                 */
/*****************************************************************************/
#define RSUM_CSUM_THRESH   (RSUM_CSUM_THRESH_PER_PEL * SUB_BLK_WD)

/*****************************************************************************/
/* The 3-field filter is of type [-k 2k -k, 0.5 0.5, -k 2k -k], where k is   */
/* the COEFF_THREE_FIELD defined below.                                      */
/*****************************************************************************/
#define COEFF_THREE_FIELD 13

/*****************************************************************************/
/* Definitions used by the variance calculations module. */
/*****************************************************************************/
#define SQR_SUB_BLK_SZ       (FLD_BLK_SIZE * FLD_BLK_SIZE)
#define SUB_BLK_SZ_SHIFT     5                       /* 2^5  = 32 */
#define SQR_SUB_BLK_SZ_SHIFT (SUB_BLK_SZ_SHIFT << 1) /* 2^10 = 1024 = 32 * 32 */



#endif /* __IDEINT_DEFS_H__ */
