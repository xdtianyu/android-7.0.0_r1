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
******************************************************************************
* @file ih264e_distortion_metrics.h
*
* @brief
*  This file contains declarations of routines that compute distortion
*  between two macro/sub blocks of identical dimensions
*
* @author
*  Ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IME_DISTORTION_METRICS_H_
#define IME_DISTORTION_METRICS_H_


/*****************************************************************************/
/* Type definitions for function prototypes                                  */
/*****************************************************************************/

typedef void ime_compute_sad_ft(UWORD8 *pu1_src,
                                UWORD8 *pu1_est,
                                WORD32 src_strd,
                                WORD32 est_strd,
                                WORD32 i4_max_sad,
                                WORD32 *pi4_mb_distortion);

typedef void ime_compute_sad4_diamond(UWORD8 *pu1_ref,
                                      UWORD8 *pu1_src,
                                      WORD32 ref_strd,
                                      WORD32 src_strd,
                                      WORD32 *pi4_sad);

typedef void ime_compute_sad3_diamond(UWORD8 *pu1_ref1,
                                      UWORD8 *pu1_ref2,
                                      UWORD8 *pu1_ref3,
                                      UWORD8 *pu1_src,
                                      WORD32 ref_strd,
                                      WORD32 src_strd,
                                      WORD32 *pi4_sad);

typedef void ime_compute_sad2_diamond(UWORD8 *pu1_ref1,
                                      UWORD8 *pu1_ref2,
                                      UWORD8 *pu1_src,
                                      WORD32 ref_strd,
                                      WORD32 src_strd,
                                      WORD32 *pi4_sad);

typedef void ime_sub_pel_compute_sad_16x16_ft(UWORD8 *pu1_src,
                                              UWORD8 *pu1_ref_half_x,
                                              UWORD8 *pu1_ref_half_y,
                                              UWORD8 *pu1_ref_half_xy,
                                              WORD32 src_strd,
                                              WORD32 ref_strd,
                                              WORD32 *pi4_sad);

typedef void ime_compute_sad_stat(UWORD8 *pu1_src,
                                  UWORD8 *pu1_est,
                                  WORD32 src_strd,
                                  WORD32 est_strd,
                                  UWORD16 *pu2_thrsh,
                                  WORD32 *pi4_mb_distortion,
                                  UWORD32 *pu4_is_zero);

typedef void ime_compute_satqd_16x16_lumainter_ft(UWORD8 *pu1_src,
                                         UWORD8 *pu1_est,
                                         WORD32 src_strd,
                                         WORD32 est_strd,
                                         UWORD16 *pu2_thrsh,
                                         WORD32 *pi4_mb_distortion,
                                         UWORD32 *pu4_is_zero);

typedef void ime_compute_satqd_8x16_chroma_ft(UWORD8 *pu1_src,
                                     UWORD8 *pu1_est,
                                     WORD32 src_strd,
                                     WORD32 est_strd,
                                     WORD32 i4_max_sad,
                                     UWORD16 *thrsh);

typedef void ime_compute_satqd_16x16_lumaintra_ft(UWORD8 *pu1_src,
                                         UWORD8 *pu1_est,
                                         WORD32 src_strd,
                                         WORD32 est_strd,
                                         WORD32 i4_max_sad,
                                         UWORD16 *thrsh,
                                         WORD32 *pi4_mb_distortion,
                                         UWORD8 *sig_nz_sad);

/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/

ime_compute_sad_ft ime_compute_sad_16x16;
ime_compute_sad_ft ime_compute_sad_16x16_fast;
ime_compute_sad_ft ime_compute_sad_16x8;
ime_compute_sad_ft ime_compute_sad_16x16_ea8;
ime_compute_sad_ft ime_compute_sad_8x8;
ime_compute_sad_ft ime_compute_sad_4x4;
ime_compute_sad4_diamond ime_calculate_sad4_prog;
ime_compute_sad3_diamond ime_calculate_sad3_prog;
ime_compute_sad2_diamond ime_calculate_sad2_prog;
ime_sub_pel_compute_sad_16x16_ft ime_sub_pel_compute_sad_16x16;
ime_compute_sad_stat ime_compute_16x16_sad_stat;
ime_compute_satqd_16x16_lumainter_ft ime_compute_satqd_16x16_lumainter;
ime_compute_satqd_8x16_chroma_ft ime_compute_satqd_8x16_chroma;
ime_compute_satqd_16x16_lumaintra_ft ime_compute_satqd_16x16_lumaintra;


/*SSE4.2 Declarations*/
ime_compute_sad_ft ime_compute_sad_16x16_sse42;
ime_compute_sad_ft ime_compute_sad_16x16_fast_sse42;
ime_compute_sad_ft ime_compute_sad_16x8_sse42;
ime_compute_sad_ft ime_compute_sad_16x16_ea8_sse42;
ime_sub_pel_compute_sad_16x16_ft ime_sub_pel_compute_sad_16x16_sse42;
ime_compute_sad4_diamond ime_calculate_sad4_prog_sse42;
ime_compute_satqd_16x16_lumainter_ft ime_compute_satqd_16x16_lumainter_sse42;

/* assembly */
ime_compute_sad_ft ime_compute_sad_16x16_a9q;
ime_compute_sad_ft ime_compute_sad_16x16_fast_a9q;
ime_compute_sad_ft ime_compute_sad_16x8_a9q;
ime_compute_sad_ft ime_compute_sad_16x16_ea8_a9q;
ime_compute_sad4_diamond ime_calculate_sad4_prog_a9q;
ime_compute_sad3_diamond ime_calculate_sad3_prog_a9q;
ime_compute_sad2_diamond ime_calculate_sad2_prog_a9q;
ime_sub_pel_compute_sad_16x16_ft ime_sub_pel_compute_sad_16x16_a9q;
ime_compute_sad_stat ime_compute_16x16_sad_stat_a9;
ime_compute_satqd_16x16_lumainter_ft ime_compute_satqd_16x16_lumainter_a9q;


/* assembly - AV8 declarations */
ime_compute_sad_ft ime_compute_sad_16x16_av8;
ime_compute_sad_ft ime_compute_sad_16x16_fast_av8;
ime_compute_sad_ft ime_compute_sad_16x8_av8;
ime_compute_sad_ft ime_compute_sad_16x16_ea8_av8;
ime_compute_sad4_diamond ime_calculate_sad4_prog_av8;
ime_compute_sad3_diamond ime_calculate_sad3_prog_av8;
ime_compute_sad2_diamond ime_calculate_sad2_prog_av8;
ime_sub_pel_compute_sad_16x16_ft ime_sub_pel_compute_sad_16x16_av8;
ime_compute_sad_stat ime_compute_16x16_sad_stat_av8;
ime_compute_satqd_16x16_lumainter_ft ime_compute_satqd_16x16_lumainter_av8;

#endif /* IME_DISTORTION_METRICS_H_ */


