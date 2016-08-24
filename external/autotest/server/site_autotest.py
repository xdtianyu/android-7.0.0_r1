# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import tempfile
import urllib2

from autotest_lib.client.common_lib import error, global_config
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server import installable_object, autoserv_parser
from autotest_lib.server import utils as server_utils
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.server.cros.dynamic_suite.constants import JOB_REPO_URL


_CONFIG = global_config.global_config
_PARSER = autoserv_parser.autoserv_parser


class SiteAutotest(installable_object.InstallableObject):
    """Site implementation of Autotest."""

    def get(self, location=None):
        if not location:
            location = os.path.join(self.serverdir, '../client')
            location = os.path.abspath(location)
        installable_object.InstallableObject.get(self, location)
        self.got = True


    def _get_fetch_location_from_host_attribute(self):
        """Get repo to use for packages from host attribute, if possible.

        Hosts are tagged with an attribute containing the URL
        from which to source packages when running a test on that host.
        If self.host is set, attempt to look this attribute up by calling out
        to the AFE.

        @returns value of the 'job_repo_url' host attribute, if present.
        """
        try:
            from autotest_lib.server import frontend
            if self.host:
                afe = frontend.AFE(debug=False)
                hosts = afe.get_hosts(hostname=self.host.hostname)
                if hosts and JOB_REPO_URL in hosts[0].attributes:
                    return hosts[0].attributes[JOB_REPO_URL]
                logging.warning("No %s for %s", JOB_REPO_URL, self.host)
        except (ImportError, urllib2.URLError):
            logging.warning('Not attempting to look for %s', JOB_REPO_URL)
            pass
        return None


    def get_fetch_location(self):
        """Generate list of locations where autotest can look for packages.

        Old n' busted: Autotest packages are always stored at a URL that can
        be derived from the one passed via the voodoo magic --image argument.
        New hotness: Hosts are tagged with an attribute containing the URL
        from which to source packages when running a test on that host.

        @returns the list of candidate locations to check for packages.
        """
        repos = super(SiteAutotest, self).get_fetch_location()

        if _PARSER.options.image:
            image_opt = _PARSER.options.image
            if image_opt.startswith('http://'):
                # A devserver HTTP url was specified, set that as the repo_url.
                repos.append(image_opt.replace(
                    'update', 'static').rstrip('/') + '/autotest')
            else:
                # An image_name like stumpy-release/R27-3437.0.0 was specified,
                # set this as the repo_url for the host. If an AFE is not being
                # run, this will ensure that the installed build uses the
                # associated artifacts for the test specified when running
                # autoserv with --image. However, any subsequent tests run on
                # the host will no longer have the context of the image option
                # and will revert back to utilizing test code/artifacts that are
                # currently present in the users source checkout.
                devserver_url = dev_server.ImageServer.resolve(image_opt).url()
                repo_url = tools.get_package_url(devserver_url, image_opt)
                repos.append(repo_url)
        elif not server_utils.is_inside_chroot():
            # Only try to get fetch location from host attribute if the test
            # is not running inside chroot.
            # No --image option was specified, look for the repo url via
            # the host attribute. If we are not running with a full AFE
            # autoserv will fall back to serving packages itself from whatever
            # source version it is sync'd to rather than using the proper
            # artifacts for the build on the host.
            found_repo = self._get_fetch_location_from_host_attribute()
            if found_repo is not None:
                # Add our new repo to the end, the package manager will
                # later reverse the list of repositories resulting in ours
                # being first
                repos.append(found_repo)

        return repos


    def install(self, host=None, autodir=None, use_packaging=True):
        """Install autotest.  If |host| is not None, stores it in |self.host|.

        @param host A Host instance on which autotest will be installed
        @param autodir Location on the remote host to install to
        @param use_packaging Enable install modes that use the packaging system.

        """
        if host:
            self.host = host

        super(SiteAutotest, self).install(host=host, autodir=autodir,
                                          use_packaging=use_packaging)


    def _install(self, host=None, autodir=None, use_autoserv=True,
                 use_packaging=True):
        """
        Install autotest.  If get() was not called previously, an
        attempt will be made to install from the autotest svn
        repository.

        @param host A Host instance on which autotest will be installed
        @param autodir Location on the remote host to install to
        @param use_autoserv Enable install modes that depend on the client
            running with the autoserv harness
        @param use_packaging Enable install modes that use the packaging system

        @exception AutoservError if a tarball was not specified and
            the target host does not have svn installed in its path
        """
        # TODO(milleral): http://crbug.com/258161
        super(SiteAutotest, self)._install(host, autodir, use_autoserv,
                                           use_packaging)
        # Send over the most recent global_config.ini after installation if one
        # is available.
        # This code is a bit duplicated from
        # _BaseRun._create_client_config_file, but oh well.
        if self.installed and self.source_material:
            logging.info('Installing updated global_config.ini.')
            destination = os.path.join(self.host.get_autodir(),
                                       'global_config.ini')
            with tempfile.NamedTemporaryFile() as client_config:
                config = global_config.global_config
                client_section = config.get_section_values('CLIENT')
                client_section.write(client_config)
                client_config.flush()
                self.host.send_file(client_config.name, destination)


    def run_static_method(self, module, method, results_dir='.', host=None,
                          *args):
        """Runs a non-instance method with |args| from |module| on the client.

        This method runs a static/class/module autotest method on the client.
        For example:
          run_static_method("autotest_lib.client.cros.cros_ui", "reboot")

        Will run autotest_lib.client.cros.cros_ui.reboot() on the client.

        @param module: module name as you would refer to it when importing in a
            control file. e.g. autotest_lib.client.common_lib.module_name.
        @param method: the method you want to call.
        @param results_dir: A str path where the results should be stored
            on the local filesystem.
        @param host: A Host instance on which the control file should
            be run.
        @param args: args to pass to the method.
        """
        control = "\n".join(["import %s" % module,
                             "%s.%s(%s)\n" % (module, method,
                                              ','.join(map(repr, args)))])
        self.run(control, results_dir=results_dir, host=host)


class SiteClientLogger(object):
    """Overrides default client logger to allow for using a local package cache.
    """

    def _process_line(self, line):
        """Returns the package checksum file if it exists."""
        logging.debug(line)
        fetch_package_match = self.fetch_package_parser.search(line)
        if fetch_package_match:
            pkg_name, dest_path, fifo_path = fetch_package_match.groups()
            serve_packages = _CONFIG.get_config_value(
                "PACKAGES", "serve_packages_from_autoserv", type=bool)
            if serve_packages and pkg_name == 'packages.checksum':
                try:
                    checksum_file = os.path.join(
                        self.job.pkgmgr.pkgmgr_dir, 'packages', pkg_name)
                    if os.path.exists(checksum_file):
                        self.host.send_file(checksum_file, dest_path)
                except error.AutoservRunError:
                    msg = "Package checksum file not found, continuing anyway"
                    logging.exception(msg)

                try:
                    # When fetching a package, the client expects to be
                    # notified when the fetching is complete. Autotest
                    # does this pushing a B to a fifo queue to the client.
                    self.host.run("echo B > %s" % fifo_path)
                except error.AutoservRunError:
                    msg = "Checksum installation failed, continuing anyway"
                    logging.exception(msg)
                finally:
                    return

        # Fall through to process the line using the default method.
        super(SiteClientLogger, self)._process_line(line)


    def _send_tarball(self, pkg_name, remote_dest):
        """Uses tarballs in package manager by default."""
        try:
            server_package = os.path.join(self.job.pkgmgr.pkgmgr_dir,
                                          'packages', pkg_name)
            if os.path.exists(server_package):
              self.host.send_file(server_package, remote_dest)
              return

        except error.AutoservRunError:
            msg = ("Package %s could not be sent from the package cache." %
                   pkg_name)
            logging.exception(msg)

        # Fall through to send tarball the default method.
        super(SiteClientLogger, self)._send_tarball(pkg_name, remote_dest)


class _SiteRun(object):
    pass
