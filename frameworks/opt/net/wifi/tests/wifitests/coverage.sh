#!/usr/bin/env bash

if [[ ! ( ($# == 1) || ($# == 2 && ($2 == "HTML" || $2 == "XML" || $2 == "CSV"))) ]]; then
  echo "$0: usage: coverage.sh OUTPUT_DIR [REPORT_TYPE]"
  echo "REPORT_TYPE [HTML | XML | CSV] : the type of the report (default is HTML)"
  exit 1
fi

if [ -z $ANDROID_BUILD_TOP ]; then
  echo "You need to source and lunch before you can use this script"
  exit 1
fi

REPORTER_JAR=$ANDROID_BUILD_TOP/prebuilts/sdk/tools/jack-jacoco-reporter.jar

OUTPUT_DIR=$1
if [[ $# == 2 ]]; then
  REPORT_TYPE=$2
else
  REPORT_TYPE="HTML"
fi

echo "Running tests and generating coverage report"
echo "Output dir: $OUTPUT_DIR"
echo "Report type: $REPORT_TYPE"

REMOTE_COVERAGE_OUTPUT_FILE=/data/data/com.android.server.wifi.test/files/coverage.ec
COVERAGE_OUTPUT_FILE=$OUTPUT_DIR/wifi_coverage.ec
COVERAGE_METADATA_FILE=$ANDROID_BUILD_TOP/out/target/common/obj/APPS/FrameworksWifiTests_intermediates/coverage.em

set -e # fail early

echo "+ EMMA_INSTRUMENT_STATIC=true mmma -j32 $ANDROID_BUILD_TOP/frameworks/opt/net/wifi/tests"
# NOTE Don't actually run the command above since this shell doesn't inherit functions from the
#      caller.
EMMA_INSTRUMENT_STATIC=true make -j32 -C $ANDROID_BUILD_TOP -f build/core/main.mk MODULES-IN-frameworks-opt-net-wifi-tests

set -x # print commands

adb root
adb wait-for-device

adb shell rm -f $REMOTE_COVERAGE_OUTPUT_FILE

adb install -r -g "$OUT/data/app/FrameworksWifiTests/FrameworksWifiTests.apk"

adb shell am instrument -e coverage true -w 'com.android.server.wifi.test/android.support.test.runner.AndroidJUnitRunner'

mkdir -p $OUTPUT_DIR

adb pull $REMOTE_COVERAGE_OUTPUT_FILE $COVERAGE_OUTPUT_FILE

java -jar $REPORTER_JAR \
  --report-dir $OUTPUT_DIR \
  --metadata-file $COVERAGE_METADATA_FILE \
  --coverage-file $COVERAGE_OUTPUT_FILE \
  --report-type $REPORT_TYPE \
  --source-dir $ANDROID_BUILD_TOP/frameworks/opt/net/wifi/tests/wifitests/src \
  --source-dir $ANDROID_BUILD_TOP/frameworks/opt/net/wifi/service/java \
  --source-dir $ANDROID_BUILD_TOP/frameworks/base/wifi/java
