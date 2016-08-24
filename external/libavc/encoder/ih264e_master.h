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
*  ih264e_master.h
*
* @brief
*  Contains declarations of functions used by master thread
*
* @author
*  ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_MASTER_H_
#define IH264E_MASTER_H_

/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/

/**
******************************************************************************
*
* @brief
*  This function joins all the spawned threads after successful completion of
*  their tasks
*
* @par   Description
*
* @param[in] ps_codec
*  pointer to codec context
*
* @returns  none
*
******************************************************************************
*/
void ih264e_join_threads(codec_t *ps_codec);

/**
******************************************************************************
*
* @brief This function puts the current thread to sleep for a duration
*  of sleep_us
*
* @par Description
*  ithread_yield() method causes the calling thread to yield execution to another
*  thread that is ready to run on the current processor. The operating system
*  selects the thread to yield to. ithread_usleep blocks the current thread for
*  the specified number of milliseconds. In other words, yield just says,
*  end my timeslice prematurely, look around for other threads to run. If there
*  is nothing better than me, continue. Sleep says I don't want to run for x
*  milliseconds. Even if no other thread wants to run, don't make me run.
*
* @param[in] sleep_us
*  thread sleep duration
*
* @returns error_status
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_wait_for_thread(UWORD32 sleep_us);

/**
******************************************************************************
*
* @brief
*  Encodes in synchronous api mode
*
* @par Description
*  This routine processes input yuv, encodes it and outputs bitstream and recon
*
* @param[in] ps_codec_obj
*  Pointer to codec object at API level
*
* @param[in] pv_api_ip
*  Pointer to input argument structure
*
* @param[out] pv_api_op
*  Pointer to output argument structure
*
* @returns  Status
*
******************************************************************************
*/
WORD32 ih264e_encode(iv_obj_t *ps_codec_obj, void *pv_api_ip, void *pv_api_op);

/**
*******************************************************************************
*
* @brief update encoder configuration parameters
*
* @par Description:
*  updates encoder configuration parameters from the given config set.
*  Initialize/reinitialize codec parameters according to new configurations.
*
* @param[in] ps_codec
*  Pointer to codec context
*
* @param[in] ps_cfg
*  Pointer to config param set
*
* @remarks none
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_codec_update_config(codec_t *ps_codec, cfg_params_t *ps_cfg);

#endif /* IH264E_MASTER_H_ */
