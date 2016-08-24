# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime, logging, os, time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib import sequence_utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import constants as cros_constants
from autotest_lib.client.cros.chameleon import chameleon
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_video_capturer
from autotest_lib.client.cros.image_comparison import publisher
from autotest_lib.client.cros.video import constants
from autotest_lib.client.cros.video import frame_checksum_utils
from autotest_lib.client.cros.video import native_html5_player
from autotest_lib.client.cros.multimedia import local_facade_factory


class video_GlitchDetection(test.test):
    """
    Video playback test using image comparison.

    Captures frames using chameleon and compares them to known golden frames.

    If frames don't match, upload to GS for viewing later.

    """
    version = 2


    def run_once(self, source_path, codec, resolution, host, args,
                 collect_only = False):

        board = utils.get_current_board()

        file_utils.make_leaf_dir(constants.TEST_DIR)

        with chrome.Chrome(extension_paths = [
            cros_constants.MULTIMEDIA_TEST_EXTENSION], autotest_ext = True) as cr:

            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            html_fullpath = os.path.join(self.bindir, 'video.html')
            player = native_html5_player.NativeHtml5Player(
                tab = cr.browser.tabs[0],
                full_url = cr.browser.platform.http_server.UrlOf(html_fullpath),
                video_id = 'video',
                video_src_path = source_path,
                event_timeout = 120)

            chameleon_board = chameleon.create_chameleon_board(host.hostname,
                                                               args)
            display_facade = local_facade_factory.LocalFacadeFactory(
                cr).create_display_facade()

            finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, display_facade)

            capturer = chameleon_video_capturer.ChameleonVideoCapturer(
                finder.find_port(interface = 'hdmi'), display_facade)

            with capturer:
                player.load_video()

                player.verify_video_can_play()

                display_facade.move_to_display(
                    display_facade.get_first_external_display_index())
                display_facade.set_fullscreen(True)
                # HACK: Unset and reset fullscreen. There is a bug in Chrome
                # that fails to move the window to a correct position.
                # Resetting fullscren helps, check http://crbug.com/574284
                display_facade.set_fullscreen(False)
                display_facade.set_fullscreen(True)
                time.sleep(5)

                box = (0, 0, constants.DESIRED_WIDTH, constants.DESIRED_HEIGHT)

                #TODO: mussa, Revisit once crbug/580736 is fixed
                for n in xrange(constants.NUM_CAPTURE_TRIES):
                    logging.debug('Trying to capture frames. TRY #%d', n + 1)
                    raw_test_checksums = capturer.capture_only(
                        player, max_frame_count = constants.FCOUNT,
                        box = box)

                    raw_test_checksums = [tuple(checksum) for checksum in
                                          raw_test_checksums]

                    overreach_counts = self.overreach_frame_counts(
                            raw_test_checksums,
                            constants.MAX_FRAME_REPEAT_COUNT)

                    if not overreach_counts: # no checksums exceeded threshold
                        break

                    player.pause()
                    player.seek_to(datetime.timedelta(seconds=0))

                else:
                    msg = ('Framecount overreach detected even after %d '
                           'tries. Checksums: %s' % (constants.NUM_CAPTURE_TRIES,
                                                     overreach_counts))
                    raise error.TestFail(msg)


                # produces unique checksums mapped to their occur. indices
                test_checksum_indices = frame_checksum_utils.checksum_indices(
                        raw_test_checksums)

                test_checksums = test_checksum_indices.keys()

                test_indices = test_checksum_indices.values()

                golden_checksums_filepath = os.path.join(
                    constants.TEST_DIR,
                    constants.GOLDEN_CHECKSUMS_FILENAME)

                if collect_only:
                    capturer.write_images(test_indices, constants.TEST_DIR,
                                          constants.IMAGE_FORMAT)

                    logging.debug("Write golden checksum file to %s",
                                  golden_checksums_filepath)

                    with open(golden_checksums_filepath, "w+") as f:
                        for checksum in test_checksums:
                            f.write(' '.join([str(i) for i in checksum]) + '\n')
                    return

                golden_checksums_remote_filepath = os.path.join(
                    constants.GOLDEN_CHECKSUM_REMOTE_BASE_DIR,
                    board,
                    codec + '_' + resolution,
                    constants.GOLDEN_CHECKSUMS_FILENAME)

                file_utils.download_file(golden_checksums_remote_filepath,
                                         golden_checksums_filepath)

                golden_checksums = self.read_checksum_file(
                    golden_checksums_filepath)

                golden_checksum_count = len(golden_checksums)
                test_checksum_count = len(test_checksums)

                eps = constants.MAX_DIFF_TOTAL_FCOUNT
                if golden_checksum_count - test_checksum_count > eps:
                    msg = ('Expecting about %d checksums, received %d. '
                           'Allowed delta is %d') % (
                            golden_checksum_count,
                            test_checksum_count,
                            eps)
                    raise error.TestFail(msg)

                # Some frames might be missing during either golden frame
                # collection or during a test run. Using LCS ensures we
                # ignore a few missing frames while comparing test vs golden

                lcs_len = sequence_utils.lcs_length(golden_checksums,
                                                    test_checksums)

                missing_frames_count = len(golden_checksums) - lcs_len
                unknown_frames_count = len(test_checksums) - lcs_len

                msg = ('# of matching frames : %d. # of missing frames : %d. '
                       '# of unknown test frames : %d. Max allowed # of '
                       'missing frames : %d. # of golden frames : %d. # of '
                       'test_checksums : %d' %(lcs_len, missing_frames_count,
                                               unknown_frames_count,
                                               constants.MAX_NONMATCHING_FCOUNT,
                                               len(golden_checksums),
                                               len(test_checksums)))
                logging.debug(msg)

                if (missing_frames_count + unknown_frames_count >
                        constants.MAX_NONMATCHING_FCOUNT):
                    unknown_frames = set(test_checksums) - set(golden_checksums)

                    store_indices = [test_checksum_indices[c] for c in
                                     unknown_frames]

                    paths = capturer.write_images(store_indices,
                                                  constants.TEST_DIR,
                                                  constants.IMAGE_FORMAT)

                    path_publish = publisher.ImageDiffPublisher(self.resultsdir)
                    path_publish.publish_paths(paths, self.tagged_testname)

                    raise error.TestFail("Too many non-matching frames")


    def overreach_frame_counts(self, checksums, max_frame_repeat_count):
        """
        Checks that captured frames have not exceed the max repeat count.

        @param checksums: list of frame checksums received from chameleon.
        @param max_frame_repeat_count: int. max allowed count.
        @return : dictionary, checksums and their counts

        """

        logging.debug("Verify no frame is repeated more than %d",
                      max_frame_repeat_count)

        counts = frame_checksum_utils.checksum_counts(checksums)
        overreach_counts = {}

        for k, v in counts.iteritems():
            logging.debug("%s : %s", k, v)
            if v > max_frame_repeat_count:
                overreach_counts[k] = v

        return overreach_counts



    def read_checksum_file(self, path):
        """
        Reads the golden checksum file. Each line in file has the format
        w x y z where w x y z is a chameleon frame checksum
        @param path: complete path to the golden checksum file.
        @returns a list of 4-tuples (w x y z).

        """
        checksums = []
        with open(path) as f:
            for line in f:
                checksum = tuple(int(val) for val in line.split())
                checksums.append(checksum)
        return  checksums
