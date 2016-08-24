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
*  ideint_function_selector.h
*
* @brief
*  Contains various functions needed in function selector
*
* @author
*  Ittiam
*
* @par List of Functions:
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef __IDEINT_FUNCTION_SELECTOR_H__
#define __IDEINT_FUNCTION_SELECTOR_H__

ICV_ARCH_T ideint_default_arch(void);
void ideint_init_function_ptr(ctxt_t *ps_ctxt);
void ideint_init_function_ptr_generic(ctxt_t *ps_ctxt);
void ideint_init_function_ptr_a9(ctxt_t *ps_ctxt);
void ideint_init_function_ptr_av8(ctxt_t *ps_ctxt);

void ideint_init_function_ptr_ssse3(ctxt_t *ps_ctxt);
void ideint_init_function_ptr_sse42(ctxt_t *ps_ctxt);

#endif /* __IDEINT_FUNCTION_SELECTOR_H__ */
