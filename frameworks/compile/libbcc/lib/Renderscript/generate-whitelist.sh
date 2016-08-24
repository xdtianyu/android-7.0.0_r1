#!/bin/bash

# This shell script automatically extracts RenderScript stub functions .
# To regenerate files RSStubsWhiteList.{cpp,h} run the following command
# sh generate-whitelist.sh RSStubsWhiteList $ANDROID_BUILD_TOP/frameworks/rs/driver/rsdRuntimeStubs.cpp $ANDROID_BUILD_TOP/frameworks/rs/cpu_ref/rsCpuRuntimeStubs.cpp $ANDROID_BUILD_TOP/frameworks/rs/cpu_ref/rsCpuRuntimeMath.cpp

OUT_PATH_PREFIX=$1
OUT_PREFIX=`basename $OUT_PATH_PREFIX`
STUB_FILES=${@:2}

whitelist=`grep "{ \"_Z" $STUB_FILES | awk '{print $3}' | sort | uniq`

OUT_HEADER=$OUT_PATH_PREFIX\.h
OUT_CPP=$OUT_PATH_PREFIX\.cpp

read -d '' COPYRIGHT << EOF
/*
 * Copyright 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
EOF


cat > $OUT_HEADER << EOF
$COPYRIGHT

#ifndef ${OUT_PREFIX}_H
#define ${OUT_PREFIX}_H

#include <cstdlib>
#include <vector>
#include <string>

extern std::vector<std::string> stubList;

#endif // ${OUT_PREFIX}_H
EOF

cat > $OUT_CPP  << EOF
$COPYRIGHT

#include "$OUT_PREFIX.h"

std::vector<std::string> stubList = {
$whitelist
};
EOF

echo Wrote to $OUT_HEADER $OUT_CPP
