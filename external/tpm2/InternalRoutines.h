// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef        INTERNAL_ROUTINES_H
#define        INTERNAL_ROUTINES_H
//
//     NULL definition
//
#ifndef              NULL
#define              NULL        (0)
#endif
//
//     UNUSED_PARAMETER
//
#ifndef              UNUSED_PARAMETER
#define              UNUSED_PARAMETER(param)    (void)(param);
#endif
//
//     Internal data definition
//
#include "Global.h"
#include "VendorString.h"
//
//     Error Reporting
//
#include "TpmError.h"
//
//     DRTM functions
//
#include "_TPM_Hash_Data_fp.h"
#include "_TPM_Hash_End_fp.h"
#include "_TPM_Hash_Start_fp.h"
//
//     Internal subsystem functions
//
#include   "DA_fp.h"
#include   "Entity_fp.h"
#include   "Hierarchy_fp.h"
#include   "NV_fp.h"
#include   "Object_fp.h"
#include   "PCR_fp.h"
#include   "Session_fp.h"
#include   "TpmFail_fp.h"
//
//     Internal support functions
//
#include   "AlgorithmCap_fp.h"
#include   "Bits_fp.h"
#include   "CommandAudit_fp.h"
#include   "CommandCodeAttributes_fp.h"
#include   "Commands_fp.h"
#include   "Handle_fp.h"
#include   "Locality_fp.h"
#include   "Manufacture_fp.h"
#include   "MemoryLib_fp.h"
#include   "Power_fp.h"
#include   "PropertyCap_fp.h"
#include   "PP_fp.h"
#include   "Time_fp.h"
#include   "tpm_generated.h"
//
//     Internal crypto functions
//
#include "CryptSelfTest_fp.h"
#include "CryptUtil_fp.h"
#include "Ticket_fp.h"
#endif
