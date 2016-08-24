#!/bin/bash

if [[ "$OUT" == "" ]]; then
  echo "In order for this script to function, please choose an mips target"
  echo "using source build/envsetup.sh and lunch XXX\n"
  exit 1
fi

mips_cc="${ANDROID_TOOLCHAIN}/mipsel-linux-android-gcc"
mips_cpp="${ANDROID_TOOLCHAIN}/mipsel-linux-android-g++"

includes=(
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/arch-mips/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libstdc++/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/kernel/uapi"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libc/kernel/uapi/asm-mips"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libm/include"
  "-isystem ${ANDROID_BUILD_TOP}/bionic/libm/include/mips"
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
  "-Wl,--no-undefined"
  "-ldl"
)

eval ./configure CC=\"${mips_cc} ${includes[@]}\" \
                 CPP=\"${mips_cc} ${includes[@]} -E\" \
                 CXX=\"${mips_cpp} ${includes[@]}\" \
                 CXXCPP=\"${mips_cpp} ${includes[@]} -E\" \
                 LDFLAGS=\"${ldflags[@]}\" \
                 --host=mips-android-linux \
                 --disable-valgrind \
                 --with-jemalloc_prefix=je_ \

