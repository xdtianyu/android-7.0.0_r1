//
// Copyright (C) 2014 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "trunks/error_codes.h"

#include <sstream>
#include <string>

#include <base/logging.h>

namespace {

// Masks out the P and N bits (see TPM 2.0 Part 2 Table 14).
const trunks::TPM_RC kFormatOneErrorMask = 0x0BF;
// Selects just the N bits that identify the subject index.
const trunks::TPM_RC kFormatOneSubjectMask = 0x700;
const trunks::TPM_RC kLayerMask = 0xFFFFF000;

// Returns a known error code or the empty string if unknown.
std::string GetErrorStringInternal(trunks::TPM_RC error) {
  switch (error) {
    case trunks::TPM_RC_SUCCESS: return "TPM_RC_SUCCESS";
    case trunks::TPM_RC_BAD_TAG: return "TPM_RC_BAD_TAG";
    case trunks::TPM_RC_INITIALIZE: return "TPM_RC_INITIALIZE";
    case trunks::TPM_RC_FAILURE: return "TPM_RC_FAILURE";
    case trunks::TPM_RC_SEQUENCE: return "TPM_RC_SEQUENCE";
    case trunks::TPM_RC_PRIVATE: return "TPM_RC_PRIVATE";
    case trunks::TPM_RC_HMAC: return "TPM_RC_HMAC";
    case trunks::TPM_RC_DISABLED: return "TPM_RC_DISABLED";
    case trunks::TPM_RC_EXCLUSIVE: return "TPM_RC_EXCLUSIVE";
    case trunks::TPM_RC_AUTH_TYPE: return "TPM_RC_AUTH_TYPE";
    case trunks::TPM_RC_AUTH_MISSING: return "TPM_RC_AUTH_MISSING";
    case trunks::TPM_RC_POLICY: return "TPM_RC_POLICY";
    case trunks::TPM_RC_PCR: return "TPM_RC_PCR";
    case trunks::TPM_RC_PCR_CHANGED: return "TPM_RC_PCR_CHANGED";
    case trunks::TPM_RC_UPGRADE: return "TPM_RC_UPGRADE";
    case trunks::TPM_RC_TOO_MANY_CONTEXTS: return "TPM_RC_TOO_MANY_CONTEXTS";
    case trunks::TPM_RC_AUTH_UNAVAILABLE: return "TPM_RC_AUTH_UNAVAILABLE";
    case trunks::TPM_RC_REBOOT: return "TPM_RC_REBOOT";
    case trunks::TPM_RC_UNBALANCED: return "TPM_RC_UNBALANCED";
    case trunks::TPM_RC_COMMAND_SIZE: return "TPM_RC_COMMAND_SIZE";
    case trunks::TPM_RC_COMMAND_CODE: return "TPM_RC_COMMAND_CODE";
    case trunks::TPM_RC_AUTHSIZE: return "TPM_RC_AUTHSIZE";
    case trunks::TPM_RC_AUTH_CONTEXT: return "TPM_RC_AUTH_CONTEXT";
    case trunks::TPM_RC_NV_RANGE: return "TPM_RC_NV_RANGE";
    case trunks::TPM_RC_NV_SIZE: return "TPM_RC_NV_SIZE";
    case trunks::TPM_RC_NV_LOCKED: return "TPM_RC_NV_LOCKED";
    case trunks::TPM_RC_NV_AUTHORIZATION: return "TPM_RC_NV_AUTHORIZATION";
    case trunks::TPM_RC_NV_UNINITIALIZED: return "TPM_RC_NV_UNINITIALIZED";
    case trunks::TPM_RC_NV_SPACE: return "TPM_RC_NV_SPACE";
    case trunks::TPM_RC_NV_DEFINED: return "TPM_RC_NV_DEFINED";
    case trunks::TPM_RC_BAD_CONTEXT: return "TPM_RC_BAD_CONTEXT";
    case trunks::TPM_RC_CPHASH: return "TPM_RC_CPHASH";
    case trunks::TPM_RC_PARENT: return "TPM_RC_PARENT";
    case trunks::TPM_RC_NEEDS_TEST: return "TPM_RC_NEEDS_TEST";
    case trunks::TPM_RC_NO_RESULT: return "TPM_RC_NO_RESULT";
    case trunks::TPM_RC_SENSITIVE: return "TPM_RC_SENSITIVE";
    case trunks::TPM_RC_ASYMMETRIC: return "TPM_RC_ASYMMETRIC";
    case trunks::TPM_RC_ATTRIBUTES: return "TPM_RC_ATTRIBUTES";
    case trunks::TPM_RC_HASH: return "TPM_RC_HASH";
    case trunks::TPM_RC_VALUE: return "TPM_RC_VALUE";
    case trunks::TPM_RC_HIERARCHY: return "TPM_RC_HIERARCHY";
    case trunks::TPM_RC_KEY_SIZE: return "TPM_RC_KEY_SIZE";
    case trunks::TPM_RC_MGF: return "TPM_RC_MGF";
    case trunks::TPM_RC_MODE: return "TPM_RC_MODE";
    case trunks::TPM_RC_TYPE: return "TPM_RC_TYPE";
    case trunks::TPM_RC_HANDLE: return "TPM_RC_HANDLE";
    case trunks::TPM_RC_KDF: return "TPM_RC_KDF";
    case trunks::TPM_RC_RANGE: return "TPM_RC_RANGE";
    case trunks::TPM_RC_AUTH_FAIL: return "TPM_RC_AUTH_FAIL";
    case trunks::TPM_RC_NONCE: return "TPM_RC_NONCE";
    case trunks::TPM_RC_PP: return "TPM_RC_PP";
    case trunks::TPM_RC_SCHEME: return "TPM_RC_SCHEME";
    case trunks::TPM_RC_SIZE: return "TPM_RC_SIZE";
    case trunks::TPM_RC_SYMMETRIC: return "TPM_RC_SYMMETRIC";
    case trunks::TPM_RC_TAG: return "TPM_RC_TAG";
    case trunks::TPM_RC_SELECTOR: return "TPM_RC_SELECTOR";
    case trunks::TPM_RC_INSUFFICIENT: return "TPM_RC_INSUFFICIENT";
    case trunks::TPM_RC_SIGNATURE: return "TPM_RC_SIGNATURE";
    case trunks::TPM_RC_KEY: return "TPM_RC_KEY";
    case trunks::TPM_RC_POLICY_FAIL: return "TPM_RC_POLICY_FAIL";
    case trunks::TPM_RC_INTEGRITY: return "TPM_RC_INTEGRITY";
    case trunks::TPM_RC_TICKET: return "TPM_RC_TICKET";
    case trunks::TPM_RC_RESERVED_BITS: return "TPM_RC_RESERVED_BITS";
    case trunks::TPM_RC_BAD_AUTH: return "TPM_RC_BAD_AUTH";
    case trunks::TPM_RC_EXPIRED: return "TPM_RC_EXPIRED";
    case trunks::TPM_RC_POLICY_CC: return "TPM_RC_POLICY_CC";
    case trunks::TPM_RC_BINDING: return "TPM_RC_BINDING";
    case trunks::TPM_RC_CURVE: return "TPM_RC_CURVE";
    case trunks::TPM_RC_ECC_POINT: return "TPM_RC_ECC_POINT";
    case trunks::TPM_RC_CONTEXT_GAP: return "TPM_RC_CONTEXT_GAP";
    case trunks::TPM_RC_OBJECT_MEMORY: return "TPM_RC_OBJECT_MEMORY";
    case trunks::TPM_RC_SESSION_MEMORY: return "TPM_RC_SESSION_MEMORY";
    case trunks::TPM_RC_MEMORY: return "TPM_RC_MEMORY";
    case trunks::TPM_RC_SESSION_HANDLES: return "TPM_RC_SESSION_HANDLES";
    case trunks::TPM_RC_OBJECT_HANDLES: return "TPM_RC_OBJECT_HANDLES";
    case trunks::TPM_RC_LOCALITY: return "TPM_RC_LOCALITY";
    case trunks::TPM_RC_YIELDED: return "TPM_RC_YIELDED";
    case trunks::TPM_RC_CANCELED: return "TPM_RC_CANCELED";
    case trunks::TPM_RC_TESTING: return "TPM_RC_TESTING";
    case trunks::TPM_RC_REFERENCE_H0: return "TPM_RC_REFERENCE_H0";
    case trunks::TPM_RC_REFERENCE_H1: return "TPM_RC_REFERENCE_H1";
    case trunks::TPM_RC_REFERENCE_H2: return "TPM_RC_REFERENCE_H2";
    case trunks::TPM_RC_REFERENCE_H3: return "TPM_RC_REFERENCE_H3";
    case trunks::TPM_RC_REFERENCE_H4: return "TPM_RC_REFERENCE_H4";
    case trunks::TPM_RC_REFERENCE_H5: return "TPM_RC_REFERENCE_H5";
    case trunks::TPM_RC_REFERENCE_H6: return "TPM_RC_REFERENCE_H6";
    case trunks::TPM_RC_REFERENCE_S0: return "TPM_RC_REFERENCE_S0";
    case trunks::TPM_RC_REFERENCE_S1: return "TPM_RC_REFERENCE_S1";
    case trunks::TPM_RC_REFERENCE_S2: return "TPM_RC_REFERENCE_S2";
    case trunks::TPM_RC_REFERENCE_S3: return "TPM_RC_REFERENCE_S3";
    case trunks::TPM_RC_REFERENCE_S4: return "TPM_RC_REFERENCE_S4";
    case trunks::TPM_RC_REFERENCE_S5: return "TPM_RC_REFERENCE_S5";
    case trunks::TPM_RC_REFERENCE_S6: return "TPM_RC_REFERENCE_S6";
    case trunks::TPM_RC_NV_RATE: return "TPM_RC_NV_RATE";
    case trunks::TPM_RC_LOCKOUT: return "TPM_RC_LOCKOUT";
    case trunks::TPM_RC_RETRY: return "TPM_RC_RETRY";
    case trunks::TPM_RC_NV_UNAVAILABLE: return "TPM_RC_NV_UNAVAILABLE";
    case trunks::TPM_RC_NOT_USED: return "TPM_RC_NOT_USED";
    case trunks::TRUNKS_RC_AUTHORIZATION_FAILED:
      return "TRUNKS_RC_AUTHORIZATION_FAILED";
    case trunks::TRUNKS_RC_ENCRYPTION_FAILED:
      return "TRUNKS_RC_ENCRYPTION_FAILED";
    case trunks::TRUNKS_RC_READ_ERROR: return "TRUNKS_RC_READ_ERROR";
    case trunks::TRUNKS_RC_WRITE_ERROR: return "TRUNKS_RC_WRITE_ERROR";
    case trunks::TRUNKS_RC_IPC_ERROR: return "TRUNKS_RC_IPC_ERROR";
    case trunks::TCTI_RC_TRY_AGAIN: return "TCTI_RC_TRY_AGAIN";
    case trunks::TCTI_RC_GENERAL_FAILURE: return "TCTI_RC_GENERAL_FAILURE";
    case trunks::TCTI_RC_BAD_CONTEXT: return "TCTI_RC_BAD_CONTEXT";
    case trunks::TCTI_RC_WRONG_ABI_VERSION: return "TCTI_RC_WRONG_ABI_VERSION";
    case trunks::TCTI_RC_NOT_IMPLEMENTED: return "TCTI_RC_NOT_IMPLEMENTED";
    case trunks::TCTI_RC_BAD_PARAMETER: return "TCTI_RC_BAD_PARAMETER";
    case trunks::TCTI_RC_INSUFFICIENT_BUFFER:
      return "TCTI_RC_INSUFFICIENT_BUFFER";
    case trunks::TCTI_RC_NO_CONNECTION: return "TCTI_RC_NO_CONNECTION";
    case trunks::TCTI_RC_DRIVER_NOT_FOUND: return "TCTI_RC_DRIVER_NOT_FOUND";
    case trunks::TCTI_RC_DRIVERINFO_NOT_FOUND:
      return "TCTI_RC_DRIVERINFO_NOT_FOUND";
    case trunks::TCTI_RC_NO_RESPONSE: return "TCTI_RC_NO_RESPONSE";
    case trunks::TCTI_RC_BAD_VALUE: return "TCTI_RC_BAD_VALUE";
    case trunks::SAPI_RC_INVALID_SESSIONS: return "SAPI_RC_INVALID_SESSIONS";
    case trunks::SAPI_RC_ABI_MISMATCH: return "SAPI_RC_ABI_MISMATCH";
    case trunks::SAPI_RC_INSUFFICIENT_BUFFER:
      return "SAPI_RC_INSUFFICIENT_BUFFER";
    case trunks::SAPI_RC_BAD_PARAMETER: return "SAPI_RC_BAD_PARAMETER";
    case trunks::SAPI_RC_BAD_SEQUENCE: return "SAPI_RC_BAD_SEQUENCE";
    case trunks::SAPI_RC_NO_DECRYPT_PARAM: return "SAPI_RC_NO_DECRYPT_PARAM";
    case trunks::SAPI_RC_NO_ENCRYPT_PARAM: return "SAPI_RC_NO_ENCRYPT_PARAM";
    case trunks::SAPI_RC_NO_RESPONSE_RECEIVED:
      return "SAPI_RC_NO_RESPONSE_RECEIVED";
    case trunks::SAPI_RC_BAD_SIZE: return "SAPI_RC_BAD_SIZE";
    case trunks::SAPI_RC_CORRUPTED_DATA: return "SAPI_RC_CORRUPTED_DATA";
    case trunks::SAPI_RC_INSUFFICIENT_CONTEXT:
      return "SAPI_RC_INSUFFICIENT_CONTEXT";
    case trunks::SAPI_RC_INSUFFICIENT_RESPONSE:
      return "SAPI_RC_INSUFFICIENT_RESPONSE";
    case trunks::SAPI_RC_INCOMPATIBLE_TCTI: return "SAPI_RC_INCOMPATIBLE_TCTI";
    case trunks::SAPI_RC_MALFORMED_RESPONSE:
      return "SAPI_RC_MALFORMED_RESPONSE";
    case trunks::SAPI_RC_BAD_TCTI_STRUCTURE:
      return "SAPI_RC_BAD_TCTI_STRUCTURE";
    default: return std::string();
  }
  NOTREACHED();
  return std::string();
}

bool IsFormatOne(trunks::TPM_RC error) {
  return (error & kLayerMask) == 0 && (error & trunks::RC_FMT1) != 0;
}

}  // namespace

namespace trunks {

std::string GetErrorString(TPM_RC error) {
  std::string error_string = GetErrorStringInternal(error);
  if (!error_string.empty()) {
    return error_string;
  }
  std::stringstream ss;
  if ((error & kLayerMask) == kResourceManagerTpmErrorBase) {
    error &= ~kLayerMask;
    error_string = GetErrorStringInternal(error);
    ss << "Resource Manager: ";
  }
  // Check if we have a TPM 'Format-One' response code.
  if (IsFormatOne(error)) {
    if (error & TPM_RC_P) {
      ss << "Parameter ";
    } else if (error & TPM_RC_S) {
      ss << "Session ";
    } else {
      ss << "Handle ";
    }
    // Bits 8-10 specify which handle / parameter / session.
    ss << ((error & kFormatOneSubjectMask) >> 8) << ": ";
    // Mask out everything but the format bit and error number.
    error_string = GetErrorStringInternal(error & kFormatOneErrorMask);
    if (!error_string.empty()) {
      ss << error_string;
    }
  }
  if (error_string.empty()) {
    ss << "Unknown error: " << error << " (0x" << std::hex << error << ")";
  }
  return ss.str();
}

TPM_RC GetFormatOneError(TPM_RC error) {
  if (IsFormatOne(error)) {
    return (error & kFormatOneErrorMask);
  }
  return error;
}

std::string CreateErrorResponse(TPM_RC error_code) {
  const uint32_t kErrorResponseSize = 10;
  std::string response;
  CHECK_EQ(Serialize_TPM_ST(TPM_ST_NO_SESSIONS, &response), TPM_RC_SUCCESS);
  CHECK_EQ(Serialize_UINT32(kErrorResponseSize, &response), TPM_RC_SUCCESS);
  CHECK_EQ(Serialize_TPM_RC(error_code, &response), TPM_RC_SUCCESS);
  return response;
}

}  // namespace trunks
