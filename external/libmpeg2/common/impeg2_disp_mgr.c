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
*  impeg2_disp_mgr.c
*
* @brief
*  Contains function definitions for display management
*
* @author
*  Srinivas T
*
* @par List of Functions:
*   - impeg2_disp_mgr_init()
*   - impeg2_disp_mgr_add()
*   - impeg2_disp_mgr_get()
*
* @remarks
*  None
*
*******************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include "iv_datatypedef.h"
#include "impeg2_defs.h"
#include "impeg2_disp_mgr.h"

/**
*******************************************************************************
*
* @brief
*    Initialization function for display buffer manager
*
* @par Description:
*    Initializes the display buffer management structure
*
* @param[in] ps_disp_mgr
*  Pointer to the display buffer management structure
*
* @returns none
*
* @remarks
*  None
*
*******************************************************************************
*/
void impeg2_disp_mgr_init(
                disp_mgr_t *ps_disp_mgr)
{
    WORD32 id;


    for(id = 0; id < DISP_MGR_MAX_CNT; id++)
    {
        ps_disp_mgr->apv_ptr[id] = NULL;
    }

    ps_disp_mgr->i4_wr_idx = 0;
    ps_disp_mgr->i4_rd_idx = 0;
}


/**
*******************************************************************************
*
* @brief
*     Adds a buffer to the display manager
*
* @par Description:
*      Adds a buffer to the display buffer manager
*
* @param[in] ps_disp_mgr
*  Pointer to the diaplay buffer management structure
*
* @param[in] buf_id
*  ID of the display buffer
*
* @param[in] abs_poc
*  Absolute POC of the display buffer
*
* @param[in] pv_ptr
*  Pointer to the display buffer
*
* @returns  0 if success, -1 otherwise
*
* @remarks
*  None
*
*******************************************************************************
*/
WORD32 impeg2_disp_mgr_add(disp_mgr_t *ps_disp_mgr,
                          void *pv_ptr,
                          WORD32 i4_buf_id)
{


    WORD32 id;
    id = ps_disp_mgr->i4_wr_idx % DISP_MGR_MAX_CNT;

    ps_disp_mgr->apv_ptr[id] = pv_ptr;
    ps_disp_mgr->ai4_buf_id[id] = i4_buf_id;
    ps_disp_mgr->i4_wr_idx++;

    return 0;
}


/**
*******************************************************************************
*
* @brief
*  Gets the next buffer
*
* @par Description:
*  Gets the next display buffer
*
* @param[in] ps_disp_mgr
*  Pointer to the display buffer structure
*
* @param[out]  pi4_buf_id
*  Pointer to hold buffer id of the display buffer being returned
*
* @returns  Pointer to the next display buffer
*
* @remarks
*  None
*
*******************************************************************************
*/
void* impeg2_disp_mgr_get(disp_mgr_t *ps_disp_mgr, WORD32 *pi4_buf_id)
{
    WORD32 id;

    *pi4_buf_id = -1;

    if(ps_disp_mgr->i4_rd_idx < ps_disp_mgr->i4_wr_idx)
    {
        id = ps_disp_mgr->i4_rd_idx % DISP_MGR_MAX_CNT;
        if(NULL == ps_disp_mgr->apv_ptr[id])
        {
            return NULL;
        }

        *pi4_buf_id = ps_disp_mgr->ai4_buf_id[id];

        ps_disp_mgr->i4_rd_idx++;

        return ps_disp_mgr->apv_ptr[id];
    }
    else
        return NULL;

}
