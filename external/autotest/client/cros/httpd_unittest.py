#!/usr/bin/python

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""HTTPlistener unittest."""

import logging, os, sys, threading, urllib, unittest

from httpd import HTTPListener, SecureHTTPListener

GET_TEST_PATH = '/get_test'

def run_get_test(test_server, url):
    err = 1
    get_done = test_server.add_wait_url(GET_TEST_PATH)
    get_resp = ''
    try:
        get_resp = urllib.urlopen(url).read()
    except IOError, e:
        pass
    if not (get_done.is_set() and get_resp):
        print 'FAILED'
    else:
        print 'PASSED'
        err = 0
    return err


def test():
    test_server = HTTPListener(8000, docroot='/tmp')
    post_done = test_server.add_wait_url("/post_test",
                                         matchParams={'test': 'passed'})
    def _Spam():
        while not post_done.is_set():
            print 'TEST: server running'
            post_done.wait()
        return
    test_server.run()
    t = threading.Thread(target=_Spam).start()
    params = urllib.urlencode({'test': 'passed'})
    err = 1

    # TODO(seano): This test doesn't seem to pass.
    post_resp = ''
    try:
        post_resp = urllib.urlopen('http://localhost:8000/post_test',
                                   params).read()
    except IOError, e:
        pass
    if not (post_done.is_set() and
            test_server.get_form_entries()['test'] != 'passed'):
        print 'FAILED'
    else:
        print 'PASSED'
        err = 0


    err = run_get_test(test_server, 'http://localhost:8000' + GET_TEST_PATH)
    test_server.stop()
    if err != 0:
        return err

    creds_path = (os.path.dirname(os.path.realpath( __file__)) +
                  '/httpd_unittest_server')
    ssl_port=50000
    test_server = SecureHTTPListener(port=ssl_port,
                                     cert_path=(creds_path+'.pem'),
                                     key_path=(creds_path+'.key'))
    test_server.run()
    err = run_get_test(test_server,
                       'https://localhost:%d%s' % (ssl_port, GET_TEST_PATH))
    test_server.stop()
    return err


def run_server():
    """Example method showing how to start a HTTPListener."""
    test_server = HTTPListener(8000, docroot='/tmp')
    latch = test_server.add_wait_url('/quitquitquit')
    test_server.run()
    logging.info('server started')
    while not latch.is_set():
        try:
            latch.wait(1)
        except KeyboardInterrupt:
            sys.exit()
    test_server.stop()
    return


if __name__ == '__main__':
    if len(sys.argv) > 1:
        run_server()
    else:
        test()


if __name__ == '__main__':
    unittest.main()
