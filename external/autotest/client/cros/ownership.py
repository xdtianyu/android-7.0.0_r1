# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, shutil, tempfile

import common, constants, cryptohome
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import autotemp, error
from autotest_lib.client.cros import cros_ui


PK12UTIL = 'pk12util'
CERTUTIL = 'certutil'
OPENSSLP12 = 'openssl pkcs12'
OPENSSLX509 = 'openssl x509'
OPENSSLRSA = 'openssl rsa'
OPENSSLREQ = 'openssl req'
OPENSSLCRYPTO = 'openssl sha1'

TESTUSER = 'ownership_test@chromium.org'
TESTPASS = 'testme'


class OwnershipError(error.TestError):
    """Generic error for ownership-related failures."""
    pass


class scoped_tempfile(object):
    """A wrapper that provides scoped semantics for temporary files.

    Providing a file path causes the scoped_tempfile to take ownership of the
    file at the provided path.  The file at the path will be deleted when this
    object goes out of scope.  If no path is provided, then a temporary file
    object will be created for the lifetime of the scoped_tempfile

    autotemp.tempfile objects don't seem to play nicely with being
    used in system commands, so they can't be used for my purposes.
    """

    tempdir = autotemp.tempdir(unique_id='ownership')

    def __init__(self, name=None):
        self.name = name
        if not self.name:
            self.fo = tempfile.TemporaryFile()


    def __del__(self):
        if self.name:
            if os.path.exists(self.name):
                os.unlink(self.name)
        else:
            self.fo.close()  # Will destroy the underlying tempfile


def system_output_on_fail(cmd):
    """Run a |cmd|, capturing output and logging it only on error.

    @param cmd: the command to run.
    """
    output = None
    try:
        output = utils.system_output(cmd)
    except:
        logging.error(output)
        raise


def __unlink(filename):
    """unlink a file, but log OSError and IOError instead of raising.

    This allows unlinking files that don't exist safely.

    @param filename: the file to attempt to unlink.
    """
    try:
        os.unlink(filename)
    except (IOError, OSError) as error:
        logging.info(error)


def restart_ui_to_clear_ownership_files():
    """Remove on-disk state related to device ownership.

    The UI must be stopped while we do this, or the session_manager will
    write the policy and key files out again.
    """
    cros_ui.stop(allow_fail=not cros_ui.is_up())
    clear_ownership_files_no_restart()
    cros_ui.start()


def clear_ownership_files_no_restart():
    """Remove on-disk state related to device ownership.

    The UI must be stopped while we do this, or the session_manager will
    write the policy and key files out again.
    """
    if cros_ui.is_up():
        raise error.TestError("Tried to clear ownership with UI running.")
    __unlink(constants.OWNER_KEY_FILE)
    __unlink(constants.SIGNED_POLICY_FILE)
    __unlink(os.path.join(constants.USER_DATA_DIR, 'Local State'))


def fake_ownership():
    """Fake ownership by generating the necessary magic files."""
    # Determine the module directory.
    dirname = os.path.dirname(__file__)
    mock_certfile = os.path.join(dirname, constants.MOCK_OWNER_CERT)
    mock_signedpolicyfile = os.path.join(dirname,
                                         constants.MOCK_OWNER_POLICY)
    utils.open_write_close(constants.OWNER_KEY_FILE,
                           cert_extract_pubkey_der(mock_certfile))
    shutil.copy(mock_signedpolicyfile,
                constants.SIGNED_POLICY_FILE)


POLICY_TYPE = 'google/chromeos/device'


def assert_has_policy_data(response_proto):
    """Assert that given protobuf has a policy_data field.

    @param response_proto: a PolicyFetchResponse protobuf.
    @raises OwnershipError on failure.
    """
    if not response_proto.HasField("policy_data"):
        raise OwnershipError('Malformatted response.')


def assert_has_device_settings(data_proto):
    """Assert that given protobuf is a policy with device settings in it.

    @param data_proto: a PolicyData protobuf.
    @raises OwnershipError if this isn't CrOS policy, or has no settings inside.
    """
    if (not data_proto.HasField("policy_type") or
        data_proto.policy_type != POLICY_TYPE or
        not data_proto.HasField("policy_value")):
        raise OwnershipError('Malformatted response.')


def assert_username(data_proto, username):
    """Assert that given protobuf is a policy associated with the given user.

    @param data_proto: a PolicyData protobuf.
    @param username: the username to check for
    @raises OwnershipError if data_proto isn't associated with username
    """
    if data_proto.username != username:
        raise OwnershipError('Incorrect username.')


def assert_guest_setting(settings, guests):
    """Assert that given protobuf has given guest-related settings.

    @param settings: a ChromeDeviceSettingsProto protobuf.
    @param guests: boolean indicating whether guests are allowed to sign in.
    @raises OwnershipError if settings doesn't enforce the provided setting.
    """
    if not settings.HasField("guest_mode_enabled"):
        raise OwnershipError('No guest mode setting protobuf.')
    if not settings.guest_mode_enabled.HasField("guest_mode_enabled"):
        raise OwnershipError('No guest mode setting.')
    if settings.guest_mode_enabled.guest_mode_enabled != guests:
        raise OwnershipError('Incorrect guest mode setting.')


def assert_show_users(settings, show_users):
    """Assert that given protobuf has given user-avatar-showing settings.

    @param settings: a ChromeDeviceSettingsProto protobuf.
    @param show_users: boolean indicating whether avatars are shown on sign in.
    @raises OwnershipError if settings doesn't enforce the provided setting.
    """
    if not settings.HasField("show_user_names"):
        raise OwnershipError('No show users setting protobuf.')
    if not settings.show_user_names.HasField("show_user_names"):
        raise OwnershipError('No show users setting.')
    if settings.show_user_names.show_user_names != show_users:
        raise OwnershipError('Incorrect show users setting.')


def assert_roaming(settings, roaming):
    """Assert that given protobuf has given roaming settings.

    @param settings: a ChromeDeviceSettingsProto protobuf.
    @param roaming: boolean indicating whether roaming is allowed.
    @raises OwnershipError if settings doesn't enforce the provided setting.
    """
    if not settings.HasField("data_roaming_enabled"):
        raise OwnershipError('No roaming setting protobuf.')
    if not settings.data_roaming_enabled.HasField("data_roaming_enabled"):
        raise OwnershipError('No roaming setting.')
    if settings.data_roaming_enabled.data_roaming_enabled != roaming:
        raise OwnershipError('Incorrect roaming setting.')


def assert_new_users(settings, new_users):
    """Assert that given protobuf has given new user settings.

    @param settings: a ChromeDeviceSettingsProto protobuf.
    @param new_users: boolean indicating whether adding users is allowed.
    @raises OwnershipError if settings doesn't enforce the provided setting.
    """
    if not settings.HasField("allow_new_users"):
        raise OwnershipError('No allow new users setting protobuf.')
    if not settings.allow_new_users.HasField("allow_new_users"):
        raise OwnershipError('No allow new users setting.')
    if settings.allow_new_users.allow_new_users != new_users:
        raise OwnershipError('Incorrect allow new users setting.')


def assert_users_on_whitelist(settings, users):
    """Assert that given protobuf has given users on the whitelist.

    @param settings: a ChromeDeviceSettingsProto protobuf.
    @param users: iterable containing usernames that should be on whitelist.
    @raises OwnershipError if settings doesn't enforce the provided setting.
    """
    if settings.HasField("user_whitelist"):
        for user in users:
            if user not in settings.user_whitelist.user_whitelist:
                raise OwnershipError(user + ' not whitelisted.')
    else:
        raise OwnershipError('No user whitelist.')


def assert_proxy_settings(settings, proxies):
    """Assert that given protobuf has given proxy settings.

    @param settings: a ChromeDeviceSettingsProto protobuf.
    @param proxies: dict { 'proxy_mode': <mode string> }
    @raises OwnershipError if settings doesn't enforce the provided setting.
    """
    if not settings.HasField("device_proxy_settings"):
        raise OwnershipError('No proxy settings protobuf.')
    if not settings.device_proxy_settings.HasField("proxy_mode"):
        raise OwnershipError('No proxy_mode setting.')
    if settings.device_proxy_settings.proxy_mode != proxies['proxy_mode']:
        raise OwnershipError('Incorrect proxies: %s' % proxies)


def __user_nssdb(user):
    """Returns the path to the NSSDB for the provided user.

    @param user: the user whose NSSDB the caller wants.
    @return: absolute path to user's NSSDB.
    """
    return os.path.join(cryptohome.user_path(user), '.pki', 'nssdb')


def use_known_ownerkeys(user):
    """Sets the system up to use a well-known keypair for owner operations.

    Assuming the appropriate cryptohome is already mounted, configures the
    device to accept policies signed with the checked-in 'mock' owner key.

    @param user: the user whose NSSDB should be populated with key material.
    """
    dirname = os.path.dirname(__file__)
    mock_keyfile = os.path.join(dirname, constants.MOCK_OWNER_KEY)
    mock_certfile = os.path.join(dirname, constants.MOCK_OWNER_CERT)
    push_to_nss(mock_keyfile, mock_certfile, __user_nssdb(user))
    utils.open_write_close(constants.OWNER_KEY_FILE,
                           cert_extract_pubkey_der(mock_certfile))


def known_privkey():
    """Returns the mock owner private key in PEM format.

    @return: mock owner private key in PEM format.
    """
    dirname = os.path.dirname(__file__)
    return utils.read_file(os.path.join(dirname, constants.MOCK_OWNER_KEY))


def known_pubkey():
    """Returns the mock owner public key in DER format.

    @return: mock owner public key in DER format.
    """
    dirname = os.path.dirname(__file__)
    return cert_extract_pubkey_der(os.path.join(dirname,
                                                constants.MOCK_OWNER_CERT))


def pairgen():
    """Generate a self-signed cert and associated private key.

    Generates a self-signed X509 certificate and the associated private key.
    The key is 2048 bits.  The generated material is stored in PEM format
    and the paths to the two files are returned.

    The caller is responsible for cleaning up these files.

    @return: (/path/to/private_key, /path/to/self-signed_cert)
    """
    keyfile = scoped_tempfile.tempdir.name + '/private.key'
    certfile = scoped_tempfile.tempdir.name + '/cert.pem'
    cmd = '%s -x509 -subj %s -newkey rsa:2048 -nodes -keyout %s -out %s' % (
        OPENSSLREQ, '/CN=me', keyfile, certfile)
    system_output_on_fail(cmd)
    return (keyfile, certfile)


def pairgen_as_data():
    """Generates keypair, returns keys as data.

    Generates a fresh owner keypair and then passes back the
    PEM-encoded private key and the DER-encoded public key.

    @return: (PEM-encoded private key, DER-encoded public key)
    """
    (keypath, certpath) = pairgen()
    keyfile = scoped_tempfile(keypath)
    certfile = scoped_tempfile(certpath)
    return (utils.read_file(keyfile.name),
            cert_extract_pubkey_der(certfile.name))


def push_to_nss(keyfile, certfile, nssdb):
    """Takes a pre-generated key pair and pushes them to an NSS DB.

    Given paths to a private key and cert in PEM format, stores the pair
    in the provided nssdb.

    @param keyfile: path to PEM-formatted private key file.
    @param certfile: path to PEM-formatted cert file for associated public key.
    @param nssdb: path to NSSDB to be populated with the provided keys.
    """
    for_push = scoped_tempfile(scoped_tempfile.tempdir.name + '/for_push.p12')
    cmd = '%s -export -in %s -inkey %s -out %s ' % (
        OPENSSLP12, certfile, keyfile, for_push.name)
    cmd += '-passin pass: -passout pass:'
    system_output_on_fail(cmd)
    cmd = '%s -d "sql:%s" -i %s -W ""' % (PK12UTIL,
                                          nssdb,
                                          for_push.name)
    system_output_on_fail(cmd)


def cert_extract_pubkey_der(pem):
    """Given a PEM-formatted cert, extracts the public key in DER format.

    Pass in an X509 certificate in PEM format, and you'll get back the
    DER-formatted public key as a string.

    @param pem: path to a PEM-formatted cert file.
    @return: DER-encoded public key from cert, as a string.
    """
    outfile = scoped_tempfile(scoped_tempfile.tempdir.name + '/pubkey.der')
    cmd = '%s -in %s -pubkey -noout ' % (OPENSSLX509, pem)
    cmd += '| %s -outform DER -pubin -out %s' % (OPENSSLRSA,
                                                 outfile.name)
    system_output_on_fail(cmd)
    der = utils.read_file(outfile.name)
    return der


def sign(pem_key, data):
    """Signs |data| with key from |pem_key|, returns signature.

    Using the PEM-formatted private key in |pem_key|, generates an
    RSA-with-SHA1 signature over |data| and returns the signature in
    a string.

    @param pem_key: PEM-formatted private key, as a string.
    @param data: data to be signed.
    @return: signature as a string.
    """
    sig = scoped_tempfile()
    err = scoped_tempfile()
    data_file = scoped_tempfile()
    data_file.fo.write(data)
    data_file.fo.seek(0)

    pem_key_file = scoped_tempfile(scoped_tempfile.tempdir.name + '/pkey.pem')
    utils.open_write_close(pem_key_file.name, pem_key)

    cmd = '%s -sign %s' % (OPENSSLCRYPTO, pem_key_file.name)
    try:
        utils.run(cmd,
                  stdin=data_file.fo,
                  stdout_tee=sig.fo,
                  stderr_tee=err.fo)
    except:
        err.fo.seek(0)
        logging.error(err.fo.read())
        raise

    sig.fo.seek(0)
    sig_data = sig.fo.read()
    if not sig_data:
        raise error.OwnershipError('Empty signature!')
    return sig_data


def get_user_policy_key_filename(username):
    """Returns the path to the user policy key for the given username.

    @param username: the user whose policy key we want the path to.
    @return: absolute path to user's policy key file.
    """
    return os.path.join(constants.USER_POLICY_DIR,
                        cryptohome.get_user_hash(username),
                        constants.USER_POLICY_KEY_FILENAME)
