#!/bin/bash

if [[ "$OUT" == "" ]]; then
  echo "In order for this script to function, please choose an arm64 target"
  echo "using source build/envsetup.sh and lunch XXX\n"
  exit 1
fi

aarch64_cc="${ANDROID_TOOLCHAIN}/aarch64-linux-android-gcc"
aarch64_cpp="${ANDROID_TOOLCHAIN}/aarch64-linux-android-g++"

includes=(
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/arch-arm64/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libstdc++/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/kernel/uapi"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/kernel/uapi/asm-arm64"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libm/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libm/include/arm64"
)

# Copy libm.so to libpthread.so to allow -lpthread to work.
cp ${OUT}/obj/lib/libm.so ${OUT}/obj/lib/libpthread.so

ldflags=(
  "-nostdlib"
  "-Bdynamic"
  "-fPIE"
  "-pie"
  "-Wl,-dynamic-linker,/system/bin/linker64"
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
  "-Wl,--no-undefined"
  "-ldl"
)

eval ./configure CC=\"${aarch64_cc} ${includes[@]}\" \
                 CPP=\"${aarch64_cc} ${includes[@]} -E\" \
                 CXX=\"${aarch64_cpp} ${includes[@]}\" \
                 CXXCPP=\"${aarch64_cpp} ${includes[@]} -E\" \
                 LDFLAGS=\"${ldflags[@]}\" \
                 --host=aarch64-android-linux \
                 --disable-valgrind \
                 --with-jemalloc_prefix=je_ \

