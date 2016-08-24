# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome


class platform_Attestation(test.test):
    version = 1

    def enroll(self):
        utils.system(cryptohome.CRYPTOHOME_CMD +
                     ' --action=tpm_attestation_start_enroll' +
                     ' --file=/tmp/__attestation_enroll_request')
        utils.system('curl' +
                     ' --data-binary "@/tmp/__attestation_enroll_request"' +
                     ' -o "/tmp/__attestation_enroll_response"' +
                     ' -H "Content-Type: application/octet-stream"' +
                     ' https://chromeos-ca.gstatic.com/enroll')
        utils.system(cryptohome.CRYPTOHOME_CMD +
                     ' --action=tpm_attestation_finish_enroll' +
                     ' --file=/tmp/__attestation_enroll_response')

    def cert_request(self):
        utils.system(cryptohome.CRYPTOHOME_CMD +
                     ' --action=tpm_attestation_start_cert_request' +
                     ' --file=/tmp/__attestation_cert_request')
        utils.system('curl --data-binary "@/tmp/__attestation_cert_request"' +
                     ' -o "/tmp/__attestation_cert_response"' +
                     ' -H "Content-Type: application/octet-stream"' +
                     ' https://chromeos-ca.gstatic.com/sign')
        utils.system(cryptohome.CRYPTOHOME_CMD +
                     ' --action=tpm_attestation_finish_cert_request' +
                     ' --file=/tmp/__attestation_cert_response' +
                     ' --name=attest-ent-machine')

    def run_once(self):
        status = cryptohome.get_tpm_attestation_status()
        if (not status['Prepared']):
            raise error.TestFail('Attestation enrollment is not possible.')
        self.enroll()
        status = cryptohome.get_tpm_attestation_status()
        if (not status['Enrolled']):
            raise error.TestFail('Attestation not successfully enrolled.')
        self.cert_request()
