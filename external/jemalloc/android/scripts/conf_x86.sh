#!/bin/bash

if [[ "$OUT" == "" ]]; then
  echo "In order for this script to function, please choose an x86 target"
  echo "using source build/envsetup.sh and lunch XXX\n"
  exit 1
fi

x86_cc="${ANDROID_TOOLCHAIN}/x86_64-linux-android-gcc -m32"
x86_cpp="${ANDROID_TOOLCHAIN}/x86_64-linux-android-g++ -m32"

includes=(
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/arch-x86/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libstdc++/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/kernel/uapi"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/kernel/uapi/asm-x86"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libm/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libm/include/i387"
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

eval ./configure CC=\"${x86_cc} ${includes[@]}\" \
                 CPP=\"${x86_cc} ${includes[@]} -E\" \
                 CXX=\"${x86_cpp} ${includes[@]}\" \
                 CXXCPP=\"${x86_cpp} ${includes[@]} -E\" \
                 LDFLAGS=\"${ldflags[@]}\" \
                 --host=x86-android-linux \
                 --disable-valgrind \
                 --with-jemalloc_prefix=je_ \

