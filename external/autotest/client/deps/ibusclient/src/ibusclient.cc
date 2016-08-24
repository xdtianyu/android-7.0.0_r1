// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <glib.h>
#include <ibus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

namespace {

const gchar kDummySection[] = "aaa/bbb";
const gchar kDummyConfigName[] = "ccc";

const gboolean kDummyValueBoolean = TRUE;
const gint kDummyValueInt = 12345;
const gdouble kDummyValueDouble = 2345.5432;
const gchar kDummyValueString[] = "dummy value";

const size_t kArraySize = 3;
const gboolean kDummyValueBooleanArray[kArraySize] = { FALSE, TRUE, FALSE };
const gint kDummyValueIntArray[kArraySize] = { 123, 234, 345 };
const gdouble kDummyValueDoubleArray[kArraySize] = { 111.22, 333.44, 555.66 };
const gchar* kDummyValueStringArray[kArraySize] = {
  "DUMMY_VALUE 1", "DUMMY_VALUE 2", "DUMMY_VALUE 3",
};

const char kGeneralSectionName[] = "general";
const char kPreloadEnginesConfigName[] = "preload_engines";

// Converts |list_type_string| into its element type (e.g. "int_list" to "int").
std::string GetElementType(const std::string& list_type_string) {
  const std::string suffix = "_list";
  if (list_type_string.length() > suffix.length()) {
    return list_type_string.substr(
        0, list_type_string.length() - suffix.length());
  }
  return list_type_string;
}

// Converts |type_string| into GVariantClass.
GVariantClass GetGVariantClassFromStringOrDie(const std::string& type_string) {
  if (type_string == "boolean") {
    return G_VARIANT_CLASS_BOOLEAN;
  } else if (type_string == "int") {
    return G_VARIANT_CLASS_INT32;
  } else if (type_string == "double") {
    return G_VARIANT_CLASS_DOUBLE;
  } else if (type_string == "string") {
    return G_VARIANT_CLASS_STRING;
  } else if (GetElementType(type_string) != type_string) {
    return G_VARIANT_CLASS_ARRAY;
  }
  printf("FAIL (unknown type: %s)\n", type_string.c_str());
  abort();
}

// Unsets a dummy value from ibus config service.
void UnsetConfigAndPrintResult(IBusConfig* ibus_config) {
  if (ibus_config_unset(ibus_config, kDummySection, kDummyConfigName)) {
    printf("OK\n");
  } else {
    printf("FAIL\n");
  }
}

// Sets a dummy value to ibus config service. You can specify a type of the
// dummy value by |type_string|. "boolean", "int", "double", or "string" are
// allowed.
void SetConfigAndPrintResult(
    IBusConfig* ibus_config, const std::string& type_string) {
  GVariant* variant = NULL;
  GVariantClass klass = GetGVariantClassFromStringOrDie(type_string);

  switch (klass) {
    case G_VARIANT_CLASS_BOOLEAN:
      variant = g_variant_new_boolean(kDummyValueBoolean);
      break;
    case G_VARIANT_CLASS_INT32:
      variant = g_variant_new_int32(kDummyValueInt);
      break;
    case G_VARIANT_CLASS_DOUBLE:
      variant = g_variant_new_double(kDummyValueDouble);
      break;
    case G_VARIANT_CLASS_STRING:
      variant = g_variant_new_string(kDummyValueString);
      break;
    case G_VARIANT_CLASS_ARRAY: {
      const GVariantClass element_klass
          = GetGVariantClassFromStringOrDie(GetElementType(type_string));
      g_assert(element_klass != G_VARIANT_CLASS_ARRAY);

      GVariantBuilder variant_builder;
      for (size_t i = 0; i < kArraySize; ++i) {
        switch (element_klass) {
          case G_VARIANT_CLASS_BOOLEAN:
            if (i == 0) {
              g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("ab"));
            }
            g_variant_builder_add(
                &variant_builder, "b", kDummyValueBooleanArray[i]);
            break;
          case G_VARIANT_CLASS_INT32:
            if (i == 0) {
              g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("ai"));
            }
            g_variant_builder_add(
                &variant_builder, "i", kDummyValueIntArray[i]);
            break;
          case G_VARIANT_CLASS_DOUBLE:
            if (i == 0) {
              g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("ad"));
            }
            g_variant_builder_add(
                &variant_builder, "d", kDummyValueDoubleArray[i]);
            break;
          case G_VARIANT_CLASS_STRING:
            if (i == 0) {
              g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("as"));
            }
            g_variant_builder_add(
                &variant_builder, "s", kDummyValueStringArray[i]);
            break;
          default:
            printf("FAIL\n");
            return;
        }
      }
      variant = g_variant_builder_end(&variant_builder);
      break;
    }
    default:
      printf("FAIL\n");
      return;
  }
  if (!variant) {
    printf("FAIL\n");
    return;
  }
  if (ibus_config_set_value(
          ibus_config, kDummySection, kDummyConfigName, variant)) {
    printf("OK\n");
    return;
  }
  printf("FAIL\n");
}

// Gets a dummy value from ibus config service. This function checks if the
// dummy value is |type_string| type.
void GetConfigAndPrintResult(
    IBusConfig* ibus_config, const std::string& type_string) {
  GVariant* variant = ibus_config_get_value(
      ibus_config, kDummySection, kDummyConfigName);
  if (!variant) {
    printf("FAIL (not found)\n");
    return;
  }
  switch(g_variant_classify(variant)) {
    case G_VARIANT_CLASS_BOOLEAN: {
      if (g_variant_get_boolean(variant) != kDummyValueBoolean) {
        printf("FAIL (value mismatch)\n");
        return;
      }
      break;
    }
    case G_VARIANT_CLASS_INT32: {
      if (g_variant_get_int32(variant) != kDummyValueInt) {
        printf("FAIL (value mismatch)\n");
        return;
      }
      break;
    }
    case G_VARIANT_CLASS_DOUBLE: {
      if (g_variant_get_double(variant) != kDummyValueDouble) {
        printf("FAIL (value mismatch)\n");
        return;
      }
      break;
    }
    case G_VARIANT_CLASS_STRING: {
      const char* value = g_variant_get_string(variant, NULL);
      if (value == NULL ||
          value != std::string(kDummyValueString)) {
        printf("FAIL (value mismatch)\n");
        return;
      }
      break;
    }
    case G_VARIANT_CLASS_ARRAY: {
      const GVariantType* variant_element_type
          = g_variant_type_element(g_variant_get_type(variant));
      GVariantIter iter;
      g_variant_iter_init(&iter, variant);

      size_t i;
      GVariant* element = g_variant_iter_next_value(&iter);
      for (i = 0; element; ++i) {
        bool match = false;
        if (g_variant_type_equal(
                variant_element_type, G_VARIANT_TYPE_BOOLEAN)) {
          const gboolean value = g_variant_get_boolean(element);
          match = (value == kDummyValueBooleanArray[i]);
        } else if (g_variant_type_equal(
            variant_element_type, G_VARIANT_TYPE_INT32)) {
          const gint32 value = g_variant_get_int32(element);
          match = (value == kDummyValueIntArray[i]);
        } else if (g_variant_type_equal(
            variant_element_type, G_VARIANT_TYPE_DOUBLE)) {
          const gdouble value = g_variant_get_double(element);
          match = (value == kDummyValueDoubleArray[i]);
        } else if (g_variant_type_equal(
            variant_element_type, G_VARIANT_TYPE_STRING)) {
          const char* value = g_variant_get_string(element, NULL);
          match = (value && (value == std::string(kDummyValueStringArray[i])));
        } else {
          printf("FAIL (list type mismatch)\n");
          return;
        }
        if (!match) {
          printf("FAIL (value mismatch)\n");
          return;
        }
        g_variant_unref(element);
        element = g_variant_iter_next_value(&iter);
      }
      if (i != kArraySize) {
        printf("FAIL (invalid array)\n");
        return;
      }
      break;
    }
    default:
      printf("FAIL (unknown type)\n");
      return;
  }
  printf("OK\n");
}

// Prints out the array. It is assumed that the array contains STRING values.
// On success, returns true
// On failure, prints out "FAIL (error message)" and returns false
bool PrintArray(GVariant* variant) {
  if (g_variant_classify(variant) != G_VARIANT_CLASS_ARRAY) {
    printf("FAIL (Not an array)\n");
    return false;
  }
  const GVariantType* variant_element_type
      = g_variant_type_element(g_variant_get_type(variant));
  if (!g_variant_type_equal(variant_element_type, G_VARIANT_TYPE_STRING)) {
    printf("FAIL (Array element type is not STRING)\n");
    return false;
  }
  GVariantIter iter;
  g_variant_iter_init(&iter, variant);
  GVariant* element = g_variant_iter_next_value(&iter);
  while(element) {
    const char* value = g_variant_get_string(element, NULL);
    if (!value) {
      printf("FAIL (Array element type is NULL)\n");
      return false;
    }
    printf("%s\n", value);
    element = g_variant_iter_next_value(&iter);
  }
  return true;
}

// Print out the list of unused config variables from ibus.
// On failure, prints out "FAIL (error message)" instead.
void PrintUnused(IBusConfig* ibus_config) {
  GVariant* unread = NULL;
  GVariant* unwritten = NULL;
  if (!ibus_config_get_unused(ibus_config, &unread, &unwritten)) {
    printf("FAIL (get_unused failed)\n");
    return;
  }

  if (g_variant_classify(unread) != G_VARIANT_CLASS_ARRAY) {
    printf("FAIL (unread is not an array)\n");
    g_variant_unref(unread);
    g_variant_unref(unwritten);
    return;
  }

  if (g_variant_classify(unwritten) != G_VARIANT_CLASS_ARRAY) {
    printf("FAIL (unwritten is not an array)\n");
    g_variant_unref(unread);
    g_variant_unref(unwritten);
    return;
  }

  printf("Unread:\n");
  if (!PrintArray(unread)) {
    g_variant_unref(unread);
    g_variant_unref(unwritten);
    return;
  }

  printf("Unwritten:\n");
  if (!PrintArray(unwritten)) {
    g_variant_unref(unread);
    g_variant_unref(unwritten);
    return;
  }

  g_variant_unref(unread);
  g_variant_unref(unwritten);
}

// Set the preload engines to those named in the array |engines| of size
// |num_engines| and prints the result.
//
// Note that this only fails if it can't set the config value; it does not check
// that the names of the engines are valid.
void PreloadEnginesAndPrintResult(IBusConfig* ibus_config, int num_engines,
                                  char** engines) {
  GVariant* variant = NULL;
  GVariantBuilder variant_builder;
  g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("as"));
  for (int i = 0; i < num_engines; ++i) {
    g_variant_builder_add(&variant_builder, "s", engines[i]);
  }
  variant = g_variant_builder_end(&variant_builder);

  if (ibus_config_set_value(ibus_config, kGeneralSectionName,
                            kPreloadEnginesConfigName, variant)) {
    printf("OK\n");
  } else {
    printf("FAIL\n");
  }
  g_variant_unref(variant);
}

// Sets |engine_name| as the active IME engine.
void ActivateEngineAndPrintResult(IBusBus* ibus, const char* engine_name) {
  if (!ibus_bus_set_global_engine(ibus, engine_name)) {
    printf("FAIL (could not start engine)\n");
  } else {
    printf("OK\n");
  }
}

// Prints the name of the active IME engine.
void PrintActiveEngine(IBusBus* ibus) {
  IBusEngineDesc* engine_desc = ibus_bus_get_global_engine(ibus);
  if (engine_desc) {
    printf("%s\n", ibus_engine_desc_get_name(engine_desc));
    g_object_unref(engine_desc);
  } else {
    printf("FAIL (Could not get active engine)\n");
  }
}

// Prints the names of the given engines. Takes the ownership of |engines|.
void PrintEngineNames(GList* engines) {
  for (GList* cursor = engines; cursor; cursor = g_list_next(cursor)) {
    IBusEngineDesc* engine_desc = IBUS_ENGINE_DESC(cursor->data);
    assert(engine_desc);
    printf("%s\n", ibus_engine_desc_get_name(engine_desc));
    g_object_unref(IBUS_ENGINE_DESC(cursor->data));
  }
  g_list_free(engines);
}

void PrintUsage(const char* argv0) {
  printf("Usage: %s COMMAND\n", argv0);
  printf("check_reachable      Check if ibus-daemon is reachable\n");
  printf("list_engines         List engine names (all engines)\n");
  printf("list_active_engines  List active engine names\n");
  // TODO(yusukes): Add 2 parameters, config_key and config_value, to
  // set_config and get_config commands.
  printf("set_config (boolean|int|double|string|\n"
         "            boolean_list|int_list|double_list|string_list)\n"
         "                     Set a dummy value to ibus config service\n");
  printf("get_config (boolean|int|double|string\n"
         "            boolean_list|int_list|double_list|string_list)\n"
         "                     Get a dummy value from ibus config service\n");
  // TODO(yusukes): Add config_key parameter to unset_config.
  printf("unset_config         Unset a dummy value from ibus config service\n");
  printf("get_unused           List all keys that never were used.\n");
  printf("preload_engines      Preload the listed engines.\n");
  printf("activate_engine      Activate the specified engine.\n");
  printf("get_active_engine    Print the name of the current active engine.\n");
}

}  // namespace

int main(int argc, char **argv) {
  if (argc == 1) {
    PrintUsage(argv[0]);
    return 1;
  }

  ibus_init();
  bool connected = false;
  IBusBus* ibus = ibus_bus_new();
  if (ibus) {
    connected = ibus_bus_is_connected(ibus);
  }

  const std::string command = argv[1];
  if (command == "check_reachable") {
    printf("%s\n", connected ? "YES" : "NO");
    return 0;
  } else if (!connected) {
    printf("FAIL (Not connected)\n");
    return 0;
  }

  // Other commands need the bus to be connected.
  assert(ibus);
  assert(connected);
  GDBusConnection* ibus_connection = ibus_bus_get_connection(ibus);
  assert(ibus_connection);
  IBusConfig* ibus_config = ibus_config_new(ibus_connection, NULL, NULL);
  assert(ibus_config);

  if (command == "list_engines") {
    PrintEngineNames(ibus_bus_list_engines(ibus));
  } else if (command == "list_active_engines") {
    PrintEngineNames(ibus_bus_list_active_engines(ibus));
  } else if (command == "set_config") {
    if (argc != 3) {
      PrintUsage(argv[0]);
      return 1;
    }
    SetConfigAndPrintResult(ibus_config, argv[2]);
  } else if (command == "get_config") {
    if (argc != 3) {
      PrintUsage(argv[0]);
      return 1;
    }
    GetConfigAndPrintResult(ibus_config, argv[2]);
  } else if (command == "unset_config") {
    UnsetConfigAndPrintResult(ibus_config);
  } else if (command == "get_unused") {
    PrintUnused(ibus_config);
  } else if (command == "preload_engines") {
    if (argc < 3) {
      PrintUsage(argv[0]);
      return 1;
    }
    PreloadEnginesAndPrintResult(ibus_config, argc-2, &(argv[2]));
  } else if (command == "activate_engine") {
    if (argc != 3) {
      PrintUsage(argv[0]);
      return 1;
    }
    ActivateEngineAndPrintResult(ibus, argv[2]);
  } else if (command == "get_active_engine") {
    PrintActiveEngine(ibus);
  } else {
    PrintUsage(argv[0]);
    return 1;
  }

  return 0;
}
