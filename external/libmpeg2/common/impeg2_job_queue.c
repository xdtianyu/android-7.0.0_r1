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
*  impeg2d_job_queue.c
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
/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "iv_datatypedef.h"
#include "iv.h"
#include "ithread.h"
#include "impeg2_macros.h"
#include "impeg2_job_queue.h"

/**
*******************************************************************************
*
* @brief Returns size for job queue context. Does not include job queue buffer
* requirements
*
* @par   Description
* Returns size for job queue context. Does not include job queue buffer
* requirements. Buffer size required to store the jobs should be allocated in
* addition to the value returned here.
*
* @returns Size of the job queue context
*
* @remarks
*
*******************************************************************************
*/
WORD32 impeg2_jobq_ctxt_size()
{
    WORD32 i4_size;
    i4_size = sizeof(jobq_t);
    i4_size += ithread_get_mutex_lock_size();
    return i4_size;
}

/**
*******************************************************************************
*
* @brief
*   Locks the jobq conext
*
* @par   Description
*   Locks the jobq conext by calling ithread_mutex_lock()
*
* @param[in] ps_jobq
*   Job Queue context
*
* @returns IMPEG2D_FAIL if mutex lock fails else IV_SUCCESS
*
* @remarks
*
*******************************************************************************
*/
IV_API_CALL_STATUS_T impeg2_jobq_lock(jobq_t *ps_jobq)
{
    WORD32 i4_ret_val;
    i4_ret_val = ithread_mutex_lock(ps_jobq->pv_mutex);
    if(i4_ret_val)
    {
        return IV_FAIL;
    }
    return IV_SUCCESS;
}

/**
*******************************************************************************
*
* @brief
*   Unlocks the jobq conext
*
* @par   Description
*   Unlocks the jobq conext by calling ithread_mutex_unlock()
*
* @param[in] ps_jobq
*   Job Queue context
*
* @returns IMPEG2D_FAIL if mutex unlock fails else IV_SUCCESS
*
* @remarks
*
*******************************************************************************
*/

IV_API_CALL_STATUS_T impeg2_jobq_unlock(jobq_t *ps_jobq)
{
    WORD32 i4_ret_val;
    i4_ret_val = ithread_mutex_unlock(ps_jobq->pv_mutex);
    if(i4_ret_val)
    {
        return IV_FAIL;
    }
    return IV_SUCCESS;

}
/**
*******************************************************************************
*
* @brief
*   Yeilds the thread
*
* @par   Description
*   Unlocks the jobq conext by calling
* impeg2_jobq_unlock(), ithread_yield() and then impeg2_jobq_lock()
* jobq is unlocked before to ensure the jobq can be accessed by other threads
* If unlock is not done before calling yield then no other thread can access
* the jobq functions and update jobq.
*
* @param[in] ps_jobq
*   Job Queue context
*
* @returns IMPEG2D_FAIL if mutex lock unlock or yield fails else IV_SUCCESS
*
* @remarks
*
*******************************************************************************
*/
IV_API_CALL_STATUS_T impeg2_jobq_yield(jobq_t *ps_jobq)
{

    IV_API_CALL_STATUS_T e_ret = IV_SUCCESS;

    IV_API_CALL_STATUS_T e_ret_tmp;
    e_ret_tmp = impeg2_jobq_unlock(ps_jobq);
    RETURN_IF((e_ret_tmp != IV_SUCCESS), e_ret_tmp);

    //NOP(1024 * 8);
    ithread_yield();

    e_ret_tmp = impeg2_jobq_lock(ps_jobq);
    RETURN_IF((e_ret_tmp != IV_SUCCESS), e_ret_tmp);
    return e_ret;
}


/**
*******************************************************************************
*
* @brief free the job queue pointers
*
* @par   Description
* Frees the jobq context
*
* @param[in] pv_buf
* Memoy for job queue buffer and job queue context
*
* @returns Pointer to job queue context
*
* @remarks
* Since it will be called only once by master thread this is not thread safe.
*
*******************************************************************************
*/
IV_API_CALL_STATUS_T impeg2_jobq_free(jobq_t *ps_jobq)
{
    WORD32 i4_ret;
    i4_ret = ithread_mutex_destroy(ps_jobq->pv_mutex);

    if(0 == i4_ret)
        return IV_SUCCESS;
    else
        return IV_FAIL;
}

/**
*******************************************************************************
*
* @brief Initialize the job queue
*
* @par   Description
* Initializes the jobq context and sets write and read pointers to start of
* job queue buffer
*
* @param[in] pv_buf
* Memoy for job queue buffer and job queue context
*
* @param[in] buf_size
* Size of the total memory allocated
*
* @returns Pointer to job queue context
*
* @remarks
* Since it will be called only once by master thread this is not thread safe.
*
*******************************************************************************
*/
void* impeg2_jobq_init(void *pv_buf, WORD32 i4_buf_size)
{
    jobq_t *ps_jobq;
    UWORD8 *pu1_buf;
    pu1_buf = (UWORD8 *)pv_buf;

    ps_jobq = (jobq_t *)pu1_buf;
    pu1_buf += sizeof(jobq_t);
    i4_buf_size -= sizeof(jobq_t);

    ps_jobq->pv_mutex = pu1_buf;
    pu1_buf += ithread_get_mutex_lock_size();
    i4_buf_size -= ithread_get_mutex_lock_size();

    if(i4_buf_size <= 0)
        return NULL;

    ithread_mutex_init(ps_jobq->pv_mutex);

    ps_jobq->pv_buf_base = pu1_buf;
    ps_jobq->pv_buf_wr = pu1_buf;
    ps_jobq->pv_buf_rd = pu1_buf;
    ps_jobq->pv_buf_end = pu1_buf + i4_buf_size;
    ps_jobq->i4_terminate = 0;


    return ps_jobq;
}
/**
*******************************************************************************
*
* @brief
*   Resets the jobq conext
*
* @par   Description
*   Resets the jobq conext by initilizing job queue context elements
*
* @param[in] ps_jobq
*   Job Queue context
*
* @returns IMPEG2D_FAIL if lock unlock fails else IV_SUCCESS
*
* @remarks
*
*******************************************************************************
*/
IV_API_CALL_STATUS_T impeg2_jobq_reset(jobq_t *ps_jobq)
{
    IV_API_CALL_STATUS_T e_ret = IV_SUCCESS;
    e_ret = impeg2_jobq_lock(ps_jobq);
    RETURN_IF((e_ret != IV_SUCCESS), e_ret);

    ps_jobq->pv_buf_wr      = ps_jobq->pv_buf_base;
    ps_jobq->pv_buf_rd      = ps_jobq->pv_buf_base;
    ps_jobq->i4_terminate   = 0;
    e_ret = impeg2_jobq_unlock(ps_jobq);
    RETURN_IF((e_ret != IV_SUCCESS), e_ret);

    return e_ret;
}

/**
*******************************************************************************
*
* @brief
*   Deinitializes the jobq conext
*
* @par   Description
*   Deinitializes the jobq conext by calling impeg2_jobq_reset()
* and then destrying the mutex created
*
* @param[in] ps_jobq
*   Job Queue context
*
* @returns IMPEG2D_FAIL if lock unlock fails else IV_SUCCESS
*
* @remarks
*
*******************************************************************************
*/
IV_API_CALL_STATUS_T impeg2_jobq_deinit(jobq_t *ps_jobq)
{
    WORD32 i4_ret_val;
    IV_API_CALL_STATUS_T e_ret = IV_SUCCESS;

    e_ret = impeg2_jobq_reset(ps_jobq);
    RETURN_IF((e_ret != IV_SUCCESS), e_ret);

    i4_ret_val = ithread_mutex_destroy(ps_jobq->pv_mutex);
    if(i4_ret_val)
    {
        return IV_FAIL;
    }

    return IV_SUCCESS;
}


/**
*******************************************************************************
*
* @brief
*   Terminates the jobq
*
* @par   Description
*   Terminates the jobq by setting a flag in context.
*
* @param[in] ps_jobq
*   Job Queue context
*
* @returns IMPEG2D_FAIL if lock unlock fails else IV_SUCCESS
*
* @remarks
*
*******************************************************************************
*/

IV_API_CALL_STATUS_T impeg2_jobq_terminate(jobq_t *ps_jobq)
{
    IV_API_CALL_STATUS_T e_ret = IV_SUCCESS;
    e_ret = impeg2_jobq_lock(ps_jobq);
    RETURN_IF((e_ret != IV_SUCCESS), e_ret);

    ps_jobq->i4_terminate = 1;

    e_ret = impeg2_jobq_unlock(ps_jobq);
    RETURN_IF((e_ret != IV_SUCCESS), e_ret);
    return e_ret;
}


/**
*******************************************************************************
*
* @brief Adds a job to the queue
*
* @par   Description
* Adds a job to the queue and updates wr address to next location.
* Format/content of the job structure is abstracted and hence size of the job
* buffer is being passed.
*
* @param[in] ps_jobq
*   Job Queue context
*
* @param[in] pv_job
*   Pointer to the location that contains details of the job to be added
*
* @param[in] job_size
*   Size of the job buffer
*
* @param[in] blocking
*   To signal if the write is blocking or non-blocking.
*
* @returns
*
* @remarks
* Job Queue buffer is assumed to be allocated to handle worst case number of jobs
* Wrap around is not supported
*
*******************************************************************************
*/
IV_API_CALL_STATUS_T impeg2_jobq_queue(jobq_t *ps_jobq,
                                       void *pv_job,
                                       WORD32 i4_job_size,
                                       WORD32 i4_blocking,
                                       WORD32 i4_lock)
{
    IV_API_CALL_STATUS_T e_ret = IV_SUCCESS;
    IV_API_CALL_STATUS_T e_ret_tmp;
    UWORD8 *pu1_buf;
    UNUSED(i4_blocking);

    if(i4_lock)
    {
        e_ret_tmp = impeg2_jobq_lock(ps_jobq);
        RETURN_IF((e_ret_tmp != IV_SUCCESS), e_ret_tmp);
    }
    pu1_buf = (UWORD8 *)ps_jobq->pv_buf_wr;
    if((UWORD8 *)ps_jobq->pv_buf_end >= (pu1_buf + i4_job_size))
    {
        memcpy(ps_jobq->pv_buf_wr, pv_job, i4_job_size);
        ps_jobq->pv_buf_wr = (UWORD8 *)ps_jobq->pv_buf_wr + i4_job_size;
        e_ret = IV_SUCCESS;
    }
    else
    {
        /* Handle wrap around case */
        /* Wait for pv_buf_rd to consume first job_size number of bytes
         * from the beginning of job queue
         */
        e_ret = IV_FAIL;
    }

    ps_jobq->i4_terminate = 0;

    if(i4_lock)
    {
        e_ret_tmp = impeg2_jobq_unlock(ps_jobq);
        RETURN_IF((e_ret_tmp != IV_SUCCESS), e_ret_tmp);
    }

    return e_ret;
}
/**
*******************************************************************************
*
* @brief Gets next from the Job queue
*
* @par   Description
* Gets next job from the job queue and updates rd address to next location.
* Format/content of the job structure is abstracted and hence size of the job
* buffer is being passed. If it is a blocking call and if there is no new job
* then this functions unlocks the mutext and calls yield and then locks it back.
* and continues till a job is available or terminate is set
*
* @param[in] ps_jobq
*   Job Queue context
*
* @param[out] pv_job
*   Pointer to the location that contains details of the job to be written
*
* @param[in] job_size
*   Size of the job buffer
*
* @param[in] blocking
*   To signal if the read is blocking or non-blocking.
*
* @returns
*
* @remarks
* Job Queue buffer is assumed to be allocated to handle worst case number of jobs
* Wrap around is not supported
*
*******************************************************************************
*/
IV_API_CALL_STATUS_T impeg2_jobq_dequeue(jobq_t *ps_jobq,
                                         void *pv_job,
                                         WORD32 i4_job_size,
                                         WORD32 i4_blocking,
                                         WORD32 i4_lock)
{
    IV_API_CALL_STATUS_T e_ret;
    IV_API_CALL_STATUS_T e_ret_tmp;
    volatile UWORD8 *pu1_buf;
    if(i4_lock)
    {
        e_ret_tmp = impeg2_jobq_lock(ps_jobq);
        RETURN_IF((e_ret_tmp != IV_SUCCESS), e_ret_tmp);
    }
    pu1_buf = (UWORD8 *)ps_jobq->pv_buf_rd;


    if((UWORD8 *)ps_jobq->pv_buf_end >= (pu1_buf + i4_job_size))
    {
        while(1)
        {
            pu1_buf = (UWORD8 *)ps_jobq->pv_buf_rd;
            if((UWORD8 *)ps_jobq->pv_buf_wr >= (pu1_buf + i4_job_size))
            {
                memcpy(pv_job, ps_jobq->pv_buf_rd, i4_job_size);
                ps_jobq->pv_buf_rd = (UWORD8 *)ps_jobq->pv_buf_rd + i4_job_size;
                e_ret = IV_SUCCESS;
                break;
            }
            else
            {
                /* If all the entries have been dequeued, then break and return */
                if(1 == ps_jobq->i4_terminate)
                {
                    e_ret = IV_FAIL;
                    break;
                }

                if((1 == i4_blocking) && (1 == i4_lock))
                {
                    impeg2_jobq_yield(ps_jobq);

                }
                else
                {
                    /* If there is no job available,
                     * and this is non blocking call then return fail */
                    e_ret = IV_FAIL;
                }
            }
        }
    }
    else
    {
        /* Handle wrap around case */
        /* Wait for pv_buf_rd to consume first i4_job_size number of bytes
         * from the beginning of job queue
         */
        e_ret = IV_FAIL;
    }
    if(i4_lock)
    {
        e_ret_tmp = impeg2_jobq_unlock(ps_jobq);
        RETURN_IF((e_ret_tmp != IV_SUCCESS), e_ret_tmp);
    }

    return e_ret;
}
