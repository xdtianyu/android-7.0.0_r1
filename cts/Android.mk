#
# Copyright (C) 2008 The Android Open Source Project
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

include cts/OldCtsBuild.mk
include cts/CtsCoverage.mk
include $(call all-subdir-makefiles)


# Have the default build also build the tools for CTS so it is possible
# to build individual tests with mmm without doing extra targets first.
files: \
    $(CTS_JAVA_TEST_SCANNER_DOCLET) \
    $(CTS_JAVA_TEST_SCANNER) \
    $(CTS_NATIVE_TEST_SCANNER) \
    $(CTS_XML_GENERATOR)

