/*
 * cblas_icamax.c
 *
 * The program is a C interface to icamax.
 * It calls the fortran wrapper before calling icamax.
 *
 * Written by Keita Teranishi.  2/11/1998
 *
 */
#include "cblas.h"
#include "cblas_f77.h"
CBLAS_INDEX cblas_icamax( const int N, const void *X, const int incX)
{
   int iamax;
#ifdef F77_INT
   F77_INT F77_N=N, F77_incX=incX;
#else 
   #define F77_N N
   #define F77_incX incX
#endif
   iamax = F77_icamax( &F77_N, X, &F77_incX);
   return iamax ? iamax-1 : 0;
}
