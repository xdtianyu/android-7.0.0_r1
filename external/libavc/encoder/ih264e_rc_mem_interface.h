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
*  ih264e_rc_mem_interface.h
*
* @brief
*  This file contains function declaration and structures for rate control
*  memtabs
*
* @author
*  ittiam
*
* @remarks
*  The rate control library is a global library across various codecs. It
*  anticipates certain structures definitions. Those definitions are to be
*  imported from global workspace. Instead of that, the structures needed for
*  rc library are copied in to this file and exported to rc library. If the
*  structures / enums / ... in the global workspace change, this file also needs
*  to be modified accordingly.
*
******************************************************************************
*/
#ifndef IH264E_RC_MEM_INTERFACE_H_
#define IH264E_RC_MEM_INTERFACE_H_


/*****************************************************************************/
/* Function Macros                                                           */
/*****************************************************************************/

#define FILL_MEMTAB(m_pv_mem_rec, m_j, m_mem_size, m_align, m_type)      \
{                                                                        \
    m_pv_mem_rec[m_j].u4_size = sizeof(iv_mem_rec_t);                    \
    m_pv_mem_rec[m_j].u4_mem_size = m_mem_size;                          \
    m_pv_mem_rec[m_j].u4_mem_alignment = m_align;                        \
    m_pv_mem_rec[m_j].e_mem_type = m_type;                               \
}

/*****************************************************************************/
/* Enums                                                                     */
/*****************************************************************************/
typedef enum
{
    ALIGN_BYTE = 1,
    ALIGN_WORD16 = 2,
    ALIGN_WORD32 = 4,
    ALIGN_WORD64 = 8,
    ALIGN_128_BYTE = 128
}ITT_MEM_ALIGNMENT_TYPE_E;

typedef enum
{
    SCRATCH = 0,
    PERSISTENT = 1,
    WRITEONCE  = 2
}ITT_MEM_USAGE_TYPE_E;

typedef enum
{
    L1D = 0,
    SL2 = 1,
    DDR = 3
}ITT_MEM_REGION_E;

typedef enum
{
    GET_NUM_MEMTAB = 0,
    FILL_MEMTAB = 1,
    USE_BASE = 2,
    FILL_BASE =3
}ITT_FUNC_TYPE_E;


/*****************************************************************************/
/* Structures                                                                */
/*****************************************************************************/

/*NOTE : This should be an exact replica of IALG_MemRec, any change in IALG_MemRec
         must be replicated here*/
typedef struct
{
    /* Size in bytes */
    UWORD32 u4_size;

    /* Alignment in bytes */
    WORD32 i4_alignment;

    /* decides which memory region to be placed */
    ITT_MEM_REGION_E e_mem_region;

    /* memory is scratch or persistent */
    ITT_MEM_USAGE_TYPE_E e_usage;

    /* Base pointer for allocated memory */
    void *pv_base;
} itt_memtab_t;


/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/

/**
******************************************************************************
*
* @brief This function fills memory record attributes
*
* @par   Description
*  This function fills memory record attributes
*
* @param[in] ps_mem_tab
*  pointer to mem records
*
* @param[in] u4_size
*  size of the record
*
* @param[in] i4_alignment
*  memory alignment size
*
* @param[in] e_usage
*  usage
*
* @param[in] e_mem_region
*  mem region
*
* @return void
*
******************************************************************************
*/
void fill_memtab(itt_memtab_t *ps_mem_tab, WORD32 u4_size, WORD32 i4_alignment,
                 ITT_MEM_USAGE_TYPE_E e_usage, ITT_MEM_REGION_E e_mem_region);

/**
******************************************************************************
*
* @brief This function fills memory record attributes
*
* @par   Description
*  This function fills memory record attributes
*
* @param[in] ps_mem_tab
*  pointer to mem records
*
* @param[in] ptr_to_be_filled
*  handle to the memory record storage space
*
* @param[in] e_func_type
*  enum that dictates fill memory records or use memory records
*
* @return void
*
******************************************************************************
*/
WORD32 use_or_fill_base(itt_memtab_t *ps_mem_tab, void **ptr_to_be_filled,
                        ITT_FUNC_TYPE_E e_func_type);


#endif // IH264E_RC_MEM_INTERFACE_H_

