#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import BaseHTTPServer
import urlparse

class TestEndpointHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    """
    A web server that is used by cellular tests.  It serves up the following
    pages:
            - http://<ip>/generate_204
              This URL is used by shill's portal detection.

            - http://<ip>/download?size=%d
              Tests use this URL to download an arbitrary amount of data.

    """
    GENERATE_204_PATH = '/generate_204'
    DOWNLOAD_URL_PATH = '/download'
    SIZE_PARAM = 'size'

    def do_GET(self):
        """Handles GET requests."""
        url = urlparse.urlparse(self.path)
        print 'URL: %s' % url.path
        if url.path == self.GENERATE_204_PATH:
            self.send_response(204)
        elif url.path == self.DOWNLOAD_URL_PATH:
            parsed_query = urlparse.parse_qs(url.query)
            if self.SIZE_PARAM not in parsed_query:
                pass
            self.send_response(200)
            self.send_header('Content-type', 'application/octet-stream')
            self.end_headers()
            self.wfile.write('0' * int(parsed_query[self.SIZE_PARAM][0]))
        else:
            print 'Unsupported URL path: %s' % url.path


def main():
    """Main entry point when this script is run from the command line."""
    try:
        server = BaseHTTPServer.HTTPServer(('', 80), TestEndpointHandler)
        server.serve_forever()
    except KeyboardInterrupt:
        server.socket.close()


if __name__ == '__main__':
    main()
