# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob, logging, os, re, stat
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.graphics import graphics_utils


class camera_V4L2(test.test):
    version = 1
    preserve_srcdir = True
    v4l2_major_dev_num = 81
    v4l2_minor_dev_num_min = 0
    v4l2_minor_dev_num_max = 64


    def setup(self):
        # TODO(jiesun): make binary here when cross compile issue is resolved.
        os.chdir(self.srcdir)
        utils.make('clean')
        utils.make()


    def run_once(self, run_unit_tests=True, run_capture_tests=True,
                 run_default_capture_test=False, time=0,
                 assert_mandatory_controls=False):

        self.assert_mandatory_controls = assert_mandatory_controls
        self.find_video_capture_devices()
        time = time / len(self.v4l2_devices)

        for device in self.v4l2_devices:
            if run_unit_tests:
                self.run_v4l2_unittests(device)
            if run_capture_tests:
                self.run_v4l2_capture_tests(device)
            if run_default_capture_test:
                self.run_v4l2_default_capture_test(device, time)


    def is_v4l2_capture_device(self, device):
        executable = os.path.join(self.bindir, "media_v4l2_is_capture_device")
        cmd = "%s %s" % (executable, device)
        logging.info("Running %s" % cmd)
        return (utils.system(cmd, ignore_status=True) == 0)


    def find_video_capture_devices(self):
        self.v4l2_devices = []
        for device in glob.glob("/dev/video*"):
            statinfo = os.stat(device)
            if (stat.S_ISCHR(statinfo.st_mode) and
                os.major(statinfo.st_rdev) == self.v4l2_major_dev_num and
                os.minor(statinfo.st_rdev) >= self.v4l2_minor_dev_num_min and
                os.minor(statinfo.st_rdev) < self.v4l2_minor_dev_num_max and
                self.is_v4l2_capture_device(device)):
                self.v4l2_devices.append(device)
        logging.info("Detected devices: %s\n" % self.v4l2_devices)
        if not self.v4l2_devices:
            raise error.TestFail("No V4L2 devices found!")


    def unittest_passed(self, testname, stdout):
        return re.search(r"OK \] V4L2DeviceTest\." + testname, stdout);


    def run_v4l2_unittests(self, device):
        self.executable = os.path.join(self.bindir, "media_v4l2_unittest")
        cmd = "%s --device=%s" % (self.executable, device)
        logging.info("Running %s" % cmd)
        stdout = utils.system_output(cmd, retain_output=True)

        # Check the result of unittests.
        # We had exercise all the optional ioctls in unittest which maybe
        # optional by V4L2 Specification.  Therefore we need to check those
        # tests that we thought are mandatory.
        # 1. Multiple open should be supported for panel application.
        if not self.unittest_passed("MultipleOpen", stdout):
            raise error.TestError(device + " does not support multiple open!")

        # 2. Need to make sure this is really support or just driver error.
        if not self.unittest_passed("MultipleInit", stdout):
            raise error.TestError(device + " does support multiple init!")

        # 3. EnumInput and EnumStandard is optional.

        # 4. EnumControl is mandatory.
        if not self.unittest_passed("EnumControl", stdout):
            raise error.TestError(device + " does support enum controls!")
        pattern = re.compile(r"Control (\w+) is enabled\((\d+)-(\d+):(\d+)\)")
        control_info = pattern.findall(stdout)
        self.supported_controls = [ x[0] for x in control_info ]
        logging.info("Supported Controls: %s\n" % self.supported_controls)

        # TODO(jiesun): what is required?
        mandatory_controls = [
            "Brightness",
            "Contrast",
            "Saturation",
            "Hue",
            "Gamma"]
        for control in mandatory_controls:
            if self.assert_mandatory_controls and \
                control not in self.supported_controls:
                raise error.TestError(device + " does not support " + control)

        # 5. SetControl is mandatory.
        if not self.unittest_passed("SetControl", stdout):
            raise error.TestError(device + " does not support set controls!")

        # 6. 7. Set/GetCrop are both optional.

        # 8. ProbeCaps is mandatory.
        if not self.unittest_passed("ProbeCaps", stdout):
            raise error.TestError(device + " does not support probe caps!")

        if not re.search(r"support video capture interface.>>>", stdout):
            raise error.TestFail(device + " does not support video capture!")

        pattern = r"support streaming i/o interface.>>>"
        self.support_streaming = True if re.search(pattern, stdout) else False

        pattern = r"support streaming read/write interface.>>>"
        self.support_readwrite = True if re.search(pattern, stdout) else False

        # Currently I assume streaming (mmap) is mandatroy.
        if not self.support_streaming:
            raise error.TestFail(device + " does not support streaming!")

        # 9. EnumFormats is always mandatory.
        if not self.unittest_passed("EnumFormats", stdout):
            raise error.TestError(device + " does not support enum formats!")

        pattern = re.compile(r"supported format #\d+: .* \((....)\)")
        format_info = pattern.findall(stdout)
        # Remove duplicated pixel formats from list.
        self.supported_formats = list(set(format_info))
        logging.info("Supported pixel format: %s\n", self.supported_formats)

        # 10. Get/SetParam for framerate is optional.
        # 11. EnumFrameSize is optional on some kernel/v4l2 version.


    def run_v4l2_capture_test(self, fail_okay, options):
        executable = os.path.join(self.bindir, "media_v4l2_test")
        try:
            cmd = "%s %s" % (executable, " ".join(options))
            cmd = graphics_utils.xcommand(cmd)
            logging.info("Running %s" % cmd)
            stdout = utils.system_output(cmd, retain_output=True)
        except:
            if fail_okay:
                stdout = ""
                return (False, stdout)
            else:
                raise
        else:
            return (True, stdout)


    def run_v4l2_default_capture_test(self, device, time):
        options = ["--device=%s" % device ]
        if time:
            options.append("--time=%d" % time)
        okay, stdout = self.run_v4l2_capture_test(False, options)


    def run_v4l2_capture_tests(self, device):
        default_options = ["--device=%s" % device ]

        # If the device claims to support read/write i/o.
        if self.support_readwrite:
            option = default_options + ["--read"]
            okay, stdout = self.run_v4l2_capture_test(False, option)

        # If the device claims to support stream i/o.
        # This could mean either mmap stream i/o or user pointer stream i/o.
        if self.support_streaming:
            option = default_options + ["--mmap"]
            mmap_okay, stdout = self.run_v4l2_capture_test(True, option)

            option = default_options + ["--userp"]
            userp_okay, stdout = self.run_v4l2_capture_test(True, option)

            if not userp_okay and not mmap_okay:
                raise error.TestFail("Stream i/o failed!")


        # TODO(jiesun): test with different mandatory resultions that
        # the capture device must support without scaling by ourselves.
        required_resolutions = [
            (320, 240, 30),  # QVGA
            (640, 480, 30)]  # VGA
        for (width, height, minfps) in required_resolutions:
            # Note use default mmap i/o here.
            option = default_options[:]
            # Note use first supported pixel format.
            option.append("--pixel-format=%s" % self.supported_formats[0])
            option.append("--width=%s" % width)
            option.append("--height=%s" % height)
            okay, stdout = self.run_v4l2_capture_test(False, option)
            # Check if the actual format is desired.
            pattern = (r"actual format for capture (\d+)x(\d+)"
                       r" (....) picture at (\d+) fps")
            match = re.search(pattern, stdout)
            if (not match or
                int(match.group(1)) != width or
                int(match.group(2)) != height or
                match.group(3) != self.supported_formats[0] or
                int(match.group(4)) < minfps):
                raise error.TestError("capture test failed")

            okay, stdout = self.run_v4l2_capture_test(False, option)
