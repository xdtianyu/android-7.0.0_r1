// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include <stdlib.h>

#include "CryptoEngine.h"
#include "OsslCryptoEngine.h"
static void Trap(const char *function, int line, int code);
FAIL_FUNCTION       TpmFailFunction = (FAIL_FUNCTION)&Trap;
//
//
//          Functions
//
//          FAILURE_TRAP()
//
//     This function is called if the caller to _cpri__InitCryptoUnits() doesn't provide a call back address.
//
static void
Trap(
     const char          *function,
     int                  line,
     int                  code
     )
{
     UNREFERENCED(function);
     UNREFERENCED(line);
     UNREFERENCED(code);
     abort();
}
//
//
//          _cpri__InitCryptoUnits()
//
//     This function calls the initialization functions of the other crypto modules that are part of the crypto engine
//     for this implementation. This function should be called as a result of _TPM_Init(). The parameter to this
//     function is a call back function it TPM.lib that is called when the crypto engine has a failure.
//
LIB_EXPORT CRYPT_RESULT
_cpri__InitCryptoUnits(
     FAIL_FUNCTION        failFunction
     )
{
   TpmFailFunction = failFunction;
   _cpri__RngStartup();
   _cpri__HashStartup();
   _cpri__SymStartup();
#ifdef TPM_ALG_RSA
   _cpri__RsaStartup();
#endif
#ifdef TPM_ALG_ECC
   _cpri__EccStartup();
#endif
   return CRYPT_SUCCESS;
}
//
//
//          _cpri__StopCryptoUnits()
//
//     This function calls the shutdown functions of the other crypto modules that are part of the crypto engine
//     for this implementation.
//
LIB_EXPORT void
_cpri__StopCryptoUnits(
   void
   )
{
   return;
}
//
//
//          _cpri__Startup()
//
//     This function calls the startup functions of the other crypto modules that are part of the crypto engine for
//     this implementation. This function should be called during processing of TPM2_Startup().
//
LIB_EXPORT BOOL
_cpri__Startup(
   void
   )
{
   return(       _cpri__HashStartup()
              && _cpri__RngStartup()
#ifdef     TPM_ALG_RSA
              && _cpri__RsaStartup()
#endif     // TPM_ALG_RSA
#ifdef     TPM_ALG_ECC
              && _cpri__EccStartup()
#endif     // TPM_ALG_ECC
              && _cpri__SymStartup());
}
