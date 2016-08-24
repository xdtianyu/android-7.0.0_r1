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
*  ideint_function_selector_generic.c
*
* @brief
*  This file contains the function selector related code
*
* @author
*  Ittiam
*
* @par List of Functions:
* ih264e_init_function_ptr_generic
*
* @remarks
*  None
*
*******************************************************************************
*/
/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
/* System include files */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


/* User include files */
#include "icv_datatypes.h"
#include "icv_macros.h"
#include "icv_platform_macros.h"
#include "icv.h"
#include "icv_variance.h"
#include "icv_sad.h"
#include "ideint.h"

#include "ideint_defs.h"
#include "ideint_structs.h"
#include "ideint_utils.h"
#include "ideint_cac.h"
#include "ideint_debug.h"

/**
*******************************************************************************
*
* @brief
*  Initialize the function pointers
*
* @par Description
*  The current routine initializes the function pointers as generic c functions
*
* @param[in] ps_ctxt
*  Context pointer
*
* @returns  none
*
* @remarks none
*
*******************************************************************************
*/
void ideint_init_function_ptr_generic(ctxt_t *ps_ctxt)
{
    ps_ctxt->pf_sad_8x4 = icv_sad_8x4;
    ps_ctxt->pf_variance_8x4 = icv_variance_8x4;
    ps_ctxt->pf_spatial_filter = ideint_spatial_filter;
    ps_ctxt->pf_cac_8x8 = ideint_cac_8x8;
    return;
}
