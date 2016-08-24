// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef        TCP_TPM_PROTOCOL_H
#define        TCP_TPM_PROTOCOL_H
//
//     TPM Commands. All commands acknowledge processing by returning a UINT32 == 0 except where
//     noted
//
#define    TPM_SIGNAL_POWER_ON         1
#define    TPM_SIGNAL_POWER_OFF        2
#define    TPM_SIGNAL_PHYS_PRES_ON     3
#define    TPM_SIGNAL_PHYS_PRES_OFF    4
#define    TPM_SIGNAL_HASH_START       5
#define    TPM_SIGNAL_HASH_DATA        6
          // {UINT32 BufferSize, BYTE[BufferSize] Buffer}
#define    TPM_SIGNAL_HASH_END         7
#define    TPM_SEND_COMMAND            8
          // {BYTE Locality, UINT32 InBufferSize, BYTE[InBufferSize] InBuffer} ->
          //     {UINT32 OutBufferSize, BYTE[OutBufferSize] OutBuffer}
#define    TPM_SIGNAL_CANCEL_ON        9
#define    TPM_SIGNAL_CANCEL_OFF       10
#define    TPM_SIGNAL_NV_ON            11
#define    TPM_SIGNAL_NV_OFF           12
#define    TPM_SIGNAL_KEY_CACHE_ON     13
#define    TPM_SIGNAL_KEY_CACHE_OFF    14
#define    TPM_REMOTE_HANDSHAKE        15
#define    TPM_SET_ALTERNATIVE_RESULT 16
#define    TPM_SIGNAL_RESET            17
#define    TPM_SESSION_END             20
#define    TPM_STOP                    21
#define    TPM_GET_COMMAND_RESPONSE_SIZES 25
#define    TPM_TEST_FAILURE_MODE      30
enum TpmEndPointInfo
{
   tpmPlatformAvailable = 0x01,
   tpmUsesTbs = 0x02,
   tpmInRawMode = 0x04,
   tpmSupportsPP = 0x08
};
// Existing RPC interface type definitions retained so that the implementation
// can be re-used
typedef struct
{
   unsigned long BufferSize;
   unsigned char *Buffer;
} _IN_BUFFER;
typedef unsigned char *_OUTPUT_BUFFER;
typedef struct
{
   uint32_t             BufferSize;
   _OUTPUT_BUFFER       Buffer;
} _OUT_BUFFER;
//** TPM Command Function Prototypes
void _rpc__Signal_PowerOn(BOOL isReset);
void _rpc__Signal_PowerOff();
void _rpc__ForceFailureMode();
void _rpc__Signal_PhysicalPresenceOn();
void _rpc__Signal_PhysicalPresenceOff();
void _rpc__Signal_Hash_Start();
void _rpc__Signal_Hash_Data(
   _IN_BUFFER input
);
void _rpc__Signal_HashEnd();
void _rpc__Send_Command(
   unsigned char   locality,
   _IN_BUFFER       request,
   _OUT_BUFFER      *response
);
void _rpc__Signal_CancelOn();
void _rpc__Signal_CancelOff();
void _rpc__Signal_NvOn();
void _rpc__Signal_NvOff();
BOOL _rpc__InjectEPS(
   const char* seed,
   int seedSize
);
//
//     start the TPM server on the indicated socket. The TPM is single-threaded and will accept connections
//     first-come-first-served. Once a connection is dropped another client can connect.
//
BOOL TpmServer(SOCKET ServerSocket);
#endif
