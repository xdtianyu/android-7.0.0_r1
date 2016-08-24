# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob, os
import numpy as np

# Dimension padding/unpadding function for converting points matrices to
# the OpenCV format (channel-based).
def Pad(x):
    return np.expand_dims(x, axis=0)


def Unpad(x):
    return np.squeeze(x)


class Pod(object):
    '''A POD (plain-old-data) object containing arbitrary fields.'''
    def __init__(self, **args):
        self.__dict__.update(args)

    def __repr__(self):
        '''Returns a representation of the object, including its properties.'''
        return (self.__class__.__name__ + '(' +
        ', '.join('%s=%s' % (k, v) for k, v in sorted(self.__dict__.items())
                  if not k.startswith('_')) + ')')


def find_camera():
    """
    Find a V4L camera device.

    @return (device_name, device_index). If no camera is found, (None, None).
    """
    cameras = [os.path.basename(camera) for camera in
               glob.glob('/sys/bus/usb/drivers/uvcvideo/*/video4linux/video*')]
    if not cameras:
        return None, None
    camera = cameras[0]
    return camera, int(camera[5:])
