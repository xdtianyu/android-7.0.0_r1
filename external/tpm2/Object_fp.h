/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_OBJECT_FP_H
#define __TPM2_OBJECT_FP_H

BOOL AreAttributesForParent(OBJECT *parentObject  // IN: parent handle
                            );
TPMI_YES_NO ObjectCapGetLoaded(TPMI_DH_OBJECT handle,  // IN: start handle
                               UINT32 count,  // IN: count of returned handles
                               TPML_HANDLE *handleList  // OUT: list of handle
                               );
UINT32 ObjectCapGetTransientAvail(void);
void ObjectCleanupEvict(void);
void ObjectComputeName(TPMT_PUBLIC *publicArea,  // IN: public area of an object
                       TPM2B_NAME *name          // OUT: name of the object
                       );
void ObjectComputeQualifiedName(
    TPM2B_NAME *parentQN,      //   IN: parent's qualified name
    TPM_ALG_ID nameAlg,        //   IN: name hash
    TPM2B_NAME *name,          //   IN: name of the object
    TPM2B_NAME *qualifiedName  //   OUT: qualified name of the object
    );
TPM_RC ObjectContextLoad(
    OBJECT *object,         // IN: object structure from saved context
    TPMI_DH_OBJECT *handle  // OUT: object handle
    );
TPM_RC ObjectCreateEventSequence(
    TPM2B_AUTH *auth,          // IN: authValue
    TPMI_DH_OBJECT *newHandle  // OUT: sequence object handle
    );
TPM_RC ObjectCreateHMACSequence(
    TPMI_ALG_HASH hashAlg,     // IN: hash algorithm
    TPM_HANDLE handle,         // IN: the handle associated with sequence object
    TPM2B_AUTH *auth,          // IN: authValue
    TPMI_DH_OBJECT *newHandle  // OUT: HMAC sequence object handle
    );
TPM_RC ObjectCreateHashSequence(
    TPMI_ALG_HASH hashAlg,     // IN: hash algorithm
    TPM2B_AUTH *auth,          // IN: authValue
    TPMI_DH_OBJECT *newHandle  // OUT: sequence object handle
    );
TPMI_RH_HIERARCHY ObjectDataGetHierarchy(OBJECT *object  // IN :object
                                         );
BOOL ObjectDataIsStorage(
    TPMT_PUBLIC *publicArea  // IN: public area of the object
    );
OBJECT *ObjectGet(TPMI_DH_OBJECT handle  // IN: handle of the object
                  );
TPMI_RH_HIERARCHY ObjectGetHierarchy(TPMI_DH_OBJECT handle  // IN :object handle
                                     );
TPMI_ALG_HASH ObjectGetNameAlg(
    TPMI_DH_OBJECT handle  // IN: handle of the object
    );
TPM_RC ObjectLoadEvict(TPM_HANDLE *handle,  // IN:OUT: evict object handle. If
                                            // success, it will be replace by
                                            // the loaded object handle
                       TPM_CC commandCode   // IN: the command being processed
                       );
void ObjectFlush(TPMI_DH_OBJECT handle  // IN: handle to be freed
                 );
void ObjectFlushHierarchy(
    TPMI_RH_HIERARCHY hierarchy  // IN: hierarchy to be flush
    );
OBJECT *ObjectGet(TPMI_DH_OBJECT handle  // IN: handle of the object
                  );
UINT16 ObjectGetName(TPMI_DH_OBJECT handle,  // IN: handle of the object
                     NAME *name              // OUT: name of the object
                     );
void ObjectGetQualifiedName(
    TPMI_DH_OBJECT handle,     // IN: handle of the object
    TPM2B_NAME *qualifiedName  // OUT: qualified name of the object
    );
BOOL ObjectIsPresent(TPMI_DH_OBJECT handle  // IN: handle to be checked
                     );
BOOL ObjectIsSequence(OBJECT *object  // IN: handle to be checked
                      );
BOOL ObjectIsStorage(TPMI_DH_OBJECT handle  // IN: object handle
                     );
TPM_RC ObjectLoad(
    TPMI_RH_HIERARCHY hierarchy,  //   IN: hierarchy to which the object belongs
    TPMT_PUBLIC *publicArea,      //   IN: public area
    TPMT_SENSITIVE *sensitive,    //   IN: sensitive area (may be null)
    TPM2B_NAME *name,             //   IN: object's name (may be null)
    TPM_HANDLE parentHandle,      //   IN: handle of parent
    BOOL skipChecks,  //   IN: flag to indicate if it is OK to skip consistency
                      //   checks.
    TPMI_DH_OBJECT *handle  //   OUT: object handle
    );
void ObjectStartup(void);
void ObjectTerminateEvent(void);

#endif  // __TPM2_OBJECT_FP_H
