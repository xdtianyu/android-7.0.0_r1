// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef   _TPM_BUILD_SWITCHES_H
#define   _TPM_BUILD_SWITCHES_H
#define   SIMULATION
#define   FIPS_COMPLIANT
//
//     Define the alignment macro appropriate for the build environment For MS C compiler
//
#ifdef __GNUC__
#define ALIGN_TO(boundary)            __attribute__ ((aligned(boundary)))
#define __declspec(x)
#else
#define ALIGN_TO(boundary)            __declspec(align(boundary))
#endif
//
//     For ISO 9899:2011
//
// #define ALIGN_TO(boundary)                 _Alignas(boundary)
//
//     This switch enables the RNG state save and restore
//
#undef _DRBG_STATE_SAVE
#define _DRBG_STATE_SAVE                    // Comment this out if no state save is wanted
//
//     Set the alignment size for the crypto. It would be nice to set this according to macros automatically
//     defined by the build environment, but that doesn't seem possible because there isn't any simple set for
//     that. So, this is just a plugged value. Your compiler should complain if this alignment isn't possible.
//
//     NOTE:           this value can be set at the command line or just plugged in here.
//
#ifdef CRYPTO_ALIGN_16
#   define CRYPTO_ALIGNMENT    16
#elif defined CRYPTO_ALIGN_8
#   define CRYPTO_ALIGNMENT    8
#eliF defined CRYPTO_ALIGN_2
#   define CRYPTO_ALIGNMENT    2
#elif defined CRTYPO_ALIGN_1
#   define CRYPTO_ALIGNMENT    1
#else
#   define CRYPTO_ALIGNMENT    4    // For 32-bit builds
#endif
#define CRYPTO_ALIGNED ALIGN_TO(CRYPTO_ALIGNMENT)
//
//     This macro is used to handle LIB_EXPORT of function and variable names in lieu of a .def file
//
#define LIB_EXPORT __declspec(dllexport)
// #define LIB_EXPORT
//
//
//
//     For import of a variable
//
#define LIB_IMPORT __declspec(dllimport)
//#define LIB_IMPORT
//
//     This is defined to indicate a function that does not return. This is used in static code anlaysis.
//
#define _No_Return_ __declspec(noreturn)
//#define _No_Return_
#ifdef SELF_TEST
#pragma comment(lib, "algorithmtests.lib")
#endif
//
//     The switches in this group can only be enabled when running a simulation
//
#ifdef SIMULATION
#   define RSA_KEY_CACHE
#   define TPM_RNG_FOR_DEBUG
#else
#   undef RSA_KEY_CACHE
#   undef TPM_RNG_FOR_DEBUG
#endif // SIMULATION
#define INLINE __inline
#endif // _TPM_BUILD_SWITCHES_H
