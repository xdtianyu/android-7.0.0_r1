// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "RSA_Encrypt_fp.h"
#ifdef TPM_ALG_RSA
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_ATTRIBUTES                 decrypt attribute is not SET in key referenced by keyHandle
//     TPM_RC_KEY                        keyHandle does not reference an RSA key
//     TPM_RC_SCHEME                     incorrect input scheme, or the chosen scheme is not a valid RSA
//                                       decrypt scheme
//     TPM_RC_VALUE                      the numeric value of message is greater than the public modulus of
//                                       the key referenced by keyHandle, or label is not a null-terminated
//                                       string
//
TPM_RC
TPM2_RSA_Encrypt(
   RSA_Encrypt_In        *in,                 // IN: input parameter list
   RSA_Encrypt_Out       *out                 // OUT: output parameter list
   )
{
   TPM_RC                    result;
   OBJECT                    *rsaKey;
   TPMT_RSA_DECRYPT          *scheme;
   char                      *label = NULL;

// Input Validation

   rsaKey = ObjectGet(in->keyHandle);

   // selected key must be an RSA key
   if(rsaKey->publicArea.type != TPM_ALG_RSA)
       return TPM_RC_KEY + RC_RSA_Encrypt_keyHandle;

   // selected key must have the decryption attribute
   if(rsaKey->publicArea.objectAttributes.decrypt != SET)
       return TPM_RC_ATTRIBUTES + RC_RSA_Encrypt_keyHandle;

   // Is there a label?
   if(in->label.t.size > 0)
   {
       // label is present, so make sure that is it NULL-terminated
       if(in->label.t.buffer[in->label.t.size - 1] != 0)
           return TPM_RC_VALUE + RC_RSA_Encrypt_label;
       label = (char *)in->label.t.buffer;
   }

// Command Output

   // Select a scheme for encryption
   scheme = CryptSelectRSAScheme(in->keyHandle, &in->inScheme);
   if(scheme == NULL)
       return TPM_RC_SCHEME + RC_RSA_Encrypt_inScheme;

   // Encryption. TPM_RC_VALUE, or TPM_RC_SCHEME errors my be returned buy
   // CryptEncyptRSA. Note: It can also return TPM_RC_ATTRIBUTES if the key does
   // not have the decrypt attribute but that was checked above.
   out->outData.t.size = sizeof(out->outData.t.buffer);
   result = CryptEncryptRSA(&out->outData.t.size, out->outData.t.buffer, rsaKey,
                          scheme, in->message.t.size, in->message.t.buffer,
                          label);
   return result;
}
#endif
