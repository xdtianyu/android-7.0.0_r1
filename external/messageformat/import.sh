#!/bin/bash
TOP=$1
mkdir -p java/com/ibm/icu/impl
mkdir -p java/com/ibm/icu/simple
mkdir -p java/com/ibm/icu/text
mkdir -p java/com/ibm/icu/util
cp ${TOP}/main/classes/core/src/com/ibm/icu/impl/PatternProps.java java/com/ibm/icu/impl
cp ${TOP}/main/classes/core/src/com/ibm/icu/impl/ICUConfig.java java/com/ibm/icu/impl
cp ${TOP}/main/classes/core/src/com/ibm/icu/impl/ICUData.java java/com/ibm/icu/impl
cp ${TOP}/main/classes/core/src/com/ibm/icu/simple/*.java java/com/ibm/icu/simple/
cp ${TOP}/main/classes/core/src/com/ibm/icu/text/MessagePattern.java java/com/ibm/icu/text
cp ${TOP}/main/classes/core/src/com/ibm/icu/text/SelectFormat.java java/com/ibm/icu/text
cp ${TOP}/main/classes/core/src/com/ibm/icu/util/ICUUncheckedIOException.java java/com/ibm/icu/util
cp ${TOP}/main/classes/core/src/com/ibm/icu/util/ICUCloneNotSupportedException.java java/com/ibm/icu/util
cp ${TOP}/main/classes/core/src/com/ibm/icu/util/ICUException.java java/com/ibm/icu/util
cp ${TOP}/main/classes/core/src/com/ibm/icu/util/Output.java java/com/ibm/icu/util
cp ${TOP}/main/classes/core/src/com/ibm/icu/util/Freezable.java java/com/ibm/icu/util
