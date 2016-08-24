# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

GIT_CLONE_URL = "https://chromium.googlesource.com/apps/camera"
BUILD_CAMERA_DIR = camera

all:
	# Clone the ToT version of the Camera App.
	rm -rf $(BUILD_CAMERA_DIR)
	mkdir $(BUILD_CAMERA_DIR)
	cd $(BUILD_CAMERA_DIR)
	git clone $(GIT_CLONE_URL)

	# Always build from scratch.
	$(MAKE) -C $(BUILD_CAMERA_DIR) clean build/camera

clean:
	rm -rf $(BUILD_CAMERA_DIR)
