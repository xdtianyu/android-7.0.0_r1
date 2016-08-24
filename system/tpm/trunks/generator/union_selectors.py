#
# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""Provides selector information for TPM 2.0 unions.

Describing this information explicitly is easier than extracting it from the
specification.
"""

_SELECTORS = {
    'TPMU_HA': {
        'type': ['TPMI_ALG_HASH'],
        'selectors': {
            'TPM_ALG_SHA1': 'sha1[SHA1_DIGEST_SIZE]',
            'TPM_ALG_SHA256': 'sha256[SHA256_DIGEST_SIZE]',
            'TPM_ALG_SM3_256': 'sm3_256[SM3_256_DIGEST_SIZE]',
            'TPM_ALG_SHA384': 'sha384[SHA384_DIGEST_SIZE]',
            'TPM_ALG_SHA512': 'sha512[SHA512_DIGEST_SIZE]',
            'TPM_ALG_NULL': ''
        }
    },
    'TPMU_CAPABILITIES': {
        'type': ['TPM_CAP'],
        'selectors': {
            'TPM_CAP_ALGS': 'algorithms',
            'TPM_CAP_HANDLES': 'handles',
            'TPM_CAP_COMMANDS': 'command',
            'TPM_CAP_PP_COMMANDS': 'ppCommands',
            'TPM_CAP_AUDIT_COMMANDS': 'auditCommands',
            'TPM_CAP_PCRS': 'assignedPCR',
            'TPM_CAP_TPM_PROPERTIES': 'tpmProperties',
            'TPM_CAP_PCR_PROPERTIES': 'pcrProperties',
            'TPM_CAP_ECC_CURVES': 'eccCurves',
        }
    },
    'TPMU_ATTEST': {
        'type': ['TPMI_ST_ATTEST'],
        'selectors': {
            'TPM_ST_ATTEST_CERTIFY': 'certify',
            'TPM_ST_ATTEST_CREATION': 'creation',
            'TPM_ST_ATTEST_QUOTE': 'quote',
            'TPM_ST_ATTEST_COMMAND_AUDIT': 'commandAudit',
            'TPM_ST_ATTEST_SESSION_AUDIT': 'sessionAudit',
            'TPM_ST_ATTEST_TIME': 'time',
            'TPM_ST_ATTEST_NV': 'nv',
        }
    },
    'TPMU_SYM_KEY_BITS': {
        'type': ['TPMI_ALG_SYM', 'TPMI_ALG_SYM_OBJECT'],
        'selectors': {
            'TPM_ALG_AES': 'aes',
            'TPM_ALG_SM4': 'SM4',
            'TPM_ALG_XOR': 'xor_',
            'TPM_ALG_NULL': '',
        }
    },
    'TPMU_SYM_MODE': {
        'type': ['TPMI_ALG_SYM', 'TPMI_ALG_SYM_OBJECT'],
        'selectors': {
            'TPM_ALG_AES': 'aes',
            'TPM_ALG_SM4': 'SM4',
            'TPM_ALG_XOR': '',
            'TPM_ALG_NULL': '',
        }
    },
    'TPMU_SYM_DETAILS': {
        'type': ['TPMI_ALG_SYM', 'TPMI_ALG_SYM_OBJECT'],
        'selectors': {}
    },
    'TPMU_SCHEME_KEYEDHASH': {
        'type': ['TPMI_ALG_KEYEDHASH_SCHEME'],
        'selectors': {
            'TPM_ALG_HMAC': 'hmac',
            'TPM_ALG_XOR': 'xor_',
            'TPM_ALG_NULL': '',
        }
    },
    'TPMU_SIG_SCHEME': {
        'type': ['TPMI_ALG_SIG_SCHEME', 'TPMI_ALG_ECC_SCHEME'],
        'selectors': {
            'TPM_ALG_RSASSA': 'rsassa',
            'TPM_ALG_RSAPSS': 'rsapss',
            'TPM_ALG_ECDSA': 'ecdsa',
            'TPM_ALG_SM2': 'sm2',
            'TPM_ALG_ECDAA': 'ecdaa',
            'TPM_ALG_ECSCHNORR': 'ecSchnorr',
            'TPM_ALG_HMAC': 'hmac',
            'TPM_ALG_NULL': '',
        }
    },
    'TPMU_KDF_SCHEME': {
        'type': ['TPMI_ALG_KDF'],
        'selectors': {
            'TPM_ALG_MGF1': 'mgf1',
            'TPM_ALG_KDF1_SP800_56a': 'kdf1_SP800_56a',
            'TPM_ALG_KDF2': 'kdf2',
            'TPM_ALG_KDF1_SP800_108': 'kdf1_sp800_108',
            'TPM_ALG_NULL': '',
        }
    },
    'TPMU_ASYM_SCHEME': {
        'type': ['TPMI_ALG_ASYM_SCHEME',
                 'TPMI_ALG_RSA_SCHEME',
                 'TPMI_ALG_RSA_DECRYPT',
                 'TPMI_ALG_ECC_SCHEME'],
        'selectors': {
            'TPM_ALG_RSASSA': 'rsassa',
            'TPM_ALG_RSAPSS': 'rsapss',
            'TPM_ALG_RSAES': '',
            'TPM_ALG_OAEP': 'oaep',
            'TPM_ALG_ECDSA': 'ecdsa',
            'TPM_ALG_SM2': 'sm2',
            'TPM_ALG_ECDAA': 'ecdaa',
            'TPM_ALG_ECSCHNORR': 'ecSchnorr',
            'TPM_ALG_ECDH': 'ecdh',
            'TPM_ALG_NULL': '',
        }
    },
    'TPMU_SIGNATURE': {
        'type': ['TPMI_ALG_SIG_SCHEME'],
        'selectors': {
            'TPM_ALG_RSASSA': 'rsassa',
            'TPM_ALG_RSAPSS': 'rsapss',
            'TPM_ALG_ECDSA': 'ecdsa',
            'TPM_ALG_SM2': 'sm2',
            'TPM_ALG_ECDAA': 'ecdaa',
            'TPM_ALG_ECSCHNORR': 'ecschnorr',
            'TPM_ALG_HMAC': 'hmac',
            'TPM_ALG_NULL': '',
        }
    },
    'TPMU_PUBLIC_PARMS': {
        'type': ['TPMI_ALG_PUBLIC'],
        'selectors': {
            'TPM_ALG_KEYEDHASH': 'keyedHashDetail',
            'TPM_ALG_SYMCIPHER': 'symDetail',
            'TPM_ALG_RSA': 'rsaDetail',
            'TPM_ALG_ECC': 'eccDetail',
        }
    },
    'TPMU_PUBLIC_ID': {
        'type': ['TPMI_ALG_PUBLIC'],
        'selectors': {
            'TPM_ALG_KEYEDHASH': 'keyedHash',
            'TPM_ALG_SYMCIPHER': 'sym',
            'TPM_ALG_RSA': 'rsa',
            'TPM_ALG_ECC': 'ecc',
        }
    },
    'TPMU_SENSITIVE_COMPOSITE': {
        'type': ['TPMI_ALG_PUBLIC'],
        'selectors': {
            'TPM_ALG_KEYEDHASH': 'bits',
            'TPM_ALG_SYMCIPHER': 'sym',
            'TPM_ALG_RSA': 'rsa',
            'TPM_ALG_ECC': 'ecc',
        }
    },
}


def GetUnionSelectorType(union_type):
  """Returns the selector type for a given union."""
  return _SELECTORS[union_type]['type'][0]


def GetUnionSelectorTypes(union_type):
  """Returns a list of all acceptable selector types for a given union."""
  return _SELECTORS[union_type]['type']


def GetUnionSelectorValues(union_type):
  """Returns the list of possible selector values for a given union."""
  return _SELECTORS[union_type]['selectors'].keys()


def GetUnionSelectorField(union_type, selector_value):
  """Returns the union field associated with a given selector value."""
  return _SELECTORS[union_type]['selectors'][selector_value]
