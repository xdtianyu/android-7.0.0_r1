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

#ifndef _RC_CNTRL_PARAM_H_
#define _RC_CNTRL_PARAM_H_

/* This file should contain only enumerations exported to codec by RC */

/* RC algo type */
typedef enum
{
    VBR_STORAGE = 0,
    VBR_STORAGE_DVD_COMP = 1,
    VBR_STREAMING = 2,
    CONST_QP = 3,
    CBR_LDRC = 4,
    CBR_NLDRC = 5

} rc_type_e;

/* Picture type structure*/
typedef enum
{
    BUF_PIC = -1, I_PIC = 0, P_PIC, B_PIC, MAX_PIC_TYPE

} picture_type_e;

/* MB Type structure*/
typedef enum
{
    /* Based on MB TYPES added the array size increases */
    MB_TYPE_INTRA, MB_TYPE_INTER, MAX_MB_TYPE
} mb_type_e;

typedef enum
{
    VBV_NORMAL, VBV_UNDERFLOW, VBV_OVERFLOW, VBR_CAUTION

} vbv_buf_status_e;

#endif

