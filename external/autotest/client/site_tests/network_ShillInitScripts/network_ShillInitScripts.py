import grp
import mock_flimflam
import os
import pwd
import stat
import time
import utils

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class network_ShillInitScripts(test.test):
    """ Test that shill init scripts perform as expected.  Use the
        real filesystem (doing a best effort to archive and restore
        current state).  The shill manager is stopped and a proxy
        DBus entity is installed to accept DBus messages that are sent
        via "dbus-send" in the shill startup scripts.  However, the
        "real" shill is still also started from time to time and we
        check that it is run with the right command line arguments.
    """
    version = 1
    save_directories = [ '/var/cache/shill',
                         '/var/cache/flimflam',
                         '/var/run/shill',
                         '/var/run/state/logged-in',
                         '/var/run/dhcpcd',
                         '/var/lib/dhcpcd',
                         '/home/chronos/.disable_shill' ]
    fake_user = 'not-a-real-user@chromium.org'
    saved_config = '/tmp/network_ShillInitScripts_saved_config.tgz'
    cryptohome_path_command = 'cryptohome-path'
    guest_shill_user_profile_dir = '/var/run/shill/guest_user_profile/shill'
    guest_shill_user_log_dir = '/var/run/shill/guest_user_profile/shill_logs'
    magic_header = '# --- shill init file test magic header ---'


    def start_shill(self):
        """ Starts a shill instance. """
        utils.system('start shill')


    def stop_shill(self):
        """ Halt the running shill instance. """
        utils.system('stop shill', ignore_status=True)

        for attempt in range(10):
            if not self.find_pid('shill'):
                break
            time.sleep(1)
        else:
            raise error.TestFail('Shill process does not appear to be dying')


    def login(self, user=None):
        """ Simulate the login process.

        Note: "start" blocks until the "script" block completes.

        @param user string user name (email address) to log in.

        """
        utils.system('start shill-start-user-session CHROMEOS_USER=%s' %
                     (user or self.fake_user))


    def login_guest(self):
        """ Simulate guest login.

        For guest login, session-manager passes an empty CHROMEOS_USER arg.

        """
        self.login('""')


    def logout(self):
        """ Simulate user logout.

        Note: "start" blocks until the "script" block completes.

        """
        utils.system('start shill-stop-user-session')


    def start_test(self):
        """ Setup the start of the test.  Stop shill and create test harness."""
        # Stop a system process on test duts for keeping connectivity up.
        ret = utils.system('stop recover_duts', ignore_status=True)
        self.recover_duts_stopped = (ret == 0);

        self.stop_shill()

        # Deduce the root cryptohome directory name for our fake user.
        self.root_cryptohome_dir = utils.system_output(
            '%s system %s' % (self.cryptohome_path_command, self.fake_user))

        # Deduce the user cryptohome directory name for our fake user.
        self.user_cryptohome_dir = utils.system_output(
            '%s user %s' % (self.cryptohome_path_command, self.fake_user))

        # Deduce the directory for memory log storage.
        self.user_cryptohome_log_dir = ('%s/shill_logs' %
                                        self.root_cryptohome_dir)

        # The sanitized hash of the username is the basename of the cryptohome.
        self.fake_user_hash = os.path.basename(self.root_cryptohome_dir)

        # Just in case this hash actually exists, add these to the list of
        # saved directories.
        self.save_directories.append(self.root_cryptohome_dir)
        self.save_directories.append(self.user_cryptohome_dir)

        # Archive the system state we will be modifying, then remove them.
        utils.system('tar zcvf %s --directory / --ignore-failed-read %s'
                     ' 2>/dev/null' %
                     (self.saved_config, ' '.join(self.save_directories)))
        utils.system('rm -rf %s' % ' '.join(self.save_directories),
                     ignore_status=True)

        # Create the fake user's system cryptohome directory.
        os.mkdir(self.root_cryptohome_dir)
        self.new_shill_user_profile_dir = ('%s/shill' %
                                           self.root_cryptohome_dir)
        self.new_shill_user_profile = ('%s/shill.profile' %
                                       self.new_shill_user_profile_dir)

        # Create the fake user's user cryptohome directory.
        os.mkdir(self.user_cryptohome_dir)
        self.flimflam_user_profile_dir = ('%s/flimflam' %
                                          self.user_cryptohome_dir)
        self.flimflam_user_profile = ('%s/flimflam.profile' %
                                      self.flimflam_user_profile_dir)
        self.old_shill_user_profile_dir = ('%s/shill' %
                                           self.user_cryptohome_dir)
        self.old_shill_user_profile = ('%s/shill.profile' %
                                       self.old_shill_user_profile_dir)
        self.mock_flimflam = None


    def start_mock_flimflam(self):
        """ Start a mock flimflam instance to accept and log DBus calls. """
        self.mock_flimflam = mock_flimflam.MockFlimflam()
        self.mock_flimflam.start()


    def erase_state(self):
        """ Remove all the test harness files. """
        utils.system('rm -rf %s' % ' '.join(self.save_directories))
        os.mkdir(self.root_cryptohome_dir)
        os.mkdir(self.user_cryptohome_dir)


    def end_test(self):
        """ Perform cleanup at the end of the test. """
        if self.mock_flimflam:
            self.mock_flimflam.quit()
            self.mock_flimflam.join()
        self.erase_state()
        utils.system('tar zxvf %s --directory /' % self.saved_config)
        utils.system('rm -f %s' % self.saved_config)
        self.restart_system_processes()


    def restart_system_processes(self):
        """ Restart vital system services at the end of the test. """
        utils.system('start shill', ignore_status=True)
        if self.recover_duts_stopped:
            utils.system('start recover_duts', ignore_status=True)


    def assure(self, must_be_true, assertion_name):
        """ Perform a named assertion.

        @param must_be_true boolean parameter that must be true.
        @param assertion_name string name of this assertion.

        """
        if not must_be_true:
            raise error.TestFail('%s: Assertion failed: %s' %
                                 (self.test_name, assertion_name))


    def assure_path_owner(self, path, owner):
        """ Assert that |path| is owned by |owner|.

        @param path string pathname to test.
        @param owner string user name that should own |path|.

        """
        self.assure(pwd.getpwuid(os.stat(path).st_uid)[0] == owner,
                    'Path %s is owned by %s' % (path, owner))


    def assure_path_group(self, path, group):
        """ Assert that |path| is owned by |group|.

        @param path string pathname to test.
        @param group string group name that should own |path|.

        """
        self.assure(grp.getgrgid(os.stat(path).st_gid)[0] == group,
                    'Path %s is group-owned by %s' % (path, group))


    def assure_exists(self, path, path_friendly_name):
        """ Assert that |path| exists.

        @param path string pathname to test.
        @param path_friendly_name string user-parsable description of |path|.

        """
        self.assure(os.path.exists(path), '%s exists' % path_friendly_name)


    def assure_is_dir(self, path, path_friendly_name):
        """ Assert that |path| is a directory.

        @param path string pathname to test.
        @param path_friendly_name string user-parsable description of |path|.

        """
        self.assure_exists(path, path_friendly_name)
        self.assure(stat.S_ISDIR(os.lstat(path).st_mode),
                    '%s is a directory' % path_friendly_name)


    def assure_is_link(self, path, path_friendly_name):
        """ Assert that |path| is a symbolic link.

        @param path string pathname to test.
        @param path_friendly_name string user-parsable description of |path|.

        """
        self.assure_exists(path, path_friendly_name)
        self.assure(stat.S_ISLNK(os.lstat(path).st_mode),
                    '%s is a symbolic link' % path_friendly_name)


    def assure_is_link_to(self, path, pointee, path_friendly_name):
        """ Assert that |path| is a symbolic link to |pointee|.

        @param path string pathname to test.
        @param pointee string pathname that |path| should point to.
        @param path_friendly_name string user-parsable description of |path|.

        """
        self.assure_is_link(path, path_friendly_name)
        self.assure(os.readlink(path) == pointee,
                    '%s is a symbolic link to %s' %
                    (path_friendly_name, pointee))


    def assure_method_calls(self, expected_method_calls, assertion_name):
        """ Assert that |expected_method_calls| were executed on mock_flimflam.

        @param expected_method_calls list of string-tuple pairs of method
            name + tuple of arguments.
        @param assertion_name string name to assign to the assertion.

        """
        method_calls = self.mock_flimflam.get_method_calls()
        if len(expected_method_calls) != len(method_calls):
            self.assure(False, '%s: method call count does not match' %
                        assertion_name)
        for expected, actual in zip(expected_method_calls, method_calls):
            self.assure(actual.method == expected[0],
                        '%s: method %s matches expected %s' %
                        (assertion_name, actual.method, expected[0]))
            self.assure(actual.argument == expected[1],
                        '%s: argument %s matches expected %s' %
                        (assertion_name, actual.argument, expected[1]))


    def create_file_with_contents(self, filename, contents):
        """ Create a file named |filename| that contains |contents|.

        @param filename string name of file.
        @param contents string contents of file.

        """
        with open(filename, 'w') as f:
            f.write(contents)


    def touch(self, filename):
        """ Create an empty file named |filename|.

        @param filename string name of file.

        """
        self.create_file_with_contents(filename, '')


    def create_new_shill_user_profile(self, contents):
        """ Create a fake new user profile with |contents|.

        @param contents string contents of the new user profile.

        """
        os.mkdir(self.new_shill_user_profile_dir)
        self.create_file_with_contents(self.new_shill_user_profile, contents)


    def create_old_shill_user_profile(self, contents):
        """ Create a fake old-style user profile with |contents|.

        @param contents string contents of the old user profile.

        """
        os.mkdir(self.old_shill_user_profile_dir)
        self.create_file_with_contents(self.old_shill_user_profile, contents)


    def create_flimflam_user_profile(self, contents):
        """ Create a legacy flimflam user profile with |contents|.

        @param contents string contents of the flimflam user profile.

        """
        os.mkdir(self.flimflam_user_profile_dir)
        self.create_file_with_contents(self.flimflam_user_profile, contents)


    def file_contents(self, filename):
        """ Returns the contents of |filename|.

        @param filename string name of file to read.

        """
        with open(filename) as f:
            return f.read()


    def find_pid(self, process_name):
        """ Returns the process id of |process_name|.

        @param process_name string name of process to search for.

        """
        return utils.system_output('pgrep %s' % process_name,
                                   ignore_status=True).split('\n')[0]


    def get_commandline(self):
        """ Returns the command line of the current shill executable. """
        pid = self.find_pid('shill')
        return file('/proc/%s/cmdline' % pid).read().split('\0')


    def run_once(self):
        """ Main test loop. """
        try:
            self.start_test()
        except:
            self.restart_system_processes()
            raise

        try:
            self.run_tests([
                self.test_start_shill,
                self.test_start_logged_in,
                self.test_start_port_flimflam_profile])

            # The tests above run a real instance of shill, whereas the tests
            # below rely on a mock instance of shill.  We must take care not
            # to run the mock at the same time as a real shill instance.
            self.start_mock_flimflam()

            self.run_tests([
                self.test_login,
                self.test_login_guest,
                self.test_login_profile_exists,
                self.test_login_old_shill_profile,
                self.test_login_invalid_old_shill_profile,
                self.test_login_ignore_old_shill_profile,
                self.test_login_flimflam_profile,
                self.test_login_ignore_flimflam_profile,
                self.test_login_prefer_old_shill_profile,
                self.test_login_multi_profile,
                self.test_logout])
        finally:
            # Stop any shill instances started during testing.
            self.stop_shill()
            self.end_test()


    def run_tests(self, tests):
        """ Executes each of the test subparts in sequence.

        @param tests list of methods to run.

        """
        for test in tests:
          self.test_name = test.__name__
          test()
          self.stop_shill()
          self.erase_state()


    def test_start_shill(self):
        """ Test all created pathnames during shill startup.

        Also ensure the push argument is not provided by default.

        """
        self.touch('/home/chronos/.disable_shill')
        self.start_shill()
        self.assure_is_dir('/var/run/shill', 'Shill run directory')
        self.assure_is_dir('/var/lib/dhcpcd', 'dhcpcd lib directory')
        self.assure_path_owner('/var/lib/dhcpcd', 'dhcp')
        self.assure_path_group('/var/lib/dhcpcd', 'dhcp')
        self.assure_is_dir('/var/run/dhcpcd', 'dhcpcd run directory')
        self.assure_path_owner('/var/run/dhcpcd', 'dhcp')
        self.assure_path_group('/var/run/dhcpcd', 'dhcp')
        self.assure(not os.path.exists('/home/chronos/.disable_shill'),
                    'Shill disable file does not exist')
        self.assure('--push=~chronos/shill' not in self.get_commandline(),
                    'Shill command line does not contain push argument')


    def test_start_logged_in(self):
        """ Tests starting up shill while a user is already logged in.

        The "--push" argument should not be added even though shill is started
        while a user is logged in.

        """
        os.mkdir('/var/run/shill')
        os.mkdir('/var/run/shill/user_profiles')
        self.create_new_shill_user_profile('')
        os.symlink(self.new_shill_user_profile_dir,
                   '/var/run/shill/user_profiles/chronos')
        self.touch('/var/run/state/logged-in')
        self.start_shill()
        self.assure('--push=~chronos/shill' not in self.get_commandline(),
                    'Shill command line does not contain push argument')
        os.unlink('/var/run/state/logged-in')


    def test_start_port_flimflam_profile(self):
        """ Test that we can port a flimflam profile to a new shill profile.

        Startup should move an old flimflam profile into place if a shill
        profile does not already exist.

        """
        os.mkdir('/var/cache/flimflam')
        flimflam_profile = '/var/cache/flimflam/default.profile'
        self.create_file_with_contents(flimflam_profile, self.magic_header)
        shill_profile = '/var/cache/shill/default.profile'
        self.start_shill()
        self.assure(not os.path.exists(flimflam_profile),
                    'Flimflam profile no longer exists')
        self.assure(os.path.exists(shill_profile),
                    'Shill profile exists')
        self.assure(self.magic_header in self.file_contents(shill_profile),
                    'Shill default profile contains our magic header')


    def test_start_ignore_flimflam_profile(self):
        """ Test that we ignore a flimflam profile if a new profile exists.

        Startup should ignore an old flimflam profile if a shill profile
        already exists.

        """
        os.mkdir('/var/cache/flimflam')
        os.mkdir('/var/cache/shill')
        flimflam_profile = '/var/cache/flimflam/default.profile'
        self.create_file_with_contents(flimflam_profile, self.magic_header)
        shill_profile = '/var/cache/shill/default.profile'
        self.touch(shill_profile)
        self.start_shill()
        self.assure(os.path.exists(flimflam_profile),
                    'Flimflam profile still exists')
        self.assure(self.magic_header not in self.file_contents(shill_profile),
                    'Shill default profile does not contain our magic header')


    def test_login(self):
        """ Test the login process.

        Login should create a profile directory, then create and push
        a user profile, given no previous state.

        """
        os.mkdir('/var/run/shill')
        self.login()
        self.assure(not os.path.exists(self.flimflam_user_profile),
                    'Flimflam user profile does not exist')
        self.assure(not os.path.exists(self.old_shill_user_profile),
                    'Old shill user profile does not exist')
        self.assure(not os.path.exists(self.new_shill_user_profile),
                    'New shill user profile does not exist')
        # The DBus "CreateProfile" method should have been handled
        # by our mock_flimflam instance, so the profile directory
        # should not have actually been created.
        self.assure_is_dir(self.new_shill_user_profile_dir,
                           'New shill user profile directory')
        self.assure_is_dir('/var/run/shill/user_profiles',
                           'Shill profile root')
        self.assure_is_link_to('/var/run/shill/user_profiles/chronos',
                               self.new_shill_user_profile_dir,
                               'Shill profile link')
        self.assure_is_dir(self.user_cryptohome_log_dir,
                           'shill user log directory')
        self.assure_is_link_to('/var/run/shill/log',
                               self.user_cryptohome_log_dir,
                               'Shill logs link')
        self.assure_method_calls([[ 'CreateProfile', '~chronos/shill' ],
                                  [ 'InsertUserProfile',
                                    ('~chronos/shill', self.fake_user_hash) ]],
                                 'CreateProfile and InsertUserProfile '
                                 'are called')


    def test_login_guest(self):
        """ Tests the guest login process.

        Login should create a temporary profile directory in /var/run,
        instead of using one within the root directory for normal users.

        """
        os.mkdir('/var/run/shill')
        self.login_guest()
        self.assure(not os.path.exists(self.flimflam_user_profile),
                    'Flimflam user profile does not exist')
        self.assure(not os.path.exists(self.old_shill_user_profile),
                    'Old shill user profile does not exist')
        self.assure(not os.path.exists(self.new_shill_user_profile),
                    'New shill user profile does not exist')
        self.assure(not os.path.exists(self.new_shill_user_profile_dir),
                    'New shill user profile directory')
        self.assure_is_dir(self.guest_shill_user_profile_dir,
                           'shill guest user profile directory')
        self.assure_is_dir('/var/run/shill/user_profiles',
                           'Shill profile root')
        self.assure_is_link_to('/var/run/shill/user_profiles/chronos',
                               self.guest_shill_user_profile_dir,
                               'Shill profile link')
        self.assure_is_dir(self.guest_shill_user_log_dir,
                           'shill guest user log directory')
        self.assure_is_link_to('/var/run/shill/log',
                               self.guest_shill_user_log_dir,
                               'Shill logs link')
        self.assure_method_calls([[ 'CreateProfile', '~chronos/shill' ],
                                  [ 'InsertUserProfile',
                                    ('~chronos/shill', '') ]],
                                 'CreateProfile and InsertUserProfile '
                                 'are called')


    def test_login_profile_exists(self):
        """ Test logging in a user whose profile already exists.

        Login script should only push (and not create) the user profile
        if a user profile already exists.
        """
        os.mkdir('/var/run/shill')
        os.mkdir(self.new_shill_user_profile_dir)
        self.touch(self.new_shill_user_profile)
        self.login()
        self.assure_method_calls([[ 'InsertUserProfile',
                                    ('~chronos/shill', self.fake_user_hash) ]],
                                 'Only InsertUserProfile is called')


    def test_login_old_shill_profile(self):
        """ Test logging in a user with an old-style shill profile.

        Login script should move an old shill user profile into place
        if a new one does not exist.
        """
        os.mkdir('/var/run/shill')
        self.create_old_shill_user_profile(self.magic_header)
        self.login()
        self.assure(not os.path.exists(self.old_shill_user_profile),
                    'Old shill user profile no longer exists')
        self.assure(not os.path.exists(self.old_shill_user_profile_dir),
                    'Old shill user profile directory no longer exists')
        self.assure_exists(self.new_shill_user_profile,
                           'New shill profile')
        self.assure(self.magic_header in
                    self.file_contents(self.new_shill_user_profile),
                    'Shill user profile contains our magic header')
        self.assure_method_calls([[ 'InsertUserProfile',
                                    ('~chronos/shill', self.fake_user_hash) ]],
                                 'Only InsertUserProfile is called')


    def make_symlink(self, path):
        """ Create a symbolic link named |path|.

        @param path string pathname of the symbolic link.

        """
        os.symlink('/etc/hosts', path)


    def make_special_file(self, path):
        """ Create a special file named |path|.

        @param path string pathname of the special file.

        """
        os.mknod(path, stat.S_IFIFO)


    def make_bad_owner(self, path):
        """ Create a regular file with a strange ownership.

        @param path string pathname of the file.

        """
        self.touch(path)
        os.lchown(path, 1000, 1000)


    def test_login_invalid_old_shill_profile(self):
        """ Test logging in with an invalid old-style shill profile.

        Login script should ignore non-regular files or files not owned
        by the correct user.  The original file should be removed.

        """
        os.mkdir('/var/run/shill')
        for file_creation_method in (self.make_symlink,
                                     self.make_special_file,
                                     os.mkdir,
                                     self.make_bad_owner):
            os.mkdir(self.old_shill_user_profile_dir)
            file_creation_method(self.old_shill_user_profile)
            self.login()
            self.assure(not os.path.exists(self.old_shill_user_profile),
                        'Old shill user profile no longer exists')
            self.assure(not os.path.exists(self.old_shill_user_profile_dir),
                        'Old shill user profile directory no longer exists')
            self.assure(not os.path.exists(self.new_shill_user_profile),
                        'New shill profile was not created')
            self.assure_method_calls([[ 'CreateProfile', '~chronos/shill' ],
                                      [ 'InsertUserProfile',
                                        ('~chronos/shill',
                                         self.fake_user_hash) ]],
                                     'CreateProfile and InsertUserProfile '
                                     'are called')
            os.unlink('/var/run/shill/user_profiles/chronos')


    def test_login_ignore_old_shill_profile(self):
        """ Test logging in with both an old and new profile present.

        Login script should ignore an old shill user profile if a new one
        exists.

        """
        os.mkdir('/var/run/shill')
        self.create_new_shill_user_profile('')
        self.create_old_shill_user_profile(self.magic_header)
        self.login()
        self.assure(os.path.exists(self.old_shill_user_profile),
                    'Old shill user profile still exists')
        self.assure_exists(self.new_shill_user_profile,
                           'New shill profile')
        self.assure(self.magic_header not in
                    self.file_contents(self.new_shill_user_profile),
                    'Shill user profile does not contain our magic header')
        self.assure_method_calls([[ 'InsertUserProfile',
                                    ('~chronos/shill', self.fake_user_hash) ]],
                                 'Only InsertUserProfile is called')


    def test_login_flimflam_profile(self):
        """ Test logging in with an old flimflam profile.

        Login script should move a flimflam user profile into place
        if a shill one does not exist.

        """
        os.mkdir('/var/run/shill')
        self.create_flimflam_user_profile(self.magic_header)
        self.login()
        self.assure(not os.path.exists(self.flimflam_user_profile),
                    'Flimflam user profile no longer exists')
        self.assure(not os.path.exists(self.flimflam_user_profile_dir),
                    'Flimflam user profile directory no longer exists')
        self.assure_exists(self.new_shill_user_profile,
                           'New shill profile')
        self.assure(self.magic_header in
                    self.file_contents(self.new_shill_user_profile),
                    'Shill user profile contains our magic header')
        self.assure_method_calls([[ 'InsertUserProfile',
                                    ('~chronos/shill', self.fake_user_hash) ]],
                                 'Only InsertUserProfile is called')


    def test_login_ignore_flimflam_profile(self):
        """ Test logging in with both a flimflam profile and a new profile.

        Login script should ignore an old flimflam user profile if a new
        one exists.

        """
        os.mkdir('/var/run/shill')
        self.create_flimflam_user_profile(self.magic_header)
        self.create_new_shill_user_profile('')
        self.login()
        self.assure_exists(self.new_shill_user_profile,
                           'New shill profile')
        self.assure(self.magic_header not in
                    self.file_contents(self.new_shill_user_profile),
                    'Shill user profile does not contain our magic header')
        self.assure_method_calls([[ 'InsertUserProfile',
                                    ('~chronos/shill', self.fake_user_hash) ]],
                                 'Only InsertUserProfile is called')


    def test_login_prefer_old_shill_profile(self):
        """ Test logging in with both a flimflam and old-style shill profile.

        Login script should use the old shill user profile in preference
        to a flimflam user profile if the new user profile does not
        exist.

        """
        os.mkdir('/var/run/shill')
        self.create_flimflam_user_profile('')
        self.create_old_shill_user_profile(self.magic_header)
        self.login()
        self.assure(not os.path.exists(self.flimflam_user_profile),
                    'Flimflam user profile was removed')
        self.assure(not os.path.exists(self.old_shill_user_profile),
                    'Old shill user profile no longer exists')
        self.assure_exists(self.new_shill_user_profile,
                           'New shill profile')
        self.assure(self.magic_header in
                    self.file_contents(self.new_shill_user_profile),
                    'Shill user profile contains our magic header')
        self.assure_method_calls([[ 'InsertUserProfile',
                                    ('~chronos/shill', self.fake_user_hash) ]],
                                 'Only InsertUserProfile is called')


    def test_login_multi_profile(self):
        """ Test signalling shill about multiple logged-in users.

        Login script should not create multiple profiles in parallel
        if called more than once without an intervening logout.  Only
        the initial user profile should be created.

        """
        os.mkdir('/var/run/shill')
        self.create_new_shill_user_profile('')

        # First logged-in user should create a profile (tested above).
        self.login()

        # Clear the mock method-call queue.
        self.mock_flimflam.get_method_calls()

        for attempt in range(5):
            self.login()
            self.assure_method_calls([], 'No more profiles are added to shill')
            profile_links = os.listdir('/var/run/shill/user_profiles')
            self.assure(len(profile_links) == 1, 'Only one profile exists')
            self.assure(profile_links[0] == 'chronos',
                        'The profile link is for the chronos user')
            self.assure_is_link_to('/var/run/shill/log',
                                   self.user_cryptohome_log_dir,
                                   'Shill log link for chronos')


    def test_logout(self):
        """ Test the logout process. """
        os.makedirs('/var/run/shill/user_profiles')
        os.makedirs(self.guest_shill_user_profile_dir)
        os.makedirs(self.guest_shill_user_log_dir)
        self.touch('/var/run/state/logged-in')
        self.logout()
        self.assure(not os.path.exists('/var/run/shill/user_profiles'),
                    'User profile directory was removed')
        self.assure(not os.path.exists(self.guest_shill_user_profile_dir),
                    'Guest user profile directory was removed')
        self.assure(not os.path.exists(self.guest_shill_user_log_dir),
                    'Guest user log directory was removed')
        self.assure_method_calls([[ 'PopAllUserProfiles', '' ]],
                                 'PopAllUserProfiles is called')
