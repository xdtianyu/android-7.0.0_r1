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
 *  ih264e_platform_macros.h
 *
 * @brief
 *  Contains platform specific routines used for codec context intialization
 *
 * @author
 *  ittiam
 *
 * @remarks
 *  none
 *
 *******************************************************************************
 */


#ifndef IH264E_PLATFORM_MACROS_H_
#define IH264E_PLATFORM_MACROS_H_

#define DATA_SYNC()
/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief Initialize the intra/inter/transform/deblk function pointers of
* codec context
*
* @par Description: the current routine initializes the function pointers of
* codec context basing on the architecture in use
*
* @param[in] ps_codec
*  Codec context pointer
*
* @returns  none
*
* @remarks none
*
*******************************************************************************
*/
void ih264e_init_function_ptr_generic(codec_t *ps_codec);

/**
*******************************************************************************
*
* @brief Initialize the intra/inter/transform/deblk function pointers of
* codec context
*
* @par Description: the current routine initializes the function pointers of
* codec context basing on the architecture in use
*
* @param[in] ps_codec
*  Codec context pointer
*
* @returns  none
*
* @remarks none
*
*******************************************************************************
*/
void ih264e_init_function_ptr(void *pv_codec);

/**
*******************************************************************************
*
* @brief Determine the architecture of the encoder executing environment
*
* @par Description: This routine returns the architecture of the enviro-
* ment in which the current encoder is being tested
*
* @param[in] void
*
* @returns  IV_ARCH_T
*  architecture
*
* @remarks none
*
*******************************************************************************
*/
IV_ARCH_T ih264e_default_arch(void);

/**
*******************************************************************************
*
* @brief Data Memory Barrier, Data Synchronization Barrier
*
*
* @par Description: These functions do nothing on x86 side. But on arm platforms,
*
* Data Memory Barrier acts as a memory barrier. It ensures that all explicit
* memory accesses that appear in program order before the DMB instruction are
* observed before any explicit memory accesses that appear in program order
* after the DMB instruction. It does not affect the ordering of any other
* instructions executing on the processor
*
* Data Synchronization Barrier acts as a special kind of memory barrier. No
* instruction in program order after this instruction executes until this instruction
* completes. This instruction completes when:
*       1. All explicit memory accesses before this instruction complete.
*       2. All Cache, Branch predictor and TLB maintenance operations before
*       this instruction complete.
*
* @param[in] void
*
* @returns  void
*
* @remarks none
*
*******************************************************************************
*/

#endif /* IH264E_PLATFORM_MACROS_H_ */
