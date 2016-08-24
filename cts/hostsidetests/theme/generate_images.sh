#!/bin/sh
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

# This script is used to generate reference images for the CTS theme tests.
# See the accompanying README file for more information.

# retry <command> <tries> <message> <delay>
function retry {
  RETRY="0"
  while true; do
    if (("$RETRY" >= "$2")); then
      echo $OUTPUT
      exit
    fi

    OUTPUT=`$1 |& grep error`

    if [ -z "$OUTPUT" ]; then
      break
    fi

    echo $3
    sleep $4
    RETRY=$[$RETRY + 1]
  done
}

themeApkPath="$ANDROID_HOST_OUT/cts/android-cts/testcases/CtsThemeDeviceApp.apk"
outDir="$ANDROID_BUILD_TOP/cts/hostsidetests/theme/assets"
exe="$ANDROID_BUILD_TOP/cts/hostsidetests/theme/run_theme_capture_device.py"

if [ -z "$ANDROID_BUILD_TOP" ]; then
  echo "Missing environment variables. Did you run build/envsetup.sh and lunch?"
  exit
fi

if [ ! -e "$themeApkPath" ]; then
  echo "Couldn't find test APK. Did you run make cts?"
  exit
fi

adb devices
python $exe $themeApkPath $outDir
