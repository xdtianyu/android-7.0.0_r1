# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LDFLAGS =  -lrt -ldl

LDFLAGS_UNITTEST = -lrt

LDFLAGS_HELPER = -lrt

SRC = media_v4l2_device.cc  \
      media_v4l2_test.cc

SRC_UNITTEST = media_v4l2_device.cc \
               media_v4l2_unittest.cc

SRC_HELPER = media_v4l2_device.cc \
             media_v4l2_is_capture_device.cc

TARGET = ../media_v4l2_test

TARGET_UNITTEST = ../media_v4l2_unittest

TARGET_HELPER = ../media_v4l2_is_capture_device

all: $(TARGET) $(TARGET_UNITTEST) $(TARGET_HELPER)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -O3 -o $@ $^ $(LDFLAGS)

$(TARGET_UNITTEST): $(SRC_UNITTEST)
	$(CXX) $(CXXFLAGS) -O3 -o $@ $^ $(LDFLAGS_UNITTEST)

$(TARGET_HELPER): $(SRC_HELPER)
	$(CXX) $(CXXFLAGS) -O3 -o $@ $^ $(LDFLAGS_HELPER)

clean:
	$(RM) $(TARGET)
	$(RM) $(TARGET_UNITTEST)
	$(RM) $(TARGET_HELPER)
