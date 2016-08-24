// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
//
//
//          Functions
//
//         PCRGetProperty()
//
//     This function accepts a property selection and, if so, sets value to the value of the property.
//     All the fixed values are vendor dependent or determined by a platform-specific specification. The values
//     in the table below are examples and should be changed by the vendor.
//
//     Return Value                      Meaning
//
//     TRUE                              referenced property exists and value set
//     FALSE                             referenced property does not exist
//
static BOOL
TPMPropertyIsDefined(
    TPM_PT               property,           // IN: property
    UINT32              *value               // OUT: property value
    )
{
   switch(property)
   {
       case TPM_PT_FAMILY_INDICATOR:
           // from the title page of the specification
           // For this specification, the value is "2.0".
           *value = TPM_SPEC_FAMILY;
           break;
       case TPM_PT_LEVEL:
           // from the title page of the specification
           *value = TPM_SPEC_LEVEL;
           break;
       case TPM_PT_REVISION:
           // from the title page of the specification
           *value = TPM_SPEC_VERSION;
           break;
       case TPM_PT_DAY_OF_YEAR:
           // computed from the date value on the title page of the specification
           *value = TPM_SPEC_DAY_OF_YEAR;
           break;
       case TPM_PT_YEAR:
           // from the title page of the specification
           *value = TPM_SPEC_YEAR;
           break;
       case TPM_PT_MANUFACTURER:
           // vendor ID unique to each TPM manufacturer
           *value = BYTE_ARRAY_TO_UINT32(MANUFACTURER);
           break;
       case TPM_PT_VENDOR_STRING_1:
           // first four characters of the vendor ID string
           *value = BYTE_ARRAY_TO_UINT32(VENDOR_STRING_1);
           break;
       case TPM_PT_VENDOR_STRING_2:
           // second four characters of the vendor ID string
#ifdef VENDOR_STRING_2
           *value = BYTE_ARRAY_TO_UINT32(VENDOR_STRING_2);
#else
           *value = 0;
#endif
           break;
       case TPM_PT_VENDOR_STRING_3:
           // third four characters of the vendor ID string
#ifdef VENDOR_STRING_3
           *value = BYTE_ARRAY_TO_UINT32(VENDOR_STRING_3);
#else
           *value = 0;
#endif
           break;
       case TPM_PT_VENDOR_STRING_4:
           // fourth four characters of the vendor ID string
#ifdef VENDOR_STRING_4
           *value = BYTE_ARRAY_TO_UINT32(VENDOR_STRING_4);
#else
           *value = 0;
#endif
           break;
       case TPM_PT_VENDOR_TPM_TYPE:
           // vendor-defined value indicating the TPM model
           *value = 1;
           break;
       case TPM_PT_FIRMWARE_VERSION_1:
           // more significant 32-bits of a vendor-specific value
           *value = gp.firmwareV1;
           break;
       case TPM_PT_FIRMWARE_VERSION_2:
           // less significant 32-bits of a vendor-specific value
           *value = gp.firmwareV2;
           break;
       case TPM_PT_INPUT_BUFFER:
           // maximum size of TPM2B_MAX_BUFFER
           *value = MAX_DIGEST_BUFFER;
           break;
       case TPM_PT_HR_TRANSIENT_MIN:
           // minimum number of transient objects that can be held in TPM
           // RAM
           *value = MAX_LOADED_OBJECTS;
           break;
       case TPM_PT_HR_PERSISTENT_MIN:
           // minimum number of persistent objects that can be held in
           // TPM NV memory
           // In this implementation, there is no minimum number of
           // persistent objects.
           *value = MIN_EVICT_OBJECTS;
           break;
       case TPM_PT_HR_LOADED_MIN:
           // minimum number of authorization sessions that can be held in
           // TPM RAM
           *value = MAX_LOADED_SESSIONS;
           break;
       case TPM_PT_ACTIVE_SESSIONS_MAX:
           // number of authorization sessions that may be active at a time
           *value = MAX_ACTIVE_SESSIONS;
           break;
       case TPM_PT_PCR_COUNT:
           // number of PCR implemented
           *value = IMPLEMENTATION_PCR;
           break;
       case TPM_PT_PCR_SELECT_MIN:
           // minimum number of bytes in a TPMS_PCR_SELECT.sizeOfSelect
           *value = PCR_SELECT_MIN;
           break;
       case TPM_PT_CONTEXT_GAP_MAX:
           // maximum allowed difference (unsigned) between the contextID
           // values of two saved session contexts
           *value = (1 << (sizeof(CONTEXT_SLOT) * 8)) - 1;
            break;
        case TPM_PT_NV_COUNTERS_MAX:
            // maximum number of NV indexes that are allowed to have the
            // TPMA_NV_COUNTER attribute SET
            // In this implementation, there is no limitation on the number
            // of counters, except for the size of the NV Index memory.
            *value = 0;
            break;
        case TPM_PT_NV_INDEX_MAX:
            // maximum size of an NV index data area
            *value = MAX_NV_INDEX_SIZE;
            break;
        case TPM_PT_MEMORY:
            // a TPMA_MEMORY indicating the memory management method for the TPM
        {
            TPMA_MEMORY         attributes = {0};
            attributes.sharedNV = SET;
            attributes.objectCopiedToRam = SET;
             // Note: Different compilers may require a different method to cast
             // a bit field structure to a UINT32.
             memcpy(value, &attributes, sizeof(UINT32));
             break;
        }
        case TPM_PT_CLOCK_UPDATE:
            // interval, in seconds, between updates to the copy of
            // TPMS_TIME_INFO .clock in NV
            *value = (1 << NV_CLOCK_UPDATE_INTERVAL);
            break;
        case TPM_PT_CONTEXT_HASH:
            // algorithm used for the integrity hash on saved contexts and
            // for digesting the fuData of TPM2_FirmwareRead()
            *value = CONTEXT_INTEGRITY_HASH_ALG;
            break;
        case TPM_PT_CONTEXT_SYM:
            // algorithm used for encryption of saved contexts
            *value = CONTEXT_ENCRYPT_ALG;
            break;
        case TPM_PT_CONTEXT_SYM_SIZE:
            // size of the key used for encryption of saved contexts
            *value = CONTEXT_ENCRYPT_KEY_BITS;
            break;
        case TPM_PT_ORDERLY_COUNT:
            // maximum difference between the volatile and non-volatile
            // versions of TPMA_NV_COUNTER that have TPMA_NV_ORDERLY SET
            *value = MAX_ORDERLY_COUNT;
            break;
        case TPM_PT_MAX_COMMAND_SIZE:
            // maximum value for 'commandSize'
            *value = MAX_COMMAND_SIZE;
            break;
        case TPM_PT_MAX_RESPONSE_SIZE:
            // maximum value for 'responseSize'
            *value = MAX_RESPONSE_SIZE;
            break;
        case TPM_PT_MAX_DIGEST:
            // maximum size of a digest that can be produced by the TPM
            *value = sizeof(TPMU_HA);
            break;
        case TPM_PT_MAX_OBJECT_CONTEXT:
            // maximum size of a TPMS_CONTEXT that will be returned by
            // TPM2_ContextSave for object context
            *value = 0;
             // adding sequence, saved handle and hierarchy
             *value += sizeof(UINT64) + sizeof(TPMI_DH_CONTEXT) +
                        sizeof(TPMI_RH_HIERARCHY);
              // add size field in TPM2B_CONTEXT
              *value += sizeof(UINT16);
              // add integrity hash size
              *value += sizeof(UINT16) +
                        CryptGetHashDigestSize(CONTEXT_INTEGRITY_HASH_ALG);
              // Add fingerprint size, which is the same as sequence size
              *value += sizeof(UINT64);
            // Add OBJECT structure size
            *value += sizeof(OBJECT);
            break;
        case TPM_PT_MAX_SESSION_CONTEXT:
            // the maximum size of a TPMS_CONTEXT that will be returned by
            // TPM2_ContextSave for object context
            *value = 0;
              // adding sequence, saved handle and hierarchy
              *value += sizeof(UINT64) + sizeof(TPMI_DH_CONTEXT) +
                        sizeof(TPMI_RH_HIERARCHY);
              // Add size field in TPM2B_CONTEXT
              *value += sizeof(UINT16);
              // Add integrity hash size
              *value += sizeof(UINT16) +
                        CryptGetHashDigestSize(CONTEXT_INTEGRITY_HASH_ALG);
              // Add fingerprint size, which is the same as sequence size
              *value += sizeof(UINT64);
           // Add SESSION structure size
           *value += sizeof(SESSION);
           break;
       case TPM_PT_PS_FAMILY_INDICATOR:
           // platform specific values for the TPM_PT_PS parameters from
           // the relevant platform-specific specification
           // In this reference implementation, all of these values are 0.
           *value = 0;
           break;
       case TPM_PT_PS_LEVEL:
           // level of the platform-specific specification
           *value = 0;
           break;
       case TPM_PT_PS_REVISION:
           // specification Revision times 100 for the platform-specific
           // specification
           *value = 0;
           break;
       case TPM_PT_PS_DAY_OF_YEAR:
           // platform-specific specification day of year using TCG calendar
           *value = 0;
           break;
       case TPM_PT_PS_YEAR:
           // platform-specific specification year using the CE
           *value = 0;
           break;
       case TPM_PT_SPLIT_MAX:
           // number of split signing operations supported by the TPM
           *value = 0;
   #ifdef TPM_ALG_ECC
           *value = sizeof(gr.commitArray) * 8;
   #endif
           break;
       case TPM_PT_TOTAL_COMMANDS:
           // total number of commands implemented in the TPM
             // Since the reference implementation does not have any
             // vendor-defined commands, this will be the same as the
             // number of library commands.
        {
             UINT32 i;
             *value = 0;
             // calculate implemented command numbers
             for(i = TPM_CC_FIRST; i <= TPM_CC_LAST; i++)
             {
                 if(CommandIsImplemented(i)) (*value)++;
             }
             break;
        }
        case TPM_PT_LIBRARY_COMMANDS:
            // number of commands from the TPM library that are implemented
        {
            UINT32 i;
            *value = 0;
             // calculate implemented command numbers
             for(i = TPM_CC_FIRST; i <= TPM_CC_LAST; i++)
             {
                 if(CommandIsImplemented(i)) (*value)++;
             }
             break;
        }
        case TPM_PT_VENDOR_COMMANDS:
            // number of vendor commands that are implemented
            *value = 0;
            break;
        case TPM_PT_PERMANENT:
            // TPMA_PERMANENT
        {
            TPMA_PERMANENT           flags = {0};
            if(gp.ownerAuth.t.size != 0)
                flags.ownerAuthSet = SET;
            if(gp.endorsementAuth.t.size != 0)
                flags.endorsementAuthSet = SET;
            if(gp.lockoutAuth.t.size != 0)
                flags.lockoutAuthSet = SET;
            if(gp.disableClear)
                flags.disableClear = SET;
            if(gp.failedTries >= gp.maxTries)
                flags.inLockout = SET;
            // In this implementation, EPS is always generated by TPM
            flags.tpmGeneratedEPS = SET;
             // Note: Different compilers may require a different method to cast
             // a bit field structure to a UINT32.
             memcpy(value, &flags, sizeof(UINT32));
             break;
        }
        case TPM_PT_STARTUP_CLEAR:
            // TPMA_STARTUP_CLEAR
        {
            TPMA_STARTUP_CLEAR      flags = {0};
            if(g_phEnable)
                flags.phEnable = SET;
            if(gc.shEnable)
                flags.shEnable = SET;
            if(gc.ehEnable)
                flags.ehEnable = SET;
            if(gc.phEnableNV)
                flags.phEnableNV = SET;
            if(g_prevOrderlyState != SHUTDOWN_NONE)
                  flags.orderly = SET;
              // Note: Different compilers may require a different method to cast
              // a bit field structure to a UINT32.
              memcpy(value, &flags, sizeof(UINT32));
              break;
        }
        case TPM_PT_HR_NV_INDEX:
            // number of NV indexes currently defined
            *value = NvCapGetIndexNumber();
            break;
        case TPM_PT_HR_LOADED:
            // number of authorization sessions currently loaded into TPM
            // RAM
            *value = SessionCapGetLoadedNumber();
            break;
        case TPM_PT_HR_LOADED_AVAIL:
            // number of additional authorization sessions, of any type,
            // that could be loaded into TPM RAM
            *value = SessionCapGetLoadedAvail();
            break;
        case TPM_PT_HR_ACTIVE:
            // number of active authorization sessions currently being
            // tracked by the TPM
            *value = SessionCapGetActiveNumber();
            break;
        case TPM_PT_HR_ACTIVE_AVAIL:
            // number of additional authorization sessions, of any type,
            // that could be created
            *value = SessionCapGetActiveAvail();
            break;
        case TPM_PT_HR_TRANSIENT_AVAIL:
            // estimate of the number of additional transient objects that
            // could be loaded into TPM RAM
            *value = ObjectCapGetTransientAvail();
            break;
        case TPM_PT_HR_PERSISTENT:
            // number of persistent objects currently loaded into TPM
            // NV memory
            *value = NvCapGetPersistentNumber();
            break;
        case TPM_PT_HR_PERSISTENT_AVAIL:
            // number of additional persistent objects that could be loaded
            // into NV memory
            *value = NvCapGetPersistentAvail();
            break;
        case TPM_PT_NV_COUNTERS:
            // number of defined NV indexes that have NV TPMA_NV_COUNTER
            // attribute SET
            *value = NvCapGetCounterNumber();
            break;
        case TPM_PT_NV_COUNTERS_AVAIL:
            // number of additional NV indexes that can be defined with their
            // TPMA_NV_COUNTER attribute SET
            *value = NvCapGetCounterAvail();
            break;
        case TPM_PT_ALGORITHM_SET:
            // region code for the TPM
            *value = gp.algorithmSet;
            break;
       case TPM_PT_LOADED_CURVES:
   #ifdef TPM_ALG_ECC
           // number of loaded ECC curves
           *value = CryptCapGetEccCurveNumber();
   #else // TPM_ALG_ECC
             *value = 0;
     #endif // TPM_ALG_ECC
             break;
          case TPM_PT_LOCKOUT_COUNTER:
              // current value of the lockout counter
              *value = gp.failedTries;
              break;
          case TPM_PT_MAX_AUTH_FAIL:
              // number of authorization failures before DA lockout is invoked
              *value = gp.maxTries;
              break;
          case TPM_PT_LOCKOUT_INTERVAL:
              // number of seconds before the value reported by
              // TPM_PT_LOCKOUT_COUNTER is decremented
              *value = gp.recoveryTime;
              break;
          case TPM_PT_LOCKOUT_RECOVERY:
              // number of seconds after a lockoutAuth failure before use of
              // lockoutAuth may be attempted again
              *value = gp.lockoutRecovery;
              break;
          case TPM_PT_AUDIT_COUNTER_0:
              // high-order 32 bits of the command audit counter
              *value = (UINT32) (gp.auditCounter >> 32);
              break;
          case TPM_PT_AUDIT_COUNTER_1:
              // low-order 32 bits of the command audit counter
              *value = (UINT32) (gp.auditCounter);
              break;
          default:
              // property is not defined
              return FALSE;
              break;
     }
     return TRUE;
}
//
//
//           TPMCapGetProperties()
//
//      This function is used to get the TPM_PT values. The search of properties will start at property and
//      continue until propertyList has as many values as will fit, or the last property has been reported, or the list
//      has as many values as requested in count.
//
//      Return Value                      Meaning
//
//      YES                               more properties are available
//      NO                                no more properties to be reported
//
TPMI_YES_NO
TPMCapGetProperties(
     TPM_PT                              property,           // IN: the starting TPM property
     UINT32                              count,              // IN: maximum number of returned
                                                             //     propertie
     TPML_TAGGED_TPM_PROPERTY           *propertyList        // OUT: property list
     )
{
     TPMI_YES_NO        more = NO;
     UINT32             i;
     // initialize output property list
     propertyList->count = 0;
      // maximum count of properties we may return is MAX_PCR_PROPERTIES
      if(count > MAX_TPM_PROPERTIES) count = MAX_TPM_PROPERTIES;
      // If property is less than PT_FIXED, start from PT_FIXED.
      if(property < PT_FIXED) property = PT_FIXED;
      // Scan through the TPM properties of the requested group.
      // The size of TPM property group is PT_GROUP * 2 for fix and
      // variable groups.
      for(i = property; i <= PT_FIXED + PT_GROUP * 2; i++)
      {
          UINT32          value;
          if(TPMPropertyIsDefined((TPM_PT) i, &value))
          {
              if(propertyList->count < count)
              {
                    // If the list is not full, add this property
                    propertyList->tpmProperty[propertyList->count].property =
                        (TPM_PT) i;
                    propertyList->tpmProperty[propertyList->count].value = value;
                    propertyList->count++;
              }
              else
              {
                  // If the return list is full but there are more properties
                  // available, set the indication and exit the loop.
                  more = YES;
                  break;
              }
          }
      }
      return more;
}
