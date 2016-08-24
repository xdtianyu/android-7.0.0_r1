#!/bin/bash

if [[ "$OUT" == "" ]]; then
  echo "In order for this script to function, please choose an arm target"
  echo "using source build/envsetup.sh and lunch XXX\n"
  exit 1
fi

arm_cc="${ANDROID_TOOLCHAIN}/arm-linux-androideabi-gcc"
arm_cpp="${ANDROID_TOOLCHAIN}/arm-linux-androideabi-g++"

includes=(
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/arch-arm/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libstdc++/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/kernel/uapi"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/kernel/uapi/asm-arm"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libm/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libm/include/arm"
)

# Copy libm.so to libpthread.so to allow -lpthread to work.
cp ${OUT}/obj/lib/libm.so ${OUT}/obj/lib/libpthread.so

ldflags=(
  "-nostdlib"
  "-Bdynamic"
  "-fPIE"
  "-pie"
  "-Wl,-dynamic-linker,/system/bin/linker"
  "-Wl,--gc-sections"
  "-Wl,-z,nocopyreloc"
  "-L${OUT}/obj/lib"
  "-Wl,-rpath-link=${OUT}/obj/lib"
  "${OUT}/obj/lib/crtbegin_dynamic.o"
  "-Wl,--whole-archive"
  "-Wl,--no-whole-archive"
  "-lc"
  "-lstdc++"
  "-lgcc"
  "-lm"
  "-Wl,-z,noexecstack"
  "-Wl,-z,relro"
  "-Wl,-z,now"
  "-Wl,--warn-shared-textrel"
  "-Wl,--fatal-warnings"
  "-Wl,--icf=safe"
  "-Wl,--no-undefined"
  "-ldl"
)

eval ./configure CC=\"${arm_cc} ${includes[@]}\" \
                 CPP=\"${arm_cc} ${includes[@]} -E\" \
                 CXX=\"${arm_cpp} ${includes[@]}\" \
                 CXXCPP=\"${arm_cpp} ${includes[@]} -E\" \
                 LDFLAGS=\"${ldflags[@]}\" \
                 --host=arm-android-linux \
                 --disable-valgrind \
                 --with-jemalloc_prefix=je_ \

