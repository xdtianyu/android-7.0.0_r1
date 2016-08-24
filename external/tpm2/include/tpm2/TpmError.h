// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef _TPM_ERROR_H
#define _TPM_ERROR_H
#include "TpmBuildSwitches.h"
#define     FATAL_ERROR_ALLOCATION                         (1)
#define     FATAL_ERROR_DIVIDE_ZERO                        (2)
#define     FATAL_ERROR_INTERNAL                           (3)
#define     FATAL_ERROR_PARAMETER                          (4)
#define     FATAL_ERROR_ENTROPY                            (5)
#define     FATAL_ERROR_SELF_TEST                          (6)
#define     FATAL_ERROR_CRYPTO                             (7)
#define     FATAL_ERROR_NV_UNRECOVERABLE                   (8)
#define     FATAL_ERROR_REMANUFACTURED                     (9) // indicates that the TPM has
                                                               // been re-manufactured after an
                                                               // unrecoverable NV error
#define        FATAL_ERROR_DRBG                            (10)
#define        FATAL_ERROR_FORCED                          (666)
//
//     These are the crypto assertion routines. When a function returns an unexpected and unrecoverable
//     result, the assertion fails and the TpmFail() is called
//
void
TpmFail(const char *function, int line, int code);
typedef void    (*FAIL_FUNCTION)(const char *, int, int);
#define FAIL(a) (TpmFail(__FUNCTION__, __LINE__, a))
#if defined(EMPTY_ASSERT)
#   define pAssert(a) ((void)0)
#else
#   define pAssert(a) {if (!(a)) { FAIL(FATAL_ERROR_PARAMETER);}}
#endif
#endif // _TPM_ERROR_H
