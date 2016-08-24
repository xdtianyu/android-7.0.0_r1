/* stats.c - statistics from the bus driver
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <config.h>
#include "stats.h"

#include <dbus/dbus-internals.h>
#include <dbus/dbus-connection-internal.h>

#include "connection.h"
#include "services.h"
#include "utils.h"

#ifdef DBUS_ENABLE_STATS

static DBusMessage *
new_asv_reply (DBusMessage      *message,
               DBusMessageIter  *iter,
               DBusMessageIter  *arr_iter)
{
  DBusMessage *reply = dbus_message_new_method_return (message);

  if (reply == NULL)
    return NULL;

  dbus_message_iter_init_append (reply, iter);

  if (!dbus_message_iter_open_container (iter, DBUS_TYPE_ARRAY, "{sv}",
                                         arr_iter))
    {
      dbus_message_unref (reply);
      return NULL;
    }

  return reply;
}

static dbus_bool_t
open_asv_entry (DBusMessageIter *arr_iter,
                DBusMessageIter *entry_iter,
                const char      *key,
                const char      *type,
                DBusMessageIter *var_iter)
{
  if (!dbus_message_iter_open_container (arr_iter, DBUS_TYPE_DICT_ENTRY,
                                         NULL, entry_iter))
    return FALSE;

  if (!dbus_message_iter_append_basic (entry_iter, DBUS_TYPE_STRING, &key))
    {
      dbus_message_iter_abandon_container (arr_iter, entry_iter);
      return FALSE;
    }

  if (!dbus_message_iter_open_container (entry_iter, DBUS_TYPE_VARIANT,
                                         type, var_iter))
    {
      dbus_message_iter_abandon_container (arr_iter, entry_iter);
      return FALSE;
    }

  return TRUE;
}

static dbus_bool_t
close_asv_entry (DBusMessageIter *arr_iter,
                 DBusMessageIter *entry_iter,
                 DBusMessageIter *var_iter)
{
  if (!dbus_message_iter_close_container (entry_iter, var_iter))
    {
      dbus_message_iter_abandon_container (arr_iter, entry_iter);
      return FALSE;
    }

  if (!dbus_message_iter_close_container (arr_iter, entry_iter))
    return FALSE;

  return TRUE;
}

static dbus_bool_t
close_asv_reply (DBusMessageIter *iter,
                 DBusMessageIter *arr_iter)
{
  return dbus_message_iter_close_container (iter, arr_iter);
}

static void
abandon_asv_entry (DBusMessageIter *arr_iter,
                   DBusMessageIter *entry_iter,
                   DBusMessageIter *var_iter)
{
  dbus_message_iter_abandon_container (entry_iter, var_iter);
  dbus_message_iter_abandon_container (arr_iter, entry_iter);
}

static void
abandon_asv_reply (DBusMessageIter *iter,
                 DBusMessageIter *arr_iter)
{
  dbus_message_iter_abandon_container (iter, arr_iter);
}

static dbus_bool_t
asv_add_uint32 (DBusMessageIter *iter,
                DBusMessageIter *arr_iter,
                const char *key,
                dbus_uint32_t value)
{
  DBusMessageIter entry_iter, var_iter;

  if (!open_asv_entry (arr_iter, &entry_iter, key, DBUS_TYPE_UINT32_AS_STRING,
                       &var_iter))
    goto oom;

  if (!dbus_message_iter_append_basic (&var_iter, DBUS_TYPE_UINT32,
                                       &value))
    {
      abandon_asv_entry (arr_iter, &entry_iter, &var_iter);
      goto oom;
    }

  if (!close_asv_entry (arr_iter, &entry_iter, &var_iter))
    goto oom;

  return TRUE;

oom:
  abandon_asv_reply (iter, arr_iter);
  return FALSE;
}

static dbus_bool_t
asv_add_string (DBusMessageIter *iter,
                DBusMessageIter *arr_iter,
                const char *key,
                const char *value)
{
  DBusMessageIter entry_iter, var_iter;

  if (!open_asv_entry (arr_iter, &entry_iter, key, DBUS_TYPE_STRING_AS_STRING,
                       &var_iter))
    goto oom;

  if (!dbus_message_iter_append_basic (&var_iter, DBUS_TYPE_STRING,
                                       &value))
    {
      abandon_asv_entry (arr_iter, &entry_iter, &var_iter);
      goto oom;
    }

  if (!close_asv_entry (arr_iter, &entry_iter, &var_iter))
    goto oom;

  return TRUE;

oom:
  abandon_asv_reply (iter, arr_iter);
  return FALSE;
}

dbus_bool_t
bus_stats_handle_get_stats (DBusConnection *connection,
                            BusTransaction *transaction,
                            DBusMessage    *message,
                            DBusError      *error)
{
  BusConnections *connections;
  DBusMessage *reply = NULL;
  DBusMessageIter iter, arr_iter;
  static dbus_uint32_t stats_serial = 0;
  dbus_uint32_t in_use, in_free_list, allocated;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);

  connections = bus_transaction_get_connections (transaction);

  reply = new_asv_reply (message, &iter, &arr_iter);

  if (reply == NULL)
    goto oom;

  /* Globals */

  if (!asv_add_uint32 (&iter, &arr_iter, "Serial", stats_serial++))
    goto oom;

  _dbus_list_get_stats (&in_use, &in_free_list, &allocated);
  if (!asv_add_uint32 (&iter, &arr_iter, "ListMemPoolUsedBytes", in_use) ||
      !asv_add_uint32 (&iter, &arr_iter, "ListMemPoolCachedBytes",
                       in_free_list) ||
      !asv_add_uint32 (&iter, &arr_iter, "ListMemPoolAllocatedBytes",
                       allocated))
    goto oom;

  /* Connections */

  if (!asv_add_uint32 (&iter, &arr_iter, "ActiveConnections",
        bus_connections_get_n_active (connections)) ||
      !asv_add_uint32 (&iter, &arr_iter, "IncompleteConnections",
        bus_connections_get_n_incomplete (connections)) ||
      !asv_add_uint32 (&iter, &arr_iter, "MatchRules",
        bus_connections_get_total_match_rules (connections)) ||
      !asv_add_uint32 (&iter, &arr_iter, "PeakMatchRules",
        bus_connections_get_peak_match_rules (connections)) ||
      !asv_add_uint32 (&iter, &arr_iter, "PeakMatchRulesPerConnection",
        bus_connections_get_peak_match_rules_per_conn (connections)) ||
      !asv_add_uint32 (&iter, &arr_iter, "BusNames",
        bus_connections_get_total_bus_names (connections)) ||
      !asv_add_uint32 (&iter, &arr_iter, "PeakBusNames",
        bus_connections_get_peak_bus_names (connections)) ||
      !asv_add_uint32 (&iter, &arr_iter, "PeakBusNamesPerConnection",
        bus_connections_get_peak_bus_names_per_conn (connections)))
    goto oom;

  /* end */

  if (!close_asv_reply (&iter, &arr_iter))
    goto oom;

  if (!bus_transaction_send_from_driver (transaction, connection, reply))
    goto oom;

  dbus_message_unref (reply);
  return TRUE;

oom:
  if (reply != NULL)
    dbus_message_unref (reply);

  BUS_SET_OOM (error);
  return FALSE;
}

dbus_bool_t
bus_stats_handle_get_connection_stats (DBusConnection *caller_connection,
                                       BusTransaction *transaction,
                                       DBusMessage    *message,
                                       DBusError      *error)
{
  const char *bus_name = NULL;
  DBusString bus_name_str;
  DBusMessage *reply = NULL;
  DBusMessageIter iter, arr_iter;
  static dbus_uint32_t stats_serial = 0;
  dbus_uint32_t in_messages, in_bytes, in_fds, in_peak_bytes, in_peak_fds;
  dbus_uint32_t out_messages, out_bytes, out_fds, out_peak_bytes, out_peak_fds;
  BusRegistry *registry;
  BusService *service;
  DBusConnection *stats_connection;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);

  registry = bus_connection_get_registry (caller_connection);

  if (! dbus_message_get_args (message, error,
                               DBUS_TYPE_STRING, &bus_name,
                               DBUS_TYPE_INVALID))
      return FALSE;

  _dbus_string_init_const (&bus_name_str, bus_name);
  service = bus_registry_lookup (registry, &bus_name_str);

  if (service == NULL)
    {
      dbus_set_error (error, DBUS_ERROR_NAME_HAS_NO_OWNER,
                      "Bus name '%s' has no owner", bus_name);
      return FALSE;
    }

  stats_connection = bus_service_get_primary_owners_connection (service);
  _dbus_assert (stats_connection != NULL);

  reply = new_asv_reply (message, &iter, &arr_iter);

  if (reply == NULL)
    goto oom;

  /* Bus daemon per-connection stats */

  if (!asv_add_uint32 (&iter, &arr_iter, "Serial", stats_serial++) ||
      !asv_add_uint32 (&iter, &arr_iter, "MatchRules",
        bus_connection_get_n_match_rules (stats_connection)) ||
      !asv_add_uint32 (&iter, &arr_iter, "PeakMatchRules",
        bus_connection_get_peak_match_rules (stats_connection)) ||
      !asv_add_uint32 (&iter, &arr_iter, "BusNames",
        bus_connection_get_n_services_owned (stats_connection)) ||
      !asv_add_uint32 (&iter, &arr_iter, "PeakBusNames",
        bus_connection_get_peak_bus_names (stats_connection)) ||
      !asv_add_string (&iter, &arr_iter, "UniqueName",
        bus_connection_get_name (stats_connection)))
    goto oom;

  /* DBusConnection per-connection stats */

  _dbus_connection_get_stats (stats_connection,
                              &in_messages, &in_bytes, &in_fds,
                              &in_peak_bytes, &in_peak_fds,
                              &out_messages, &out_bytes, &out_fds,
                              &out_peak_bytes, &out_peak_fds);

  if (!asv_add_uint32 (&iter, &arr_iter, "IncomingMessages", in_messages) ||
      !asv_add_uint32 (&iter, &arr_iter, "IncomingBytes", in_bytes) ||
      !asv_add_uint32 (&iter, &arr_iter, "IncomingFDs", in_fds) ||
      !asv_add_uint32 (&iter, &arr_iter, "PeakIncomingBytes", in_peak_bytes) ||
      !asv_add_uint32 (&iter, &arr_iter, "PeakIncomingFDs", in_peak_fds) ||
      !asv_add_uint32 (&iter, &arr_iter, "OutgoingMessages", out_messages) ||
      !asv_add_uint32 (&iter, &arr_iter, "OutgoingBytes", out_bytes) ||
      !asv_add_uint32 (&iter, &arr_iter, "OutgoingFDs", out_fds) ||
      !asv_add_uint32 (&iter, &arr_iter, "PeakOutgoingBytes", out_peak_bytes) ||
      !asv_add_uint32 (&iter, &arr_iter, "PeakOutgoingFDs", out_peak_fds))
    goto oom;

  /* end */

  if (!close_asv_reply (&iter, &arr_iter))
    goto oom;

  if (!bus_transaction_send_from_driver (transaction, caller_connection,
                                         reply))
    goto oom;

  dbus_message_unref (reply);
  return TRUE;

oom:
  if (reply != NULL)
    dbus_message_unref (reply);

  BUS_SET_OOM (error);
  return FALSE;
}

#endif
