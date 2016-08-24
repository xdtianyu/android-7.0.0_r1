/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _POSIX_C_SOURCE 201108L
#define _XOPEN_SOURCE 600

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <glib.h>
#include <dbus/dbus-glib.h>

GIOChannel* ioc;
int masterfd;

typedef struct {
  GRegex *command;
  char *reply; // generic text
  char *responsetext; // ERROR, +CMS ERROR, etc.
} Pattern;

typedef struct _FakeModem {
  GObject parent;
  gboolean echo;
  gboolean verbose;
  GPtrArray *patterns;
} FakeModem;

typedef struct _FakeModemClass
{
  GObjectClass parent_class;
} FakeModemClass;

GType fakemodem_get_type (void) G_GNUC_CONST;

#define FAKEMODEM_TYPE         (fake_modem_get_type ())
#define FAKEMODEM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FAKEMODEM_TYPE, FakeModem))

G_DEFINE_TYPE (FakeModem, fake_modem, G_TYPE_OBJECT)

static void
fake_modem_init (FakeModem* self)
{

  self->echo = TRUE;
  self->verbose = TRUE;
  self->patterns = NULL;
}

static void
fake_modem_class_init (FakeModemClass* self)
{
}

static gboolean master_read (GIOChannel *source, GIOCondition condition,
                             gpointer data);

static const gchar *handle_cmd (FakeModem *fakemodem, const gchar *cmd);

static gboolean send_unsolicited (FakeModem* fakemodem, const gchar* text);
static gboolean set_response (FakeModem* fakemodem, const gchar* command,
                              const gchar* reply, const gchar* response);
static gboolean remove_response (FakeModem* fakemodem, const gchar* command);

#include "fakemodem-dbus.h"

GPtrArray *
parse_pattern_files(char **pattern_files, GError **error)
{
  gint linenum;
  GRegex *skip, *parts;
  GPtrArray *patterns;
  int i;

  patterns = g_ptr_array_new();

  skip = g_regex_new ("^\\s*(#.*)?$", 0, 0, error);
  if (skip == NULL)
    return NULL;
  parts = g_regex_new ("^(\\S+)\\s*(\"([^\"]*)\")?\\s*(.*)$", 0, 0, error);
  if (parts == NULL)
    return NULL;

  for (i = 0 ; pattern_files[i] != NULL; i++) {
    GIOChannel *pf;
    gchar *pattern_file;
    gchar *line;
    gsize len, term;

    pattern_file = pattern_files[i];

    pf = g_io_channel_new_file (pattern_file, "r", error);
    if (pf == NULL)
      return NULL;

    linenum = 0;
    while (g_io_channel_read_line (pf, &line, &len, &term, error) ==
           G_IO_STATUS_NORMAL) {
      /* Don't need the terminator */
      line[term] = '\0';
      linenum++;

      if (!g_regex_match (skip, line, 0, NULL)) {
        GMatchInfo *info;
        gboolean ret;
        gchar *command, *responsetext;
        ret = g_regex_match (parts, line, 0, &info);
        if (ret) {
          Pattern *pat;
          pat = g_malloc (sizeof (*pat));
          command = g_match_info_fetch (info, 1);
          pat->command = g_regex_new (command,
                                      G_REGEX_ANCHORED |
                                      G_REGEX_CASELESS |
                                      G_REGEX_RAW |
                                      G_REGEX_OPTIMIZE,
                                      0,
                                      error);
          g_free (command);
          if (pat->command == NULL) {
            printf ("error: %s\n", (*error)->message);
            g_error_free (*error);
            *error = NULL;
          }
          responsetext = g_match_info_fetch (info, 3);
          if (strlen (responsetext) == 0) {
            g_free (responsetext);
            responsetext = NULL;
          }
          pat->responsetext = responsetext;
          pat->reply = g_match_info_fetch (info, 4);
          while (pat->reply[strlen (pat->reply) - 1] == '\\') {
            gchar *origstr;
            pat->reply[strlen (pat->reply) - 1] = '\0';
            g_free (line); /* probably invalidates fields in 'info' */
            g_io_channel_read_line (pf, &line, &len, &term, error);
            line[term] = '\0';
            linenum++;
            origstr = pat->reply;
            pat->reply = g_strjoin ("\r\n", origstr, line, NULL);
            g_free (origstr);
          }
          g_ptr_array_add (patterns, pat);
        } else {
          printf (" Line %d '%s' was not parsed"
                  " as a command-response pattern\n",
                  linenum, line);
        }
        g_match_info_free (info);
      }
      g_free (line);
    }
    g_io_channel_shutdown (pf, TRUE, NULL);
  }

  g_regex_unref (skip);
  g_regex_unref (parts);

  return patterns;
}

#define FM_DBUS_SERVICE "org.chromium.FakeModem"

static DBusGProxy *
create_dbus_proxy (DBusGConnection *bus)
{
    DBusGProxy *proxy;
    GError *err = NULL;
    int request_name_result;

    proxy = dbus_g_proxy_new_for_name (bus,
                                       "org.freedesktop.DBus",
                                       "/org/freedesktop/DBus",
                                       "org.freedesktop.DBus");

    if (!dbus_g_proxy_call (proxy, "RequestName", &err,
                            G_TYPE_STRING, FM_DBUS_SERVICE,
                            G_TYPE_UINT, DBUS_NAME_FLAG_DO_NOT_QUEUE,
                            G_TYPE_INVALID,
                            G_TYPE_UINT, &request_name_result,
                            G_TYPE_INVALID)) {
        g_print ("Could not acquire the %s service.\n"
                 "  Message: '%s'\n", FM_DBUS_SERVICE, err->message);

        g_error_free (err);
        g_object_unref (proxy);
        proxy = NULL;
    } else if (request_name_result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        g_print ("Could not acquire the " FM_DBUS_SERVICE
                 " service as it is already taken. Return: %d\n",
                 request_name_result);

        g_object_unref (proxy);
        proxy = NULL;
    } else {
        dbus_g_proxy_add_signal (proxy, "NameOwnerChanged",
                                 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                 G_TYPE_INVALID);
    }

    return proxy;
}

int
main (int argc, char *argv[])
{
  DBusGConnection *bus;
  DBusGProxy *proxy;
  GMainLoop* loop;
  const char *slavedevice;
  struct termios t;
  FakeModem *fakemodem;
  GOptionContext *opt_ctx;
  char **pattern_files = NULL;
  gboolean session = FALSE;
  GError *err = NULL;

  GOptionEntry entries[] = {
    { "patternfile", 0, 0, G_OPTION_ARG_STRING_ARRAY, &pattern_files,
      "Path to pattern file", NULL},
    { "session", 0, 0, G_OPTION_ARG_NONE, &session,
      "Bind to session bus", NULL},
    { "system", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &session,
      "Bind to system bus (default)", NULL},
    { NULL }
  };

  #if !GLIB_CHECK_VERSION(2,35,0)
  g_type_init ();
  #endif

  opt_ctx = g_option_context_new (NULL);
  g_option_context_set_summary (opt_ctx,
                                "Emulate a modem with a set of "
                                "regexp-programmed responses.");
  g_option_context_add_main_entries (opt_ctx, entries, NULL);
  if (!g_option_context_parse (opt_ctx, &argc, &argv, &err)) {
    g_warning ("%s\n", err->message);
    g_error_free (err);
    exit (1);
  }

  g_option_context_free (opt_ctx);

  fakemodem = g_object_new (FAKEMODEM_TYPE, NULL);
  if (pattern_files) {
    fakemodem->patterns = parse_pattern_files (pattern_files, &err);
    if (fakemodem->patterns == NULL) {
      g_warning ("%s\n", err->message);
      g_error_free (err);
      exit (1);
    }
  } else
    fakemodem->patterns = g_ptr_array_sized_new (0);

  loop = g_main_loop_new (NULL, FALSE);

  dbus_g_object_type_install_info (FAKEMODEM_TYPE,
                                   &dbus_glib_fakemodem_object_info);

  err = NULL;
  if (session)
    bus = dbus_g_bus_get (DBUS_BUS_SESSION, &err);
  else
    bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);

  if (bus == NULL) {
      g_warning ("%s\n", err->message);
      g_error_free (err);
      exit (1);
  }

  proxy = create_dbus_proxy (bus);
  if (!proxy)
    exit (1);

  dbus_g_connection_register_g_object (bus,
                                       "/",
                                       G_OBJECT (fakemodem));

  masterfd = posix_openpt (O_RDWR | O_NOCTTY);

  if (masterfd == -1
      || grantpt (masterfd) == -1
      || unlockpt (masterfd) == -1
      || (slavedevice = ptsname (masterfd)) == NULL)
    exit (1);

  printf ("%s\n", slavedevice);
  fflush (stdout);

  /* Echo is actively harmful here */
  tcgetattr (masterfd, &t);
  t.c_lflag &= ~ECHO;
  tcsetattr (masterfd, TCSANOW, &t);

  ioc = g_io_channel_unix_new (masterfd);
  g_io_channel_set_encoding (ioc, NULL, NULL);
  g_io_channel_set_line_term (ioc, "\r", 1);
  g_io_add_watch (ioc, G_IO_IN, master_read, fakemodem);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);

  g_object_unref (fakemodem);
  return 0;
}


/*
 * &?[A-CE-RT-Z][0-9]*
 * S[0-9]+?
 * S[0-9]+=(([0-9A-F]+|"[^"]*")?,)+
 */

/*
 * action +[A-Z][A-Z0-9%-./:_]{0,15}
 * test   +[A-Z][A-Z0-9%-./:_]{0,15}=?
 * get    +[A-Z][A-Z0-9%-./:_]{0,15}?
 * set    +[A-Z][A-Z0-9%-./:_]{0,15}=(([0-9A-F]+|"[^"]*")?,)+
 */


#define VALUE "([0-9A-F]+|\"[^\"]*\")"
#define CVALUE VALUE "?(," VALUE "?)*"
static char *command_patterns[] =
{"\\s*(&?[A-CE-RT-Z][0-9]*)",
 "\\s*(S[0-9]+\\?)",
 "\\s*(S[0-9]+=" CVALUE ")",
 /* ATD... (dial string) handling is missing */
 "\\s*;?\\s*([+*%&][A-Z][A-Z0-9%-./:_]{0,15}=\\?)",
 "\\s*;?\\s*([+*%&][A-Z][A-Z0-9%-./:_]{0,15}=" CVALUE ")",
 "\\s*;?\\s*([+*%&][A-Z][A-Z0-9%-./:_]{0,15}(\\?)?)",
};

#undef VALUE
#undef CVALUE

static gboolean master_read (GIOChannel *source, GIOCondition condition,
                             gpointer data)
{
  FakeModem *fakemodem = data;
  gchar *line, *next;
  const gchar *response;
  gsize term;
  GError *error = NULL;
  GIOStatus status;
  int i, rval;

  static GPtrArray *commands;

  if (commands == NULL) {
    int n;
    n = sizeof (command_patterns) / sizeof (command_patterns[0]);
    commands = g_ptr_array_sized_new (n);
    for (i = 0 ; i < n ; i++) {
      GRegex *re = g_regex_new (command_patterns[i],
                                G_REGEX_CASELESS |
                                G_REGEX_ANCHORED |
                                G_REGEX_RAW |
                                G_REGEX_OPTIMIZE,
                                0,
                                &error);
      if (re == NULL) {
        g_warning ("Couldn't generate command regex: %s\n", error->message);
        g_error_free (error);
        exit (1);
      }
      g_ptr_array_add (commands, re);
    }
  }

  status = g_io_channel_read_line (source, &line, NULL, &term, &error);
  if (status == G_IO_STATUS_ERROR)
    return FALSE;
  line[term] = '\0';

  printf ("Line: '%s'\n", line);

  if (fakemodem->echo) {
    rval = write (masterfd, line, term);
    assert(term == rval);
    rval = write (masterfd, "\r\n", 2);
    assert(2 == rval);
  }

  if (g_ascii_strncasecmp (line, "AT", 2) != 0) {
    if (line[0] == '\0')
      goto out;
    response = "ERROR";
    goto done;
  }

  response = NULL;
  next = line + 2;

  while (!response && *next) {
    for (i = 0 ; i < commands->len; i++) {
      GMatchInfo *info;
      if (g_regex_match (g_ptr_array_index (commands, i), next, 0, &info)) {
        gint start, end;
        gchar *cmd;
        g_match_info_fetch_pos (info, 1, &start, &end);
        cmd = g_strndup (next + start, end - start);
        response = handle_cmd (fakemodem, cmd);
        g_free (cmd);
        g_match_info_free (info);
        next += end;
        break;
      }
      g_match_info_free (info);
    }
    if (i == commands->len) {
      response = "ERROR";
      break;
    }
  }


done:
  if (fakemodem->verbose) {
    gchar *rstr;
    if (response == NULL)
      response = "OK";
    rstr = g_strdup_printf("\r\n%s\r\n", response);
    rval = write (masterfd, rstr, strlen (rstr));
    assert(strlen(rstr) == rval);
    g_free (rstr);
  } else {
    gchar *rstr;
    rstr = g_strdup_printf("%s\n", response);
    rval = write (masterfd, rstr, strlen (rstr));
    assert(strlen(rstr) == rval);
    g_free (rstr);
  }

out:
  g_free (line);
  return TRUE;
}

static const gchar *
handle_cmd(FakeModem *fakemodem, const gchar *cmd)
{
  guint i;
  Pattern *pat = NULL;

  printf (" Cmd:  '%s'\n", cmd);

  if (toupper (cmd[0]) >= 'A' && toupper (cmd[0]) <= 'Z') {
    switch (toupper (cmd[0])) {
      case 'E':
        if (cmd[1] == '0')
          fakemodem->echo = FALSE;
        else if (cmd[1] == '1')
          fakemodem->echo = TRUE;
        else
          return "ERROR";
        return "OK";
      case 'V':
        if (cmd[1] == '0')
          fakemodem->verbose = FALSE;
        else if (cmd[1] == '1')
          fakemodem->verbose = TRUE;
        else
          return "ERROR";
        return "OK";
      case 'Z':
        fakemodem->echo = TRUE;
        fakemodem->verbose = TRUE;
        return "OK";
    }
  }

  for (i = 0 ; i < fakemodem->patterns->len; i++) {
    pat = (Pattern *)g_ptr_array_index (fakemodem->patterns, i);
    if (g_regex_match (pat->command, cmd, 0, NULL)) {
      break;
    }
  }

  if (i == fakemodem->patterns->len)
    return "ERROR";

  if (pat->reply && pat->reply[0]) {
    int rval;
    printf (" Reply: '%s'\n", pat->reply);
    rval = write (masterfd, pat->reply, strlen (pat->reply));
    assert(strlen(pat->reply) == rval);
    rval = write (masterfd, "\r\n", 2);
    assert(2 == rval);
  }

  return pat->responsetext; /* NULL implies "OK" and keep processing */
}


static gboolean
send_unsolicited (FakeModem *fakemodem, const gchar* text)
{
  int rval;

  rval = write (masterfd, "\r\n", 2);
  rval = write (masterfd, text, strlen (text));
  assert(strlen(text) == rval);
  rval = write (masterfd, "\r\n", 2);
  assert(2 == rval);

  return TRUE;
}

static gboolean
set_response (FakeModem *fakemodem,
              const gchar* command,
              const gchar* reply,
              const gchar* response)
{
  int i;
  Pattern *pat;

  if (strlen (response) == 0)
    response = "OK";

  for (i = 0 ; i < fakemodem->patterns->len; i++) {
    pat = (Pattern *)g_ptr_array_index (fakemodem->patterns, i);
    if (strcmp (g_regex_get_pattern (pat->command), command) == 0) {
      g_free (pat->reply);
      pat->reply = g_strdup (reply);
      g_free (pat->responsetext);
      pat->responsetext = g_strdup (response);
      break;
    }
  }

  if (i == fakemodem->patterns->len) {
    GError *error = NULL;
    pat = g_malloc (sizeof (*pat));
    pat->command = g_regex_new (command,
                                G_REGEX_ANCHORED |
                                G_REGEX_CASELESS |
                                G_REGEX_RAW |
                                G_REGEX_OPTIMIZE,
                                0,
                                &error);
    if (pat->command == NULL) {
      printf ("error: %s\n", error->message);
      g_free (pat);
      return FALSE;
    }
    pat->responsetext = g_strdup (response);
    pat->reply = g_strdup (reply);
    g_ptr_array_add (fakemodem->patterns, pat);
  }

  return TRUE;
}

static gboolean
remove_response (FakeModem* fakemodem, const gchar* command)
{
  int i;
  gboolean found;
  Pattern *pat;

  found = FALSE;
  for (i = 0 ; i < fakemodem->patterns->len; i++) {
    pat = (Pattern *)g_ptr_array_index (fakemodem->patterns, i);
    if (strcmp (g_regex_get_pattern (pat->command), command) == 0) {
      g_ptr_array_remove_index (fakemodem->patterns, i);
      found = TRUE;
      break;
    }
  }

  return found;
}
