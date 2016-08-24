# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Default values for the USE flags. Override these USE flags from your product
# by setting BRILLO_USE_* values. Note that we define local variables like
# local_use_* to prevent leaking our default setting for other packages.
local_use_dbus := $(if $(BRILLO_USE_DBUS),$(BRILLO_USE_DBUS),1)

LOCAL_PATH := $(call my-dir)

libbrillo_cpp_extension := .cc
libbrillo_core_sources := \
    brillo/backoff_entry.cc \
    brillo/data_encoding.cc \
    brillo/errors/error.cc \
    brillo/errors/error_codes.cc \
    brillo/flag_helper.cc \
    brillo/key_value_store.cc \
    brillo/message_loops/base_message_loop.cc \
    brillo/message_loops/message_loop.cc \
    brillo/message_loops/message_loop_utils.cc \
    brillo/mime_utils.cc \
    brillo/osrelease_reader.cc \
    brillo/process.cc \
    brillo/process_information.cc \
    brillo/secure_blob.cc \
    brillo/strings/string_utils.cc \
    brillo/syslog_logging.cc \
    brillo/type_name_undecorate.cc \
    brillo/url_utils.cc \
    brillo/userdb_utils.cc \
    brillo/value_conversion.cc \

libbrillo_linux_sources := \
    brillo/asynchronous_signal_handler.cc \
    brillo/daemons/daemon.cc \
    brillo/file_utils.cc \
    brillo/process_reaper.cc \

libbrillo_binder_sources := \
    brillo/binder_watcher.cc \

libbrillo_dbus_sources := \
    brillo/any.cc \
    brillo/daemons/dbus_daemon.cc \
    brillo/dbus/async_event_sequencer.cc \
    brillo/dbus/data_serialization.cc \
    brillo/dbus/dbus_connection.cc \
    brillo/dbus/dbus_method_invoker.cc \
    brillo/dbus/dbus_method_response.cc \
    brillo/dbus/dbus_object.cc \
    brillo/dbus/dbus_service_watcher.cc \
    brillo/dbus/dbus_signal.cc \
    brillo/dbus/exported_object_manager.cc \
    brillo/dbus/exported_property_set.cc \
    brillo/dbus/utils.cc \

libbrillo_http_sources := \
    brillo/http/curl_api.cc \
    brillo/http/http_connection_curl.cc \
    brillo/http/http_form_data.cc \
    brillo/http/http_request.cc \
    brillo/http/http_transport.cc \
    brillo/http/http_transport_curl.cc \
    brillo/http/http_utils.cc \

libbrillo_policy_sources := \
    policy/device_policy.cc \
    policy/libpolicy.cc \

libbrillo_stream_sources := \
    brillo/streams/file_stream.cc \
    brillo/streams/input_stream_set.cc \
    brillo/streams/memory_containers.cc \
    brillo/streams/memory_stream.cc \
    brillo/streams/openssl_stream_bio.cc \
    brillo/streams/stream.cc \
    brillo/streams/stream_errors.cc \
    brillo/streams/stream_utils.cc \
    brillo/streams/tls_stream.cc \

libbrillo_test_helpers_sources := \
    brillo/http/http_connection_fake.cc \
    brillo/http/http_transport_fake.cc \
    brillo/message_loops/fake_message_loop.cc \
    brillo/streams/fake_stream.cc \

libbrillo_test_sources := \
    brillo/asynchronous_signal_handler_unittest.cc \
    brillo/backoff_entry_unittest.cc \
    brillo/data_encoding_unittest.cc \
    brillo/errors/error_codes_unittest.cc \
    brillo/errors/error_unittest.cc \
    brillo/file_utils_unittest.cc \
    brillo/flag_helper_unittest.cc \
    brillo/http/http_connection_curl_unittest.cc \
    brillo/http/http_form_data_unittest.cc \
    brillo/http/http_request_unittest.cc \
    brillo/http/http_transport_curl_unittest.cc \
    brillo/http/http_utils_unittest.cc \
    brillo/key_value_store_unittest.cc \
    brillo/map_utils_unittest.cc \
    brillo/message_loops/base_message_loop_unittest.cc \
    brillo/message_loops/fake_message_loop_unittest.cc \
    brillo/mime_utils_unittest.cc \
    brillo/osrelease_reader_unittest.cc \
    brillo/process_reaper_unittest.cc \
    brillo/process_unittest.cc \
    brillo/secure_blob_unittest.cc \
    brillo/streams/fake_stream_unittest.cc \
    brillo/streams/file_stream_unittest.cc \
    brillo/streams/input_stream_set_unittest.cc \
    brillo/streams/memory_containers_unittest.cc \
    brillo/streams/memory_stream_unittest.cc \
    brillo/streams/openssl_stream_bio_unittests.cc \
    brillo/streams/stream_unittest.cc \
    brillo/streams/stream_utils_unittest.cc \
    brillo/strings/string_utils_unittest.cc \
    brillo/type_name_undecorate_unittest.cc \
    brillo/unittest_utils.cc \
    brillo/url_utils_unittest.cc \
    brillo/value_conversion_unittest.cc \

libbrillo_dbus_test_sources := \
    brillo/any_unittest.cc \
    brillo/any_internal_impl_unittest.cc \
    brillo/dbus/async_event_sequencer_unittest.cc \
    brillo/dbus/data_serialization_unittest.cc \
    brillo/dbus/dbus_method_invoker_unittest.cc \
    brillo/dbus/dbus_object_unittest.cc \
    brillo/dbus/dbus_param_reader_unittest.cc \
    brillo/dbus/dbus_param_writer_unittest.cc \
    brillo/dbus/dbus_signal_handler_unittest.cc \
    brillo/dbus/exported_object_manager_unittest.cc \
    brillo/dbus/exported_property_set_unittest.cc \
    brillo/dbus/test.proto \
    brillo/variant_dictionary_unittest.cc \

libbrillo_CFLAGS := \
    -Wall \
    -Werror \
    -DUSE_DBUS=$(local_use_dbus)
libbrillo_CPPFLAGS :=
libbrillo_includes := external/gtest/include
libbrillo_shared_libraries := libchrome

# Shared library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo
LOCAL_SRC_FILES := $(libbrillo_core_sources) $(libbrillo_linux_sources)
LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries)
LOCAL_STATIC_LIBRARIES := libmodpb64
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS)
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)

# Shared binder library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo-binder
LOCAL_SRC_FILES := $(libbrillo_binder_sources)
LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_SHARED_LIBRARIES := \
    $(libbrillo_shared_libraries) \
    libbinder \
    libbrillo \
    libutils
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS)
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)

ifeq ($(local_use_dbus),1)

# Shared dbus library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo-dbus
LOCAL_SRC_FILES := $(libbrillo_dbus_sources)
LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries) libbrillo \
    libchrome-dbus libdbus
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS)
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH) external/dbus
include $(BUILD_SHARED_LIBRARY)

endif  # local_use_dbus == 1

# Shared minijail library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo-minijail
LOCAL_SRC_FILES := brillo/minijail/minijail.cc \

LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries) libbrillo \
    libminijail
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS)
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)

# Shared stream library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo-stream
LOCAL_SRC_FILES := $(libbrillo_stream_sources)
LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries) libbrillo \
    libcrypto libssl
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS)
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)

# Shared http library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo-http
LOCAL_SRC_FILES := $(libbrillo_http_sources)
LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries) libbrillo \
    libbrillo-stream libcurl
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS)
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)

# Shared policy library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo-policy
LOCAL_SRC_FILES := $(libbrillo_policy_sources)
LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries)
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS)
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)

# Static library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo
LOCAL_SRC_FILES := $(libbrillo_core_sources) $(libbrillo_linux_sources)
LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries)
LOCAL_STATIC_LIBRARIES := libmodpb64
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS)
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)

# Static test-helpers library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo-test-helpers
LOCAL_SRC_FILES := $(libbrillo_test_helpers_sources)
LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_STATIC_LIBRARIES := libgtest libgmock
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries) libbrillo libcurl \
    libbrillo-http libbrillo-stream libcrypto
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS) -Wno-sign-compare
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)

# Shared library for host
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo
LOCAL_SRC_FILES := $(libbrillo_core_sources)
LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries)
LOCAL_STATIC_LIBRARIES := libmodpb64-host
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := \
    -D__ANDROID_HOST__ \
    $(libbrillo_CPPFLAGS)
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_HOST_SHARED_LIBRARY)

ifeq ($(HOST_OS),linux)

# Shared stream library for host
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo-stream
LOCAL_SRC_FILES := $(libbrillo_stream_sources)
LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries) libbrillo \
    libcrypto-host libssl-host
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS)
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_HOST_SHARED_LIBRARY)

# Shared http library for host
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo-http
LOCAL_SRC_FILES := $(libbrillo_http_sources)
LOCAL_C_INCLUDES := $(libbrillo_includes)
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries) libbrillo \
    libbrillo-stream libcurl-host
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS)
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_HOST_SHARED_LIBRARY)

endif  # HOST_OS == linux

# Unit tests.
# ========================================================
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := $(libbrillo_cpp_extension)
LOCAL_MODULE := libbrillo_test
LOCAL_MODULE_CLASS := EXECUTABLES
ifdef BRILLO
  LOCAL_MODULE_TAGS := eng
endif
generated_sources_dir := $(call local-generated-sources-dir)
LOCAL_SRC_FILES := $(libbrillo_test_sources)
LOCAL_C_INCLUDES := \
    $(libbrillo_includes) \
    $(generated_sources_dir)/proto/external/libbrillo
LOCAL_STATIC_LIBRARIES := libgtest libchrome_test_helpers \
    libbrillo-test-helpers libgmock libBionicGtestMain
LOCAL_SHARED_LIBRARIES := $(libbrillo_shared_libraries) libbrillo libcurl \
    libbrillo-http libbrillo-stream libcrypto libprotobuf-cpp-lite
ifeq ($(local_use_dbus),1)
LOCAL_SRC_FILES += $(libbrillo_dbus_test_sources)
LOCAL_STATIC_LIBRARIES += libchrome_dbus_test_helpers
LOCAL_SHARED_LIBRARIES += libbrillo-dbus libchrome-dbus libdbus
endif  # local_use_dbus == 1
LOCAL_CFLAGS := $(libbrillo_CFLAGS)
LOCAL_CPPFLAGS := $(libbrillo_CPPFLAGS) -Wno-sign-compare
LOCAL_CLANG := true
include $(BUILD_NATIVE_TEST)

# Run unit tests on target
# ========================================================
# We su shell because process tests try setting "illegal"
# uid/gids and expecting failures, but root can legally
# set those to any value.
runtargettests: libbrillo_test
	adb sync
	adb shell su shell /data/nativetest/libbrillo_test/libbrillo_test
