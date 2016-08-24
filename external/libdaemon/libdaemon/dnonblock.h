#ifndef foodaemonnonblockhfoo
#define foodaemonnonblockhfoo

/***
  This file is part of libdaemon.

  Copyright 2003-2008 Lennart Poettering

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

***/

#ifdef __cplusplus
extern "C" {
#endif

/** \file
 *
 * Contains a single function used to change a file descriptor to
 * non-blocking mode using fcntl().
 */

/** Change the passed file descriptor to non-blocking or blocking
 * mode, depending on b.
 * @param fd The file descriptor to manipulation
 * @param b TRUE if non-blocking mode should be enabled, FALSE if it
 * should be disabled
 * @return Zero on success, nonzero on failure.
 */
int daemon_nonblock(int fd, int b);

#ifdef __cplusplus
}
#endif

#endif
