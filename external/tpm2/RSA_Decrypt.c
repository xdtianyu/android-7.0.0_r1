// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "RSA_Decrypt_fp.h"
#ifdef TPM_ALG_RSA
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_BINDING                    The public an private parts of the key are not properly bound
//     TPM_RC_KEY                        keyHandle does not reference an unrestricted decrypt key
//     TPM_RC_SCHEME                     incorrect input scheme, or the chosen scheme is not a valid RSA
//                                       decrypt scheme
//     TPM_RC_SIZE                       cipherText is not the size of the modulus of key referenced by
//                                       keyHandle
//     TPM_RC_VALUE                      label is not a null terminated string or the value of cipherText is
//                                       greater that the modulus of keyHandle
//
TPM_RC
TPM2_RSA_Decrypt(
   RSA_Decrypt_In        *in,                   // IN: input parameter list
   RSA_Decrypt_Out       *out                   // OUT: output parameter list
   )
{
   TPM_RC                             result;
   OBJECT                            *rsaKey;
   TPMT_RSA_DECRYPT                  *scheme;
   char                              *label = NULL;

// Input Validation

   rsaKey = ObjectGet(in->keyHandle);

   // The selected key must be an RSA key
   if(rsaKey->publicArea.type != TPM_ALG_RSA)
       return TPM_RC_KEY + RC_RSA_Decrypt_keyHandle;

   // The selected key must be an unrestricted decryption key
   if(   rsaKey->publicArea.objectAttributes.restricted == SET
      || rsaKey->publicArea.objectAttributes.decrypt == CLEAR)
       return TPM_RC_ATTRIBUTES + RC_RSA_Decrypt_keyHandle;

   //   NOTE: Proper operation of this command requires that the sensitive area
   //   of the key is loaded. This is assured because authorization is required
   //   to use the sensitive area of the key. In order to check the authorization,
   //   the sensitive area has to be loaded, even if authorization is with policy.

   // If label is present, make sure that it is a NULL-terminated string
   if(in->label.t.size > 0)
   {
       // Present, so make sure that it is NULL-terminated
       if(in->label.t.buffer[in->label.t.size - 1] != 0)
           return TPM_RC_VALUE + RC_RSA_Decrypt_label;
       label = (char *)in->label.t.buffer;
   }

// Command Output

   // Select a scheme for decrypt.
   scheme = CryptSelectRSAScheme(in->keyHandle, &in->inScheme);
  if(scheme == NULL)
      return TPM_RC_SCHEME + RC_RSA_Decrypt_inScheme;

  // Decryption. TPM_RC_VALUE, TPM_RC_SIZE, and TPM_RC_KEY error may be
  // returned by CryptDecryptRSA.
  // NOTE: CryptDecryptRSA can also return TPM_RC_ATTRIBUTES or TPM_RC_BINDING
  // when the key is not a decryption key but that was checked above.
  out->message.t.size = sizeof(out->message.t.buffer);
  result = CryptDecryptRSA(&out->message.t.size, out->message.t.buffer, rsaKey,
                           scheme, in->cipherText.t.size,
                           in->cipherText.t.buffer,
                           label);

   return result;
}
#endif
