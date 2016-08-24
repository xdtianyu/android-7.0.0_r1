# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus, gobject, logging, os, random, re, shutil, string
from dbus.mainloop.glib import DBusGMainLoop

import common, constants
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cros_disks import DBusClient

CRYPTOHOME_CMD = '/usr/sbin/cryptohome'
GUEST_USER_NAME = '$guest'
UNAVAILABLE_ACTION = 'Unknown action or no action given.'

class ChromiumOSError(error.TestError):
    """Generic error for ChromiumOS-specific exceptions."""
    pass

def __run_cmd(cmd):
    return utils.system_output(cmd + ' 2>&1', retain_output=True,
                               ignore_status=True).strip()

def get_user_hash(user):
    """Get the user hash for the given user."""
    return utils.system_output(['cryptohome', '--action=obfuscate_user',
                                '--user=%s' % user])


def user_path(user):
    """Get the user mount point for the given user."""
    return utils.system_output(['cryptohome-path', 'user', user])


def system_path(user):
    """Get the system mount point for the given user."""
    return utils.system_output(['cryptohome-path', 'system', user])


def ensure_clean_cryptohome_for(user, password=None):
    """Ensure a fresh cryptohome exists for user.

    @param user: user who needs a shiny new cryptohome.
    @param password: if unset, a random password will be used.
    """
    if not password:
        password = ''.join(random.sample(string.ascii_lowercase, 6))
    remove_vault(user)
    mount_vault(user, password, create=True)


def get_tpm_status():
    """Get the TPM status.

    Returns:
        A TPM status dictionary, for example:
        { 'Enabled': True,
          'Owned': True,
          'Being Owned': False,
          'Ready': True,
          'Password': ''
        }
    """
    out = __run_cmd(CRYPTOHOME_CMD + ' --action=tpm_status')
    status = {}
    for field in ['Enabled', 'Owned', 'Being Owned', 'Ready']:
        match = re.search('TPM %s: (true|false)' % field, out)
        if not match:
            raise ChromiumOSError('Invalid TPM status: "%s".' % out)
        status[field] = match.group(1) == 'true'
    match = re.search('TPM Password: (\w*)', out)
    status['Password'] = ''
    if match:
        status['Password'] = match.group(1)
    return status


def get_tpm_more_status():
    """Get more of the TPM status.

    Returns:
        A TPM more status dictionary, for example:
        { 'dictionary_attack_lockout_in_effect': False,
          'attestation_prepared': False,
          'boot_lockbox_finalized': False,
          'enabled': True,
          'owned': True,
          'owner_password': ''
          'dictionary_attack_counter': 0,
          'dictionary_attack_lockout_seconds_remaining': 0,
          'dictionary_attack_threshold': 10,
          'attestation_enrolled': False,
          'initialized': True,
          'verified_boot_measured': False,
          'install_lockbox_finalized': True
        }
        An empty dictionary is returned if the command is not supported.
    """
    status = {}
    out = __run_cmd(CRYPTOHOME_CMD + ' --action=tpm_more_status | grep :')
    if out.startswith(UNAVAILABLE_ACTION):
        # --action=tpm_more_status only exists >= 41.
        logging.info('Method not supported!')
        return status
    for line in out.splitlines():
        items = line.strip().split(':')
        if items[1].strip() == 'false':
            value = False
        elif items[1].strip() == 'true':
            value = True
        elif items[1].strip().isdigit():
            value = int(items[1].strip())
        else:
            value = items[1].strip(' "')
        status[items[0]] = value
    return status


def is_tpm_lockout_in_effect():
    """Returns true if the TPM lockout is in effect; false otherwise."""
    status = get_tpm_more_status()
    return status.get('dictionary_attack_lockout_in_effect', None)


def get_login_status():
    """Query the login status

    Returns:
        A login status dictionary containing:
        { 'owner_user_exists': True|False,
          'boot_lockbox_finalized': True|False
        }
    """
    out = __run_cmd(CRYPTOHOME_CMD + ' --action=get_login_status')
    status = {}
    for field in ['owner_user_exists', 'boot_lockbox_finalized']:
        match = re.search('%s: (true|false)' % field, out)
        if not match:
            raise ChromiumOSError('Invalid login status: "%s".' % out)
        status[field] = match.group(1) == 'true'
    return status


def get_tpm_attestation_status():
    """Get the TPM attestation status.  Works similar to get_tpm_status().
    """
    out = __run_cmd(CRYPTOHOME_CMD + ' --action=tpm_attestation_status')
    status = {}
    for field in ['Prepared', 'Enrolled']:
        match = re.search('Attestation %s: (true|false)' % field, out)
        if not match:
            raise ChromiumOSError('Invalid attestation status: "%s".' % out)
        status[field] = match.group(1) == 'true'
    return status


def take_tpm_ownership():
    """Take TPM owernship.

    Blocks until TPM is owned.
    """
    __run_cmd(CRYPTOHOME_CMD + ' --action=tpm_take_ownership')
    __run_cmd(CRYPTOHOME_CMD + ' --action=tpm_wait_ownership')


def verify_ek():
    """Verify the TPM endorsement key.

    Returns true if EK is valid.
    """
    cmd = CRYPTOHOME_CMD + ' --action=tpm_verify_ek'
    return (utils.system(cmd, ignore_status=True) == 0)


def remove_vault(user):
    """Remove the given user's vault from the shadow directory."""
    logging.debug('user is %s', user)
    user_hash = get_user_hash(user)
    logging.debug('Removing vault for user %s with hash %s' % (user, user_hash))
    cmd = CRYPTOHOME_CMD + ' --action=remove --force --user=%s' % user
    __run_cmd(cmd)
    # Ensure that the vault does not exist.
    if os.path.exists(os.path.join(constants.SHADOW_ROOT, user_hash)):
        raise ChromiumOSError('Cryptohome could not remove the user\'s vault.')


def remove_all_vaults():
    """Remove any existing vaults from the shadow directory.

    This function must be run with root privileges.
    """
    for item in os.listdir(constants.SHADOW_ROOT):
        abs_item = os.path.join(constants.SHADOW_ROOT, item)
        if os.path.isdir(os.path.join(abs_item, 'vault')):
            logging.debug('Removing vault for user with hash %s' % item)
            shutil.rmtree(abs_item)


def mount_vault(user, password, create=False):
    """Mount the given user's vault."""
    args = [CRYPTOHOME_CMD, '--action=mount', '--user=%s' % user,
            '--password=%s' % password, '--async']
    if create:
        args.append('--create')
    logging.info(__run_cmd(' '.join(args)))
    # Ensure that the vault exists in the shadow directory.
    user_hash = get_user_hash(user)
    if not os.path.exists(os.path.join(constants.SHADOW_ROOT, user_hash)):
        raise ChromiumOSError('Cryptohome vault not found after mount.')
    # Ensure that the vault is mounted.
    if not is_vault_mounted(
            user=user,
            device_regex=constants.CRYPTOHOME_DEV_REGEX_REGULAR_USER,
            allow_fail=True):
        raise ChromiumOSError('Cryptohome created a vault but did not mount.')


def mount_guest():
    """Mount the given user's vault."""
    args = [CRYPTOHOME_CMD, '--action=mount_guest', '--async']
    logging.info(__run_cmd(' '.join(args)))
    # Ensure that the guest tmpfs is mounted.
    if not is_guest_vault_mounted(allow_fail=True):
        raise ChromiumOSError('Cryptohome did not mount tmpfs.')


def test_auth(user, password):
    cmd = [CRYPTOHOME_CMD, '--action=test_auth', '--user=%s' % user,
           '--password=%s' % password, '--async']
    return 'Authentication succeeded' in utils.system_output(cmd)


def unmount_vault(user):
    """Unmount the given user's vault.

    Once unmounting for a specific user is supported, the user parameter will
    name the target user. See crosbug.com/20778.
    """
    __run_cmd(CRYPTOHOME_CMD + ' --action=unmount')
    # Ensure that the vault is not mounted.
    if is_vault_mounted(user, allow_fail=True):
        raise ChromiumOSError('Cryptohome did not unmount the user.')


def __get_mount_info(mount_point, allow_fail=False):
    """Get information about the active mount at a given mount point."""
    cryptohomed_path = '/proc/$(pgrep cryptohomed)/mounts'
    try:
        logging.info(utils.system_output('cat %s' % cryptohomed_path))
        mount_line = utils.system_output(
            'grep %s %s' % (mount_point, cryptohomed_path),
            ignore_status=allow_fail)
    except Exception as e:
        logging.error(e)
        raise ChromiumOSError('Could not get info about cryptohome vault '
                              'through %s. See logs for complete mount-point.'
                              % os.path.dirname(str(mount_point)))
    return mount_line.split()


def __get_user_mount_info(user, allow_fail=False):
    """Get information about the active mounts for a given user.

    Returns the active mounts at the user's user and system mount points. If no
    user is given, the active mount at the shared mount point is returned
    (regular users have a bind-mount at this mount point for backwards
    compatibility; the guest user has a mount at this mount point only).
    """
    return [__get_mount_info(mount_point=user_path(user),
                             allow_fail=allow_fail),
            __get_mount_info(mount_point=system_path(user),
                             allow_fail=allow_fail)]

def is_vault_mounted(
        user,
        device_regex=constants.CRYPTOHOME_DEV_REGEX_ANY,
        fs_regex=constants.CRYPTOHOME_FS_REGEX_ANY,
        allow_fail=False):
    """Check whether a vault is mounted for the given user.

    If no user is given, the shared mount point is checked, determining whether
    a vault is mounted for any user.
    """
    user_mount_info = __get_user_mount_info(user=user, allow_fail=allow_fail)
    for mount_info in user_mount_info:
        if (len(mount_info) < 3 or
                not re.match(device_regex, mount_info[0]) or
                not re.match(fs_regex, mount_info[2])):
            return False
    return True


def is_guest_vault_mounted(allow_fail=False):
    """Check whether a vault backed by tmpfs is mounted for the guest user."""
    return is_vault_mounted(
        user=GUEST_USER_NAME,
        device_regex=constants.CRYPTOHOME_DEV_REGEX_GUEST,
        fs_regex=constants.CRYPTOHOME_FS_REGEX_TMPFS,
        allow_fail=allow_fail)


def get_mounted_vault_devices(user, allow_fail=False):
    """Get the device(s) backing the vault mounted for the given user.

    Returns the devices mounted at the user's user and system mount points. If
    no user is given, the device mounted at the shared mount point is returned.
    """
    return [mount_info[0]
            for mount_info
            in __get_user_mount_info(user=user, allow_fail=allow_fail)
            if len(mount_info)]


def canonicalize(credential):
    """Perform basic canonicalization of |email_address|.

    Perform basic canonicalization of |email_address|, taking into account that
    gmail does not consider '.' or caps inside a username to matter. It also
    ignores everything after a '+'. For example,
    c.masone+abc@gmail.com == cMaSone@gmail.com, per
    http://mail.google.com/support/bin/answer.py?hl=en&ctx=mail&answer=10313
    """
    if not credential:
      return None

    parts = credential.split('@')
    if len(parts) != 2:
        raise error.TestError('Malformed email: ' + credential)

    (name, domain) = parts
    name = name.partition('+')[0]
    if (domain == constants.SPECIAL_CASE_DOMAIN):
        name = name.replace('.', '')
    return '@'.join([name, domain]).lower()


def crash_cryptohomed():
    # Try to kill cryptohomed so we get something to work with.
    pid = __run_cmd('pgrep cryptohomed')
    try:
        pid = int(pid)
    except ValueError, e:  # empty or invalid string
        raise error.TestError('Cryptohomed was not running')
    utils.system('kill -ABRT %d' % pid)
    # CONT just in case cryptohomed had a spurious STOP.
    utils.system('kill -CONT %d' % pid)
    utils.poll_for_condition(
        lambda: utils.system('ps -p %d' % pid,
                             ignore_status=True) != 0,
            timeout=180,
            exception=error.TestError(
                'Timeout waiting for cryptohomed to coredump'))


class CryptohomeProxy(DBusClient):
    """A DBus proxy client for testing the Cryptohome DBus server.
    """
    CRYPTOHOME_BUS_NAME = 'org.chromium.Cryptohome'
    CRYPTOHOME_OBJECT_PATH = '/org/chromium/Cryptohome'
    CRYPTOHOME_INTERFACE = 'org.chromium.CryptohomeInterface'
    ASYNC_CALL_STATUS_SIGNAL = 'AsyncCallStatus'
    ASYNC_CALL_STATUS_SIGNAL_ARGUMENTS = (
        'async_id', 'return_status', 'return_code'
    )
    DBUS_PROPERTIES_INTERFACE = 'org.freedesktop.DBus.Properties'


    def __init__(self, bus_loop=None):
        self.main_loop = gobject.MainLoop()
        if bus_loop is None:
            bus_loop = DBusGMainLoop(set_as_default=True)
        self.bus = dbus.SystemBus(mainloop=bus_loop)
        super(CryptohomeProxy, self).__init__(self.main_loop, self.bus,
                                              self.CRYPTOHOME_BUS_NAME,
                                              self.CRYPTOHOME_OBJECT_PATH)
        self.iface = dbus.Interface(self.proxy_object,
                                    self.CRYPTOHOME_INTERFACE)
        self.properties = dbus.Interface(self.proxy_object,
                                         self.DBUS_PROPERTIES_INTERFACE)
        self.handle_signal(self.CRYPTOHOME_INTERFACE,
                           self.ASYNC_CALL_STATUS_SIGNAL,
                           self.ASYNC_CALL_STATUS_SIGNAL_ARGUMENTS)


    # Wrap all proxied calls to catch cryptohomed failures.
    def __call(self, method, *args):
        try:
            return method(*args, timeout=180)
        except dbus.exceptions.DBusException, e:
            if e.get_dbus_name() == 'org.freedesktop.DBus.Error.NoReply':
                logging.error('Cryptohome is not responding. Sending ABRT')
                crash_cryptohomed()
                raise ChromiumOSError('cryptohomed aborted. Check crashes!')
            raise e


    def __wait_for_specific_signal(self, signal, data):
      """Wait for the |signal| with matching |data|
         Returns the resulting dict on success or {} on error.
      """
      # Do not bubble up the timeout here, just return {}.
      result = {}
      try:
          result = self.wait_for_signal(signal)
      except utils.TimeoutError:
          return {}
      for k in data.keys():
          if not result.has_key(k) or result[k] != data[k]:
            return {}
      return result


    # Perform a data-less async call.
    # TODO(wad) Add __async_data_call.
    def __async_call(self, method, *args):
        # Clear out any superfluous async call signals.
        self.clear_signal_content(self.ASYNC_CALL_STATUS_SIGNAL)
        out = self.__call(method, *args)
        logging.debug('Issued call ' + str(method) +
                      ' with async_id ' + str(out))
        result = {}
        try:
            # __wait_for_specific_signal has a 10s timeout
            result = utils.poll_for_condition(
                lambda: self.__wait_for_specific_signal(
                    self.ASYNC_CALL_STATUS_SIGNAL, {'async_id' : out}),
                timeout=180,
                desc='matching %s signal' % self.ASYNC_CALL_STATUS_SIGNAL)
        except utils.TimeoutError, e:
            logging.error('Cryptohome timed out. Sending ABRT.')
            crash_cryptohomed()
            raise ChromiumOSError('cryptohomed aborted. Check crashes!')
        return result


    def mount(self, user, password, create=False, async=True):
        """Mounts a cryptohome.

        Returns True if the mount succeeds or False otherwise.
        TODO(ellyjones): Migrate mount_vault() to use a multi-user-safe
        heuristic, then remove this method. See <crosbug.com/20778>.
        """
        if async:
            return self.__async_call(self.iface.AsyncMount, user, password,
                                     create, False, [])['return_status']
        out = self.__call(self.iface.Mount, user, password, create, False, [])
        # Sync returns (return code, return status)
        return out[1] if len(out) > 1 else False


    def unmount(self, user):
        """Unmounts a cryptohome.

        Returns True if the unmount suceeds or false otherwise.
        TODO(ellyjones): Once there's a per-user unmount method, use it. See
        <crosbug.com/20778>.
        """
        return self.__call(self.iface.Unmount)


    def is_mounted(self, user):
        """Tests whether a user's cryptohome is mounted."""
        return (utils.is_mountpoint(user_path(user))
                and utils.is_mountpoint(system_path(user)))


    def require_mounted(self, user):
        """Raises a test failure if a user's cryptohome is not mounted."""
        utils.require_mountpoint(user_path(user))
        utils.require_mountpoint(system_path(user))


    def migrate(self, user, oldkey, newkey, async=True):
        """Migrates the specified user's cryptohome from one key to another."""
        if async:
            return self.__async_call(self.iface.AsyncMigrateKey,
                                     user, oldkey, newkey)['return_status']
        return self.__call(self.iface.MigrateKey, user, oldkey, newkey)


    def remove(self, user, async=True):
        if async:
            return self.__async_call(self.iface.AsyncRemove,
                                     user)['return_status']
        return self.__call(self.iface.Remove, user)


    def ensure_clean_cryptohome_for(self, user, password=None):
        """Ensure a fresh cryptohome exists for user.

        @param user: user who needs a shiny new cryptohome.
        @param password: if unset, a random password will be used.
        """
        if not password:
            password = ''.join(random.sample(string.ascii_lowercase, 6))
        self.remove(user)
        self.mount(user, password, create=True)
