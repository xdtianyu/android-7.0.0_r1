# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse, common, datetime

from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import media_test_factory, \
    sequence_generator


def main():
    """
    Loads specified video in a chrome HTML5 player and collects its screenshots
    at specified time instances.
    """

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,

        description='Capture images of a video.',

        epilog='''
        This tool supports image-comparison-based video glitch detection tests.

        Use this tool to capture golden/reference images that will be used to
        verify test images captured during a video playback.

        Run this tool directly on the device.

        Copy tool first into /usr/local/autotest/cros so that autotest's
        common.py is imported so as to resolve autotest references correctly.

        Output images will be placed under /tmp/test. Images will be saved as
        hh_mm_ss_mss.png denoting the moment in time it was captured.

        ''')

    parser.add_argument("name",
                        help="Name of video to use.")

    parser.add_argument("format",
                        choices=['mp4', 'webm'],
                        help="Video format to use.")

    parser.add_argument("definition",
                        choices=['480p', '720p', '1080p', '720p_1080p'],
                        help="Video definition to use.")

    parser.add_argument("--start",
                        default="00:01",
                        help="Time to start taking screenshots. (mm:ss)")

    parser.add_argument("--stop",
                        help="Time to stop taking screenshots. (mm:ss).")

    parser.add_argument("interval",
                        type=int,
                        help="Seconds between two successive captures.")

    args = parser.parse_args()

    time_format = '%M:%S'

    # Parse time arguments from user
    # Start time has a default argument of 01:00, parse right away
    # Parse stop time later as we don't know the length of the video,
    # the factory does
    tmp = datetime.datetime.strptime(args.start, time_format)
    start = datetime.timedelta(minutes=tmp.minute, seconds=tmp.second)

    with chrome.Chrome() as cr:
        bindir = '/usr/local/autotest/cros/video'

        cr.browser.platform.SetHTTPServerDirectories(bindir)

        factory = media_test_factory.MediaTestFactory(
                      cr.browser.tabs[0],
                      cr.browser.platform.http_server,
                      bindir,
                      'dev',
                      args.name,
                      args.format,
                      args.definition)

        # if stop time is not specified, use the length of the video
        if args.stop is None:
            stop = factory.media_length
        else:
            tmp = datetime.datetime.strptime(args.stop, time_format)
            stop = datetime.timedelta(minutes=tmp.minute, seconds=tmp.second)

        file_utils.rm_dir_if_exists(factory.test_working_dir)

        file_utils.make_leaf_dir(factory.test_working_dir)

        seq = sequence_generator.generate_interval_sequence(start,
                                                            stop,
                                                            args.interval)

        collector = factory.make_video_screenshot_collector()

        collector.ensure_player_is_ready()

        collector.collect_multiple_screenshots(seq)


if __name__ == '__main__':
    main()