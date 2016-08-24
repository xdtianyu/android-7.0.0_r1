/*
 * Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

int main(void)
{
	char buf[128];
	int fd, ret;
	unsigned int i;

	fd = open("/dev/zero", O_RDONLY);
	if (fd < 0)
		return 1;

	ret = read(fd, buf, sizeof(buf));
	if (ret < 0)
		return 2;

	for (i = 0; i < (sizeof(buf) / sizeof(buf[0])); i++) {
		if (buf[i] != 0)
			return 3;
	}

	ret = close(fd);
	if (ret < 0)
		return 4;

	return 0;
}
