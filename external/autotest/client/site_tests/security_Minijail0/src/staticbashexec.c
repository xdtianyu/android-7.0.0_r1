/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	return execv("/bin/bash", argv);
}
