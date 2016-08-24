# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common, constants, logging, os, socket, stat, sys, threading, time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

class LocalDns(object):
    """A wrapper around miniFakeDns that runs the server in a separate thread
    and redirects all DNS queries to it.
    """
    # This is a symlink.  We look up the real path at runtime by following it.
    _resolv_bak_file = 'resolv.conf.bak'

    def __init__(self, fake_ip="127.0.0.1", local_port=53):
        import miniFakeDns  # So we don't need to install it in the chroot.
        self._dns = miniFakeDns.DNSServer(fake_ip=fake_ip, port=local_port)
        self._stopper = threading.Event()
        self._thread = threading.Thread(target=self._dns.run,
                                        args=(self._stopper,))

    def __get_host_by_name(self, hostname):
        """Resolve the dotted-quad IPv4 address of |hostname|

        This used to use suave python code, like this:
            hosts = socket.getaddrinfo(hostname, 80, socket.AF_INET)
            (fam, socktype, proto, canonname, (host, port)) = hosts[0]
            return host

        But that hangs sometimes, and we don't understand why.  So, use
        a subprocess with a timeout.
        """
        try:
            host = utils.system_output('%s -c "import socket; '
                                       'print socket.gethostbyname(\'%s\')"' % (
                                       sys.executable, hostname),
                                       ignore_status=True, timeout=2)
        except Exception as e:
            logging.warning(e)
            return None
        return host or None

    def __attempt_resolve(self, hostname, ip, expected=True):
        logging.debug('Attempting to resolve %s to %s' % (hostname, ip))
        host = self.__get_host_by_name(hostname)
        logging.debug('Resolve attempt for %s got %s' % (hostname, host))
        return host and (host == ip) == expected

    def run(self):
        """Start the mock DNS server and redirect all queries to it."""
        self._thread.start()
        # Redirect all DNS queries to the mock DNS server.
        try:
            # Follow resolv.conf symlink.
            resolv = os.path.realpath(constants.RESOLV_CONF_FILE)
            # Grab path to the real file, do following work in that directory.
            resolv_dir = os.path.dirname(resolv)
            resolv_bak = os.path.join(resolv_dir, self._resolv_bak_file)
            resolv_contents = 'nameserver 127.0.0.1'
            # Test to make sure the current resolv.conf isn't already our
            # specially modified version.  If this is the case, we have
            # probably been interrupted while in the middle of this test
            # in a previous run.  The last thing we want to do at this point
            # is to overwrite a legitimate backup.
            if (utils.read_one_line(resolv) == resolv_contents and
                os.path.exists(resolv_bak)):
                logging.error('Current resolv.conf is setup for our local '
                              'server, and a backup already exists!  '
                              'Skipping the backup step.')
            else:
                # Back up the current resolv.conf.
                os.rename(resolv, resolv_bak)
            # To stop flimflam from editing resolv.conf while we're working
            # with it, we want to make the directory -r-xr-xr-x.  Open an
            # fd to the file first, so that we'll retain the ability to
            # alter it.
            resolv_fd = open(resolv, 'w')
            self._resolv_dir_mode = os.stat(resolv_dir).st_mode
            os.chmod(resolv_dir, (stat.S_IRUSR | stat.S_IXUSR |
                                  stat.S_IRGRP | stat.S_IXGRP |
                                  stat.S_IROTH | stat.S_IXOTH))
            resolv_fd.write(resolv_contents)
            resolv_fd.close()
            assert utils.read_one_line(resolv) == resolv_contents
        except Exception as e:
            logging.error(str(e))
            raise e

        utils.poll_for_condition(
            lambda: self.__attempt_resolve('www.google.com.', '127.0.0.1'),
            utils.TimeoutError('Timed out waiting for DNS changes.'),
            timeout=10)

    def stop(self):
        """Restore the backed-up DNS settings and stop the mock DNS server."""
        try:
            # Follow resolv.conf symlink.
            resolv = os.path.realpath(constants.RESOLV_CONF_FILE)
            # Grab path to the real file, do following work in that directory.
            resolv_dir = os.path.dirname(resolv)
            resolv_bak = os.path.join(resolv_dir, self._resolv_bak_file)
            os.chmod(resolv_dir, self._resolv_dir_mode)
            if os.path.exists(resolv_bak):
                os.rename(resolv_bak, resolv)
            else:
                # This probably means shill restarted during the execution
                # of our test, and has cleaned up the .bak file we created.
                raise error.TestError('Backup file %s no longer exists!  '
                                      'Connection manager probably crashed '
                                      'during the test run.' %
                                      resolv_bak)

            utils.poll_for_condition(
                lambda: self.__attempt_resolve('www.google.com.',
                                               '127.0.0.1',
                                               expected=False),
                utils.TimeoutError('Timed out waiting to revert DNS.  '
                                   'resolv.conf contents are: ' +
                                   utils.read_one_line(resolv)),
                timeout=10)
        finally:
            # Stop the DNS server.
            self._stopper.set()
            self._thread.join()
