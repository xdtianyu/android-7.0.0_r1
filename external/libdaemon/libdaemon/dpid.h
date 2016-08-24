#ifndef foodaemonpidhfoo
#define foodaemonpidhfoo

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

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \file
 *
 * Contains an API for manipulating PID files.
 */

/** Prototype of a function for generating the name of a PID file.
 */
typedef const char* (*daemon_pid_file_proc_t)(void);

/** Identification string for the PID file name, only used when
 * daemon_pid_file_proc is set to daemon_pid_file_proc_default(). Use
 * daemon_ident_from_argv0() to generate an identification string from
 * argv[0]
 */
extern const char *daemon_pid_file_ident;

/** A function pointer which is used to generate the name of the PID
 * file to manipulate. Points to daemon_pid_file_proc_default() by
 * default.
 */
extern daemon_pid_file_proc_t daemon_pid_file_proc;

/** A function for creating a pid file name from
 * daemon_pid_file_ident
 * @return The PID file path
 */
const char *daemon_pid_file_proc_default(void);

/** Creates PID pid file for the current process
 * @return zero on success, nonzero on failure
 */
int daemon_pid_file_create(void);

/** Removes the PID file of the current process
 * @return zero on success, nonzero on failure
 */
int daemon_pid_file_remove(void);

/** Returns the PID file of a running daemon, if available
 * @return The PID or negative on failure
 */
pid_t daemon_pid_file_is_running(void);

/** Kills a running daemon, if available
 * @param s The signal to send
 * @return zero on success, nonzero on failure
 */
int daemon_pid_file_kill(int s);

/** This variable is defined to 1 iff daemon_pid_file_kill_wait() is supported.
 * @since 0.3
 * @see daemon_pid_file_kill_wait() */
#define DAEMON_PID_FILE_KILL_WAIT_AVAILABLE 1

/** Similar to daemon_pid_file_kill() but waits until the process
 * died.  This functions is new in libdaemon 0.3. The macro
 * DAEMON_PID_FILE_KILL_WAIT_AVAILABLE is defined iff libdaemon
 * supports this function.
 *
 * @param s The signal to send
 * @param m Seconds to wait at maximum
 * @return zero on success, nonzero on failure (timeout condition is considered a failure)
 * @since 0.3
 * @see DAEMON_PID_FILE_KILL_WAIT_AVAILABLE
 */
int daemon_pid_file_kill_wait(int s, int m);

#ifdef __cplusplus
}
#endif

#endif
