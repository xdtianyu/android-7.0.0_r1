/* sigsegv.c
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Forces a denied system call to trigger a SIGSEGV at the instruction after
 * the call using a SIGSYS handler.. This can be useful when debugging
 * frameworks have trouble tracing through the SIGSYS handler.
 * Proof of concept using amd64 registers and 'syscall'.
 */

#include <asm/siginfo.h>
#define __have_siginfo_t 1
#define __have_sigval_t 1
#define __have_sigevent_t 1

#include <linux/filter.h>
#include <sys/prctl.h>
#include <linux/prctl.h>
#include <linux/seccomp.h>
#include <limits.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <syscall.h>
#define __USE_GNU 1
#include <sys/ucontext.h>
#include <sys/mman.h>

#include "test_harness.h"

#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS 38
#define PR_GET_NO_NEW_PRIVS 39
#endif

#if defined(__i386__)
#define REG_IP	REG_EIP
#define REG_SP	REG_ESP
#define REG_RESULT	REG_EAX
#define REG_SYSCALL	REG_EAX
#define REG_ARG0	REG_EBX
#define REG_ARG1	REG_ECX
#define REG_ARG2	REG_EDX
#define REG_ARG3	REG_ESI
#define REG_ARG4	REG_EDI
#define REG_ARG5	REG_EBP
#elif defined(__x86_64__)
#define REG_IP	REG_RIP
#define REG_SP	REG_RSP
#define REG_RESULT	REG_RAX
#define REG_SYSCALL	REG_RAX
#define REG_ARG0	REG_RDI
#define REG_ARG1	REG_RSI
#define REG_ARG2	REG_RDX
#define REG_ARG3	REG_R10
#define REG_ARG4	REG_R8
#define REG_ARG5	REG_R9
#endif

FIXTURE_DATA(TRAP) {
	struct sock_fprog prog;
};

FIXTURE_SETUP(TRAP) {
	/* instruction after the syscall. Will be arch specific, of course. */
	{
		struct sock_filter filter[] = {
			BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
				offsetof(struct seccomp_data, nr)),
			/* Whitelist anything you might need in the sigaction */
#ifdef __NR_sigreturn
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_sigreturn, 4, 0),
#endif
			/* TODO: only allow PROT_NONE */
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_mprotect, 3, 0),
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_exit, 2, 0),
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_rt_sigreturn, 1, 0),
			/* Allow __NR_write so easy logging. */
			BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_write, 0, 1),
			BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
			BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRAP),
		};
		memset(&self->prog, 0, sizeof(self->prog));
		self->prog.filter = malloc(sizeof(filter));
		ASSERT_NE(NULL, self->prog.filter);
		memcpy(self->prog.filter, filter, sizeof(filter));
		self->prog.len = (unsigned short)(sizeof(filter)/sizeof(filter[0]));
	}
}

FIXTURE_TEARDOWN(TRAP) {
	if (self->prog.filter)
		free(self->prog.filter);
};

struct arch_sigsys {
		void *_call_addr; /* calling user insn */
		int _syscall;	/* triggering system call number */
		unsigned int _arch;	/* AUDIT_ARCH_* of syscall */
};

#define _ALIGN(x,sz) (((x + ((sz)-1)) & ~((sz)-1)) - (sz))
#define ALIGN(x,sz) ((typeof(x))_ALIGN((unsigned long)(x),(unsigned long)(sz)))
static long local_mprotect(void *target, unsigned long sz)
{
	register unsigned long res asm ("rax") = __NR_mprotect;
	__attribute__((unused)) register void *addr asm ("rdi") = ALIGN(target, sz);
	__attribute__((unused)) register long len asm ("rsi") = sz;
	__attribute__((unused)) register long num asm ("rdx") = PROT_NONE;
	__asm__("syscall\n");
	return res;
}

static void TRAP_action(int nr, siginfo_t *info, void *void_context)
{
	ucontext_t *ctx = (ucontext_t *)void_context;
	char buf[256];
	int len;
	struct arch_sigsys *sys = (struct arch_sigsys *)
#ifdef si_syscall
		&(info->si_call_addr);
#else
		&(info->si_pid);
#endif

	if (info->si_code != SYS_SECCOMP)
		return;
	if (!ctx)
		return;
	len = snprintf(buf, sizeof(buf),
			"@0x%lX:%X:%d:0x%lX:0x%lX:0x%lX:0x%lX:0x%lX:0x%lX [0x%lX]\n",
			(unsigned long)sys->_call_addr,
			sys->_arch,
			sys->_syscall,
			(unsigned long)ctx->uc_mcontext.gregs[REG_ARG0],
			(unsigned long)ctx->uc_mcontext.gregs[REG_ARG1],
			(unsigned long)ctx->uc_mcontext.gregs[REG_ARG2],
			(unsigned long)ctx->uc_mcontext.gregs[REG_ARG3],
			(unsigned long)ctx->uc_mcontext.gregs[REG_ARG4],
			(unsigned long)ctx->uc_mcontext.gregs[REG_ARG5],
			(unsigned long)ALIGN(ctx->uc_mcontext.gregs[REG_IP],
			4096));
	/* Emit some useful logs or whatever. */
	syscall(__NR_write, STDOUT_FILENO, buf, len);
	/* Make the calling page non-exec */
	/* Careful on how it is called since it may make the syscall() instructions non-exec. */
	local_mprotect((void *)ctx->uc_mcontext.gregs[REG_IP], sysconf(_SC_PAGE_SIZE));
}

TEST_F_SIGNAL(TRAP, sigsegv, SIGSEGV) {
	int ret;
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

	/* Call anything! */
	ret = syscall(__NR_getpid);
}

TEST_HARNESS_MAIN
