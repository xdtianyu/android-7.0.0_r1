# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, multiprocessing, os, time
import numpy
from autotest_lib.client.bin import test
from autotest_lib.client.cros import sys_power
from autotest_lib.client.cros.camera import camera_utils
from autotest_lib.client.common_lib import error

try:
    # HACK: We need to succeed if OpenCV is missing to allow "emerge
    # autotest-tests" to succeed, as OpenCV is not available in the build
    # environment. It is available on the target where the test actually runs.
    import cv2
except ImportError:
    pass


def async_suspend():
    try:
        time.sleep(5) # allow some time to start capturing
        sys_power.kernel_suspend(10)
    except:
        # Any exception will be re-raised in main process, but the stack trace
        # will be wrong. Log it here with the correct stack trace.
        logging.exception('suspend raised exception')
        raise


class power_CameraSuspend(test.test):
    """Test camera before & after suspend."""

    version = 1

    def run_once(self, save_images=False):
        # open the camera via opencv
        cam_name, cam_index = camera_utils.find_camera()
        if cam_index is None:
            raise error.TestError('no camera found')
        cam = cv2.VideoCapture(cam_index)

        # kick off async suspend
        logging.info('starting subprocess to suspend system')
        pool = multiprocessing.Pool(processes=1)
        # TODO(spang): Move async suspend to library.
        result = pool.apply_async(async_suspend)

        # capture images concurrently with suspend
        capture_start = time.time()
        logging.info('start capturing at %d', capture_start)
        image_count = 0
        resume_count = None
        last_image = None

        while True:
            # terminate if we've captured a few frames after resume
            if result.ready() and resume_count is None:
                result.get() # reraise exception, if any
                resume_count = image_count
                logging.info('suspend task finished')
            if resume_count is not None and image_count - resume_count >= 10:
                break

            # capture one frame
            image_ok, image = cam.read()
            image_count += 1
            if not image_ok:
                logging.error('failed capture at image %d', image_count)
                raise error.TestFail('image capture failed from %s', cam_name)

            # write image to disk, if requested
            if save_images and image_count <= 200:
                path = os.path.join(self.outputdir, '%03d.jpg' % image_count)
                cv2.imwrite(path, image)

            # verify camera produces a unique image on each capture
            if last_image is not None and numpy.array_equal(image, last_image):
                raise error.TestFail('camera produced two identical images')
            last_image = image

        capture_end = time.time()
        logging.info('done capturing at %d', capture_end)

        logging.info('captured %d frames in %d seconds',
                     image_count, capture_end - capture_start)
