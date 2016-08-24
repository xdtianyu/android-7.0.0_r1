/* seccomp_bpf_tests.c
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Test code for seccomp bpf.
 */

/* Need to include sys/types.h before asm/siginfo.h such that clock_t, pid_t,
 * and timer_t are defined. */
#include <sys/types.h>
#include <asm/siginfo.h>
#define __have_siginfo_t 1
#define __have_sigval_t 1
#define __have_sigevent_t 1

#include <errno.h>
#include <linux/filter.h>
#include <linux/prctl.h>
#include <linux/ptrace.h>
#include <linux/seccomp.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <syscall.h>

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>

#include "test_harness.h"

#ifndef PR_SET_PTRACER
# define PR_SET_PTRACER 0x59616d61
#endif

#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS 38
#define PR_GET_NO_NEW_PRIVS 39
#endif

#ifndef PR_SECCOMP_EXT
#define PR_SECCOMP_EXT 43
#endif

#ifndef SECCOMP_EXT_ACT
#define SECCOMP_EXT_ACT 1
#endif

#ifndef SECCOMP_EXT_ACT_TSYNC
#define SECCOMP_EXT_ACT_TSYNC 1
#endif

#ifndef SECCOMP_MODE_STRICT
#define SECCOMP_MODE_STRICT 1
#endif

#ifndef SECCOMP_MODE_FILTER
#define SECCOMP_MODE_FILTER 2
#endif

#ifndef SECCOMP_RET_KILL
#define SECCOMP_RET_KILL        0x00000000U // kill the task immediately
#define SECCOMP_RET_TRAP        0x00030000U // disallow and force a SIGSYS
#define SECCOMP_RET_ERRNO       0x00050000U // returns an errno
#define SECCOMP_RET_TRACE       0x7ff00000U // pass to a tracer or disallow
#define SECCOMP_RET_ALLOW       0x7fff0000U // allow

/* Masks for the return value sections. */
#define SECCOMP_RET_ACTION      0x7fff0000U
#define SECCOMP_RET_DATA        0x0000ffffU

struct seccomp_data {
	int nr;
	__u32 arch;
	__u64 instruction_pointer;
	__u64 args[6];
};
#endif

#define syscall_arg(_n) (offsetof(struct seccomp_data, args[_n]))

#define SIBLING_EXIT_UNKILLED	0xbadbeef
#define SIBLING_EXIT_FAILURE	0xbadface

TEST(mode_strict_support) {
	long ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT, NULL, NULL, NULL);
	ASSERT_EQ(0, ret) {
		TH_LOG("Kernel does not support CONFIG_SECCOMP");
	}
	syscall(__NR_exit, 1);
}

TEST_SIGNAL(mode_strict_cannot_call_prctl, SIGKILL) {
	long ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT, NULL, NULL, NULL);
	ASSERT_EQ(0, ret) {
		TH_LOG("Kernel does not support CONFIG_SECCOMP");
	}
	syscall(__NR_prctl, PR_SET_SECCOMP, SECCOMP_MODE_FILTER, NULL, NULL, NULL);
	EXPECT_FALSE(true) {
		TH_LOG("Unreachable!");
	}
}

/* Note! This doesn't test no new privs behavior */
TEST(no_new_privs_support) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	EXPECT_EQ(0, ret) {
		TH_LOG("Kernel does not support PR_SET_NO_NEW_PRIVS!");
	}
}

/* Tests kernel support by checking for a copy_from_user() fault on * NULL. */
TEST(mode_filter_support) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, NULL, 0, 0);
	ASSERT_EQ(0, ret) {
		TH_LOG("Kernel does not support PR_SET_NO_NEW_PRIVS!");
	}
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, NULL, NULL, NULL);
	EXPECT_EQ(-1, ret);
	EXPECT_EQ(EFAULT, errno) {
		TH_LOG("Kernel does not support CONFIG_SECCOMP_FILTER!");
	}
}

TEST(mode_filter_without_nnp) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};
	long ret = prctl(PR_GET_NO_NEW_PRIVS, 0, NULL, 0, 0);
	ASSERT_LE(0, ret) {
		TH_LOG("Expected 0 or unsupported for NO_NEW_PRIVS");
	}
	errno = 0;
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog, 0, 0);
	/* Succeeds with CAP_SYS_ADMIN, fails without */
	/* TODO(wad) check caps not euid */
	if (geteuid()) {
		EXPECT_EQ(-1, ret);
		EXPECT_EQ(EACCES, errno);
	} else {
		EXPECT_EQ(0, ret);
	}
}

TEST(mode_filter_cannot_move_to_strict) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};

	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog, 0, 0);
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT, NULL, 0, 0);
	EXPECT_EQ(-1, ret);
	EXPECT_EQ(EINVAL, errno);
}


TEST(ALLOW_all) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};

	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
	ASSERT_EQ(0, ret);
}

TEST(empty_prog) {
	struct sock_filter filter[] = {
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};

	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
	EXPECT_EQ(-1, ret);
	EXPECT_EQ(EINVAL, errno);
}

TEST_SIGNAL(unknown_ret_is_kill_inside, SIGSYS) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_RET+BPF_K, 0x10000000U),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};

	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
	ASSERT_EQ(0, ret);
	EXPECT_EQ(0, syscall(__NR_getpid)) {
		TH_LOG("getpid() shouldn't ever return");
	}
}

/* return code >= 0x80000000 is unused. */
TEST_SIGNAL(unknown_ret_is_kill_above_allow, SIGSYS) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_RET+BPF_K, 0x90000000U),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};

	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
	ASSERT_EQ(0, ret);
	EXPECT_EQ(0, syscall(__NR_getpid)) {
		TH_LOG("getpid() shouldn't ever return");
	}
}

TEST_SIGNAL(KILL_all, SIGSYS) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};

	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
	ASSERT_EQ(0, ret);
}

TEST_SIGNAL(KILL_one, SIGSYS) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_getpid, 0, 1),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
	ASSERT_EQ(0, ret);

	EXPECT_EQ(parent, syscall(__NR_getppid));
	/* getpid() should never return. */
	EXPECT_EQ(0, syscall(__NR_getpid));
}

TEST_SIGNAL(KILL_one_arg_one, SIGSYS) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_getpid, 1, 0),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
		/* Only both with lower 32-bit for now. */
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS, syscall_arg(0)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0x0C0FFEE, 0, 1),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	pid_t pid = getpid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
	ASSERT_EQ(0, ret);

	EXPECT_EQ(parent, syscall(__NR_getppid));
	EXPECT_EQ(pid, syscall(__NR_getpid));
	/* getpid() should never return. */
	EXPECT_EQ(0, syscall(__NR_getpid, 0x0C0FFEE));
}

TEST_SIGNAL(KILL_one_arg_six, SIGSYS) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_getpid, 1, 0),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
		/* Only both with lower 32-bit for now. */
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS, syscall_arg(5)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0x0C0FFEE, 0, 1),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	pid_t pid = getpid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
	ASSERT_EQ(0, ret);

	EXPECT_EQ(parent, syscall(__NR_getppid));
	EXPECT_EQ(pid, syscall(__NR_getpid));
	/* getpid() should never return. */
	EXPECT_EQ(0, syscall(__NR_getpid, 1, 2, 3, 4, 5, 0x0C0FFEE));
}

/* TODO(wad) add 64-bit versus 32-bit arg tests. */

TEST(arg_out_of_range) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS, syscall_arg(6)),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
	EXPECT_EQ(-1, ret);
	EXPECT_EQ(EINVAL, errno);
}

TEST(ERRNO_one) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_read, 0, 1),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ERRNO | E2BIG),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
	ASSERT_EQ(0, ret);

	EXPECT_EQ(parent, syscall(__NR_getppid));
	EXPECT_EQ(-1, read(0, NULL, 0));
	EXPECT_EQ(E2BIG, errno);
}

TEST(ERRNO_one_ok) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_read, 0, 1),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ERRNO | 0),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
	ASSERT_EQ(0, ret);

	EXPECT_EQ(parent, syscall(__NR_getppid));
	/* "errno" of 0 is ok. */
	EXPECT_EQ(0, read(0, NULL, 0));
}

FIXTURE_DATA(TRAP) {
	struct sock_fprog prog;
};

FIXTURE_SETUP(TRAP) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_getpid, 0, 1),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRAP),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	memset(&self->prog, 0, sizeof(self->prog));
	self->prog.filter = malloc(sizeof(filter));
	ASSERT_NE(NULL, self->prog.filter);
	memcpy(self->prog.filter, filter, sizeof(filter));
	self->prog.len = (unsigned short)(sizeof(filter)/sizeof(filter[0]));
}

FIXTURE_TEARDOWN(TRAP) {
	if (self->prog.filter)
		free(self->prog.filter);
};

TEST_F_SIGNAL(TRAP, dfl, SIGSYS) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->prog);
	ASSERT_EQ(0, ret);
	syscall(__NR_getpid);
}

/* Ensure that SIGSYS overrides SIG_IGN */
TEST_F_SIGNAL(TRAP, ign, SIGSYS) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);

	signal(SIGSYS, SIG_IGN);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->prog);
	ASSERT_EQ(0, ret);
	syscall(__NR_getpid);
}

static struct siginfo TRAP_info;
static volatile int TRAP_nr;
static void TRAP_action(int nr, siginfo_t *info, void *void_context)
{
	memcpy(&TRAP_info, info, sizeof(TRAP_info));
	TRAP_nr = nr;
	return;
}

TEST_F(TRAP, handler) {
	int ret, test;
	struct sigaction act;
	sigset_t mask;
	memset(&act, 0, sizeof(act));
	sigemptyset(&mask);
	sigaddset(&mask, SIGSYS);

	act.sa_sigaction = &TRAP_action;
	act.sa_flags = SA_SIGINFO;
	ret = sigaction(SIGSYS, &act, NULL);
	ASSERT_EQ(0, ret) {
		TH_LOG("sigaction failed");
	}
	ret = sigprocmask(SIG_UNBLOCK, &mask, NULL);
	ASSERT_EQ(0, ret) {
		TH_LOG("sigprocmask failed");
	}

	ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->prog);
	ASSERT_EQ(0, ret);
	TRAP_nr = 0;
	memset(&TRAP_info, 0, sizeof(TRAP_info));
	/* Expect the registers to be rolled back. (nr = error) may vary
	 * based on arch. */
	ret = syscall(__NR_getpid);
	/* Silence gcc warning about volatile. */
	test = TRAP_nr;
	EXPECT_EQ(SIGSYS, test);
	struct local_sigsys {
			void *_call_addr; /* calling user insn */
			int _syscall;	/* triggering system call number */
			unsigned int _arch;	/* AUDIT_ARCH_* of syscall */
	} *sigsys = (struct local_sigsys *)
#ifdef si_syscall
		&(TRAP_info.si_call_addr);
#else
		&TRAP_info.si_pid;
#endif
	EXPECT_EQ(__NR_getpid, sigsys->_syscall);
	/* Make sure arch is non-zero. */
	EXPECT_NE(0, sigsys->_arch);
	EXPECT_NE(0, (unsigned long)sigsys->_call_addr);
}

FIXTURE_DATA(precedence) {
	struct sock_fprog allow;
	struct sock_fprog trace;
	struct sock_fprog error;
	struct sock_fprog trap;
	struct sock_fprog kill;
};

FIXTURE_SETUP(precedence) {
	struct sock_filter allow_insns[] = {
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_filter trace_insns[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_getpid, 1, 0),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRACE),
	};
	struct sock_filter error_insns[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_getpid, 1, 0),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ERRNO),
	};
	struct sock_filter trap_insns[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_getpid, 1, 0),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRAP),
	};
	struct sock_filter kill_insns[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_getpid, 1, 0),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL),
	};
	memset(self, 0, sizeof(*self));
#define FILTER_ALLOC(_x) \
	self->_x.filter = malloc(sizeof(_x##_insns)); \
	ASSERT_NE(NULL, self->_x.filter); \
	memcpy(self->_x.filter, &_x##_insns, sizeof(_x##_insns)); \
	self->_x.len = (unsigned short)(sizeof(_x##_insns)/sizeof(_x##_insns[0]))
	FILTER_ALLOC(allow);
	FILTER_ALLOC(trace);
	FILTER_ALLOC(error);
	FILTER_ALLOC(trap);
	FILTER_ALLOC(kill);
}

FIXTURE_TEARDOWN(precedence) {
#define FILTER_FREE(_x) if (self->_x.filter) free(self->_x.filter)
	FILTER_FREE(allow);
	FILTER_FREE(trace);
	FILTER_FREE(error);
	FILTER_FREE(trap);
	FILTER_FREE(kill);
}

TEST_F(precedence, allow_ok) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	pid_t res = 0;
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->allow);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trace);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->error);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trap);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->kill);
	ASSERT_EQ(0, ret);
	/* Should work just fine. */
	res = syscall(__NR_getppid);
	EXPECT_EQ(parent, res);
}

TEST_F_SIGNAL(precedence, kill_is_highest, SIGSYS) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	pid_t res = 0;
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->allow);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trace);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->error);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trap);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->kill);
	ASSERT_EQ(0, ret);
	/* Should work just fine. */
	res = syscall(__NR_getppid);
	EXPECT_EQ(parent, res);
	/* getpid() should never return. */
	res = syscall(__NR_getpid);
	EXPECT_EQ(0, res);
}

TEST_F_SIGNAL(precedence, kill_is_highest_in_any_order, SIGSYS) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->allow);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->kill);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->error);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trace);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trap);
	ASSERT_EQ(0, ret);
	/* Should work just fine. */
	EXPECT_EQ(parent, syscall(__NR_getppid));
	/* getpid() should never return. */
	EXPECT_EQ(0, syscall(__NR_getpid));
}

TEST_F_SIGNAL(precedence, trap_is_second, SIGSYS) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->allow);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trace);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->error);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trap);
	ASSERT_EQ(0, ret);
	/* Should work just fine. */
	EXPECT_EQ(parent, syscall(__NR_getppid));
	/* getpid() should never return. */
	EXPECT_EQ(0, syscall(__NR_getpid));
}

TEST_F_SIGNAL(precedence, trap_is_second_in_any_order, SIGSYS) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->allow);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trap);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trace);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->error);
	ASSERT_EQ(0, ret);
	/* Should work just fine. */
	EXPECT_EQ(parent, syscall(__NR_getppid));
	/* getpid() should never return. */
	EXPECT_EQ(0, syscall(__NR_getpid));
}

TEST_F(precedence, errno_is_third) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->allow);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trace);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->error);
	ASSERT_EQ(0, ret);
	/* Should work just fine. */
	EXPECT_EQ(parent, syscall(__NR_getppid));
	EXPECT_EQ(0, syscall(__NR_getpid));
}

TEST_F(precedence, errno_is_third_in_any_order) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->error);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trace);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->allow);
	ASSERT_EQ(0, ret);
	/* Should work just fine. */
	EXPECT_EQ(parent, syscall(__NR_getppid));
	EXPECT_EQ(0, syscall(__NR_getpid));
}

TEST_F(precedence, trace_is_fourth) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->allow);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trace);
	ASSERT_EQ(0, ret);
	/* Should work just fine. */
	EXPECT_EQ(parent, syscall(__NR_getppid));
	/* No ptracer */
	EXPECT_EQ(-1, syscall(__NR_getpid));
}

TEST_F(precedence, trace_is_fourth_in_any_order) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	pid_t parent = getppid();
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->trace);
	ASSERT_EQ(0, ret);
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->allow);
	ASSERT_EQ(0, ret);
	/* Should work just fine. */
	EXPECT_EQ(parent, syscall(__NR_getppid));
	/* No ptracer */
	EXPECT_EQ(-1, syscall(__NR_getpid));
}

#ifndef PTRACE_O_TRACESECCOMP
#define PTRACE_O_TRACESECCOMP	0x00000080
#endif

/* Catch the Ubuntu 12.04 value error. */
#if PTRACE_EVENT_SECCOMP != 7
#undef PTRACE_EVENT_SECCOMP
#endif

#ifndef PTRACE_EVENT_SECCOMP
#define PTRACE_EVENT_SECCOMP 7
#endif

#define IS_SECCOMP_EVENT(status) ((status >> 16) == PTRACE_EVENT_SECCOMP)
bool tracer_running;
void tracer_stop(int sig)
{
	tracer_running = false;
}
void tracer(struct __test_metadata *_metadata, pid_t tracee,
	    unsigned long poke_addr, int fd) {
	int ret = -1;
	struct sigaction action = {
		.sa_handler = tracer_stop,
	};

	/* Allow external shutdown. */
	tracer_running = true;
	ASSERT_EQ(0, sigaction(SIGUSR1, &action, NULL));

	errno = 0;
	while (ret == -1 && errno != EINVAL) {
		ret = ptrace(PTRACE_ATTACH, tracee, NULL, 0);
	}
	ASSERT_EQ(0, ret) {
		kill(tracee, SIGKILL);
	}
	/* Wait for attach stop */
	wait(NULL);

	ret = ptrace(PTRACE_SETOPTIONS, tracee, NULL, PTRACE_O_TRACESECCOMP);
	ASSERT_EQ(0, ret) {
		TH_LOG("Failed to set PTRACE_O_TRACESECCOMP");
		kill(tracee, SIGKILL);
	}
	ptrace(PTRACE_CONT, tracee, NULL, 0);

	/* Unblock the tracee */
	ASSERT_EQ(1, write(fd, "A", 1));
	ASSERT_EQ(0, close(fd));

	/* Run until we're shut down. */
	while (tracer_running) {
		int status;
		unsigned long msg;
		if (wait(&status) != tracee)
			continue;
		if (WIFSIGNALED(status) || WIFEXITED(status))
			/* Child is dead. Time to go. */
			return;

		/* Make sure this is a seccomp event. */
		EXPECT_EQ(true, IS_SECCOMP_EVENT(status));

		ret = ptrace(PTRACE_GETEVENTMSG, tracee, NULL, &msg);
		EXPECT_EQ(0, ret);
		/* If this fails, don't try to recover. */
		ASSERT_EQ(0x1001, msg) {
			kill(tracee, SIGKILL);
		}
		/*
		 * Poke in the message.
		 * Registers are not touched to try to keep this relatively arch
		 * agnostic.
		 */
		ret = ptrace(PTRACE_POKEDATA, tracee, poke_addr, 0x1001);
		EXPECT_EQ(0, ret);
		ret = ptrace(PTRACE_CONT, tracee, NULL, NULL);
		EXPECT_EQ(0, ret);
	}
	/* Directly report the status of our test harness results. */
	syscall(__NR_exit, _metadata->passed ? EXIT_SUCCESS : EXIT_FAILURE);
}

FIXTURE_DATA(TRACE) {
	struct sock_fprog prog;
	pid_t tracer;
	long poked;
};

void cont_handler(int num) {
}

FIXTURE_SETUP(TRACE) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_read, 0, 1),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRACE | 0x1001),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	int pipefd[2];
	char sync;
	pid_t tracer_pid;
	pid_t tracee = getpid();
	unsigned long poke_addr = (unsigned long)&self->poked;
	self->poked = 0;
	memset(&self->prog, 0, sizeof(self->prog));
	self->prog.filter = malloc(sizeof(filter));
	ASSERT_NE(NULL, self->prog.filter);
	memcpy(self->prog.filter, filter, sizeof(filter));
	self->prog.len = (unsigned short)(sizeof(filter)/sizeof(filter[0]));

	/* Setup a pipe for clean synchronization. */
	ASSERT_EQ(0, pipe(pipefd));

	/* Fork a child which we'll promote to tracer */
	tracer_pid = fork();
	ASSERT_LE(0, tracer_pid);
	signal(SIGALRM, cont_handler);
	if (tracer_pid == 0) {
		close(pipefd[0]);
		tracer(_metadata, tracee, poke_addr, pipefd[1]);
		syscall(__NR_exit, 0);
	}
	close(pipefd[1]);
	self->tracer = tracer_pid;
	prctl(PR_SET_PTRACER, self->tracer, 0, 0, 0);
	long ret = read(pipefd[0], &sync, 1);
	close(pipefd[0]);
}

FIXTURE_TEARDOWN(TRACE) {
	if (self->tracer) {
		int status;
		/*
		 * Extract the exit code from the other process and
		 * adopt it for ourselves in case its asserts failed.
		 */
		ASSERT_EQ(0, kill(self->tracer, SIGUSR1));
		ASSERT_EQ(self->tracer, waitpid(self->tracer, &status, 0));
		if (WEXITSTATUS(status))
			_metadata->passed = 0;
	}
	if (self->prog.filter)
		free(self->prog.filter);
};

TEST_F(TRACE, read_has_side_effects) {
	ssize_t ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->prog, 0, 0);
	ASSERT_EQ(0, ret);

	EXPECT_EQ(0, self->poked);
	ret = read(-1, NULL, 0);
	EXPECT_EQ(-1, ret);
	EXPECT_EQ(0x1001, self->poked);
}

TEST_F(TRACE, getpid_runs_normally) {
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret);

	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &self->prog, 0, 0);
	ASSERT_EQ(0, ret);

	EXPECT_EQ(0, self->poked);
	EXPECT_NE(0, syscall(__NR_getpid));
	EXPECT_EQ(0, self->poked);
}

#ifndef __NR_seccomp
# if defined(__i386__)
#  define __NR_seccomp 354
# elif defined(__x86_64__)
#  define __NR_seccomp 317
# else
#  define __NR_seccomp 0xffff
# endif
#endif

#ifndef SECCOMP_SET_MODE_STRICT
#define SECCOMP_SET_MODE_STRICT 0
#endif

#ifndef SECCOMP_SET_MODE_FILTER
#define SECCOMP_SET_MODE_FILTER 1
#endif

#ifndef SECCOMP_FLAG_FILTER_TSYNC
#define SECCOMP_FLAG_FILTER_TSYNC 1
#endif

#ifndef seccomp
int seccomp(unsigned int op, unsigned int flags, struct sock_fprog *filter)
{
	errno = 0;
	return syscall(__NR_seccomp, op, flags, filter);
}
#endif

/* The following tests are commented out because they test
 * features that are currently unsupported.
 */
#if 0

TEST(seccomp_syscall) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	ASSERT_EQ(0, ret) {
		TH_LOG("Kernel does not support PR_SET_NO_NEW_PRIVS!");
	}

	/* Reject insane operation. */
	ret = seccomp(-1, 0, &prog);
	EXPECT_EQ(EINVAL, errno) {
		TH_LOG("Did not reject crazy op value!");
	}

	/* Reject strict with flags or pointer. */
	ret = seccomp(SECCOMP_SET_MODE_STRICT, -1, NULL);
	EXPECT_EQ(EINVAL, errno) {
		TH_LOG("Did not reject mode strict with flags!");
	}
	ret = seccomp(SECCOMP_SET_MODE_STRICT, 0, &prog);
	EXPECT_EQ(EINVAL, errno) {
		TH_LOG("Did not reject mode strict with uargs!");
	}

	/* Reject insane args for filter. */
	ret = seccomp(SECCOMP_SET_MODE_FILTER, -1, &prog);
	EXPECT_EQ(EINVAL, errno) {
		TH_LOG("Did not reject crazy filter flags!");
	}
	ret = seccomp(SECCOMP_SET_MODE_FILTER, 0, NULL);
	EXPECT_EQ(EFAULT, errno) {
		TH_LOG("Did not reject NULL filter!");
	}

	ret = seccomp(SECCOMP_SET_MODE_FILTER, 0, &prog);
	EXPECT_EQ(0, errno) {
		TH_LOG("Kernel does not support SECCOMP_SET_MODE_FILTER: %s",
			strerror(errno));
	}
}

TEST(seccomp_syscall_mode_lock) {
	struct sock_filter filter[] = {
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};
	long ret = prctl(PR_SET_NO_NEW_PRIVS, 1, NULL, 0, 0);
	ASSERT_EQ(0, ret) {
		TH_LOG("Kernel does not support PR_SET_NO_NEW_PRIVS!");
	}

	ret = seccomp(SECCOMP_SET_MODE_FILTER, 0, &prog);
	EXPECT_EQ(0, ret) {
		TH_LOG("Could not install filter!");
	}

	/* Make sure neither entry point will switch to strict. */
	ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT, 0, 0, 0);
	EXPECT_EQ(EINVAL, errno) {
		TH_LOG("Switched to mode strict!");
	}

	ret = seccomp(SECCOMP_SET_MODE_STRICT, 0, NULL);
	EXPECT_EQ(EINVAL, errno) {
		TH_LOG("Switched to mode strict!");
	}
}

#define TSYNC_SIBLINGS 2
struct tsync_sibling {
	pthread_t tid;
	pid_t system_tid;
	sem_t *started;
	pthread_cond_t *cond;
	pthread_mutex_t *mutex;
	int diverge;
	int num_waits;
	struct sock_fprog *prog;
	struct __test_metadata *metadata;
};

FIXTURE_DATA(TSYNC) {
	struct sock_fprog root_prog, apply_prog;
	struct tsync_sibling sibling[TSYNC_SIBLINGS];
	sem_t started;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int sibling_count;
};

FIXTURE_SETUP(TSYNC) {
	struct sock_filter root_filter[] = {
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_filter apply_filter[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_read, 0, 1),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	memset(&self->root_prog, 0, sizeof(self->root_prog));
	memset(&self->apply_prog, 0, sizeof(self->apply_prog));
	memset(&self->sibling, 0, sizeof(self->sibling));
	self->root_prog.filter = malloc(sizeof(root_filter));
	ASSERT_NE(NULL, self->root_prog.filter);
	memcpy(self->root_prog.filter, &root_filter, sizeof(root_filter));
	self->root_prog.len = (unsigned short)(sizeof(root_filter)/sizeof(root_filter[0]));

	self->apply_prog.filter = malloc(sizeof(apply_filter));
	ASSERT_NE(NULL, self->apply_prog.filter);
	memcpy(self->apply_prog.filter, &apply_filter, sizeof(apply_filter));
	self->apply_prog.len = (unsigned short)(sizeof(apply_filter)/sizeof(apply_filter[0]));

	self->sibling_count = 0;
	pthread_mutex_init(&self->mutex, NULL);
	pthread_cond_init(&self->cond, NULL);
	sem_init(&self->started, 0, 0);
	self->sibling[0].tid = 0;
	self->sibling[0].cond = &self->cond;
	self->sibling[0].started = &self->started;
	self->sibling[0].mutex = &self->mutex;
	self->sibling[0].diverge = 0;
	self->sibling[0].num_waits = 1;
	self->sibling[0].prog = &self->root_prog;
	self->sibling[0].metadata = _metadata;
	self->sibling[1].tid = 0;
	self->sibling[1].cond = &self->cond;
	self->sibling[1].started = &self->started;
	self->sibling[1].mutex = &self->mutex;
	self->sibling[1].diverge = 0;
	self->sibling[1].prog = &self->root_prog;
	self->sibling[1].num_waits = 1;
	self->sibling[1].metadata = _metadata;

	ASSERT_EQ(0, prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
		TH_LOG("Kernel does not support PR_SET_NO_NEW_PRIVS!");
	}
}

FIXTURE_TEARDOWN(TSYNC) {
	int sib = 0;
	if (self->root_prog.filter)
		free(self->root_prog.filter);
	if (self->apply_prog.filter)
		free(self->apply_prog.filter);

	for ( ; sib < self->sibling_count; ++sib) {
		struct tsync_sibling *s = &self->sibling[sib];
		void *status;
		if (!s->tid)
			continue;
		if (pthread_kill(s->tid, 0)) {
			pthread_cancel(s->tid);
			pthread_join(s->tid, &status);
		}
	}
	pthread_mutex_destroy(&self->mutex);
	pthread_cond_destroy(&self->cond);
	sem_destroy(&self->started);
};

void *tsync_sibling(void *data)
{
	long ret = 0;
	struct tsync_sibling *me = data;
	struct __test_metadata *_metadata = me->metadata; /* enable TH_LOG */
	me->system_tid = syscall(__NR_gettid);

	pthread_mutex_lock(me->mutex);
	if (me->diverge) {
		/* Just re-apply the root prog to fork the tree */
		ret = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER,
				me->prog, 0, 0);
	}
	sem_post(me->started);
	/* Return outside of started so parent notices failures. */
	if (ret) {
		pthread_mutex_unlock(me->mutex);
		return (void *)SIBLING_EXIT_FAILURE;
	}
	do {
		pthread_cond_wait(me->cond, me->mutex);
		me->num_waits = me->num_waits - 1;
	}
	while (me->num_waits);
	pthread_mutex_unlock(me->mutex);
	ret = read(0, NULL, 0);
	return (void *)SIBLING_EXIT_UNKILLED;
}

void tsync_start_sibling(struct tsync_sibling *sibling)
{
	pthread_create(&sibling->tid, NULL, tsync_sibling, (void *)sibling);
}

TEST_F(TSYNC, siblings_fail_prctl) {
	long ret, sib;
	void *status;
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
			offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_prctl, 0, 1),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ERRNO | EINVAL),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};

	/* Check prctl failure detection by requesting sib 0 diverge. */
	ret = seccomp(SECCOMP_SET_MODE_FILTER, 0, &prog);

	self->sibling[0].diverge = 1;
	tsync_start_sibling(&self->sibling[0]);
	tsync_start_sibling(&self->sibling[1]);

	while (self->sibling_count < TSYNC_SIBLINGS) {
		sem_wait(&self->started);
		self->sibling_count++;
	}

	/* Signal the threads to clean up*/
	pthread_mutex_lock(&self->mutex);
	ASSERT_EQ(0, pthread_cond_broadcast(&self->cond)) {
		TH_LOG("cond broadcast non-zero");
	}
	pthread_mutex_unlock(&self->mutex);

	/* Ensure diverging sibling failed to call prctl. */
	pthread_join(self->sibling[0].tid, &status);
	EXPECT_EQ(SIBLING_EXIT_FAILURE, (long)status);
	pthread_join(self->sibling[1].tid, &status);
	EXPECT_EQ(SIBLING_EXIT_UNKILLED, (long)status);
}

TEST_F(TSYNC, two_siblings_with_ancestor) {
	long ret = seccomp(SECCOMP_SET_MODE_FILTER, 0, &self->root_prog);
	void *status;
	ASSERT_EQ(0, ret) {
		TH_LOG("Kernel does not support SECCOMP_SET_MODE_FILTER!");
	}
	tsync_start_sibling(&self->sibling[0]);
	tsync_start_sibling(&self->sibling[1]);

	while (self->sibling_count < TSYNC_SIBLINGS) {
		sem_wait(&self->started);
		self->sibling_count++;
	}

	ret = seccomp(SECCOMP_SET_MODE_FILTER, SECCOMP_FLAG_FILTER_TSYNC,
		      &self->apply_prog);
	ASSERT_EQ(0, ret) {
		TH_LOG("Could install filter on all threads!");
	}
	/* Tell the siblings to test the policy */
	pthread_mutex_lock(&self->mutex);
	ASSERT_EQ(0, pthread_cond_broadcast(&self->cond)) {
		TH_LOG("cond broadcast non-zero");
	}
	pthread_mutex_unlock(&self->mutex);
	/* Ensure they are both killed and don't exit cleanly. */
	pthread_join(self->sibling[0].tid, &status);
	EXPECT_EQ(0x0, (long)status);
	pthread_join(self->sibling[1].tid, &status);
	EXPECT_EQ(0x0, (long)status);
}

TEST_F(TSYNC, two_siblings_with_one_divergence) {
	long ret = seccomp(SECCOMP_SET_MODE_FILTER, 0, &self->root_prog);
	void *status;
	ASSERT_EQ(0, ret) {
		TH_LOG("Kernel does not support SECCOMP_SET_MODE_FILTER!");
	}
	self->sibling[0].diverge = 1;
	tsync_start_sibling(&self->sibling[0]);
	tsync_start_sibling(&self->sibling[1]);

	while (self->sibling_count < TSYNC_SIBLINGS) {
		sem_wait(&self->started);
		self->sibling_count++;
	}

	ret = seccomp(SECCOMP_SET_MODE_FILTER, SECCOMP_FLAG_FILTER_TSYNC,
		      &self->apply_prog);
	ASSERT_EQ(self->sibling[0].system_tid, ret) {
		TH_LOG("Did not fail on diverged sibling.");
	}

	/* Wake the threads */
	pthread_mutex_lock(&self->mutex);
	ASSERT_EQ(0, pthread_cond_broadcast(&self->cond)) {
		TH_LOG("cond broadcast non-zero");
	}
	pthread_mutex_unlock(&self->mutex);

	/* Ensure they are both unkilled. */
	pthread_join(self->sibling[0].tid, &status);
	EXPECT_EQ(SIBLING_EXIT_UNKILLED, (long)status);
	pthread_join(self->sibling[1].tid, &status);
	EXPECT_EQ(SIBLING_EXIT_UNKILLED, (long)status);
}

TEST_F(TSYNC, two_siblings_not_under_filter) {
	long ret, sib;
	void *status;
	/*
	 * Sibling 0 will have its own seccomp policy
	 * and Sibling 1 will not be under seccomp at
	 * all. Sibling 1 will enter seccomp and 0
	 * will cause failure.
	 */
	self->sibling[0].diverge = 1;
	tsync_start_sibling(&self->sibling[0]);
	tsync_start_sibling(&self->sibling[1]);

	while (self->sibling_count < TSYNC_SIBLINGS) {
		sem_wait(&self->started);
		self->sibling_count++;
	}

	ret = seccomp(SECCOMP_SET_MODE_FILTER, 0, &self->root_prog);
	ASSERT_EQ(0, ret) {
		TH_LOG("Kernel does not support SECCOMP_SET_MODE_FILTER!");
	}

	ret = seccomp(SECCOMP_SET_MODE_FILTER, SECCOMP_FLAG_FILTER_TSYNC,
		      &self->apply_prog);
	ASSERT_EQ(ret, self->sibling[0].system_tid) {
		TH_LOG("Did not fail on diverged sibling.");
	}
	sib = 1;
	if (ret == self->sibling[0].system_tid)
		sib = 0;

	pthread_mutex_lock(&self->mutex);

	/* Increment the other siblings num_waits so we can clean up
	 * the one we just saw.
	 */
	self->sibling[!sib].num_waits += 1;

	/* Signal the thread to clean up*/
	ASSERT_EQ(0, pthread_cond_broadcast(&self->cond)) {
		TH_LOG("cond broadcast non-zero");
	}
	pthread_mutex_unlock(&self->mutex);
	pthread_join(self->sibling[sib].tid, &status);
	EXPECT_EQ(SIBLING_EXIT_UNKILLED, (long)status);
	/* Poll for actual task death. pthread_join doesn't guarantee it. */
	while (!kill(self->sibling[sib].system_tid, 0)) sleep(0.1);
	/* Switch to the remaining sibling */
	sib = !sib;

	ret = seccomp(SECCOMP_SET_MODE_FILTER, SECCOMP_FLAG_FILTER_TSYNC,
		      &self->apply_prog);
	ASSERT_EQ(0, ret) {
		TH_LOG("Expected the remaining sibling to sync");
	};

	pthread_mutex_lock(&self->mutex);

	/* If remaining sibling didn't have a chance to wake up during
	 * the first broadcast, manually reduce the num_waits now.
	 */
	if (self->sibling[sib].num_waits > 1)
		self->sibling[sib].num_waits = 1;
	ASSERT_EQ(0, pthread_cond_broadcast(&self->cond)) {
		TH_LOG("cond broadcast non-zero");
	}
	pthread_mutex_unlock(&self->mutex);
	pthread_join(self->sibling[sib].tid, &status);
	EXPECT_EQ(0, (long)status);
	/* Poll for actual task death. pthread_join doesn't guarantee it. */
	while (!kill(self->sibling[sib].system_tid, 0)) sleep(0.1);

	ret = seccomp(SECCOMP_SET_MODE_FILTER, SECCOMP_FLAG_FILTER_TSYNC,
		      &self->apply_prog);
	ASSERT_EQ(0, ret);  /* just us chickens */
}

#endif

/*
 * TODO:
 * - add microbenchmarks
 * - expand NNP testing
 * - better arch-specific TRACE and TRAP handlers.
 * - endianness checking when appropriate
 * - 64-bit arg prodding
 * - arch value testing (x86 modes especially)
 * - ...
 */

TEST_HARNESS_MAIN
