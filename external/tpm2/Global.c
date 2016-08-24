// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#define GLOBAL_C
#include "InternalRoutines.h"
//
//
//           Global Data Values
//
//      These values are visible across multiple modules.
//
BOOL                      g_phEnable;
const UINT16              g_rcIndex[15] = {TPM_RC_1,       TPM_RC_2,    TPM_RC_3, TPM_RC_4,
                                          TPM_RC_5,       TPM_RC_6,    TPM_RC_7, TPM_RC_8,
                                          TPM_RC_9,       TPM_RC_A,    TPM_RC_B, TPM_RC_C,
                                          TPM_RC_D,       TPM_RC_E,    TPM_RC_F
                                       };
TPM_HANDLE              g_exclusiveAuditSession;
UINT64                  g_time;
BOOL                    g_pcrReConfig;
TPMI_DH_OBJECT          g_DRTMHandle;
BOOL                    g_DrtmPreStartup;
BOOL                    g_StartupLocality3;
BOOL                    g_clearOrderly;
TPM_SU                  g_prevOrderlyState;
BOOL                    g_updateNV;
BOOL                    g_nvOk;
TPM2B_AUTH              g_platformUniqueDetails;
STATE_CLEAR_DATA        gc;
STATE_RESET_DATA        gr;
PERSISTENT_DATA         gp;
ORDERLY_DATA            go;
//
//
//          Private Values
//
//          SessionProcess.c
//
#ifndef __IGNORE_STATE__           // DO NOT DEFINE THIS VALUE
//
//     These values do not need to be retained between commands.
//
TPM_HANDLE           s_sessionHandles[MAX_SESSION_NUM];
TPMA_SESSION         s_attributes[MAX_SESSION_NUM];
TPM_HANDLE           s_associatedHandles[MAX_SESSION_NUM];
TPM2B_NONCE          s_nonceCaller[MAX_SESSION_NUM];
TPM2B_AUTH           s_inputAuthValues[MAX_SESSION_NUM];
UINT32               s_encryptSessionIndex;
UINT32               s_decryptSessionIndex;
UINT32               s_auditSessionIndex;
TPM2B_DIGEST         s_cpHashForAudit;
UINT32               s_sessionNum;
#endif // __IGNORE_STATE__
BOOL                 s_DAPendingOnNV;
#ifdef TPM_CC_GetCommandAuditDigest
TPM2B_DIGEST         s_cpHashForCommandAudit;
#endif
//
//
//          DA.c
//
UINT64                  s_selfHealTimer;
UINT64                  s_lockoutTimer;
//
//
//          NV.c
//
UINT32                  s_reservedAddr[NV_RESERVE_LAST];
UINT32                  s_reservedSize[NV_RESERVE_LAST];
UINT32                  s_ramIndexSize;
BYTE                    s_ramIndex[RAM_INDEX_SPACE];
UINT32                  s_ramIndexSizeAddr;
UINT32                  s_ramIndexAddr;
UINT32                  s_maxCountAddr;
UINT32                  s_evictNvStart;
UINT32                  s_evictNvEnd;
TPM_RC                  s_NvStatus;
//
//
//
//          Object.c
//
OBJECT_SLOT               s_objects[MAX_LOADED_OBJECTS];
//
//
//          PCR.c
//
PCR                       s_pcrs[IMPLEMENTATION_PCR];
//
//
//          Session.c
//
SESSION_SLOT              s_sessions[MAX_LOADED_SESSIONS];
UINT32                    s_oldestSavedSession;
int                       s_freeSessionSlots;
//
//
//          Manufacture.c
//
BOOL                      g_manufactured = FALSE;
//
//
//          Power.c
//
BOOL                      s_initialized = FALSE;
//
//
//          MemoryLib.c
//
//     The s_actionOutputBuffer should not be modifiable by the host system until the TPM has returned a
//     response code. The s_actionOutputBuffer should not be accessible until response parameter encryption,
//     if any, is complete. This memory is not used between commands
//
#ifndef __IGNORE_STATE__        // DO NOT DEFINE THIS VALUE
#ifndef EMBEDDED_MODE
UINT32   s_actionInputBuffer[1024];          // action input buffer
UINT32   s_actionOutputBuffer[1024];         // action output buffer
#endif  // EMBEDDED_MODE   ^^^ not defined
BYTE     s_responseBuffer[MAX_RESPONSE_SIZE];// response buffer
#endif  // __IGNORE_STATE__   ^^^ not defined
//
//
//         SelfTest.c
//
//     Define these values here if the AlgorithmTests() project is not used
//
#ifndef SELF_TEST
ALGORITHM_VECTOR          g_implementedAlgorithms;
ALGORITHM_VECTOR          g_toTest;
#endif
//
//
//         TpmFail.c
//
#ifndef EMBEDDED_MODE
jmp_buf                   g_jumpBuffer;
#endif  // EMBEDDED_MODE   ^^^ not defined
BOOL                      g_forceFailureMode;
BOOL                      g_inFailureMode;
UINT32                    s_failFunction;
UINT32                    s_failLine;
UINT32                    s_failCode;
