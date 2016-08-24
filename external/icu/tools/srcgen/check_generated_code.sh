#!/bin/bash

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

# A script for checking the android_icu4j source for errors.

source ./common.sh

java -cp ${CLASSPATH} com.android.icu4j.srcgen.checker.CheckAndroidIcu4JSource \
    ${ANDROID_ICU4J_DIR}/src/main/java \
    ./android_icu4j_source_report.txt
