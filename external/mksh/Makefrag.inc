# Makefile fragment for building mksh R51 2015/07/10

PROG=		mksh
MAN=		mksh.1
SRCS=		lalloc.c eval.c exec.c expr.c funcs.c histrap.c jobs.c lex.c main.c misc.c shf.c syn.c tree.c var.c edit.c
SRCS_FP=	../src/lalloc.c ../src/eval.c ../src/exec.c ../src/expr.c ../src/funcs.c ../src/histrap.c ../src/jobs.c ../src/lex.c ../src/main.c ../src/misc.c ../src/shf.c ../src/syn.c ../src/tree.c ../src/var.c ../src/edit.c
OBJS_BP=	 lalloc.o eval.o exec.o expr.o funcs.o histrap.o jobs.o lex.o main.o misc.o shf.o syn.o tree.o var.o edit.o
INDSRCS=	emacsfn.h rlimits.opt sh.h sh_flags.opt var_spec.h
NONSRCS_INST=	dot.mkshrc $(MAN)
NONSRCS_NOINST=	Build.sh Makefile Rebuild.sh check.pl check.t test.sh
CC=		/huge-ssd/aosp-arm64/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/*-gcc
CFLAGS=		 -fno-exceptions -Wno-multichar -fpic -fPIE -ffunction-sections -fdata-sections -funwind-tables -fstack-protector -Wa,--noexecstack -Werror=format-security -fno-short-enums -Wno-unused-but-set-variable -fno-builtin-sin -fno-strict-volatile-bitfields -Wno-psabi -fmessage-length=0 -W -Wall -Wno-unused -Winit-self -Wpointer-arith -Werror=return-type -Werror=non-virtual-dtor -Werror=address -Werror=sequence-point -g -Wstrict-aliasing=2 -fgcse-after-reload -frerun-cse-after-loop -frename-registers -Os -fomit-frame-pointer -fno-strict-aliasing -Wno-deprecated-declarations -fno-asynchronous-unwind-tables -fstack-protector-strong -fwrapv
CPPFLAGS=	-I. -I'../src'  -isystem /huge-ssd/aosp-arm64/bionic/libc/arch-arm64/include -isystem /huge-ssd/aosp-arm64/bionic/libc/include -isystem /huge-ssd/aosp-arm64/bionic/libc/kernel/uapi -isystem /huge-ssd/aosp-arm64/bionic/libc/kernel/uapi/asm-arm64 -isystem /huge-ssd/aosp-arm64/bionic/libm/include -isystem /huge-ssd/aosp-arm64/bionic/libm/include/arm64 -D_FORTIFY_SOURCE=2 -include /huge-ssd/aosp-arm64/build/core/combo/include/arch/linux-arm64/AndroidConfig.h -I/huge-ssd/aosp-arm64/build/core/combo/include/arch/linux-arm64/ -DANDROID -DNDEBUG -UDEBUG -DDEBUG_LEAKS -DMKSH_ASSUME_UTF8 -DMKSH_CONSERVATIVE_FDS -DMKSH_DONT_EMIT_IDSTRING -DMKSH_NOPWNAM -DMKSH_BUILDSH -D_GNU_SOURCE -DSETUID_CAN_FAIL_WITH_EAGAIN -DHAVE_ATTRIBUTE_BOUNDED=0 -DHAVE_ATTRIBUTE_FORMAT=1 -DHAVE_ATTRIBUTE_NORETURN=1 -DHAVE_ATTRIBUTE_PURE=1 -DHAVE_ATTRIBUTE_UNUSED=1 -DHAVE_ATTRIBUTE_USED=1 -DHAVE_SYS_TIME_H=1 -DHAVE_TIME_H=1 -DHAVE_BOTH_TIME_H=1 -DHAVE_SYS_BSDTYPES_H=0 -DHAVE_SYS_FILE_H=1 -DHAVE_SYS_MKDEV_H=0 -DHAVE_SYS_MMAN_H=1 -DHAVE_SYS_PARAM_H=1 -DHAVE_SYS_RESOURCE_H=1 -DHAVE_SYS_SELECT_H=1 -DHAVE_SYS_SYSMACROS_H=1 -DHAVE_BSTRING_H=0 -DHAVE_GRP_H=1 -DHAVE_IO_H=0 -DHAVE_LIBGEN_H=1 -DHAVE_LIBUTIL_H=0 -DHAVE_PATHS_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_TERMIOS_H=1 -DHAVE_ULIMIT_H=0 -DHAVE_VALUES_H=0 -DHAVE_CAN_INTTYPES=1 -DHAVE_CAN_UCBINTS=1 -DHAVE_CAN_INT8TYPE=1 -DHAVE_CAN_UCBINT8=1 -DHAVE_RLIM_T=1 -DHAVE_SIG_T=1 -DHAVE_SYS_ERRLIST=0 -DHAVE_SYS_SIGNAME=1 -DHAVE_SYS_SIGLIST=1 -DHAVE_FLOCK=1 -DHAVE_LOCK_FCNTL=1 -DHAVE_GETRUSAGE=1 -DHAVE_GETSID=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_ISSETUGID=0 -DHAVE_KILLPG=1 -DHAVE_MEMMOVE=1 -DHAVE_MKNOD=0 -DHAVE_MMAP=1 -DHAVE_NICE=1 -DHAVE_REVOKE=0 -DHAVE_SETLOCALE_CTYPE=0 -DHAVE_LANGINFO_CODESET=0 -DHAVE_SELECT=1 -DHAVE_SETRESUGID=1 -DHAVE_SETGROUPS=1 -DHAVE_STRERROR=1 -DHAVE_STRSIGNAL=0 -DHAVE_STRLCPY=1 -DHAVE_FLOCK_DECL=1 -DHAVE_REVOKE_DECL=1 -DHAVE_SYS_ERRLIST_DECL=0 -DHAVE_SYS_SIGLIST_DECL=1 -DHAVE_PERSISTENT_HISTORY=0 -DMKSH_BUILD_R=511
LDFLAGS=	 -nostdlib -Bdynamic -fPIE -pie -Wl,-dynamic-linker,/system/bin/linker -Wl,--gc-sections -Wl,-z,nocopyreloc -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,--warn-shared-textrel -Wl,--fatal-warnings -Wl,--no-undefined /huge-ssd/aosp-arm64/out/target/product/flounder/obj/lib/crtbegin_dynamic.o
LIBS=		 -L/huge-ssd/aosp-arm64/out/target/product/flounder/obj/lib -Wl,-rpath-link=/huge-ssd/aosp-arm64/out/target/product/flounder/obj/lib -Wl,--no-whole-archive /huge-ssd/aosp-arm64/out/target/product/flounder/obj/STATIC_LIBRARIES/libcompiler_rt-extras_intermediates/libcompiler_rt-extras.a -lc /huge-ssd/aosp-arm64/out/target/product/flounder/obj/lib/crtend_android.o

.depend $(OBJS_BP): rlimits.gen sh_flags.gen
rlimits.gen: ../src/Build.sh ../src/rlimits.opt
			srcfile=../src/rlimits.opt; BUILDSH_RUN_GENOPT=1; . ../src/Build.sh
sh_flags.gen: ../src/Build.sh ../src/sh_flags.opt
			srcfile=../src/sh_flags.opt; BUILDSH_RUN_GENOPT=1; . ../src/Build.sh

# not BSD make only:
#VPATH=		../src
#all: $(PROG)
#$(PROG): $(OBJS_BP)
#	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS_BP) $(LIBS)
#$(OBJS_BP): $(SRCS_FP) $(NONSRCS)
#.c.o:
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

# for all make variants:
#REGRESS_FLAGS=	-f
#regress:
#	./test.sh $(REGRESS_FLAGS)
check_categories= shell:legacy-no int:32 android convfds no-histfile

# for BSD make only:
#.PATH: ../src
#.include <bsd.prog.mk>
