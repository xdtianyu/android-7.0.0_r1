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

#ifndef _FRAME_INFO_COLLECTOR_H_
#define _FRAME_INFO_COLLECTOR_H_

typedef struct
{
    /* Number of MBs in each type */
    WORD32 num_mbs[MAX_MB_TYPE];

    /* Sum of all MB SADs of each MB type  */
    WORD32 tot_mb_sad[MAX_MB_TYPE];

    /* Sum of QPs for each mb type */
    WORD32 qp_sum[MAX_MB_TYPE];

    /* Header bits consumed other than MB headers */
    WORD32 other_header_bits;

    /* Header bits consumed for each type of MBs */
    WORD32 mb_header_bits[MAX_MB_TYPE];

    /* Texture bits consumed for each type of MBs */
    WORD32 mb_texture_bits[MAX_MB_TYPE];

    /* Sum of all MB activity */
    WORD32 activity_sum;

    /* Sum of all the Intra MB cost values for the entire frame */
    WORD32 intra_mb_cost_sum;

} frame_info_t;

void irc_init_frame_info(frame_info_t *frame_info);

/*
 * Update functions: Collecting information from encoder
 */
#define FI_UPDATE_OTHER_HEADER_BITS(frame_info,header_bits)\
    {(frame_info)->other_header_bits += (header_bits);}

#define FI_UPDATE_MB_HEADER(frame_info,header_bits,mb_type)\
    {(frame_info)->mb_header_bits[(mb_type)] += (header_bits);}

#define FI_UPDATE_MB_TEXTURE(frame_info,texture_bits,mb_type)\
    {(frame_info)->mb_texture_bits[(mb_type)] += (texture_bits);}

#define FI_UPDATE_MB_SAD(frame_info,mb_sad,mb_type)\
    {(frame_info)->tot_mb_sad[(mb_type)] += (mb_sad);}

#define FI_UPDATE_MB_QP(frame_info,qp,mb_type)\
    {(frame_info)->qp_sum[(mb_type)] += (qp);(frame_info)->num_mbs[(mb_type)]++;}

#define FI_UPDATE_ACTIVITY(frame_info,mb_activity)\
    {(frame_info)->activity_sum += (mb_activity);}

#define FI_UPDATE_INTRA_MB_COST(frame_info,intra_mb_cost)\
    {(frame_info)->intra_mb_cost_sum += (intra_mb_cost);}

/*
 * GET Functions: Sending back collected information to the rate control module
 */

/* Frame Level Model Information */
WORD32 irc_fi_get_total_header_bits(frame_info_t *frame_info);

WORD32 irc_fi_get_total_texture_bits(frame_info_t *frame_info);

WORD32 irc_fi_get_average_qp(frame_info_t *frame_info);

WORD32 irc_fi_get_total_frame_sad(frame_info_t *frame_info);

WORD32 irc_fi_get_avg_activity(frame_info_t *frame_info);

/* Number of Intra MBs for Scene Change Detection */
WORD32 irc_fi_get_num_intra_mb(frame_info_t *frame_info);

/* MB Level Model Information */
WORD32 irc_fi_get_avg_mb_header(frame_info_t *frame_info, UWORD8 mb_type);

WORD32 irc_fi_get_total_mb_texture_bits(frame_info_t *frame_info,
                                        UWORD8 mb_type);

WORD32 irc_fi_get_total_mb_sad(frame_info_t *frame_info, UWORD8 mb_type);

WORD32 irc_fi_get_total_mb_qp(frame_info_t *frame_info, UWORD8 mb_type);

WORD32 irc_fi_get_total_mb(frame_info_t *frame_info, UWORD8 mb_type);

WORD32 irc_fi_get_total_intra_mb_cost(frame_info_t *frame_info);
#endif
