// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include <string.h>

#include   "OsslCryptoEngine.h"

#ifdef TPM_ALG_ECC
#include   "CpriDataEcc.h"
#include   "CpriDataEcc.c"
//
//
//      Functions
//
//      _cpri__EccStartup()
//
//     This function is called at TPM Startup to initialize the crypto units.
//     In this implementation, no initialization is performed at startup but a future version may initialize the self-
//     test functions here.
//
LIB_EXPORT BOOL
_cpri__EccStartup(
    void
    )
{
    return TRUE;
}
//
//
//      _cpri__GetCurveIdByIndex()
//
//     This function returns the number of the i-th implemented curve. The normal use would be to call this
//     function with i starting at 0. When the i is greater than or equal to the number of implemented curves,
//     TPM_ECC_NONE is returned.
//
LIB_EXPORT TPM_ECC_CURVE
_cpri__GetCurveIdByIndex(
    UINT16                i
    )
{
    if(i >= ECC_CURVE_COUNT)
        return TPM_ECC_NONE;
    return eccCurves[i].curveId;
}
LIB_EXPORT UINT32
_cpri__EccGetCurveCount(
    void
    )
{
    return ECC_CURVE_COUNT;
}
//
//
//      _cpri__EccGetParametersByCurveId()
//
//     This function returns a pointer to the curve data that is associated with the indicated curveId. If there is no
//     curve with the indicated ID, the function returns NULL.
//
//
//
//
//     Return Value                      Meaning
//
//     NULL                              curve with the      indicated   TPM_ECC_CURVE    value   is   not
//                                       implemented
//     non-NULL                          pointer to the curve data
//
LIB_EXPORT const ECC_CURVE *
_cpri__EccGetParametersByCurveId(
   TPM_ECC_CURVE       curveId               // IN: the curveID
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
static const ECC_CURVE_DATA *
GetCurveData(
   TPM_ECC_CURVE       curveId               // IN: the curveID
   )
{
   const ECC_CURVE     *curve = _cpri__EccGetParametersByCurveId(curveId);
   return curve->curveData;
}
//
//
//      Point2B()
//
//     This function makes a TPMS_ECC_POINT from a BIGNUM EC_POINT.
//
static BOOL
Point2B(
   EC_GROUP           *group,                //   IN: group for the point
   TPMS_ECC_POINT     *p,                    //   OUT: receives the converted point
   EC_POINT           *ecP,                  //   IN: the point to convert
   INT16               size,                 //   IN: size of the coordinates
   BN_CTX             *context               //   IN: working context
   )
{
   BIGNUM             *bnX;
   BIGNUM             *bnY;
   BN_CTX_start(context);
   bnX = BN_CTX_get(context);
   bnY = BN_CTX_get(context);
   if(        bnY == NULL
        // Get the coordinate values
       || EC_POINT_get_affine_coordinates_GFp(group, ecP, bnX, bnY, context) != 1
       // Convert x
       || (!BnTo2B(&p->x.b, bnX, size))
       // Convert y
       || (!BnTo2B(&p->y.b, bnY, size))
      )
            FAIL(FATAL_ERROR_INTERNAL);
   BN_CTX_end(context);
   return TRUE;
}
//
//
//       EccCurveInit()
//
//      This function initializes the OpenSSL() group definition structure
//      This function is only used within this file.
//      It is a fatal error if groupContext is not provided.
//
//      Return Value                       Meaning
//
//      NULL                               the TPM_ECC_CURVE is not valid
//      non-NULL                           points to a structure in groupContext static EC_GROUP *
//
static EC_GROUP *
EccCurveInit(
    TPM_ECC_CURVE         curveId,             // IN: the ID of the curve
    BN_CTX               *groupContext         // IN: the context in which the group is to be
                                               //     created
    )
{
    const ECC_CURVE_DATA            *curveData = GetCurveData(curveId);
    EC_GROUP                        *group = NULL;
    EC_POINT                        *P = NULL;
    BN_CTX                          *context;
    BIGNUM                          *bnP;
    BIGNUM                          *bnA;
    BIGNUM                          *bnB;
    BIGNUM                          *bnX;
    BIGNUM                          *bnY;
    BIGNUM                          *bnN;
    BIGNUM                          *bnH;
    int                              ok = FALSE;
    // Context must be provided and curve selector must be valid
    pAssert(groupContext != NULL && curveData != NULL);
    context = BN_CTX_new();
    if(context == NULL)
        FAIL(FATAL_ERROR_ALLOCATION);
    BN_CTX_start(context);
    bnP = BN_CTX_get(context);
    bnA = BN_CTX_get(context);
    bnB = BN_CTX_get(context);
    bnX = BN_CTX_get(context);
    bnY = BN_CTX_get(context);
    bnN = BN_CTX_get(context);
    bnH = BN_CTX_get(context);
    if (bnH == NULL)
        goto Cleanup;
    // Convert the number formats
    BnFrom2B(bnP,      curveData->p);
    BnFrom2B(bnA,      curveData->a);
    BnFrom2B(bnB,      curveData->b);
    BnFrom2B(bnX,      curveData->x);
    BnFrom2B(bnY,      curveData->y);
    BnFrom2B(bnN,      curveData->n);
    BnFrom2B(bnH,      curveData->h);
   // initialize EC group, associate a generator point and initialize the point
   // from the parameter data
   ok = (   (group = EC_GROUP_new_curve_GFp(bnP, bnA, bnB, groupContext)) != NULL
         && (P = EC_POINT_new(group)) != NULL
         && EC_POINT_set_affine_coordinates_GFp(group, P, bnX, bnY, groupContext)
         && EC_GROUP_set_generator(group, P, bnN, bnH)
        );
Cleanup:
   if (!ok && group != NULL)
   {
       EC_GROUP_free(group);
       group = NULL;
   }
   if(P != NULL)
       EC_POINT_free(P);
   BN_CTX_end(context);
   BN_CTX_free(context);
   return group;
}
//
//
//       PointFrom2B()
//
//      This function sets the coordinates of an existing BN Point from a TPMS_ECC_POINT.
//
static EC_POINT *
PointFrom2B(
   EC_GROUP           *group,           //   IN:   the group for the point
   EC_POINT           *ecP,             //   IN:   an existing BN point in the group
   TPMS_ECC_POINT     *p,               //   IN:   the 2B coordinates of the point
   BN_CTX             *context          //   IN:   the BIGNUM context
   )
{
   BIGNUM             *bnX;
   BIGNUM             *bnY;
   // If the point is not allocated then just return a NULL
   if(ecP == NULL)
       return NULL;
   BN_CTX_start(context);
   bnX = BN_CTX_get(context);
   bnY = BN_CTX_get(context);
   if( // Set the coordinates of the point
         bnY == NULL
      || BN_bin2bn(p->x.t.buffer, p->x.t.size, bnX) == NULL
      || BN_bin2bn(p->y.t.buffer, p->y.t.size, bnY) == NULL
      || !EC_POINT_set_affine_coordinates_GFp(group, ecP, bnX, bnY, context)
      )
      FAIL(FATAL_ERROR_INTERNAL);
   BN_CTX_end(context);
   return ecP;
}
//
//
//       EccInitPoint2B()
//
//      This function allocates a point in the provided group and initializes it with the values in a
//      TPMS_ECC_POINT.
//
static EC_POINT *
EccInitPoint2B(
   EC_GROUP           *group,           // IN: group for the point
   TPMS_ECC_POINT     *p,               // IN: the coordinates for the point
    BN_CTX              *context                // IN: the BIGNUM context
    )
{
    EC_POINT            *ecP;
    BN_CTX_start(context);
    ecP = EC_POINT_new(group);
    if(PointFrom2B(group, ecP, p, context) == NULL)
        FAIL(FATAL_ERROR_INTERNAL);
    BN_CTX_end(context);
    return ecP;
}
//
//
//       PointMul()
//
//      This function does a point multiply and checks for the result being the point at infinity. Q = ([A]G + [B]P)
//
//      Return Value                      Meaning
//
//      CRYPT_NO_RESULT                   point is at infinity
//      CRYPT_SUCCESS                     point not at infinity
//
static CRYPT_RESULT
PointMul(
    EC_GROUP            *group,                 //      IN: group curve
    EC_POINT            *ecpQ,                  //      OUT: result
    BIGNUM              *bnA,                   //      IN: scalar for [A]G
    EC_POINT            *ecpP,                  //      IN: point for [B]P
    BIGNUM              *bnB,                   //      IN: scalar for [B]P
    BN_CTX              *context                //      IN: working context
    )
{
       if(EC_POINT_mul(group, ecpQ, bnA, ecpP, bnB, context) != 1)
            FAIL(FATAL_ERROR_INTERNAL);
        if(EC_POINT_is_at_infinity(group, ecpQ))
            return CRYPT_NO_RESULT;
        return CRYPT_SUCCESS;
}
//
//
//       GetRandomPrivate()
//
//      This function gets a random value (d) to use as a private ECC key and then qualifies the key so that it is
//      between 0 < d < n.
//      It is a fatal error if dOut or pIn is not provided or if the size of pIn is larger than MAX_ECC_KEY_BYTES
//      (the largest buffer size of a TPM2B_ECC_PARAMETER)
//
static void
GetRandomPrivate(
    TPM2B_ECC_PARAMETER            *dOut,                    // OUT: the qualified random value
    const TPM2B                    *pIn                      // IN: the maximum value for the key
    )
{
    int             i;
    BYTE           *pb;
    pAssert(pIn != NULL && dOut != NULL && pIn->size <= MAX_ECC_KEY_BYTES);
    // Set the size of the output
    dOut->t.size = pIn->size;
    // Get some random bits
    while(TRUE)
    {
        _cpri__GenerateRandom(dOut->t.size, dOut->t.buffer);
        // See if the d < n
        if(memcmp(dOut->t.buffer, pIn->buffer, pIn->size) < 0)
        {
            // dOut < n so make sure that 0 < dOut
            for(pb = dOut->t.buffer, i = dOut->t.size; i > 0; i--)
            {
                if(*pb++ != 0)
                    return;
            }
        }
    }
}
//
//
//       _cpri__EccPointMultiply
//
//      This function computes 'R := [dIn]G + [uIn]QIn. Where dIn and uIn are scalars, G and QIn are points on
//      the specified curve and G is the default generator of the curve.
//      The xOut and yOut parameters are optional and may be set to NULL if not used.
//      It is not necessary to provide uIn if QIn is specified but one of uIn and dIn must be provided. If dIn and
//      QIn are specified but uIn is not provided, then R = [dIn]QIn.
//      If the multiply produces the point at infinity, the CRYPT_NO_RESULT is returned.
//      The sizes of xOut and yOut' will be set to be the size of the degree of the curve
//      It is a fatal error if dIn and uIn are both unspecified (NULL) or if Qin or Rout is unspecified.
//
//
//
//
//      Return Value                    Meaning
//
//      CRYPT_SUCCESS                   point multiplication succeeded
//      CRYPT_POINT                     the point Qin is not on the curve
//      CRYPT_NO_RESULT                 the product point is at infinity
//
LIB_EXPORT CRYPT_RESULT
_cpri__EccPointMultiply(
   TPMS_ECC_POINT                *Rout,                  //   OUT: the product point R
   TPM_ECC_CURVE                  curveId,               //   IN: the curve to use
   TPM2B_ECC_PARAMETER           *dIn,                   //   IN: value to multiply against the
                                                         //       curve generator
   TPMS_ECC_POINT                *Qin,                   //   IN: point Q
   TPM2B_ECC_PARAMETER           *uIn                    //   IN: scalar value for the multiplier
                                                         //       of Q
   )
{
   BN_CTX                    *context;
   BIGNUM                    *bnD;
   BIGNUM                    *bnU;
   EC_GROUP                  *group;
   EC_POINT                  *R = NULL;
   EC_POINT                  *Q = NULL;
   CRYPT_RESULT               retVal = CRYPT_SUCCESS;
   // Validate that the required parameters are provided.
   pAssert((dIn != NULL || uIn != NULL) && (Qin != NULL || dIn != NULL));
   // If a point is provided for the multiply, make sure that it is on the curve
   if(Qin != NULL && !_cpri__EccIsPointOnCurve(curveId, Qin))
       return CRYPT_POINT;
   context = BN_CTX_new();
   if(context == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BN_CTX_start(context);
   bnU = BN_CTX_get(context);
   bnD = BN_CTX_get(context);
   group = EccCurveInit(curveId, context);
   // There should be no path for getting a bad curve ID into this function.
   pAssert(group != NULL);
   // check allocations should have worked and allocate R
   if(   bnD == NULL
      || (R = EC_POINT_new(group)) == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   // If Qin is present, create the point
   if(Qin != NULL)
   {
       // Assume the size variables do not overflow. This should not happen in
       // the contexts in which this function will be called.
       assert2Bsize(Qin->x.t);
       assert2Bsize(Qin->x.t);
       Q = EccInitPoint2B(group, Qin, context);
   }
   if(dIn != NULL)
   {
       // Assume the size variables do not overflow, which should not happen in
       // the contexts that this function will be called.
       assert2Bsize(dIn->t);
        BnFrom2B(bnD, &dIn->b);
    }
    else
        bnD = NULL;
    // If uIn is specified, initialize its BIGNUM
    if(uIn != NULL)
    {
        // Assume the size variables do not overflow, which should not happen in
        // the contexts that this function will be called.
        assert2Bsize(uIn->t);
        BnFrom2B(bnU, &uIn->b);
    }
    // If uIn is not specified but Q is, then we are going to
    // do R = [d]Q
    else if(Qin != NULL)
    {
        bnU = bnD;
        bnD = NULL;
    }
    // If neither Q nor u is specified, then null this pointer
    else
        bnU = NULL;
    // Use the generator of the curve
    if((retVal = PointMul(group, R, bnD, Q, bnU, context)) == CRYPT_SUCCESS)
        Point2B(group, Rout, R, (INT16) ((EC_GROUP_get_degree(group)+7)/8), context);
    if (Q)
        EC_POINT_free(Q);
    if(R)
        EC_POINT_free(R);
    if(group)
        EC_GROUP_free(group);
    BN_CTX_end(context);
    BN_CTX_free(context);
    return retVal;
}
//
//
//       ClearPoint2B()
//
//      Initialize the size values of a point
//
static void
ClearPoint2B(
    TPMS_ECC_POINT       *p                 // IN: the point
    )
{
    if(p != NULL) {
        p->x.t.size = 0;
        p->y.t.size = 0;
    }
}
#if defined TPM_ALG_ECDAA || defined TPM_ALG_SM2 //%
//
//
//       _cpri__EccCommitCompute()
//
//      This function performs the point multiply operations required by TPM2_Commit().
//      If B or M is provided, they must be on the curve defined by curveId. This routine does not check that they
//      are on the curve and results are unpredictable if they are not.
//
//
//
//      It is a fatal error if r or d is NULL. If B is not NULL, then it is a fatal error if K and L are both NULL. If M is
//      not NULL, then it is a fatal error if E is NULL.
//
//      Return Value                       Meaning
//
//      CRYPT_SUCCESS                      computations completed normally
//      CRYPT_NO_RESULT                    if K, L or E was computed to be the point at infinity
//      CRYPT_CANCEL                       a cancel indication was asserted during this function
//
LIB_EXPORT CRYPT_RESULT
_cpri__EccCommitCompute(
    TPMS_ECC_POINT                  *K,                   //   OUT: [d]B or [r]Q
    TPMS_ECC_POINT                  *L,                   //   OUT: [r]B
    TPMS_ECC_POINT                  *E,                   //   OUT: [r]M
    TPM_ECC_CURVE                    curveId,             //   IN: the curve for the computations
    TPMS_ECC_POINT                  *M,                   //   IN: M (optional)
    TPMS_ECC_POINT                  *B,                   //   IN: B (optional)
    TPM2B_ECC_PARAMETER             *d,                   //   IN: d (required)
    TPM2B_ECC_PARAMETER             *r                    //   IN: the computed r value (required)
    )
{
    BN_CTX                    *context;
    BIGNUM                    *bnY, *bnR, *bnD;
    EC_GROUP                  *group;
    EC_POINT                  *pK = NULL, *pL = NULL, *pE = NULL, *pM = NULL, *pB = NULL;
    UINT16                     keySizeInBytes;
    CRYPT_RESULT               retVal = CRYPT_SUCCESS;
    // Validate that the required parameters are provided.
    // Note: E has to be provided if computing E := [r]Q or E := [r]M. Will do
    // E := [r]Q if both M and B are NULL.

    pAssert((r && (K || !B) && (L || !B)) || (E || (!M && B)));
    context = BN_CTX_new();
    if(context == NULL)
        FAIL(FATAL_ERROR_ALLOCATION);
    BN_CTX_start(context);
    bnR = BN_CTX_get(context);
    bnD = BN_CTX_get(context);
    bnY = BN_CTX_get(context);
    if(bnY == NULL)
        FAIL(FATAL_ERROR_ALLOCATION);
    // Initialize the output points in case they are not computed
    ClearPoint2B(K);
    ClearPoint2B(L);
    ClearPoint2B(E);
    if((group = EccCurveInit(curveId, context)) == NULL)
    {
        retVal = CRYPT_PARAMETER;
        goto Cleanup2;
    }
    keySizeInBytes = (UINT16) ((EC_GROUP_get_degree(group)+7)/8);
    // Sizes of the r and d parameters may not be zero
    pAssert(((int) r->t.size > 0) && ((int) d->t.size > 0));
    // Convert scalars to BIGNUM
    BnFrom2B(bnR, &r->b);
    BnFrom2B(bnD, &d->b);
   // If B is provided, compute K=[d]B and L=[r]B
   if(B != NULL)
   {
       // Allocate the points to receive the value
       if(    (pK = EC_POINT_new(group)) == NULL
           || (pL = EC_POINT_new(group)) == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
       // need to compute K = [d]B
       // Allocate and initialize BIGNUM version of B
       pB = EccInitPoint2B(group, B, context);
        // do the math for K = [d]B
        if((retVal = PointMul(group, pK, NULL, pB, bnD, context)) != CRYPT_SUCCESS)
            goto Cleanup;
        // Convert BN K to TPM2B K
        Point2B(group, K, pK, (INT16)keySizeInBytes, context);
        // compute L= [r]B after checking for cancel
        if(_plat__IsCanceled())
        {
            retVal = CRYPT_CANCEL;
            goto Cleanup;
        }
        // compute L = [r]B
        if((retVal = PointMul(group, pL, NULL, pB, bnR, context)) != CRYPT_SUCCESS)
            goto Cleanup;
        // Convert BN L to TPM2B L
        Point2B(group, L, pL, (INT16)keySizeInBytes, context);
   }
   if(M != NULL || B == NULL)
   {
       // if this is the third point multiply, check for cancel first
       if(B != NULL && _plat__IsCanceled())
       {
           retVal = CRYPT_CANCEL;
           goto Cleanup;
       }
        // Allocate E
        if((pE = EC_POINT_new(group)) == NULL)
            FAIL(FATAL_ERROR_ALLOCATION);
        // Create BIGNUM version of M unless M is NULL
        if(M != NULL)
        {
             // M provided so initialize a BIGNUM M and compute E = [r]M
             pM = EccInitPoint2B(group, M, context);
             retVal = PointMul(group, pE, NULL, pM, bnR, context);
        }
        else
             // compute E = [r]G (this is only done if M and B are both NULL
             retVal = PointMul(group, pE, bnR, NULL, NULL, context);
        if(retVal == CRYPT_SUCCESS)
            // Convert E to 2B format
            Point2B(group, E, pE, (INT16)keySizeInBytes, context);
   }
Cleanup:
   EC_GROUP_free(group);
   if(pK != NULL) EC_POINT_free(pK);
   if(pL != NULL) EC_POINT_free(pL);
   if(pE != NULL) EC_POINT_free(pE);
   if(pM != NULL) EC_POINT_free(pM);
   if(pB != NULL) EC_POINT_free(pB);
Cleanup2:
   BN_CTX_end(context);
   BN_CTX_free(context);
   return retVal;
}
#endif //%
//
//
//       _cpri__EccIsPointOnCurve()
//
//      This function is used to test if a point is on a defined curve. It does this by checking that y^2 mod p = x^3
//      + a*x + b mod p
//      It is a fatal error if Q is not specified (is NULL).
//
//      Return Value                        Meaning
//
//      TRUE                                point is on curve
//      FALSE                               point is not on curve or curve is not supported
//
LIB_EXPORT BOOL
_cpri__EccIsPointOnCurve(
    TPM_ECC_CURVE          curveId,             // IN: the curve selector
    TPMS_ECC_POINT        *Q                    // IN: the point.
    )
{
    BN_CTX                           *context;
    BIGNUM                           *bnX;
    BIGNUM                           *bnY;
    BIGNUM                           *bnA;
    BIGNUM                           *bnB;
    BIGNUM                           *bnP;
    BIGNUM                           *bn3;
    const ECC_CURVE_DATA             *curveData = GetCurveData(curveId);
    BOOL                              retVal;
    pAssert(Q != NULL && curveData != NULL);
    if((context = BN_CTX_new()) == NULL)
        FAIL(FATAL_ERROR_ALLOCATION);
    BN_CTX_start(context);
    bnX = BN_CTX_get(context);
    bnY = BN_CTX_get(context);
    bnA = BN_CTX_get(context);
    bnB = BN_CTX_get(context);
    bn3 = BN_CTX_get(context);
    bnP = BN_CTX_get(context);
    if(bnP == NULL)
        FAIL(FATAL_ERROR_ALLOCATION);
    // Convert values
    if (    !BN_bin2bn(Q->x.t.buffer, Q->x.t.size, bnX)
         || !BN_bin2bn(Q->y.t.buffer, Q->y.t.size, bnY)
         || !BN_bin2bn(curveData->p->buffer, curveData->p->size, bnP)
         || !BN_bin2bn(curveData->a->buffer, curveData->a->size, bnA)
         || !BN_set_word(bn3, 3)
         || !BN_bin2bn(curveData->b->buffer, curveData->b->size, bnB)
       )
         FAIL(FATAL_ERROR_INTERNAL);
    // The following sequence is probably not optimal but it seems to be correct.
    // compute x^3 + a*x + b mod p
            // first, compute a*x mod p
    if(   !BN_mod_mul(bnA, bnA, bnX, bnP, context)
//
              // next, compute a*x + b mod p
         || !BN_mod_add(bnA, bnA, bnB, bnP, context)
              // next, compute X^3 mod p
         || !BN_mod_exp(bnX, bnX, bn3, bnP, context)
              // finally, compute x^3 + a*x + b mod p
         || !BN_mod_add(bnX, bnX, bnA, bnP, context)
              // then compute y^2
         || !BN_mod_mul(bnY, bnY, bnY, bnP, context)
        )
          FAIL(FATAL_ERROR_INTERNAL);
    retVal = BN_cmp(bnX, bnY) == 0;
    BN_CTX_end(context);
    BN_CTX_free(context);
    return retVal;
}
//
//
//       _cpri__GenerateKeyEcc()
//
//      This function generates an ECC key pair based on the input parameters. This routine uses KDFa() to
//      produce candidate numbers. The method is according to FIPS 186-3, section B.4.1 "GKey() Pair
//      Generation Using Extra Random Bits." According to the method in FIPS 186-3, the resulting private value
//      d should be 1 <= d < n where n is the order of the base point. In this implementation, the range of the
//      private value is further restricted to be 2^(nLen/2) <= d < n where nLen is the order of n.
//
//      EXAMPLE:         If the curve is NIST-P256, then nLen is 256 bits and d will need to be between 2^128 <= d < n
//
//      It is a fatal error if Qout, dOut, or seed is not provided (is NULL).
//
//      Return Value                         Meaning
//
//      CRYPT_PARAMETER                      the hash algorithm is not supported
//
LIB_EXPORT CRYPT_RESULT
_cpri__GenerateKeyEcc(
    TPMS_ECC_POINT                    *Qout,                  //   OUT: the public point
    TPM2B_ECC_PARAMETER               *dOut,                  //   OUT: the private scalar
    TPM_ECC_CURVE                      curveId,               //   IN: the curve identifier
    TPM_ALG_ID                         hashAlg,               //   IN: hash algorithm to use in the key
                                                              //       generation process
    TPM2B                             *seed,                  //   IN: the seed to use
    const char                        *label,                 //   IN: A label for the generation
                                                              //       process.
    TPM2B                             *extra,                 //   IN: Party 1 data for the KDF
    UINT32                            *counter                //   IN/OUT: Counter value to allow KDF
                                                              //       iteration to be propagated across
                                                              //       multiple functions
    )
{
    const ECC_CURVE_DATA              *curveData = GetCurveData(curveId);
    INT16                              keySizeInBytes;
    UINT32                             count = 0;
    CRYPT_RESULT                       retVal;
    UINT16                             hLen = _cpri__GetDigestSize(hashAlg);
    BIGNUM                            *bnNm1;          // Order of the curve minus one
    BIGNUM                            *bnD;            // the private scalar
    BN_CTX                            *context;        // the context for the BIGNUM values
    BYTE                               withExtra[MAX_ECC_KEY_BYTES + 8]; // trial key with
                                                                           //extra bits
    TPM2B_4_BYTE_VALUE                 marshaledCounter = {.t = {4}};
    UINT32                             totalBits;
    // Validate parameters (these are fatal)
   pAssert(     seed != NULL && dOut != NULL && Qout != NULL && curveData != NULL);
   // Non-fatal parameter checks.
   if(hLen <= 0)
       return CRYPT_PARAMETER;
   // allocate the local BN values
   context = BN_CTX_new();
   if(context == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BN_CTX_start(context);
   bnNm1 = BN_CTX_get(context);
   bnD = BN_CTX_get(context);
   // The size of the input scalars is limited by the size of the size of a
   // TPM2B_ECC_PARAMETER. Make sure that it is not irrational.
   pAssert((int) curveData->n->size <= MAX_ECC_KEY_BYTES);
   if(   bnD == NULL
      || BN_bin2bn(curveData->n->buffer, curveData->n->size, bnNm1) == NULL
      || (keySizeInBytes = (INT16) BN_num_bytes(bnNm1)) > MAX_ECC_KEY_BYTES)
       FAIL(FATAL_ERROR_INTERNAL);
   // get the total number of bits
   totalBits = BN_num_bits(bnNm1) + 64;
   // Reduce bnNm1 from 'n' to 'n' - 1
   BN_sub_word(bnNm1, 1);
   // Initialize the count value
   if(counter != NULL)
       count = *counter;
   if(count == 0)
       count = 1;
   // Start search for key (should be quick)
   for(; count != 0; count++)
   {
        UINT32_TO_BYTE_ARRAY(count, marshaledCounter.t.buffer);
        _cpri__KDFa(hashAlg, seed, label, extra, &marshaledCounter.b,
                    totalBits, withExtra, NULL, FALSE);
        // Convert the result and modular reduce
        // Assume the size variables do not overflow, which should not happen in
        // the contexts that this function will be called.
        pAssert(keySizeInBytes <= MAX_ECC_KEY_BYTES);
        if (    BN_bin2bn(withExtra, keySizeInBytes+8, bnD) == NULL
             || BN_mod(bnD, bnD, bnNm1, context) != 1)
             FAIL(FATAL_ERROR_INTERNAL);
        // Add one to get 0 < d < n
        BN_add_word(bnD, 1);
        if(BnTo2B(&dOut->b, bnD, keySizeInBytes) != 1)
                FAIL(FATAL_ERROR_INTERNAL);
        // Do the point multiply to create the public portion of the key. If
        // the multiply generates the point at infinity (unlikely), do another
        // iteration.
        if(    (retVal = _cpri__EccPointMultiply(Qout, curveId, dOut, NULL, NULL))
            != CRYPT_NO_RESULT)
            break;
   }
   if(count == 0) // if counter wrapped, then the TPM should go into failure mode
       FAIL(FATAL_ERROR_INTERNAL);
   // Free up allocated BN values
   BN_CTX_end(context);
   BN_CTX_free(context);
   if(counter != NULL)
       *counter = count;
   return retVal;
}
//
//
//       _cpri__GetEphemeralEcc()
//
//      This function creates an ephemeral ECC. It is ephemeral in that is expected that the private part of the
//      key will be discarded
//
LIB_EXPORT CRYPT_RESULT
_cpri__GetEphemeralEcc(
   TPMS_ECC_POINT                *Qout,            // OUT: the public point
   TPM2B_ECC_PARAMETER           *dOut,            // OUT: the private scalar
   TPM_ECC_CURVE                  curveId          // IN: the curve for the key
   )
{
   CRYPT_RESULT                   retVal;
   const ECC_CURVE_DATA          *curveData = GetCurveData(curveId);
   pAssert(curveData != NULL);
   // Keep getting random values until one is found that doesn't create a point
   // at infinity. This will never, ever, ever, ever, ever, happen but if it does
   // we have to get a next random value.
   while(TRUE)
   {
       GetRandomPrivate(dOut, curveData->p);
        // _cpri__EccPointMultiply does not return CRYPT_ECC_POINT if no point is
        // provided. CRYPT_PARAMTER should not be returned because the curve ID
        // has to be supported. Thus the only possible error is CRYPT_NO_RESULT.
        retVal = _cpri__EccPointMultiply(Qout, curveId, dOut, NULL, NULL);
        if(retVal != CRYPT_NO_RESULT)
            return retVal; // Will return CRYPT_SUCCESS
   }
}
#ifdef TPM_ALG_ECDSA      //%
//
//
//       SignEcdsa()
//
//      This function implements the ECDSA signing algorithm. The method is described in the comments below.
//      It is a fatal error if rOut, sOut, dIn, or digest are not provided.
//
LIB_EXPORT CRYPT_RESULT
SignEcdsa(
   TPM2B_ECC_PARAMETER           *rOut,            //   OUT: r component of the signature
   TPM2B_ECC_PARAMETER           *sOut,            //   OUT: s component of the signature
   TPM_ECC_CURVE                  curveId,         //   IN: the curve used in the signature
                                                   //       process
   TPM2B_ECC_PARAMETER           *dIn,             //   IN: the private key
   TPM2B                         *digest           //   IN: the value to sign
   )
{
   BIGNUM                        *bnK;
   BIGNUM                        *bnIk;
   BIGNUM                        *bnN;
   BIGNUM                        *bnR;
//
    BIGNUM                    *bnD;
    BIGNUM                    *bnZ;
    TPM2B_ECC_PARAMETER        k;
    TPMS_ECC_POINT             R;
    BN_CTX                    *context;
    CRYPT_RESULT               retVal = CRYPT_SUCCESS;
    const ECC_CURVE_DATA      *curveData = GetCurveData(curveId);
    pAssert(rOut != NULL && sOut != NULL && dIn != NULL && digest != NULL);
    context = BN_CTX_new();
    if(context == NULL)
        FAIL(FATAL_ERROR_ALLOCATION);
    BN_CTX_start(context);
    bnN = BN_CTX_get(context);
    bnZ = BN_CTX_get(context);
    bnR = BN_CTX_get(context);
    bnD = BN_CTX_get(context);
    bnIk = BN_CTX_get(context);
    bnK = BN_CTX_get(context);
    // Assume the size variables do not overflow, which should not happen in
    // the contexts that this function will be called.
    pAssert(curveData->n->size <= MAX_ECC_PARAMETER_BYTES);
    if(   bnK == NULL
       || BN_bin2bn(curveData->n->buffer, curveData->n->size, bnN) == NULL)
        FAIL(FATAL_ERROR_INTERNAL);
//   The algorithm as described in "Suite B Implementer's Guide to FIPS 186-3(ECDSA)"
//   1. Use one of the routines in Appendix A.2 to generate (k, k^-1), a per-message
//      secret number and its inverse modulo n. Since n is prime, the
//      output will be invalid only if there is a failure in the RBG.
//   2. Compute the elliptic curve point R = [k]G = (xR, yR) using EC scalar
//      multiplication (see [Routines]), where G is the base point included in
//      the set of domain parameters.
//   3. Compute r = xR mod n. If r = 0, then return to Step 1. 1.
//   4. Use the selected hash function to compute H = Hash(M).
//   5. Convert the bit string H to an integer e as described in Appendix B.2.
//   6. Compute s = (k^-1 * (e + d * r)) mod n. If s = 0, return to Step 1.2.
//   7. Return (r, s).
    // Generate a random value k in the range 1 <= k < n
    // Want a K value that is the same size as the curve order
    k.t.size = curveData->n->size;
    while(TRUE) // This implements the loop at step 6. If s is zero, start over.
    {
        while(TRUE)
        {
            // Step 1 and 2 -- generate an ephemeral key and the modular inverse
            // of the private key.
            while(TRUE)
            {
                GetRandomPrivate(&k, curveData->n);
                  // Do the point multiply to generate a point and check to see if
                  // the point it at infinity
                  if(    _cpri__EccPointMultiply(&R, curveId, &k, NULL, NULL)
                      != CRYPT_NO_RESULT)
                      break; // can only be CRYPT_SUCCESS
              }
              // x coordinate is mod p. Make it mod n
              // Assume the size variables do not overflow, which should not happen
              // in the contexts that this function will be called.
              assert2Bsize(R.x.t);
              BN_bin2bn(R.x.t.buffer, R.x.t.size, bnR);
              BN_mod(bnR, bnR, bnN, context);
              // Make sure that it is not zero;
              if(BN_is_zero(bnR))
                  continue;
              // Make sure that a modular inverse exists
              // Assume the size variables do not overflow, which should not happen
              // in the contexts that this function will be called.
              assert2Bsize(k.t);
              BN_bin2bn(k.t.buffer, k.t.size, bnK);
              if( BN_mod_inverse(bnIk, bnK, bnN, context) != NULL)
                  break;
        }
        // Set z = leftmost bits of the digest
        // NOTE: This is implemented such that the key size needs to be
        //        an even number of bytes in length.
        if(digest->size > curveData->n->size)
        {
             // Assume the size variables do not overflow, which should not happen
             // in the contexts that this function will be called.
             pAssert(curveData->n->size <= MAX_ECC_KEY_BYTES);
             // digest is larger than n so truncate
             BN_bin2bn(digest->buffer, curveData->n->size, bnZ);
        }
        else
        {
             // Assume the size variables do not overflow, which should not happen
             // in the contexts that this function will be called.
             pAssert(digest->size <= MAX_DIGEST_SIZE);
             // digest is same or smaller than n so use it all
             BN_bin2bn(digest->buffer, digest->size, bnZ);
        }
        // Assume the size variables do not overflow, which should not happen in
        // the contexts that this function will be called.
        assert2Bsize(dIn->t);
        if(   bnZ == NULL
             // need the private scalar of the signing key
             || BN_bin2bn(dIn->t.buffer, dIn->t.size, bnD) == NULL)
              FAIL(FATAL_ERROR_INTERNAL);
        //   NOTE: When the result of an operation is going to be reduced mod x
        //   any modular multiplication is done so that the intermediate values
        //   don't get too large.
        //
        // now have inverse of K (bnIk), z (bnZ), r (bnR),      d (bnD) and n (bnN)
        // Compute s = k^-1 (z + r*d)(mod n)
            // first do d = r*d mod n
        if( !BN_mod_mul(bnD, bnR, bnD, bnN, context)
             // d = z + r * d
             || !BN_add(bnD, bnZ, bnD)
             // d = k^(-1)(z + r * d)(mod n)
             || !BN_mod_mul(bnD, bnIk, bnD, bnN, context)
             // convert to TPM2B format
             || !BnTo2B(&sOut->b, bnD, curveData->n->size)
             //   and write the modular reduced version of r
             //   NOTE: this was deferred to reduce the number of
             //   error checks.
             ||   !BnTo2B(&rOut->b, bnR, curveData->n->size))
              FAIL(FATAL_ERROR_INTERNAL);
        if(!BN_is_zero(bnD))
            break; // signature not zero so done
        // if the signature value was zero, start over
   }
   // Free up allocated BN values
   BN_CTX_end(context);
   BN_CTX_free(context);
   return retVal;
}
#endif //%
#if defined TPM_ALG_ECDAA || defined TPM_ALG_ECSCHNORR                //%
//
//
//       EcDaa()
//
//      This function is used to perform a modified Schnorr signature for ECDAA.
//      This function performs s = k + T * d mod n where
//      a) 'k is a random, or pseudo-random value used in the commit phase
//      b) T is the digest to be signed, and
//      c) d is a private key.
//      If tIn is NULL then use tOut as T
//
//      Return Value                        Meaning
//
//      CRYPT_SUCCESS                       signature created
//
static CRYPT_RESULT
EcDaa(
   TPM2B_ECC_PARAMETER              *tOut,             //   OUT: T component of the signature
   TPM2B_ECC_PARAMETER              *sOut,             //   OUT: s component of the signature
   TPM_ECC_CURVE                     curveId,          //   IN: the curve used in signing
   TPM2B_ECC_PARAMETER              *dIn,              //   IN: the private key
   TPM2B                            *tIn,              //   IN: the value to sign
   TPM2B_ECC_PARAMETER              *kIn               //   IN: a random value from commit
   )
{
   BIGNUM                           *bnN, *bnK, *bnT, *bnD;
   BN_CTX                           *context;
   const TPM2B                      *n;
   const ECC_CURVE_DATA             *curveData = GetCurveData(curveId);
   BOOL                              OK = TRUE;
   // Parameter checks
    pAssert(   sOut != NULL && dIn != NULL && tOut != NULL
            && kIn != NULL && curveData != NULL);
   // this just saves key strokes
   n = curveData->n;
   if(tIn != NULL)
       Copy2B(&tOut->b, tIn);
   // The size of dIn and kIn input scalars is limited by the size of the size
   // of a TPM2B_ECC_PARAMETER and tIn can be no larger than a digest.
   // Make sure they are within range.
   pAssert(   (int) dIn->t.size <= MAX_ECC_KEY_BYTES
           && (int) kIn->t.size <= MAX_ECC_KEY_BYTES
//
             && (int) tOut->t.size <= MAX_DIGEST_SIZE
            );
   context = BN_CTX_new();
   if(context == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BN_CTX_start(context);
   bnN = BN_CTX_get(context);
   bnK = BN_CTX_get(context);
   bnT = BN_CTX_get(context);
   bnD = BN_CTX_get(context);
   // Check for allocation problems
   if(bnD == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   // Convert values
   if(   BN_bin2bn(n->buffer, n->size, bnN) == NULL
      || BN_bin2bn(kIn->t.buffer, kIn->t.size, bnK) == NULL
      || BN_bin2bn(dIn->t.buffer, dIn->t.size, bnD) == NULL
      || BN_bin2bn(tOut->t.buffer, tOut->t.size, bnT) == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
   // Compute T = T mod n
   OK = OK && BN_mod(bnT, bnT, bnN, context);
   // compute (s = k + T * d mod n)
           //   d = T * d mod n
   OK = OK && BN_mod_mul(bnD, bnT, bnD, bnN, context) == 1;
           //   d = k + T * d mod n
   OK = OK && BN_mod_add(bnD, bnK, bnD, bnN, context) == 1;
           //   s = d
   OK = OK && BnTo2B(&sOut->b, bnD, n->size);
           //   r = T
   OK = OK && BnTo2B(&tOut->b, bnT, n->size);
   if(!OK)
       FAIL(FATAL_ERROR_INTERNAL);
   // Cleanup
   BN_CTX_end(context);
   BN_CTX_free(context);
   return CRYPT_SUCCESS;
}
#endif //%
#ifdef TPM_ALG_ECSCHNORR //%
//
//
//       Mod2B()
//
//      Function does modular reduction of TPM2B values.
//
static CRYPT_RESULT
Mod2B(
    TPM2B                *x,                 // IN/OUT: value to reduce
    const TPM2B          *n                  // IN: mod
    )
{
    int         compare;
    compare = _math__uComp(x->size, x->buffer, n->size, n->buffer);
    if(compare < 0)
        // if x < n, then mod is x
        return CRYPT_SUCCESS;
    if(compare == 0)
    {
        // if x == n then mod is 0
        x->size = 0;
        x->buffer[0] = 0;
        return CRYPT_SUCCESS;
    }
   return _math__Div(x, n, NULL, x);
}

//
//
//       SchnorrEcc()
//
//      This function is used to perform a modified Schnorr signature.
//      This function will generate a random value k and compute
//      a) (xR, yR) = [k]G
//      b) r = hash(P || xR)(mod n)
//      c) s= k + r * ds
//      d) return the tuple T, s
//
//
//
//
//      Return Value                  Meaning
//
//      CRYPT_SUCCESS                 signature created
//      CRYPT_SCHEME                  hashAlg can't produce zero-length digest
//
static CRYPT_RESULT
SchnorrEcc(
   TPM2B_ECC_PARAMETER        *rOut,               //   OUT: r component of the signature
   TPM2B_ECC_PARAMETER        *sOut,               //   OUT: s component of the signature
   TPM_ALG_ID                  hashAlg,            //   IN: hash algorithm used
   TPM_ECC_CURVE               curveId,            //   IN: the curve used in signing
   TPM2B_ECC_PARAMETER        *dIn,                //   IN: the private key
   TPM2B                      *digest,             //   IN: the digest to sign
   TPM2B_ECC_PARAMETER        *kIn                 //   IN: for testing
   )
{
   TPM2B_ECC_PARAMETER      k;
   BIGNUM                  *bnR, *bnN, *bnK, *bnT, *bnD;
   BN_CTX                  *context;
   const TPM2B             *n;
   EC_POINT                *pR = NULL;
   EC_GROUP                *group = NULL;
   CPRI_HASH_STATE          hashState;
   UINT16                   digestSize = _cpri__GetDigestSize(hashAlg);
   const ECC_CURVE_DATA    *curveData = GetCurveData(curveId);
   TPM2B_TYPE(T, MAX(MAX_DIGEST_SIZE, MAX_ECC_PARAMETER_BYTES));
   TPM2B_T                  T2b;
   BOOL                     OK = TRUE;
   // Parameter checks
   // Must have a place for the 'r' and 's' parts of the signature, a private
   // key ('d')
   pAssert(   rOut != NULL && sOut != NULL && dIn != NULL
           && digest != NULL && curveData != NULL);
   // to save key strokes
   n = curveData->n;
   // If the digest does not produce a hash, then null the signature and return
   // a failure.
   if(digestSize == 0)
   {
       rOut->t.size = 0;
       sOut->t.size = 0;
       return CRYPT_SCHEME;
   }
   // Allocate big number values
   context = BN_CTX_new();
   if(context == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BN_CTX_start(context);
   bnR = BN_CTX_get(context);
   bnN = BN_CTX_get(context);
   bnK = BN_CTX_get(context);
   bnT = BN_CTX_get(context);
   bnD = BN_CTX_get(context);
   if(   bnD == NULL
           // initialize the group parameters
      || (group = EccCurveInit(curveId, context)) == NULL
          // allocate a local point
      || (pR = EC_POINT_new(group)) == NULL
     )
        FAIL(FATAL_ERROR_ALLOCATION);
   if(BN_bin2bn(curveData->n->buffer, curveData->n->size, bnN) == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
   while(OK)
   {
// a) set k to a random value such that 1 k n-1
       if(kIn != NULL)
       {
            Copy2B(&k.b, &kIn->b); // copy input k if testing
            OK = FALSE;              // not OK to loop
       }
       else
       // If get a random value in the correct range
            GetRandomPrivate(&k, n);
        // Convert 'k' and generate pR = ['k']G
        BnFrom2B(bnK, &k.b);
// b) compute E (xE, yE) [k]G
       if(PointMul(group, pR, bnK, NULL, NULL, context) == CRYPT_NO_RESULT)
// c) if E is the point at infinity, go to a)
           continue;
// d) compute e xE (mod n)
       // Get the x coordinate of the point
       EC_POINT_get_affine_coordinates_GFp(group, pR, bnR, NULL, context);
        // make (mod n)
        BN_mod(bnR, bnR, bnN, context);
// e) if e is zero, go to a)
       if(BN_is_zero(bnR))
           continue;
        // Convert xR to a string (use T as a temp)
        BnTo2B(&T2b.b, bnR, (UINT16)(BN_num_bits(bnR)+7)/8);
// f) compute r HschemeHash(P || e) (mod n)
       _cpri__StartHash(hashAlg, FALSE, &hashState);
       _cpri__UpdateHash(&hashState, digest->size, digest->buffer);
       _cpri__UpdateHash(&hashState, T2b.t.size, T2b.t.buffer);
       if(_cpri__CompleteHash(&hashState, digestSize, T2b.b.buffer) != digestSize)
           FAIL(FATAL_ERROR_INTERNAL);
       T2b.t.size = digestSize;
       BnFrom2B(bnT, &T2b.b);
       BN_div(NULL, bnT, bnT, bnN, context);
       BnTo2B(&rOut->b, bnT, (UINT16)BN_num_bytes(bnT));
        // We have a value and we are going to exit the loop successfully
        OK = TRUE;
        break;
   }
   // Cleanup
   EC_POINT_free(pR);
   EC_GROUP_free(group);
   BN_CTX_end(context);
   BN_CTX_free(context);
   // If we have a value, finish the signature
   if(OK)
       return EcDaa(rOut, sOut, curveId, dIn, NULL, &k);
   else
       return CRYPT_NO_RESULT;
}
#endif //%
#ifdef TPM_ALG_SM2 //%
#ifdef _SM2_SIGN_DEBUG //%
static int
cmp_bn2hex(
   BIGNUM              *bn,               // IN: big number value
   const char          *c                 // IN: character string number
   )
{
   int         result;
   BIGNUM      *bnC = BN_new();
   pAssert(bnC != NULL);
   BN_hex2bn(&bnC, c);
   result = BN_ucmp(bn, bnC);
   BN_free(bnC);
   return result;
}
static int
cmp_2B2hex(
   TPM2B               *a,                // IN: TPM2B number to compare
   const char          *c                 // IN: character string
   )
{
   int            result;
   int            sl = strlen(c);
   BIGNUM         *bnA;
   result = (a->size * 2) - sl;
   if(result != 0)
       return result;
   pAssert((bnA = BN_bin2bn(a->buffer, a->size, NULL)) != NULL);
   result = cmp_bn2hex(bnA, c);
   BN_free(bnA);
   return result;
}
static void
cpy_hexTo2B(
   TPM2B               *b,                // OUT: receives value
   const char          *c                 // IN: source string
   )
{
   BIGNUM      *bnB = BN_new();
   pAssert((strlen(c) & 1) == 0);         // must have an even number of digits
   b->size = strlen(c) / 2;
   BN_hex2bn(&bnB, c);
   pAssert(bnB != NULL);
   BnTo2B(b, bnB, b->size);
   BN_free(bnB);
}
#endif //% _SM2_SIGN_DEBUG
//
//
//        SignSM2()
//
//       This function signs a digest using the method defined in SM2 Part 2. The method in the standard will add
//       a header to the message to be signed that is a hash of the values that define the key. This then hashed
//       with the message to produce a digest (e) that is signed. This function signs e.
//
//
//
//
//       Return Value                      Meaning
//
//       CRYPT_SUCCESS                     sign worked
//
static CRYPT_RESULT
SignSM2(
   TPM2B_ECC_PARAMETER            *rOut,                 //   OUT: r component of the signature
   TPM2B_ECC_PARAMETER            *sOut,                 //   OUT: s component of the signature
   TPM_ECC_CURVE                   curveId,              //   IN: the curve used in signing
   TPM2B_ECC_PARAMETER            *dIn,                  //   IN: the private key
   TPM2B                          *digest                //   IN: the digest to sign
   )
{
   BIGNUM                         *bnR;
   BIGNUM                         *bnS;
   BIGNUM                         *bnN;
   BIGNUM                         *bnK;
   BIGNUM                         *bnX1;
   BIGNUM                         *bnD;
   BIGNUM                         *bnT;        // temp
   BIGNUM                         *bnE;
   BN_CTX                  *context;
   TPM2B_ECC_PARAMETER      k;
   TPMS_ECC_POINT           p2Br;
   const ECC_CURVE_DATA    *curveData = GetCurveData(curveId);
   pAssert(curveData != NULL);
   context = BN_CTX_new();
   BN_CTX_start(context);
   bnK = BN_CTX_get(context);
   bnR = BN_CTX_get(context);
   bnS = BN_CTX_get(context);
   bnX1 = BN_CTX_get(context);
   bnN = BN_CTX_get(context);
   bnD = BN_CTX_get(context);
   bnT = BN_CTX_get(context);
   bnE = BN_CTX_get(context);
   if(bnE == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BnFrom2B(bnE, digest);
   BnFrom2B(bnN, curveData->n);
   BnFrom2B(bnD, &dIn->b);
#ifdef _SM2_SIGN_DEBUG
BN_hex2bn(&bnE, "B524F552CD82B8B028476E005C377FB19A87E6FC682D48BB5D42E3D9B9EFFE76");
BN_hex2bn(&bnD, "128B2FA8BD433C6C068C8D803DFF79792A519A55171B1B650C23661D15897263");
#endif
// A3: Use random number generator to generate random number 1 <= k <= n-1;
// NOTE: Ax: numbers are from the SM2 standard
   k.t.size = curveData->n->size;
loop:
   {
       // Get a random number
       _cpri__GenerateRandom(k.t.size, k.t.buffer);
#ifdef _SM2_SIGN_DEBUG
BN_hex2bn(&bnK, "6CB28D99385C175C94F94E934817663FC176D925DD72B727260DBAAE1FB2F96F");
BnTo2B(&k.b,bnK, 32);
k.t.size = 32;
#endif
       //make sure that the number is 0 < k < n
       BnFrom2B(bnK, &k.b);
        if(      BN_ucmp(bnK, bnN) >= 0
              || BN_is_zero(bnK))
              goto loop;
// A4: Figure out the point of elliptic curve (x1, y1)=[k]G, and according
// to details specified in 4.2.7 in Part 1 of this document, transform the
// data type of x1 into an integer;
       if(    _cpri__EccPointMultiply(&p2Br, curveId, &k, NULL, NULL)
           == CRYPT_NO_RESULT)
            goto loop;
        BnFrom2B(bnX1, &p2Br.x.b);
// A5: Figure out r = (e + x1) mod n,
       if(!BN_mod_add(bnR, bnE, bnX1, bnN, context))
           FAIL(FATAL_ERROR_INTERNAL);
#ifdef _SM2_SIGN_DEBUG
pAssert(cmp_bn2hex(bnR,
               "40F1EC59F793D9F49E09DCEF49130D4194F79FB1EED2CAA55BACDB49C4E755D1")
       == 0);
#endif
           // if r=0 or r+k=n, return to A3;
         if(!BN_add(bnT, bnK, bnR))
            FAIL(FATAL_ERROR_INTERNAL);
        if(BN_is_zero(bnR) || BN_ucmp(bnT, bnN) == 0)
            goto loop;
// A6: Figure out s = ((1 + dA)^-1 (k - r dA)) mod n, if s=0, return to A3;
       // compute t = (1+d)-1
       BN_copy(bnT, bnD);
       if(     !BN_add_word(bnT, 1)
           || !BN_mod_inverse(bnT, bnT, bnN, context) // (1 + dA)^-1 mod n
           )
             FAIL(FATAL_ERROR_INTERNAL);
#ifdef _SM2_SIGN_DEBUG
pAssert(cmp_bn2hex(bnT,
                 "79BFCF3052C80DA7B939E0C6914A18CBB2D96D8555256E83122743A7D4F5F956")
       == 0);
#endif
       // compute s = t * (k - r * dA) mod n
       if(     !BN_mod_mul(bnS, bnD, bnR, bnN, context) // (r * dA) mod n
           || !BN_mod_sub(bnS, bnK, bnS, bnN, context) // (k - (r * dA) mod n
           || !BN_mod_mul(bnS, bnT, bnS, bnN, context))// t * (k - (r * dA) mod n
           FAIL(FATAL_ERROR_INTERNAL);
#ifdef _SM2_SIGN_DEBUG
pAssert(cmp_bn2hex(bnS,
                 "6FC6DAC32C5D5CF10C77DFB20F7C2EB667A457872FB09EC56327A67EC7DEEBE7")
       == 0);
#endif
        if(BN_is_zero(bnS))
            goto loop;
   }
// A7: According to details specified in 4.2.1 in Part 1 of this document, transform
// the data type of r, s into bit strings, signature of message M is (r, s).
   BnTo2B(&rOut->b, bnR, curveData->n->size);
   BnTo2B(&sOut->b, bnS, curveData->n->size);
#ifdef _SM2_SIGN_DEBUG
pAssert(cmp_2B2hex(&rOut->b,
               "40F1EC59F793D9F49E09DCEF49130D4194F79FB1EED2CAA55BACDB49C4E755D1")
       == 0);
pAssert(cmp_2B2hex(&sOut->b,
                  "6FC6DAC32C5D5CF10C77DFB20F7C2EB667A457872FB09EC56327A67EC7DEEBE7")
        == 0);
#endif
   BN_CTX_end(context);
   BN_CTX_free(context);
   return CRYPT_SUCCESS;
}
#endif //% TPM_ALG_SM2
//
//
//        _cpri__SignEcc()
//
//       This function is the dispatch function for the various ECC-based signing schemes.
//
//       Return Value                      Meaning
//
//       CRYPT_SCHEME                      scheme is not supported
//
LIB_EXPORT CRYPT_RESULT
_cpri__SignEcc(
   TPM2B_ECC_PARAMETER            *rOut,              //   OUT: r component of the signature
   TPM2B_ECC_PARAMETER            *sOut,              //   OUT: s component of the signature
   TPM_ALG_ID                      scheme,            //   IN: the scheme selector
   TPM_ALG_ID                      hashAlg,           //   IN: the hash algorithm if need
   TPM_ECC_CURVE                   curveId,           //   IN: the curve used in the signature
                                                      //       process
   TPM2B_ECC_PARAMETER            *dIn,               //   IN: the private key
   TPM2B                          *digest,            //   IN: the digest to sign
   TPM2B_ECC_PARAMETER            *kIn                //   IN: k for input
   )
{
   switch (scheme)
   {
       case TPM_ALG_ECDSA:
           // SignEcdsa always works
           return SignEcdsa(rOut, sOut, curveId, dIn, digest);
           break;
#ifdef TPM_ALG_ECDAA
       case TPM_ALG_ECDAA:
           if(rOut != NULL)
                rOut->b.size = 0;
           return EcDaa(rOut, sOut, curveId, dIn, digest, kIn);
           break;
#endif
#ifdef TPM_ALG_ECSCHNORR
       case TPM_ALG_ECSCHNORR:
           return SchnorrEcc(rOut, sOut, hashAlg, curveId, dIn, digest, kIn);
           break;
#endif
#ifdef TPM_ALG_SM2
       case TPM_ALG_SM2:
           return SignSM2(rOut, sOut, curveId, dIn, digest);
           break;
#endif
       default:
           return CRYPT_SCHEME;
   }
}
#ifdef TPM_ALG_ECDSA //%
//
//
//        ValidateSignatureEcdsa()
//
//       This function validates an ECDSA signature. rIn and sIn shoudl have been checked to make sure that
//       they are not zero.
//
//       Return Value                  Meaning
//
//       CRYPT_SUCCESS                 signature valid
//       CRYPT_FAIL                    signature not valid
//
static CRYPT_RESULT
ValidateSignatureEcdsa(
   TPM2B_ECC_PARAMETER        *rIn,                //   IN: r component of the signature
   TPM2B_ECC_PARAMETER        *sIn,                //   IN: s component of the signature
   TPM_ECC_CURVE               curveId,            //   IN: the curve used in the signature
                                                   //       process
   TPMS_ECC_POINT             *Qin,                //   IN: the public point of the key
   TPM2B                      *digest              //   IN: the digest that was signed
   )
{
   TPM2B_ECC_PARAMETER         U1;
   TPM2B_ECC_PARAMETER         U2;
   TPMS_ECC_POINT              R;
   const TPM2B                *n;
   BN_CTX                     *context;
   EC_POINT                   *pQ = NULL;
   EC_GROUP                   *group = NULL;
   BIGNUM                     *bnU1;
   BIGNUM                     *bnU2;
   BIGNUM                     *bnR;
   BIGNUM                     *bnS;
   BIGNUM                     *bnW;
   BIGNUM                     *bnV;
   BIGNUM                     *bnN;
   BIGNUM                     *bnE;
   BIGNUM                     *bnQx;
   BIGNUM                     *bnQy;
   CRYPT_RESULT                retVal = CRYPT_FAIL;
   int                         t;
   const ECC_CURVE_DATA       *curveData = GetCurveData(curveId);
   // The curve selector should have been filtered by the unmarshaling process
   pAssert (curveData != NULL);
   n = curveData->n;
// 1. If r and s are not both integers in the interval [1, n - 1], output
//    INVALID.
// rIn and sIn are known to be greater than zero (was checked by the caller).
   if(     _math__uComp(rIn->t.size, rIn->t.buffer, n->size, n->buffer) >= 0
       || _math__uComp(sIn->t.size, sIn->t.buffer, n->size, n->buffer) >= 0
     )
      return CRYPT_FAIL;
   context = BN_CTX_new();
   if(context == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BN_CTX_start(context);
   bnR = BN_CTX_get(context);
   bnS = BN_CTX_get(context);
   bnN = BN_CTX_get(context);
   bnE = BN_CTX_get(context);
   bnV = BN_CTX_get(context);
   bnW = BN_CTX_get(context);
   bnQx = BN_CTX_get(context);
   bnQy = BN_CTX_get(context);
   bnU1 = BN_CTX_get(context);
   bnU2 = BN_CTX_get(context);
   // Assume the size variables do not overflow, which should not happen in
   // the contexts that this function will be called.
   assert2Bsize(Qin->x.t);
   assert2Bsize(rIn->t);
   assert2Bsize(sIn->t);
   // BN_CTX_get() is sticky so only need to check the last value to know that
   // all worked.
   if(   bnU2 == NULL
        // initialize the group parameters
       || (group = EccCurveInit(curveId, context)) == NULL
       // allocate a local point
       || (pQ = EC_POINT_new(group)) == NULL
       //   use the public key values (QxIn and QyIn) to initialize Q
       ||   BN_bin2bn(Qin->x.t.buffer, Qin->x.t.size, bnQx) == NULL
       ||   BN_bin2bn(Qin->x.t.buffer, Qin->x.t.size, bnQy) == NULL
       ||   !EC_POINT_set_affine_coordinates_GFp(group, pQ, bnQx, bnQy, context)
       // convert the signature values
       || BN_bin2bn(rIn->t.buffer, rIn->t.size, bnR) == NULL
       || BN_bin2bn(sIn->t.buffer, sIn->t.size, bnS) == NULL
       // convert the curve order
       || BN_bin2bn(curveData->n->buffer, curveData->n->size, bnN) == NULL)
        FAIL(FATAL_ERROR_INTERNAL);
// 2. Use the selected hash function to compute H0 = Hash(M0).
   // This is an input parameter
// 3. Convert the bit string H0 to an integer e as described in Appendix B.2.
   t = (digest->size > rIn->t.size) ? rIn->t.size : digest->size;
   if(BN_bin2bn(digest->buffer, t, bnE) == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
// 4. Compute w = (s')^-1 mod n, using the routine in Appendix B.1.
   if (BN_mod_inverse(bnW, bnS, bnN, context) == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
// 5. Compute u1 = (e' *   w) mod n, and compute u2 = (r' *     w) mod n.
   if(   !BN_mod_mul(bnU1, bnE, bnW, bnN, context)
      || !BN_mod_mul(bnU2, bnR, bnW, bnN, context))
       FAIL(FATAL_ERROR_INTERNAL);
   BnTo2B(&U1.b, bnU1, (INT16) BN_num_bytes(bnU1));
   BnTo2B(&U2.b, bnU2, (INT16) BN_num_bytes(bnU2));
// 6. Compute the elliptic curve point R = (xR, yR) = u1G+u2Q, using EC
//    scalar multiplication and EC addition (see [Routines]). If R is equal to
//    the point at infinity O, output INVALID.
   if(_cpri__EccPointMultiply(&R, curveId, &U1, Qin, &U2) == CRYPT_SUCCESS)
   {
       // 7. Compute v = Rx mod n.
       if(    BN_bin2bn(R.x.t.buffer, R.x.t.size, bnV) == NULL
           || !BN_mod(bnV, bnV, bnN, context))
            FAIL(FATAL_ERROR_INTERNAL);
   // 8. Compare v and r0. If v = r0, output VALID; otherwise, output INVALID
       if(BN_cmp(bnV, bnR) == 0)
           retVal = CRYPT_SUCCESS;
   }
   if(pQ != NULL) EC_POINT_free(pQ);
   if(group != NULL) EC_GROUP_free(group);
   BN_CTX_end(context);
   BN_CTX_free(context);
   return retVal;
}
#endif      //% TPM_ALG_ECDSA
#ifdef TPM_ALG_ECSCHNORR //%
//
//
//        ValidateSignatureEcSchnorr()
//
//       This function is used to validate an EC Schnorr signature. rIn and sIn are required to be greater than
//       zero. This is checked in _cpri__ValidateSignatureEcc().
//
//       Return Value                   Meaning
//
//       CRYPT_SUCCESS                  signature valid
//       CRYPT_FAIL                     signature not valid
//       CRYPT_SCHEME                   hashAlg is not supported
//
static CRYPT_RESULT
ValidateSignatureEcSchnorr(
   TPM2B_ECC_PARAMETER         *rIn,                //   IN: r component of the signature
   TPM2B_ECC_PARAMETER         *sIn,                //   IN: s component of the signature
   TPM_ALG_ID                   hashAlg,            //   IN: hash algorithm of the signature
   TPM_ECC_CURVE                curveId,            //   IN: the curve used in the signature
                                                    //       process
   TPMS_ECC_POINT              *Qin,                //   IN: the public point of the key
   TPM2B                       *digest              //   IN: the digest that was signed
   )
{
   TPMS_ECC_POINT               pE;
   const TPM2B                 *n;
   CPRI_HASH_STATE              hashState;
   TPM2B_DIGEST                 rPrime;
   TPM2B_ECC_PARAMETER          minusR;
   UINT16                       digestSize = _cpri__GetDigestSize(hashAlg);
   const ECC_CURVE_DATA        *curveData = GetCurveData(curveId);
   // The curve parameter should have been filtered by unmarshaling code
   pAssert(curveData != NULL);
   if(digestSize == 0)
       return CRYPT_SCHEME;
   // Input parameter validation
   pAssert(rIn != NULL && sIn != NULL && Qin != NULL && digest != NULL);
   n = curveData->n;
   // if sIn or rIn are not between 1 and N-1, signature check fails
   // sIn and rIn were verified to be non-zero by the caller
   if(   _math__uComp(sIn->b.size, sIn->b.buffer, n->size, n->buffer) >= 0
      || _math__uComp(rIn->b.size, rIn->b.buffer, n->size, n->buffer) >= 0
     )
       return CRYPT_FAIL;
   //E = [s]InG - [r]InQ
   _math__sub(n->size, n->buffer,
              rIn->t.size, rIn->t.buffer,
              &minusR.t.size, minusR.t.buffer);
   if(_cpri__EccPointMultiply(&pE, curveId, sIn, Qin, &minusR) != CRYPT_SUCCESS)
       return CRYPT_FAIL;
   // Ex = Ex mod N
   if(Mod2B(&pE.x.b, n) != CRYPT_SUCCESS)
       FAIL(FATAL_ERROR_INTERNAL);
   _math__Normalize2B(&pE.x.b);
   // rPrime = h(digest || pE.x) mod n;
   _cpri__StartHash(hashAlg, FALSE, &hashState);
   _cpri__UpdateHash(&hashState, digest->size, digest->buffer);
   _cpri__UpdateHash(&hashState, pE.x.t.size, pE.x.t.buffer);
   if(_cpri__CompleteHash(&hashState, digestSize, rPrime.t.buffer) != digestSize)
       FAIL(FATAL_ERROR_INTERNAL);
   rPrime.t.size = digestSize;
   // rPrime = rPrime (mod n)
   if(Mod2B(&rPrime.b, n) != CRYPT_SUCCESS)
       FAIL(FATAL_ERROR_INTERNAL);
   // if the values don't match, then the signature is bad
   if(_math__uComp(rIn->t.size, rIn->t.buffer,
                   rPrime.t.size, rPrime.t.buffer) != 0)
       return CRYPT_FAIL;
   else
       return CRYPT_SUCCESS;
}
#endif //% TPM_ALG_ECSCHNORR
#ifdef TPM_ALG_SM2 //%
//
//
//        ValidateSignatueSM2Dsa()
//
//       This function is used to validate an SM2 signature.
//
//       Return Value                      Meaning
//
//       CRYPT_SUCCESS                     signature valid
//       CRYPT_FAIL                        signature not valid
//
static CRYPT_RESULT
ValidateSignatureSM2Dsa(
   TPM2B_ECC_PARAMETER            *rIn,                //   IN: r component of the signature
   TPM2B_ECC_PARAMETER            *sIn,                //   IN: s component of the signature
   TPM_ECC_CURVE                   curveId,            //   IN: the curve used in the signature
                                                       //       process
   TPMS_ECC_POINT                 *Qin,                //   IN: the public point of the key
   TPM2B                          *digest              //   IN: the digest that was signed
   )
{
   BIGNUM                         *bnR;
   BIGNUM                         *bnRp;
   BIGNUM                         *bnT;
   BIGNUM                         *bnS;
   BIGNUM                         *bnE;
   BIGNUM                         *order;
   EC_POINT                       *pQ;
   BN_CTX                         *context;
   EC_GROUP                       *group = NULL;
   const ECC_CURVE_DATA           *curveData = GetCurveData(curveId);
   BOOL                            fail = FALSE;
//
   if((context = BN_CTX_new()) == NULL || curveData == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
   bnR = BN_CTX_get(context);
   bnRp= BN_CTX_get(context);
   bnE = BN_CTX_get(context);
   bnT = BN_CTX_get(context);
   bnS = BN_CTX_get(context);
   order = BN_CTX_get(context);
   if(   order == NULL
      || (group = EccCurveInit(curveId, context)) == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
#ifdef _SM2_SIGN_DEBUG
   cpy_hexTo2B(&Qin->x.b,
          "0AE4C7798AA0F119471BEE11825BE46202BB79E2A5844495E97C04FF4DF2548A");
   cpy_hexTo2B(&Qin->y.b,
          "7C0240F88F1CD4E16352A73C17B7F16F07353E53A176D684A9FE0C6BB798E857");
   cpy_hexTo2B(digest,
          "B524F552CD82B8B028476E005C377FB19A87E6FC682D48BB5D42E3D9B9EFFE76");
#endif
   pQ = EccInitPoint2B(group, Qin, context);
#ifdef _SM2_SIGN_DEBUG
   pAssert(EC_POINT_get_affine_coordinates_GFp(group, pQ, bnT, bnS, context));
   pAssert(cmp_bn2hex(bnT,
               "0AE4C7798AA0F119471BEE11825BE46202BB79E2A5844495E97C04FF4DF2548A")
           == 0);
   pAssert(cmp_bn2hex(bnS,
               "7C0240F88F1CD4E16352A73C17B7F16F07353E53A176D684A9FE0C6BB798E857")
           == 0);
#endif
   BnFrom2B(bnR, &rIn->b);
   BnFrom2B(bnS, &sIn->b);
   BnFrom2B(bnE, digest);
#ifdef _SM2_SIGN_DEBUG
// Make sure that the input signature is the test signature
pAssert(cmp_2B2hex(&rIn->b,
       "40F1EC59F793D9F49E09DCEF49130D4194F79FB1EED2CAA55BACDB49C4E755D1") == 0);
pAssert(cmp_2B2hex(&sIn->b,
       "6FC6DAC32C5D5CF10C77DFB20F7C2EB667A457872FB09EC56327A67EC7DEEBE7") == 0);
#endif
// a) verify that r and s are in the inclusive interval 1 to (n   1)
   if (!EC_GROUP_get_order(group, order, context)) goto Cleanup;
   fail = (BN_ucmp(bnR, order) >= 0);
   fail = (BN_ucmp(bnS, order) >= 0) || fail;
   if(fail)
   // There is no reason to continue. Since r and s are inputs from the caller,
   // they can know that the values are not in the proper range. So, exiting here
   // does not disclose any information.
       goto Cleanup;
// b) compute t := (r + s) mod n
   if(!BN_mod_add(bnT, bnR, bnS, order, context))
       FAIL(FATAL_ERROR_INTERNAL);
#ifdef _SM2_SIGN_DEBUG
   pAssert(cmp_bn2hex(bnT,
               "2B75F07ED7ECE7CCC1C8986B991F441AD324D6D619FE06DD63ED32E0C997C801")
           == 0);
#endif
// c) verify that t > 0
   if(BN_is_zero(bnT)) {
       fail = TRUE;
       // set to a value that should allow rest of the computations to run without
         // trouble
         BN_copy(bnT, bnS);
   }
// d) compute (x, y) := [s]G + [t]Q
   if(!EC_POINT_mul(group, pQ, bnS, pQ, bnT, context))
       FAIL(FATAL_ERROR_INTERNAL);
   // Get the x coordinate of the point
   if(!EC_POINT_get_affine_coordinates_GFp(group, pQ, bnT, NULL, context))
       FAIL(FATAL_ERROR_INTERNAL);
#ifdef _SM2_SIGN_DEBUG
   pAssert(cmp_bn2hex(bnT,
               "110FCDA57615705D5E7B9324AC4B856D23E6D9188B2AE47759514657CE25D112")
               == 0);
#endif
// e) compute r' := (e + x) mod n (the x coordinate is in bnT)
   if(!BN_mod_add(bnRp, bnE, bnT, order, context))
       FAIL(FATAL_ERROR_INTERNAL);
// f) verify that r' = r
   fail = BN_ucmp(bnR, bnRp) != 0 || fail;
Cleanup:
   if(pQ) EC_POINT_free(pQ);
   if(group) EC_GROUP_free(group);
   BN_CTX_end(context);
   BN_CTX_free(context);
    if(fail)
        return CRYPT_FAIL;
    else
        return CRYPT_SUCCESS;
}
#endif //% TPM_ALG_SM2
//
//
//        _cpri__ValidateSignatureEcc()
//
//       This function validates
//
//       Return Value                      Meaning
//
//       CRYPT_SUCCESS                     signature is valid
//       CRYPT_FAIL                        not a valid signature
//       CRYPT_SCHEME                      unsupported scheme
//
LIB_EXPORT CRYPT_RESULT
_cpri__ValidateSignatureEcc(
    TPM2B_ECC_PARAMETER           *rIn,                  //   IN: r component of the signature
    TPM2B_ECC_PARAMETER           *sIn,                  //   IN: s component of the signature
    TPM_ALG_ID                     scheme,               //   IN: the scheme selector
    TPM_ALG_ID                     hashAlg,              //   IN: the hash algorithm used (not used
                                                         //       in all schemes)
    TPM_ECC_CURVE                   curveId,             //   IN: the curve used in the signature
                                                         //       process
    TPMS_ECC_POINT                *Qin,                  //   IN: the public point of the key
    TPM2B                         *digest                //   IN: the digest that was signed
    )
{
    CRYPT_RESULT                  retVal;
    // return failure if either part of the signature is zero
    if(_math__Normalize2B(&rIn->b) == 0 || _math__Normalize2B(&sIn->b) == 0)
        return CRYPT_FAIL;
   switch (scheme)
   {
       case TPM_ALG_ECDSA:
           retVal = ValidateSignatureEcdsa(rIn, sIn, curveId, Qin, digest);
           break;
#ifdef   TPM_ALG_ECSCHNORR
        case TPM_ALG_ECSCHNORR:
            retVal = ValidateSignatureEcSchnorr(rIn, sIn, hashAlg, curveId, Qin,
                                              digest);
            break;
#endif
#ifdef TPM_ALG_SM2
       case TPM_ALG_SM2:
           retVal = ValidateSignatureSM2Dsa(rIn, sIn, curveId, Qin, digest);
#endif
       default:
           retVal = CRYPT_SCHEME;
           break;
   }
   return retVal;
}
#if CC_ZGen_2Phase == YES //%
#ifdef TPM_ALG_ECMQV
//
//
//        avf1()
//
//       This function does the associated value computation required by MQV key exchange. Process:
//       a) Convert xQ to an integer xqi using the convention specified in Appendix C.3.
//       b) Calculate xqm = xqi mod 2^ceil(f/2) (where f = ceil(log2(n)).
//       c) Calculate the associate value function avf(Q) = xqm + 2ceil(f / 2)
//
static BOOL
avf1(
   BIGNUM              *bnX,               // IN/OUT: the reduced value
   BIGNUM              *bnN                // IN: the order of the curve
   )
{
// compute f = 2^(ceil(ceil(log2(n)) / 2))
   int                      f = (BN_num_bits(bnN) + 1) / 2;
// x' = 2^f + (x mod 2^f)
   BN_mask_bits(bnX, f);   // This is mod 2*2^f but it doesn't matter because
                           // the next operation will SET the extra bit anyway
   BN_set_bit(bnX, f);
   return TRUE;
}
//
//
//        C_2_2_MQV()
//
//       This function performs the key exchange defined in SP800-56A 6.1.1.4 Full MQV, C(2, 2, ECC MQV).
//       CAUTION: Implementation of this function may require use of essential claims in patents not owned by
//       TCG members.
//       Points QsB() and QeB() are required to be on the curve of inQsA. The function will fail, possibly
//       catastrophically, if this is not the case.
//
//
//
//       Return Value                      Meaning
//
//       CRYPT_SUCCESS                     results is valid
//       CRYPT_NO_RESULT                   the value for dsA does not give a valid point on the curve
//
static CRYPT_RESULT
C_2_2_MQV(
   TPMS_ECC_POINT                  *outZ,                //   OUT: the computed point
   TPM_ECC_CURVE                    curveId,             //   IN: the curve for the computations
   TPM2B_ECC_PARAMETER             *dsA,                 //   IN: static private TPM key
   TPM2B_ECC_PARAMETER             *deA,                 //   IN: ephemeral private TPM key
   TPMS_ECC_POINT                  *QsB,                 //   IN: static public party B key
   TPMS_ECC_POINT                  *QeB                  //   IN: ephemeral public party B key
   )
{
   BN_CTX                          *context;
   EC_POINT                        *pQeA = NULL;
   EC_POINT                        *pQeB = NULL;
   EC_POINT                        *pQsB = NULL;
   EC_GROUP                        *group = NULL;
   BIGNUM                          *bnTa;
   BIGNUM                          *bnDeA;
   BIGNUM                          *bnDsA;
   BIGNUM                          *bnXeA;         // x coordinate of ephemeral party A key
   BIGNUM                          *bnH;
   BIGNUM                          *bnN;
   BIGNUM                          *bnXeB;
   const ECC_CURVE_DATA            *curveData = GetCurveData(curveId);
   CRYPT_RESULT                    retVal;
   pAssert(       curveData != NULL && outZ != NULL && dsA != NULL
           &&           deA != NULL && QsB != NULL && QeB != NULL);
   context = BN_CTX_new();
   if(context == NULL || curveData == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BN_CTX_start(context);
   bnTa = BN_CTX_get(context);
   bnDeA = BN_CTX_get(context);
   bnDsA = BN_CTX_get(context);
   bnXeA = BN_CTX_get(context);
   bnH = BN_CTX_get(context);
   bnN = BN_CTX_get(context);
   bnXeB = BN_CTX_get(context);
   if(bnXeB == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
// Process:
// 1. implicitsigA = (de,A + avf(Qe,A)ds,A ) mod n.
// 2. P = h(implicitsigA)(Qe,B + avf(Qe,B)Qs,B).
// 3. If P = O, output an error indicator.
// 4. Z=xP, where xP is the x-coordinate of P.
   // Initialize group parameters and local values of input
   if((group = EccCurveInit(curveId, context)) == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
   if((pQeA = EC_POINT_new(group)) == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BnFrom2B(bnDeA, &deA->b);
   BnFrom2B(bnDsA, &dsA->b);
   BnFrom2B(bnH, curveData->h);
   BnFrom2B(bnN, curveData->n);
   BnFrom2B(bnXeB, &QeB->x.b);
   pQeB = EccInitPoint2B(group, QeB, context);
   pQsB = EccInitPoint2B(group, QsB, context);
   // Compute the public ephemeral key pQeA = [de,A]G
   if(    (retVal = PointMul(group, pQeA, bnDeA, NULL, NULL, context))
      != CRYPT_SUCCESS)
       goto Cleanup;
   if(EC_POINT_get_affine_coordinates_GFp(group, pQeA, bnXeA, NULL, context) != 1)
           FAIL(FATAL_ERROR_INTERNAL);
// 1. implicitsigA = (de,A + avf(Qe,A)ds,A ) mod n.
// tA := (ds,A + de,A avf(Xe,A)) mod n (3)
// Compute 'tA' = ('deA' + 'dsA' avf('XeA')) mod n
   // Ta = avf(XeA);
   BN_copy(bnTa, bnXeA);
   avf1(bnTa, bnN);
   if(// do Ta = ds,A * Ta mod n = dsA * avf(XeA) mod n
         !BN_mod_mul(bnTa, bnDsA, bnTa, bnN, context)
       // now Ta = deA + Ta mod n = deA + dsA * avf(XeA) mod n
       || !BN_mod_add(bnTa, bnDeA, bnTa, bnN, context)
      )
            FAIL(FATAL_ERROR_INTERNAL);
// 2. P = h(implicitsigA)(Qe,B + avf(Qe,B)Qs,B).
// Put this in because almost every case of h is == 1 so skip the call when
   // not necessary.
   if(!BN_is_one(bnH))
   {
       // Cofactor is not 1 so compute Ta := Ta * h mod n
       if(!BN_mul(bnTa, bnTa, bnH, context))
           FAIL(FATAL_ERROR_INTERNAL);
   }
   // Now that 'tA' is (h * 'tA' mod n)
   // 'outZ' = (tA)(Qe,B + avf(Qe,B)Qs,B).
   // first, compute XeB = avf(XeB)
   avf1(bnXeB, bnN);
   // QsB := [XeB]QsB
   if(     !EC_POINT_mul(group, pQsB, NULL, pQsB, bnXeB, context)
        // QeB := QsB + QeB
        || !EC_POINT_add(group, pQeB, pQeB, pQsB, context)
       )
        FAIL(FATAL_ERROR_INTERNAL);
   // QeB := [tA]QeB = [tA](QsB + [Xe,B]QeB) and check for at infinity
   if(PointMul(group, pQeB, NULL, pQeB, bnTa, context) == CRYPT_SUCCESS)
       // Convert BIGNUM E to TPM2B E
       Point2B(group, outZ, pQeB, (INT16)BN_num_bytes(bnN), context);
Cleanup:
   if(pQeA != NULL) EC_POINT_free(pQeA);
   if(pQeB != NULL) EC_POINT_free(pQeB);
   if(pQsB != NULL) EC_POINT_free(pQsB);
   if(group != NULL) EC_GROUP_free(group);
   BN_CTX_end(context);
   BN_CTX_free(context);
   return retVal;
}
#endif // TPM_ALG_ECMQV
#ifdef TPM_ALG_SM2 //%
//
//
//        avfSm2()
//
//       This function does the associated value computation required by SM2 key exchange. This is different
//       form the avf() in the international standards because it returns a value that is half the size of the value
//       returned by the standard avf. For example, if n is 15, Ws (w in the standard) is 2 but the W here is 1. This
//       means that an input value of 14 (1110b) would return a value of 110b with the standard but 10b with the
//       scheme in SM2.
//
static BOOL
avfSm2(
    BIGNUM              *bnX,                  // IN/OUT: the reduced value
    BIGNUM              *bnN                   // IN: the order of the curve
    )
{
// a) set w := ceil(ceil(log2(n)) / 2) - 1
   int                      w = ((BN_num_bits(bnN) + 1) / 2) - 1;
// b) set x' := 2^w + ( x & (2^w - 1))
// This is just like the avf for MQV where x' = 2^w + (x mod 2^w)
   BN_mask_bits(bnX, w);   // as wiht avf1, this is too big by a factor of 2 but
                           // it doesn't matter becasue we SET the extra bit anyway
   BN_set_bit(bnX, w);
   return TRUE;
}
//
//       SM2KeyExchange() This function performs the key exchange defined in SM2. The first step is to compute
//       tA = (dsA + deA avf(Xe,A)) mod n Then, compute the Z value from outZ = (h tA mod n) (QsA +
//       [avf(QeB().x)](QeB())). The function will compute the ephemeral public key from the ephemeral private
//       key. All points are required to be on the curve of inQsA. The function will fail catastrophically if this is not
//       the case
//
//       Return Value                      Meaning
//
//       CRYPT_SUCCESS                     results is valid
//       CRYPT_NO_RESULT                   the value for dsA does not give a valid point on the curve
//
static CRYPT_RESULT
SM2KeyExchange(
    TPMS_ECC_POINT                 *outZ,                //   OUT: the computed point
    TPM_ECC_CURVE                   curveId,             //   IN: the curve for the computations
    TPM2B_ECC_PARAMETER            *dsA,                 //   IN: static private TPM key
    TPM2B_ECC_PARAMETER            *deA,                 //   IN: ephemeral private TPM key
    TPMS_ECC_POINT                 *QsB,                 //   IN: static public party B key
    TPMS_ECC_POINT                 *QeB                  //   IN: ephemeral public party B key
    )
{
    BN_CTX                         *context;
    EC_POINT                       *pQeA = NULL;
    EC_POINT                       *pQeB = NULL;
    EC_POINT                       *pQsB = NULL;
    EC_GROUP                       *group = NULL;
    BIGNUM                         *bnTa;
    BIGNUM                         *bnDeA;
    BIGNUM                         *bnDsA;
    BIGNUM                         *bnXeA;               // x coordinate of ephemeral party A key
    BIGNUM                         *bnH;
    BIGNUM                         *bnN;
    BIGNUM                         *bnXeB;
//
   const ECC_CURVE_DATA      *curveData = GetCurveData(curveId);
   CRYPT_RESULT              retVal;
   pAssert(       curveData != NULL && outZ != NULL && dsA != NULL
           &&           deA != NULL && QsB != NULL && QeB != NULL);
   context = BN_CTX_new();
   if(context == NULL || curveData == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BN_CTX_start(context);
   bnTa = BN_CTX_get(context);
   bnDeA = BN_CTX_get(context);
   bnDsA = BN_CTX_get(context);
   bnXeA = BN_CTX_get(context);
   bnH = BN_CTX_get(context);
   bnN = BN_CTX_get(context);
   bnXeB = BN_CTX_get(context);
   if(bnXeB == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   // Initialize group parameters and local values of input
   if((group = EccCurveInit(curveId, context)) == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
   if((pQeA = EC_POINT_new(group)) == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BnFrom2B(bnDeA, &deA->b);
   BnFrom2B(bnDsA, &dsA->b);
   BnFrom2B(bnH, curveData->h);
   BnFrom2B(bnN, curveData->n);
   BnFrom2B(bnXeB, &QeB->x.b);
   pQeB = EccInitPoint2B(group, QeB, context);
   pQsB = EccInitPoint2B(group, QsB, context);
   // Compute the public ephemeral key pQeA = [de,A]G
   if(    (retVal = PointMul(group, pQeA, bnDeA, NULL, NULL, context))
      != CRYPT_SUCCESS)
       goto Cleanup;
   if(EC_POINT_get_affine_coordinates_GFp(group, pQeA, bnXeA, NULL, context) != 1)
           FAIL(FATAL_ERROR_INTERNAL);
// tA := (ds,A + de,A avf(Xe,A)) mod n (3)
// Compute 'tA' = ('dsA' + 'deA' avf('XeA')) mod n
   // Ta = avf(XeA);
   BN_copy(bnTa, bnXeA);
   avfSm2(bnTa, bnN);
   if(// do Ta = de,A * Ta mod n = deA * avf(XeA) mod n
         !BN_mod_mul(bnTa, bnDeA, bnTa, bnN, context)
       // now Ta = dsA + Ta mod n = dsA + deA * avf(XeA) mod n
       || !BN_mod_add(bnTa, bnDsA, bnTa, bnN, context)
      )
            FAIL(FATAL_ERROR_INTERNAL);
// outZ ? [h tA mod n] (Qs,B + [avf(Xe,B)](Qe,B)) (4)
   // Put this in because almost every case of h is == 1 so skip the call when
   // not necessary.
   if(!BN_is_one(bnH))
   {
       // Cofactor is not 1 so compute Ta := Ta * h mod n
       if(!BN_mul(bnTa, bnTa, bnH, context))
           FAIL(FATAL_ERROR_INTERNAL);
   }
   // Now that 'tA' is (h * 'tA' mod n)
   // 'outZ' = ['tA'](QsB + [avf(QeB.x)](QeB)).
   // first, compute XeB = avf(XeB)
   avfSm2(bnXeB, bnN);
   // QeB := [XeB]QeB
   if(     !EC_POINT_mul(group, pQeB, NULL, pQeB, bnXeB, context)
         // QeB := QsB + QeB
         || !EC_POINT_add(group, pQeB, pQeB, pQsB, context)
        )
         FAIL(FATAL_ERROR_INTERNAL);
   // QeB := [tA]QeB = [tA](QsB + [Xe,B]QeB) and check for at infinity
   if(PointMul(group, pQeB, NULL, pQeB, bnTa, context) == CRYPT_SUCCESS)
       // Convert BIGNUM E to TPM2B E
       Point2B(group, outZ, pQeB, (INT16)BN_num_bytes(bnN), context);
Cleanup:
   if(pQeA != NULL) EC_POINT_free(pQeA);
   if(pQeB != NULL) EC_POINT_free(pQeB);
   if(pQsB != NULL) EC_POINT_free(pQsB);
   if(group != NULL) EC_GROUP_free(group);
   BN_CTX_end(context);
   BN_CTX_free(context);
   return retVal;
}
#endif       //% TPM_ALG_SM2
//
//
//        C_2_2_ECDH()
//
//       This function performs the two phase key exchange defined in SP800-56A, 6.1.1.2 Full Unified Model,
//       C(2, 2, ECC CDH).
//
static CRYPT_RESULT
C_2_2_ECDH(
   TPMS_ECC_POINT                *outZ1,         //   OUT: Zs
   TPMS_ECC_POINT                *outZ2,         //   OUT: Ze
   TPM_ECC_CURVE                  curveId,       //   IN: the curve for the computations
   TPM2B_ECC_PARAMETER           *dsA,           //   IN: static private TPM key
   TPM2B_ECC_PARAMETER           *deA,           //   IN: ephemeral private TPM key
   TPMS_ECC_POINT                *QsB,           //   IN: static public party B key
   TPMS_ECC_POINT                *QeB            //   IN: ephemeral public party B key
   )
{
   BIGNUM                        *order;
   BN_CTX                        *context;
   EC_POINT                      *pQ = NULL;
   EC_GROUP                      *group = NULL;
   BIGNUM                        *bnD;
   INT16                          size;
   const ECC_CURVE_DATA          *curveData = GetCurveData(curveId);
   context = BN_CTX_new();
   if(context == NULL || curveData == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BN_CTX_start(context);
   order = BN_CTX_get(context);
   if((bnD = BN_CTX_get(context)) == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
   // Initialize group parameters and local values of input
   if((group = EccCurveInit(curveId, context)) == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
   if (!EC_GROUP_get_order(group, order, context))
       FAIL(FATAL_ERROR_INTERNAL);
   size = (INT16)BN_num_bytes(order);
   // Get the static private key of A
   BnFrom2B(bnD, &dsA->b);
   // Initialize the static public point from B
   pQ = EccInitPoint2B(group, QsB, context);
   // Do the point multiply for the Zs value
   if(PointMul(group, pQ, NULL, pQ, bnD, context) != CRYPT_NO_RESULT)
       // Convert the Zs value
       Point2B(group, outZ1, pQ, size, context);
   // Get the ephemeral private key of A
   BnFrom2B(bnD, &deA->b);
   // Initalize the ephemeral public point from B
   PointFrom2B(group, pQ, QeB, context);
   // Do the point multiply for the Ze value
   if(PointMul(group, pQ, NULL, pQ, bnD, context) != CRYPT_NO_RESULT)
       // Convert the Ze value.
       Point2B(group, outZ2, pQ, size, context);
   if(pQ != NULL) EC_POINT_free(pQ);
   if(group != NULL) EC_GROUP_free(group);
   BN_CTX_end(context);
   BN_CTX_free(context);
   return CRYPT_SUCCESS;
}
//
//
//        _cpri__C_2_2_KeyExchange()
//
//       This function is the dispatch routine for the EC key exchange function that use two ephemeral and two
//       static keys.
//
//       Return Value                   Meaning
//
//       CRYPT_SCHEME                   scheme is not defined
//
LIB_EXPORT CRYPT_RESULT
_cpri__C_2_2_KeyExchange(
   TPMS_ECC_POINT              *outZ1,                //   OUT: a computed point
   TPMS_ECC_POINT              *outZ2,                //   OUT: and optional second point
   TPM_ECC_CURVE                curveId,              //   IN: the curve for the computations
   TPM_ALG_ID                   scheme,               //   IN: the key exchange scheme
   TPM2B_ECC_PARAMETER         *dsA,                  //   IN: static private TPM key
   TPM2B_ECC_PARAMETER         *deA,                  //   IN: ephemeral private TPM key
   TPMS_ECC_POINT              *QsB,                  //   IN: static public party B key
   TPMS_ECC_POINT              *QeB                   //   IN: ephemeral public party B key
   )
{
   pAssert(   outZ1 != NULL
           && dsA != NULL && deA != NULL
           && QsB != NULL && QeB != NULL);
   // Initalize the output points so that they are empty until one of the
   // functions decides otherwise
   outZ1->x.b.size = 0;
   outZ1->y.b.size = 0;
   if(outZ2 != NULL)
   {
       outZ2->x.b.size = 0;
        outZ2->y.b.size = 0;
   }
   switch (scheme)
   {
       case TPM_ALG_ECDH:
           return C_2_2_ECDH(outZ1, outZ2, curveId, dsA, deA, QsB, QeB);
           break;
#ifdef TPM_ALG_ECMQV
       case TPM_ALG_ECMQV:
           return C_2_2_MQV(outZ1, curveId, dsA, deA, QsB, QeB);
           break;
#endif
#ifdef TPM_ALG_SM2
       case TPM_ALG_SM2:
           return SM2KeyExchange(outZ1, curveId, dsA, deA, QsB, QeB);
           break;
#endif
       default:
           return CRYPT_SCHEME;
   }
}
#else       //%
//
//       Stub used when the 2-phase key exchange is not defined so that the linker has something to associate
//       with the value in the .def file.
//
LIB_EXPORT CRYPT_RESULT
_cpri__C_2_2_KeyExchange(
   void
   )
{
   return CRYPT_FAIL;
}
#endif //% CC_ZGen_2Phase
#endif // TPM_ALG_ECC
