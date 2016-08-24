# Please keep this code python 2.4 compatible and stand alone.

import logging, os, shutil, sys, tempfile, time, urllib2
import subprocess, re
from autotest_lib.client.common_lib import autotemp, revision_control, utils

_READ_SIZE = 64*1024
_MAX_PACKAGE_SIZE = 100*1024*1024


class Error(Exception):
    """Local exception to be raised by code in this file."""

class FetchError(Error):
    """Failed to fetch a package from any of its listed URLs."""


def _checksum_file(full_path):
    """@returns The hex checksum of a file given its pathname."""
    inputfile = open(full_path, 'rb')
    try:
        hex_sum = utils.hash('sha1', inputfile.read()).hexdigest()
    finally:
        inputfile.close()
    return hex_sum


def system(commandline):
    """Same as os.system(commandline) but logs the command first.

    @param commandline: commandline to be called.
    """
    logging.info(commandline)
    return os.system(commandline)


def find_top_of_autotest_tree():
    """@returns The full path to the top of the autotest directory tree."""
    dirname = os.path.dirname(__file__)
    autotest_dir = os.path.abspath(os.path.join(dirname, '..'))
    return autotest_dir


class ExternalPackage(object):
    """
    Defines an external package with URLs to fetch its sources from and
    a build_and_install() method to unpack it, build it and install it
    beneath our own autotest/site-packages directory.

    Base Class.  Subclass this to define packages.
    Note: Unless your subclass has a specific reason to, it should not
    re-install the package every time build_externals is invoked, as this
    happens periodically through the scheduler. To avoid doing so the is_needed
    method needs to return an appropriate value.

    Attributes:
      @attribute urls - A tuple of URLs to try fetching the package from.
      @attribute local_filename - A local filename to use when saving the
              fetched package.
      @attribute hex_sum - The hex digest (currently SHA1) of this package
              to be used to verify its contents.
      @attribute module_name - The installed python module name to be used for
              for a version check.  Defaults to the lower case class name with
              the word Package stripped off.
      @attribute version - The desired minimum package version.
      @attribute os_requirements - A dictionary mapping pathname tuples on the
              the OS distribution to a likely name of a package the user
              needs to install on their system in order to get this file.
              One of the files in the tuple must exist.
      @attribute name - Read only, the printable name of the package.
      @attribute subclasses - This class attribute holds a list of all defined
              subclasses.  It is constructed dynamically using the metaclass.
    """
    # Modules that are meant to be installed in system directory, rather than
    # autotest/site-packages. These modules should be skipped if the module
    # is already installed in system directory. This prevents an older version
    # of the module from being installed in system directory.
    SYSTEM_MODULES = ['setuptools']

    subclasses = []
    urls = ()
    local_filename = None
    hex_sum = None
    module_name = None
    version = None
    os_requirements = None


    class __metaclass__(type):
        """Any time a subclass is defined, add it to our list."""
        def __init__(mcs, name, bases, dict):
            if name != 'ExternalPackage' and not name.startswith('_'):
                mcs.subclasses.append(mcs)


    def __init__(self):
        self.verified_package = ''
        if not self.module_name:
            self.module_name = self.name.lower()
        self.installed_version = ''


    @property
    def name(self):
        """Return the class name with any trailing 'Package' stripped off."""
        class_name = self.__class__.__name__
        if class_name.endswith('Package'):
            return class_name[:-len('Package')]
        return class_name


    def is_needed(self, install_dir):
        """
        Check to see if we need to reinstall a package. This is contingent on:
        1. Module name: If the name of the module is different from the package,
            the class that installs it needs to specify a module_name string,
            so we can try importing the module.

        2. Installed version: If the module doesn't contain a __version__ the
            class that installs it needs to override the
            _get_installed_version_from_module method to return an appropriate
            version string.

        3. Version/Minimum version: The class that installs the package should
            contain a version string, and an optional minimum version string.

        4. install_dir: If the module exists in a different directory, e.g.,
            /usr/lib/python2.7/dist-packages/, the module will be forced to be
            installed in install_dir.

        @param install_dir: install directory.
        @returns True if self.module_name needs to be built and installed.
        """
        if not self.module_name or not self.version:
            logging.warning('version and module_name required for '
                            'is_needed() check to work.')
            return True
        try:
            module = __import__(self.module_name)
        except ImportError, e:
            logging.info("%s isn't present. Will install.", self.module_name)
            return True
        if (not module.__file__.startswith(install_dir) and
            not self.module_name in self.SYSTEM_MODULES):
            logging.info('Module %s is installed in %s, rather than %s. The '
                         'module will be forced to be installed in %s.',
                         self.module_name, module.__file__, install_dir,
                         install_dir)
            return True
        self.installed_version = self._get_installed_version_from_module(module)
        logging.info('imported %s version %s.', self.module_name,
                     self.installed_version)
        if hasattr(self, 'minimum_version'):
            return self.minimum_version > self.installed_version
        else:
            return self.version > self.installed_version


    def _get_installed_version_from_module(self, module):
        """Ask our module its version string and return it or '' if unknown."""
        try:
            return module.__version__
        except AttributeError:
            logging.error('could not get version from %s', module)
            return ''


    def _build_and_install(self, install_dir):
        """Subclasses MUST provide their own implementation."""
        raise NotImplementedError


    def _build_and_install_current_dir(self, install_dir):
        """
        Subclasses that use _build_and_install_from_package() MUST provide
        their own implementation of this method.
        """
        raise NotImplementedError


    def build_and_install(self, install_dir):
        """
        Builds and installs the package.  It must have been fetched already.

        @param install_dir - The package installation directory.  If it does
            not exist it will be created.
        """
        if not self.verified_package:
            raise Error('Must call fetch() first.  - %s' % self.name)
        self._check_os_requirements()
        return self._build_and_install(install_dir)


    def _check_os_requirements(self):
        if not self.os_requirements:
            return
        failed = False
        for file_names, package_name in self.os_requirements.iteritems():
            if not any(os.path.exists(file_name) for file_name in file_names):
                failed = True
                logging.error('Can\'t find %s, %s probably needs it.',
                              ' or '.join(file_names), self.name)
                logging.error('Perhaps you need to install something similar '
                              'to the %s package for OS first.', package_name)
        if failed:
            raise Error('Missing OS requirements for %s.  (see above)' %
                        self.name)


    def _build_and_install_current_dir_setup_py(self, install_dir):
        """For use as a _build_and_install_current_dir implementation."""
        egg_path = self._build_egg_using_setup_py(setup_py='setup.py')
        if not egg_path:
            return False
        return self._install_from_egg(install_dir, egg_path)


    def _build_and_install_current_dir_setupegg_py(self, install_dir):
        """For use as a _build_and_install_current_dir implementation."""
        egg_path = self._build_egg_using_setup_py(setup_py='setupegg.py')
        if not egg_path:
            return False
        return self._install_from_egg(install_dir, egg_path)


    def _build_and_install_current_dir_noegg(self, install_dir):
        if not self._build_using_setup_py():
            return False
        return self._install_using_setup_py_and_rsync(install_dir)


    def _build_and_install_from_package(self, install_dir):
        """
        This method may be used as a _build_and_install() implementation
        for subclasses if they implement _build_and_install_current_dir().

        Extracts the .tar.gz file, chdirs into the extracted directory
        (which is assumed to match the tar filename) and calls
        _build_and_isntall_current_dir from there.

        Afterwards the build (regardless of failure) extracted .tar.gz
        directory is cleaned up.

        @returns True on success, False otherwise.

        @raises OSError If the expected extraction directory does not exist.
        """
        self._extract_compressed_package()
        if self.verified_package.endswith('.tar.gz'):
            extension = '.tar.gz'
        elif self.verified_package.endswith('.tar.bz2'):
            extension = '.tar.bz2'
        elif self.verified_package.endswith('.zip'):
            extension = '.zip'
        else:
            raise Error('Unexpected package file extension on %s' %
                        self.verified_package)
        os.chdir(os.path.dirname(self.verified_package))
        os.chdir(self.local_filename[:-len(extension)])
        extracted_dir = os.getcwd()
        try:
            return self._build_and_install_current_dir(install_dir)
        finally:
            os.chdir(os.path.join(extracted_dir, '..'))
            shutil.rmtree(extracted_dir)


    def _extract_compressed_package(self):
        """Extract the fetched compressed .tar or .zip within its directory."""
        if not self.verified_package:
            raise Error('Package must have been fetched first.')
        os.chdir(os.path.dirname(self.verified_package))
        if self.verified_package.endswith('gz'):
            status = system("tar -xzf '%s'" % self.verified_package)
        elif self.verified_package.endswith('bz2'):
            status = system("tar -xjf '%s'" % self.verified_package)
        elif self.verified_package.endswith('zip'):
            status = system("unzip '%s'" % self.verified_package)
        else:
            raise Error('Unknown compression suffix on %s.' %
                        self.verified_package)
        if status:
            raise Error('tar failed with %s' % (status,))


    def _build_using_setup_py(self, setup_py='setup.py'):
        """
        Assuming the cwd is the extracted python package, execute a simple
        python setup.py build.

        @param setup_py - The name of the setup.py file to execute.

        @returns True on success, False otherwise.
        """
        if not os.path.exists(setup_py):
            raise Error('%s does not exist in %s' % (setup_py, os.getcwd()))
        status = system("'%s' %s build" % (sys.executable, setup_py))
        if status:
            logging.error('%s build failed.', self.name)
            return False
        return True


    def _build_egg_using_setup_py(self, setup_py='setup.py'):
        """
        Assuming the cwd is the extracted python package, execute a simple
        python setup.py bdist_egg.

        @param setup_py - The name of the setup.py file to execute.

        @returns The relative path to the resulting egg file or '' on failure.
        """
        if not os.path.exists(setup_py):
            raise Error('%s does not exist in %s' % (setup_py, os.getcwd()))
        egg_subdir = 'dist'
        if os.path.isdir(egg_subdir):
            shutil.rmtree(egg_subdir)
        status = system("'%s' %s bdist_egg" % (sys.executable, setup_py))
        if status:
            logging.error('bdist_egg of setuptools failed.')
            return ''
        # I've never seen a bdist_egg lay multiple .egg files.
        for filename in os.listdir(egg_subdir):
            if filename.endswith('.egg'):
                return os.path.join(egg_subdir, filename)


    def _install_from_egg(self, install_dir, egg_path):
        """
        Install a module from an egg file by unzipping the necessary parts
        into install_dir.

        @param install_dir - The installation directory.
        @param egg_path - The pathname of the egg file.
        """
        status = system("unzip -q -o -d '%s' '%s'" % (install_dir, egg_path))
        if status:
            logging.error('unzip of %s failed', egg_path)
            return False
        egg_info = os.path.join(install_dir, 'EGG-INFO')
        if os.path.isdir(egg_info):
            shutil.rmtree(egg_info)
        return True


    def _get_temp_dir(self):
        return tempfile.mkdtemp(dir='/var/tmp')


    def _site_packages_path(self, temp_dir):
        # This makes assumptions about what python setup.py install
        # does when given a prefix.  Is this always correct?
        python_xy = 'python%s' % sys.version[:3]
        return os.path.join(temp_dir, 'lib', python_xy, 'site-packages')


    def _rsync (self, temp_site_dir, install_dir):
        """Rsync contents. """
        status = system("rsync -r '%s/' '%s/'" %
                        (os.path.normpath(temp_site_dir),
                         os.path.normpath(install_dir)))
        if status:
            logging.error('%s rsync to install_dir failed.', self.name)
            return False
        return True


    def _install_using_setup_py_and_rsync(self, install_dir,
                                          setup_py='setup.py',
                                          temp_dir=None):
        """
        Assuming the cwd is the extracted python package, execute a simple:

          python setup.py install --prefix=BLA

        BLA will be a temporary directory that everything installed will
        be picked out of and rsynced to the appropriate place under
        install_dir afterwards.

        Afterwards, it deconstructs the extra lib/pythonX.Y/site-packages/
        directory tree that setuptools created and moves all installed
        site-packages directly up into install_dir itself.

        @param install_dir the directory for the install to happen under.
        @param setup_py - The name of the setup.py file to execute.

        @returns True on success, False otherwise.
        """
        if not os.path.exists(setup_py):
            raise Error('%s does not exist in %s' % (setup_py, os.getcwd()))

        if temp_dir is None:
            temp_dir = self._get_temp_dir()

        try:
            status = system("'%s' %s install --no-compile --prefix='%s'"
                            % (sys.executable, setup_py, temp_dir))
            if status:
                logging.error('%s install failed.', self.name)
                return False

            if os.path.isdir(os.path.join(temp_dir, 'lib')):
                # NOTE: This ignores anything outside of the lib/ dir that
                # was installed.
                temp_site_dir = self._site_packages_path(temp_dir)
            else:
                temp_site_dir = temp_dir

            return self._rsync(temp_site_dir, install_dir)
        finally:
            shutil.rmtree(temp_dir)



    def _build_using_make(self, install_dir):
        """Build the current package using configure/make.

        @returns True on success, False otherwise.
        """
        install_prefix = os.path.join(install_dir, 'usr', 'local')
        status = system('./configure --prefix=%s' % install_prefix)
        if status:
            logging.error('./configure failed for %s', self.name)
            return False
        status = system('make')
        if status:
            logging.error('make failed for %s', self.name)
            return False
        status = system('make check')
        if status:
            logging.error('make check failed for %s', self.name)
            return False
        return True


    def _install_using_make(self):
        """Install the current package using make install.

        Assumes the install path was set up while running ./configure (in
        _build_using_make()).

        @returns True on success, False otherwise.
        """
        status = system('make install')
        return status == 0


    def fetch(self, dest_dir):
        """
        Fetch the package from one its URLs and save it in dest_dir.

        If the the package already exists in dest_dir and the checksum
        matches this code will not fetch it again.

        Sets the 'verified_package' attribute with the destination pathname.

        @param dest_dir - The destination directory to save the local file.
            If it does not exist it will be created.

        @returns A boolean indicating if we the package is now in dest_dir.
        @raises FetchError - When something unexpected happens.
        """
        if not os.path.exists(dest_dir):
            os.makedirs(dest_dir)
        local_path = os.path.join(dest_dir, self.local_filename)

        # If the package exists, verify its checksum and be happy if it is good.
        if os.path.exists(local_path):
            actual_hex_sum = _checksum_file(local_path)
            if self.hex_sum == actual_hex_sum:
                logging.info('Good checksum for existing %s package.',
                             self.name)
                self.verified_package = local_path
                return True
            logging.warning('Bad checksum for existing %s package.  '
                            'Re-downloading', self.name)
            os.rename(local_path, local_path + '.wrong-checksum')

        # Download the package from one of its urls, rejecting any if the
        # checksum does not match.
        for url in self.urls:
            logging.info('Fetching %s', url)
            try:
                url_file = urllib2.urlopen(url)
            except (urllib2.URLError, EnvironmentError):
                logging.warning('Could not fetch %s package from %s.',
                                self.name, url)
                continue

            data_length = int(url_file.info().get('Content-Length',
                                                  _MAX_PACKAGE_SIZE))
            if data_length <= 0 or data_length > _MAX_PACKAGE_SIZE:
                raise FetchError('%s from %s fails Content-Length %d '
                                 'sanity check.' % (self.name, url,
                                                    data_length))
            checksum = utils.hash('sha1')
            total_read = 0
            output = open(local_path, 'wb')
            try:
                while total_read < data_length:
                    data = url_file.read(_READ_SIZE)
                    if not data:
                        break
                    output.write(data)
                    checksum.update(data)
                    total_read += len(data)
            finally:
                output.close()
            if self.hex_sum != checksum.hexdigest():
                logging.warning('Bad checksum for %s fetched from %s.',
                                self.name, url)
                logging.warning('Got %s', checksum.hexdigest())
                logging.warning('Expected %s', self.hex_sum)
                os.unlink(local_path)
                continue
            logging.info('Good checksum.')
            self.verified_package = local_path
            return True
        else:
            return False


# NOTE: This class definition must come -before- all other ExternalPackage
# classes that need to use this version of setuptools so that is is inserted
# into the ExternalPackage.subclasses list before them.
class SetuptoolsPackage(ExternalPackage):
    """setuptools package"""
    # For all known setuptools releases a string compare works for the
    # version string.  Hopefully they never release a 0.10.  (Their own
    # version comparison code would break if they did.)
    # Any system with setuptools > 18.0.1 is fine. If none installed, then
    # try to install the latest found on the upstream.
    minimum_version = '18.0.1'
    version = '18.0.1'
    urls = ('http://pypi.python.org/packages/source/s/setuptools/'
            'setuptools-%s.tar.gz' % (version,),)
    local_filename = 'setuptools-%s.tar.gz' % version
    hex_sum = 'ebc4fe81b7f6d61d923d9519f589903824044f52'

    SUDO_SLEEP_DELAY = 15


    def _build_and_install(self, install_dir):
        """Install setuptools on the system."""
        logging.info('NOTE: setuptools install does not use install_dir.')
        return self._build_and_install_from_package(install_dir)


    def _build_and_install_current_dir(self, install_dir):
        egg_path = self._build_egg_using_setup_py()
        if not egg_path:
            return False

        print '!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n'
        print 'About to run sudo to install setuptools', self.version
        print 'on your system for use by', sys.executable, '\n'
        print '!! ^C within', self.SUDO_SLEEP_DELAY, 'seconds to abort.\n'
        time.sleep(self.SUDO_SLEEP_DELAY)

        # Copy the egg to the local filesystem /var/tmp so that root can
        # access it properly (avoid NFS squashroot issues).
        temp_dir = self._get_temp_dir()
        try:
            shutil.copy(egg_path, temp_dir)
            egg_name = os.path.split(egg_path)[1]
            temp_egg = os.path.join(temp_dir, egg_name)
            p = subprocess.Popen(['sudo', '/bin/sh', temp_egg],
                                 stdout=subprocess.PIPE)
            regex = re.compile('Copying (.*?) to (.*?)\n')
            match = regex.search(p.communicate()[0])
            status = p.wait()

            if match:
                compiled = os.path.join(match.group(2), match.group(1))
                os.system("sudo chmod a+r '%s'" % compiled)
        finally:
            shutil.rmtree(temp_dir)

        if status:
            logging.error('install of setuptools from egg failed.')
            return False
        return True


class MySQLdbPackage(ExternalPackage):
    """mysql package, used in scheduler."""
    module_name = 'MySQLdb'
    version = '1.2.3'
    urls = ('http://downloads.sourceforge.net/project/mysql-python/'
            'mysql-python/%(version)s/MySQL-python-%(version)s.tar.gz'
            % dict(version=version),)
    local_filename = 'MySQL-python-%s.tar.gz' % version
    hex_sum = '3511bb8c57c6016eeafa531d5c3ea4b548915e3c'

    _build_and_install_current_dir = (
            ExternalPackage._build_and_install_current_dir_setup_py)


    def _build_and_install(self, install_dir):
        if not os.path.exists('/usr/bin/mysql_config'):
            error_msg = ('You need to install /usr/bin/mysql_config.\n'
                         'On Ubuntu or Debian based systems use this: '
                         'sudo apt-get install libmysqlclient15-dev')
            logging.error(error_msg)
            return False, error_msg
        return self._build_and_install_from_package(install_dir)


class DjangoPackage(ExternalPackage):
    """django package."""
    version = '1.5.1'
    local_filename = 'Django-%s.tar.gz' % version
    urls = ('http://www.djangoproject.com/download/%s/tarball/' % version,)
    hex_sum = '0ab97b90c4c79636e56337f426f1e875faccbba1'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
            ExternalPackage._build_and_install_current_dir_noegg)


    def _get_installed_version_from_module(self, module):
        try:
            return module.get_version().split()[0]
        except AttributeError:
            return '0.9.6'



class NumpyPackage(ExternalPackage):
    """numpy package, required by matploglib."""
    version = '1.7.0'
    local_filename = 'numpy-%s.tar.gz' % version
    urls = ('http://downloads.sourceforge.net/project/numpy/NumPy/%(version)s/'
            'numpy-%(version)s.tar.gz' % dict(version=version),)
    hex_sum = 'ba328985f20390b0f969a5be2a6e1141d5752cf9'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
            ExternalPackage._build_and_install_current_dir_setupegg_py)


class MatplotlibPackage(ExternalPackage):
    """
    matplotlib package

    This requires numpy so it must be declared after numpy to guarantee that
    it is already installed.
    """
    version = '0.98.5.3'
    short_version = '0.98.5'
    local_filename = 'matplotlib-%s.tar.gz' % version
    urls = ('http://downloads.sourceforge.net/project/matplotlib/matplotlib/'
            'matplotlib-%s/matplotlib-%s.tar.gz' % (short_version, version),)
    hex_sum = '2f6c894cf407192b3b60351bcc6468c0385d47b6'
    os_requirements = {('/usr/include/freetype2/ft2build.h',
                        '/usr/include/ft2build.h'): 'libfreetype6-dev',
                       ('/usr/include/png.h'): 'libpng12-dev'}

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
            ExternalPackage._build_and_install_current_dir_setupegg_py)


class AtForkPackage(ExternalPackage):
    """atfork package"""
    version = '0.1.2'
    local_filename = 'atfork-%s.zip' % version
    urls = ('http://python-atfork.googlecode.com/files/' + local_filename,)
    hex_sum = '5baa64c73e966b57fa797040585c760c502dc70b'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
            ExternalPackage._build_and_install_current_dir_noegg)


class ParamikoPackage(ExternalPackage):
    """paramiko package"""
    version = '1.7.5'
    local_filename = 'paramiko-%s.zip' % version
    urls = ('https://pypi.python.org/packages/source/p/paramiko/' + local_filename,)
    hex_sum = 'd23e437c0d8bd6aeb181d9990a9d670fb30d0c72'


    _build_and_install = ExternalPackage._build_and_install_from_package


    def _check_for_pycrypto(self):
        # NOTE(gps): Linux distros have better python-crypto packages than we
        # can easily get today via a wget due to the library's age and staleness
        # yet many security and behavior bugs are fixed by patches that distros
        # already apply.  PyCrypto has a new active maintainer in 2009.  Once a
        # new release is made (http://pycrypto.org/) we should add an installer.
        try:
            import Crypto
        except ImportError:
            logging.error('Please run "sudo apt-get install python-crypto" '
                          'or your Linux distro\'s equivalent.')
            return False
        return True


    def _build_and_install_current_dir(self, install_dir):
        if not self._check_for_pycrypto():
            return False
        # paramiko 1.7.4 doesn't require building, it is just a module directory
        # that we can rsync into place directly.
        if not os.path.isdir('paramiko'):
            raise Error('no paramiko directory in %s.' % os.getcwd())
        status = system("rsync -r 'paramiko' '%s/'" % install_dir)
        if status:
            logging.error('%s rsync to install_dir failed.', self.name)
            return False
        return True


class RequestsPackage(ExternalPackage):
    """requests package"""
    version = '0.11.2'
    local_filename = 'requests-%s.tar.gz' % version
    urls = ('http://pypi.python.org/packages/source/r/requests/' +
            local_filename,)
    hex_sum = '00a49e8bd6dd8955acf6f6269d1b85f50c70b712'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_setup_py)


class JsonRPCLib(ExternalPackage):
    """jsonrpclib package"""
    version = '0.1.3'
    module_name = 'jsonrpclib'
    local_filename = '%s-%s.tar.gz' % (module_name, version)
    urls = ('http://pypi.python.org/packages/source/j/%s/%s' %
            (module_name, local_filename), )
    hex_sum = '431714ed19ab677f641ce5d678a6a95016f5c452'

    def _get_installed_version_from_module(self, module):
        # jsonrpclib doesn't contain a proper version
        return self.version

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_noegg)


class Httplib2Package(ExternalPackage):
    """httplib2 package"""
    version = '0.6.0'
    local_filename = 'httplib2-%s.tar.gz' % version
    urls = ('http://httplib2.googlecode.com/files/' + local_filename,)
    hex_sum = '995344b2704826cc0d61a266e995b328d92445a5'

    def _get_installed_version_from_module(self, module):
        # httplib2 doesn't contain a proper version
        return self.version

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_noegg)


class GwtPackage(ExternalPackage):
    """Fetch and extract a local copy of GWT used to build the frontend."""

    version = '2.3.0'
    local_filename = 'gwt-%s.zip' % version
    urls = ('http://google-web-toolkit.googlecode.com/files/' + local_filename,)
    hex_sum = 'd51fce9166e6b31349659ffca89baf93e39bc84b'
    name = 'gwt'
    about_filename = 'about.txt'
    module_name = None  # Not a Python module.


    def is_needed(self, install_dir):
        gwt_dir = os.path.join(install_dir, self.name)
        about_file = os.path.join(install_dir, self.name, self.about_filename)

        if not os.path.exists(gwt_dir) or not os.path.exists(about_file):
            logging.info('gwt not installed for autotest')
            return True

        f = open(about_file, 'r')
        version_line = f.readline()
        f.close()

        match = re.match(r'Google Web Toolkit (.*)', version_line)
        if not match:
            logging.info('did not find gwt version')
            return True

        logging.info('found gwt version %s', match.group(1))
        return match.group(1) != self.version


    def _build_and_install(self, install_dir):
        os.chdir(install_dir)
        self._extract_compressed_package()
        extracted_dir = self.local_filename[:-len('.zip')]
        target_dir = os.path.join(install_dir, self.name)
        if os.path.exists(target_dir):
            shutil.rmtree(target_dir)
        os.rename(extracted_dir, target_dir)
        return True


class GVizAPIPackage(ExternalPackage):
    """gviz package"""
    module_name = 'gviz_api'
    version = '1.7.0'
    url_filename = 'gviz_api_py-%s.tar.gz' % version
    local_filename = 'google-visualization-python.tar.gz'
    urls = ('http://google-visualization-python.googlecode.com/files/%s' % (
        url_filename),)
    hex_sum = 'cd9a0fb4ca5c4f86c0d85756f501fd54ccf492d2'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_noegg)

    def _get_installed_version_from_module(self, module):
        # gviz doesn't contain a proper version
        return self.version


class StatsdPackage(ExternalPackage):
    """python-statsd package"""
    version = '1.7.2'
    url_filename = 'python-statsd-%s.tar.gz' % version
    local_filename = url_filename
    urls = ('http://pypi.python.org/packages/source/p/python-statsd/%s' % (
        url_filename),)
    hex_sum = '2cc186ebdb723e2420b432ab71639786d877694b'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_setup_py)


class GdataPackage(ExternalPackage):
    """
    Pulls the GData library, giving us an API to query tracker.
    """

    version = '2.0.14'
    url_filename = 'gdata-%s.tar.gz' % version
    local_filename = url_filename
    urls = ('http://gdata-python-client.googlecode.com/files/%s' % (
        url_filename),)
    hex_sum = '5eed0e01ab931e3f706ec544fc8f06ecac384e91'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_noegg)

    def _get_installed_version_from_module(self, module):
        # gdata doesn't contain a proper version
        return self.version


class GoogleAPIClientPackage(ExternalPackage):
    """
    Pulls the Python Google API client library.
    """
    version = '1.1'
    module_name = 'apiclient'
    url_filename = 'google-api-python-client-%s.tar.gz' % version
    local_filename = url_filename
    urls = ('https://google-api-python-client.googlecode.com/files/%s' % (
        url_filename),)
    hex_sum = '2294949683e367b3d4ecaeb77502509c5af21e60'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_setup_py)


class GFlagsPackage(ExternalPackage):
    """
    Gets the Python GFlags client library.
    """
    # gflags doesn't contain a proper version
    version = '2.0'
    url_filename = 'python-gflags-%s.tar.gz' % version
    local_filename = url_filename
    urls = ('https://python-gflags.googlecode.com/files/%s' % (
        url_filename),)
    hex_sum = 'db309e6964b102ff36de319ce551db512a78281e'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_setup_py)


    def _get_installed_version_from_module(self, module):
        return self.version


class DnsPythonPackage(ExternalPackage):
    """
    dns module

    Used in unittests.
    """
    module_name = 'dns'
    version = '1.3.5'
    url_filename = 'dnspython-%s.tar.gz' % version
    local_filename = url_filename
    urls = ('http://www.dnspython.org/kits/%s/%s' % (
        version, url_filename),)

    hex_sum = '06314dad339549613435470c6add992910e26e5d'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_noegg)

    def _get_installed_version_from_module(self, module):
        """Ask our module its version string and return it or '' if unknown."""
        try:
            __import__(self.module_name + '.version')
            return module.version.version
        except AttributeError:
            logging.error('could not get version from %s', module)
            return ''


class PyudevPackage(ExternalPackage):
    """
    pyudev module

    Used in unittests.
    """
    version = '0.16.1'
    url_filename = 'pyudev-%s.tar.gz' % version
    local_filename = url_filename
    urls = ('http://pypi.python.org/packages/source/p/pyudev/%s' % (
        url_filename),)
    hex_sum = 'b36bc5c553ce9b56d32a5e45063a2c88156771c0'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_setup_py)


class PyMoxPackage(ExternalPackage):
    """
    mox module

    Used in unittests.
    """
    module_name = 'mox'
    version = '0.5.3'
    url_filename = 'mox-%s.tar.gz' % version
    local_filename = url_filename
    urls = ('http://pypi.python.org/packages/source/m/mox/%s' % (
        url_filename),)
    hex_sum = '1c502d2c0a8aefbba2c7f385a83d33e7d822452a'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_noegg)

    def _get_installed_version_from_module(self, module):
        # mox doesn't contain a proper version
        return self.version


class PySeleniumPackage(ExternalPackage):
    """
    selenium module

    Used in wifi_interop suite.
    """
    module_name = 'selenium'
    version = '2.37.2'
    url_filename = 'selenium-%s.tar.gz' % version
    local_filename = url_filename
    urls = ('https://pypi.python.org/packages/source/s/selenium/%s' % (
        url_filename),)
    hex_sum = '66946d5349e36d946daaad625c83c30c11609e36'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_setup_py)


class FaultHandlerPackage(ExternalPackage):
    """
    faulthandler module
    """
    module_name = 'faulthandler'
    version = '2.3'
    url_filename = '%s-%s.tar.gz' % (module_name, version)
    local_filename = url_filename
    urls = ('http://pypi.python.org/packages/source/f/faulthandler/%s' %
            (url_filename),)
    hex_sum = 'efb30c068414fba9df892e48fcf86170cbf53589'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
            ExternalPackage._build_and_install_current_dir_noegg)


class PsutilPackage(ExternalPackage):
    """
    psutil module
    """
    module_name = 'psutil'
    version = '2.1.1'
    url_filename = '%s-%s.tar.gz' % (module_name, version)
    local_filename = url_filename
    urls = ('http://pypi.python.org/packages/source/p/psutil/%s' %
            (url_filename),)
    hex_sum = '0c20a20ed316e69f2b0881530439213988229916'

    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
                        ExternalPackage._build_and_install_current_dir_setup_py)


class ElasticSearchPackage(ExternalPackage):
    """elasticsearch-py package."""
    version = '1.6.0'
    url_filename = 'elasticsearch-%s.tar.gz' % version
    local_filename = url_filename
    urls = ('https://pypi.python.org/packages/source/e/elasticsearch/%s' %
            (url_filename),)
    hex_sum = '3e676c96f47935b1f52df82df3969564bd356b1c'
    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
            ExternalPackage._build_and_install_current_dir_setup_py)


class Urllib3Package(ExternalPackage):
    """elasticsearch-py package."""
    version = '1.9'
    url_filename = 'urllib3-%s.tar.gz' % version
    local_filename = url_filename
    urls = ('https://pypi.python.org/packages/source/u/urllib3/%s' %
            (url_filename),)
    hex_sum = '9522197efb2a2b49ce804de3a515f06d97b6602f'
    _build_and_install = ExternalPackage._build_and_install_from_package
    _build_and_install_current_dir = (
            ExternalPackage._build_and_install_current_dir_setup_py)


class ImagingLibraryPackage(ExternalPackage):
    """Python Imaging Library (PIL)."""
    version = '1.1.7'
    url_filename = 'Imaging-%s.tar.gz' % version
    local_filename = url_filename
    urls = ('http://effbot.org/downloads/%s' % url_filename,)
    hex_sum = '76c37504251171fda8da8e63ecb8bc42a69a5c81'

    def _build_and_install(self, install_dir):
        # The path of zlib library might be different from what PIL setup.py is
        # expected. Following change does the best attempt to link the library
        # to a path PIL setup.py will try.
        libz_possible_path = '/usr/lib/x86_64-linux-gnu/libz.so'
        libz_expected_path = '/usr/lib/libz.so'
        if (os.path.exists(libz_possible_path) and
            not os.path.exists(libz_expected_path)):
            utils.run('sudo ln -s %s %s' %
                      (libz_possible_path, libz_expected_path))
        return self._build_and_install_from_package(install_dir)

    _build_and_install_current_dir = (
            ExternalPackage._build_and_install_current_dir_noegg)


class _ExternalGitRepo(ExternalPackage):
    """
    Parent class for any package which needs to pull a git repo.

    This class inherits from ExternalPackage only so we can sync git
    repos through the build_externals script. We do not reuse any of
    ExternalPackage's other methods. Any package that needs a git repo
    should subclass this and override build_and_install or fetch as
    they see appropriate.
    """

    os_requirements = {('/usr/bin/git') : 'git-core'}

    # All the chromiumos projects used on the lab servers should have a 'prod'
    # branch used to track the software version deployed in prod.
    PROD_BRANCH = 'prod'

    def is_needed(self, unused_install_dir):
        """Tell build_externals that we need to fetch."""
        # TODO(beeps): check if we're already upto date.
        return True


    def build_and_install(self, unused_install_dir):
        """
        Fall through method to install a package.

        Overwritten in base classes to pull a git repo.
        """
        raise NotImplementedError


    def fetch(self, unused_dest_dir):
        """Fallthrough method to fetch a package."""
        return True


class HdctoolsRepo(_ExternalGitRepo):
    """Clones or updates the hdctools repo."""

    module_name = 'servo'
    temp_hdctools_dir = tempfile.mktemp(suffix='hdctools')
    _GIT_URL = ('https://chromium.googlesource.com/'
                'chromiumos/third_party/hdctools')

    def fetch(self, unused_dest_dir):
        """
        Fetch repo to a temporary location.

        We use an intermediate temp directory to stage our
        installation because we only care about the servo package.
        If we can't get at the top commit hash after fetching
        something is wrong. This can happen when we've cloned/pulled
        an empty repo. Not something we expect to do.

        @parma unused_dest_dir: passed in because we inherit from
            ExternalPackage.

        @return: True if repo sync was successful.
        """
        git_repo = revision_control.GitRepo(
                        self.temp_hdctools_dir,
                        self._GIT_URL,
                        None,
                        abs_work_tree=self.temp_hdctools_dir)
        git_repo.reinit_repo_at(self.PROD_BRANCH)

        if git_repo.get_latest_commit_hash():
            return True
        return False


    def build_and_install(self, install_dir):
        """Reach into the hdctools repo and rsync only the servo directory."""

        servo_dir = os.path.join(self.temp_hdctools_dir, 'servo')
        if not os.path.exists(servo_dir):
            return False

        rv = self._rsync(servo_dir, os.path.join(install_dir, 'servo'))
        shutil.rmtree(self.temp_hdctools_dir)
        return rv


class ChromiteRepo(_ExternalGitRepo):
    """Clones or updates the chromite repo."""

    _GIT_URL = ('https://chromium.googlesource.com/chromiumos/chromite')

    def build_and_install(self, install_dir):
        """
        Clone if the repo isn't initialized, pull clean bits if it is.

        Unlike it's hdctools counterpart the chromite repo clones master
        directly into site-packages. It doesn't use an intermediate temp
        directory because it doesn't need installation.

        @param install_dir: destination directory for chromite installation.
        """
        local_chromite_dir = os.path.join(install_dir, 'chromite')
        git_repo = revision_control.GitRepo(
                local_chromite_dir,
                self._GIT_URL,
                abs_work_tree=local_chromite_dir)
        git_repo.reinit_repo_at(self.PROD_BRANCH)


        if git_repo.get_latest_commit_hash():
            return True
        return False


class DevServerRepo(_ExternalGitRepo):
    """Clones or updates the chromite repo."""

    _GIT_URL = ('https://chromium.googlesource.com/'
                'chromiumos/platform/dev-util')

    def build_and_install(self, install_dir):
        """
        Clone if the repo isn't initialized, pull clean bits if it is.

        Unlike it's hdctools counterpart the dev-util repo clones master
        directly into site-packages. It doesn't use an intermediate temp
        directory because it doesn't need installation.

        @param install_dir: destination directory for chromite installation.
        """
        local_devserver_dir = os.path.join(install_dir, 'devserver')
        git_repo = revision_control.GitRepo(local_devserver_dir, self._GIT_URL,
                                            abs_work_tree=local_devserver_dir)
        git_repo.reinit_repo_at(self.PROD_BRANCH)

        if git_repo.get_latest_commit_hash():
            return True
        return False


class BtsocketRepo(_ExternalGitRepo):
    """Clones or updates the btsocket repo."""

    _GIT_URL = ('https://chromium.googlesource.com/'
                'chromiumos/platform/btsocket')

    def fetch(self, unused_dest_dir):
        """
        Fetch repo to a temporary location.

        We use an intermediate temp directory because we have to build an
        egg for installation.  If we can't get at the top commit hash after
        fetching something is wrong. This can happen when we've cloned/pulled
        an empty repo. Not something we expect to do.

        @parma unused_dest_dir: passed in because we inherit from
            ExternalPackage.

        @return: True if repo sync was successful.
        """
        self.temp_btsocket_dir = autotemp.tempdir(unique_id='btsocket')
        try:
            git_repo = revision_control.GitRepo(
                            self.temp_btsocket_dir.name,
                            self._GIT_URL,
                            None,
                            abs_work_tree=self.temp_btsocket_dir.name)
            git_repo.reinit_repo_at(self.PROD_BRANCH)

            if git_repo.get_latest_commit_hash():
                return True
        except:
            self.temp_btsocket_dir.clean()
            raise

        self.temp_btsocket_dir.clean()
        return False


    def build_and_install(self, install_dir):
        """
        Install the btsocket module using setup.py

        @param install_dir: Target installation directory.

        @return: A boolean indicating success of failure.
        """
        work_dir = os.getcwd()
        try:
            os.chdir(self.temp_btsocket_dir.name)
            rv = self._build_and_install_current_dir_setup_py(install_dir)
        finally:
            os.chdir(work_dir)
            self.temp_btsocket_dir.clean()
        return rv
