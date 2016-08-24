/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include "JniConstants.h"
#include <ScopedUtfChars.h>
#include <ScopedBytes.h>
#include <utils/misc.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <utils/String16.h>
#include <ctype.h>
#include <sys/socket.h>
#include <linux/if.h>
#include "wifi.h"
#include "wifi_hal.h"
#include "jni_helper.h"
#include "wifi_hal_mock.h"
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace android {

jobject mock_mObj; /* saved HalMock object (not class!) */
JavaVM* mock_mVM = NULL; /* saved JVM pointer */

/* Variable and function declared and defined in:
 *  com_android_server_wifi_nan_WifiNanNative.cpp
 */
extern wifi_hal_fn hal_fn;
extern "C" jint Java_com_android_server_wifi_WifiNative_registerNatives(
    JNIEnv* env, jclass clazz);

namespace hal_json_tags {
static constexpr const char* const type_tag = "type";
static constexpr const char* const value_tag = "value";

static constexpr const char* const type_int_tag = "int";
static constexpr const char* const type_byte_array_tag = "byte_array";
}

HalMockJsonWriter::HalMockJsonWriter()
    : allocator(doc.GetAllocator()) {
  doc.SetObject();
}

void HalMockJsonWriter::put_int(const char* name, int x) {
  rapidjson::Value object(rapidjson::kObjectType);
  object.AddMember(
      rapidjson::Value(hal_json_tags::type_tag,
                       strlen(hal_json_tags::type_tag)),
      rapidjson::Value(hal_json_tags::type_int_tag,
                       strlen(hal_json_tags::type_int_tag)),
      allocator);
  object.AddMember(
      rapidjson::Value(hal_json_tags::value_tag,
                       strlen(hal_json_tags::value_tag)),
      rapidjson::Value(x), allocator);
  doc.AddMember(rapidjson::Value(name, strlen(name)), object, allocator);
}

void HalMockJsonWriter::put_byte_array(const char* name, u8* byte_array,
                                       int array_length) {
  rapidjson::Value object(rapidjson::kObjectType);
  object.AddMember(
      rapidjson::Value(hal_json_tags::type_tag,
                       strlen(hal_json_tags::type_tag)),
      rapidjson::Value(hal_json_tags::type_byte_array_tag,
                       strlen(hal_json_tags::type_byte_array_tag)),
      allocator);

  rapidjson::Value array(rapidjson::kArrayType);
  for (int i = 0; i < array_length; ++i) {
    array.PushBack((int) byte_array[i], allocator);
  }

  object.AddMember(
      rapidjson::Value(hal_json_tags::value_tag,
                       strlen(hal_json_tags::value_tag)),
      array, allocator);
  doc.AddMember(rapidjson::Value(name, strlen(name)), object, allocator);
}

std::string HalMockJsonWriter::to_string() {
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer < rapidjson::StringBuffer > writer(strbuf);
  doc.Accept(writer);
  return strbuf.GetString();
}

HalMockJsonReader::HalMockJsonReader(const char* str) {
  doc.Parse(str);
  assert(doc.IsObject());
}

int HalMockJsonReader::get_int(const char* key, bool* error) {
  if (!doc.HasMember(key)) {
    *error = true;
    ALOGE("get_int: can't find %s key", key);
    return 0;
  }
  const rapidjson::Value& element = doc[key];
  if (!element.HasMember(hal_json_tags::value_tag)) {
    *error = true;
    ALOGE("get_int: can't find the 'value' sub-key for %s key", key);
    return 0;
  }
  const rapidjson::Value& value = element[hal_json_tags::value_tag];
  if (!value.IsInt()) {
    *error = true;
    ALOGE("get_int: the value isn't an 'int' for the %s key", key);
    return 0;
  }
  return value.GetInt();
}

void HalMockJsonReader::get_byte_array(const char* key, bool* error, u8* array,
                                       unsigned int max_array_size) {
  if (!doc.HasMember(key)) {
    *error = true;
    ALOGE("get_byte_array: can't find %s key", key);
    return;
  }
  const rapidjson::Value& element = doc[key];
  if (!element.HasMember(hal_json_tags::value_tag)) {
    *error = true;
    ALOGE("get_byte_array: can't find the 'value' sub-key for %s key", key);
    return;
  }
  const rapidjson::Value& value = element[hal_json_tags::value_tag];
  if (!value.IsArray()) {
    *error = true;
    ALOGE("get_byte_array: the value isn't an 'array' for the %s key", key);
    return;
  }

  if (value.Size() > max_array_size) {
    *error = true;
    ALOGE("get_byte_array: size of array (%d) is larger than maximum "
          "allocated (%d)",
          value.Size(), max_array_size);
    return;
  }

  for (unsigned int i = 0; i < value.Size(); ++i) {
    const rapidjson::Value& item = value[i];
    if (!item.IsInt()) {
      *error = true;
      ALOGE("get_byte_array: the value isn't an 'int' for the %s[%d] key", key,
            i);
      return;
    }
    array[i] = item.GetInt();
  }
}


int init_wifi_hal_func_table_mock(wifi_hal_fn *fn) {
  if (fn == NULL) {
    return -1;
  }

  /* TODO: add other Wi-Fi HAL registrations here - once you have mocks */

  return 0;
}

extern "C" jint Java_com_android_server_wifi_HalMockUtils_initHalMock(
    JNIEnv* env, jclass clazz) {
  env->GetJavaVM(&mock_mVM);

  Java_com_android_server_wifi_WifiNative_registerNatives(env, clazz);
  return init_wifi_hal_func_table_mock(&hal_fn);
}

extern "C" void Java_com_android_server_wifi_HalMockUtils_setHalMockObject(
    JNIEnv* env, jclass clazz, jobject hal_mock_object) {
  mock_mObj = (jobject) env->NewGlobalRef(hal_mock_object);
}

}  // namespace android
