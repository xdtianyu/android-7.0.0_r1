# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import shutil


from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import test
from autotest_lib.server.cros import sonic_extension_downloader


class sonic_AppTest(test.test):
    """Tests that a sonic device can start its apps."""
    version = 1


    def initialize(self, sonic_host, extension_dir=None):
        """Download the latest extension, or use a local path if specified."""
        # TODO: crbug.com/337708
        if not extension_dir:
            logging.info('Downloading ToT extension for test since no local '
                         'extension specified.')
            extension_path = os.path.join(self.job.clientdir, 'deps',
                                          'sonic_extension')
            sonic_extension_downloader.setup_extension(extension_path)
            self.extension_path = extension_path
        else:
            logging.info('Using local extension for test %s.',
                         self.extension_dir)
            self.extension_path = None


    def run_once(self, cros_host, sonic_host, app='ChromeCast', payload=None,
                 extension_dir=None):
        """Sonic test to start an app.

        By default this test will test tab cast by installing an extension
        on the cros host and using chromedriver to cast a tab. If another app
        is specified, like YouTube or Netflix, the app is tested directly
        through the server running on port 8080 on the sonic device.

        @param app: The name of the application to start.
            eg: YouTube
        @param payload: The payload to send to the app.
            eg: http://www.youtube.com
        @param extension_dir: The directory to load a custom extension from.

        @raises CmdExecutionError: If a command failed to execute on the host.
        @raises TestError: If the app didn't start, or the app was unrecognized,
            or the payload is invalid.
        """
        logging.info('Testing app %s, sonic_host %s and chromeos device %s ',
                     app, sonic_host.hostname, cros_host.hostname)
        if app == 'ChromeCast':
            sonic_host.enable_test_extension()
            client_at = autotest.Autotest(cros_host)
            client_at.run_test('desktopui_SonicExtension',
                               chromecast_ip=sonic_host.hostname,
                               extension_dir=extension_dir)
        elif payload and (app == 'Netflix' or app == 'YouTube'):
            sonic_host.run('logcat -c')
            sonic_host.client.start_app(app, payload)
            log = sonic_host.run('logcat -d').stdout
            app_started_confirmation = 'App started:'
            for line in log.split('\n'):
                if app_started_confirmation in line:
                    logging.info('Successfully started app: %s', line)
                    break
            else:
                logging.error(log)
                raise error.TestError('App %s failed to start' % app)
        else:
            raise error.TestError('Cannot start app %s with payload %s' %
                                  (app, payload))


    def cleanup(self, cros_host, sonic_host, app='ChromeCast'):
        sonic_host.client.stop_app(app)
        if self.extension_path:
            shutil.rmtree(self.extension_path, ignore_errors=True)
