# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import tempfile

from autotest_lib.client.bin import utils

class PEMCertificate(object):
    """Object enclosing a PEM certificate.

    Uses the "openssl" utility to report various properties of a certificate.

    """
    OPENSSL_COMMAND = 'openssl'
    ATTRIBUTE_SUBJECT = 'subject'
    ATTRIBUTE_FINGERPRINT = 'fingerprint'

    def __init__(self, pem_contents):
        self._pem_contents = pem_contents
        self._fingerprint = None
        self._subject = None
        self._subject_dict = None


    def get_attribute(self, attribute):
        """Returns the named attribute of the certificate.

        @param attribute string referring to the attribute to retrieve.
        @return string containing the retrieved attribute value.

        """
        with tempfile.NamedTemporaryFile() as temp:
            temp.write(self._pem_contents)
            temp.flush()
            output = utils.system_output(
                '%s x509 -noout -%s -in %s' %
                (self.OPENSSL_COMMAND, attribute, temp.name))
        # Output is of the form "name=value..."
        return output.split('=', 1)[1]


    @property
    def fingerprint(self):
        """Returns the SHA-1 fingerprint of a certificate."""
        if self._fingerprint is None:
            self._fingerprint = self.get_attribute(self.ATTRIBUTE_FINGERPRINT)
        return self._fingerprint


    @property
    def subject(self):
        """Returns the subject DN of the certificate as a list of name=value"""
        if self._subject is None:
            subject = self.get_attribute(self.ATTRIBUTE_SUBJECT)
            # OpenSSL returns a form of:
            #   " /C=US/ST=CA/L=Mountain View/CN=chromelab..."
            # but we want to return something like:
            #   [ "C=US", "ST=CA", "L=Mountain View", "CN=chromelab..." ]
            self._subject = subject.lstrip(' /').split('/')
        return self._subject


    @property
    def subject_dict(self):
        """Returns the subject DN of the certificate as a dict of name:value"""
        if self._subject_dict is None:
          # Convert the list [ 'A=B', ... ] into a dict { 'A': 'B',  ... }
          self._subject_dict = dict(map(lambda x: x.split('=', 1),
                                        self.subject))
        return self._subject_dict
