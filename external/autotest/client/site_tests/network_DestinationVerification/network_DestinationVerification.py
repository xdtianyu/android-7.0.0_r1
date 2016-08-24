# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import json
import logging
import os.path
import tempfile

import common
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

# This modifies the include path to include the shill test-scripts.
# pylint: disable=W0611
from autotest_lib.client.cros import flimflam_test_path
# pylint: enable=W0611
import crypto_util_pb2


TEST_DATA1 = """{
    "connected":false,
    "debug_build":true,
    "has_update":false,
    "hotspot_bssid":"00:1A:11:FF:AC:DF",
    "locale":"en_US",
    "location": {
        "latitude":37.4193105,
        "longitude":-122.07878869999999
    },
    "mac_address":"4C:AA:16:A5:AC:DF",
    "name":"eureka8997",
    "public_key":"MIGJAoGBAK3SXmWZBOhJibv8It05qIbgHXXhnCXxHkW+C6jNMHR5sZgDpFaOY1xwXERjKdJxcwrEy3VAT5Uv9MgHPBvxxJku76HYh1yVfIw1rhLnHBTHSxwUzJNCrgc3l3t/UACacLjVNIzccDpYf2vnOcA+t1t6IXRjzuU2NdwY4dJXNtWPAgMBAAE=",
    "setup_state":11,
    "sign": {
        "certificate":"-----BEGIN CERTIFICATE-----\\nMIIDhzCCAm8CBFE2SCMwDQYJKoZIhvcNAQEFBQAwfTELMAkGA1UEBhMCVVMxEzAR\\nBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDU1vdW50YWluIFZpZXcxEzARBgNV\\nBAoMCkdvb2dsZSBJbmMxEjAQBgNVBAsMCUdvb2dsZSBUVjEYMBYGA1UEAwwPRXVy\\nZWthIEdlbjEgSUNBMB4XDTEzMDMwNTE5MzE0N1oXDTMzMDIyODE5MzE0N1owgYMx\\nFjAUBgNVBAcTDU1vdW50YWluIFZpZXcxEjAQBgNVBAsTCUdvb2dsZSBUVjETMBEG\\nA1UEChMKR29vZ2xlIEluYzETMBEGA1UECBMKQ2FsaWZvcm5pYTELMAkGA1UEBhMC\\nVVMxHjAcBgNVBAMUFWV2dF9lMTYxIDAwMWExMWZmYWNkZjCCASIwDQYJKoZIhvcN\\nAQEBBQADggEPADCCAQoCggEBAPHGDV0lLoTYK78q13y/2u77YTjgbBlWAOxgrSNc\\nMmGHx1K0aPyo50p99dGQnjapW6jtGrMzReWV2Wz3VL8rYlqY7oWjeJwsLQwo2tcn\\n7vIZ/PuvPz9xgnGMUbBOfhCf3Epb1N4Jz82pxxrOFhUawWAglC9C4fUeZLCZpOJs\\nQd4QeAznkydl3xbqdSm74kwxE6vkGEzSCDnC7aYx0Rvvr1mZOKdl4AinYrxzWgmV\\nsTnaFT1soSjmC5e/i6Jcrs4dDFgY6mKy9Qtly2XPSCYljm6L4SgqgJNmlpY0qYJg\\nO++BdofIbU2jsOiCMvIuKkbMn72NsPQG0QhnVMwk7kYg6kkCAwEAAaMNMAswCQYD\\nVR0TBAIwADANBgkqhkiG9w0BAQUFAAOCAQEAW0bQl9yjBc7DgMp94i7ZDOUxKQrz\\nthephuwzb3/wWiTHcw6KK6FRPefXn6NPWxKKeQmv/tBxHbVlmYRXUbrhksnD0aUk\\ni4InvtL2m0H1fPfMxmJRFE+HoSXu+s0sGON831JaMcYRbAku5uHnltaGNzOI0KPH\\nFGoCDmjAZD+IuoR2LR4FuuTrECK7KLjkdf//z5d5j7nBDPZS7uTCwC/BwM9asRj3\\ntJA5VRFbLbsit1VI7IaRCk9rsSKkpBUaVeKbPLz+y/Z6JonXXT6AxsfgUSKDd4B7\\nMYLrTwMQfGuUaaaKko6ldKIrovjrcPloQr1Hxb2bipFcjLmG7nxQLoS6vQ==\\n-----END CERTIFICATE-----\\n",
        "nonce":"+6KSGuRu833m1+TP",
        "signed_data":"vwMBgANrp5XpCswLyk/OTXT56ORPeIWjH7xAdCk3qgjkwI6+8o56zJS02+tC5hhIHWh7oppTmWYF4tKvBQ3GeCz7IW9f7HWDMtO7x7yRWxzJyehaJbCfXvLdfs0/WKllzvGVBgNpcIAwU2NSFUG/jpXclntFzds0EUJG9wHxS6PXXSYRu+PlIFdCDcQJsUlnwO9AGFOJRV/aARGh8YUTWCFIQPOtPEqT5eegt+TLf01Gq0YcrRwSTKy1I3twOnWiMfIdkJdQKPtBwwbvuAyGuqYFocfjKABbnH9Tvl04yyO3euKbYlSqaF/l8CXmzDJTyO7tDOFK59bV9auE4KljrQ=="
    },
    "ssdp_udn":"c5b2a83b-5958-7ce6-b179-e1f44699429b",
    "ssid":"",
    "timezone":"America/Los_Angeles",
    "uptime":1991.7,
    "version":4,
    "wpa_configured":false,
    "wpa_state":1
} """
TEST_DATA2 = """{
    "connected": false,
    "debug_build": true,
    "has_update": false,
    "hotspot_bssid": "00:1A:11:FF:AC:1E",
    "ip_address": "192.168.43.32",
    "locale": "en_US",
    "mac_address": "B0:EE:45:49:AC:1E",
    "name": "Greg's Eureka",
    "noise_level": -85,
    "public_key": "MIGJAoGBAOe+6bF51A7wFVMbyPiHYLdgAmP6sdhOUohqCHn4qHSfDY41AbAbVmXLbUZ5BF2KSdDYqU4fAXoaI8V8D5DRWh57Ax10Sl1/6M1u22KT6FYQyUToXGPcXldBzRRMok8H4XyiebDVevjvvV6yuABSYYhfSlrMdGj8qxRVwTxx0CItAgMBAAE=",
    "release_track": "beta-channel",
    "setup_state": 50,
    "sign": {
        "certificate": "-----BEGIN CERTIFICATE-----\\nMIIDejCCAmICBFEtN4wwDQYJKoZIhvcNAQEFBQAwfTELMAkGA1UEBhMCVVMxEzAR\\nBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDU1vdW50YWluIFZpZXcxEzARBgNV\\nBAoMCkdvb2dsZSBJbmMxEjAQBgNVBAsMCUdvb2dsZSBUVjEYMBYGA1UEAwwPRXVy\\nZWthIEdlbjEgSUNBMB4XDTEzMDIyNjIyMzAzNloXDTMzMDIyMTIyMzAzNlowdzET\\nMBEGA1UECBMKQ2FsaWZvcm5pYTELMAkGA1UEBhMCVVMxFjAUBgNVBAcTDU1vdW50\\nYWluIFZpZXcxEjAQBgNVBAsTCUdvb2dsZSBUVjETMBEGA1UEChMKR29vZ2xlIElu\\nYzESMBAGA1UEAxQJZXZ0X2UxMjYyMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\\nCgKCAQEAo7Uu+bdyCjtiUYpmNU4ZvRjDg6VkEh/g0YPDG2pICBU4XKvsqHH1i0hb\\ntWp1J79hV9Rqst1yHT02Oeh3o1SOd2zeamYzmvXRVN7AZqfQlzWxwxk/ltpXGwew\\nm+EIR2bP4kpvyEKvvziTMtTxviOK+A395QyodMhMXClKTus/Gme2r1fBoQqJJR/z\\nrmwXCsl5kpdhj7FOIII3BCYV0zejjQquzywjsKfCVON28VGgJdaKgmXxkeRYYWVN\\nnuTNna57vXe16FP6hS1ty1U77ESffLTpNJ/M4tsd2dMVVTDuGeX3q8Ix4TN8cqpq\\nu1AKEf59hygys9j6cHZRKR/div0+uQIDAQABow0wCzAJBgNVHRMEAjAAMA0GCSqG\\nSIb3DQEBBQUAA4IBAQAZx6XyEK9SLHE+rbKCVsLN9+hTEa50aikPmxOZt+lFuB4+\\nVJZ/GCPQCZJIde2tlWUe2YBgoZw2xUKgIsM3Yq42Gawi35/oZ3qycTgYU8KJP9kU\\nMbYNAH90mz9BDH7MmnRID5dFexHyBCG88EJ+ZvxmUVn0EVDcsSMt11wIAZ/T+/gs\\nE1120d/GxhjYQ9YZz7SZXBQfRdqCdcPNl2+QSHHl+WvYLzdJa2xYj39/kQu47Vp7\\nX5rZrHSBvzdVymH0Od2D18t+Q6lxbSdyUNhP1MVhdkT1Ct4OmRS3FJ4aannXMhfq\\nNg7k4Sfif5iktYT4VRKpThe0EGJNfqKJKYtvHEVC\\n-----END CERTIFICATE-----\\n",
        "nonce": "pTwRhdsB5cb3Ev8o",
        "signed_data": "WIvN6Ujo4CoxUNcsm1YaLRUe1KJVEG+oXorUBJae/fKQrLfnH9ChDfxzW+EDZlLBLPk9u5EAptr8LKK6AunbBTDIeBkjzXv3nS+xlmV9ZdA71imitva88HPzC+a2H61dJL8puNbZu9j1Zs3sCybw2F+qZbDBhbG0sJTEXytPjewqAl8iBZSAS0BoNJZYA7Q/bCPI07pg404pI392cKP8FYJR08Y4xoV94Em+jnZ2nZabSsmsScYGvpWVNeS2z+f0to6ureOxaqgT+AAckqtCRcHd66QtLGwKXWviaevKte1z185f4r55U4P5pkQi+xd6lZRsMQydwUzLxgk7UY5U3Q=="
    },
    "signal_level": -42,
    "ssdp_udn": "cd7c1f15-2f49-076b-51ac-9d651872c784",
    "ssid": "Greg's Phone",
    "timezone": "America/Los_Angeles",
    "uptime": 207.33,
    "version": 4,
    "wpa_configured": true,
    "wpa_id": 0,
    "wpa_state": 10
}
"""
TEST_DATA3 = """{
    "build_version":"10566",
    "connected":false,
    "has_update":false,
    "hotspot_bssid":"FA:8F:CA:30:08:26",
    "locale":"en_US",
    "mac_address":"B0:EE:45:68:B5:52",
    "name":"Chromekey Greg",
    "public_key":"MIIBCgKCAQEApQoxFRJWEP9Oa+lSF2PCMBCpd2LYeyPMHjVKCF2IlrGbrgQ3U9wI6tBvLkHg570KSA6onL/7e9J31RLwA8gIE0zJ9M7NToxj1cVa8jOqXPIllJP7uY/TmtMIwCvuMCu6KBNW/wzKK6jkT7wScwrPJscmwWr/0h6lKXZrv0DhwvVv3i6VGsasZzVYlOoZ7eRe1OHiJgehPFUh0Vp7lVRDzumtJ0N6hybWP/Ap6dNlcO9Hq67bljHRrgwuBT2iRJBIdZt6m4xZlf78dH6Y2gfeQ3GBtDZWbZ5v6hzaYeIxlMTelz6ZlEQa0fPegK1dsxCddvLhRCDQr2bZlwyRdof6uwIDAQAB",
    "release_track":"beta-channel",
    "setup_state":11,
    "sign": {
        "certificate": "-----BEGIN CERTIFICATE-----\\nMIIDpjCCAo4CBFFfTz8wDQYJKoZIhvcNAQEFBQAwfTELMAkGA1UEBhMCVVMxEzAR\\nBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDU1vdW50YWluIFZpZXcxEzARBgNV\\nBAoMCkdvb2dsZSBJbmMxEjAQBgNVBAsMCUdvb2dsZSBUVjEYMBYGA1UEAwwPRXVy\\nZWthIEdlbjEgSUNBMB4XDTEzMDQwNTIyMjUwM1oXDTMzMDMzMTIyMjUwM1owgYAx\\nCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpDYWxpZm9ybmlhMRMwEQYDVQQKEwpHb29n\\nbGUgSW5jMRYwFAYDVQQHEw1Nb3VudGFpbiBWaWV3MRIwEAYDVQQLEwlHb29nbGUg\\nVFYxGzAZBgNVBAMTElpaQUlNIEZBOEZDQTMwMDgyNjCCASIwDQYJKoZIhvcNAQEB\\nBQADggEPADCCAQoCggEBALmAGRcIQt/k7WfoUHDCWQhzQ/FrHDnNiHcVj09Tyu7/\\n2pIB+pVnLac8pfFwdVAcDG4xmjnROIHeajz627uq8bHEVDfPCHe8WiM+CsPrMC85\\nlQOq8rBwNw3WO82suvWL8wjdqFy0uQHajp9e9ix3FXzfZDAuf7ABHmVZnPMWQz0a\\nR3zDCCF2NhEVhBwq+IGKNNvuIPhVF/5tkdWPmihFukJabmV2kjFUuPbEf4T1OMu+\\nhyqZ+lsTc/XVIqICRIq9ZE36MquAw0g9Hoah8OFFpkq2xJqlCY+HKvdhiA1HybfY\\nXSxOsI2rTQk3YHFMrAGMu16UYuOA+CRHD+UC8SI0fqkCAwEAAaMvMC0wCQYDVR0T\\nBAIwADALBgNVHQ8EBAMCB4AwEwYDVR0lBAwwCgYIKwYBBQUHAwIwDQYJKoZIhvcN\\nAQEFBQADggEBAJGyWBoDvwZBlDcyX31l5kwGVf6nVZR4KedcQMGpm76C6fgzeLHt\\nAq7OQhZgifFVLvzG7kqYgRRsW7WQKTF/QmHn8rnCdY16YtZv3vFzLbdBWkmSqAxD\\nqmBum9oD74AYeJaR0v68KCGvAVG0juRSs3H93s8k/fhY+LespUMlHwB4DwRYxDNP\\nIu3HX8NEmOBwjodKDDa8yu1weYUf8GFXH3b+tu7IHRzQpPzbSSibs+AXc8WbJWk3\\nrtQSugqegiiM/OAdRuSvkXtc1DJP9F7OLsVpF9i/VZ2k/QDPCu06vXB2w3wlRyR3\\nf6nQGB1Ej8Klsxlo8B+rmQ6N8C7f48GMI8s=\\n-----END CERTIFICATE-----\\n",
        "nonce": "E1oqNODwWVIfSjUs",
        "signed_data": "aaT56/P7KiMxVbWCedpSNIikpxh0EzdjgEuP153pCNfYys2KmlVtvEnXeFGYKJP2ypsX0qx/9Bx2C18T8HZ4PvNW0fUEheScvkzbDTn/gBpwUJv3XT+55XneUdjIAQu83qNR/AjxYa9cDGzxbnZJL91Fd1Grw/fNaKEMiedWRiESABQREiS2OW+iZ3Z/JRs99o4M6m+6f/mvNkgZ3GrFxNnPEXe7g1sYjspPhnDLvASNzd9j24YIxu+PTWZCNL6clps+/ghNA/0fB3PBDILrcGnER36Vc8gNhjD1r1f/ACU1qk06yMNC/Kgt2pMQkpjaXby+B75Z9yrogNHs4f5tIA=="
    },
    "ssdp_udn":"b91027b5-f19e-7df6-6533-5056ad190575",
    "ssid":"Greg's Phone",
    "uptime":262650.75,
    "version":4,
    "wpa_configured":false,
    "wpa_id":0,
    "wpa_state":1
}"""
# This is just a DER formatted public key with the first 22 bytes stripped.
# Technically, this is called RSAPublicKey format.
ENCRYPTION_PUBLIC_KEY = (
    '0\x81\x89\x02\x81\x81\x00\xd4\xe3z\x82\x0b6D\x8a\x1dY'
    '\x1d\xdel\xbf\xad,\r\x8b\xefm\xe3\xbd\xef<\x954\x96C<'
    '\x9e\x7fB\xef\xe8&h\xfb\xc4Z\x0e\x196C\x1d\xd7f\x88\\5'
    '\xd5\x1e\xb8\xe0\x8d\xc4\xbc\xd1\xb3\xc4\xd3)!7\xd2{'
    '\x11\x89\t\xfb\x8d\xe8\xbaRL\xacI\xf2\x0f.JB\x8e\xd4'
    '\xdc\xc2\x81v6\x8a\x154\xbe8\xfc\xa8\x84zH\xf0j\x87('
    '\xcf\x142B\xcfV\xd0\n\xbbPW\x80d\xd2\x0e\xf1\x1a\x04'
    '\x0b\xa8U=\x8f3\x85\xcd\x02\x03\x01\x00\x01'
)
# This is the matching private key.  Note that the public key is
# a subset of this information should you need it for debugging.
ENCRYPTION_PRIVATE_KEY = """
-----BEGIN RSA PRIVATE KEY-----
MIICXQIBAAKBgQDU43qCCzZEih1ZHd5sv60sDYvvbeO97zyVNJZDPJ5/Qu/oJmj7
xFoOGTZDHddmiFw11R644I3EvNGzxNMpITfSexGJCfuN6LpSTKxJ8g8uSkKO1NzC
gXY2ihU0vjj8qIR6SPBqhyjPFDJCz1bQCrtQV4Bk0g7xGgQLqFU9jzOFzQIDAQAB
AoGBALfdR/9s44/KoZJIQ8Q0v8HeaU9+30U5jF9pLaYggtty2nTsR5u6d/TZPY42
BcVeXBV6XbBa8NZMJelXQvCw6d3hrirtRgqo+R9n6Z5Ji9oKA9NR0ZD0d66OLgRz
7U1w2wiM9KqfjRpRhuID1MrDF8R0mxORSFnlMeoHD74ncK6ZAkEA+NBV2OOigyU/
CVWCTKmOpxUGb6bRZfvQRln4XlVUhSgZU0Acf0APl3Av1vUPyPjRIXRnMS5ZBDFK
YoUqWH3ymwJBANsJhX3HHYXk7aDrmPKE7q25xR7Eu4C7XEdiG0SSl8PNAicU0APy
zFs9GLhTgGCTZf/SCzH6ejEIiOC/RZeqW7cCQCapAF3F6O9lryi9H5TX17GAY9Kf
YfPtr4vu2NeXfJ2AAIdd88+V3ZZTOSu2QjCg8KW5F3udzvkGy58JP+4mC7cCQBEU
+gMoHyZNBzcwiHoJYe/MeBIBN7o/Yl/yx7ueTxWnDE7t8ZcNPWC0MBRX9sARXrgH
snXQWe0vBDW61PuR/psCQQDthOoMBnuWamOEZrEJr+itK5+ZPlE+KpT3ladXui57
zvz3CljmlLwyufahWaF+LzfohVAG5CLr4Zi/bv7j1lnJ
-----END RSA PRIVATE KEY-----
"""


def unicode2str(value):
    """
    Recurses through value, converting unicode to strings.

    The output of the json python decoder looks like a dictionary with
    unicode keys mapping to more unicode strings, and possibly
    dictionaries.  Convert those unicode strings to ASCII strings
    recursively with this function.

    @param value: dict or unicode.
    @returns value with unicode recursively replaced with string.

    """
    if isinstance(value, dict):
        ret = {}
        for k,v in value.items():
            ret[unicode2str(k)] = unicode2str(v)
        return ret

    if isinstance(value, unicode):
        return str(value)

    return value


class network_DestinationVerification(test.test):
    """Tests that the destination verification logic in shill works.

    This logic allows us to verify that (for instance) we trust the
    credentials of a Chromekey.

    """
    version = 1


    @property
    def shim_path(self):
        """@return path to crypto-util shim."""
        lib_paths = ['lib', 'lib64']
        shim_path = '/usr/%s/shill/shims/crypto-util'
        for lib in lib_paths:
            path = shim_path % lib
            if os.path.exists(path):
                return path
        raise error.TestFail('Unable to find crypto-util.')


    def _call_shim_with_args(self, args, protobuffer):
        """Calls the crypto_util shim with args and stdin as specified.

        Calls crypto shim with args for paramters, and the serialized form of
        protobuffer on stdin.  Asserts that the command completes successfully
        and returns the raw output bytestring.

        @param args: tuple of string arguments to the shim.
        @param protobuffer: python protocol buffer object.
        @return stdout of the shim call as string.

        """
        raw_input_bytes = protobuffer.SerializeToString()
        result  = utils.run(self.shim_path,
                            stderr_tee=utils.TEE_TO_LOGS,
                            stdin=raw_input_bytes,
                            verbose=True,
                            args=args,
                            timeout=10,
                            ignore_status=True)
        if result.exit_status:
            return None
        raw_output = result.stdout
        logging.debug('Got raw output: %s.',
                      ''.join([hex(ord(c)) for c in raw_output]))
        return raw_output


    def _test_verify(self, test_data, expect_failure=False):
        """Test that shim verify operation works correctly.

        Call into the shim to perform a verify operation, check that the call
        succeeds unless |expect_failure|.  |test_data| looks like the decoded
        json data we're given by the destination.

        @param test_data: dictionary of test data to use with the call.
        @param expect_failure: bool true if test_data should fail to verify.

        @raises TestFail when verification doesn't go as expected.

        """
        # See doc/manager-api.txt, but the format goes roughly:
        #     ssid,udn,bssid,key,nonce
        sign_bundle = test_data['sign']
        unsigned_data = '%s,%s,%s,%s,%s' % (test_data['name'],
                                            test_data['ssdp_udn'],
                                            test_data['hotspot_bssid'],
                                            test_data['public_key'],
                                            sign_bundle['nonce'])
        message = crypto_util_pb2.VerifyCredentialsMessage()
        message.certificate = sign_bundle['certificate']
        message.signed_data = base64.b64decode(sign_bundle['signed_data'])
        message.unsigned_data = unsigned_data
        message.mac_address = test_data['hotspot_bssid']
        output = self._call_shim_with_args(('verify',), message)
        if expect_failure:
            if output is None:
                # Expected a failure, and got it.  We're done here.
                return
            raise error.TestFail('Expected verification to fail, but it '
                                 'succeeded.')

        if not output:
            raise error.TestFail('Verification failed unexpectedly.')

        response = crypto_util_pb2.VerifyCredentialsResponse()
        response.ParseFromString(output)
        if response.ret != crypto_util_pb2.OK:
            raise error.TestFail('Expected verification success, '
                                 'but got: %d.' % response.ret)


    def _test_encrypt(self, public_key, data_to_encrypt, expect_failure=False):
        """Test that shim encrypt operation works correctly.

        Call into the shim to perform an encrypt operation.
        Raise a test error if the call fails, unless |expect_failure|.

        @param public_key: string containing RSAPublicKey format RSA key.
        @param data_to_encrypt: string data to encrypt.
        @param expect_failure: bool true if encrypt should fail.

        """
        message = crypto_util_pb2.EncryptDataMessage()
        message.public_key = public_key
        message.data = data_to_encrypt
        output = self._call_shim_with_args(('encrypt',), message)
        if expect_failure:
            if output is None:
                # Expected a failure, and got it.  We're done here.
                return
            raise error.TestFail('Expected encryption to fail, '
                                 'but it did not.')

        if not output:
            raise error.TestFail('Encryption failed unexpectedly.')

        response = crypto_util_pb2.EncryptDataResponse()
        response.ParseFromString(output)
        if response.ret != crypto_util_pb2.OK:
            raise error.TestFail('Data encryption failed.')

        logging.debug('Decrypting %d bytes of encrypted data to confirm match.',
                      len(response.encrypted_data))
        try:
            private_key = tempfile.NamedTemporaryFile()
            encrypted_data = tempfile.NamedTemporaryFile()
            private_key.write(ENCRYPTION_PRIVATE_KEY)
            private_key.flush()
            encrypted_data.write(response.encrypted_data)
            encrypted_data.flush()
            result = utils.run('openssl rsautl -decrypt -inkey %s -in %s' %
                                       (private_key.name, encrypted_data.name),
                               stdin=response.encrypted_data,
                               stderr_tee=utils.TEE_TO_LOGS,
                               verbose=True,
                               ignore_status=True)
            if result.exit_status:
                if data_to_encrypt == '':
                    # Due to an open bug in OpenSSL, rsautl doesn't handle this
                    # case correctly.  For now, just pass and assume that
                    # we're doing this correctly.
                    # http://rt.openssl.org/Ticket/Display.html?id=2018
                    return
                raise error.TestFail('Failed to decrypt shim output.')

            if result.stdout != data_to_encrypt:
                raise error.TestFail('Data encyption failed: '
                                     'expected: %s, but got: %s' %
                                     (data_to_encrypt, result.stdout))

        finally:
            private_key.close()
            encrypted_data.close()


    def run_once(self):
        """Body of the test."""
        logging.info('Checking basic encryption operation of the shim.')
        self._test_encrypt(ENCRYPTION_PUBLIC_KEY, 'disco boy')

        logging.info('Checking graceful fail for too much data to encrypt.')
        self._test_encrypt(ENCRYPTION_PUBLIC_KEY,
                           ''.join(['x' for i in range(500)]),
                            expect_failure=True)

        logging.info('Checking graceful fail for a bad key format.')
        self._test_encrypt('this will not parse in openssl',
                            'disco boy',
                            expect_failure=True)

        logging.info('Checking that we encrypt the empty string correctly.')
        self._test_encrypt(ENCRYPTION_PUBLIC_KEY, '')

        logging.info('Checking basic verification operation.')
        test_data = unicode2str(json.loads(TEST_DATA1))
        self._test_verify(test_data)

        logging.info('Checking bad nonce in unsigned_data.')
        test_data = unicode2str(json.loads(TEST_DATA1))
        test_data['sign']['nonce'] = 'does not match'
        self._test_verify(test_data, expect_failure=True)

        logging.info('Checking for graceful fail for invalid '
                     'certificate format')
        test_data = unicode2str(json.loads(TEST_DATA1))
        test_data['sign']['certificate'] = 'not a certificate'
        self._test_verify(test_data, expect_failure=True)

        logging.info('Verification fails when the certificate is signed, but '
                     'subject is malformed.')
        test_data = unicode2str(json.loads(TEST_DATA2))
        self._test_verify(test_data, expect_failure=True)

        logging.info('Verification should succeed on third set of data.')
        test_data = unicode2str(json.loads(TEST_DATA3))
        self._test_verify(test_data)

        logging.info('Test completed successfully.')
