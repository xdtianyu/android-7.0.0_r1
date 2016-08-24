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
/*****************************************************************************/
/*                                                                           */
/*  File Name         : datatypedef.h                                        */
/*                                                                           */
/*  Description       : This file contains all the necessary data type       */
/*                      definitions.                                         */
/*                                                                           */
/*  List of Functions : None                                                 */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         29 12 2006  Rajendra C Y          Draft                           */
/*                                                                           */
/*****************************************************************************/

#ifndef __IV_DATATYPEDEF_H__
#define __IV_DATATYPEDEF_H__

/*****************************************************************************/
/* Typedefs                                                                  */
/*****************************************************************************/

typedef int             WORD32;
typedef unsigned int    UWORD32;

typedef short           WORD16;
typedef unsigned short  UWORD16;

typedef char            WORD8;
typedef unsigned char   UWORD8;

typedef char            CHAR;
#ifndef NULL
#define NULL            ((void *)0)

#endif

typedef enum
{
    IT_FALSE,
    IT_TRUE
} IT_BOOL;


typedef enum
{
    IT_OK,
    IT_ERROR = -1
} IT_STATUS;

/*****************************************************************************/
/* Input and Output Parameter identifiers                                    */
/*****************************************************************************/
#define                 IT_IN
#define                 IT_OUT


#endif /* __IV_DATATYPEDEF_H__ */

