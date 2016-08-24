# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome


class desktopui_CameraApp(test.test):
    """Tests if the Camera App works correctly."""
    version = 1
    preserve_srcdir = True


    def setup(self):
        """Fetches and builds the ToT Camera App."""
        os.chdir(self.srcdir)
        utils.make('clean')
        utils.make('all')


    def run_once(self):
        """Runs the integration test."""
        # Create the browser instance.
        camera_path = os.path.join(os.path.dirname(__file__),
                                   'src/camera/build/camera')
        browser = chrome.Chrome(extension_paths=[camera_path],
                                is_component=False)

        # Start the Camera app.
        extension = browser.get_extension(camera_path)
        extension.ExecuteJavaScript('camera.bg.createForTesting();')

        # Wait until the Camera app acquires the stream.
        js_is_capturing = (
                'camera.bg.appWindow && '
                'camera.bg.appWindow.contentWindow.camera && '
                'camera.bg.appWindow.contentWindow.camera.Camera && '
                'camera.bg.appWindow.contentWindow.camera.Camera.getInstance().'
                'cameraView.capturing')

        # Verify if the camera initializes correctly in up to 30 seconds.
        utils.poll_for_condition(
                condition=lambda: extension.EvaluateJavaScript(js_is_capturing),
                exception=error.TestError('Camera initialization timed out.'),
                sleep_interval=1,
                timeout=30)

