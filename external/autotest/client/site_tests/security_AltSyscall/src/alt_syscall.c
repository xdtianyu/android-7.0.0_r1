/*
 * Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <unistd.h>

#include <sys/prctl.h>

#ifndef PR_ALT_SYSCALL
#define PR_ALT_SYSCALL 0x43724f53
#define PR_ALT_SYSCALL_SET_SYSCALL_TABLE 1
#endif

int main(void)
{
	int ret;

	ret = prctl(PR_ALT_SYSCALL, PR_ALT_SYSCALL_SET_SYSCALL_TABLE,
		    "read_write_test");
	if (ret < 0)
		return 1;

	return 0;
}
