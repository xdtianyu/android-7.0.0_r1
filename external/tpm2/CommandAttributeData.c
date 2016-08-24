// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

static const TPMA_CC           s_ccAttr [] =      {
       {0x011f, 0, 1,        0, 0, 2, 0, 0,      0},    //   TPM_CC_NV_UndefineSpaceSpecial
       {0x0120, 0, 1,        0, 0, 2, 0, 0,      0},    //   TPM_CC_EvictControl
       {0x0121, 0, 1,        1, 0, 1, 0, 0,      0},    //   TPM_CC_HierarchyControl
       {0x0122, 0, 1,        0, 0, 2, 0, 0,      0},    //   TPM_CC_NV_UndefineSpace
       {0x0123, 0, 0,        0, 0, 0, 0, 0,      0},    //   No command
       {0x0124, 0, 1,        1, 0, 1, 0, 0,      0},    //   TPM_CC_ChangeEPS
       {0x0125, 0, 1,        1, 0, 1, 0, 0,      0},    //   TPM_CC_ChangePPS
       {0x0126, 0, 1,        1, 0, 1, 0, 0,      0},    //   TPM_CC_Clear
       {0x0127, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_ClearControl
       {0x0128, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_ClockSet
       {0x0129, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_HierarchyChangeAuth
       {0x012a, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_NV_DefineSpace
       {0x012b, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_PCR_Allocate
       {0x012c, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_PCR_SetAuthPolicy
       {0x012d, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_PP_Commands
       {0x012e, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_SetPrimaryPolicy
       {0x012f, 0, 0,        0, 0, 2, 0, 0,      0},    //   TPM_CC_FieldUpgradeStart
       {0x0130, 0, 0,        0, 0, 1, 0, 0,      0},    //   TPM_CC_ClockRateAdjust
       {0x0131, 0, 0,        0, 0, 1, 1, 0,      0},    //   TPM_CC_CreatePrimary
       {0x0132, 0, 0,        0, 0, 1, 0, 0,      0},    //   TPM_CC_NV_GlobalWriteLock
       {0x0133, 0, 1,        0, 0, 2, 0, 0,      0},    //   TPM_CC_GetCommandAuditDigest
       {0x0134, 0, 1,        0, 0, 2, 0, 0,      0},    //   TPM_CC_NV_Increment
       {0x0135, 0, 1,        0, 0, 2, 0, 0,      0},    //   TPM_CC_NV_SetBits
       {0x0136, 0, 1,        0, 0, 2, 0, 0,      0},    //   TPM_CC_NV_Extend
       {0x0137, 0, 1,        0, 0, 2, 0, 0,      0},    //   TPM_CC_NV_Write
       {0x0138, 0, 1,        0, 0, 2, 0, 0,      0},    //   TPM_CC_NV_WriteLock
       {0x0139, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_DictionaryAttackLockReset
       {0x013a, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_DictionaryAttackParameters
       {0x013b, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_NV_ChangeAuth
       {0x013c, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_PCR_Event
       {0x013d, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_PCR_Reset
       {0x013e, 0, 0,        0, 1, 1, 0, 0,      0},    //   TPM_CC_SequenceComplete
       {0x013f, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_SetAlgorithmSet
       {0x0140, 0, 1,        0, 0, 1, 0, 0,      0},    //   TPM_CC_SetCommandCodeAuditStatus
       {0x0141, 0, 1,        0, 0, 0, 0, 0,      0},    //   TPM_CC_FieldUpgradeData
       {0x0142, 0, 1,        0, 0, 0, 0, 0,      0},    //   TPM_CC_IncrementalSelfTest
       {0x0143, 0, 1,        0, 0, 0, 0, 0,      0},    //   TPM_CC_SelfTest
       {0x0144, 0, 1,        0, 0, 0, 0, 0,      0},    //   TPM_CC_Startup
       {0x0145, 0, 1,        0, 0, 0, 0, 0,      0},    //   TPM_CC_Shutdown
       {0x0146, 0, 1,        0, 0, 0, 0, 0,      0},    //   TPM_CC_StirRandom
        {0x0147,   0,   0,   0,   0,   2,   0,   0,   0},   //   TPM_CC_ActivateCredential
        {0x0148,   0,   0,   0,   0,   2,   0,   0,   0},   //   TPM_CC_Certify
        {0x0149,   0,   0,   0,   0,   3,   0,   0,   0},   //   TPM_CC_PolicyNV
        {0x014a,   0,   0,   0,   0,   2,   0,   0,   0},   //   TPM_CC_CertifyCreation
        {0x014b,   0,   0,   0,   0,   2,   0,   0,   0},   //   TPM_CC_Duplicate
        {0x014c,   0,   0,   0,   0,   2,   0,   0,   0},   //   TPM_CC_GetTime
        {0x014d,   0,   0,   0,   0,   3,   0,   0,   0},   //   TPM_CC_GetSessionAuditDigest
        {0x014e,   0,   0,   0,   0,   2,   0,   0,   0},   //   TPM_CC_NV_Read
        {0x014f,   0,   0,   0,   0,   2,   0,   0,   0},   //   TPM_CC_NV_ReadLock
        {0x0150,   0,   0,   0,   0,   2,   0,   0,   0},   //   TPM_CC_ObjectChangeAuth
        {0x0151,   0,   0,   0,   0,   2,   0,   0,   0},   //   TPM_CC_PolicySecret
        {0x0152,   0,   0,   0,   0,   2,   0,   0,   0},   //   TPM_CC_Rewrap
        {0x0153,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_Create
        {0x0154,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_ECDH_ZGen
        {0x0155,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_HMAC
        {0x0156,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_Import
        {0x0157,   0,   0,   0,   0,   1,   1,   0,   0},   //   TPM_CC_Load
        {0x0158,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_Quote
        {0x0159,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_RSA_Decrypt
        {0x015a,   0,   0,   0,   0,   0,   0,   0,   0},   //   No command
        {0x015b,   0,   0,   0,   0,   1,   1,   0,   0},   //   TPM_CC_HMAC_Start
        {0x015c,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_SequenceUpdate
        {0x015d,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_Sign
        {0x015e,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_Unseal
        {0x015f,   0,   0,   0,   0,   0,   0,   0,   0},   //   No command
        {0x0160,   0,   0,   0,   0,   2,   0,   0,   0},   //   TPM_CC_PolicySigned
        {0x0161,   0,   0,   0,   0,   0,   1,   0,   0},   //   TPM_CC_ContextLoad
        {0x0162,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_ContextSave
        {0x0163,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_ECDH_KeyGen
        {0x0164,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_EncryptDecrypt
        {0x0165,   0,   0,   0,   0,   0,   0,   0,   0},   //   TPM_CC_FlushContext
        {0x0166,   0,   0,   0,   0,   0,   0,   0,   0},   //   No command
        {0x0167,   0,   0,   0,   0,   0,   1,   0,   0},   //   TPM_CC_LoadExternal
        {0x0168,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_MakeCredential
        {0x0169,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_NV_ReadPublic
        {0x016a,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyAuthorize
        {0x016b,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyAuthValue
        {0x016c,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyCommandCode
        {0x016d,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyCounterTimer
        {0x016e,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyCpHash
        {0x016f,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyLocality
        {0x0170,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyNameHash
        {0x0171,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyOR
        {0x0172,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyTicket
        {0x0173,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_ReadPublic
        {0x0174,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_RSA_Encrypt
        {0x0175,   0,   0,   0,   0,   0,   0,   0,   0},   //   No command
        {0x0176,   0,   0,   0,   0,   2,   1,   0,   0},   //   TPM_CC_StartAuthSession
        {0x0177,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_VerifySignature
        {0x0178,   0,   0,   0,   0,   0,   0,   0,   0},   //   TPM_CC_ECC_Parameters
        {0x0179,   0,   0,   0,   0,   0,   0,   0,   0},   //   TPM_CC_FirmwareRead
        {0x017a,   0,   0,   0,   0,   0,   0,   0,   0},   //   TPM_CC_GetCapability
        {0x017b,   0,   0,   0,   0,   0,   0,   0,   0},   //   TPM_CC_GetRandom
        {0x017c,   0,   0,   0,   0,   0,   0,   0,   0},   //   TPM_CC_GetTestResult
        {0x017d,   0,   0,   0,   0,   0,   0,   0,   0},   //   TPM_CC_Hash
        {0x017e,   0,   0,   0,   0,   0,   0,   0,   0},   //   TPM_CC_PCR_Read
        {0x017f,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyPCR
        {0x0180,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyRestart
        {0x0181,   0,   0,   0,   0,   0,   0,   0,   0},   //   TPM_CC_ReadClock
        {0x0182,   0,   1,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PCR_Extend
        {0x0183,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PCR_SetAuthValue
        {0x0184,   0,   0,   0,   0,   3,   0,   0,   0},   //   TPM_CC_NV_Certify
        {0x0185,   0,   1,   0,   1,   2,   0,   0,   0},   //   TPM_CC_EventSequenceComplete
        {0x0186,   0,   0,   0,   0,   0,   1,   0,   0},   //   TPM_CC_HashSequenceStart
        {0x0187,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyPhysicalPresence
        {0x0188,   0,   0,   0,   0,   1,   0,   0,   0},   //   TPM_CC_PolicyDuplicationSelect
         {0x0189,   0,   0,   0,   0,   1,   0,   0,   0},     //   TPM_CC_PolicyGetDigest
         {0x018a,   0,   0,   0,   0,   0,   0,   0,   0},     //   TPM_CC_TestParms
         {0x018b,   0,   0,   0,   0,   1,   0,   0,   0},     //   TPM_CC_Commit
         {0x018c,   0,   0,   0,   0,   1,   0,   0,   0},     //   TPM_CC_PolicyPassword
         {0x018d,   0,   0,   0,   0,   1,   0,   0,   0},     //   TPM_CC_ZGen_2Phase
         {0x018e,   0,   0,   0,   0,   0,   0,   0,   0},     //   TPM_CC_EC_Ephemeral
         {0x018f,   0,   0,   0,   0,   1,   0,   0,   0}      //   TPM_CC_PolicyNvWritten
};
typedef    UINT16                    _ATTR_;
#define    NOT_IMPLEMENTED           (_ATTR_)(0)
#define    ENCRYPT_2                (_ATTR_)(1 <<          0)
#define    ENCRYPT_4                (_ATTR_)(1 <<          1)
#define    DECRYPT_2                (_ATTR_)(1 <<          2)
#define    DECRYPT_4                (_ATTR_)(1 <<          3)
#define    HANDLE_1_USER            (_ATTR_)(1 <<          4)
#define    HANDLE_1_ADMIN           (_ATTR_)(1 <<          5)
#define    HANDLE_1_DUP             (_ATTR_)(1 <<          6)
#define    HANDLE_2_USER            (_ATTR_)(1 <<          7)
#define    PP_COMMAND               (_ATTR_)(1 <<          8)
#define    IS_IMPLEMENTED           (_ATTR_)(1 <<          9)
#define    NO_SESSIONS              (_ATTR_)(1 <<         10)
#define    NV_COMMAND               (_ATTR_)(1 <<         11)
#define    PP_REQUIRED              (_ATTR_)(1 <<         12)
#define    R_HANDLE                 (_ATTR_)(1 <<         13)
//
//      This is the command code attribute structure.
//
typedef UINT16 COMMAND_ATTRIBUTES;
static const COMMAND_ATTRIBUTES    s_commandAttributes [] = {
   (_ATTR_)(CC_NV_UndefineSpaceSpecial     *
      (IS_IMPLEMENTED+HANDLE_1_ADMIN+HANDLE_2_USER+PP_COMMAND)),                                    // 0x011f
   (_ATTR_)(CC_EvictControl                *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                                   // 0x0120
   (_ATTR_)(CC_HierarchyControl            *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                                   // 0x0121
   (_ATTR_)(CC_NV_UndefineSpace            *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                                   // 0x0122
   (_ATTR_)                                  (NOT_IMPLEMENTED),
      // 0x0123 - Not assigned
   (_ATTR_)(CC_ChangeEPS                   *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                                   // 0x0124
   (_ATTR_)(CC_ChangePPS                   *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                                   // 0x0125
   (_ATTR_)(CC_Clear                       *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                                   // 0x0126
   (_ATTR_)(CC_ClearControl                *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                                   // 0x0127
   (_ATTR_)(CC_ClockSet                    *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                                   // 0x0128
   (_ATTR_)(CC_HierarchyChangeAuth         *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+PP_COMMAND)),                                         // 0x0129
   (_ATTR_)(CC_NV_DefineSpace              *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+PP_COMMAND)),                                         // 0x012a
   (_ATTR_)(CC_PCR_Allocate                *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                                   // 0x012b
   (_ATTR_)(CC_PCR_SetAuthPolicy           *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+PP_COMMAND)),                                         // 0x012c
   (_ATTR_)(CC_PP_Commands                 *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_REQUIRED)),                                                  // 0x012d
   (_ATTR_)(CC_SetPrimaryPolicy            *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+PP_COMMAND)),                                         // 0x012e
   (_ATTR_)(CC_FieldUpgradeStart           *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_ADMIN+PP_COMMAND)),                                        // 0x012f
   (_ATTR_)(CC_ClockRateAdjust             *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                                   // 0x0130
//
   (_ATTR_)(CC_CreatePrimary               *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+PP_COMMAND+ENCRYPT_2+R_HANDLE)), // 0x0131
   (_ATTR_)(CC_NV_GlobalWriteLock          *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                // 0x0132
   (_ATTR_)(CC_GetCommandAuditDigest       *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+HANDLE_2_USER+ENCRYPT_2)),         // 0x0133
   (_ATTR_)(CC_NV_Increment                * (IS_IMPLEMENTED+HANDLE_1_USER)),
      // 0x0134
   (_ATTR_)(CC_NV_SetBits                  * (IS_IMPLEMENTED+HANDLE_1_USER)),
      // 0x0135
   (_ATTR_)(CC_NV_Extend                   *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),                                 // 0x0136
   (_ATTR_)(CC_NV_Write                    *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),                                 // 0x0137
   (_ATTR_)(CC_NV_WriteLock                * (IS_IMPLEMENTED+HANDLE_1_USER)),
      // 0x0138
   (_ATTR_)(CC_DictionaryAttackLockReset * (IS_IMPLEMENTED+HANDLE_1_USER)),
      // 0x0139
   (_ATTR_)(CC_DictionaryAttackParameters * (IS_IMPLEMENTED+HANDLE_1_USER)),
      // 0x013a
   (_ATTR_)(CC_NV_ChangeAuth               *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_ADMIN)),                                // 0x013b
   (_ATTR_)(CC_PCR_Event                   *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),                                 // 0x013c
   (_ATTR_)(CC_PCR_Reset                   * (IS_IMPLEMENTED+HANDLE_1_USER)),
      // 0x013d
   (_ATTR_)(CC_SequenceComplete            *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                       // 0x013e
   (_ATTR_)(CC_SetAlgorithmSet             * (IS_IMPLEMENTED+HANDLE_1_USER)),
      // 0x013f
   (_ATTR_)(CC_SetCommandCodeAuditStatus *
      (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),                                // 0x0140
   (_ATTR_)(CC_FieldUpgradeData            * (IS_IMPLEMENTED+DECRYPT_2)),
      // 0x0141
   (_ATTR_)(CC_IncrementalSelfTest         * (IS_IMPLEMENTED)),
      // 0x0142
   (_ATTR_)(CC_SelfTest                    * (IS_IMPLEMENTED)),
      // 0x0143
   (_ATTR_)(CC_Startup                     * (IS_IMPLEMENTED+NO_SESSIONS)),
      // 0x0144
   (_ATTR_)(CC_Shutdown                    * (IS_IMPLEMENTED)),
      // 0x0145
   (_ATTR_)(CC_StirRandom                  * (IS_IMPLEMENTED+DECRYPT_2)),
      // 0x0146
   (_ATTR_)(CC_ActivateCredential          *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_ADMIN+HANDLE_2_USER+ENCRYPT_2)),        // 0x0147
   (_ATTR_)(CC_Certify                     *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_ADMIN+HANDLE_2_USER+ENCRYPT_2)),        // 0x0148
   (_ATTR_)(CC_PolicyNV                    *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),                                 // 0x0149
   (_ATTR_)(CC_CertifyCreation             *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                       // 0x014a
   (_ATTR_)(CC_Duplicate                   *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_DUP+ENCRYPT_2)),                        // 0x014b
   (_ATTR_)(CC_GetTime                     *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+HANDLE_2_USER+ENCRYPT_2)),         // 0x014c
   (_ATTR_)(CC_GetSessionAuditDigest       *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+HANDLE_2_USER+ENCRYPT_2)),         // 0x014d
   (_ATTR_)(CC_NV_Read                     *
      (IS_IMPLEMENTED+HANDLE_1_USER+ENCRYPT_2)),                                 // 0x014e
   (_ATTR_)(CC_NV_ReadLock                 * (IS_IMPLEMENTED+HANDLE_1_USER)),
      // 0x014f
   (_ATTR_)(CC_ObjectChangeAuth            *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_ADMIN+ENCRYPT_2)),                      // 0x0150
   (_ATTR_)(CC_PolicySecret                *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                       // 0x0151
   (_ATTR_)(CC_Rewrap                     *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                      // 0x0152
   (_ATTR_)(CC_Create                     *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                      // 0x0153
   (_ATTR_)(CC_ECDH_ZGen                  *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                      // 0x0154
   (_ATTR_)(CC_HMAC                       *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                      // 0x0155
   (_ATTR_)(CC_Import                     *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                      // 0x0156
   (_ATTR_)(CC_Load                       *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2+R_HANDLE)),             // 0x0157
   (_ATTR_)(CC_Quote                      *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                      // 0x0158
   (_ATTR_)(CC_RSA_Decrypt                *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                      // 0x0159
   (_ATTR_)                                 (NOT_IMPLEMENTED),
      // 0x015a - Not assigned
   (_ATTR_)(CC_HMAC_Start                 *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+R_HANDLE)),                       // 0x015b
   (_ATTR_)(CC_SequenceUpdate             *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),                                // 0x015c
   (_ATTR_)(CC_Sign                       *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),                                // 0x015d
   (_ATTR_)(CC_Unseal                     *
      (IS_IMPLEMENTED+HANDLE_1_USER+ENCRYPT_2)),                                // 0x015e
   (_ATTR_)                                 (NOT_IMPLEMENTED),
      // 0x015f - Not assigned
   (_ATTR_)(CC_PolicySigned               * (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2)),
      // 0x0160
   (_ATTR_)(CC_ContextLoad                * (IS_IMPLEMENTED+NO_SESSIONS+R_HANDLE)),
      // 0x0161
   (_ATTR_)(CC_ContextSave                * (IS_IMPLEMENTED+NO_SESSIONS)),
      // 0x0162
   (_ATTR_)(CC_ECDH_KeyGen                * (IS_IMPLEMENTED+ENCRYPT_2)),
      // 0x0163
   (_ATTR_)(CC_EncryptDecrypt             *
      (IS_IMPLEMENTED+HANDLE_1_USER+ENCRYPT_2)),                                // 0x0164
   (_ATTR_)(CC_FlushContext               * (IS_IMPLEMENTED+NO_SESSIONS)),
      // 0x0165
   (_ATTR_)                                 (NOT_IMPLEMENTED),
      // 0x0166 - Not assigned
   (_ATTR_)(CC_LoadExternal               *
      (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2+R_HANDLE)),                           // 0x0167
   (_ATTR_)(CC_MakeCredential             * (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2)),
      // 0x0168
   (_ATTR_)(CC_NV_ReadPublic              * (IS_IMPLEMENTED+ENCRYPT_2)),
      // 0x0169
   (_ATTR_)(CC_PolicyAuthorize            * (IS_IMPLEMENTED+DECRYPT_2)),
      // 0x016a
   (_ATTR_)(CC_PolicyAuthValue            * (IS_IMPLEMENTED)),
      // 0x016b
   (_ATTR_)(CC_PolicyCommandCode          * (IS_IMPLEMENTED)),
      // 0x016c
   (_ATTR_)(CC_PolicyCounterTimer         * (IS_IMPLEMENTED+DECRYPT_2)),
      // 0x016d
   (_ATTR_)(CC_PolicyCpHash               * (IS_IMPLEMENTED+DECRYPT_2)),
      // 0x016e
   (_ATTR_)(CC_PolicyLocality             * (IS_IMPLEMENTED)),
      // 0x016f
   (_ATTR_)(CC_PolicyNameHash             * (IS_IMPLEMENTED+DECRYPT_2)),
      // 0x0170
   (_ATTR_)(CC_PolicyOR                   * (IS_IMPLEMENTED)),
      // 0x0171
   (_ATTR_)(CC_PolicyTicket               * (IS_IMPLEMENTED+DECRYPT_2)),
      // 0x0172
   (_ATTR_)(CC_ReadPublic                 * (IS_IMPLEMENTED+ENCRYPT_2)),
      // 0x0173
   (_ATTR_)(CC_RSA_Encrypt                * (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2)),
      // 0x0174
   (_ATTR_)                                 (NOT_IMPLEMENTED),
      // 0x0175 - Not assigned
   (_ATTR_)(CC_StartAuthSession           *
      (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2+R_HANDLE)),                           // 0x0176
   (_ATTR_)(CC_VerifySignature            * (IS_IMPLEMENTED+DECRYPT_2)),
      // 0x0177
   (_ATTR_)(CC_ECC_Parameters             * (IS_IMPLEMENTED)),
      // 0x0178
   (_ATTR_)(CC_FirmwareRead               * (IS_IMPLEMENTED+ENCRYPT_2)),
      // 0x0179
   (_ATTR_)(CC_GetCapability              * (IS_IMPLEMENTED)),
      // 0x017a
   (_ATTR_)(CC_GetRandom                  * (IS_IMPLEMENTED+ENCRYPT_2)),
      // 0x017b
   (_ATTR_)(CC_GetTestResult              * (IS_IMPLEMENTED+ENCRYPT_2)),
      // 0x017c
   (_ATTR_)(CC_Hash                       * (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2)),
      // 0x017d
   (_ATTR_)(CC_PCR_Read                   * (IS_IMPLEMENTED)),
      // 0x017e
   (_ATTR_)(CC_PolicyPCR                  * (IS_IMPLEMENTED+DECRYPT_2)),
      // 0x017f
   (_ATTR_)(CC_PolicyRestart              * (IS_IMPLEMENTED)),
      // 0x0180
   (_ATTR_)(CC_ReadClock                  * (IS_IMPLEMENTED+NO_SESSIONS)),
      // 0x0181
   (_ATTR_)(CC_PCR_Extend                 * (IS_IMPLEMENTED+HANDLE_1_USER)),
      // 0x0182
   (_ATTR_)(CC_PCR_SetAuthValue           *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),                                // 0x0183
   (_ATTR_)(CC_NV_Certify                 *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+HANDLE_2_USER+ENCRYPT_2)),        // 0x0184
   (_ATTR_)(CC_EventSequenceComplete      *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+HANDLE_2_USER)),                  // 0x0185
   (_ATTR_)(CC_HashSequenceStart          * (IS_IMPLEMENTED+DECRYPT_2+R_HANDLE)),
      // 0x0186
   (_ATTR_)(CC_PolicyPhysicalPresence     * (IS_IMPLEMENTED)),
      // 0x0187
   (_ATTR_)(CC_PolicyDuplicationSelect    * (IS_IMPLEMENTED+DECRYPT_2)),
      // 0x0188
   (_ATTR_)(CC_PolicyGetDigest            * (IS_IMPLEMENTED+ENCRYPT_2)),
      // 0x0189
   (_ATTR_)(CC_TestParms                  * (IS_IMPLEMENTED)),
      // 0x018a
   (_ATTR_)(CC_Commit                     *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                      // 0x018b
   (_ATTR_)(CC_PolicyPassword             * (IS_IMPLEMENTED)),
      // 0x018c
   (_ATTR_)(CC_ZGen_2Phase                *
      (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),                      // 0x018d
   (_ATTR_)(CC_EC_Ephemeral               * (IS_IMPLEMENTED+ENCRYPT_2)),
      // 0x018e
   (_ATTR_)(CC_PolicyNvWritten            * (IS_IMPLEMENTED))
      // 0x018f
};
