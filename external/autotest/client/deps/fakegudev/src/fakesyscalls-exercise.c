/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int
main (int argc, const char *argv[])
{
  int f;
  const char *stuff = "stuff";
  const int stuff_len = strlen(stuff) + 1;
  char read_back[10];
  int retval = 0;

  if (argc != 3) {
    fprintf (stderr, "Usage: %s <file_name> <redirected_file>\n", argv[0]);
    return 1;
  }

  f = open (argv[1], O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IROTH);
  if (f == -1) {
    fprintf (stderr, "Inconclusive: Could not open file to write.\n");
    return 1;
  }
  if (write (f, stuff, stuff_len) < stuff_len) {
    fprintf (stderr, "Inconclusive: Could not write to the file.\n");
    return 1;
  }

  if (close (f) != 0) {
    fprintf (stderr, "Inconclusive: Error closing write file.\n");
    return 1;
  }

  f = open (argv[2], O_RDONLY);
  if (f == -1) {
    retval = 1;
    fprintf (stderr, "Failed. Couldn't open file to read.\n");
  } else if (read (f, read_back, stuff_len) != stuff_len) {
    retval = 1;
    fprintf (stderr, "Failed. Couldn't read back data.\n");
  } else if (strncmp (stuff, read_back, stuff_len) != 0) {
    retval = 1;
    fprintf (stderr, "Failed. The read back string does not match the orignial."
                     " Original: |%s|, Read back: |%s|\n",
                     stuff, read_back);
  } else {
    fprintf (stdout, "Success. Woohoo!\n");
  }

  if (f != -1)
    close (f);

  return retval;
}
