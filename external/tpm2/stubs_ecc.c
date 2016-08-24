/* This file includes functions that were extracted from the TPM2
 * source, but were present in files not included in compilation.
 */
#include "Global.h"
#include "CryptoEngine.h"

#ifdef TPM_ALG_ECC
#include "CpriDataEcc.h"
#include "CpriDataEcc.c"

const ECC_CURVE *_cpri__EccGetParametersByCurveId(
  TPM_ECC_CURVE curveId         // IN: the curveID
  )
{
   int          i;
   for(i = 0; i < ECC_CURVE_COUNT; i++)
   {
       if(eccCurves[i].curveId == curveId)
           return &eccCurves[i];
   }
   FAIL(FATAL_ERROR_INTERNAL);

   return NULL; // Never reached.
}

TPM_ECC_CURVE _cpri__GetCurveIdByIndex(
  UINT16 i)
{
    if(i >= ECC_CURVE_COUNT)
        return TPM_ECC_NONE;
    return eccCurves[i].curveId;
}

UINT32 _cpri__EccGetCurveCount(
  void)
{
    return ECC_CURVE_COUNT;
}

#endif // TPM_ALG_ECC
