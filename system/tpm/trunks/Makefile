#
# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

PWD ?= $(CURDIR)
OUT ?= $(PWD)/build-opt-local

include common.mk

PC_DEPS = libchrome-$(BASE_VER) libbrillo-$(BASE_VER) openssl
PC_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PC_DEPS))
PC_LIBS := $(shell $(PKG_CONFIG) --libs $(PC_DEPS))

CXXFLAGS += -I$(SRC)/.. $(PC_CFLAGS)
LDLIBS += $(PC_LIBS)

TEST_OBJS := $(filter %_test.o trunks_testrunner.o mock_%.o, $(CXX_OBJECTS))
SHARED_OBJS := $(filter-out $(TEST_OBJS), $(CXX_OBJECTS))

CXX_BINARY(trunks_testrunner): CXXFLAGS += $(shell gtest-config --cxxflags) \
                                           $(shell gmock-config --cxxflags)
CXX_BINARY(trunks_testrunner): LDLIBS += $(shell gtest-config --libs) \
                                         $(shell gmock-config --libs)
CXX_BINARY(trunks_testrunner): $(TEST_OBJS) $(SHARED_OBJS)

all: $(SHARED_OBJS)
tests: TEST(CXX_BINARY(trunks_testrunner))
clean: CLEAN(CXX_BINARY(trunks_testrunner))
