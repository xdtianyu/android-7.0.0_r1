#!/bin/bash

if [[ "$OUT" == "" ]]; then
  echo "In order for this script to function, please choose an x86_64 target"
  echo "using source build/envsetup.sh and lunch XXX\n"
  exit 1
fi

x86_64_cc="${ANDROID_TOOLCHAIN}/x86_64-linux-android-gcc"
x86_64_cpp="${ANDROID_TOOLCHAIN}/x86_64-linux-android-g++"

includes=(
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/arch-x86_64/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libstdc++/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/kernel/uapi"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/kernel/uapi/asm-x86"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libm/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libm/include/amd64"
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
  "-Wl,--icf=safe"
  "-Wl,--no-undefined"
  "-ldl"
)

eval ./configure CC=\"${x86_64_cc} ${includes[@]}\" \
                 CPP=\"${x86_64_cc} ${includes[@]} -E\" \
                 CXX=\"${x86_64_cpp} ${includes[@]}\" \
                 CXXCPP=\"${x86_64_cpp} ${includes[@]} -E\" \
                 LDFLAGS=\"${ldflags[@]}\" \
                 --host=x86_64-android-linux \
                 --disable-valgrind \
                 --with-jemalloc_prefix=je_ \

