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
*  impeg2_job_queue.h
*
* @brief
*  Contains functions for job queue
*
* @author
*  Harish
*
* @par List of Functions:
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef _IMPEG2_JOB_QUEUE_H_
#define _IMPEG2_JOB_QUEUE_H_

typedef struct
{
    /** Pointer to buffer base which contains the jobs */
    void *pv_buf_base;

    /** Pointer to current address where new job can be added */
    void *pv_buf_wr;

    /** Pointer to current address from where next job can be obtained */
    void *pv_buf_rd;

    /** Pointer to end of job buffer */
    void *pv_buf_end;

    /** Mutex used to keep the functions thread-safe */
    void *pv_mutex;

    /** Flag to indicate jobq has to be terminated */
    WORD32 i4_terminate;
}jobq_t;

WORD32 impeg2_jobq_ctxt_size(void);
void* impeg2_jobq_init(void *pv_buf, WORD32 buf_size);
IV_API_CALL_STATUS_T impeg2_jobq_free(jobq_t *ps_jobq);
IV_API_CALL_STATUS_T impeg2_jobq_reset(jobq_t *ps_jobq);
IV_API_CALL_STATUS_T impeg2_jobq_deinit(jobq_t *ps_jobq);
IV_API_CALL_STATUS_T impeg2_jobq_terminate(jobq_t *ps_jobq);
IV_API_CALL_STATUS_T impeg2_jobq_queue(jobq_t *ps_jobq, void *pv_job, WORD32 job_size, WORD32 blocking, WORD32 lock);
IV_API_CALL_STATUS_T impeg2_jobq_dequeue(jobq_t *ps_jobq, void *pv_job, WORD32 job_size, WORD32 blocking, WORD32 lock);

#endif /* _IMPEG2_JOB_QUEUE_H_ */
