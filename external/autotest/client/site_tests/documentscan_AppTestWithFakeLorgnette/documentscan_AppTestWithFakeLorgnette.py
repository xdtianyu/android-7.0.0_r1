# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import mock_lorgnette
import os

from autotest_lib.client.cros import touch_playback_test_base
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome


class documentscan_AppTestWithFakeLorgnette(
        touch_playback_test_base.touch_playback_test_base):
    """ Test that an extension using the DocumentScan Chrome API can
        successfully retrieve a scanned document from a mocked version
        of the lorgnette daemon.
    """
    version = 1

    # Application ID of the test scan application.
    _APP_ID = 'mljeglgkknlanoeffbeehogdhkhnaidk'

    # Document to open in order to launch the scan application.
    _APP_DOCUMENT = 'scan.html'

    # Window ID that references the scan application window.
    _APP_WINDOW_ID = 'ChromeApps-Sample-Document-Scan'

    # Element within the scan application document that contains image scans.
    _APP_SCANNED_IMAGE_ELEMENT = 'scannedImages'

    # Description of the fake mouse we add to the system.
    _MOUSE_DESCRIPTION = 'amazon_mouse.prop'

    # This input file was created as follows:
    #  - Insert USB mouse (in this case the Amazon mouse)
    #  - head /sys/class/input/*/name | grep -iB1 mouse
    #    This will give you the /sys/class/inputXX for the mouse.
    #  - evemu-record /dev/input/eventXX -1 > /tmp/button_click.event
    #    Move the mouse diagonally upwards to the upper left, move
    #    down and right a bit then click.
    _PLAYBACK_FILE = 'button_click.event'

    # Image file to serve up to Chrome in response to a scan request.
    _IMAGE_FILENAME = 'lorgnette-test.png'

    # Expected prefix for the SRC tag of the scanned images.
    _BASE64_IMAGE_HEADER = 'data:image/png;base64,'

    def _play_events(self, event_filename):
        """Simulate mouse events since the Chrome API enforces that
        the scan action come from a user gesture.

        @param event_filename string filename containing events to play back
        """

        file_path = os.path.join(self.bindir, event_filename)
        self._blocking_playback(file_path, touch_type='mouse')


    def _launch_app(self, chrome_instance):
        """Launches the sample scanner Chrome app.

        @param chrome_instance object of type chrome.Chrome
        """

        self._extension = chrome_instance.get_extension(self._extension_path)

        # TODO(pstew): chrome.management.launchApp() would have been
        # ideal here, but is not available even after adding the
        # "management" permission to the app.  Instead, we perform
        # the launch action of the extension directly.
        cmd = '''
            chrome.app.window.create('%s', {
              singleton: true,
              id: '%s',
              state: 'fullscreen'
            });
        ''' % (self._APP_DOCUMENT, self._APP_WINDOW_ID)
        self._extension.ExecuteJavaScript(cmd)


    def _query_scan_element(self, query):
        """Queries the "scannedImages" element within the app window.

        @param query string javascript query to execute on the DIV element.
        """

        cmd = '''
           app_window = chrome.app.window.get('%s');
           element = app_window.contentWindow.document.getElementById('%s');
           element.%s;
        ''' % (self._APP_WINDOW_ID, self._APP_SCANNED_IMAGE_ELEMENT, query)
        return self._extension.EvaluateJavaScript(cmd)


    def _get_scan_count(self):
        """Counts the number of successful scanned images displayed.

        @param chrome_instance object of type chrome.Chrome
        """

        result = self._query_scan_element('childNodes.length')

        # Subtract 1 for the text node member of the DIV element.
        return int(result) - 1


    def _validate_image_data(self, expected_image_data):
        """Validates that the scanned image displayed by the app is the same
        as the image provided by the fake lorgnette daemon.
        """

        image_src = self._query_scan_element('childNodes[0].src')
        if not image_src.startswith(self._BASE64_IMAGE_HEADER):
            raise error.TestError(
                    'Image SRC does not start with base64 data header: %s' %
                    image_src)

        base64_data = image_src[len(self._BASE64_IMAGE_HEADER):]
        data = base64.b64decode(base64_data)
        if expected_image_data != data:
            raise error.TestError('Image data from tag is not the same as '
                                  'the test image data')


    def _validate_mock_method_calls(self, calls):
        """Validate the method calls made on the lorgnette mock instance.

        @param calls list of MethodCall named tuples from mock lorgnette.
        """

        if len(calls) != 2:
            raise error.TestError('Expected 2 method calls but got: %r' % calls)

        for index, method_name in enumerate(['ListScanners', 'ScanImage']):
            if calls[index].method != method_name:
                raise error.TestError('Call #%d was %s instead of expected %s' %
                                      (index, calls[index].method, method_name))


    def run_once(self):
        """Entry point of this test."""
        mouse_file = os.path.join(self.bindir, self._MOUSE_DESCRIPTION)
        self._emulate_mouse(property_file=mouse_file)

        self._extension_path = os.path.join(os.path.dirname(__file__),
                                            'document_scan_test_app')

        with chrome.Chrome(extension_paths=[self._extension_path],
                           is_component=False) as chrome_instance:
            img = os.path.join(self.bindir, self._IMAGE_FILENAME)
            with mock_lorgnette.MockLorgnette(img) as lorgnette_instance:
                self._launch_app(chrome_instance)

                self._play_events(self._PLAYBACK_FILE)

                scan_count = self._get_scan_count()
                if scan_count != 1:
                    raise error.TestError('Scan count is %d instead of 1' %
                                          scan_count)

                self._validate_image_data(lorgnette_instance.image_data)
                self._validate_mock_method_calls(
                        lorgnette_instance.get_method_calls())
