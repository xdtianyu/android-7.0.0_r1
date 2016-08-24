#!/usr/bin/env python

import argparse
import sys

argparser = argparse.ArgumentParser(
    description="Take a screenshot!",
    epilog="I can output PNG, JPEG, GIF, and other PIL-supported formats.")
argparser.add_argument("-c", "--crtc", type=int, default=0,
                       help="CRTC id (default first screen)")
argparser.add_argument("path", help="output image location")

args = argparser.parse_args()

# Do some evil.
sys.path.insert(0, "/usr/local/autotest")

# This import can't be moved to before the sys.path alteration.
from cros.graphics.drm import crtcScreenshot

image = crtcScreenshot(args.crtc)
image.save(args.path)
