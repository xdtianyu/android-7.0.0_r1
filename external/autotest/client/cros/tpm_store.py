# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import tempfile

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome

class TPMStore(object):
    """Context enclosing the use of the TPM."""

    CHAPS_CLIENT_COMMAND = 'chaps_client'
    CONVERT_TYPE_RSA = 'rsa'
    CONVERT_TYPE_X509 = 'x509'
    CRYPTOHOME_ACTION_TAKE_OWNERSHIP = 'tpm_take_ownership'
    CRYPTOHOME_ACTION_WAIT_OWNERSHIP = 'tpm_wait_ownership'
    CRYPTOHOME_COMMAND = '/usr/sbin/cryptohome'
    OPENSSL_COMMAND = 'openssl'
    OUTPUT_TYPE_CERTIFICATE = 'cert'
    OUTPUT_TYPE_PRIVATE_KEY = 'privkey'
    PIN = '11111'
    # TPM maintain two slots for certificates, slot 0 for system specific
    # certificates, slot 1 for user specific certificates. Currently, all
    # certificates are programmed in slot 1. So hardcode this slot ID for now.
    SLOT_ID = '1'
    PKCS11_REPLAY_COMMAND = 'p11_replay --slot=%s' % SLOT_ID
    TPM_GROUP = 'chronos-access'
    TPM_USER = 'chaps'


    def __enter__(self):
        self.setup()
        return self

    def __exit__(self, exception, value, traceback):
        self.reset()


    def _cryptohome_action(self, action):
        """Set the TPM up for operation in tests."""
        utils.system('%s --action=%s' % (self.CRYPTOHOME_COMMAND, action),
                     ignore_status=True)


    def _install_object(self, pem, identifier, conversion_type, output_type):
        """Convert a PEM object to DER and store it in the TPM.

        @param pem string PEM encoded object to be stored.
        @param identifier string associated with the new object.
        @param conversion_type the object type to use in PEM to DER conversion.
        @param output_type the object type to use in inserting into the TPM.

        """
        if cryptohome.is_tpm_lockout_in_effect():
            raise error.TestError('The TPM is in dictonary defend mode. '
                                  'The TPMStore may behave in unexpected '
                                  'ways, exiting.')
        pem_file = tempfile.NamedTemporaryFile()
        pem_file.file.write(pem)
        pem_file.file.flush()
        der_file = tempfile.NamedTemporaryFile()
        utils.system('%s %s -in %s -out %s -inform PEM -outform DER' %
                     (self.OPENSSL_COMMAND, conversion_type, pem_file.name,
                      der_file.name))
        utils.system('%s --import --type=%s --path=%s --id="%s"' %
                     (self.PKCS11_REPLAY_COMMAND, output_type, der_file.name,
                      identifier))


    def setup(self):
        """Set the TPM up for operation in tests."""
        self.reset()
        self._directory = tempfile.mkdtemp()
        utils.system('chown %s:%s %s' %
                     (self.TPM_USER, self.TPM_GROUP, self._directory))
        utils.system('%s --load --path=%s --auth="%s"' %
                     (self.CHAPS_CLIENT_COMMAND, self._directory, self.PIN))


    def reset(self):
        """Reset the crypto store and take ownership of the device."""
        utils.system('initctl restart chapsd')
        self._cryptohome_action(self.CRYPTOHOME_ACTION_TAKE_OWNERSHIP)
        self._cryptohome_action(self.CRYPTOHOME_ACTION_WAIT_OWNERSHIP)


    def install_certificate(self, certificate, identifier):
        """Install a certificate into the TPM, returning the certificate ID.

        @param certificate string PEM x509 contents of the certificate.
        @param identifier string associated with this certificate in the TPM.

        """
        return self._install_object(certificate,
                                    identifier,
                                    self.CONVERT_TYPE_X509,
                                    self.OUTPUT_TYPE_CERTIFICATE)


    def install_private_key(self, key, identifier):
        """Install a private key into the TPM, returning the certificate ID.

        @param key string PEM RSA private key contents.
        @param identifier string associated with this private key in the TPM.

        """
        return self._install_object(key,
                                    identifier,
                                    self.CONVERT_TYPE_RSA,
                                    self.OUTPUT_TYPE_PRIVATE_KEY)
