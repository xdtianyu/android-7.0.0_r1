#ifndef foodaemonsignalhfoo
#define foodaemonsignalhfoo

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
 * Contains the API for serializing signals to a pipe for
 * usage with select() or poll().
 *
 * You should register all signals you
 * wish to handle with select() in your main loop with
 * daemon_signal_init() or daemon_signal_install(). After that you
 * should sleep on the file descriptor returned by daemon_signal_fd()
 * and get the next signal recieved with daemon_signal_next(). You
 * should call daemon_signal_done() before exiting.
 */

/** Installs signal handlers for the specified signals
 * @param s, ... The signals to install handlers for. The list should be terminated by 0
 * @return zero on success, nonzero on failure
 */
int daemon_signal_init(int s, ...);

/** Install a  signal handler for the specified signal
 * @param s The signalto install handler for
 * @return zero onsuccess,nonzero on failure
 */
int daemon_signal_install(int s);

/** Free resources of signal handling, should be called before daemon exit
 */
void daemon_signal_done(void);

/** Return the next signal recieved. This function will not
 * block. Instead it returns 0 if no signal is queued.
 * @return The next queued signal if one is queued, zero if none is
 * queued, negative on failure.
 */
int daemon_signal_next(void);

/** Return the file descriptor the daemon should select() on for
 * reading. Whenever the descriptor is ready you should call
 * daemon_signal_next() to get the next signal queued.
 * @return The file descriptor or negative on failure
 */
int daemon_signal_fd(void);

#ifdef __cplusplus
}
#endif

#endif
