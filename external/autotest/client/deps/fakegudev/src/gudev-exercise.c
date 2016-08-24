/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>

#define G_UDEV_API_IS_SUBJECT_TO_CHANGE
#include <gudev/gudev.h>

gboolean lookup (const gpointer data);

static GMainLoop* loop;

int
main (int argc, const char *argv[])
{
  int i;

  #if !GLIB_CHECK_VERSION(2,35,0)
  g_type_init ();
  #endif

  loop = g_main_loop_new (NULL, FALSE);

  for (i = 1 ; i < argc ; i++)
    g_idle_add (lookup, (const gpointer)argv[i]);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);

  return 0;
}

static void
print_device(GUdevDevice *device)
{
  GHashTable *properties;
  GHashTableIter iter;
  gpointer key, value;

  printf (" Name:        %s\n", g_udev_device_get_name (device));
  printf (" Device file: %s\n", g_udev_device_get_device_file (device));
  printf (" Devtype:     %s\n", g_udev_device_get_devtype (device));
  printf (" Driver:      %s\n", g_udev_device_get_driver (device));
  printf (" Subsystem:   %s\n", g_udev_device_get_subsystem (device));
  printf (" Sysfs path:  %s\n", g_udev_device_get_sysfs_path (device));

  /* We want to print out properties in some fixed order every time.
   * To do this, we hash on the property name, and then iterate.
   */
  const gchar * const * keys = g_udev_device_get_property_keys (device);
  properties = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  for (;*keys;++keys) {
    const gchar * prop;

    prop = g_udev_device_get_property (device, *keys);
    g_hash_table_insert (properties, g_strdup (*keys), g_strdup (prop));
  }

  g_hash_table_iter_init (&iter, properties);
  while (g_hash_table_iter_next (&iter, &key, &value))
    printf ("  Property %s: %s\n", (gchar *)key, (gchar *)value);

  g_hash_table_unref (properties);
}

gboolean
lookup (const gpointer data)
{
  const char *path = data;

  GUdevClient *guclient = g_udev_client_new (NULL);
  GUdevDevice *device;

  if (path[0] == '=') {
    gchar **parts;
    parts = g_strsplit (path+1, ",", 2);

    device = g_udev_client_query_by_subsystem_and_name (guclient, parts[0],
                                                        parts[1]);
    g_strfreev (parts);
  } else if (strncmp (path, "/sys/", 5) == 0) {
    device = g_udev_client_query_by_sysfs_path (guclient, path);
  } else {
    device = g_udev_client_query_by_device_file (guclient, path);
  }

  if (device) {
    print_device (device);
    if (1) {
      GUdevDevice *parent;
      parent = g_udev_device_get_parent (device);
      if (parent) {
        printf ("Parent device:\n");
        print_device (parent);
        g_object_unref (parent);
      }
    }
    g_object_unref (device);
  }
  printf("\n");

  g_object_unref (guclient);

  g_main_loop_quit (loop);

  return FALSE;
}
