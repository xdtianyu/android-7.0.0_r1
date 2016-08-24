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

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* System include files */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
/* User include files */

#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264e.h"
#include "app.h"

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/
#define PEAK_WINDOW_SIZE    8
/*****************************************************************************/
/*  Macros                                                                   */
/*****************************************************************************/
/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/
IV_STATUS_T write_output(FILE *fp, UWORD8 *pu1_buf, WORD32 num_bytes)
{
    WORD32 bytes;

    bytes = fwrite(pu1_buf, sizeof(UWORD8), num_bytes, fp);
    if(bytes != num_bytes)
        return IV_FAIL;
    fflush(fp);

    return IV_SUCCESS;
}

void allocate_output(app_ctxt_t *ps_app_ctxt)
{

    WORD32 num_bufs;
    WORD32 i;
    UWORD8 *pu1_buf;
    WORD32 buf_size;
    num_bufs = MAX(DEFAULT_NUM_OUTPUT_BUFS, ps_app_ctxt->s_get_buf_info_op.s_ive_op.u4_min_out_bufs);
    num_bufs = MIN(DEFAULT_MAX_OUTPUT_BUFS, num_bufs);

    buf_size = ps_app_ctxt->s_get_buf_info_op.s_ive_op.au4_min_out_buf_size[0];
    /* Memset the output buffer array to set is_free to 0 */
    memset(ps_app_ctxt->as_output_buf, 0, sizeof(output_buf_t) * DEFAULT_MAX_OUTPUT_BUFS);

    for(i = 0; i < num_bufs; i++)
    {
        pu1_buf = (UWORD8 *)ih264a_aligned_malloc(16, buf_size);
        if(NULL == pu1_buf)
        {
            CHAR ac_error[STRLENGTH];
            sprintf(ac_error, "Allocation failed for output buffer of size %d\n",
                    buf_size);
            codec_exit(ac_error);
        }
        ps_app_ctxt->as_output_buf[i].pu1_buf = pu1_buf;
        ps_app_ctxt->as_output_buf[i].u4_buf_size = buf_size;
        ps_app_ctxt->as_output_buf[i].u4_is_free = 1;

    }
    return;
}

void free_output(app_ctxt_t *ps_app_ctxt)
{

    WORD32 num_bufs;
    WORD32 i;

    num_bufs = MAX(DEFAULT_NUM_OUTPUT_BUFS, ps_app_ctxt->s_get_buf_info_op.s_ive_op.u4_min_out_bufs);
    num_bufs = MIN(DEFAULT_MAX_OUTPUT_BUFS, num_bufs);
    for(i = 0; i < num_bufs; i++)
    {

        ih264a_aligned_free(ps_app_ctxt->as_output_buf[i].pu1_buf);
    }
    return;
}

