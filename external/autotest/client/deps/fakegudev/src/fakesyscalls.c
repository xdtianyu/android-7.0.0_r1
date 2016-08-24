/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _GNU_SOURCE /* for RTLD_NEXT in dlfcn.h */

#include <glib.h>

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/* The purpose of this library is to override the open/creat syscalls to
 * redirect these calls for selected devices. Adding the library file to
 * LD_PRELOAD is the general way to accomplish this. The arbitrary file mapping
 * is specified in the environment variable FILE_REDIRECTION_PRELOADS as
 * follows:
 *
 * FILE_REDIRECTIONS_PRELOAD=<path1>=<target1>:<path2>=<target2>
 *
 * Here, <path1> etc are the absolute paths to files for which open/close should
 * be intercepted. <target1> etc are the alternative files to which these calls
 * should be redirected.
 *
 *  - ':' is used to separate file mappings
 *  - The special character ':' in the paths should be escaped with '\'
 *
 *  Example:
 *    export FILE_REDIRECTIONS_PRELOAD=/tmp/file1=/tmp/file2
 *    LD_PRELOAD=./libfakesyscalls.so ./write_to_tmp_file1
 *
 *  where write_to_tmp_file1 is some executable that opens and writes to
 *  /tmp/file1. When the program exits, /tmp/file2 would have been created and
 *  written to, not /tmp/file1.
 *
 *  cf: fakesyscalls-exercise.c
 *
 *  Thread safety: This library is not thread-safe. If two threads
 *  simultaneously call open/creat for the first time, internal data-structures
 *  in the library can be corrupted.
 *  It is safe to have subsequent calls to open/creat be concurrent.
 */

#ifdef FAKE_SYSCALLS_DEBUG
static const char *k_tmp_logging_file_full_path = "/tmp/fake_syscalls.dbg";
static FILE *debug_file;

#define fake_syscalls_debug_init() \
  debug_file = fopen (k_tmp_logging_file_full_path, "w")

#define fake_syscalls_debug(...) \
  do { \
    if (debug_file) { \
      fprintf (debug_file, __VA_ARGS__); \
      fprintf (debug_file, "\n"); \
    } \
  } while (0)

#define fake_syscalls_debug_finish() \
  do { \
    if (debug_file) { \
      fclose (debug_file); \
      debug_file = NULL; \
    } \
  } while (0)

#else /* FAKE_SYSCALLS_DEBUG */
#define fake_syscalls_debug_init()
#define fake_syscalls_debug(...)
#define fake_syscalls_debug_finish()
#endif  /* FAKE_SYSCALLS_DEBUG */

static GHashTable *file_redirection_map;

static const char *k_env_file_redirections = "FILE_REDIRECTIONS_PRELOAD";
static const char *k_func_open = "open";
static const char *k_func_creat = "creat";

void __attribute__ ((constructor))
fake_syscalls_init (void)
{
  fake_syscalls_debug_init ();
  fake_syscalls_debug ("Initialized fakesyscalls library.");
}

void __attribute__ ((destructor))
fake_syscalls_finish (void)
{
  if (file_redirection_map)
    g_hash_table_unref (file_redirection_map);
  fake_syscalls_debug ("Quit fakesyscalls library.");
  fake_syscalls_debug_finish ();
}

static void
abort_on_error (GError *error) {
  if (!error)
    return;

  fake_syscalls_debug ("Aborting on error: |%s|", error->message);
  g_error_free (error);
  fake_syscalls_debug_finish ();
  g_assert (0);
}

static void
setup_redirection_map (void)
{
  const char *orig_env;
  GRegex *entry_delimiter, *key_value_delimiter, *escaped_colon;
  gchar *buf;
  gchar **redirections;
  gchar **redirections_iter;
  GError *error = NULL;

  file_redirection_map = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                                g_free);

  orig_env = getenv (k_env_file_redirections);
  if (orig_env == NULL)
    orig_env = "";
  fake_syscalls_debug ("FILE_REDIRECTIONS_PRELOAD=|%s|", orig_env);

  entry_delimiter = g_regex_new ("(?:([^\\\\]):)|(?:^:)", 0, 0, &error);
  abort_on_error (error);
  key_value_delimiter = g_regex_new ("=", 0, 0, &error);
  abort_on_error (error);
  escaped_colon = g_regex_new ("(?:[^\\\\]\\\\:)|(?:^\\\\:)", 0, 0, &error);
  abort_on_error (error);

  buf = g_regex_replace (entry_delimiter, orig_env, -1, 0, "\\1;", 0, &error);
  abort_on_error (error);
  redirections = g_strsplit (buf, ";", 0);
  g_free (buf);

  for (redirections_iter = redirections;
       *redirections_iter;
       ++redirections_iter) {
    gchar **parts;

    if (g_strcmp0 ("", *redirections_iter) == 0)
      continue;

    /* Any ':' in the map has to be escaped with a '\' to allow for ':' to act
     * as delimiter. Clean away the '\'.
     */
    buf = g_regex_replace_literal (escaped_colon, *redirections_iter, -1, 0,
                                   ":", 0, &error);
    abort_on_error (error);
    parts = g_regex_split (key_value_delimiter, buf, 0);
    g_free (buf);

    if (g_strv_length (parts) != 2) {
      fake_syscalls_debug ("Error parsing redirection: |%s|. Malformed map?",
                           *redirections_iter);
      g_strfreev (parts);
      continue;
    }
    if (strlen (parts[0]) == 0 || parts[0][0] != '/' ||
        strlen (parts[1]) == 0 || parts[1][0] != '/') {
      fake_syscalls_debug ("Error parsing redirection: |%s|."
                           "Invalid absolute paths.",
                           *redirections_iter);
      g_strfreev (parts);
      continue;
    }

    fake_syscalls_debug ("Inserted redirection: |%s|->|%s|",
                         parts[0], parts[1]);
    g_hash_table_insert (file_redirection_map,
                         g_strdup (parts[0]), g_strdup (parts[1]));
    g_strfreev (parts);
  }

  g_regex_unref (entry_delimiter);
  g_regex_unref (key_value_delimiter);
  g_regex_unref (escaped_colon);
  g_strfreev (redirections);
}

int
open (const char *pathname, int flags, ...)
{
  static int(*realfunc)(const char *, int, ...);
  const char *redirection;
  va_list ap;
  gboolean is_creat = FALSE;
  mode_t mode = S_IRUSR;  /* Make compiler happy. Remain restrictive. */

  if (file_redirection_map == NULL)
    setup_redirection_map ();

  redirection = (char *) g_hash_table_lookup (file_redirection_map, pathname);
  if (redirection == NULL)
    redirection = pathname;

  if (realfunc == NULL)
    realfunc = (int(*)(const char *, int, ...))dlsym (RTLD_NEXT, k_func_open);

  is_creat = flags & O_CREAT;

  if (is_creat) {
    va_start (ap, flags);
    mode = va_arg (ap, mode_t);
    va_end (ap);
    fake_syscalls_debug (
        "Redirect: open (%s, %d, %d) --> open (%s, %d, %d)",
        pathname, flags, mode, redirection, flags, mode);
    return realfunc (redirection, flags, mode);
  } else {
    fake_syscalls_debug (
        "Redirect: open (%s, %d) --> open (%s, %d)",
        pathname, flags, redirection, flags);
    return realfunc (redirection, flags);
  }
}

int
creat (const char *pathname, mode_t mode)
{
  static int(*realfunc)(const char *, mode_t);
  const char *redirection;

  if (file_redirection_map == NULL)
    setup_redirection_map ();

  redirection = (char *) g_hash_table_lookup (file_redirection_map, pathname);
  if (redirection == NULL)
    redirection = pathname;
  fake_syscalls_debug (
      "Redirect: creat (%s, %d) --> creat (%s, %d)",
      pathname, mode, redirection, mode);

  if (realfunc == NULL)
    realfunc = (int(*)(const char *, mode_t))dlsym (RTLD_NEXT, k_func_creat);

  return realfunc (redirection, mode);
}
