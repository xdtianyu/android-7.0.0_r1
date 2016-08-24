# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import urlparse

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server import autotest, test

def _split_url(url):
    """Splits a URL into the URL base and path."""
    split_url = urlparse.urlsplit(url)
    url_base = urlparse.urlunsplit(
            (split_url.scheme, split_url.netloc, '', '', ''))
    url_path = split_url.path
    return url_base, url_path.lstrip('/')


class autoupdate_CatchBadSignatures(test.test):
    """This is a test to verify that update_engine correctly checks
    signatures in the metadata hash and the update payload
    itself. This is achieved by feeding updates to update_engine where
    the private key used to make the signature, intentionally does not
    match with the public key used for verification.

    By its very nature, this test requires an image signed with a
    well-known key. Since payload-generation is a resource-intensive
    process, we prepare the image ahead of time. Also, since the image
    is never successfully applied, we can get away with not caring that
    the image is built for one board but used on another.

    If you ever need to replace the test image, follow these eight
    simple steps:

    1. Build a test image:

      $ cd ~/trunk/src/scripts
      $ ./build_packages --board=${BOARD}
      $ ./build_image --board=${BOARD} --noenable_rootfs_verification test

    2. Serve the image the DUT like this:

      $ cd ~/trunk/strc/platform/dev
      $ ./devserver.py --test_image                               \
                       --private_key                              \
                         ../update_engine/unittest_key.pem        \
                       --private_key_for_metadata_hash_signature  \
                         ../update_engine/unittest_key.pem        \
                       --public_key                               \
                          ../update_engine/unittest_key2.pub.pem

    3. Update the DUT - the update should fail at the metadata
       verification stage.

    4. From the update_engine logs (stored in /var/log/update_engine/)
       on the DUT, find the Omaha response sent to the DUT and update
       the following constants with values from the XML:

        _IMAGE_SHA256: set it to the 'sha256'
        _IMAGE_METADATA_SIZE: set it to the 'MetadataSize'
        _IMAGE_PUBLIC_KEY2: set it to the 'PublicKeyRsa'
        _IMAGE_METADATA_SIGNATURE_WITH_KEY1: set it to 'MetadataSignatureRsa'

       Also download the image payload ('url' and 'codebase' tags),
       upload it to Google Storage and update the _IMAGE_GS_URL and
       _IMAGE_SIZE constants with the resulting URL and the size.

    5. Serve the image to the DUT again and note the slightly different
       parameters this time. Note that the image served is the same,
       however the Omaha response will be different.

      $ cd ~/trunk/strc/platform/dev
      $ ./devserver.py --test_image                               \
                       --private_key                              \
                         ../update_engine/unittest_key.pem        \
                       --private_key_for_metadata_hash_signature  \
                         ../update_engine/unittest_key2.pem       \
                       --public_key                               \
                          ../update_engine/unittest_key2.pub.pem

    6. Update the DUT - the update should fail at the payload
       verification stage.

    7. Like in step 4., examine the update_engine logs and update the
       following constants:

        _IMAGE_METADATA_SIGNATURE_WITH_KEY2: set to 'MetadataSignatureRsa'

    8. Now run the test and ensure that it passes

      $ cd ~/trunk/src/scripts
      $ test_that -b ${BOARD} --fast <DUT_IP> autoupdate_CatchBadSignatures

    """
    version = 1

    # The test image to use and the values associated with it.
    _IMAGE_GS_URL='gs://chromiumos-test-assets-public/autoupdate/autoupdate_CatchBadSignatures-payload-lumpy-R33-4970.0.2013_11_15_1654-a1'
    _IMAGE_SIZE=369080798
    _IMAGE_METADATA_SIZE=58439
    _IMAGE_SHA256='FeWxyPLdz1/UDHxskkYSkQm64D5kQgaSb1IEXX3/sjA='
    _IMAGE_PUBLIC_KEY1='LS0tLS1CRUdJTiBQVUJMSUMgS0VZLS0tLS0KTUlJQklqQU5CZ2txaGtpRzl3MEJBUUVGQUFPQ0FROEFNSUlCQ2dLQ0FRRUF4NmhxUytIYmM3ak44MmVrK09CawpISk52bFdYa3R5UzJYQ3VRdEd0bkhqRVY3T3U1aEhORjk2czV3RW44UkR0cmRMb2NtMGErU3FYWGY3S3ljRlJUClp4TGREYnFXQU1VbFBYT2FQSStQWkxXa0I3L0tWN0NkajZ2UEdiZXE3ZWx1K2FUL2J1ZGh6VHZxMnN0WXJyQWwKY3IvMjF0T1ZEUGlXdGZDZHlraDdGQ1hpNkZhWUhvTnk1QTZFS1FMZkxCdUpvVS9Rb0N1ZmxkbXdsRmFGREtsKwpLb29zNlIxUVlKZkNOWmZnb2NyVzFQSWgrOHQxSkl2dzZJem84K2ZUbWU3bmV2N09sMllaaU1XSnBSWUt4OE1nCnhXMlVnVFhsUnBtUU41NnBjOUxVc25WQThGTkRCTjU3K2dNSmorWG1kRG1idE1wT3N5WGZTTkVnbnV3TVBjRWMKbXdJREFRQUIKLS0tLS1FTkQgUFVCTElDIEtFWS0tLS0tCg=='
    _IMAGE_METADATA_SIGNATURE_WITH_KEY1='ZA3p2Gfh6qKNqf0as3LeoEou1AsfP73khfk0+hiJ0UFmqTTMj1b8PBeHSHzeRNoJvNZfZBD372PH0BSlKm4BeJ6qyySVDTyC55pKOQyQaC/c5tncvknId2acSEp0XSM5wvkXON0kS9sPfJi7qxDaTJnoCGi6gDKiMjEH3WhsE/1FG5AQ1HPibbeBK3RTtxGmqOIYses+RvJTag7wobdUnXe2Q7l6/c+wCD6m99yXK6l6Vm05gjAR7nhMd4ZyVfN2xaX8KSt9VybO3UuvG9yQUDhxy+ZURY0aaQPLdYcTsuocg/hqDlXctl6WBf6lKogeVqgfaypXqkPlYfgf0tHDGg=='
    _IMAGE_PUBLIC_KEY2='LS0tLS1CRUdJTiBQVUJMSUMgS0VZLS0tLS0KTUlJQklqQU5CZ2txaGtpRzl3MEJBUUVGQUFPQ0FROEFNSUlCQ2dLQ0FRRUFxZE03Z25kNDNjV2ZRenlydDE2UQpESEUrVDB5eGcxOE9aTys5c2M4aldwakMxekZ0b01Gb2tFU2l1OVRMVXArS1VDMjc0ZitEeElnQWZTQ082VTVECkpGUlBYVXp2ZTF2YVhZZnFsalVCeGMrSlljR2RkNlBDVWw0QXA5ZjAyRGhrckduZi9ya0hPQ0VoRk5wbTUzZG8Kdlo5QTZRNUtCZmNnMUhlUTA4OG9wVmNlUUd0VW1MK2JPTnE1dEx2TkZMVVUwUnUwQW00QURKOFhtdzRycHZxdgptWEphRm1WdWYvR3g3K1RPbmFKdlpUZU9POUFKSzZxNlY4RTcrWlppTUljNUY0RU9zNUFYL2xaZk5PM1JWZ0cyCk83RGh6emErbk96SjNaSkdLNVI0V3daZHVobjlRUllvZ1lQQjBjNjI4NzhxWHBmMkJuM05wVVBpOENmL1JMTU0KbVFJREFRQUIKLS0tLS1FTkQgUFVCTElDIEtFWS0tLS0tCg=='
    _IMAGE_METADATA_SIGNATURE_WITH_KEY2='k+mg0w5Jy8DXdF9Vw0MJdJdAj1S4EYR3k9fR4ECZmZplhmzUFvyPWVAHEYDEGLtNBbdaa66+ErOE/clERfxvjkIHbIlTUWDnqgKKPYnZ5dNuEDrHn8875ild9OwBgHEK7NSxaNyRGThLfVCqIUKUzMnjBk/elAiiY0hlLIN9Owitw3f+p9E2chYSdh1dpqlcs14JCULcO/+p+ZfQdeNkN600tS02SGOBwV4W8wXt7EWYdu2awp39z+zDniudFShIpamhhUbddqAn7aZTNE6qGgYVZuWNv3O3kBY4dMb7NsSZInn+fkC39QlXlqoM+ShLVhlpJa/MdOpX7g1UQKLa9A=='

    @staticmethod
    def _string_has_strings(haystack, needles):
        """Returns True iff all the strings in the list |needles| are
        present in the string |haystack|."""
        for n in needles:
            if haystack.find(n) == -1:
                return False
        return True

    def _check_signature(self, metadata_signature, public_key,
                         expected_log_messages, failure_message):
        """Helper function for updating with a Canned Omaha response."""

        # Runs the update on the DUT and expect it to fail.
        client_host = autotest.Autotest(self._host)
        client_host.run_test(
                'autoupdate_CannedOmahaUpdate',
                image_url=self._staged_payload_url,
                image_size=self._IMAGE_SIZE,
                image_sha256=self._IMAGE_SHA256,
                allow_failure=True,
                metadata_size=self._IMAGE_METADATA_SIZE,
                metadata_signature=metadata_signature,
                public_key=public_key)

        cmdresult = self._host.run('cat /var/log/update_engine.log')
        if not self._string_has_strings(cmdresult.stdout,
                                        expected_log_messages):
            raise error.TestFail(failure_message)


    def _check_bad_metadata_signature(self):
        """Checks that update_engine rejects updates where the payload
        and Omaha response do not agree on the metadata signature."""

        expected_log_messages = [
                'Mandating payload hash checks since Omaha Response for '
                'unofficial build includes public RSA key',
                'Mandatory metadata signature validation failed']

        self._check_signature(
                metadata_signature=self._IMAGE_METADATA_SIGNATURE_WITH_KEY1,
                public_key=self._IMAGE_PUBLIC_KEY2,
                expected_log_messages=expected_log_messages,
                failure_message='Check for bad metadata signature failed.')


    def _check_bad_payload_signature(self):
        """Checks that update_engine rejects updates where the payload
        signature does not match what is expected."""

        expected_log_messages = [
                'Mandating payload hash checks since Omaha Response for '
                'unofficial build includes public RSA key',
                'Metadata hash signature matches value in Omaha response.',
                'Public key verification failed, thus update failed']

        self._check_signature(
                metadata_signature=self._IMAGE_METADATA_SIGNATURE_WITH_KEY2,
                public_key=self._IMAGE_PUBLIC_KEY2,
                expected_log_messages=expected_log_messages,
                failure_message='Check for payload signature failed.')


    def _stage_image(self, image_url):
        """Requests an image server from the lab to stage the image
        specified by |image_url| (typically a Google Storage
        URL). Returns the URL to the staged image."""

        # We don't have a build so just fake the string.
        build = 'x86-fake-release/R42-4242.0.0-a1-bFAKE'
        image_server = dev_server.ImageServer.resolve(build)
        archive_url = os.path.dirname(image_url)
        filename = os.path.basename(image_url)
        # ImageServer expects an image parameter, but we don't have one.
        image_server.stage_artifacts(image='fake_image',
                                     files=[filename],
                                     archive_url=archive_url)

        # ImageServer has no way to give us the URL of the staged file...
        base, name = _split_url(image_url)
        staged_url = '%s/static/%s' % (image_server.url(), name)
        return staged_url


    def cleanup(self):
        if self._host:
            self._host.reboot()


    def run_once(self, host):
        """Runs the test on the DUT represented by |host|."""

        self._host = host

        # First, stage the image.
        self._staged_payload_url = self._stage_image(self._IMAGE_GS_URL)

        # Then run the tests.
        self._check_bad_metadata_signature()
        self._check_bad_payload_signature()

