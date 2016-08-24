/* Integration tests for the dbus-daemon
 *
 * Author: Simon McVittie <simon.mcvittie@collabora.co.uk>
 * Copyright © 2010-2011 Nokia Corporation
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <config.h>

#include <glib.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <string.h>

#ifdef DBUS_WIN
# include <io.h>
# include <windows.h>
#else
# include <signal.h>
# include <unistd.h>
#endif

typedef struct {
    gboolean skip;

    DBusError e;
    GError *ge;

    GPid daemon_pid;

    DBusConnection *left_conn;

    DBusConnection *right_conn;
    gboolean right_conn_echo;
} Fixture;

#define assert_no_error(e) _assert_no_error (e, __FILE__, __LINE__)
static void
_assert_no_error (const DBusError *e,
    const char *file,
    int line)
{
  if (G_UNLIKELY (dbus_error_is_set (e)))
    g_error ("%s:%d: expected success but got error: %s: %s",
        file, line, e->name, e->message);
}

static gchar *
spawn_dbus_daemon (gchar *binary,
    gchar *configuration,
    GPid *daemon_pid)
{
  GError *error = NULL;
  GString *address;
  gint address_fd;
  gchar *argv[] = {
      binary,
      configuration,
      "--nofork",
      "--print-address=1", /* stdout */
      NULL
  };

  g_spawn_async_with_pipes (NULL, /* working directory */
      argv,
      NULL, /* envp */
      G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
      NULL, /* child_setup */
      NULL, /* user data */
      daemon_pid,
      NULL, /* child's stdin = /dev/null */
      &address_fd,
      NULL, /* child's stderr = our stderr */
      &error);
  g_assert_no_error (error);

  address = g_string_new (NULL);

  /* polling until the dbus-daemon writes out its address is a bit stupid,
   * but at least it's simple, unlike dbus-launch... in principle we could
   * use select() here, but life's too short */
  while (1)
    {
      gssize bytes;
      gchar buf[4096];
      gchar *newline;

      bytes = read (address_fd, buf, sizeof (buf));

      if (bytes > 0)
        g_string_append_len (address, buf, bytes);

      newline = strchr (address->str, '\n');

      if (newline != NULL)
        {
          g_string_truncate (address, newline - address->str);
          break;
        }

      g_usleep (G_USEC_PER_SEC / 10);
    }

  return g_string_free (address, FALSE);
}

static DBusConnection *
connect_to_bus (const gchar *address)
{
  DBusConnection *conn;
  DBusError error = DBUS_ERROR_INIT;
  dbus_bool_t ok;

  conn = dbus_connection_open_private (address, &error);
  assert_no_error (&error);
  g_assert (conn != NULL);

  ok = dbus_bus_register (conn, &error);
  assert_no_error (&error);
  g_assert (ok);
  g_assert (dbus_bus_get_unique_name (conn) != NULL);

  dbus_connection_setup_with_g_main (conn, NULL);
  return conn;
}

static DBusHandlerResult
echo_filter (DBusConnection *connection,
    DBusMessage *message,
    void *user_data)
{
  DBusMessage *reply;

  if (dbus_message_get_type (message) != DBUS_MESSAGE_TYPE_METHOD_CALL)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  reply = dbus_message_new_method_return (message);

  if (reply == NULL)
    g_error ("OOM");

  if (!dbus_connection_send (connection, reply, NULL))
    g_error ("OOM");

  dbus_message_unref (reply);

  return DBUS_HANDLER_RESULT_HANDLED;
}

typedef struct {
    const char *bug_ref;
    guint min_messages;
    const char *config_file;
} Config;

static void
setup (Fixture *f,
    gconstpointer context)
{
  const Config *config = context;
  gchar *dbus_daemon;
  gchar *arg;
  gchar *address;

  f->ge = NULL;
  dbus_error_init (&f->e);

  if (config != NULL && config->config_file != NULL)
    {
      if (g_getenv ("DBUS_TEST_DATA") == NULL)
        {
          g_message ("SKIP: set DBUS_TEST_DATA to a directory containing %s",
              config->config_file);
          f->skip = TRUE;
          return;
        }

      arg = g_strdup_printf (
          "--config-file=%s/%s",
          g_getenv ("DBUS_TEST_DATA"), config->config_file);
    }
  else if (g_getenv ("DBUS_TEST_SYSCONFDIR") != NULL)
    {
      arg = g_strdup_printf ("--config-file=%s/dbus-1/session.conf",
          g_getenv ("DBUS_TEST_SYSCONFDIR"));
    }
  else if (g_getenv ("DBUS_TEST_DATA") != NULL)
    {
      arg = g_strdup_printf (
          "--config-file=%s/valid-config-files/session.conf",
          g_getenv ("DBUS_TEST_DATA"));
    }
  else
    {
      arg = g_strdup ("--session");
    }

  dbus_daemon = g_strdup (g_getenv ("DBUS_TEST_DAEMON"));

  if (dbus_daemon == NULL)
    dbus_daemon = g_strdup ("dbus-daemon");

  address = spawn_dbus_daemon (dbus_daemon, arg, &f->daemon_pid);

  g_free (dbus_daemon);
  g_free (arg);

  f->left_conn = connect_to_bus (address);
  f->right_conn = connect_to_bus (address);
  g_free (address);
}

static void
add_echo_filter (Fixture *f)
{
  if (!dbus_connection_add_filter (f->right_conn, echo_filter, NULL, NULL))
    g_error ("OOM");

  f->right_conn_echo = TRUE;
}

static void
pc_count (DBusPendingCall *pc,
    void *data)
{
  guint *received_p = data;

  (*received_p)++;
}

static void
test_echo (Fixture *f,
    gconstpointer context)
{
  const Config *config = context;
  guint count = 2000;
  guint sent;
  guint received = 0;
  double elapsed;

  if (f->skip)
    return;

  if (config != NULL && config->bug_ref != NULL)
    g_test_bug (config->bug_ref);

  if (g_test_perf ())
    count = 100000;

  if (config != NULL)
    count = MAX (config->min_messages, count);

  add_echo_filter (f);

  g_test_timer_start ();

  for (sent = 0; sent < count; sent++)
    {
      DBusMessage *m = dbus_message_new_method_call (
          dbus_bus_get_unique_name (f->right_conn), "/",
          "com.example", "Spam");
      DBusPendingCall *pc;

      if (m == NULL)
        g_error ("OOM");

      if (!dbus_connection_send_with_reply (f->left_conn, m, &pc,
                                            DBUS_TIMEOUT_INFINITE) ||
          pc == NULL)
        g_error ("OOM");

      if (dbus_pending_call_get_completed (pc))
        pc_count (pc, &received);
      else if (!dbus_pending_call_set_notify (pc, pc_count, &received,
            NULL))
        g_error ("OOM");

      dbus_pending_call_unref (pc);
      dbus_message_unref (m);
    }

  while (received < count)
    g_main_context_iteration (NULL, TRUE);

  elapsed = g_test_timer_elapsed ();

  g_test_maximized_result (count / elapsed, "%u messages / %f seconds",
      count, elapsed);
}

static void
teardown (Fixture *f,
    gconstpointer context G_GNUC_UNUSED)
{
  dbus_error_free (&f->e);
  g_clear_error (&f->ge);

  if (f->left_conn != NULL)
    {
      dbus_connection_close (f->left_conn);
      dbus_connection_unref (f->left_conn);
      f->left_conn = NULL;
    }

  if (f->right_conn != NULL)
    {
      if (f->right_conn_echo)
        {
          dbus_connection_remove_filter (f->right_conn, echo_filter, NULL);
          f->right_conn_echo = FALSE;
        }

      dbus_connection_close (f->right_conn);
      dbus_connection_unref (f->right_conn);
      f->right_conn = NULL;
    }

  if (f->daemon_pid != 0)
    {
#ifdef DBUS_WIN
      TerminateProcess (f->daemon_pid, 1);
#else
      kill (f->daemon_pid, SIGTERM);
#endif

      g_spawn_close_pid (f->daemon_pid);
      f->daemon_pid = 0;
    }
}

static Config limited_config = {
    "34393", 10000, "valid-config-files/incoming-limit.conf"
};

int
main (int argc,
    char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_bug_base ("https://bugs.freedesktop.org/show_bug.cgi?id=");

  g_test_add ("/echo/session", Fixture, NULL, setup, test_echo, teardown);
  g_test_add ("/echo/limited", Fixture, &limited_config,
      setup, test_echo, teardown);

  return g_test_run ();
}
