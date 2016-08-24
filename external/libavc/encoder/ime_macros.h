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
*  ihevc_typedefs.h
*
* @brief
*  Type definitions used in the code
*
*
* @remarks
*  None
*
*******************************************************************************
*/
#ifndef _IME_MACROS_H_
#define _IME_MACROS_H_

#define ABS(x)          ((x) < 0 ? (-(x)) : (x))
#define MAX(a,b) ((a > b)?(a):(b))
#define MIN(a,b) ((a < b)?(a):(b))

#define CLIP3(miny, maxy, y) (((y) < (miny))?(miny):(((y) > maxy)?(maxy):(y)))
#define UNUSED(x) ((void)(x))

#endif /*_IME_MACROS_H_*/
