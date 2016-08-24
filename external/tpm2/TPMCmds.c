// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <windows.h>
#include <strsafe.h>
#include "string.h"
#include "TpmTcpProtocol.h"
#include "..\tpm\include\TpmBuildSwitches.h"
#include "..\tpm\include\prototypes\Manufacture_fp.h"
#define PURPOSE \
"TPM Reference Simulator.\nCopyright Microsoft 2010, 2011.\n"
#define DEFAULT_TPM_PORT 2321
void* MainPointer;
int _plat__NVEnable(void* platParameters);
void _plat__NVDisable();
int StartTcpServer(int PortNumber);
//
//
//          Functions
//
//          Usage()
//
//     This function prints the proper calling sequence for the simulator.
//
void
Usage(
     char                      *pszProgramName
     )
{
     fprintf_s(stderr, "%s", PURPOSE);
     fprintf_s(stderr, "Usage:\n");
     fprintf_s(stderr, "%s         - Starts the TPM server listening on port %d\n",
               pszProgramName, DEFAULT_TPM_PORT);
     fprintf_s(stderr,
               "%s PortNum - Starts the TPM server listening on port PortNum\n",
               pszProgramName);
     fprintf_s(stderr, "%s ?       - This message\n", pszProgramName);
     exit(1);
}
//
//
//          main()
//
//     This is the main entry point for the simulator.
//     main: register the interface, start listening for clients
//
void __cdecl
main(
     int                  argc,
     char                *argv[]
     )
{
   int portNum = DEFAULT_TPM_PORT;
   if(argc>2)
   {
       Usage(argv[0]);
   }
   if(argc==2)
   {
       if(strcmp(argv[1], "?") ==0)
       {
           Usage(argv[0]);
       }
       portNum = atoi(argv[1]);
       if(portNum <=0 || portNum>65535)
       {
           Usage(argv[0]);
       }
   }
   _plat__NVEnable(NULL);
   if(TPM_Manufacture(1) != 0)
   {
       exit(1);
   }
   // Coverage test - repeated manufacturing attempt
   if(TPM_Manufacture(0) != 1)
   {
       exit(2);
   }
   // Coverage test - re-manufacturing
   TPM_TearDown();
   if(TPM_Manufacture(1) != 0)
   {
       exit(3);
   }
   // Disable NV memory
   _plat__NVDisable();
   StartTcpServer(portNum);
   return;
}
