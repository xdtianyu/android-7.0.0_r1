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
*  impeg2_disp_mgr.h
*
* @brief
*  Function declarations used for display management
*
* @author
*  Srinivas T
*
*
* @remarks
*  None
*
*******************************************************************************
*/
#ifndef _IMPEG2_DISP_MGR_H_
#define _IMPEG2_DISP_MGR_H_

#define DISP_MGR_MAX_CNT 64
#define DEFAULT_POC 0x7FFFFFFF

typedef struct
{
    /**
     * apv_ptr[DISP_MGR_MAX_CNT]
     */
    void    *apv_ptr[DISP_MGR_MAX_CNT];

    WORD32   ai4_buf_id[DISP_MGR_MAX_CNT];

    WORD32  i4_wr_idx;

    WORD32  i4_rd_idx;
}disp_mgr_t;

void impeg2_disp_mgr_init(
                disp_mgr_t *ps_disp_mgr);

WORD32 impeg2_disp_mgr_add(
                disp_mgr_t *ps_disp_mgr,
                void *pv_ptr,
                WORD32 i4_buf_id);

void* impeg2_disp_mgr_get(disp_mgr_t *ps_disp_mgr, WORD32 *pi4_buf_id);

#endif  //_IMPEG2_DISP_MGR_H_
