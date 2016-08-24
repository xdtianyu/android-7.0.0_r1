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
*  ih264e_globals.c
*
* @brief
*  Contains definitions of global variables used across the encoder
*
* @author
*  ittiam
*
* @par List of functions
*
*
* @remarks
*
*******************************************************************************
*/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* User include files */
#include "ih264_typedefs.h"
#include "ih264_defs.h"
#include "ih264e_defs.h"
#include "ih264e_globals.h"

/*****************************************************************************/
/* Extern global definitions                                                 */
/*****************************************************************************/

/**
******************************************************************************
* @brief  lamda for varying quantizer scales that would be used to
* compute the RD cost while deciding on the MB modes.
* input  : qp
* output : lambda
* @remarks lambda = 0.85 * pow(2, (qp - 12)/3), when SSD is used as metric
* for computing distortion (Bit rate estimation for cost function of H.264/
* AVC by Mohd Golam Sarwer et. al.)  If the use of distortion metric is SAD
* rather than SSD in the stage of encoding, consider sqrt(lambda) simply to
* adjust lambda for the lack of squaring operation in the error computation
* (from rate distortion optimization for video compression by sullivan).
******************************************************************************
*/
const UWORD16 gu2_qp_lambda[52]=
{
       0,      0,      0,      0,      0,      0,      0,      1,
       1,      1,      1,      1,      1,      1,      1,      1,
       1,      2,      2,      2,      2,      3,      3,      3,
       4,      4,      5,      5,      6,      7,      7,      8,
       9,     10,     12,     13,     15,     17,     19,     21,
      23,     26,     30,     33,     37,     42,     47,     53,
      59,     66,     74,     83,
};

/**
******************************************************************************
* @brief  Lamda for varying quantizer scales that would be used to
* compute the RD cost while deciding on the MB modes.
* input  : qp
* output : lambda
* @remarks lambda = pow(2, (qp - 12)/6)
******************************************************************************
*/
const UWORD8 gu1_qp0[52]=
{
       0,      0,      0,      0,      0,      0,      0,      0,
       0,      0,      0,      0,      1,      1,      1,      1,
       2,      2,      2,      2,      3,      3,      3,      4,
       4,      4,      5,      6,      6,      7,      8,      9,
      10,     11,     13,     14,     16,     18,     20,     23,
      25,     29,     32,     36,     40,     45,     51,     57,
      64,     72,     81,     91,
};

/**
******************************************************************************
* @brief  unsigned exp. goulumb codelengths to assign cost to a coefficient of
* mb types.
* input  : Integer
* output : codelength
* @remarks Refer sec. 9-1 in h264 specification
******************************************************************************
*/
const UWORD8 u1_uev_codelength[32] =
{
     1,      3,      3,      5,      5,      5,      5,      7,
     7,      7,      7,      7,      7,      7,      7,      9,
     9,      9,      9,      9,      9,      9,      9,      9,
     9,      9,      9,      9,      9,      9,      9,      11,
};


/**
******************************************************************************
* @brief  Look up table to assign cost to a coefficient of a residual block
* basing on its surrounding coefficients
* input  : Numbers of T1's
* output : coeff_cost
* @remarks Refer Section 2.3 Elimination of single coefficients in inter
* macroblocks in document JVT-O079
******************************************************************************
*/
const UWORD8 gu1_coeff_cost[6] =
{
     3, 2, 2, 1, 1, 1
};

/**
******************************************************************************
* @brief  Indices map to raster scan for luma 4x4 block
* input  : scan index
* output : scan location
* @remarks None
******************************************************************************
*/
const UWORD8 gu1_luma_scan_order[16] =
{
     0,  1,  4,  8,  5,  2,  3,  6,  9,  12, 13, 10, 7,  11, 14, 15
};

/**
******************************************************************************
* @brief  Indices map to raster scan for chroma AC block
* input  : scan index
* output : scan location
* @remarks None
******************************************************************************
*/
const UWORD8 gu1_chroma_scan_order[15] =
{
     1,  4,  8,  5,  2,  3,  6,  9,  12, 13, 10, 7,  11, 14, 15
};

/**
******************************************************************************
* @brief  Indices map to raster scan for luma 4x4 dc block
* input  : scan index
* output : scan location
* @remarks : None
******************************************************************************
*/
const UWORD8 gu1_luma_scan_order_dc[16] =
{
     0, 1,  4,  8,  5,  2,  3,  6,  9,  12, 13, 10, 7,  11, 14, 15
};

/**
******************************************************************************
* @brief  Indices map to raster scan for chroma 2x2 dc block
* input  : scan index
* output : scan location
* @remarks None
******************************************************************************
*/
const UWORD8 gu1_chroma_scan_order_dc[4] =
{
     0, 1,  2,  3
};

/**
******************************************************************************
* @brief  choice of motion vectors to be used during mv prediction
* input  : formatted reference idx comparison metric
* output : mv prediction has to be median or a simple straight forward selec
* tion from neighbors.
* @remarks If only one of the candidate blocks has a reference frame equal to
    the current block then use the same block as the final predictor. A simple
    look up table to assist this mv prediction condition
******************************************************************************
*/
const WORD8 gi1_mv_pred_condition[8] =
{
     -1,    0,    1,    -1,    2,    -1,    -1,    -1
};


/*******************************************************************************
 * Translation of MPEG QP to H264 QP
 ******************************************************************************/
/*
 * Note : RC library models QP and bits assuming the QP to be MPEG2.
 *        Since MPEG qp varies linearly, when the relationship is computed,
 *        it learns that delta(qp) => delta(bits). Now what we are doing by the
 *        transation of qp is that
 *              QPrc = a + b*2^(QPen)
 *        By not considering the weight matrix in both MPEG and H264 we in effect
 *        only changing the relation to
 *              QPrc = c + d*2^(QPen)
 *        This will only entatil changin the RC model parameters, and this will
 *        not affect rc relation at all
 *
 *
 * We have MPEG qp which varies from 0-228. The quantization factor has a linear
 * relation ship with the size of quantized values
 *
 * We also have H264 Qp, which varies such that for a change in QP of 6 , we
 * double the corresponding scaling factor. Hence the scaling is linear in terms
 * of 2^(QPh/6)
 *
 * Now we want to have translation between QPm and QPh. Hence we can write
 *
 * QPm = a + b*2^(QPh/6)
 *
 * Appling boundary condition that
 *      1) QPm = 0.625 if QPh = 0
 *      2) QPm =   224 if QPh = 51,
 *
 * we will have
 *  a = 0.0063, b = 0.6187
 *
 * Hence the relatiohship is
 *  QPm = a + b*2^(Qph/6)
 *  QPh = 6*log((Qpm - a)/b)
 *
 *
 * Unrounded values for gau1_h264_to_mpeg2_qmap[H264_QP_ELEM] =
 *
 *   0.625       0.70077     0.78581     0.88127     0.98843     1.10870
 *   1.24370     1.39523     1.56533     1.75625     1.97055     2.21110
 *   2.48110     2.78417     3.12435     3.50620     3.93480     4.41589
 *   4.95590     5.56204     6.24241     7.00609     7.86330     8.82548
 *   9.90550     11.11778    12.47851    14.00588    15.72030    17.64467
 *   19.80470    22.22925    24.95072    28.00547    31.43430    35.28304
 *   39.60310    44.45221    49.89514    56.00463    62.86230    70.55978
 *   79.19990    88.89811    99.78398    112.00296   125.71830   141.11325
 *   158.39350   177.78992   199.56167   223.99963
 *
 *
 *
 * Unrounded values for gau1_mpeg2_to_h264_qmap[MPEG2_QP_ELEM]
 *
 *   0         4.1014    10.1288   13.6477   16.1425   18.0768   19.6568
 *   20.9925   22.1493   23.1696   24.0822   24.9078   25.6614   26.3546
 *   26.9964   27.5938   28.1527   28.6777   29.1726   29.6408   30.0850
 *   30.5074   30.9102   31.2951   31.6636   32.0171   32.3567   32.6834
 *   32.9983   33.3021   33.5957   33.8795   34.1544   34.4208   34.6793
 *   34.9303   35.1742   35.4114   35.6423   35.8671   36.0863   36.3001
 *   36.5087   36.7124   36.9115   37.1060   37.2963   37.4825   37.6648
 *   37.8433   38.0182   38.1896   38.3577   38.5226   38.6844   38.8433
 *   38.9993   39.1525   39.3031   39.4511   39.5966   39.7397   39.8804
 *   40.0189   40.1553   40.2895   40.4217   40.5518   40.6801   40.8065
 *   40.9310   41.0538   41.1749   41.2943   41.4121   41.5283   41.6430
 *   41.7561   41.8678   41.9781   42.0870   42.1946   42.3008   42.4057
 *   42.5094   42.6118   42.7131   42.8132   42.9121   43.0099   43.1066
 *   43.2023   43.2969   43.3905   43.4831   43.5747   43.6653   43.7550
 *   43.8438   43.9317   44.0187   44.1049   44.1901   44.2746   44.3582
 *   44.4411   44.5231   44.6044   44.6849   44.7647   44.8438   44.9221
 *   44.9998   45.0767   45.1530   45.2286   45.3035   45.3779   45.4515
 *   45.5246   45.5970   45.6689   45.7401   45.8108   45.8809   45.9504
 *   46.0194   46.0878   46.1557   46.2231   46.2899   46.3563   46.4221
 *   46.4874   46.5523   46.6166   46.6805   46.7439   46.8069   46.8694
 *   46.9314   46.9930   47.0542   47.1150   47.1753   47.2352   47.2947
 *   47.3538   47.4125   47.4708   47.5287   47.5862   47.6433   47.7001
 *   47.7565   47.8125   47.8682   47.9235   47.9785   48.0331   48.0874
 *   48.1413   48.1949   48.2482   48.3011   48.3537   48.4060   48.4580
 *   48.5097   48.5611   48.6122   48.6629   48.7134   48.7636   48.8135
 *   48.8631   48.9124   48.9615   49.0102   49.0587   49.1069   49.1549
 *   49.2026   49.2500   49.2972   49.3441   49.3908   49.4372   49.4834
 *   49.5293   49.5750   49.6204   49.6656   49.7106   49.7553   49.7998
 *   49.8441   49.8882   49.9320   49.9756   50.0190   50.0622   50.1051
 *   50.1479   50.1904   50.2327   50.2749   50.3168   50.3585   50.4000
 *   50.4413   50.4825   50.5234   50.5641   50.6047   50.6450   50.6852
 *   50.7252   50.7650   50.8046   50.8440   50.8833   50.9224   50.9613
 *   51.0000
 */

const UWORD8 gau1_h264_to_mpeg2_qmap[H264_QP_ELEM] =
{
     1,    1,    1,    1,   1,    1,    1,   1,
     2,    2,    2,    2,   2,    3,    3,   4,
     4,    4,    5,    6,   6,    7,    8,   9,
     10,   11,   12,   14,  16,   18,   20,  22,
     25,   28,   31,   35,  40,   44,   50,  56,
     63,   71,   79,   89,  100,  112,  126, 141,
     158,  178,  200,  224
};

const UWORD8 gau1_mpeg2_to_h264_qmap[MPEG2_QP_ELEM] =
{
     0,    4,    10,  14,   16,   18,  20,  21,
     22,   23,   24,  25,   26,   26,  27,  28,
     28,   29,   29,  30,   30,   31,  31,  31,
     32,   32,   32,  33,   33,   33,  34,  34,
     34,   34,   35,  35,   35,   35,  36,  36,
     36,   36,   37,  37,   37,   37,  37,  37,
     38,   38,   38,  38,   38,   39,  39,  39,
     39,   39,   39,  39,   40,   40,  40,  40,
     40,   40,   40,  41,   41,   41,  41,  41,
     41,   41,   41,  42,   42,   42,  42,  42,
     42,   42,   42,  42,   43,   43,  43,  43,
     43,   43,   43,  43,   43,   43,  43,  44,
     44,   44,   44,  44,   44,   44,  44,  44,
     44,   44,   45,  45,   45,   45,  45,  45,
     45,   45,   45,  45,   45,   45,  45,  46,
     46,   46,   46,  46,   46,   46,  46,  46,
     46,   46,   46,  46,   46,   46,  47,  47,
     47,   47,   47,  47,   47,   47,  47,  47,
     47,   47,   47,  47,   47,   47,  48,  48,
     48,   48,   48,  48,   48,   48,  48,  48,
     48,   48,   48,  48,   48,   48,  48,  48,
     49,   49,   49,  49,   49,   49,  49,  49,
     49,   49,   49,  49,   49,   49,  49,  49,
     49,   49,   49,  49,   49,   50,  50,  50,
     50,   50,   50,  50,   50,   50,  50,  50,
     50,   50,   50,  50,   50,   50,  50,  50,
     50,   50,   50,  50,   51,   51,  51,  51,
     51,   51,   51,  51,   51,   51,  51,  51,
     51
};

