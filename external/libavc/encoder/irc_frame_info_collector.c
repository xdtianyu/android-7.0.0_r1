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

/******************************************************************************/
/* File Includes                                                              */
/******************************************************************************/

/* User include files */
#include "irc_datatypes.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"

void irc_init_frame_info(frame_info_t *frame_info)
{
    WORD32 i;

    for(i = 0; i < MAX_MB_TYPE; i++)
    {
        frame_info->mb_header_bits[i] = 0;
        frame_info->tot_mb_sad[i] = 0;
        frame_info->num_mbs[i] = 0;
        frame_info->qp_sum[i] = 0;
        frame_info->mb_texture_bits[i] = 0;
    }

    frame_info->other_header_bits = 0;
    frame_info->activity_sum = 0;
    frame_info->intra_mb_cost_sum = 0;
}

/******************************************************************************
 * GET Functions: Sending back collected information to the rate control module
 ******************************************************************************/
WORD32 irc_fi_get_total_header_bits(frame_info_t *frame_info)
{
    WORD32 total_header_bits = 0, i;

    for(i = 0; i < MAX_MB_TYPE; i++)
    {
        total_header_bits += frame_info->mb_header_bits[i];
    }
    total_header_bits += frame_info->other_header_bits;

    return (total_header_bits);
}

WORD32 irc_fi_get_total_texture_bits(frame_info_t *frame_info)
{
    WORD32 total_texture_bits = 0, i;

    for(i = 0; i < MAX_MB_TYPE; i++)
    {
        total_texture_bits += frame_info->mb_texture_bits[i];
    }

    return (total_texture_bits);
}

WORD32 irc_fi_get_total_frame_sad(frame_info_t *frame_info)
{
    WORD32 total_sad = 0, i;

    for(i = 0; i < MAX_MB_TYPE; i++)
    {
        total_sad += frame_info->tot_mb_sad[i];
    }

    return (total_sad);
}

WORD32 irc_fi_get_average_qp(frame_info_t *frame_info)
{
    WORD32 i, total_qp = 0, total_mbs = 0;

    for(i = 0; i < MAX_MB_TYPE; i++)
    {
        total_qp += frame_info->qp_sum[i];
        total_mbs += frame_info->num_mbs[i];
    }

    if(total_mbs)
    {
        return (total_qp / total_mbs);
    }
    else
    {
        return 0;
    }
}

WORD32 irc_fi_get_avg_mb_header(frame_info_t *frame_info, UWORD8 mb_type)
{
    if(frame_info->num_mbs[mb_type])
    {
        return (frame_info->mb_header_bits[mb_type]
                        / frame_info->num_mbs[mb_type]);
    }
    else
    {
        return 0;
    }
}

WORD32 irc_fi_get_total_mb_texture_bits(frame_info_t *frame_info,
                                        UWORD8 mb_type)
{
    return (frame_info->mb_texture_bits[mb_type]);
}

WORD32 irc_fi_get_total_mb_sad(frame_info_t *frame_info, UWORD8 mb_type)
{
    return (frame_info->tot_mb_sad[mb_type]);
}

WORD32 irc_fi_get_total_mb_qp(frame_info_t *frame_info, UWORD8 mb_type)
{
    if(frame_info->num_mbs[mb_type])
    {
        return (frame_info->qp_sum[mb_type]);
    }
    else
    {
        return 0;
    }
}

WORD32 irc_fi_get_total_mb(frame_info_t *frame_info, UWORD8 mb_type)
{
    return (frame_info->num_mbs[mb_type]);
}

WORD32 irc_fi_get_num_intra_mb(frame_info_t *frame_info)
{
    return (frame_info->num_mbs[MB_TYPE_INTRA]);
}

WORD32 irc_fi_get_avg_activity(frame_info_t *frame_info)
{
    WORD32 i;
    WORD32 i4_tot_mbs = 0;

    for(i = 0; i < MAX_MB_TYPE; i++)
    {
        i4_tot_mbs += frame_info->num_mbs[i];
    }

    if(i4_tot_mbs)
    {
        return (frame_info->activity_sum / i4_tot_mbs);
    }
    else
    {
        return 0;
    }
}

WORD32 irc_fi_get_total_intra_mb_cost(frame_info_t *frame_info)
{
    return (frame_info->intra_mb_cost_sum);
}
