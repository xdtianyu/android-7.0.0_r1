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
******************************************************************************
* @file
*  ih264e_trace.h
*
* @brief
*  This file contains extern declarations of routines that could be helpful
*  for debugging purposes.
*
* @author
*  ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_TRACE_H_
#define IH264E_TRACE_H_

#if ENABLE_TRACE
/*****************************************************************************/
/* Structures                                                                */
/*****************************************************************************/

/**
******************************************************************************
 *  @brief      Data for the trace functionality
******************************************************************************
 */
typedef struct
{
    /**
     * fp
     */
    FILE    *fp;
}enc_trace_t;

/*****************************************************************************/
/* Extern variable declarations                                              */
/*****************************************************************************/
extern enc_trace_t g_enc_trace;

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/

/**
******************************************************************************
 *  @brief      defines flag used for enabling trace
******************************************************************************
 */


/*****************************************************************************/
/* Function Macros                                                           */
/*****************************************************************************/

/**
******************************************************************************
 *  @brief   Macro to print trace messages
******************************************************************************
 */

#define ENTROPY_TRACE(syntax_string, value)                                           \
    {                                                                                 \
        if(g_enc_trace.fp)                                                            \
        {                                                                             \
            fprintf( g_enc_trace.fp, "%-40s : %d\n", syntax_string, value );          \
            fflush ( g_enc_trace.fp);                                                 \
        }                                                                             \
    }


/**
******************************************************************************
 *  @brief   Macro to print CABAC trace messages
******************************************************************************
 */

#define AEV_TRACE(string, value, range)                                      \
    if(range && g_enc_trace.fp)                                                                \
    {                                                                        \
        fprintf( g_enc_trace.fp, "%-40s:%8d R:%d\n", string, value, range);  \
        fflush ( g_enc_trace.fp);                                            \
    }

#else

/* Dummy macros when trace is disabled */
#define ENTROPY_TRACE(syntax_string, value)

#define AEV_TRACE(string, value, range)

#endif


/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/


/**
******************************************************************************
*
*  @brief Dummy trace init when trace is disabled in encoder
*
*  @par   Description
*  This routine needs to be called at start of trace
*
*  @param[in]   pu1_file_name
*  Name of file where trace outputs need to be stores (handle)
*
*  @return      success or failure error code
*
******************************************************************************
*/
extern WORD32    ih264e_trace_init
        (
            const char        *pu1_file_name
        );

/**
******************************************************************************
*
*  @brief Dummy trace de-init function when trace is disabled
*
*  @par   Description
*  This routine needs to be called at end of trace
*
*  @return      success or failure error code
*
******************************************************************************
*/
extern WORD32    ih264e_trace_deinit
        (
            void
        );

#endif // IH264E_TRACE_H_
