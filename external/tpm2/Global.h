// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef         GLOBAL_H
#define         GLOBAL_H
//#define SELF_TEST
#include        "TpmBuildSwitches.h"
#include        "Tpm.h"
#include        "TPMB.h"
#include        "CryptoEngine.h"
#ifndef EMBEDDED_MODE
#include        <setjmp.h>
#endif
//
//
//
//          Defines and Types
//
//          Unreferenced Parameter
//
//     This define is used to eliminate the compiler warning about an unreferenced parameter. Basically, it tells
//     the compiler that it is not an accident that the parameter is unreferenced.
//
#ifndef UNREFERENCED_PARAMETER
#   define UNREFERENCED_PARAMETER(a)            (a)
#endif
#include    "bits.h"
//
//
//          Crypto Self-Test Values
//
//     Define these values here if the AlgorithmTests() project is not used
//
#ifndef SELF_TEST
extern ALGORITHM_VECTOR     g_implementedAlgorithms;
extern ALGORITHM_VECTOR     g_toTest;
#else
LIB_IMPORT extern ALGORITHM_VECTOR     g_implementedAlgorithms;
LIB_IMPORT extern ALGORITHM_VECTOR     g_toTest;
#endif
//
//     These macros are used in CryptUtil() to invoke the incremental self test.
//
#define       TEST(alg) if(TEST_BIT(alg, g_toTest)) CryptTestAlgorithm(alg, NULL)
//
//     Use of TPM_ALG_NULL is reserved for RSAEP/RSADP testing. If someone is wanting to test a hash with
//     that value, don't do it.
//
#define       TEST_HASH(alg)                                                                     \
             if(     TEST_BIT(alg, g_toTest)                                                    \
                 && (alg != ALG_NULL_VALUE))                                                    \
                 CryptTestAlgorithm(alg, NULL)
//
//
//          Hash and HMAC State Structures
//
//     These definitions are for the types that can be in a hash state structure. These types are used in the
//     crypto utilities
//
typedef   BYTE    HASH_STATE_TYPE;
#define   HASH_STATE_EMPTY         ((HASH_STATE_TYPE) 0)
#define   HASH_STATE_HASH          ((HASH_STATE_TYPE) 1)
#define   HASH_STATE_HMAC          ((HASH_STATE_TYPE) 2)
//
//     A HASH_STATE structure contains an opaque hash stack state. A caller would use this structure when
//     performing incremental hash operations. The state is updated on each call. If type is an HMAC_STATE,
//     or HMAC_STATE_SEQUENCE then state is followed by the HMAC key in oPad format.
//
typedef struct
{
   CPRI_HASH_STATE          state;                   // hash state
   HASH_STATE_TYPE          type;                    // type of the context
} HASH_STATE;
//
//
//
//
//     An HMAC_STATE structure contains an opaque HMAC stack state. A caller would use this structure
//     when performing incremental HMAC operations. This structure contains a hash state and an HMAC key
//     and allows slightly better stack optimization than adding an HMAC key to each hash state.
//
typedef struct
{
   HASH_STATE                hashState;               // the hash state
   TPM2B_HASH_BLOCK          hmacKey;                 // the HMAC key
} HMAC_STATE;
//
//
//          Other Types
//
//     An AUTH_VALUE is a BYTE array containing a digest (TPMU_HA)
//
typedef BYTE        AUTH_VALUE[sizeof(TPMU_HA)];
//
//     A TIME_INFO is a BYTE array that can contain a TPMS_TIME_INFO
//
typedef BYTE        TIME_INFO[sizeof(TPMS_TIME_INFO)];
//
//     A NAME is a BYTE array that can contain a TPMU_NAME
//
typedef BYTE        NAME[sizeof(TPMU_NAME)];
//
//
//          Loaded Object Structures
//
//          Description
//
//     The structures in this section define the object layout as it exists in TPM memory.
//     Two types of objects are defined: an ordinary object such as a key, and a sequence object that may be a
//     hash, HMAC, or event.
//
//          OBJECT_ATTRIBUTES
//
//     An OBJECT_ATTRIBUTES structure contains the variable attributes of an object. These properties are
//     not part of the public properties but are used by the TPM in managing the object. An
//     OBJECT_ATTRIBUTES is used in the definition of the OBJECT data type.
//
typedef struct
{
   unsigned                  publicOnly      : 1;     //0)   SET if only the public portion of
                                                      //     an object is loaded
    unsigned                 epsHierarchy : 1;        //1)   SET if the object belongs to EPS
                                                      //     Hierarchy
    unsigned                 ppsHierarchy : 1;        //2)   SET if the object belongs to PPS
                                                      //     Hierarchy
    unsigned                 spsHierarchy : 1;        //3)   SET f the object belongs to SPS
                                                      //     Hierarchy
    unsigned                 evict           : 1;     //4)   SET if the object is a platform or
                                                      //     owner evict object. Platform-
                                                      //     evict object belongs to PPS
                                                      //     hierarchy, owner-evict object
                                                      //     belongs to SPS or EPS hierarchy.
                                                      //     This bit is also used to mark a
                                                      //     completed sequence object so it
                                                      //     will be flush when the
                                                      //     SequenceComplete command succeeds.
    unsigned                 primary        : 1;      //5)   SET for a primary object
   unsigned                 temporary      :   1;
                                                    //6) SET for a temporary object
   unsigned                 stClear        :   1;
                                                    //7) SET for an stClear object
   unsigned                 hmacSeq        :   1;
                                                    //8) SET for an HMAC sequence object
   unsigned                 hashSeq        :   1;
                                                    //9) SET for a hash sequence object
   unsigned                 eventSeq       :   1;
                                                    //10) SET for an event sequence object
   unsigned                 ticketSafe     :   1;
                                                    //11) SET if a ticket is safe to create
                                             //    for hash sequence object
   unsigned            firstBlock : 1;       //12) SET if the first block of hash
                                             //    data has been received. It
                                             //    works with ticketSafe bit
   unsigned            isParent     : 1;     //13) SET if the key has the proper
                                             //    attributes to be a parent key
   unsigned            privateExp : 1;       //14) SET when the private exponent
                                             //    of an RSA key has been validated.
   unsigned        reserved    : 1;      //15) reserved bits. unused.
} OBJECT_ATTRIBUTES;
//
//
//           OBJECT Structure
//
//      An OBJECT structure holds the object public, sensitive, and meta-data associated. This structure is
//      implementation dependent. For this implementation, the structure is not optimized for space but rather for
//      clarity of the reference implementation. Other implementations may choose to overlap portions of the
//      structure that are not used simultaneously. These changes would necessitate changes to the source code
//      but those changes would be compatible with the reference implementation.
//
typedef struct
{
   // The attributes field is required to be first followed by the publicArea.
   // This allows the overlay of the object structure and a sequence structure
   OBJECT_ATTRIBUTES   attributes;         // object attributes
   TPMT_PUBLIC         publicArea;         // public area of an object
   TPMT_SENSITIVE      sensitive;          // sensitive area of an object
#ifdef TPM_ALG_RSA
   TPM2B_PUBLIC_KEY_RSA privateExponent;             // Additional field for the private
                                                     // exponent of an RSA key.
#endif
   TPM2B_NAME               qualifiedName;           //   object qualified name
   TPMI_DH_OBJECT           evictHandle;             //   if the object is an evict object,
                                                     //   the original handle is kept here.
                                                     //   The 'working' handle will be the
                                                     //   handle of an object slot.
   TPM2B_NAME               name;                    // Name of the object name. Kept here
                                                     // to avoid repeatedly computing it.
} OBJECT;
//
//
//           HASH_OBJECT Structure
//
//      This structure holds a hash sequence object or an event sequence object.
//      The first four components of this structure are manually set to be the same as the first four components of
//      the object structure. This prevents the object from being inadvertently misused as sequence objects
//      occupy the same memory as a regular object. A debug check is present to make sure that the offsets are
//      what they are supposed to be.
//
typedef struct
{
   OBJECT_ATTRIBUTES        attributes;              //   The attributes of the HASH object
   TPMI_ALG_PUBLIC          type;                    //   algorithm
   TPMI_ALG_HASH            nameAlg;                 //   name algorithm
   TPMA_OBJECT              objectAttributes;        //   object attributes
   // The data below is unique to a sequence object
   TPM2B_AUTH          auth;               // auth for use of sequence
   union
   {
       HASH_STATE      hashState[HASH_COUNT];
       HMAC_STATE      hmacState;
   }                   state;
} HASH_OBJECT;
//
//
//           ANY_OBJECT
//
//      This is the union for holding either a sequence object or a regular object.
//
typedef union
{
   OBJECT                    entity;
   HASH_OBJECT               hash;
} ANY_OBJECT;
//
//
//           AUTH_DUP Types
//
//      These values are used in the authorization processing.
//
typedef   UINT32              AUTH_ROLE;
#define   AUTH_NONE           ((AUTH_ROLE)(0))
#define   AUTH_USER           ((AUTH_ROLE)(1))
#define   AUTH_ADMIN          ((AUTH_ROLE)(2))
#define   AUTH_DUP            ((AUTH_ROLE)(3))
//
//
//           Active Session Context
//
//           Description
//
//      The structures in this section define the internal structure of a session context.
//
//           SESSION_ATTRIBUTES
//
//      The attributes in the SESSION_ATTRIBUTES structure track the various properties of the session. It
//      maintains most of the tracking state information for the policy session. It is used within the SESSION
//      structure.
//
typedef struct
{
   unsigned                  isPolicy : 1;       //1)        SET if the session may only
                                                 //          be used for policy
    unsigned                 isAudit : 1;        //2)        SET if the session is used
                                                 //          for audit
    unsigned                 isBound : 1;        //3)        SET if the session is bound to
                                                 //          with an entity.
                                                 //          This attribute will be CLEAR if
                                                 //          either isPolicy or isAudit is SET.
    unsigned                 iscpHashDefined : 1;//4)        SET if the cpHash has been defined
                                                 //          This attribute is not SET unless
                                                 //          'isPolicy' is SET.
    unsigned                 isAuthValueNeeded : 1;
                                                 //5)        SET if the authValue is required
                                                 //          for computing the session HMAC.
                                                 //          This attribute is not SET unless
                                                   //   isPolicy is SET.
   unsigned                    isPasswordNeeded : 1;
                                                   //6) SET if a password authValue is
                                                   //   required for authorization
                                                   //   This attribute is not SET unless
                                                   //   isPolicy is SET.
   unsigned                    isPPRequired : 1;   //7) SET if physical presence is
                                                   //   required to be asserted when the
                                                   //   authorization is checked.
                                                   //   This attribute is not SET unless
                                                   //   isPolicy is SET.
   unsigned                    isTrialPolicy : 1; //8) SET if the policy session is
                                                   //   created for trial of the policy's
                                                   //   policyHash generation.
                                                   //   This attribute is not SET unless
                                                   //   isPolicy is SET.
   unsigned                    isDaBound : 1;      //9) SET if the bind entity had noDA
                                                   //   CLEAR. If this is SET, then an
                                                   //   auth failure using this session
                                                   //   will count against lockout even
                                                   //   if the object being authorized is
                                                   //   exempt from DA.
   unsigned                    isLockoutBound : 1; //10)SET if the session is bound to
                                                   //   lockoutAuth.
   unsigned                    requestWasBound : 1;//11) SET if the session is being used
                                                   //    with the bind entity. If SET
                                                   //    the authValue will not be use
                                                   //    in the response HMAC computation.
   unsigned                    checkNvWritten : 1; //12) SET if the TPMA_NV_WRITTEN
                                                   //    attribute needs to be checked
                                                   //    when the policy is used for
                                                   //    authorization for NV access.
                                                   //    If this is SET for any other
                                                   //    type, the policy will fail.
   unsigned                    nvWrittenState : 1; //13) SET if TPMA_NV_WRITTEN is
                                                   //    required to be SET.
} SESSION_ATTRIBUTES;
//
//
//           SESSION Structure
//
//      The SESSION structure contains all the context of a session except for the associated contextID.
//
//      NOTE:           The contextID of a session is only relevant when the session context is stored off the TPM.
//
typedef struct
{
   TPM_ALG_ID                  authHashAlg;                   // session hash algorithm
   TPM2B_NONCE                 nonceTPM;                      // last TPM-generated nonce for
                                                              // this session
   TPMT_SYM_DEF                symmetric;                     // session symmetric algorithm (if any)
   TPM2B_AUTH                  sessionKey;                    // session secret value used for
                                                              // generating HMAC and encryption keys
   SESSION_ATTRIBUTES          attributes;                    //   session attributes
   TPM_CC                      commandCode;                   //   command code (policy session)
   TPMA_LOCALITY               commandLocality;               //   command locality (policy session)
   UINT32                      pcrCounter;                    //   PCR counter value when PCR is
                                                              //   included (policy session)
                                                              //   If no PCR is included, this
                                                              //   value is 0.
   UINT64                      startTime;                     // value of TPMS_CLOCK_INFO.clock when
                                                              // the session was started (policy
//
                                                    // session)
   UINT64                    timeOut;               //   timeout relative to
                                                    //   TPMS_CLOCK_INFO.clock
                                                    //   There is no timeout if this value
                                                    //   is 0.
   union
   {
       TPM2B_NAME            boundEntity;            // value used to track the entity to
                                                     // which the session is bound
         TPM2B_DIGEST        cpHash;                 // the required cpHash value for the
                                                     // command being authorized
   } u1;                                             // 'boundEntity' and 'cpHash' may
                                                     // share the same space to save memory
   union
   {
       TPM2B_DIGEST          auditDigest;           // audit session digest
       TPM2B_DIGEST          policyDigest;            // policyHash
   } u2;                                            // audit log and policyHash may
                                                    // share space to save memory
} SESSION;
//
//
//           PCR
//
//           PCR_SAVE Structure
//
//      The PCR_SAVE structure type contains the PCR data that are saved across power cycles. Only the static
//      PCR are required to be saved across power cycles. The DRTM and resettable PCR are not saved. The
//      number of static and resettable PCR is determined by the platform-specific specification to which the TPM
//      is built.
//
typedef struct
{
#ifdef TPM_ALG_SHA1
   BYTE                      sha1[NUM_STATIC_PCR][SHA1_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SHA256
   BYTE                      sha256[NUM_STATIC_PCR][SHA256_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SHA384
   BYTE                      sha384[NUM_STATIC_PCR][SHA384_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SHA512
   BYTE                      sha512[NUM_STATIC_PCR][SHA512_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SM3_256
   BYTE                      sm3_256[NUM_STATIC_PCR][SM3_256_DIGEST_SIZE];
#endif
   // This counter increments whenever the PCR are updated.
   // NOTE: A platform-specific specification may designate
   //       certain PCR changes as not causing this counter
   //       to increment.
   UINT32              pcrCounter;
} PCR_SAVE;
//
//
//
//           PCR_POLICY
//
//      This structure holds the PCR policies, one for each group of PCR controlled by policy.
//
typedef struct
{
   TPMI_ALG_HASH               hashAlg[NUM_POLICY_PCR_GROUP];
   TPM2B_DIGEST                a;
   TPM2B_DIGEST                policy[NUM_POLICY_PCR_GROUP];
} PCR_POLICY;
//
//
//           PCR_AUTHVALUE
//
//      This structure holds the PCR policies, one for each group of PCR controlled by policy.
//
typedef struct
{
   TPM2B_DIGEST                auth[NUM_AUTHVALUE_PCR_GROUP];
} PCR_AUTHVALUE;
//
//
//           Startup
//
//           SHUTDOWN_NONE
//
//      Part 2 defines the two shutdown/startup types that may be used in TPM2_Shutdown() and
//      TPM2_Starup(). This additional define is used by the TPM to indicate that no shutdown was received.
//
//      NOTE:           This is a reserved value.
//
#define SHUTDOWN_NONE           (TPM_SU)(0xFFFF)
//
//
//           STARTUP_TYPE
//
//      This enumeration is the possible startup types. The type is determined by the combination of
//      TPM2_ShutDown() and TPM2_Startup().
//
typedef enum
{
   SU_RESET,
   SU_RESTART,
   SU_RESUME
} STARTUP_TYPE;
//
//
//           NV
//
//           NV_RESERVE
//
//      This enumeration defines the master list of the elements of a reserved portion of NV. This list includes all
//      the pre-defined data that takes space in NV, either as persistent data or as state save data. The
//      enumerations are used as indexes into an array of offset values. The offset values then are used to index
//      into NV. This is method provides an imperfect analog to an actual NV implementation.
//
typedef enum
{
// Entries below mirror the PERSISTENT_DATA structure. These values are written
// to NV as individual items.
   // hierarchy
   NV_DISABLE_CLEAR,
   NV_OWNER_ALG,
   NV_ENDORSEMENT_ALG,
   NV_LOCKOUT_ALG,
   NV_OWNER_POLICY,
   NV_ENDORSEMENT_POLICY,
   NV_LOCKOUT_POLICY,
   NV_OWNER_AUTH,
   NV_ENDORSEMENT_AUTH,
   NV_LOCKOUT_AUTH,
   NV_EP_SEED,
   NV_SP_SEED,
   NV_PP_SEED,
   NV_PH_PROOF,
   NV_SH_PROOF,
   NV_EH_PROOF,
   // Time
   NV_TOTAL_RESET_COUNT,
   NV_RESET_COUNT,
   // PCR
   NV_PCR_POLICIES,
   NV_PCR_ALLOCATED,
   // Physical Presence
   NV_PP_LIST,
   // Dictionary Attack
   NV_FAILED_TRIES,
   NV_MAX_TRIES,
   NV_RECOVERY_TIME,
   NV_LOCKOUT_RECOVERY,
   NV_LOCKOUT_AUTH_ENABLED,
   // Orderly State flag
   NV_ORDERLY,
   // Command Audit
   NV_AUDIT_COMMANDS,
   NV_AUDIT_HASH_ALG,
   NV_AUDIT_COUNTER,
   // Algorithm Set
   NV_ALGORITHM_SET,
   NV_FIRMWARE_V1,
   NV_FIRMWARE_V2,
// The entries above are in PERSISTENT_DATA. The entries below represent
// structures that are read and written as a unit.
// ORDERLY_DATA data structure written on each orderly shutdown
   NV_ORDERLY_DATA,
// STATE_CLEAR_DATA structure written on each Shutdown(STATE)
   NV_STATE_CLEAR,
// STATE_RESET_DATA structure written on each Shutdown(STATE)
   NV_STATE_RESET,
   NV_RESERVE_LAST             // end of NV reserved data list
} NV_RESERVE;
//
//           NV_INDEX
//
//      The NV_INDEX structure defines the internal format for an NV index. The indexData size varies
//      according to the type of the index. In this implementation, all of the index is manipulated as a unit.
//
typedef struct
{
   TPMS_NV_PUBLIC           publicArea;
   TPM2B_AUTH               authValue;
} NV_INDEX;
//
//
//           COMMIT_INDEX_MASK
//
//      This is the define for the mask value that is used when manipulating the bits in the commit bit array. The
//      commit counter is a 64-bit value and the low order bits are used to index the commitArray. This mask
//      value is applied to the commit counter to extract the bit number in the array.
//
#ifdef TPM_ALG_ECC
#define COMMIT_INDEX_MASK ((UINT16)((sizeof(gr.commitArray)*8)-1))
#endif
//
//
//           RAM Global Values
//
//          Description
//
//      The values in this section are only extant in RAM. They are defined here and instanced in Global.c.
//
//          g_rcIndex
//
//      This array is used to contain the array of values that are added to a return code when it is a parameter-,
//      handle-, or session-related error. This is an implementation choice and the same result can be achieved
//      by using a macro.
//
extern const UINT16          g_rcIndex[15];
//
//
//          g_exclusiveAuditSession
//
//      This location holds the session handle for the current exclusive audit session. If there is no exclusive
//      audit session, the location is set to TPM_RH_UNASSIGNED.
//
extern TPM_HANDLE            g_exclusiveAuditSession;
//
//
//          g_time
//
//      This value is the count of milliseconds since the TPM was powered up. This value is initialized at
//      _TPM_Init().
//
extern     UINT64            g_time;
//
//
//          g_phEnable
//
//      This is the platform hierarchy control and determines if the platform hierarchy is available. This value is
//      SET on each TPM2_Startup(). The default value is SET.
//
extern BOOL                  g_phEnable;
//          g_pceReConfig
//
//      This value is SET if a TPM2_PCR_Allocate() command successfully executed since the last
//      TPM2_Startup(). If so, then the next shutdown is required to be Shutdown(CLEAR).
//
extern BOOL                   g_pcrReConfig;
//
//
//          g_DRTMHandle
//
//      This location indicates the sequence object handle that holds the DRTM sequence data. When not used,
//      it is set to TPM_RH_UNASSIGNED. A sequence DRTM sequence is started on either _TPM_Init() or
//      _TPM_Hash_Start().
//
extern TPMI_DH_OBJECT         g_DRTMHandle;
//
//
//          g_DrtmPreStartup
//
//      This value indicates that an H-CRTM occurred after _TPM_Init() but before TPM2_Startup(). The define
//      for PRE_STARTUP_FLAG is used to add the g_DrtmPreStartup value to gp_orderlyState at shutdown.
//      This hack is to avoid adding another NV variable.
//
extern BOOL              g_DrtmPreStartup;
#define PRE_STARTUP_FLAG     0x8000
//
//
//          g_StartupLocality3
//
//      This value indicates that a TPM2_Startup() occured at locality 3. Otherwise, it at locality 0. The define for
//      STARTUP_LOCALITY_3 is to indicate that the startup was not at locality 0. This hack is to avoid adding
//      another NV variable.
//
extern BOOL             g_StartupLocality3;
#define STARTUP_LOCALITY_3       0x4000
//
//
//       g_updateNV
//
//      This flag indicates if NV should be updated at the end of a command. This flag is set to FALSE at the
//      beginning of each command in ExecuteCommand(). This flag is checked in ExecuteCommand() after the
//      detailed actions of a command complete. If the command execution was successful and this flag is SET,
//      any pending NV writes will be committed to NV.
//
extern BOOL                   g_updateNV;
//
//
//       g_clearOrderly
//
//      This flag indicates if the execution of a command should cause the orderly state to be cleared. This flag
//      is set to FALSE at the beginning of each command in ExecuteCommand() and is checked in
//      ExecuteCommand() after the detailed actions of a command complete but before the check of
//      g_updateNV. If this flag is TRUE, and the orderly state is not SHUTDOWN_NONE, then the orderly state
//      in NV memory will be changed to SHUTDOWN_NONE.
//
extern BOOL                   g_clearOrderly;
//
//
//
//       g_prevOrderlyState
//
//      This location indicates how the TPM was shut down before the most recent TPM2_Startup(). This value,
//      along with the startup type, determines if the TPM should do a TPM Reset, TPM Restart, or TPM
//      Resume.
//
extern TPM_SU                 g_prevOrderlyState;
//
//
//       g_nvOk
//
//      This value indicates if the NV integrity check was successful or not. If not and the failure was severe, then
//      the TPM would have been put into failure mode after it had been re-manufactured. If the NV failure was in
//      the area where the state-save data is kept, then this variable will have a value of FALSE indicating that a
//      TPM2_Startup(CLEAR) is required.
//
extern BOOL                   g_nvOk;
//
//
//       g_platformUnique
//
//      This location contains the unique value(s) used to identify the TPM. It is loaded on every
//      _TPM2_Startup() The first value is used to seed the RNG. The second value is used as a vendor
//      authValue. The value used by the RNG would be the value derived from the chip unique value (such as
//      fused) with a dependency on the authorities of the code in the TPM boot path. The second would be
//      derived from the chip unique value with a dependency on the details of the code in the boot path. That is,
//      the first value depends on the various signers of the code and the second depends on what was signed.
//      The TPM vendor should not be able to know the first value but they are expected to know the second.
//
extern TPM2B_AUTH             g_platformUniqueAuthorities; // Reserved for RNG
extern TPM2B_AUTH             g_platformUniqueDetails;   // referenced by VENDOR_PERMANENT
//
//
//           Persistent Global Values
//
//           Description
//
//      The values in this section are global values that are persistent across power events. The lifetime of the
//      values determines the structure in which the value is placed.
//
//           PERSISTENT_DATA
//
//      This structure holds the persistent values that only change as a consequence of a specific Protected
//      Capability and are not affected by TPM power events (TPM2_Startup() or TPM2_Shutdown().
//
typedef struct
{
//*********************************************************************************
//          Hierarchy
//*********************************************************************************
// The values in this section are related to the hierarchies.
    BOOL                     disableClear;            // TRUE if TPM2_Clear() using
                                                      // lockoutAuth is disabled
    // Hierarchy authPolicies
    TPMI_ALG_HASH       ownerAlg;
    TPMI_ALG_HASH       endorsementAlg;
    TPMI_ALG_HASH       lockoutAlg;
    TPM2B_DIGEST        ownerPolicy;
    TPM2B_DIGEST             endorsementPolicy;
    TPM2B_DIGEST             lockoutPolicy;
    // Hierarchy authValues
    TPM2B_AUTH          ownerAuth;
    TPM2B_AUTH          endorsementAuth;
    TPM2B_AUTH          lockoutAuth;
    // Primary Seeds
    TPM2B_SEED          EPSeed;
    TPM2B_SEED          SPSeed;
    TPM2B_SEED          PPSeed;
    // Note there is a nullSeed in the state_reset memory.
    // Hierarchy proofs
    TPM2B_AUTH          phProof;
    TPM2B_AUTH          shProof;
    TPM2B_AUTH          ehProof;
    // Note there is a nullProof in the state_reset memory.
//*********************************************************************************
//          Reset Events
//*********************************************************************************
// A count that increments at each TPM reset and never get reset during the life
// time of TPM. The value of this counter is initialized to 1 during TPM
// manufacture process.
   UINT64               totalResetCount;
// This counter increments on each TPM Reset. The counter is reset by
// TPM2_Clear().
   UINT32              resetCount;
//*********************************************************************************
//           PCR
//*********************************************************************************
// This structure hold the policies for those PCR that have an update policy.
// This implementation only supports a single group of PCR controlled by
// policy. If more are required, then this structure would be changed to
// an array.
   PCR_POLICY          pcrPolicies;
//   This structure indicates the allocation of PCR. The structure contains a
//   list of PCR allocations for each implemented algorithm. If no PCR are
//   allocated for an algorithm, a list entry still exists but the bit map
//   will contain no SET bits.
     TPML_PCR_SELECTION pcrAllocated;
//*********************************************************************************
//          Physical Presence
//*********************************************************************************
// The PP_LIST type contains a bit map of the commands that require physical
// to be asserted when the authorization is evaluated. Physical presence will be
// checked if the corresponding bit in the array is SET and if the authorization
// handle is TPM_RH_PLATFORM.
//
// These bits may be changed with TPM2_PP_Commands().
   BYTE                ppList[((TPM_CC_PP_LAST - TPM_CC_PP_FIRST + 1) + 7)/8];
//*********************************************************************************
//          Dictionary attack values
//*********************************************************************************
// These values are used for dictionary attack tracking and control.
   UINT32              failedTries;        // the current count of unexpired
                                           // authorization failures
    UINT32                   maxTries;            // number of unexpired authorization
                                                  // failures before the TPM is in
                                                  // lockout
   UINT32                  recoveryTime;          // time between authorization failures
                                                  // before failedTries is decremented
   UINT32                  lockoutRecovery;       // time that must expire between
                                                  // authorization failures associated
                                                  // with lockoutAuth
   BOOL                    lockOutAuthEnabled; // TRUE if use of lockoutAuth is
                                               // allowed
//*****************************************************************************
//            Orderly State
//*****************************************************************************
// The orderly state for current cycle
   TPM_SU              orderlyState;
//*****************************************************************************
//           Command audit values.
//*****************************************************************************
   BYTE                auditComands[((TPM_CC_LAST - TPM_CC_FIRST + 1) + 7) / 8];
   TPMI_ALG_HASH       auditHashAlg;
   UINT64              auditCounter;
//*****************************************************************************
//           Algorithm selection
//*****************************************************************************
//
// The 'algorithmSet' value indicates the collection of algorithms that are
// currently in used on the TPM. The interpretation of value is vendor dependent.
   UINT32              algorithmSet;
//*****************************************************************************
//           Firmware version
//*****************************************************************************
// The firmwareV1 and firmwareV2 values are instanced in TimeStamp.c. This is
// a scheme used in development to allow determination of the linker build time
// of the TPM. An actual implementation would implement these values in a way that
// is consistent with vendor needs. The values are maintained in RAM for simplified
// access with a master version in NV. These values are modified in a
// vendor-specific way.
// g_firmwareV1 contains the more significant 32-bits of the vendor version number.
// In the reference implementation, if this value is printed as a hex
// value, it will have the format of yyyymmdd
   UINT32              firmwareV1;
// g_firmwareV1 contains the less significant 32-bits of the vendor version number.
// In the reference implementation, if this value is printed as a hex
// value, it will have the format of 00 hh mm ss
   UINT32              firmwareV2;
} PERSISTENT_DATA;
extern PERSISTENT_DATA      gp;
//
//
//         ORDERLY_DATA
//
//      The data in this structure is saved to NV on each TPM2_Shutdown().
//
typedef struct orderly_data
{
//
//*****************************************************************************
//           TIME
//*****************************************************************************
//   Clock has two parts. One is the state save part and one is the NV part. The
//   state save version is updated on each command. When the clock rolls over, the
//   NV version is updated. When the TPM starts up, if the TPM was shutdown in and
//   orderly way, then the sClock value is used to initialize the clock. If the
//   TPM shutdown was not orderly, then the persistent value is used and the safe
//   attribute is clear.
    UINT64                   clock;        // The orderly version of clock
    TPMI_YES_NO              clockSafe;    // Indicates if the clock value is
                                           // safe.
//*********************************************************************************
//          DRBG
//*********************************************************************************
#ifdef _DRBG_STATE_SAVE
   // This is DRBG state data. This is saved each time the value of clock is
   // updated.
   DRBG_STATE          drbgState;
#endif
} ORDERLY_DATA;
extern ORDERLY_DATA           go;
//
//
//          STATE_CLEAR_DATA
//
//      This structure contains the data that is saved on Shutdown(STATE). and restored on Startup(STATE).
//      The values are set to their default settings on any Startup(Clear). In other words the data is only
//      persistent across TPM Resume.
//      If the comments associated with a parameter indicate a default reset value, the value is applied on each
//      Startup(CLEAR).
//
typedef struct state_clear_data
{
//*****************************************************************************
//           Hierarchy Control
//*****************************************************************************
   BOOL                shEnable;           // default reset is SET
   BOOL                ehEnable;           // default reset is SET
   BOOL                phEnableNV;         // default reset is SET
   TPMI_ALG_HASH       platformAlg;        // default reset is TPM_ALG_NULL
   TPM2B_DIGEST        platformPolicy;     // default reset is an Empty Buffer
   TPM2B_AUTH          platformAuth;       // default reset is an Empty Buffer
//*****************************************************************************
//           PCR
//*****************************************************************************
// The set of PCR to be saved on Shutdown(STATE)
   PCR_SAVE            pcrSave;            // default reset is 0...0
//   This structure hold the authorization values for those PCR that have an
//   update authorization.
//   This implementation only supports a single group of PCR controlled by
//   authorization. If more are required, then this structure would be changed to
//   an array.
     PCR_AUTHVALUE        pcrAuthValues;
} STATE_CLEAR_DATA;
extern STATE_CLEAR_DATA gc;
//
//
//
//          State Reset Data
//
//      This structure contains data is that is saved on Shutdown(STATE) and restored on the subsequent
//      Startup(ANY). That is, the data is preserved across TPM Resume and TPM Restart.
//      If a default value is specified in the comments this value is applied on TPM Reset.
//
typedef struct state_reset_data
{
//*****************************************************************************
//          Hierarchy Control
//*****************************************************************************
   TPM2B_AUTH          nullProof;          // The proof value associated with
                                           // the TPM_RH_NULL hierarchy. The
                                           // default reset value is from the RNG.
   TPM2B_SEED               nullSeed;                // The seed value for the TPM_RN_NULL
                                                     // hierarchy. The default reset value
                                                     // is from the RNG.
//*****************************************************************************
//           Context
//*****************************************************************************
// The 'clearCount' counter is incremented each time the TPM successfully executes
// a TPM Resume. The counter is included in each saved context that has 'stClear'
// SET (including descendants of keys that have 'stClear' SET). This prevents these
// objects from being loaded after a TPM Resume.
// If 'clearCount' at its maximum value when the TPM receives a Shutdown(STATE),
// the TPM will return TPM_RC_RANGE and the TPM will only accept Shutdown(CLEAR).
   UINT32              clearCount;         // The default reset value is 0.
   UINT64                   objectContextID;         // This is the context ID for a saved
                                                     // object context. The default reset
                                                     // value is 0.
   CONTEXT_SLOT             contextArray[MAX_ACTIVE_SESSIONS];
                                                // This is the value from which the
                                                // 'contextID' is derived. The
                                                // default reset value is {0}.
   CONTEXT_COUNTER          contextCounter;          //   This array contains contains the
                                                     //   values used to track the version
                                                     //   numbers of saved contexts (see
                                                     //   Session.c in for details). The
                                                     //   default reset value is 0.
//*****************************************************************************
//           Command Audit
//*****************************************************************************
// When an audited command completes, ExecuteCommand() checks the return
// value. If it is TPM_RC_SUCCESS, and the command is an audited command, the
// TPM will extend the cpHash and rpHash for the command to this value. If this
// digest was the Zero Digest before the cpHash was extended, the audit counter
// is incremented.
   TPM2B_DIGEST             commandAuditDigest; // This value is set to an Empty Digest
                                                // by TPM2_GetCommandAuditDigest() or a
                                                // TPM Reset.
//*****************************************************************************
//           Boot counter
//*****************************************************************************
   UINT32                   restartCount;            // This counter counts TPM Restarts.
                                                     // The default reset value is 0.
//
//*********************************************************************************
//            PCR
//*********************************************************************************
// This counter increments whenever the PCR are updated. This counter is preserved
// across TPM Resume even though the PCR are not preserved. This is because
// sessions remain active across TPM Restart and the count value in the session
// is compared to this counter so this counter must have values that are unique
// as long as the sessions are active.
// NOTE: A platform-specific specification may designate that certain PCR changes
//       do not increment this counter to increment.
   UINT32              pcrCounter;         // The default reset value is 0.
#ifdef TPM_ALG_ECC
//*****************************************************************************
//         ECDAA
//*****************************************************************************
   UINT64              commitCounter;      // This counter increments each time
                                           // TPM2_Commit() returns
                                           // TPM_RC_SUCCESS. The default reset
                                           // value is 0.
   TPM2B_NONCE               commitNonce;            // This random value is used to compute
                                                     // the commit values. The default reset
                                                     // value is from the RNG.
// This implementation relies on the number of bits in g_commitArray being a
// power of 2 (8, 16, 32, 64, etc.) and no greater than 64K.
   BYTE                 commitArray[16];   // The default reset value is {0}.
#endif //TPM_ALG_ECC
} STATE_RESET_DATA;
extern STATE_RESET_DATA gr;
//
//
//         Global Macro Definitions
//
//      This macro is used to ensure that a handle, session, or parameter number is only added if the response
//      code is FMT1.
//
#define RcSafeAddToResult(r, v) \
   ((r) + (((r) & RC_FMT1) ? (v) : 0))
//
//      This macro is used when a parameter is not otherwise referenced in a function. This macro is normally
//      not used by itself but is paired with a pAssert() within a #ifdef pAssert. If pAssert is not defined, then a
//      parameter might not otherwise be referenced. This macro uses the parameter from the perspective of the
//      compiler so it doesn't complain.
//
#define UNREFERENCED(a) ((void)(a))
//
//
//         Private data
//
#if defined SESSION_PROCESS_C || defined GLOBAL_C || defined MANUFACTURE_C
//
//      From SessionProcess.c
//      The following arrays are used to save command sessions information so that the command
//      handle/session buffer does not have to be preserved for the duration of the command. These arrays are
//      indexed by the session index in accordance with the order of sessions in the session area of the
//      command.
//
//      Array of the authorization session handles
//
extern TPM_HANDLE             s_sessionHandles[MAX_SESSION_NUM];
//
//      Array of authorization session attributes
//
extern TPMA_SESSION           s_attributes[MAX_SESSION_NUM];
//
//      Array of handles authorized by the corresponding authorization sessions; and if none, then
//      TPM_RH_UNASSIGNED value is used
//
extern TPM_HANDLE             s_associatedHandles[MAX_SESSION_NUM];
//
//      Array of nonces provided by the caller for the corresponding sessions
//
extern TPM2B_NONCE            s_nonceCaller[MAX_SESSION_NUM];
//
//      Array of authorization values (HMAC's or passwords) for the corresponding sessions
//
extern TPM2B_AUTH             s_inputAuthValues[MAX_SESSION_NUM];
//
//      Special value to indicate an undefined session index
//
#define                  UNDEFINED_INDEX        (0xFFFF)
//
//      Index of the session used for encryption of a response parameter
//
extern UINT32                 s_encryptSessionIndex;
//
//      Index of the session used for decryption of a command parameter
//
extern UINT32                 s_decryptSessionIndex;
//
//      Index of a session used for audit
//
extern UINT32                 s_auditSessionIndex;
//
//      The cpHash for an audit session
//
extern TPM2B_DIGEST           s_cpHashForAudit;
//
//      The cpHash for command audit
//
#ifdef TPM_CC_GetCommandAuditDigest
extern TPM2B_DIGEST   s_cpHashForCommandAudit;
#endif
//
//      Number of authorization sessions present in the command
//
extern UINT32                 s_sessionNum;
//
//      Flag indicating if NV update is pending for the lockOutAuthEnabled or failedTries DA parameter
//
extern BOOL             s_DAPendingOnNV;
#endif // SESSION_PROCESS_C
#if defined DA_C || defined GLOBAL_C || defined MANUFACTURE_C
//
//      From DA.c
//
//      This variable holds the accumulated time since the last time that failedTries was decremented. This value
//      is in millisecond.
//
extern UINT64            s_selfHealTimer;
//
//      This variable holds the accumulated time that the lockoutAuth has been blocked.
//
extern UINT64       s_lockoutTimer;
#endif // DA_C
#if defined NV_C || defined GLOBAL_C
//
//      From NV.c
//      List of pre-defined address of reserved data
//
extern UINT32            s_reservedAddr[NV_RESERVE_LAST];
//
//      List of pre-defined reserved data size in byte
//
extern UINT32            s_reservedSize[NV_RESERVE_LAST];
//
//      Size of data in RAM index buffer
//
extern UINT32            s_ramIndexSize;
//
//      Reserved RAM space for frequently updated NV Index. The data layout in ram buffer is {NV_handle(),
//      size of data, data} for each NV index data stored in RAM
//
extern BYTE          s_ramIndex[RAM_INDEX_SPACE];
//
//      Address of size of RAM index space in NV
//
extern UINT32       s_ramIndexSizeAddr;
//
//      Address of NV copy of RAM index space
//
extern UINT32       s_ramIndexAddr;
//
//      Address of maximum counter value; an auxiliary variable to implement NV counters
//
extern UINT32       s_maxCountAddr;
//
//      Beginning of NV dynamic area; starts right after the s_maxCountAddr and s_evictHandleMapAddr
//      variables
//
extern UINT32       s_evictNvStart;
//
//      Beginning of NV dynamic area; also the beginning of the predefined reserved data area.
//
extern UINT32       s_evictNvEnd;
//
//      NV availability is sampled as the start of each command and stored here so that its value remains
//      consistent during the command execution
//
extern TPM_RC   s_NvStatus;
#endif
#if defined OBJECT_C || defined GLOBAL_C
//
//      From Object.c
//
//      This type is the container for an object.
//
typedef struct
{
   BOOL            occupied;
   ANY_OBJECT          object;
} OBJECT_SLOT;
//
//      This is the memory that holds the loaded objects.
//
extern OBJECT_SLOT     s_objects[MAX_LOADED_OBJECTS];
#endif // OBJECT_C
#if defined PCR_C || defined GLOBAL_C
//
//      From PCR.c
//
typedef struct
{
#ifdef TPM_ALG_SHA1
   // SHA1 PCR
   BYTE    sha1Pcr[SHA1_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SHA256
   // SHA256 PCR
   BYTE    sha256Pcr[SHA256_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SHA384
   // SHA384 PCR
   BYTE    sha384Pcr[SHA384_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SHA512
   // SHA512 PCR
   BYTE    sha512Pcr[SHA512_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SM3_256
   // SHA256 PCR
   BYTE    sm3_256Pcr[SM3_256_DIGEST_SIZE];
#endif
} PCR;
typedef struct
{
   unsigned int    stateSave : 1;                          //   if the PCR value should be
                                                           //   saved in state save
    unsigned int        resetLocality : 5;                 //   The locality that the PCR
                                                           //   can be reset
    unsigned int        extendLocality : 5;                //   The locality that the PCR
                                                           //   can be extend
} PCR_Attributes;
extern PCR          s_pcrs[IMPLEMENTATION_PCR];
#endif // PCR_C
#if defined SESSION_C || defined GLOBAL_C
//
//      From Session.c
//      Container for HMAC or policy session tracking information
//
typedef struct
{
   BOOL                      occupied;
   SESSION                   session;          // session structure
} SESSION_SLOT;
extern SESSION_SLOT           s_sessions[MAX_LOADED_SESSIONS];
//
//
//
//
//      The index in conextArray that has the value of the oldest saved session context. When no context is
//      saved, this will have a value that is greater than or equal to MAX_ACTIVE_SESSIONS.
//
extern UINT32                  s_oldestSavedSession;
//
//      The number of available session slot openings. When this is 1, a session can't be created or loaded if the
//      GAP is maxed out. The exception is that the oldest saved session context can always be loaded
//      (assuming that there is a space in memory to put it)
//
extern int                     s_freeSessionSlots;
#endif // SESSION_C
//
//      From Manufacture.c
//
extern BOOL              g_manufactured;
#if defined POWER_C || defined GLOBAL_C
//
//      From Power.c
//      This value indicates if a TPM2_Startup() commands has been receive since the power on event. This
//      flag is maintained in power simulation module because this is the only place that may reliably set this flag
//      to FALSE.
//
extern BOOL              s_initialized;
#endif // POWER_C
#if defined MEMORY_LIB_C || defined GLOBAL_C
//
//      The s_actionOutputBuffer should not be modifiable by the host system until the TPM has returned a
//      response code. The s_actionOutputBuffer should not be accessible until response parameter encryption,
//      if any, is complete.
//
extern   UINT32   s_actionInputBuffer[1024];          // action input buffer
extern   UINT32   s_actionOutputBuffer[1024];         // action output buffer
extern   BYTE     s_responseBuffer[MAX_RESPONSE_SIZE];// response buffer
#endif   // MEMORY_LIB_C
//
//      From TPMFail.c
//      This value holds the address of the string containing the name of the function in which the failure
//      occurred. This address value isn't useful for anything other than helping the vendor to know in which file
//      the failure occurred.
//
#ifndef EMBEDDED_MODE
extern jmp_buf   g_jumpBuffer;          //           the jump buffer
#endif
extern BOOL      g_inFailureMode;       //           Indicates that the TPM is in failure mode
extern BOOL      g_forceFailureMode;    //           flag to force failure mode during test
#if defined TPM_FAIL_C || defined GLOBAL_C           || 1
extern UINT32    s_failFunction;
extern UINT32    s_failLine;            //           the line in the file at which
                                       //           the error was signaled
extern UINT32    s_failCode;            //           the error code used
#endif // TPM_FAIL_C
#endif // GLOBAL_H
