// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include        "Implementation.h"
#include        "Platform.h"
#include        "PlatformData.h"
//
//     From Cancel.c
//
BOOL                      s_isCanceled;
//
//     From Clock.c
//
unsigned long long        s_initClock;
unsigned int              s_adjustRate;
//
//     From LocalityPlat.c
//
unsigned char             s_locality;
//
//     From Power.c
//
BOOL                      s_powerLost;
//
//     From Entropy.c
//
uint32_t                  lastEntropy;
int                       firstValue;
//
//     From PPPlat.c
//
BOOL   s_physicalPresence;
