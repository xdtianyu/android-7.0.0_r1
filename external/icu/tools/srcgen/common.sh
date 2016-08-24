# Copyright (C) 2015 The Android Open Source Project
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

# Common variables and actions for inclusion in srcgen scripts.

set -e

if [[ -z ${ANDROID_BUILD_TOP} ]]; then
  echo ANDROID_BUILD_TOP not set
  exit 1
fi

# source envsetup.sh because functions we use like mm are not exported.
source ${ANDROID_BUILD_TOP}/build/envsetup.sh

# Build the srcgen tools.
cd ${ANDROID_BUILD_TOP}
make -j16 android_icu4j_srcgen

ICU4J_DIR=${ANDROID_BUILD_TOP}/external/icu/icu4j
ANDROID_ICU4J_DIR=${ANDROID_BUILD_TOP}/external/icu/android_icu4j

CLASSPATH=${ANDROID_BUILD_TOP}/out/host/common/obj/JAVA_LIBRARIES/currysrc_intermediates/javalib.jar:${ANDROID_BUILD_TOP}/out/host/common/obj/JAVA_LIBRARIES/android_icu4j_srcgen_intermediates/javalib.jar

# The parts of ICU4J to include during srcgen.
#
# The following are deliberately excluded:
#   localespi - is not supported on Android
#   charset - because icu4c is used instead
INPUT_DIRS="\
    ${ICU4J_DIR}/main/classes/collate/src \
    ${ICU4J_DIR}/main/classes/core/src \
    ${ICU4J_DIR}/main/classes/currdata/src \
    ${ICU4J_DIR}/main/classes/langdata/src \
    ${ICU4J_DIR}/main/classes/regiondata/src \
    ${ICU4J_DIR}/main/classes/translit/src \
    "

SAMPLE_INPUT_DIR=${ICU4J_DIR}/samples/src/com/ibm/icu/samples
# Only generate sample files for code we know should compile on Android with the public APIs.
SAMPLE_INPUT_FILES="\
    ${SAMPLE_INPUT_DIR}/text/dateintervalformat/DateIntervalFormatSample.java \
    ${SAMPLE_INPUT_DIR}/text/datetimepatterngenerator/DateTimePatternGeneratorSample.java \
    ${SAMPLE_INPUT_DIR}/text/pluralformat/PluralFormatSample.java \
    "

# See above for an explanation as to why the tests for charset and localespi are not included here.
TEST_INPUT_DIR=${ICU4J_DIR}/main/tests
TEST_INPUT_DIRS="\
    ${TEST_INPUT_DIR}/collate/src \
    ${TEST_INPUT_DIR}/framework/src \
    ${TEST_INPUT_DIR}/testall/src \
    ${TEST_INPUT_DIR}/translit/src \
    ${TEST_INPUT_DIR}/core/src \
    ${TEST_INPUT_DIR}/packaging/src"
