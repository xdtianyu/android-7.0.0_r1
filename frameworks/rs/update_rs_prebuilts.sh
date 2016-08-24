#!/bin/bash

# We are currently in frameworks/rs, so compute our top-level directory.
MY_ANDROID_DIR=$PWD/../../
cd $MY_ANDROID_DIR

if [[ $OSTYPE == darwin* ]];
then

  DARWIN=1
  SHORT_OSNAME=darwin
  SONAME=dylib
  # Only build arm on darwin.
  TARGETS=(arm)
  SYS_NAMES=(generic)
  NUM_CORES=`sysctl -n hw.ncpu`

else

  DARWIN=0
  SHORT_OSNAME=linux
  SONAME=so
  # Target architectures and their system library names.
  TARGETS=(arm mips x86 arm64 x86_64)
  SYS_NAMES=(generic generic_mips generic_x86 generic_arm64 generic_x86_64)
  NUM_CORES=`cat /proc/cpuinfo | grep processor | tail -n 1 | cut -f 2 -d :`
  NUM_CORES=$(($NUM_CORES+1))

fi

echo "Using $NUM_CORES cores"

# Turn off the build cache and make sure we build all of LLVM from scratch.
export ANDROID_USE_BUILDCACHE=false
export FORCE_BUILD_LLVM_COMPONENTS=true

# Skip building LLVM and compiler-rt tests while updating prebuilts
export SKIP_LLVM_TESTS=true

# Ensure that we have constructed the latest "bcc" for the host. Without
# this variable, we don't build the .so files, hence we never construct the
# actual required compiler pieces.
export FORCE_BUILD_RS_COMPAT=true

# RENDERSCRIPT_V8_JAR is the generated JAVA static lib for RenderScript Support Lib.
RENDERSCRIPT_V8_JAR=out/target/common/obj/JAVA_LIBRARIES/android-support-v8-renderscript_intermediates/classes.jar

# ANDROID_HOST_OUT is where the new prebuilts will be constructed/copied from.
ANDROID_HOST_OUT=$MY_ANDROID_DIR/out/host/$SHORT_OSNAME-x86/

# HOST_LIB_DIR allows us to pick up the built librsrt_*.bc libraries.
HOST_LIB_DIR=$ANDROID_HOST_OUT/lib

# HOST_LIB64_DIR
HOST_LIB64_DIR=$ANDROID_HOST_OUT/lib64

# PREBUILTS_DIR is where we want to copy our new files to.
PREBUILTS_DIR=$MY_ANDROID_DIR/prebuilts/sdk/

print_usage() {
  echo "USAGE: $0 [-h|--help] [-n|--no-build] [-x]"
  echo "OPTIONS:"
  echo "    -h, --help     : Display this help message."
  echo "    -n, --no-build : Skip the build step and just copy files."
  echo "    -x             : Display commands before they are executed."
}

build_rs_libs() {
  echo Building for target $1
  lunch $1
  # Build the RS runtime libraries.
  cd $MY_ANDROID_DIR/frameworks/rs/driver/runtime && mma -j$NUM_CORES && cd - || exit 1
  # Build a sample support application to ensure that all the pieces are up to date.
  cd $MY_ANDROID_DIR/frameworks/rs/java/tests/RSTest_CompatLib/ && mma -j$NUM_CORES && cd - || exit 2
  # Build android-support-v8-renderscript.jar
  # We need to explicitly do so, since JACK won't generate a jar by default.
  cd $MY_ANDROID_DIR && make $RENDERSCRIPT_V8_JAR -j$NUM_CORES && cd - || exit 3
  # Build libcompiler-rt.a
  cd $MY_ANDROID_DIR/external/compiler-rt && mma -j$NUM_CORES && cd - || exit 4
  # Build the blas libraries.
  cd $MY_ANDROID_DIR/external/cblas && mma -j$NUM_CORES && cd - || exit 5
}

# Build everything by default
build_rs=1

while [ $# -gt 0 ]; do
  case "$1" in
    -h|--help)
      print_usage
      exit 0
      ;;
    -n|--no-build)
      build_rs=0
      ;;
    -x)
      # set lets us enable bash -x mode.
      set -x
      ;;
    *)
      echo Unknown argument: "$1"
      print_usage
      exit 99
      break
      ;;
  esac
  shift
done

if [ $build_rs -eq 1 ]; then

  echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  echo !!! BUILDING RS PREBUILTS !!!
  echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  source build/envsetup.sh

  for t in ${TARGETS[@]}; do
    build_rs_libs aosp_${t}-userdebug
  done

  echo DONE BUILDING RS PREBUILTS

else

  echo SKIPPING BUILD OF RS PREBUILTS

fi

DATE=`date +%Y%m%d`

cd $PREBUILTS_DIR || exit 3
repo start pb_$DATE .

# Don't copy device prebuilts on Darwin. We don't need/use them.
if [ $DARWIN -eq 0 ]; then
  for i in $(seq 0 $((${#TARGETS[@]} - 1))); do
    t=${TARGETS[$i]}
    sys_name=${SYS_NAMES[$i]}
    case "$sys_name" in
      *64)
        sys_lib_dir=$MY_ANDROID_DIR/out/target/product/$sys_name/system/lib64
        ;;
      *)
        sys_lib_dir=$MY_ANDROID_DIR/out/target/product/$sys_name/system/lib
        ;;
    esac
    obj_lib_dir=$MY_ANDROID_DIR/out/target/product/$sys_name/obj/lib
    obj_static_lib_dir=$MY_ANDROID_DIR/out/target/product/$sys_name/obj/STATIC_LIBRARIES

    for a in `find renderscript/lib/$t -name \*.so`; do
      file=`basename $a`
      cp `find $sys_lib_dir $obj_lib_dir -name $file | head -1` $a || exit 4
    done

    for a in `find renderscript/lib/$t -name \*.bc`; do
      file=`basename $a`
      cp `find $HOST_LIB_DIR $HOST_LIB64_DIR $sys_lib_dir $obj_lib_dir -name $file | head -1` $a || exit 5
    done

    for a in `find renderscript/lib/$t -name \*.a`; do
      file=`basename $a`
      cp `find $obj_static_lib_dir -name $file | head -1` $a || exit 4
    done

  done

  # javalib.jar
  cp $MY_ANDROID_DIR/$RENDERSCRIPT_V8_JAR renderscript/lib/javalib.jar

fi

# Copy header files for compilers
cp $MY_ANDROID_DIR/external/clang/lib/Headers/*.h renderscript/clang-include
cp $MY_ANDROID_DIR/frameworks/rs/scriptc/* renderscript/include


# Host-specific tools (bin/ and lib/)
TOOLS_BIN="
bcc_compat
llvm-rs-cc
"

TOOLS_LIB="
libbcc.$SONAME
libbcinfo.$SONAME
libclang.$SONAME
libc++.$SONAME
libLLVM.$SONAME
"

TOOLS_LIB32="libc++.$SONAME"

for a in $TOOLS_BIN; do
  cp $ANDROID_HOST_OUT/bin/$a tools/$SHORT_OSNAME/bin
  strip tools/$SHORT_OSNAME/bin/$a
done

for a in $TOOLS_LIB; do
  cp $HOST_LIB64_DIR/$a tools/$SHORT_OSNAME/lib64
  strip tools/$SHORT_OSNAME/lib64/$a
done

for a in $TOOLS_LIB32; do
  cp $HOST_LIB_DIR/$a tools/$SHORT_OSNAME/lib
  strip tools/$SHORT_OSNAME/lib/$a
done

if [ $DARWIN -eq 0 ]; then
  echo "DON'T FORGET TO UPDATE THE DARWIN COMPILER PREBUILTS!!!"
fi
