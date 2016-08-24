# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Output the test video parameters for VDA tests.

Output the test parameters of h264/vp8 videos for running VDA tests:
filename:width:height:frames:fragments:minFPSwithRender:minFPSnoRender:profile
(chromium content/common/gpu/media/video_decode_accelerator_unittest.cc)

Executing this script with a single h264 or vp8 video:
'output_test_video_params.py video.h264|video.vp8' will directly print the test
parameters for that video on screen.

Executing this script with a directory containing h264/vp8 videos:
'output_test_video_params.py video-dir/' will output
__test_video_list_[timestamp] file that contains a list of test parameters for
all h264/vp8 videos under video-dir. Only valid test parameters will be written
into the output file; unsupported videos/files under the video-dir will be
ignored.
"""

import mmap
import os
import re
import struct
import subprocess
import sys
import time

INVALID_PARAM = '##'

def h264_fragments(path):
    with open(path, "rb") as f:
        mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)

    # Count NAL by searching for the 0x00000001 start code prefix.
    pattern = '00000001'.decode('hex')
    regex = re.compile(pattern, re.MULTILINE)
    count = sum(1 for _ in regex.finditer(mm))
    mm.close()

    return str(count)

def vp8_fragments(path):
    # Read IVF 32-byte header and parse the frame number in the header.
    # IVF header definition: http://wiki.multimedia.cx/index.php?title=IVF
    with open(path, "rb") as f:
        mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)

    mm.seek(24, 0)
    header_frame_num = struct.unpack('i', mm.read(4))[0]
    mm.seek(32, 0)

    # Count the following frames and check if the count matches the frame number
    # in the header.
    count = 0
    size = mm.size()
    while mm.tell() + 4 <= size:
        (frame,) = struct.unpack('i', mm.read(4))
        offset = 8 + frame
        if (mm.tell() + offset <= size):
            mm.seek(offset, 1)
            count = count + 1

    mm.close()
    if header_frame_num != count:
        return INVALID_PARAM

    return str(count)

def full_test_parameters(path):
    try:
        ffmpeg_cmd = ["ffmpeg", "-i", path, "-vcodec", "copy", "-an", "-f",
                      "null", "/dev/null"]
        content = subprocess.check_output(ffmpeg_cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        content = e.output

    # Get video format, dimension, and frame number from the ffmpeg output.
    # Sample of ffmpeg output:
    # Input #0, h264, from 'video.h264':
    # Stream #0.0: Video: h264 (High), yuv420p, 640x360, 25 fps, ...
    # frame=   82 fps=  0 ...
    results = re.findall('Input #0, (\S+),', content)
    profile = INVALID_PARAM
    frag = INVALID_PARAM
    if (results):
        video_format = results[0].lower()
        if (video_format == 'h264'):
            profile = '1'
            frag = h264_fragments(path)
        elif (video_format == 'ivf'):
            profile = '11'
            frag = vp8_fragments(path)

    dimen = [INVALID_PARAM, INVALID_PARAM]
    fps = [INVALID_PARAM, INVALID_PARAM]
    results = re.findall('Stream #0.*Video:.* (\d+)x(\d+)', content)
    if (results):
        dimen = results[0]
        fps = ['30', '30']

    results = re.findall('frame= *(\d+)', content)
    frame = results[0] if results else INVALID_PARAM

    filename = os.path.basename(path)
    return '%s:%s:%s:%s:%s:%s:%s:%s' % (
            filename, dimen[0], dimen[1], frame, frag, fps[0], fps[1], profile)

def check_before_output(line):
    if INVALID_PARAM in line:
        print 'Warning: %s' % line
        return False
    return True

def main(argv):
    if len(argv) != 1:
        print 'Please provide a h264/vp8 directory or file.'
        sys.exit(1)

    if os.path.isdir(argv[0]):
        name = '__test_video_list_%s' % time.strftime("%Y%m%d_%H%M%S")
        with open(name, 'w') as output:
            output.write('[\n')
            for root, _, files in os.walk(argv[0]):
                for f in files:
                    path = os.path.join(root, f)
                    line = full_test_parameters(path)
                    if check_before_output(line):
                        # Output in json format (no trailing comma in the list.)
                        sep = '' if f is files[-1] else ','
                        output.write('\"%s\"%s\n' % (line, sep))

            output.write(']\n')
    elif os.path.isfile(argv[0]):
        line = full_test_parameters(argv[0])
        if check_before_output(line):
            print line
    else:
        print 'Invalid input.'

main(sys.argv[1:])
