# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import utils
from telemetry.internal.util import webpagereplay


class WebPageReplayWrapper(object):
    """
    Wraps around WPR Server to be conveniently used in autotest.

    """

    _REPLAY_HOST = "127.0.0.1"


    def __init__(self, archive_path):
        """
        Creates a WPR server using archive_path and pre-set arguments.

        @param archive_path: path to the .wpr archive to be used.

        """

        port = utils.get_unused_port()
        self._http_port = port if port else 8080

        port = utils.get_unused_port()
        self._https_port = port if port else 8713

        self._server = webpagereplay.ReplayServer(
                archive_path=archive_path,
                replay_host=WebPageReplayWrapper._REPLAY_HOST,
                http_port=self._http_port,
                https_port=self._https_port,
                dns_port=None,
                replay_options=[])


    @property
    def chrome_flags_for_wpr(self):
        """
        @return: list of Chrome flags needed to direct traffic to WPR server.

        """
        return ['--host-resolver-rules=MAP * %s, EXCLUDE localhost' %
                WebPageReplayWrapper._REPLAY_HOST,
                '--testing-fixed-http-port=%s' % self._http_port,
                '--testing-fixed-https-port=%s' % self._https_port,
                '--ignore-certificate-errors']


    def __enter__(self):
        return self._server.__enter__()


    def __exit__(self, exc_type, exc_val, exc_tb):
        return self._server.__exit__(exc_type, exc_val, exc_tb)


class NullWebPageReplayWrapper(object):
    """
    Empty class. Created to simply clients code, no other purpose.

    Client will do:
    with chrome.Chrome() as cr, wpr_server:
       ....

    When we are not using WPR we will return this empty class, leaving client's
    code uniform and unchanged.

    """

    @property
    def chrome_flags_for_wpr(self):
        """
        @return: an empty list. This is an empty class.

        """
        return []


    def __enter__(self):
        return self


    def __exit__(self, exc_type, exc_val, exc_tb):
        pass
