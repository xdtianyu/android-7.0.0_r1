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
void ih264e_init_function_ptr_neon_a9q(codec_t *ps_codec);

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
void ih264e_init_function_ptr_neon_av8(codec_t *ps_codec);

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

#endif /* IH264E_PLATFORM_MACROS_H_ */
