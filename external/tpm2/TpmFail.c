// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#define        TPM_FAIL_C
#include       "InternalRoutines.h"
#include       <assert.h>
//
//      On MS C compiler, can save the alignment state and set the alignment to 1 for the duration of the
//      TPM_Types.h include. This will avoid a lot of alignment warnings from the compiler for the unaligned
//      structures. The alignment of the structures is not important as this function does not use any of the
//      structures in TPM_Types.h and only include it for the #defines of the capabilities, properties, and
//      command code values.
//
#pragma pack(push, 1)
#include "TPM_Types.h"
#pragma pack (pop)
#include "swap.h"
//
//
//          Typedefs
//
//      These defines are used primarily for sizing of the local response buffer.
//
#pragma pack(push,1)
typedef struct {
   TPM_ST           tag;
   UINT32           size;
   TPM_RC           code;
} HEADER;
typedef struct {
   UINT16       size;
   struct {
       UINT32       function;
       UINT32       line;
       UINT32       code;
   } values;
   TPM_RC       returnCode;
} GET_TEST_RESULT_PARAMETERS;
typedef struct {
   TPMI_YES_NO                   moreData;
   TPM_CAP                       capability; // Always TPM_CAP_TPM_PROPERTIES
   TPML_TAGGED_TPM_PROPERTY      tpmProperty; // a single tagged property
} GET_CAPABILITY_PARAMETERS;
typedef struct {
   HEADER header;
   GET_TEST_RESULT_PARAMETERS getTestResult;
} TEST_RESPONSE;
typedef struct {
   HEADER header;
   GET_CAPABILITY_PARAMETERS     getCap;
} CAPABILITY_RESPONSE;
typedef union {
   TEST_RESPONSE            test;
   CAPABILITY_RESPONSE      cap;
} RESPONSES;
#pragma pack(pop)
//
//     Buffer to hold the responses. This may be a little larger than required due to padding that a compiler
//     might add.
//
//     NOTE:           This is not in Global.c because of the specialized data definitions above. Since the data contained in this
//                     structure is not relevant outside of the execution of a single command (when the TPM is in failure mode. There
//                     is no compelling reason to move all the typedefs to Global.h and this structure to Global.c.
//
#ifndef __IGNORE_STATE__ // Don't define this value
static BYTE response[sizeof(RESPONSES)];
#endif
//
//
//          Local Functions
//
//         MarshalUint16()
//
//     Function to marshal a 16 bit value to the output buffer.
//
static INT32
MarshalUint16(
    UINT16               integer,
    BYTE                 **buffer,
    INT32                *size
    )
{
    return UINT16_Marshal(&integer, buffer, size);
}
//
//
//         MarshalUint32()
//
//     Function to marshal a 32 bit value to the output buffer.
static INT32
MarshalUint32(
    UINT32               integer,
    BYTE                **buffer,
    INT32               *size
    )
{
    return UINT32_Marshal(&integer, buffer, size);
}
//
//
//         UnmarshalHeader()
//
//     Funtion to unmarshal the 10-byte command header.
//
static BOOL
UnmarshalHeader(
    HEADER              *header,
    BYTE                **buffer,
    INT32               *size
    )
{
    UINT32 usize;
    TPM_RC ucode;
    if(     UINT16_Unmarshal(&header->tag, buffer, size) != TPM_RC_SUCCESS
        || UINT32_Unmarshal(&usize, buffer, size) != TPM_RC_SUCCESS
        || UINT32_Unmarshal(&ucode, buffer, size) != TPM_RC_SUCCESS
        )
        return FALSE;
    header->size = usize;
    header->code = ucode;
    return TRUE;
}
//
//
//          Public Functions
//
//         SetForceFailureMode()
//
//     This function is called by the simulator to enable failure mode testing.
//
LIB_EXPORT void
SetForceFailureMode(
    void
    )
{
    g_forceFailureMode = TRUE;
    return;
}

//
//
//         TpmFail()
//
//     This function is called by TPM.lib when a failure occurs. It will set up the failure values to be returned on
//     TPM2_GetTestResult().
//
void
TpmFail(
    const char                         *function,
    int line,                 int       code
    )
{
    // Save the values that indicate where the error occurred.
    // On a 64-bit machine, this may truncate the address of the string
    // of the function name where the error occurred.
    memcpy(&s_failFunction, function, sizeof(s_failFunction));
    s_failLine = line;
    s_failCode = code;
    // if asserts are enabled, then do an assert unless the failure mode code
    // is being tested
    assert(g_forceFailureMode);
    // Clear this flag
    g_forceFailureMode = FALSE;
    // Jump to the failure mode code.
    // Note: only get here if asserts are off or if we are testing failure mode
#ifndef EMBEDDED_MODE
    longjmp(&g_jumpBuffer[0], 1);
#endif
}
//
//
//          TpmFailureMode
//
//      This function is called by the interface code when the platform is in failure mode.
//
void
TpmFailureMode (
    unsigned   int       inRequestSize,          //   IN: command buffer size
    unsigned   char     *inRequest,              //   IN: command buffer
    unsigned   int      *outResponseSize,        //   OUT: response buffer size
    unsigned   char     **outResponse            //   OUT: response buffer
    )
{
    BYTE                *buffer;
    INT32                bufferSize;
    UINT32               marshalSize;
    UINT32               capability;
    HEADER               header;     // unmarshaled command header
    UINT32               pt;     // unmarshaled property type
    UINT32               count; // unmarshaled property count
    // If there is no command buffer, then just return TPM_RC_FAILURE
    if(inRequestSize == 0 || inRequest == NULL)
        goto FailureModeReturn;
    // If the header is not correct for TPM2_GetCapability() or
    // TPM2_GetTestResult() then just return the in failure mode response;
    buffer = inRequest;
    if(!UnmarshalHeader(&header, &inRequest, (INT32 *)&inRequestSize))
        goto FailureModeReturn;
    if(   header.tag != TPM_ST_NO_SESSIONS
       || header.size < 10)
       goto FailureModeReturn;
    switch (header.code) {
    case TPM_CC_GetTestResult:
         // make sure that the command size is correct
         if(header.size != 10)
              goto FailureModeReturn;
         buffer = &response[10];
         bufferSize = MAX_RESPONSE_SIZE-10;
         marshalSize = MarshalUint16(3 * sizeof(UINT32), &buffer, &bufferSize);
         marshalSize += MarshalUint32(s_failFunction, &buffer, &bufferSize);
         marshalSize += MarshalUint32(s_failLine, &buffer, &bufferSize);
         marshalSize += MarshalUint32(s_failCode, &buffer, &bufferSize);
         if(s_failCode == FATAL_ERROR_NV_UNRECOVERABLE)
              marshalSize += MarshalUint32(TPM_RC_NV_UNINITIALIZED, &buffer, &bufferSize);
         else
              marshalSize += MarshalUint32(TPM_RC_FAILURE, &buffer, &bufferSize);
//
        break;
   case TPM_CC_GetCapability:
       // make sure that the size of the command is exactly the size
       // returned for the capability, property, and count
       if(     header.size!= (10 + (3 * sizeof(UINT32)))
               // also verify that this is requesting TPM properties
           ||      (UINT32_Unmarshal(&capability, &inRequest,
                                     (INT32 *)&inRequestSize)
               != TPM_RC_SUCCESS)
           || (capability != TPM_CAP_TPM_PROPERTIES)
           ||      (UINT32_Unmarshal(&pt, &inRequest, (INT32 *)&inRequestSize)
               != TPM_RC_SUCCESS)
           ||      (UINT32_Unmarshal(&count, &inRequest, (INT32 *)&inRequestSize)
               != TPM_RC_SUCCESS)
           )
              goto FailureModeReturn;
        // If in failure mode because of an unrecoverable read error, and the
        // property is 0 and the count is 0, then this is an indication to
        // re-manufacture the TPM. Do the re-manufacture but stay in failure
        // mode until the TPM is reset.
        // Note: this behavior is not required by the specification and it is
        // OK to leave the TPM permanently bricked due to an unrecoverable NV
        // error.
        if( count == 0 && pt == 0 && s_failCode == FATAL_ERROR_NV_UNRECOVERABLE)
        {
            g_manufactured = FALSE;
            TPM_Manufacture(0);
        }
        if(count > 0)
            count = 1;
        else if(pt > TPM_PT_FIRMWARE_VERSION_2)
            count = 0;
        if(pt < TPM_PT_MANUFACTURER)
            pt = TPM_PT_MANUFACTURER;
        // set up for return
        buffer = &response[10];
        bufferSize = MAX_RESPONSE_SIZE-10;
        // if the request was for a PT less than the last one
        // then we indicate more, otherwise, not.
        if(pt < TPM_PT_FIRMWARE_VERSION_2)
             *buffer++ = YES;
        else
             *buffer++ = NO;
        marshalSize = 1;
        // indicate     the capability type
        marshalSize     += MarshalUint32(capability, &buffer, &bufferSize);
        // indicate     the number of values that are being returned (0 or 1)
        marshalSize     += MarshalUint32(count, &buffer, &bufferSize);
        // indicate     the property
        marshalSize     += MarshalUint32(pt, &buffer, &bufferSize);
        if(count > 0)
            switch (pt) {
            case TPM_PT_MANUFACTURER:
            // the vendor ID unique to each TPM manufacturer
#ifdef   MANUFACTURER
            pt = *(UINT32*)MANUFACTURER;
#else
              pt = 0;
#endif
            break;
        case TPM_PT_VENDOR_STRING_1:
            // the first four characters of the vendor ID string
#ifdef   VENDOR_STRING_1
            pt = *(UINT32*)VENDOR_STRING_1;
#else
             pt = 0;
#endif
            break;
        case TPM_PT_VENDOR_STRING_2:
            // the second four characters of the vendor ID string
#ifdef   VENDOR_STRING_2
            pt = *(UINT32*)VENDOR_STRING_2;
#else
             pt = 0;
#endif
            break;
        case TPM_PT_VENDOR_STRING_3:
            // the third four characters of the vendor ID string
#ifdef   VENDOR_STRING_3
            pt = *(UINT32*)VENDOR_STRING_3;
#else
             pt = 0;
#endif
            break;
        case TPM_PT_VENDOR_STRING_4:
            // the fourth four characters of the vendor ID string
#ifdef   VENDOR_STRING_4
            pt = *(UINT32*)VENDOR_STRING_4;
#else
             pt = 0;
#endif
            break;
        case TPM_PT_VENDOR_TPM_TYPE:
            // vendor-defined value indicating the TPM model
            // We just make up a number here
            pt = 1;
            break;
        case TPM_PT_FIRMWARE_VERSION_1:
            // the more significant 32-bits of a vendor-specific value
            // indicating the version of the firmware
#ifdef   FIRMWARE_V1
            pt = FIRMWARE_V1;
#else
             pt = 0;
#endif
            break;
        default: // TPM_PT_FIRMWARE_VERSION_2:
            // the less significant 32-bits of a vendor-specific value
            // indicating the version of the firmware
#ifdef   FIRMWARE_V2
            pt = FIRMWARE_V2;
#else
             pt = 0;
#endif
           break;
       }
       marshalSize += MarshalUint32(pt, &buffer, &bufferSize);
       break;
   default: // default for switch (cc)
       goto FailureModeReturn;
   }
   // Now do the header
   buffer = response;
   bufferSize = 10;
   marshalSize = marshalSize + 10; // Add the header size to the
                                   // stuff already marshaled
   MarshalUint16(TPM_ST_NO_SESSIONS, &buffer, &bufferSize); // structure tag
   MarshalUint32(marshalSize, &buffer, &bufferSize); // responseSize
   MarshalUint32(TPM_RC_SUCCESS, &buffer, &bufferSize); // response code
   *outResponseSize = marshalSize;
   *outResponse = (unsigned char *)&response;
   return;
FailureModeReturn:
   buffer = response;
   bufferSize = 10;
   marshalSize = MarshalUint16(TPM_ST_NO_SESSIONS, &buffer, &bufferSize);
   marshalSize += MarshalUint32(10, &buffer, &bufferSize);
   marshalSize += MarshalUint32(TPM_RC_FAILURE, &buffer, &bufferSize);
   *outResponseSize = marshalSize;
   *outResponse = (unsigned char *)response;
   return;
}
