# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility functions used for PKCS#11 library testing."""

import grp, logging, os, pwd, re, stat, sys, shutil, pwd, grp

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

USER_TOKEN_PREFIX = 'User TPM Token '
TMP_CHAPS_DIR = '/tmp/chaps'
CHAPS_DIR_PERM = 0750
SYSTEM_TOKEN_NAME = 'System TPM Token'
SYSTEM_TOKEN_DIR = '/var/lib/chaps'
INVALID_SLOT_ID = '100'


def __run_cmd(cmd, ignore_status=False):
    """Runs a command and returns the output from both stdout and stderr."""
    return utils.system_output(cmd + ' 2>&1', retain_output=True,
                               ignore_status=ignore_status).strip()

def __get_token_paths(exclude_system_token):
    """Return a list with a path for each PKCS #11 token currently loaded."""
    token_paths = []
    for line in __run_cmd('chaps_client --list').split('\n'):
        match = re.search(r'Slot \d+: (/.*)$', line)
        if match:
            if exclude_system_token and match.group(1) == SYSTEM_TOKEN_DIR:
                continue
            token_paths.append(match.group(1))
    return token_paths

def __get_pkcs11_file_list(token_path):
    """Return string with PKCS#11 file paths and their associated metadata."""
    find_args = '-printf "\'%p\', \'%u:%g\', 0%m\n"'
    file_list_output = __run_cmd('find %s ' % token_path + find_args)
    return file_list_output

def __get_token_slot_by_path(token_path):
    token_list = __run_cmd('p11_replay --list_tokens')
    for line in token_list.split('\n'):
        match = re.search(r'^Slot (\d+): ' + token_path, line)
        if not match:
            continue
        return match.group(1)
    return INVALID_SLOT_ID

def __verify_tokenname(token_path):
    """Verify that the TPM token name is correct."""
    # The token path is expected to be of the form:
    # /home/root/<obfuscated_user_id>/chaps
    match = re.search(r'/home/root/(.*)/chaps', token_path)
    if not match:
        return False
    obfuscated_user = match.group(1)
    # We expect the token label to contain first 16 characters of the obfuscated
    # user id. This is the same value we extracted from |token_path|.
    expected_user_token_label = USER_TOKEN_PREFIX + obfuscated_user[:16]
    # The p11_replay tool will list tokens in the following form:
    # Slot 1: <token label>
    token_list = __run_cmd('p11_replay --list_tokens')
    for line in token_list.split('\n'):
        match = re.search(r'^Slot \d+: (.*)$', line)
        if not match:
            continue
        token_label = match.group(1).rstrip()
        if (token_label == expected_user_token_label):
            return True
        # Ignore the system token label.
        if token_label == SYSTEM_TOKEN_NAME:
            continue
        logging.error('Unexpected token label: |%s|', token_label)
    logging.error('Invalid or missing PKCS#11 token label!')
    return False

def __verify_permissions(token_path):
    """Verify that the permissions on the initialized token dir are correct."""
    # List of 3-tuples consisting of (path, user:group, octal permissions).
    # Can be generated (for example), by:
    # find <token_path>/chaps -printf "'%p', '%u:%g', 0%m\n"
    expected_permissions = [
        (token_path, 'chaps:chronos-access', CHAPS_DIR_PERM),
        ('%s/database' % token_path, 'chaps:chronos-access', CHAPS_DIR_PERM)]
    for item in expected_permissions:
        path = item[0]
        (user, group) = item[1].split(':')
        perms = item[2]
        stat_buf = os.lstat(path)
        if not stat_buf:
            logging.error('Could not stat %s while checking for permissions.',
                          path)
            return False
        # Check ownership.
        path_user = pwd.getpwuid(stat_buf.st_uid).pw_name
        path_group = grp.getgrgid(stat_buf.st_gid).gr_name
        if path_user != user or path_group != group:
            logging.error('Ownership of %s does not match! Got = (%s, %s)'
                          ', Expected = (%s, %s)', path, path_user, path_group,
                          user, group)
            return False

        # Check permissions.
        path_perms = stat.S_IMODE(stat_buf.st_mode)
        if path_perms != perms:
            logging.error('Permissions for %s do not match! (Got = %s'
                          ', Expected = %s)', path, oct(path_perms), oct(perms))
            return False

    return True

def verify_pkcs11_initialized():
    """Checks if the PKCS#11 token is initialized properly."""
    token_path_list = __get_token_paths(exclude_system_token=True)
    if len(token_path_list) != 1:
        logging.error('Expecting a single signed-in user with a token.')
        return False

    verify_cmd = ('cryptohome --action=pkcs11_token_status')
    __run_cmd(verify_cmd)

    verify_result = True
    # Do additional sanity tests.
    if not __verify_tokenname(token_path_list[0]):
        logging.error('Verification of token name failed!')
        verify_result = False
    if not __verify_permissions(token_path_list[0]):
        logging.error('PKCS#11 file list:\n%s',
                      __get_pkcs11_file_list(token_path_list[0]))
        logging.error(
            'Verification of PKCS#11 subsystem and token permissions failed!')
        verify_result = False
    return verify_result

def load_p11_test_token(auth_data='1234'):
    """Loads the test token onto a slot.

    @param auth_data: The authorization data to use for the token.
    """
    utils.system('sudo chaps_client --load --path=%s --auth="%s"' %
                 (TMP_CHAPS_DIR, auth_data))

def change_p11_test_token_auth_data(auth_data, new_auth_data):
    """Changes authorization data for the test token.

    @param auth_data: The current authorization data.
    @param new_auth_data: The new authorization data.
    """
    utils.system('sudo chaps_client --change_auth --path=%s --auth="%s" '
                 '--new_auth="%s"' % (TMP_CHAPS_DIR, auth_data, new_auth_data))

def unload_p11_test_token():
    """Unloads a loaded test token."""
    utils.system('sudo chaps_client --unload --path=%s' % TMP_CHAPS_DIR)

def copytree_with_ownership(src, dst):
    """Like shutil.copytree but also copies owner and group attributes.
    @param src: Source directory.
    @param dst: Destination directory.
    """
    utils.system('cp -rp %s %s' % (src, dst))

def setup_p11_test_token(unload_user_tokens, auth_data='1234'):
    """Configures a PKCS #11 token for testing.

    Any existing test token will be automatically cleaned up.

    @param unload_user_tokens: Whether to unload all user tokens.
    @param auth_data: Initial token authorization data.
    """
    cleanup_p11_test_token()
    if unload_user_tokens:
        for path in __get_token_paths(exclude_system_token=False):
            utils.system('sudo chaps_client --unload --path=%s' % path)
    os.makedirs(TMP_CHAPS_DIR)
    uid = pwd.getpwnam('chaps')[2]
    gid = grp.getgrnam('chronos-access')[2]
    os.chown(TMP_CHAPS_DIR, uid, gid)
    os.chmod(TMP_CHAPS_DIR, CHAPS_DIR_PERM)
    load_p11_test_token(auth_data)
    unload_p11_test_token()
    copytree_with_ownership(TMP_CHAPS_DIR, '%s_bak' % TMP_CHAPS_DIR)

def restore_p11_test_token():
    """Restores a PKCS #11 test token to its initial state."""
    shutil.rmtree(TMP_CHAPS_DIR)
    copytree_with_ownership('%s_bak' % TMP_CHAPS_DIR, TMP_CHAPS_DIR)

def get_p11_test_token_db_path():
    """Returns the test token database path."""
    return '%s/database' % TMP_CHAPS_DIR

def verify_p11_test_token():
    """Verifies that a test token is working and persistent."""
    output = __run_cmd('p11_replay --generate --replay_wifi',
                       ignore_status=True)
    if not re.search('Sign: CKR_OK', output):
        print >> sys.stderr, output
        return False
    unload_p11_test_token()
    load_p11_test_token()
    output = __run_cmd('p11_replay --replay_wifi --cleanup',
                       ignore_status=True)
    if not re.search('Sign: CKR_OK', output):
        print >> sys.stderr, output
        return False
    return True

def cleanup_p11_test_token():
    """Deletes the test token."""
    unload_p11_test_token()
    shutil.rmtree(TMP_CHAPS_DIR, ignore_errors=True)
    shutil.rmtree('%s_bak' % TMP_CHAPS_DIR, ignore_errors=True)

def wait_for_pkcs11_token():
    """Waits for the PKCS #11 token to be available.

    This should be called only after a login and is typically called immediately
    after a login.

    Returns:
        True if the token is available.
    """
    try:
        utils.poll_for_condition(
            lambda: utils.system('cryptohome --action=pkcs11_token_status',
                                 ignore_status=True) == 0,
            desc='PKCS #11 token.',
            timeout=300)
    except utils.TimeoutError:
        return False
    return True

def __p11_replay_on_user_token(extra_args=''):
    """Executes a typical command replay on the current user token.

    Args:
        extra_args: Additional arguments to pass to p11_replay.

    Returns:
        The command output.
    """
    if not wait_for_pkcs11_token():
       raise error.TestError('Timeout while waiting for pkcs11 token')
    return __run_cmd('p11_replay --slot=%s %s'
                     % (__get_token_slot_by_path(USER_TOKEN_PREFIX),
                        extra_args),
                     ignore_status=True)

def inject_and_test_key():
    """Injects a key into a PKCS #11 token and tests that it can sign."""
    output = __p11_replay_on_user_token('--replay_wifi --inject')
    return re.search('Sign: CKR_OK', output)

def test_and_cleanup_key():
    """Tests a PKCS #11 key before deleting it."""
    output = __p11_replay_on_user_token('--replay_wifi --cleanup')
    return re.search('Sign: CKR_OK', output)

def generate_user_key():
    """Generates a key in the current user token."""
    output = __p11_replay_on_user_token('--generate')
    return re.search('Sign: CKR_OK', output)

