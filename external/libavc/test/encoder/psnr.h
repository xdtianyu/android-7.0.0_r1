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
/****************************************************************************/
/*                                                                          */
/*  File Name         : psnr.h                                              */
/*                                                                          */
/*  Description       : Contains functions for psnr computation             */
/*                                                                          */
/*  List of Functions :   ih264e_api_function                               */
/*                        compute_psnr                                      */
/*                        print_average_psnr                                */
/*  Issues / Problems :                                                     */
/*                                                                          */
/*  Revision History  :                                                     */
/*                                                                          */
/*       DD MM YYYY      Author(s)        Changes (Describe the changes)    */
/*                                                                          */
/****************************************************************************/
#ifndef  PSNR_H
#define  PSNR_H

/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/
void init_psnr(app_ctxt_t *ps_app_ctxt);

void compute_psnr(app_ctxt_t *ps_app_ctxt,
                  iv_raw_buf_t *ps_buf1,
                  iv_raw_buf_t *ps_buf2);

void print_average_psnr(app_ctxt_t *ps_app_ctxt);

#if COMPUTE_PSNR

#define GET_AVERAGE_PSNR_Y(print) print_average_psnr(print)

#else  /* COMPUTE_PSNR */

#define GET_AVERAGE_PSNR_Y(print) 0

#endif /* COMPUTE_PSNR */

#endif


