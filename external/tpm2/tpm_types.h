// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED - DO NOT MODIFY!

#ifndef TPM2_TPM_TYPES_H_
#define TPM2_TPM_TYPES_H_
// Unprocessed: Table 1  Name Prefix Convention
// Skipped: Table 2  Unmarshaling Errors
// Table 3  Definition of Base Types
typedef uint8_t UINT8;
typedef uint8_t BYTE;
typedef int8_t INT8;
typedef int BOOL;
typedef uint16_t UINT16;
typedef int16_t INT16;
typedef uint32_t UINT32;
typedef int32_t INT32;
typedef uint64_t UINT64;
typedef int64_t INT64;

// Table 4  Defines for Logic Values
#define TRUE   1
#define FALSE  0
#define YES    1
#define NO     0
#define SET    1
#define CLEAR  0

// Table 5  Definition of Types for Documentation Clarity
typedef UINT32 TPM_ALGORITHM_ID;
typedef UINT32 TPM_MODIFIER_INDICATOR;
typedef UINT32 TPM_AUTHORIZATION_SIZE;
typedef UINT32 TPM_PARAMETER_SIZE;
typedef UINT16 TPM_KEY_SIZE;
typedef UINT16 TPM_KEY_BITS;

// Skipped: Table 6  Definition of TPM_SPEC Constants <>
// Table 7  Definition of TPM_GENERATED Constants <  O>
typedef UINT32 TPM_GENERATED;
#define TPM_GENERATED_VALUE  0xff544347

// Unprocessed: Table 8  Legend for TPM_ALG_ID Table
// Skipped: Table 9  Definition of TPM_ALG_ID Constants <  IN/OUT, S>
// Skipped: Table 10  Definition of TPM_ECC_CURVE Constants <  IN/OUT, S>
// Unprocessed: Table 11  TPM Command Format Fields Description
// Unprocessed: Table 12  Legend for Command Code Tables
// Skipped: Table 13  Definition of TPM_CC Constants <  IN/OUT, S>
// Unprocessed: Table 14  Format-Zero Response Codes
// Unprocessed: Table 15  Format-One Response Codes
// Unprocessed: Table 16  Response Code Groupings
// Table 17  Definition of TPM_RC Constants <  OUT>
typedef UINT32 TPM_RC;
#define TPM_RC_SUCCESS                                  0x000
#define TPM_RC_BAD_TAG                                  0x01E
#define RC_VER1                                         0x100
#define TPM_RC_INITIALIZE         ((TPM_RC)(RC_VER1 + 0x000))
#define TPM_RC_FAILURE            ((TPM_RC)(RC_VER1 + 0x001))
#define TPM_RC_SEQUENCE           ((TPM_RC)(RC_VER1 + 0x003))
#define TPM_RC_PRIVATE            ((TPM_RC)(RC_VER1 + 0x00B))
#define TPM_RC_HMAC               ((TPM_RC)(RC_VER1 + 0x019))
#define TPM_RC_DISABLED           ((TPM_RC)(RC_VER1 + 0x020))
#define TPM_RC_EXCLUSIVE          ((TPM_RC)(RC_VER1 + 0x021))
#define TPM_RC_AUTH_TYPE          ((TPM_RC)(RC_VER1 + 0x024))
#define TPM_RC_AUTH_MISSING       ((TPM_RC)(RC_VER1 + 0x025))
#define TPM_RC_POLICY             ((TPM_RC)(RC_VER1 + 0x026))
#define TPM_RC_PCR                ((TPM_RC)(RC_VER1 + 0x027))
#define TPM_RC_PCR_CHANGED        ((TPM_RC)(RC_VER1 + 0x028))
#define TPM_RC_UPGRADE            ((TPM_RC)(RC_VER1 + 0x02D))
#define TPM_RC_TOO_MANY_CONTEXTS  ((TPM_RC)(RC_VER1 + 0x02E))
#define TPM_RC_AUTH_UNAVAILABLE   ((TPM_RC)(RC_VER1 + 0x02F))
#define TPM_RC_REBOOT             ((TPM_RC)(RC_VER1 + 0x030))
#define TPM_RC_UNBALANCED         ((TPM_RC)(RC_VER1 + 0x031))
#define TPM_RC_COMMAND_SIZE       ((TPM_RC)(RC_VER1 + 0x042))
#define TPM_RC_COMMAND_CODE       ((TPM_RC)(RC_VER1 + 0x043))
#define TPM_RC_AUTHSIZE           ((TPM_RC)(RC_VER1 + 0x044))
#define TPM_RC_AUTH_CONTEXT       ((TPM_RC)(RC_VER1 + 0x045))
#define TPM_RC_NV_RANGE           ((TPM_RC)(RC_VER1 + 0x046))
#define TPM_RC_NV_SIZE            ((TPM_RC)(RC_VER1 + 0x047))
#define TPM_RC_NV_LOCKED          ((TPM_RC)(RC_VER1 + 0x048))
#define TPM_RC_NV_AUTHORIZATION   ((TPM_RC)(RC_VER1 + 0x049))
#define TPM_RC_NV_UNINITIALIZED   ((TPM_RC)(RC_VER1 + 0x04A))
#define TPM_RC_NV_SPACE           ((TPM_RC)(RC_VER1 + 0x04B))
#define TPM_RC_NV_DEFINED         ((TPM_RC)(RC_VER1 + 0x04C))
#define TPM_RC_BAD_CONTEXT        ((TPM_RC)(RC_VER1 + 0x050))
#define TPM_RC_CPHASH             ((TPM_RC)(RC_VER1 + 0x051))
#define TPM_RC_PARENT             ((TPM_RC)(RC_VER1 + 0x052))
#define TPM_RC_NEEDS_TEST         ((TPM_RC)(RC_VER1 + 0x053))
#define TPM_RC_NO_RESULT          ((TPM_RC)(RC_VER1 + 0x054))
#define TPM_RC_SENSITIVE          ((TPM_RC)(RC_VER1 + 0x055))
#define RC_MAX_FM0                ((TPM_RC)(RC_VER1 + 0x07F))
#define RC_FMT1                                         0x080
#define TPM_RC_ASYMMETRIC         ((TPM_RC)(RC_FMT1 + 0x001))
#define TPM_RC_ATTRIBUTES         ((TPM_RC)(RC_FMT1 + 0x002))
#define TPM_RC_HASH               ((TPM_RC)(RC_FMT1 + 0x003))
#define TPM_RC_VALUE              ((TPM_RC)(RC_FMT1 + 0x004))
#define TPM_RC_HIERARCHY          ((TPM_RC)(RC_FMT1 + 0x005))
#define TPM_RC_KEY_SIZE           ((TPM_RC)(RC_FMT1 + 0x007))
#define TPM_RC_MGF                ((TPM_RC)(RC_FMT1 + 0x008))
#define TPM_RC_MODE               ((TPM_RC)(RC_FMT1 + 0x009))
#define TPM_RC_TYPE               ((TPM_RC)(RC_FMT1 + 0x00A))
#define TPM_RC_HANDLE             ((TPM_RC)(RC_FMT1 + 0x00B))
#define TPM_RC_KDF                ((TPM_RC)(RC_FMT1 + 0x00C))
#define TPM_RC_RANGE              ((TPM_RC)(RC_FMT1 + 0x00D))
#define TPM_RC_AUTH_FAIL          ((TPM_RC)(RC_FMT1 + 0x00E))
#define TPM_RC_NONCE              ((TPM_RC)(RC_FMT1 + 0x00F))
#define TPM_RC_PP                 ((TPM_RC)(RC_FMT1 + 0x010))
#define TPM_RC_SCHEME             ((TPM_RC)(RC_FMT1 + 0x012))
#define TPM_RC_SIZE               ((TPM_RC)(RC_FMT1 + 0x015))
#define TPM_RC_SYMMETRIC          ((TPM_RC)(RC_FMT1 + 0x016))
#define TPM_RC_TAG                ((TPM_RC)(RC_FMT1 + 0x017))
#define TPM_RC_SELECTOR           ((TPM_RC)(RC_FMT1 + 0x018))
#define TPM_RC_INSUFFICIENT       ((TPM_RC)(RC_FMT1 + 0x01A))
#define TPM_RC_SIGNATURE          ((TPM_RC)(RC_FMT1 + 0x01B))
#define TPM_RC_KEY                ((TPM_RC)(RC_FMT1 + 0x01C))
#define TPM_RC_POLICY_FAIL        ((TPM_RC)(RC_FMT1 + 0x01D))
#define TPM_RC_INTEGRITY          ((TPM_RC)(RC_FMT1 + 0x01F))
#define TPM_RC_TICKET             ((TPM_RC)(RC_FMT1 + 0x020))
#define TPM_RC_RESERVED_BITS      ((TPM_RC)(RC_FMT1 + 0x021))
#define TPM_RC_BAD_AUTH           ((TPM_RC)(RC_FMT1 + 0x022))
#define TPM_RC_EXPIRED            ((TPM_RC)(RC_FMT1 + 0x023))
#define TPM_RC_POLICY_CC          ((TPM_RC)(RC_FMT1 + 0x024))
#define TPM_RC_BINDING            ((TPM_RC)(RC_FMT1 + 0x025))
#define TPM_RC_CURVE              ((TPM_RC)(RC_FMT1 + 0x026))
#define TPM_RC_ECC_POINT          ((TPM_RC)(RC_FMT1 + 0x027))
#define RC_WARN                                         0x900
#define TPM_RC_CONTEXT_GAP        ((TPM_RC)(RC_WARN + 0x001))
#define TPM_RC_OBJECT_MEMORY      ((TPM_RC)(RC_WARN + 0x002))
#define TPM_RC_SESSION_MEMORY     ((TPM_RC)(RC_WARN + 0x003))
#define TPM_RC_MEMORY             ((TPM_RC)(RC_WARN + 0x004))
#define TPM_RC_SESSION_HANDLES    ((TPM_RC)(RC_WARN + 0x005))
#define TPM_RC_OBJECT_HANDLES     ((TPM_RC)(RC_WARN + 0x006))
#define TPM_RC_LOCALITY           ((TPM_RC)(RC_WARN + 0x007))
#define TPM_RC_YIELDED            ((TPM_RC)(RC_WARN + 0x008))
#define TPM_RC_CANCELED           ((TPM_RC)(RC_WARN + 0x009))
#define TPM_RC_TESTING            ((TPM_RC)(RC_WARN + 0x00A))
#define TPM_RC_REFERENCE_H0       ((TPM_RC)(RC_WARN + 0x010))
#define TPM_RC_REFERENCE_H1       ((TPM_RC)(RC_WARN + 0x011))
#define TPM_RC_REFERENCE_H2       ((TPM_RC)(RC_WARN + 0x012))
#define TPM_RC_REFERENCE_H3       ((TPM_RC)(RC_WARN + 0x013))
#define TPM_RC_REFERENCE_H4       ((TPM_RC)(RC_WARN + 0x014))
#define TPM_RC_REFERENCE_H5       ((TPM_RC)(RC_WARN + 0x015))
#define TPM_RC_REFERENCE_H6       ((TPM_RC)(RC_WARN + 0x016))
#define TPM_RC_REFERENCE_S0       ((TPM_RC)(RC_WARN + 0x018))
#define TPM_RC_REFERENCE_S1       ((TPM_RC)(RC_WARN + 0x019))
#define TPM_RC_REFERENCE_S2       ((TPM_RC)(RC_WARN + 0x01A))
#define TPM_RC_REFERENCE_S3       ((TPM_RC)(RC_WARN + 0x01B))
#define TPM_RC_REFERENCE_S4       ((TPM_RC)(RC_WARN + 0x01C))
#define TPM_RC_REFERENCE_S5       ((TPM_RC)(RC_WARN + 0x01D))
#define TPM_RC_REFERENCE_S6       ((TPM_RC)(RC_WARN + 0x01E))
#define TPM_RC_NV_RATE            ((TPM_RC)(RC_WARN + 0x020))
#define TPM_RC_LOCKOUT            ((TPM_RC)(RC_WARN + 0x021))
#define TPM_RC_RETRY              ((TPM_RC)(RC_WARN + 0x022))
#define TPM_RC_NV_UNAVAILABLE     ((TPM_RC)(RC_WARN + 0x023))
#define TPM_RC_NOT_USED            ((TPM_RC)(RC_WARN + 0x7F))
#define TPM_RC_H                                        0x000
#define TPM_RC_P                                        0x040
#define TPM_RC_S                                        0x800
#define TPM_RC_1                                        0x100
#define TPM_RC_2                                        0x200
#define TPM_RC_3                                        0x300
#define TPM_RC_4                                        0x400
#define TPM_RC_5                                        0x500
#define TPM_RC_6                                        0x600
#define TPM_RC_7                                        0x700
#define TPM_RC_8                                        0x800
#define TPM_RC_9                                        0x900
#define TPM_RC_A                                        0xA00
#define TPM_RC_B                                        0xB00
#define TPM_RC_C                                        0xC00
#define TPM_RC_D                                        0xD00
#define TPM_RC_E                                        0xE00
#define TPM_RC_F                                        0xF00
#define TPM_RC_N_MASK                                   0xF00

// Table 18  Definition of TPM_CLOCK_ADJUST Constants <  IN>
typedef INT8 TPM_CLOCK_ADJUST;
#define TPM_CLOCK_COARSE_SLOWER  -3
#define TPM_CLOCK_MEDIUM_SLOWER  -2
#define TPM_CLOCK_FINE_SLOWER    -1
#define TPM_CLOCK_NO_CHANGE       0
#define TPM_CLOCK_FINE_FASTER     1
#define TPM_CLOCK_MEDIUM_FASTER   2
#define TPM_CLOCK_COARSE_FASTER   3

// Table 19  Definition of TPM_EO Constants <  IN/OUT>
typedef UINT16 TPM_EO;
#define TPM_EO_EQ           0x0000
#define TPM_EO_NEQ          0x0001
#define TPM_EO_SIGNED_GT    0x0002
#define TPM_EO_UNSIGNED_GT  0x0003
#define TPM_EO_SIGNED_LT    0x0004
#define TPM_EO_UNSIGNED_LT  0x0005
#define TPM_EO_SIGNED_GE    0x0006
#define TPM_EO_UNSIGNED_GE  0x0007
#define TPM_EO_SIGNED_LE    0x0008
#define TPM_EO_UNSIGNED_LE  0x0009
#define TPM_EO_BITSET       0x000A
#define TPM_EO_BITCLEAR     0x000B

// Table 20  Definition of TPM_ST Constants <  IN/OUT, S>
typedef UINT16 TPM_ST;
#define TPM_ST_RSP_COMMAND           0x00C4
#define TPM_ST_NULL                  0X8000
#define TPM_ST_NO_SESSIONS           0x8001
#define TPM_ST_SESSIONS              0x8002
#define TPM_ST_ATTEST_NV             0x8014
#define TPM_ST_ATTEST_COMMAND_AUDIT  0x8015
#define TPM_ST_ATTEST_SESSION_AUDIT  0x8016
#define TPM_ST_ATTEST_CERTIFY        0x8017
#define TPM_ST_ATTEST_QUOTE          0x8018
#define TPM_ST_ATTEST_TIME           0x8019
#define TPM_ST_ATTEST_CREATION       0x801A
#define TPM_ST_CREATION              0x8021
#define TPM_ST_VERIFIED              0x8022
#define TPM_ST_AUTH_SECRET           0x8023
#define TPM_ST_HASHCHECK             0x8024
#define TPM_ST_AUTH_SIGNED           0x8025
#define TPM_ST_FU_MANIFEST           0x8029

// Table 21  Definition of TPM_SU Constants <  IN>
typedef UINT16 TPM_SU;
#define TPM_SU_CLEAR  0x0000
#define TPM_SU_STATE  0x0001

// Table 22  Definition of TPM_SE Constants <  IN>
typedef UINT8 TPM_SE;
#define TPM_SE_HMAC    0x00
#define TPM_SE_POLICY  0x01
#define TPM_SE_TRIAL   0x03

// Table 23  Definition of TPM_CAP Constants
typedef UINT32 TPM_CAP;
#define TPM_CAP_FIRST            0x00000000
#define TPM_CAP_ALGS             0x00000000
#define TPM_CAP_HANDLES          0x00000001
#define TPM_CAP_COMMANDS         0x00000002
#define TPM_CAP_PP_COMMANDS      0x00000003
#define TPM_CAP_AUDIT_COMMANDS   0x00000004
#define TPM_CAP_PCRS             0x00000005
#define TPM_CAP_TPM_PROPERTIES   0x00000006
#define TPM_CAP_PCR_PROPERTIES   0x00000007
#define TPM_CAP_ECC_CURVES       0x00000008
#define TPM_CAP_LAST             0x00000008
#define TPM_CAP_VENDOR_PROPERTY  0x00000100

// Table 24  Definition of TPM_PT Constants <  IN/OUT, S>
typedef UINT32 TPM_PT;
#define TPM_PT_NONE                                0x00000000
#define PT_GROUP                                   0x00000100
#define PT_FIXED                               (PT_GROUP * 1)
#define TPM_PT_FAMILY_INDICATOR      ((TPM_PT)(PT_FIXED + 0))
#define TPM_PT_LEVEL                 ((TPM_PT)(PT_FIXED + 1))
#define TPM_PT_REVISION              ((TPM_PT)(PT_FIXED + 2))
#define TPM_PT_DAY_OF_YEAR           ((TPM_PT)(PT_FIXED + 3))
#define TPM_PT_YEAR                  ((TPM_PT)(PT_FIXED + 4))
#define TPM_PT_MANUFACTURER          ((TPM_PT)(PT_FIXED + 5))
#define TPM_PT_VENDOR_STRING_1       ((TPM_PT)(PT_FIXED + 6))
#define TPM_PT_VENDOR_STRING_2       ((TPM_PT)(PT_FIXED + 7))
#define TPM_PT_VENDOR_STRING_3       ((TPM_PT)(PT_FIXED + 8))
#define TPM_PT_VENDOR_STRING_4       ((TPM_PT)(PT_FIXED + 9))
#define TPM_PT_VENDOR_TPM_TYPE      ((TPM_PT)(PT_FIXED + 10))
#define TPM_PT_FIRMWARE_VERSION_1   ((TPM_PT)(PT_FIXED + 11))
#define TPM_PT_FIRMWARE_VERSION_2   ((TPM_PT)(PT_FIXED + 12))
#define TPM_PT_INPUT_BUFFER         ((TPM_PT)(PT_FIXED + 13))
#define TPM_PT_HR_TRANSIENT_MIN     ((TPM_PT)(PT_FIXED + 14))
#define TPM_PT_HR_PERSISTENT_MIN    ((TPM_PT)(PT_FIXED + 15))
#define TPM_PT_HR_LOADED_MIN        ((TPM_PT)(PT_FIXED + 16))
#define TPM_PT_ACTIVE_SESSIONS_MAX  ((TPM_PT)(PT_FIXED + 17))
#define TPM_PT_PCR_COUNT            ((TPM_PT)(PT_FIXED + 18))
#define TPM_PT_PCR_SELECT_MIN       ((TPM_PT)(PT_FIXED + 19))
#define TPM_PT_CONTEXT_GAP_MAX      ((TPM_PT)(PT_FIXED + 20))
#define TPM_PT_NV_COUNTERS_MAX      ((TPM_PT)(PT_FIXED + 22))
#define TPM_PT_NV_INDEX_MAX         ((TPM_PT)(PT_FIXED + 23))
#define TPM_PT_MEMORY               ((TPM_PT)(PT_FIXED + 24))
#define TPM_PT_CLOCK_UPDATE         ((TPM_PT)(PT_FIXED + 25))
#define TPM_PT_CONTEXT_HASH         ((TPM_PT)(PT_FIXED + 26))
#define TPM_PT_CONTEXT_SYM          ((TPM_PT)(PT_FIXED + 27))
#define TPM_PT_CONTEXT_SYM_SIZE     ((TPM_PT)(PT_FIXED + 28))
#define TPM_PT_ORDERLY_COUNT        ((TPM_PT)(PT_FIXED + 29))
#define TPM_PT_MAX_COMMAND_SIZE     ((TPM_PT)(PT_FIXED + 30))
#define TPM_PT_MAX_RESPONSE_SIZE    ((TPM_PT)(PT_FIXED + 31))
#define TPM_PT_MAX_DIGEST           ((TPM_PT)(PT_FIXED + 32))
#define TPM_PT_MAX_OBJECT_CONTEXT   ((TPM_PT)(PT_FIXED + 33))
#define TPM_PT_MAX_SESSION_CONTEXT  ((TPM_PT)(PT_FIXED + 34))
#define TPM_PT_PS_FAMILY_INDICATOR  ((TPM_PT)(PT_FIXED + 35))
#define TPM_PT_PS_LEVEL             ((TPM_PT)(PT_FIXED + 36))
#define TPM_PT_PS_REVISION          ((TPM_PT)(PT_FIXED + 37))
#define TPM_PT_PS_DAY_OF_YEAR       ((TPM_PT)(PT_FIXED + 38))
#define TPM_PT_PS_YEAR              ((TPM_PT)(PT_FIXED + 39))
#define TPM_PT_SPLIT_MAX            ((TPM_PT)(PT_FIXED + 40))
#define TPM_PT_TOTAL_COMMANDS       ((TPM_PT)(PT_FIXED + 41))
#define TPM_PT_LIBRARY_COMMANDS     ((TPM_PT)(PT_FIXED + 42))
#define TPM_PT_VENDOR_COMMANDS      ((TPM_PT)(PT_FIXED + 43))
#define TPM_PT_NV_BUFFER_MAX        ((TPM_PT)(PT_FIXED + 44))
#define PT_VAR                                 (PT_GROUP * 2)
#define TPM_PT_PERMANENT               ((TPM_PT)(PT_VAR + 0))
#define TPM_PT_STARTUP_CLEAR           ((TPM_PT)(PT_VAR + 1))
#define TPM_PT_HR_NV_INDEX             ((TPM_PT)(PT_VAR + 2))
#define TPM_PT_HR_LOADED               ((TPM_PT)(PT_VAR + 3))
#define TPM_PT_HR_LOADED_AVAIL         ((TPM_PT)(PT_VAR + 4))
#define TPM_PT_HR_ACTIVE               ((TPM_PT)(PT_VAR + 5))
#define TPM_PT_HR_ACTIVE_AVAIL         ((TPM_PT)(PT_VAR + 6))
#define TPM_PT_HR_TRANSIENT_AVAIL      ((TPM_PT)(PT_VAR + 7))
#define TPM_PT_HR_PERSISTENT           ((TPM_PT)(PT_VAR + 8))
#define TPM_PT_HR_PERSISTENT_AVAIL     ((TPM_PT)(PT_VAR + 9))
#define TPM_PT_NV_COUNTERS            ((TPM_PT)(PT_VAR + 10))
#define TPM_PT_NV_COUNTERS_AVAIL      ((TPM_PT)(PT_VAR + 11))
#define TPM_PT_ALGORITHM_SET          ((TPM_PT)(PT_VAR + 12))
#define TPM_PT_LOADED_CURVES          ((TPM_PT)(PT_VAR + 13))
#define TPM_PT_LOCKOUT_COUNTER        ((TPM_PT)(PT_VAR + 14))
#define TPM_PT_MAX_AUTH_FAIL          ((TPM_PT)(PT_VAR + 15))
#define TPM_PT_LOCKOUT_INTERVAL       ((TPM_PT)(PT_VAR + 16))
#define TPM_PT_LOCKOUT_RECOVERY       ((TPM_PT)(PT_VAR + 17))
#define TPM_PT_NV_WRITE_RECOVERY      ((TPM_PT)(PT_VAR + 18))
#define TPM_PT_AUDIT_COUNTER_0        ((TPM_PT)(PT_VAR + 19))
#define TPM_PT_AUDIT_COUNTER_1        ((TPM_PT)(PT_VAR + 20))

// Table 25  Definition of TPM_PT_PCR Constants <  IN/OUT, S>
typedef UINT32 TPM_PT_PCR;
#define TPM_PT_PCR_FIRST         0x00000000
#define TPM_PT_PCR_SAVE          0x00000000
#define TPM_PT_PCR_EXTEND_L0     0x00000001
#define TPM_PT_PCR_RESET_L0      0x00000002
#define TPM_PT_PCR_EXTEND_L1     0x00000003
#define TPM_PT_PCR_RESET_L1      0x00000004
#define TPM_PT_PCR_EXTEND_L2     0x00000005
#define TPM_PT_PCR_RESET_L2      0x00000006
#define TPM_PT_PCR_EXTEND_L3     0x00000007
#define TPM_PT_PCR_RESET_L3      0x00000008
#define TPM_PT_PCR_EXTEND_L4     0x00000009
#define TPM_PT_PCR_RESET_L4      0x0000000A
#define TPM_PT_PCR_NO_INCREMENT  0x00000011
#define TPM_PT_PCR_DRTM_RESET    0x00000012
#define TPM_PT_PCR_POLICY        0x00000013
#define TPM_PT_PCR_AUTH          0x00000014
#define TPM_PT_PCR_LAST          0x00000014

// Table 26  Definition of TPM_PS Constants <  OUT>
typedef UINT32 TPM_PS;
#define TPM_PS_MAIN            0x00000000
#define TPM_PS_PC              0x00000001
#define TPM_PS_PDA             0x00000002
#define TPM_PS_CELL_PHONE      0x00000003
#define TPM_PS_SERVER          0x00000004
#define TPM_PS_PERIPHERAL      0x00000005
#define TPM_PS_TSS             0x00000006
#define TPM_PS_STORAGE         0x00000007
#define TPM_PS_AUTHENTICATION  0x00000008
#define TPM_PS_EMBEDDED        0x00000009
#define TPM_PS_HARDCOPY        0x0000000A
#define TPM_PS_INFRASTRUCTURE  0x0000000B
#define TPM_PS_VIRTUALIZATION  0x0000000C
#define TPM_PS_TNC             0x0000000D
#define TPM_PS_MULTI_TENANT    0x0000000E
#define TPM_PS_TC              0x0000000F

// Table 27  Definition of Types for Handles
typedef UINT32 TPM_HANDLE;

// Table 28  Definition of TPM_HT Constants <  S>
typedef UINT8 TPM_HT;
#define TPM_HT_PCR             0x00
#define TPM_HT_NV_INDEX        0x01
#define TPM_HT_HMAC_SESSION    0x02
#define TPM_HT_LOADED_SESSION  0x02
#define TPM_HT_POLICY_SESSION  0x03
#define TPM_HT_ACTIVE_SESSION  0x03
#define TPM_HT_PERMANENT       0x40
#define TPM_HT_TRANSIENT       0x80
#define TPM_HT_PERSISTENT      0x81

// Table 29  Definition of TPM_RH Constants <  S>
typedef TPM_HANDLE TPM_RH;
#define TPM_RH_FIRST        0x40000000
#define TPM_RH_SRK          0x40000000
#define TPM_RH_OWNER        0x40000001
#define TPM_RH_REVOKE       0x40000002
#define TPM_RH_TRANSPORT    0x40000003
#define TPM_RH_OPERATOR     0x40000004
#define TPM_RH_ADMIN        0x40000005
#define TPM_RH_EK           0x40000006
#define TPM_RH_NULL         0x40000007
#define TPM_RH_UNASSIGNED   0x40000008
#define TPM_RS_PW           0x40000009
#define TPM_RH_LOCKOUT      0x4000000A
#define TPM_RH_ENDORSEMENT  0x4000000B
#define TPM_RH_PLATFORM     0x4000000C
#define TPM_RH_PLATFORM_NV  0x4000000D
#define TPM_RH_AUTH_00      0x40000010
#define TPM_RH_AUTH_FF      0x4000010F
#define TPM_RH_LAST         0x4000010F

// Table 30  Definition of TPM_HC Constants <  S>
typedef TPM_HANDLE TPM_HC;
#define HR_HANDLE_MASK                                            0x00FFFFFF
#define HR_RANGE_MASK                                             0xFF000000
#define HR_SHIFT                                                          24
#define HR_PCR                                     (TPM_HT_PCR <<  HR_SHIFT)
#define HR_HMAC_SESSION                   (TPM_HT_HMAC_SESSION <<  HR_SHIFT)
#define HR_POLICY_SESSION               (TPM_HT_POLICY_SESSION <<  HR_SHIFT)
#define HR_TRANSIENT                         (TPM_HT_TRANSIENT <<  HR_SHIFT)
#define HR_PERSISTENT                       (TPM_HT_PERSISTENT <<  HR_SHIFT)
#define HR_NV_INDEX                           (TPM_HT_NV_INDEX <<  HR_SHIFT)
#define HR_PERMANENT                         (TPM_HT_PERMANENT <<  HR_SHIFT)
#define PCR_FIRST                                               (HR_PCR + 0)
#define PCR_LAST                          (PCR_FIRST + IMPLEMENTATION_PCR-1)
#define HMAC_SESSION_FIRST                             (HR_HMAC_SESSION + 0)
#define HMAC_SESSION_LAST         (HMAC_SESSION_FIRST+MAX_ACTIVE_SESSIONS-1)
#define LOADED_SESSION_FIRST                              HMAC_SESSION_FIRST
#define LOADED_SESSION_LAST                                HMAC_SESSION_LAST
#define POLICY_SESSION_FIRST                         (HR_POLICY_SESSION + 0)
#define POLICY_SESSION_LAST   (POLICY_SESSION_FIRST + MAX_ACTIVE_SESSIONS-1)
#define TRANSIENT_FIRST                                   (HR_TRANSIENT + 0)
#define ACTIVE_SESSION_FIRST                            POLICY_SESSION_FIRST
#define ACTIVE_SESSION_LAST                              POLICY_SESSION_LAST
#define TRANSIENT_LAST                (TRANSIENT_FIRST+MAX_LOADED_OBJECTS-1)
#define PERSISTENT_FIRST                                 (HR_PERSISTENT + 0)
#define PERSISTENT_LAST                      (PERSISTENT_FIRST + 0x00FFFFFF)
#define PLATFORM_PERSISTENT                  (PERSISTENT_FIRST + 0x00800000)
#define NV_INDEX_FIRST                                     (HR_NV_INDEX + 0)
#define NV_INDEX_LAST                          (NV_INDEX_FIRST + 0x00FFFFFF)
#define PERMANENT_FIRST                                         TPM_RH_FIRST
#define PERMANENT_LAST                                           TPM_RH_LAST

// Table 31  Definition of TPMA_ALGORITHM Bits
typedef struct {
  UINT32 asymmetric    : 1;
  UINT32 symmetric     : 1;
  UINT32 hash          : 1;
  UINT32 object        : 1;
  UINT32 reserved4_7   : 4;
  UINT32 signing       : 1;
  UINT32 encrypting    : 1;
  UINT32 method        : 1;
  UINT32 reserved11_31 : 21;
} TPMA_ALGORITHM;

// Table 32  Definition of TPMA_OBJECT Bits
typedef struct {
  UINT32 reserved0            : 1;
  UINT32 fixedTPM             : 1;
  UINT32 stClear              : 1;
  UINT32 reserved3            : 1;
  UINT32 fixedParent          : 1;
  UINT32 sensitiveDataOrigin  : 1;
  UINT32 userWithAuth         : 1;
  UINT32 adminWithPolicy      : 1;
  UINT32 reserved8_9          : 2;
  UINT32 noDA                 : 1;
  UINT32 encryptedDuplication : 1;
  UINT32 reserved12_15        : 4;
  UINT32 restricted           : 1;
  UINT32 decrypt              : 1;
  UINT32 sign                 : 1;
  UINT32 reserved19_31        : 13;
} TPMA_OBJECT;

// Table 33  Definition of TPMA_SESSION Bits <  IN/OUT>
typedef struct {
  UINT8 continueSession : 1;
  UINT8 auditExclusive  : 1;
  UINT8 auditReset      : 1;
  UINT8 reserved3_4     : 2;
  UINT8 decrypt         : 1;
  UINT8 encrypt         : 1;
  UINT8 audit           : 1;
} TPMA_SESSION;

// Table 34  Definition of TPMA_LOCALITY Bits <  IN/OUT>
typedef struct {
  UINT8 locZero  : 1;
  UINT8 locOne   : 1;
  UINT8 locTwo   : 1;
  UINT8 locThree : 1;
  UINT8 locFour  : 1;
  UINT8 Extended : 3;
} TPMA_LOCALITY;

// Table 35  Definition of TPMA_PERMANENT Bits <  OUT>
typedef struct {
  UINT32 ownerAuthSet       : 1;
  UINT32 endorsementAuthSet : 1;
  UINT32 lockoutAuthSet     : 1;
  UINT32 reserved3_7        : 5;
  UINT32 disableClear       : 1;
  UINT32 inLockout          : 1;
  UINT32 tpmGeneratedEPS    : 1;
  UINT32 reserved11_31      : 21;
} TPMA_PERMANENT;

// Table 36  Definition of TPMA_STARTUP_CLEAR Bits <  OUT>
typedef struct {
  UINT32 phEnable     : 1;
  UINT32 shEnable     : 1;
  UINT32 ehEnable     : 1;
  UINT32 phEnableNV   : 1;
  UINT32 reserved4_30 : 27;
  UINT32 orderly      : 1;
} TPMA_STARTUP_CLEAR;

// Table 37  Definition of TPMA_MEMORY Bits <  Out>
typedef struct {
  UINT32 sharedRAM         : 1;
  UINT32 sharedNV          : 1;
  UINT32 objectCopiedToRam : 1;
  UINT32 reserved3_31      : 29;
} TPMA_MEMORY;

// Table 38  Definition of TPMA_CC Bits <  OUT>
typedef struct {
  TPM_CC commandIndex  : 16;
  TPM_CC reserved16_21 : 6;
  TPM_CC nv            : 1;
  TPM_CC extensive     : 1;
  TPM_CC flushed       : 1;
  TPM_CC cHandles      : 3;
  TPM_CC rHandle       : 1;
  TPM_CC V             : 1;
  TPM_CC Res           : 2;
} TPMA_CC;

// Table 39  Definition of TPMI_YES_NO Type
typedef BYTE TPMI_YES_NO;
// Table 40  Definition of TPMI_DH_OBJECT Type
typedef TPM_HANDLE TPMI_DH_OBJECT;
// Table 41  Definition of TPMI_DH_PERSISTENT Type
typedef TPM_HANDLE TPMI_DH_PERSISTENT;
// Table 42  Definition of TPMI_DH_ENTITY Type <  IN>
typedef TPM_HANDLE TPMI_DH_ENTITY;
// Table 43  Definition of TPMI_DH_PCR Type <  IN>
typedef TPM_HANDLE TPMI_DH_PCR;
// Table 44  Definition of TPMI_SH_AUTH_SESSION Type <  IN/OUT>
typedef TPM_HANDLE TPMI_SH_AUTH_SESSION;
// Table 45  Definition of TPMI_SH_HMAC Type <  IN/OUT>
typedef TPM_HANDLE TPMI_SH_HMAC;
// Table 46  Definition of TPMI_SH_POLICY Type <  IN/OUT>
typedef TPM_HANDLE TPMI_SH_POLICY;
// Table 47  Definition of TPMI_DH_CONTEXT Type
typedef TPM_HANDLE TPMI_DH_CONTEXT;
// Table 48  Definition of TPMI_RH_HIERARCHY Type
typedef TPM_HANDLE TPMI_RH_HIERARCHY;
// Table 49  Definition of TPMI_RH_ENABLES Type
typedef TPM_HANDLE TPMI_RH_ENABLES;
// Table 50  Definition of TPMI_RH_HIERARCHY_AUTH Type <  IN>
typedef TPM_HANDLE TPMI_RH_HIERARCHY_AUTH;
// Table 51  Definition of TPMI_RH_PLATFORM Type <  IN>
typedef TPM_HANDLE TPMI_RH_PLATFORM;
// Table 52  Definition of TPMI_RH_OWNER Type <  IN>
typedef TPM_HANDLE TPMI_RH_OWNER;
// Table 53  Definition of TPMI_RH_ENDORSEMENT Type <  IN>
typedef TPM_HANDLE TPMI_RH_ENDORSEMENT;
// Table 54  Definition of TPMI_RH_PROVISION Type <  IN>
typedef TPM_HANDLE TPMI_RH_PROVISION;
// Table 55  Definition of TPMI_RH_CLEAR Type <  IN>
typedef TPM_HANDLE TPMI_RH_CLEAR;
// Table 56  Definition of TPMI_RH_NV_AUTH Type <  IN>
typedef TPM_HANDLE TPMI_RH_NV_AUTH;
// Table 57  Definition of TPMI_RH_LOCKOUT Type <  IN>
typedef TPM_HANDLE TPMI_RH_LOCKOUT;
// Table 58  Definition of TPMI_RH_NV_INDEX Type <  IN/OUT>
typedef TPM_HANDLE TPMI_RH_NV_INDEX;
// Table 59  Definition of TPMI_ALG_HASH Type
typedef TPM_ALG_ID TPMI_ALG_HASH;
// Table 60  Definition of TPMI_ALG_ASYM Type
typedef TPM_ALG_ID TPMI_ALG_ASYM;
// Table 61  Definition of TPMI_ALG_SYM Type
typedef TPM_ALG_ID TPMI_ALG_SYM;
// Table 62  Definition of TPMI_ALG_SYM_OBJECT Type
typedef TPM_ALG_ID TPMI_ALG_SYM_OBJECT;
// Table 63  Definition of TPMI_ALG_SYM_MODE Type
typedef TPM_ALG_ID TPMI_ALG_SYM_MODE;
// Table 64  Definition of TPMI_ALG_KDF Type
typedef TPM_ALG_ID TPMI_ALG_KDF;
// Table 65  Definition of TPMI_ALG_SIG_SCHEME Type
typedef TPM_ALG_ID TPMI_ALG_SIG_SCHEME;
// Table 66  Definition of TPMI_ECC_KEY_EXCHANGE Type
typedef TPM_ALG_ID TPMI_ECC_KEY_EXCHANGE;
// Table 67  Definition of TPMI_ST_COMMAND_TAG Type
typedef TPM_ST TPMI_ST_COMMAND_TAG;
// Table 68  Definition of TPMS_EMPTY Structure <  IN/OUT>
typedef struct {
} TPMS_EMPTY;

// Table 69  Definition of TPMS_ALGORITHM_DESCRIPTION Structure <  OUT>
typedef struct {
  TPM_ALG_ID      alg;
  TPMA_ALGORITHM  attributes;
} TPMS_ALGORITHM_DESCRIPTION;

// Table 70  Definition of TPMU_HA Union <  IN/OUT, S>
typedef union {
#ifdef TPM_ALG_SHA
  BYTE  sha[SHA_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SHA1
  BYTE  sha1[SHA1_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SHA256
  BYTE  sha256[SHA256_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SHA384
  BYTE  sha384[SHA384_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SHA512
  BYTE  sha512[SHA512_DIGEST_SIZE];
#endif
#ifdef TPM_ALG_SM3_256
  BYTE  sm3_256[SM3_256_DIGEST_SIZE];
#endif
} TPMU_HA;

// Table 71  Definition of TPMT_HA Structure <  IN/OUT>
typedef struct {
  TPMI_ALG_HASH  hashAlg;
  TPMU_HA        digest;
} TPMT_HA;

// Table 72  Definition of TPM2B_DIGEST Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[sizeof(TPMU_HA)];
  } t;
  TPM2B b;
} TPM2B_DIGEST;

// Table 73  Definition of TPM2B_DATA Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[sizeof(TPMT_HA)];
  } t;
  TPM2B b;
} TPM2B_DATA;

// Table 74  Definition of Types for TPM2B_NONCE
typedef TPM2B_DIGEST TPM2B_NONCE;

// Table 75  Definition of Types for TPM2B_AUTH
typedef TPM2B_DIGEST TPM2B_AUTH;

// Table 76  Definition of Types for TPM2B_OPERAND
typedef TPM2B_DIGEST TPM2B_OPERAND;

// Table 77  Definition of TPM2B_EVENT Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[1024];
  } t;
  TPM2B b;
} TPM2B_EVENT;

// Table 78  Definition of TPM2B_MAX_BUFFER Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[MAX_DIGEST_BUFFER];
  } t;
  TPM2B b;
} TPM2B_MAX_BUFFER;

// Table 79  Definition of TPM2B_MAX_NV_BUFFER Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[MAX_NV_BUFFER_SIZE];
  } t;
  TPM2B b;
} TPM2B_MAX_NV_BUFFER;

// Table 80  Definition of TPM2B_TIMEOUT Structure <  IN/OUT>
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[sizeof(UINT64)];
  } t;
  TPM2B b;
} TPM2B_TIMEOUT;

// Table 81  Definition of TPM2B_IV Structure <  IN/OUT>
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[MAX_SYM_BLOCK_SIZE];
  } t;
  TPM2B b;
} TPM2B_IV;

// Table 82  Definition of TPMU_NAME Union <>
typedef union {
  TPMT_HA     digest;
  TPM_HANDLE  handle;
} TPMU_NAME;

// Table 83  Definition of TPM2B_NAME Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    name[sizeof(TPMU_NAME)];
  } t;
  TPM2B b;
} TPM2B_NAME;

// Table 84  Definition of TPMS_PCR_SELECT Structure
typedef struct {
  UINT8  sizeofSelect;
  BYTE   pcrSelect[PCR_SELECT_MAX];
} TPMS_PCR_SELECT;

// Table 85  Definition of TPMS_PCR_SELECTION Structure
typedef struct {
  TPMI_ALG_HASH  hash;
  UINT8          sizeofSelect;
  BYTE           pcrSelect[PCR_SELECT_MAX];
} TPMS_PCR_SELECTION;

// Unprocessed: Table 86  Values for   proof   Used in Tickets
// Unprocessed: Table 87  General Format of a Ticket
// Table 88  Definition of TPMT_TK_CREATION Structure
typedef struct {
  TPM_ST             tag;
  TPMI_RH_HIERARCHY  hierarchy;
  TPM2B_DIGEST       digest;
} TPMT_TK_CREATION;

// Table 89  Definition of TPMT_TK_VERIFIED Structure
typedef struct {
  TPM_ST             tag;
  TPMI_RH_HIERARCHY  hierarchy;
  TPM2B_DIGEST       digest;
} TPMT_TK_VERIFIED;

// Table 90  Definition of TPMT_TK_AUTH Structure
typedef struct {
  TPM_ST             tag;
  TPMI_RH_HIERARCHY  hierarchy;
  TPM2B_DIGEST       digest;
} TPMT_TK_AUTH;

// Table 91  Definition of TPMT_TK_HASHCHECK Structure
typedef struct {
  TPM_ST             tag;
  TPMI_RH_HIERARCHY  hierarchy;
  TPM2B_DIGEST       digest;
} TPMT_TK_HASHCHECK;

// Table 92  Definition of TPMS_ALG_PROPERTY Structure <  OUT>
typedef struct {
  TPM_ALG_ID      alg;
  TPMA_ALGORITHM  algProperties;
} TPMS_ALG_PROPERTY;

// Table 93  Definition of TPMS_TAGGED_PROPERTY Structure <  OUT>
typedef struct {
  TPM_PT  property;
  UINT32  value;
} TPMS_TAGGED_PROPERTY;

// Table 94  Definition of TPMS_TAGGED_PCR_SELECT Structure <  OUT>
typedef struct {
  TPM_PT  tag;
  UINT8   sizeofSelect;
  BYTE    pcrSelect[PCR_SELECT_MAX];
} TPMS_TAGGED_PCR_SELECT;

// Table 95  Definition of TPML_CC Structure
typedef struct {
  UINT32  count;
  TPM_CC  commandCodes[MAX_CAP_CC];
} TPML_CC;

// Table 96  Definition of TPML_CCA Structure <  OUT>
typedef struct {
  UINT32   count;
  TPMA_CC  commandAttributes[MAX_CAP_CC];
} TPML_CCA;

// Table 97  Definition of TPML_ALG Structure
typedef struct {
  UINT32      count;
  TPM_ALG_ID  algorithms[MAX_ALG_LIST_SIZE];
} TPML_ALG;

// Table 98  Definition of TPML_HANDLE Structure <  OUT>
typedef struct {
  UINT32      count;
  TPM_HANDLE  handle[MAX_CAP_HANDLES];
} TPML_HANDLE;

// Table 99  Definition of TPML_DIGEST Structure
typedef struct {
  UINT32        count;
  TPM2B_DIGEST  digests[8];
} TPML_DIGEST;

// Table 100  Definition of TPML_DIGEST_VALUES Structure
typedef struct {
  UINT32   count;
  TPMT_HA  digests[HASH_COUNT];
} TPML_DIGEST_VALUES;

// Table 101  Definition of TPM2B_DIGEST_VALUES Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[sizeof(TPML_DIGEST_VALUES)];
  } t;
  TPM2B b;
} TPM2B_DIGEST_VALUES;

// Table 102  Definition of TPML_PCR_SELECTION Structure
typedef struct {
  UINT32              count;
  TPMS_PCR_SELECTION  pcrSelections[HASH_COUNT];
} TPML_PCR_SELECTION;

// Table 103  Definition of TPML_ALG_PROPERTY Structure <  OUT>
typedef struct {
  UINT32             count;
  TPMS_ALG_PROPERTY  algProperties[MAX_CAP_ALGS];
} TPML_ALG_PROPERTY;

// Table 104  Definition of TPML_TAGGED_TPM_PROPERTY Structure <  OUT>
typedef struct {
  UINT32                count;
  TPMS_TAGGED_PROPERTY  tpmProperty[MAX_TPM_PROPERTIES];
} TPML_TAGGED_TPM_PROPERTY;

// Table 105  Definition of TPML_TAGGED_PCR_PROPERTY Structure <  OUT>
typedef struct {
  UINT32                  count;
  TPMS_TAGGED_PCR_SELECT  pcrProperty[MAX_PCR_PROPERTIES];
} TPML_TAGGED_PCR_PROPERTY;

// Table 106  Definition of TPML_ECC_CURVE Structure <  OUT>
typedef struct {
  UINT32         count;
  TPM_ECC_CURVE  eccCurves[MAX_ECC_CURVES];
} TPML_ECC_CURVE;

// Table 107  Definition of TPMU_CAPABILITIES Union <  OUT>
typedef union {
  TPML_ALG_PROPERTY         algorithms;
  TPML_HANDLE               handles;
  TPML_CCA                  command;
  TPML_CC                   ppCommands;
  TPML_CC                   auditCommands;
  TPML_PCR_SELECTION        assignedPCR;
  TPML_TAGGED_TPM_PROPERTY  tpmProperties;
  TPML_TAGGED_PCR_PROPERTY  pcrProperties;
  TPML_ECC_CURVE            eccCurves;
} TPMU_CAPABILITIES;

// Table 108  Definition of TPMS_CAPABILITY_DATA Structure <  OUT>
typedef struct {
  TPM_CAP            capability;
  TPMU_CAPABILITIES  data;
} TPMS_CAPABILITY_DATA;

// Table 109  Definition of TPMS_CLOCK_INFO Structure
typedef struct {
  UINT64       clock;
  UINT32       resetCount;
  UINT32       restartCount;
  TPMI_YES_NO  safe;
} TPMS_CLOCK_INFO;

// Table 110  Definition of TPMS_TIME_INFO Structure
typedef struct {
  UINT64           time;
  TPMS_CLOCK_INFO  clockInfo;
} TPMS_TIME_INFO;

// Table 111  Definition of TPMS_TIME_ATTEST_INFO Structure <  OUT>
typedef struct {
  TPMS_TIME_INFO  time;
  UINT64          firmwareVersion;
} TPMS_TIME_ATTEST_INFO;

// Table 112  Definition of TPMS_CERTIFY_INFO Structure <  OUT>
typedef struct {
  TPM2B_NAME  name;
  TPM2B_NAME  qualifiedName;
} TPMS_CERTIFY_INFO;

// Table 113  Definition of TPMS_QUOTE_INFO Structure <  OUT>
typedef struct {
  TPML_PCR_SELECTION  pcrSelect;
  TPM2B_DIGEST        pcrDigest;
} TPMS_QUOTE_INFO;

// Table 114  Definition of TPMS_COMMAND_AUDIT_INFO Structure <  OUT>
typedef struct {
  UINT64        auditCounter;
  TPM_ALG_ID    digestAlg;
  TPM2B_DIGEST  auditDigest;
  TPM2B_DIGEST  commandDigest;
} TPMS_COMMAND_AUDIT_INFO;

// Table 115  Definition of TPMS_SESSION_AUDIT_INFO Structure <  OUT>
typedef struct {
  TPMI_YES_NO   exclusiveSession;
  TPM2B_DIGEST  sessionDigest;
} TPMS_SESSION_AUDIT_INFO;

// Table 116  Definition of TPMS_CREATION_INFO Structure <  OUT>
typedef struct {
  TPM2B_NAME    objectName;
  TPM2B_DIGEST  creationHash;
} TPMS_CREATION_INFO;

// Table 117  Definition of TPMS_NV_CERTIFY_INFO Structure <  OUT>
typedef struct {
  TPM2B_NAME           indexName;
  UINT16               offset;
  TPM2B_MAX_NV_BUFFER  nvContents;
} TPMS_NV_CERTIFY_INFO;

// Table 118  Definition of TPMI_ST_ATTEST Type <  OUT>
typedef TPM_ST TPMI_ST_ATTEST;
// Table 119  Definition of TPMU_ATTEST Union <  OUT>
typedef union {
  TPMS_CERTIFY_INFO        certify;
  TPMS_CREATION_INFO       creation;
  TPMS_QUOTE_INFO          quote;
  TPMS_COMMAND_AUDIT_INFO  commandAudit;
  TPMS_SESSION_AUDIT_INFO  sessionAudit;
  TPMS_TIME_ATTEST_INFO    time;
  TPMS_NV_CERTIFY_INFO     nv;
} TPMU_ATTEST;

// Table 120  Definition of TPMS_ATTEST Structure <  OUT>
typedef struct {
  TPM_GENERATED    magic;
  TPMI_ST_ATTEST   type;
  TPM2B_NAME       qualifiedSigner;
  TPM2B_DATA       extraData;
  TPMS_CLOCK_INFO  clockInfo;
  UINT64           firmwareVersion;
  TPMU_ATTEST      attested;
} TPMS_ATTEST;

// Table 121  Definition of TPM2B_ATTEST Structure <  OUT>
typedef union {
  struct {
    UINT16  size;
    BYTE    attestationData[sizeof(TPMS_ATTEST)];
  } t;
  TPM2B b;
} TPM2B_ATTEST;

// Table 122  Definition of TPMS_AUTH_COMMAND Structure <  IN>
typedef struct {
  TPMI_SH_AUTH_SESSION  sessionHandle;
  TPM2B_NONCE           nonce;
  TPMA_SESSION          sessionAttributes;
  TPM2B_AUTH            hmac;
} TPMS_AUTH_COMMAND;

// Table 123  Definition of TPMS_AUTH_RESPONSE Structure <  OUT>
typedef struct {
  TPM2B_NONCE   nonce;
  TPMA_SESSION  sessionAttributes;
  TPM2B_AUTH    hmac;
} TPMS_AUTH_RESPONSE;

// Table 124  Definition of   TPMI_!ALG.S_KEY_BITS Type
typedef TPM_KEY_BITS TPMI_AES_KEY_BITS;
typedef TPM_KEY_BITS TPMI_SM4_KEY_BITS;
typedef TPM_KEY_BITS TPMI_CAMELLIA_KEY_BITS;


// Table 125  Definition of TPMU_SYM_KEY_BITS Union
typedef union {
#ifdef TPM_ALG_AES
  TPMI_AES_KEY_BITS       aes;
#endif
#ifdef TPM_ALG_SM4
  TPMI_SM4_KEY_BITS       sm4;
#endif
#ifdef TPM_ALG_CAMELLIA
  TPMI_CAMELLIA_KEY_BITS  camellia;
#endif
  TPM_KEY_BITS            sym;
#ifdef TPM_ALG_XOR
  TPMI_ALG_HASH           xor_;
#endif
} TPMU_SYM_KEY_BITS;

// Table 126  Definition of TPMU_SYM_MODE Union
typedef union {
#ifdef TPM_ALG_AES
  TPMI_ALG_SYM_MODE  aes;
#endif
#ifdef TPM_ALG_SM4
  TPMI_ALG_SYM_MODE  sm4;
#endif
#ifdef TPM_ALG_CAMELLIA
  TPMI_ALG_SYM_MODE  camellia;
#endif
  TPMI_ALG_SYM_MODE  sym;
} TPMU_SYM_MODE;

// Table 127 xDefinition of TPMU_SYM_DETAILS Union
typedef union {
} TPMU_SYM_DETAILS;

// Table 128  Definition of TPMT_SYM_DEF Structure
typedef struct {
  TPMI_ALG_SYM       algorithm;
  TPMU_SYM_KEY_BITS  keyBits;
  TPMU_SYM_MODE      mode;
} TPMT_SYM_DEF;

// Table 129  Definition of TPMT_SYM_DEF_OBJECT Structure
typedef struct {
  TPMI_ALG_SYM_OBJECT  algorithm;
  TPMU_SYM_KEY_BITS    keyBits;
  TPMU_SYM_MODE        mode;
} TPMT_SYM_DEF_OBJECT;

// Table 130  Definition of TPM2B_SYM_KEY Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[MAX_SYM_KEY_BYTES];
  } t;
  TPM2B b;
} TPM2B_SYM_KEY;

// Table 131  Definition of TPMS_SYMCIPHER_PARMS Structure
typedef struct {
  TPMT_SYM_DEF_OBJECT  sym;
} TPMS_SYMCIPHER_PARMS;

// Table 132  Definition of TPM2B_SENSITIVE_DATA Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[MAX_SYM_DATA];
  } t;
  TPM2B b;
} TPM2B_SENSITIVE_DATA;

// Table 133  Definition of TPMS_SENSITIVE_CREATE Structure <  IN>
typedef struct {
  TPM2B_AUTH            userAuth;
  TPM2B_SENSITIVE_DATA  data;
} TPMS_SENSITIVE_CREATE;

// Table 134  Definition of TPM2B_SENSITIVE_CREATE Structure <  IN, S>
typedef union {
  struct {
    UINT16                 size;
    TPMS_SENSITIVE_CREATE  sensitive;
  } t;
  TPM2B b;
} TPM2B_SENSITIVE_CREATE;

// Table 135  Definition of TPMS_SCHEME_HASH Structure
typedef struct {
  TPMI_ALG_HASH  hashAlg;
} TPMS_SCHEME_HASH;

// Table 136  Definition of TPMS_SCHEME_ECDAA Structure
typedef struct {
  TPMI_ALG_HASH  hashAlg;
  UINT16         count;
} TPMS_SCHEME_ECDAA;

// Table 137  Definition of TPMI_ALG_KEYEDHASH_SCHEME Type
typedef TPM_ALG_ID TPMI_ALG_KEYEDHASH_SCHEME;
// Table 138  Definition of Types for HMAC_SIG_SCHEME
typedef TPMS_SCHEME_HASH TPMS_SCHEME_HMAC;

// Table 139  Definition of TPMS_SCHEME_XOR Structure
typedef struct {
  TPMI_ALG_HASH  hashAlg;
  TPMI_ALG_KDF   kdf;
} TPMS_SCHEME_XOR;

// Table 140  Definition of TPMU_SCHEME_KEYEDHASH Union <  IN/OUT, S>
typedef union {
#ifdef TPM_ALG_HMAC
  TPMS_SCHEME_HMAC  hmac;
#endif
#ifdef TPM_ALG_XOR
  TPMS_SCHEME_XOR   xor_;
#endif
} TPMU_SCHEME_KEYEDHASH;

// Table 141  Definition of TPMT_KEYEDHASH_SCHEME Structure
typedef struct {
  TPMI_ALG_KEYEDHASH_SCHEME  scheme;
  TPMU_SCHEME_KEYEDHASH      details;
} TPMT_KEYEDHASH_SCHEME;

// Table 142  Definition of Types for RSA Signature Schemes
typedef TPMS_SCHEME_HASH TPMS_SIG_SCHEME_RSASSA;
typedef TPMS_SCHEME_HASH TPMS_SIG_SCHEME_RSAPSS;

// Table 143  Definition of Types for ECC Signature Schemes
typedef TPMS_SCHEME_HASH TPMS_SIG_SCHEME_ECDSA;
typedef TPMS_SCHEME_HASH TPMS_SIG_SCHEME_SM2;
typedef TPMS_SCHEME_HASH TPMS_SIG_SCHEME_ECSCHNORR;
typedef TPMS_SCHEME_ECDAA TPMS_SIG_SCHEME_ECDAA;

// Table 144  Definition of TPMU_SIG_SCHEME Union <  IN/OUT, S>
typedef union {
#ifdef TPM_ALG_RSASSA
  TPMS_SIG_SCHEME_RSASSA     rsassa;
#endif
#ifdef TPM_ALG_RSAPSS
  TPMS_SIG_SCHEME_RSAPSS     rsapss;
#endif
#ifdef TPM_ALG_ECDSA
  TPMS_SIG_SCHEME_ECDSA      ecdsa;
#endif
#ifdef TPM_ALG_ECDAA
  TPMS_SIG_SCHEME_ECDAA      ecdaa;
#endif
#ifdef TPM_ALG_SM2
  TPMS_SIG_SCHEME_SM2        sm2;
#endif
#ifdef TPM_ALG_ECSCHNORR
  TPMS_SIG_SCHEME_ECSCHNORR  ecschnorr;
#endif
#ifdef TPM_ALG_HMAC
  TPMS_SCHEME_HMAC           hmac;
#endif
  TPMS_SCHEME_HASH           any;
} TPMU_SIG_SCHEME;

// Table 145  Definition of TPMT_SIG_SCHEME Structure
typedef struct {
  TPMI_ALG_SIG_SCHEME  scheme;
  TPMU_SIG_SCHEME      details;
} TPMT_SIG_SCHEME;

// Table 146  Definition of Types for Encryption Schemes
typedef TPMS_SCHEME_HASH TPMS_ENC_SCHEME_OAEP;
typedef TPMS_EMPTY TPMS_ENC_SCHEME_RSAES;

// Table 147  Definition of Types for ECC Key Exchange
typedef TPMS_SCHEME_HASH TPMS_KEY_SCHEME_ECDH;
typedef TPMS_SCHEME_HASH TPMS_KEY_SCHEME_ECMQV;

// Table 148  Definition of Types for KDF Schemes
typedef TPMS_SCHEME_HASH TPMS_SCHEME_MGF1;
typedef TPMS_SCHEME_HASH TPMS_SCHEME_KDF1_SP800_56A;
typedef TPMS_SCHEME_HASH TPMS_SCHEME_KDF2;
typedef TPMS_SCHEME_HASH TPMS_SCHEME_KDF1_SP800_108;

// Table 149  Definition of TPMU_KDF_SCHEME Union <  IN/OUT, S>
typedef union {
#ifdef TPM_ALG_MGF1
  TPMS_SCHEME_MGF1            mgf1;
#endif
#ifdef TPM_ALG_KDF1_SP800_56A
  TPMS_SCHEME_KDF1_SP800_56A  kdf1_sp800_56a;
#endif
#ifdef TPM_ALG_KDF2
  TPMS_SCHEME_KDF2            kdf2;
#endif
#ifdef TPM_ALG_KDF1_SP800_108
  TPMS_SCHEME_KDF1_SP800_108  kdf1_sp800_108;
#endif
} TPMU_KDF_SCHEME;

// Table 150  Definition of TPMT_KDF_SCHEME Structure
typedef struct {
  TPMI_ALG_KDF     scheme;
  TPMU_KDF_SCHEME  details;
} TPMT_KDF_SCHEME;

// Table 151  Definition of TPMI_ALG_ASYM_SCHEME Type <>
typedef TPM_ALG_ID TPMI_ALG_ASYM_SCHEME;
// Table 152  Definition of TPMU_ASYM_SCHEME Union
typedef union {
#ifdef TPM_ALG_ECDH
  TPMS_KEY_SCHEME_ECDH       ecdh;
#endif
#ifdef TPM_ALG_ECMQV
  TPMS_KEY_SCHEME_ECMQV      ecmqv;
#endif
#ifdef TPM_ALG_RSASSA
  TPMS_SIG_SCHEME_RSASSA     rsassa;
#endif
#ifdef TPM_ALG_RSAPSS
  TPMS_SIG_SCHEME_RSAPSS     rsapss;
#endif
#ifdef TPM_ALG_ECDSA
  TPMS_SIG_SCHEME_ECDSA      ecdsa;
#endif
#ifdef TPM_ALG_ECDAA
  TPMS_SIG_SCHEME_ECDAA      ecdaa;
#endif
#ifdef TPM_ALG_SM2
  TPMS_SIG_SCHEME_SM2        sm2;
#endif
#ifdef TPM_ALG_ECSCHNORR
  TPMS_SIG_SCHEME_ECSCHNORR  ecschnorr;
#endif
#ifdef TPM_ALG_RSAES
  TPMS_ENC_SCHEME_RSAES      rsaes;
#endif
#ifdef TPM_ALG_OAEP
  TPMS_ENC_SCHEME_OAEP       oaep;
#endif
  TPMS_SCHEME_HASH           anySig;
} TPMU_ASYM_SCHEME;

// Table 153  Definition of TPMT_ASYM_SCHEME Structure <>
typedef struct {
  TPMI_ALG_ASYM_SCHEME  scheme;
  TPMU_ASYM_SCHEME      details;
} TPMT_ASYM_SCHEME;

// Table 154  Definition of TPMI_ALG_RSA_SCHEME Type
typedef TPM_ALG_ID TPMI_ALG_RSA_SCHEME;
// Table 155  Definition of TPMT_RSA_SCHEME Structure
typedef struct {
  TPMI_ALG_RSA_SCHEME  scheme;
  TPMU_ASYM_SCHEME     details;
} TPMT_RSA_SCHEME;

// Table 156  Definition of TPMI_ALG_RSA_DECRYPT Type
typedef TPM_ALG_ID TPMI_ALG_RSA_DECRYPT;
// Table 157  Definition of TPMT_RSA_DECRYPT Structure
typedef struct {
  TPMI_ALG_RSA_DECRYPT  scheme;
  TPMU_ASYM_SCHEME      details;
} TPMT_RSA_DECRYPT;

// Table 158  Definition of TPM2B_PUBLIC_KEY_RSA Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[MAX_RSA_KEY_BYTES];
  } t;
  TPM2B b;
} TPM2B_PUBLIC_KEY_RSA;

// Table 159  Definition of TPMI_RSA_KEY_BITS Type
typedef TPM_KEY_BITS TPMI_RSA_KEY_BITS;
// Table 160  Definition of TPM2B_PRIVATE_KEY_RSA Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[MAX_RSA_KEY_BYTES/2];
  } t;
  TPM2B b;
} TPM2B_PRIVATE_KEY_RSA;

// Table 161  Definition of TPM2B_ECC_PARAMETER Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[MAX_ECC_KEY_BYTES];
  } t;
  TPM2B b;
} TPM2B_ECC_PARAMETER;

// Table 162  Definition of TPMS_ECC_POINT Structure
typedef struct {
  TPM2B_ECC_PARAMETER  x;
  TPM2B_ECC_PARAMETER  y;
} TPMS_ECC_POINT;

// Table   163    Definition of TPM2B_ECC_POINT Structure
typedef union {
  struct {
    UINT16          size;
    TPMS_ECC_POINT  point;
  } t;
  TPM2B b;
} TPM2B_ECC_POINT;

// Table 164  Definition of TPMI_ALG_ECC_SCHEME Type
typedef TPM_ALG_ID TPMI_ALG_ECC_SCHEME;
// Table 165  Definition of TPMI_ECC_CURVE Type
typedef TPM_ECC_CURVE TPMI_ECC_CURVE;
// Table 166  Definition of TPMT_ECC_SCHEME Structure
typedef struct {
  TPMI_ALG_ECC_SCHEME  scheme;
  TPMU_ASYM_SCHEME     details;
} TPMT_ECC_SCHEME;

// Table 167  Definition of TPMS_ALGORITHM_DETAIL_ECC Structure <  OUT>
typedef struct {
  TPM_ECC_CURVE        curveID;
  UINT16               keySize;
  TPMT_KDF_SCHEME      kdf;
  TPMT_ECC_SCHEME      sign;
  TPM2B_ECC_PARAMETER  p;
  TPM2B_ECC_PARAMETER  a;
  TPM2B_ECC_PARAMETER  b;
  TPM2B_ECC_PARAMETER  gX;
  TPM2B_ECC_PARAMETER  gY;
  TPM2B_ECC_PARAMETER  n;
  TPM2B_ECC_PARAMETER  h;
} TPMS_ALGORITHM_DETAIL_ECC;

// Table 168  Definition of TPMS_SIGNATURE_RSA Structure
typedef struct {
  TPMI_ALG_HASH         hash;
  TPM2B_PUBLIC_KEY_RSA  sig;
} TPMS_SIGNATURE_RSA;

// Table 169  Definition of Types for Signature
typedef TPMS_SIGNATURE_RSA TPMS_SIGNATURE_RSASSA;
typedef TPMS_SIGNATURE_RSA TPMS_SIGNATURE_RSAPSS;

// Table 170  Definition of TPMS_SIGNATURE_ECC Structure
typedef struct {
  TPMI_ALG_HASH        hash;
  TPM2B_ECC_PARAMETER  signatureR;
  TPM2B_ECC_PARAMETER  signatureS;
} TPMS_SIGNATURE_ECC;

// Table 171  Definition of Types for TPMS_SIGNATUE_ECC
typedef TPMS_SIGNATURE_ECC TPMS_SIGNATURE_ECDSA;
typedef TPMS_SIGNATURE_ECC TPMS_SIGNATURE_ECDAA;
typedef TPMS_SIGNATURE_ECC TPMS_SIGNATURE_SM2;
typedef TPMS_SIGNATURE_ECC TPMS_SIGNATURE_ECSCHNORR;

// Table 172  Definition of TPMU_SIGNATURE Union <  IN/OUT, S>
typedef union {
#ifdef TPM_ALG_RSASSA
  TPMS_SIGNATURE_RSASSA     rsassa;
#endif
#ifdef TPM_ALG_RSAPSS
  TPMS_SIGNATURE_RSAPSS     rsapss;
#endif
#ifdef TPM_ALG_ECDSA
  TPMS_SIGNATURE_ECDSA      ecdsa;
#endif
#ifdef TPM_ALG_ECDAA
  TPMS_SIGNATURE_ECDAA      ecdaa;
#endif
#ifdef TPM_ALG_SM2
  TPMS_SIGNATURE_SM2        sm2;
#endif
#ifdef TPM_ALG_ECSCHNORR
  TPMS_SIGNATURE_ECSCHNORR  ecschnorr;
#endif
#ifdef TPM_ALG_HMAC
  TPMT_HA                   hmac;
#endif
  TPMS_SCHEME_HASH          any;
} TPMU_SIGNATURE;

// Table 173  Definition of TPMT_SIGNATURE Structure
typedef struct {
  TPMI_ALG_SIG_SCHEME  sigAlg;
  TPMU_SIGNATURE       signature;
} TPMT_SIGNATURE;

// Table 174  Definition of TPMU_ENCRYPTED_SECRET Union <  S>
typedef union {
#ifdef TPM_ALG_ECC
  BYTE  ecc[sizeof(TPMS_ECC_POINT)];
#endif
#ifdef TPM_ALG_RSA
  BYTE  rsa[MAX_RSA_KEY_BYTES];
#endif
#ifdef TPM_ALG_SYMCIPHER
  BYTE  symmetric[sizeof(TPM2B_DIGEST)];
#endif
#ifdef TPM_ALG_KEYEDHASH
  BYTE  keyedHash[sizeof(TPM2B_DIGEST)];
#endif
} TPMU_ENCRYPTED_SECRET;

// Table 175  Definition of TPM2B_ENCRYPTED_SECRET Structure
typedef union {
  struct {
    UINT16  size;
    BYTE    secret[sizeof(TPMU_ENCRYPTED_SECRET)];
  } t;
  TPM2B b;
} TPM2B_ENCRYPTED_SECRET;

// Table 176  Definition of TPMI_ALG_PUBLIC Type
typedef TPM_ALG_ID TPMI_ALG_PUBLIC;
// Table 177  Definition of TPMU_PUBLIC_ID Union <  IN/OUT, S>
typedef union {
#ifdef TPM_ALG_KEYEDHASH
  TPM2B_DIGEST          keyedHash;
#endif
#ifdef TPM_ALG_SYMCIPHER
  TPM2B_DIGEST          sym;
#endif
#ifdef TPM_ALG_RSA
  TPM2B_PUBLIC_KEY_RSA  rsa;
#endif
#ifdef TPM_ALG_ECC
  TPMS_ECC_POINT        ecc;
#endif
} TPMU_PUBLIC_ID;

// Table 178  Definition of TPMS_KEYEDHASH_PARMS Structure
typedef struct {
  TPMT_KEYEDHASH_SCHEME  scheme;
} TPMS_KEYEDHASH_PARMS;

// Table 179  Definition of TPMS_ASYM_PARMS Structure <>
typedef struct {
  TPMT_SYM_DEF_OBJECT  symmetric;
  TPMT_ASYM_SCHEME     scheme;
} TPMS_ASYM_PARMS;

// Table 180  Definition of TPMS_RSA_PARMS Structure
typedef struct {
  TPMT_SYM_DEF_OBJECT  symmetric;
  TPMT_RSA_SCHEME      scheme;
  TPMI_RSA_KEY_BITS    keyBits;
  UINT32               exponent;
} TPMS_RSA_PARMS;

// Table 181  Definition of TPMS_ECC_PARMS Structure
typedef struct {
  TPMT_SYM_DEF_OBJECT  symmetric;
  TPMT_ECC_SCHEME      scheme;
  TPMI_ECC_CURVE       curveID;
  TPMT_KDF_SCHEME      kdf;
} TPMS_ECC_PARMS;

// Table 182  Definition of TPMU_PUBLIC_PARMS Union <  IN/OUT, S>
typedef union {
#ifdef TPM_ALG_KEYEDHASH
  TPMS_KEYEDHASH_PARMS  keyedHashDetail;
#endif
#ifdef TPM_ALG_SYMCIPHER
  TPMS_SYMCIPHER_PARMS  symDetail;
#endif
#ifdef TPM_ALG_RSA
  TPMS_RSA_PARMS        rsaDetail;
#endif
#ifdef TPM_ALG_ECC
  TPMS_ECC_PARMS        eccDetail;
#endif
  TPMS_ASYM_PARMS       asymDetail;
} TPMU_PUBLIC_PARMS;

// Table 183  Definition of TPMT_PUBLIC_PARMS Structure
typedef struct {
  TPMI_ALG_PUBLIC    type;
  TPMU_PUBLIC_PARMS  parameters;
} TPMT_PUBLIC_PARMS;

// Table 184  Definition of TPMT_PUBLIC Structure
typedef struct {
  TPMI_ALG_PUBLIC    type;
  TPMI_ALG_HASH      nameAlg;
  TPMA_OBJECT        objectAttributes;
  TPM2B_DIGEST       authPolicy;
  TPMU_PUBLIC_PARMS  parameters;
  TPMU_PUBLIC_ID     unique;
} TPMT_PUBLIC;

// Table 185  Definition of TPM2B_PUBLIC Structure
typedef union {
  struct {
    UINT16       size;
    TPMT_PUBLIC  publicArea;
  } t;
  TPM2B b;
} TPM2B_PUBLIC;

// Table 186  Definition of TPM2B_PRIVATE_VENDOR_SPECIFIC Structure<>
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[PRIVATE_VENDOR_SPECIFIC_BYTES];
  } t;
  TPM2B b;
} TPM2B_PRIVATE_VENDOR_SPECIFIC;

// Table 187  Definition of TPMU_SENSITIVE_COMPOSITE Union <  IN/OUT, S>
typedef union {
#ifdef TPM_ALG_RSA
  TPM2B_PRIVATE_KEY_RSA          rsa;
#endif
#ifdef TPM_ALG_ECC
  TPM2B_ECC_PARAMETER            ecc;
#endif
#ifdef TPM_ALG_KEYEDHASH
  TPM2B_SENSITIVE_DATA           bits;
#endif
#ifdef TPM_ALG_SYMCIPHER
  TPM2B_SYM_KEY                  sym;
#endif
  TPM2B_PRIVATE_VENDOR_SPECIFIC  any;
} TPMU_SENSITIVE_COMPOSITE;

// Table 188  Definition of TPMT_SENSITIVE Structure
typedef struct {
  TPMI_ALG_PUBLIC           sensitiveType;
  TPM2B_AUTH                authValue;
  TPM2B_DIGEST              seedValue;
  TPMU_SENSITIVE_COMPOSITE  sensitive;
} TPMT_SENSITIVE;

// Table 189  Definition of TPM2B_SENSITIVE Structure <  IN/OUT>
typedef union {
  struct {
    UINT16          size;
    TPMT_SENSITIVE  sensitiveArea;
  } t;
  TPM2B b;
} TPM2B_SENSITIVE;

// Table 190  Definition of _PRIVATE Structure <>
typedef struct {
  TPM2B_DIGEST    integrityOuter;
  TPM2B_DIGEST    integrityInner;
  TPMT_SENSITIVE  sensitive;
} _PRIVATE;

// Table 191  Definition of TPM2B_PRIVATE Structure <  IN/OUT, S>
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[sizeof(_PRIVATE)];
  } t;
  TPM2B b;
} TPM2B_PRIVATE;

// Table 192  Definition of _ID_OBJECT Structure <>
typedef struct {
  TPM2B_DIGEST  integrityHMAC;
  TPM2B_DIGEST  encIdentity;
} _ID_OBJECT;

// Table 193  Definition of TPM2B_ID_OBJECT Structure <  IN/OUT>
typedef union {
  struct {
    UINT16  size;
    BYTE    credential[sizeof(_ID_OBJECT)];
  } t;
  TPM2B b;
} TPM2B_ID_OBJECT;

// Table 194  Definition of TPM_NV_INDEX Bits <>
typedef struct {
  UINT32 index : 24;
  UINT32 RH_NV : 8;
} TPM_NV_INDEX;

// Table 195  Definition of TPMA_NV Bits
typedef struct {
  UINT32 TPMA_NV_PPWRITE        : 1;
  UINT32 TPMA_NV_OWNERWRITE     : 1;
  UINT32 TPMA_NV_AUTHWRITE      : 1;
  UINT32 TPMA_NV_POLICYWRITE    : 1;
  UINT32 TPMA_NV_COUNTER        : 1;
  UINT32 TPMA_NV_BITS           : 1;
  UINT32 TPMA_NV_EXTEND         : 1;
  UINT32 reserved7_9            : 3;
  UINT32 TPMA_NV_POLICY_DELETE  : 1;
  UINT32 TPMA_NV_WRITELOCKED    : 1;
  UINT32 TPMA_NV_WRITEALL       : 1;
  UINT32 TPMA_NV_WRITEDEFINE    : 1;
  UINT32 TPMA_NV_WRITE_STCLEAR  : 1;
  UINT32 TPMA_NV_GLOBALLOCK     : 1;
  UINT32 TPMA_NV_PPREAD         : 1;
  UINT32 TPMA_NV_OWNERREAD      : 1;
  UINT32 TPMA_NV_AUTHREAD       : 1;
  UINT32 TPMA_NV_POLICYREAD     : 1;
  UINT32 reserved20_24          : 5;
  UINT32 TPMA_NV_NO_DA          : 1;
  UINT32 TPMA_NV_ORDERLY        : 1;
  UINT32 TPMA_NV_CLEAR_STCLEAR  : 1;
  UINT32 TPMA_NV_READLOCKED     : 1;
  UINT32 TPMA_NV_WRITTEN        : 1;
  UINT32 TPMA_NV_PLATFORMCREATE : 1;
  UINT32 TPMA_NV_READ_STCLEAR   : 1;
} TPMA_NV;

// Table 196  Definition of TPMS_NV_PUBLIC Structure
typedef struct {
  TPMI_RH_NV_INDEX  nvIndex;
  TPMI_ALG_HASH     nameAlg;
  TPMA_NV           attributes;
  TPM2B_DIGEST      authPolicy;
  UINT16            dataSize;
} TPMS_NV_PUBLIC;

// Table 197  Definition of TPM2B_NV_PUBLIC Structure
typedef union {
  struct {
    UINT16          size;
    TPMS_NV_PUBLIC  nvPublic;
  } t;
  TPM2B b;
} TPM2B_NV_PUBLIC;

// Table 198  Definition of TPM2B_CONTEXT_SENSITIVE Structure <  IN/OUT>
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[MAX_CONTEXT_SIZE];
  } t;
  TPM2B b;
} TPM2B_CONTEXT_SENSITIVE;

// Table 199  Definition of TPMS_CONTEXT_DATA Structure <  IN/OUT, S>
typedef struct {
  TPM2B_DIGEST             integrity;
  TPM2B_CONTEXT_SENSITIVE  encrypted;
} TPMS_CONTEXT_DATA;

// Table 200  Definition of TPM2B_CONTEXT_DATA Structure <  IN/OUT>
typedef union {
  struct {
    UINT16  size;
    BYTE    buffer[sizeof(TPMS_CONTEXT_DATA)];
  } t;
  TPM2B b;
} TPM2B_CONTEXT_DATA;

// Table 201  Definition of TPMS_CONTEXT Structure
typedef struct {
  UINT64              sequence;
  TPMI_DH_CONTEXT     savedHandle;
  TPMI_RH_HIERARCHY   hierarchy;
  TPM2B_CONTEXT_DATA  contextBlob;
} TPMS_CONTEXT;

// Unprocessed: Table 202  Context Handle Values
// Table 203  Definition of TPMS_CREATION_DATA Structure <  OUT>
typedef struct {
  TPML_PCR_SELECTION  pcrSelect;
  TPM2B_DIGEST        pcrDigest;
  TPMA_LOCALITY       locality;
  TPM_ALG_ID          parentNameAlg;
  TPM2B_NAME          parentName;
  TPM2B_NAME          parentQualifiedName;
  TPM2B_DATA          outsideInfo;
} TPMS_CREATION_DATA;

// Table 204  Definition of TPM2B_CREATION_DATA Structure <  OUT>
typedef union {
  struct {
    UINT16              size;
    TPMS_CREATION_DATA  creationData;
  } t;
  TPM2B b;
} TPM2B_CREATION_DATA;


#endif  // TPM2_TPM_TYPES_H_
