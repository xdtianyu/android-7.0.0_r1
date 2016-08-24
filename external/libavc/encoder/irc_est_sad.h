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

#ifndef _EST_SAD_H_
#define _EST_SAD_H_

/*
 * "est_sad_t->i4_use_est_intra_sad" Flag to control how the I frame SAD is estimated.
 * If set to zero
 * - it uses the Intra sad calculated by the previous P frame as
 * the estimated sad for the current I frame
 * else
 * - it uses the ratio of P frame sads of the previous two GOPS and
 * scales the I Frame sad with this ratio to estimate the current
 * I frame SAD
 */

/* Estimating the Average SAD for the current picture type is done by:
 * 1) if picture_type is I
 * - Estimated SAD = (n-1)th intra frame interval(ifi) P frame Avg SAD *
 * ( prev I frame SAD / (n-2)nd intra frame interval(ifi) P frame Avg SAD)
 * - if only one IFI is encoded use the previous I frame SAD
 * 2) if picture type is P
 * - Estimate SAD is previous P frame SAD
 * 3) The first P frame in a IFI could use a little better logic to decide the
 * estimated SAD but currently we assume the last coded P frames SAD
 a*/

typedef struct est_sad_t *est_sad_handle;

WORD32 irc_est_sad_num_fill_use_free_memtab(est_sad_handle *est_sad,
                                            itt_memtab_t *ps_memtab,
                                            ITT_FUNC_TYPE_E e_func_type);

void irc_init_est_sad(est_sad_handle est_sad, WORD32 i4_use_est_frame_sad);

UWORD32 irc_get_est_sad(est_sad_handle est_sad, picture_type_e e_pic_type);

void irc_update_actual_sad(est_sad_handle est_sad,
                           UWORD32 u4_actual_sad,
                           picture_type_e e_pic_type);

void irc_update_actual_sad_for_intra(est_sad_handle est_sad,
                                     WORD32 i4_intra_frm_cost);

void irc_reset_est_sad(est_sad_handle ps_est_sad);
#endif
