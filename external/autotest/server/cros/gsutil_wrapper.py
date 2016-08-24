# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging
import os.path
import subprocess
import tempfile

def copy_private_bucket(host, bucket, filename, destination, timeout_s=30):
    """
    Copies files/directories from a private google storage location to the DUT.
    Uses a test server box as a temp location.
    We do this because it's easier than trying to get the client DUT
    authenticated. The Test server is already authenticated, so copy to the test
    server and then send file to client.

    @param host: Autotest host machine object.
    @param bucket: path to name of gs bucket.
    @param filename: string, name of the file or dir in 'bucket' to copy.
    @param destination: path in DUT where the file should be copied to.
    @param timeout_s: int, timeout in seconds to wait for copy to finish

    """

    assert (bucket.startswith('gs://'))
    assert (os.path.isdir(destination))

    src = os.path.join(bucket, filename)

    log("SOURCE path: " + src)

    with tempfile.NamedTemporaryFile(suffix='.wpr') as tempsource:
        tempsourcepath = tempsource.name

        args = ['gsutil', 'cp', src, tempsourcepath]
        log("Copying to temporary test server destination : " + tempsourcepath)

        p = subprocess.Popen(args,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)

        output = p.communicate()

        log("STDOUT | " + output[0])
        log("STDERR | " + output[1])

        if p.returncode:
            raise subprocess.CalledProcessError(returncode=p.returncode,
                                                cmd=args)

        host.send_file(tempsourcepath, os.path.join(destination, filename))
        log("Sent file to DUT : " + host.hostname)


def log(message):
    """
    Wraps around logging.debug() and adds a prefix to show that messages are
    coming from this utility.

    @param message: string, the message to log.

    """
    message = "| gs wrapper | " + message
    logging.debug(message)