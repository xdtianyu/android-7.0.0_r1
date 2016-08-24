// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "GetCapability_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_HANDLE                     value of property is in an unsupported handle range for the
//                                       TPM_CAP_HANDLES capability value
//     TPM_RC_VALUE                      invalid capability; or property is not 0 for the TPM_CAP_PCRS
//                                       capability value
//
TPM_RC
TPM2_GetCapability(
   GetCapability_In      *in,                  // IN: input parameter list
   GetCapability_Out     *out                  // OUT: output parameter list
   )
{
// Command Output

   // Set output capability type the same as input type
   out->capabilityData.capability = in->capability;

   switch(in->capability)
   {
   case TPM_CAP_ALGS:
       out->moreData = AlgorithmCapGetImplemented((TPM_ALG_ID) in->property,
                       in->propertyCount, &out->capabilityData.data.algorithms);
       break;
   case TPM_CAP_HANDLES:
       switch(HandleGetType((TPM_HANDLE) in->property))
       {
       case TPM_HT_TRANSIENT:
           // Get list of handles of loaded transient objects
           out->moreData = ObjectCapGetLoaded((TPM_HANDLE) in->property,
                                              in->propertyCount,
                                              &out->capabilityData.data.handles);
           break;
       case TPM_HT_PERSISTENT:
           // Get list of handles of persistent objects
           out->moreData = NvCapGetPersistent((TPM_HANDLE) in->property,
                                              in->propertyCount,
                                              &out->capabilityData.data.handles);
           break;
       case TPM_HT_NV_INDEX:
           // Get list of defined NV index
           out->moreData = NvCapGetIndex((TPM_HANDLE) in->property,
                                         in->propertyCount,
                                         &out->capabilityData.data.handles);
           break;
       case TPM_HT_LOADED_SESSION:
           // Get list of handles of loaded sessions
           out->moreData = SessionCapGetLoaded((TPM_HANDLE) in->property,
                                               in->propertyCount,
                                               &out->capabilityData.data.handles);
           break;
       case TPM_HT_ACTIVE_SESSION:
           // Get list of handles of
           out->moreData = SessionCapGetSaved((TPM_HANDLE) in->property,
                                              in->propertyCount,
                                              &out->capabilityData.data.handles);
           break;
       case TPM_HT_PCR:
           // Get list of handles of PCR
           out->moreData = PCRCapGetHandles((TPM_HANDLE) in->property,
                                            in->propertyCount,
                                            &out->capabilityData.data.handles);
           break;
       case TPM_HT_PERMANENT:
           // Get list of permanent handles
           out->moreData = PermanentCapGetHandles(
                               (TPM_HANDLE) in->property,
                               in->propertyCount,
                               &out->capabilityData.data.handles);
           break;
       default:
           // Unsupported input handle type
           return TPM_RC_HANDLE + RC_GetCapability_property;
           break;
       }
       break;
   case TPM_CAP_COMMANDS:
       out->moreData = CommandCapGetCCList((TPM_CC) in->property,
                                           in->propertyCount,
                                           &out->capabilityData.data.command);
       break;
   case TPM_CAP_PP_COMMANDS:
       out->moreData = PhysicalPresenceCapGetCCList((TPM_CC) in->property,
                       in->propertyCount, &out->capabilityData.data.ppCommands);
       break;
   case TPM_CAP_AUDIT_COMMANDS:
       out->moreData = CommandAuditCapGetCCList((TPM_CC) in->property,
                                       in->propertyCount,
                                       &out->capabilityData.data.auditCommands);
       break;
   case TPM_CAP_PCRS:
       // Input property must be 0
       if(in->property != 0)
           return TPM_RC_VALUE + RC_GetCapability_property;
       out->moreData = PCRCapGetAllocation(in->propertyCount,
                                           &out->capabilityData.data.assignedPCR);
       break;
   case TPM_CAP_PCR_PROPERTIES:
       out->moreData = PCRCapGetProperties((TPM_PT_PCR) in->property,
                                         in->propertyCount,
                                         &out->capabilityData.data.pcrProperties);
       break;
   case TPM_CAP_TPM_PROPERTIES:
       out->moreData = TPMCapGetProperties((TPM_PT) in->property,
                                         in->propertyCount,
                                         &out->capabilityData.data.tpmProperties);
       break;
#ifdef TPM_ALG_ECC
   case TPM_CAP_ECC_CURVES:
       out->moreData = CryptCapGetECCCurve((TPM_ECC_CURVE   ) in->property,
                                           in->propertyCount,
                                           &out->capabilityData.data.eccCurves);
       break;
#endif // TPM_ALG_ECC
   case TPM_CAP_VENDOR_PROPERTY:
       // vendor property is not implemented
   default:
       // Unexpected TPM_CAP value
       return TPM_RC_VALUE;
       break;
   }

   return TPM_RC_SUCCESS;
}
