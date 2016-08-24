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

# Common variables
# ========================================================

# Set libchromeUseClang to "true" to force clang or "false" to force gcc.
libchromeUseClang :=
libchromeCommonCppExtension := .cc
libchromeTestCFlags := -Wno-unused-parameter -Wno-unused-function \
	-Wno-missing-field-initializers
libchromeCommonCFlags := -Wall -Werror
libchromeCommonCIncludes := \
	external/gmock/include \
	external/gtest/include \
	external/valgrind/include \
	external/valgrind \

libchromeExportedCIncludes := $(LOCAL_PATH) $(TOP)/external/gtest/include

libchromeCommonSrc := \
	base/at_exit.cc \
	base/base64.cc \
	base/base64url.cc \
	base/base_switches.cc \
	base/bind_helpers.cc \
	base/build_time.cc \
	base/callback_helpers.cc \
	base/callback_internal.cc \
	base/command_line.cc \
	base/cpu.cc \
	base/debug/alias.cc \
	base/debug/debugger.cc \
	base/debug/debugger_posix.cc \
	base/debug/stack_trace.cc \
	base/debug/stack_trace_posix.cc \
	base/debug/task_annotator.cc \
	base/environment.cc \
	base/files/file.cc \
	base/files/file_enumerator.cc \
	base/files/file_enumerator_posix.cc \
	base/files/file_path.cc \
	base/files/file_path_constants.cc \
	base/files/file_path_watcher.cc \
	base/files/file_posix.cc \
	base/files/file_tracing.cc \
	base/files/file_util.cc \
	base/files/file_util_posix.cc \
	base/files/important_file_writer.cc \
	base/files/scoped_file.cc \
	base/files/scoped_temp_dir.cc \
	base/guid.cc \
	base/guid_posix.cc \
	base/json/json_file_value_serializer.cc \
	base/json/json_parser.cc \
	base/json/json_reader.cc \
	base/json/json_string_value_serializer.cc \
	base/json/json_value_converter.cc \
	base/json/json_writer.cc \
	base/json/string_escape.cc \
	base/lazy_instance.cc \
	base/location.cc \
	base/logging.cc \
	base/md5.cc \
	base/memory/aligned_memory.cc \
	base/memory/ref_counted.cc \
	base/memory/ref_counted_memory.cc \
	base/memory/singleton.cc \
	base/memory/weak_ptr.cc \
	base/message_loop/incoming_task_queue.cc \
	base/message_loop/message_loop.cc \
	base/message_loop/message_loop_task_runner.cc \
	base/message_loop/message_pump.cc \
	base/message_loop/message_pump_default.cc \
	base/message_loop/message_pump_libevent.cc \
	base/metrics/bucket_ranges.cc \
	base/metrics/field_trial.cc \
	base/metrics/metrics_hashes.cc \
	base/metrics/histogram_base.cc \
	base/metrics/histogram.cc \
	base/metrics/histogram_samples.cc \
	base/metrics/histogram_snapshot_manager.cc \
	base/metrics/sample_map.cc \
	base/metrics/sample_vector.cc \
	base/metrics/sparse_histogram.cc \
	base/metrics/statistics_recorder.cc \
	base/pending_task.cc \
	base/pickle.cc \
	base/posix/file_descriptor_shuffle.cc \
	base/posix/safe_strerror.cc \
	base/process/kill.cc \
	base/process/kill_posix.cc \
	base/process/launch.cc \
	base/process/launch_posix.cc \
	base/process/process_handle.cc \
	base/process/process_handle_posix.cc \
	base/process/process_iterator.cc \
	base/process/process_metrics.cc \
	base/process/process_metrics_posix.cc \
	base/process/process_posix.cc \
	base/profiler/alternate_timer.cc \
	base/profiler/scoped_profile.cc \
	base/profiler/scoped_tracker.cc \
	base/profiler/tracked_time.cc \
	base/rand_util.cc \
	base/rand_util_posix.cc \
	base/run_loop.cc \
	base/sequence_checker_impl.cc \
	base/sequenced_task_runner.cc \
	base/sha1_portable.cc \
	base/strings/pattern.cc \
	base/strings/safe_sprintf.cc \
	base/strings/string16.cc \
	base/strings/string_number_conversions.cc \
	base/strings/string_piece.cc \
	base/strings/stringprintf.cc \
	base/strings/string_split.cc \
	base/strings/string_util.cc \
	base/strings/string_util_constants.cc \
	base/strings/utf_string_conversions.cc \
	base/strings/utf_string_conversion_utils.cc \
	base/synchronization/cancellation_flag.cc \
	base/synchronization/condition_variable_posix.cc \
	base/synchronization/lock.cc \
	base/synchronization/lock_impl_posix.cc \
	base/synchronization/waitable_event_posix.cc \
	base/sync_socket_posix.cc \
	base/sys_info.cc \
	base/sys_info_posix.cc \
	base/task/cancelable_task_tracker.cc \
	base/task_runner.cc \
	base/third_party/icu/icu_utf.cc \
	base/third_party/nspr/prtime.cc \
	base/threading/non_thread_safe_impl.cc \
	base/threading/platform_thread_posix.cc \
	base/threading/post_task_and_reply_impl.cc \
	base/threading/sequenced_worker_pool.cc \
	base/threading/simple_thread.cc \
	base/threading/thread.cc \
	base/threading/thread_checker_impl.cc \
	base/threading/thread_collision_warner.cc \
	base/threading/thread_id_name_manager.cc \
	base/threading/thread_local_posix.cc \
	base/threading/thread_local_storage.cc \
	base/threading/thread_local_storage_posix.cc \
	base/threading/thread_restrictions.cc \
	base/threading/worker_pool.cc \
	base/threading/worker_pool_posix.cc \
	base/thread_task_runner_handle.cc \
	base/time/clock.cc \
	base/time/default_clock.cc \
	base/time/default_tick_clock.cc \
	base/time/tick_clock.cc \
	base/time/time.cc \
	base/time/time_posix.cc \
	base/timer/elapsed_timer.cc \
	base/timer/timer.cc \
	base/trace_event/heap_profiler_allocation_context.cc \
	base/trace_event/heap_profiler_allocation_context_tracker.cc \
	base/trace_event/heap_profiler_stack_frame_deduplicator.cc \
	base/trace_event/heap_profiler_type_name_deduplicator.cc \
	base/trace_event/memory_allocator_dump.cc \
	base/trace_event/memory_allocator_dump_guid.cc \
	base/trace_event/memory_dump_manager.cc \
	base/trace_event/malloc_dump_provider.cc \
	base/trace_event/memory_dump_request_args.cc \
	base/trace_event/memory_dump_session_state.cc \
	base/trace_event/process_memory_dump.cc \
	base/trace_event/process_memory_maps.cc \
	base/trace_event/process_memory_maps_dump_provider.cc \
	base/trace_event/process_memory_totals.cc \
	base/trace_event/process_memory_totals_dump_provider.cc \
	base/trace_event/trace_buffer.cc \
	base/trace_event/trace_config.cc \
	base/trace_event/trace_event_argument.cc \
	base/trace_event/trace_event_impl.cc \
	base/trace_event/trace_event_memory_overhead.cc \
	base/trace_event/trace_event_synthetic_delay.cc \
	base/trace_event/trace_log.cc \
	base/trace_event/trace_log_constants.cc \
	base/trace_event/trace_sampling_thread.cc \
	base/tracked_objects.cc \
	base/tracking_info.cc \
	base/values.cc \
	base/vlog.cc \

libchromeLinuxSrc := \
	base/files/file_path_watcher_linux.cc \
	base/files/file_util_linux.cc \
	base/posix/unix_domain_socket_linux.cc \
	base/process/internal_linux.cc \
	base/process/process_handle_linux.cc \
	base/process/process_iterator_linux.cc \
	base/process/process_metrics_linux.cc \
	base/strings/sys_string_conversions_posix.cc \
	base/sys_info_linux.cc \
	base/threading/platform_thread_internal_posix.cc \
	base/threading/platform_thread_linux.cc \
	components/timers/alarm_timer_chromeos.cc \

libchromeMacSrc := \
	base/files/file_path_watcher_fsevents.cc \
	base/files/file_path_watcher_kqueue.cc \
	base/files/file_path_watcher_mac.cc \
	base/files/file_util_mac.mm \
	base/mac/bundle_locations.mm \
	base/mac/foundation_util.mm \
	base/mac/mach_logging.cc \
	base/mac/libdispatch_task_runner.cc \
	base/mac/scoped_mach_port.cc \
	base/mac/scoped_nsautorelease_pool.mm \
	base/message_loop/message_pump_mac.mm \
	base/process/launch_mac.cc \
	base/process/port_provider_mac.cc \
	base/process/process_handle_mac.cc \
	base/process/process_iterator_mac.cc \
	base/process/process_metrics_mac.cc \
	base/strings/sys_string_conversions_mac.mm \
	base/sys_info_mac.cc \
	base/time/time_mac.cc \
	base/threading/platform_thread_mac.mm \

libchromeCommonUnittestSrc := \
	base/at_exit_unittest.cc \
	base/atomicops_unittest.cc \
	base/base64_unittest.cc \
	base/base64url_unittest.cc \
	base/bind_unittest.cc \
	base/bits_unittest.cc \
	base/build_time_unittest.cc \
	base/callback_helpers_unittest.cc \
	base/callback_list_unittest.cc \
	base/callback_unittest.cc \
	base/cancelable_callback_unittest.cc \
	base/command_line_unittest.cc \
	base/cpu_unittest.cc \
	base/debug/debugger_unittest.cc \
	base/debug/leak_tracker_unittest.cc \
	base/debug/task_annotator_unittest.cc \
	base/environment_unittest.cc \
	base/file_version_info_unittest.cc \
	base/files/dir_reader_posix_unittest.cc \
	base/files/file_path_watcher_unittest.cc \
	base/files/file_path_unittest.cc \
	base/files/file_unittest.cc \
	base/files/important_file_writer_unittest.cc \
	base/files/scoped_temp_dir_unittest.cc \
	base/gmock_unittest.cc \
	base/guid_unittest.cc \
	base/id_map_unittest.cc \
	base/json/json_parser_unittest.cc \
	base/json/json_reader_unittest.cc \
	base/json/json_value_converter_unittest.cc \
	base/json/json_value_serializer_unittest.cc \
	base/json/json_writer_unittest.cc \
	base/json/string_escape_unittest.cc \
	base/lazy_instance_unittest.cc \
	base/logging_unittest.cc \
	base/md5_unittest.cc \
	base/memory/aligned_memory_unittest.cc \
	base/memory/linked_ptr_unittest.cc \
	base/memory/ref_counted_memory_unittest.cc \
	base/memory/ref_counted_unittest.cc \
	base/memory/scoped_ptr_unittest.cc \
	base/memory/scoped_vector_unittest.cc \
	base/memory/singleton_unittest.cc \
	base/memory/weak_ptr_unittest.cc \
	base/message_loop/message_loop_test.cc \
	base/message_loop/message_loop_task_runner_unittest.cc \
	base/message_loop/message_loop_unittest.cc \
	base/metrics/bucket_ranges_unittest.cc \
	base/metrics/field_trial_unittest.cc \
	base/metrics/metrics_hashes_unittest.cc \
	base/metrics/histogram_base_unittest.cc \
	base/metrics/histogram_macros_unittest.cc \
	base/metrics/histogram_snapshot_manager_unittest.cc \
	base/metrics/histogram_unittest.cc \
	base/metrics/sample_map_unittest.cc \
	base/metrics/sample_vector_unittest.cc \
	base/metrics/sparse_histogram_unittest.cc \
	base/metrics/statistics_recorder_unittest.cc \
	base/numerics/safe_numerics_unittest.cc \
	base/observer_list_unittest.cc \
	base/pickle_unittest.cc \
	base/posix/file_descriptor_shuffle_unittest.cc \
	base/posix/unix_domain_socket_linux_unittest.cc \
	base/process/process_metrics_unittest.cc \
	base/profiler/tracked_time_unittest.cc \
	base/rand_util_unittest.cc \
	base/scoped_clear_errno_unittest.cc \
	base/scoped_generic_unittest.cc \
	base/security_unittest.cc \
	base/sequence_checker_unittest.cc \
	base/sha1_unittest.cc \
	base/stl_util_unittest.cc \
	base/strings/pattern_unittest.cc \
	base/strings/string16_unittest.cc \
	base/strings/string_number_conversions_unittest.cc \
	base/strings/string_piece_unittest.cc \
	base/strings/stringprintf_unittest.cc \
	base/strings/string_split_unittest.cc \
	base/strings/string_util_unittest.cc \
	base/strings/sys_string_conversions_unittest.cc \
	base/strings/utf_string_conversions_unittest.cc \
	base/synchronization/cancellation_flag_unittest.cc \
	base/synchronization/condition_variable_unittest.cc \
	base/synchronization/lock_unittest.cc \
	base/synchronization/waitable_event_unittest.cc \
	base/sync_socket_unittest.cc \
	base/sys_info_unittest.cc \
	base/task/cancelable_task_tracker_unittest.cc \
	base/task_runner_util_unittest.cc \
	base/template_util_unittest.cc \
	base/test/multiprocess_test.cc \
	base/test/multiprocess_test_android.cc \
	base/test/opaque_ref_counted.cc \
	base/test/scoped_locale.cc \
	base/test/sequenced_worker_pool_owner.cc \
	base/test/test_file_util.cc \
	base/test/test_file_util_linux.cc \
	base/test/test_file_util_posix.cc \
	base/test/test_io_thread.cc \
	base/test/test_pending_task.cc \
	base/test/test_simple_task_runner.cc \
	base/test/test_switches.cc \
	base/test/test_timeouts.cc \
	base/test/trace_event_analyzer.cc \
	base/threading/non_thread_safe_unittest.cc \
	base/threading/platform_thread_unittest.cc \
	base/threading/simple_thread_unittest.cc \
	base/threading/thread_checker_unittest.cc \
	base/threading/thread_collision_warner_unittest.cc \
	base/threading/thread_id_name_manager_unittest.cc \
	base/threading/thread_local_storage_unittest.cc \
	base/threading/thread_local_unittest.cc \
	base/threading/thread_unittest.cc \
	base/threading/worker_pool_posix_unittest.cc \
	base/threading/worker_pool_unittest.cc \
	base/time/pr_time_unittest.cc \
	base/time/time_unittest.cc \
	base/timer/hi_res_timer_manager_unittest.cc \
	base/timer/timer_unittest.cc \
	base/trace_event/heap_profiler_allocation_context_tracker_unittest.cc \
	base/trace_event/heap_profiler_stack_frame_deduplicator_unittest.cc \
	base/trace_event/heap_profiler_type_name_deduplicator_unittest.cc \
	base/trace_event/memory_allocator_dump_unittest.cc \
	base/trace_event/memory_dump_manager_unittest.cc \
	base/trace_event/process_memory_dump_unittest.cc \
	base/trace_event/process_memory_maps_dump_provider_unittest.cc \
	base/trace_event/process_memory_totals_dump_provider_unittest.cc \
	base/trace_event/trace_config_unittest.cc \
	base/trace_event/trace_event_argument_unittest.cc \
	base/trace_event/trace_event_synthetic_delay_unittest.cc \
	base/trace_event/trace_event_unittest.cc \
	base/tracked_objects_unittest.cc \
	base/tuple_unittest.cc \
	base/values_unittest.cc \
	base/vlog_unittest.cc \
	testing/multiprocess_func_list.cc \
	testrunner.cc \

libchromeCryptoUnittestSrc := \
	crypto/secure_hash_unittest.cc \
	crypto/sha2_unittest.cc \

libchromeHostCFlags := -D__ANDROID_HOST__

ifeq ($(HOST_OS),linux)
libchromeHostSrc := $(libchromeLinuxSrc)
libchromeHostLdFlags :=
endif

ifeq ($(HOST_OS),darwin)
libchromeHostSrc := $(libchromeMacSrc)
libchromeHostCFlags += -D_FILE_OFFSET_BITS=64 -Wno-deprecated-declarations
libchromeHostLdFlags := \
	-framework AppKit \
	-framework CoreFoundation \
	-framework Foundation \
	-framework Security
endif

# libchrome shared library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libchrome
LOCAL_SRC_FILES := $(libchromeCommonSrc) $(libchromeLinuxSrc) base/sys_info_chromeos.cc
LOCAL_CPP_EXTENSION := $(libchromeCommonCppExtension)
LOCAL_CFLAGS := $(libchromeCommonCFlags)
LOCAL_CLANG := $(libchromeUseClang)
LOCAL_C_INCLUDES := $(libchromeCommonCIncludes)
LOCAL_SHARED_LIBRARIES := libevent liblog libcutils
LOCAL_STATIC_LIBRARIES := libmodpb64
LOCAL_EXPORT_C_INCLUDE_DIRS := $(libchromeExportedCIncludes)
include $(BUILD_SHARED_LIBRARY)

# libchrome shared library for host
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libchrome
LOCAL_CFLAGS := $(libchromeCommonCFlags) $(libchromeHostCFlags)
LOCAL_CLANG := $(libchromeUseClang)
LOCAL_CPP_EXTENSION := $(libchromeCommonCppExtension)
LOCAL_C_INCLUDES := $(libchromeCommonCIncludes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(libchromeExportedCIncludes)
LOCAL_SHARED_LIBRARIES := libevent-host
LOCAL_STATIC_LIBRARIES := libmodpb64-host
LOCAL_SRC_FILES := $(libchromeCommonSrc) $(libchromeHostSrc)
LOCAL_LDFLAGS := $(libchromeHostLdFlags)
include $(BUILD_HOST_SHARED_LIBRARY)

ifeq ($(local_use_dbus),1)

# libchrome-dbus shared library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libchrome-dbus
LOCAL_SRC_FILES := \
	dbus/bus.cc \
	dbus/dbus_statistics.cc \
	dbus/exported_object.cc \
	dbus/file_descriptor.cc \
	dbus/message.cc \
	dbus/object_manager.cc \
	dbus/object_path.cc \
	dbus/object_proxy.cc \
	dbus/property.cc \
	dbus/scoped_dbus_error.cc \
	dbus/string_util.cc \
	dbus/util.cc \
	dbus/values_util.cc \

LOCAL_CPP_EXTENSION := $(libchromeCommonCppExtension)
LOCAL_CFLAGS := $(libchromeCommonCFlags)
LOCAL_CLANG := $(libchromeUseClang)
LOCAL_C_INCLUDES := $(libchromeCommonCIncludes)
LOCAL_SHARED_LIBRARIES := \
	libchrome \
	libdbus \
	libprotobuf-cpp-lite \

LOCAL_STATIC_LIBRARIES :=
LOCAL_EXPORT_C_INCLUDE_DIRS := $(libchromeExportedCIncludes)
include $(BUILD_SHARED_LIBRARY)

endif  # local_use_dbus == 1

# libchrome-crypto shared library for target
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libchrome-crypto
LOCAL_SRC_FILES := \
	crypto/openssl_util.cc \
	crypto/secure_hash_openssl.cc \
	crypto/secure_util.cc \
	crypto/sha2.cc \

LOCAL_CPP_EXTENSION := $(libchromeCommonCppExtension)
LOCAL_CFLAGS := $(libchromeCommonCFlags) -Wno-unused-parameter
LOCAL_CPPFLAGS := $(libchromeCommonCppFlags)
LOCAL_CLANG := $(libchromeUseClang)
LOCAL_C_INCLUDES := $(libchromeCommonCIncludes)
LOCAL_SHARED_LIBRARIES := \
	libchrome \
	libcrypto \
	libssl \

LOCAL_STATIC_LIBRARIES :=
LOCAL_EXPORT_C_INCLUDE_DIRS := $(libchromeExportedCIncludes)
include $(BUILD_SHARED_LIBRARY)

# Helpers needed for unit tests.
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libchrome_test_helpers
LOCAL_SHARED_LIBRARIES := libchrome
LOCAL_CPP_EXTENSION := $(libchromeCommonCppExtension)
LOCAL_CFLAGS := $(libchromeCommonCFlags) $(libchromeTestCFlags)
LOCAL_CLANG := $(libchromeUseClang)
LOCAL_C_INCLUDES := $(libchromeCommonCIncludes)
LOCAL_SRC_FILES := \
	base/test/simple_test_clock.cc \
	base/test/simple_test_tick_clock.cc \
	base/test/test_file_util.cc \
	base/test/test_file_util_linux.cc \
	base/test/test_switches.cc \
	base/test/test_timeouts.cc \

include $(BUILD_STATIC_LIBRARY)

ifeq ($(local_use_dbus),1)

# Helpers needed for D-Bus unit tests.
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libchrome_dbus_test_helpers
LOCAL_SHARED_LIBRARIES := libdbus libchrome-dbus
LOCAL_CPP_EXTENSION := $(libchromeCommonCppExtension)
LOCAL_CFLAGS := $(libchromeCommonCFlags) $(libchromeTestCFlags)
LOCAL_CLANG := $(libchromeUseClang)
LOCAL_C_INCLUDES := $(libchromeCommonCIncludes)
LOCAL_SRC_FILES := \
	dbus/mock_bus.cc \
	dbus/mock_exported_object.cc \
	dbus/mock_object_manager.cc \
	dbus/mock_object_proxy.cc \

include $(BUILD_STATIC_LIBRARY)

endif  # local_use_dbus == 1

# Helpers needed for unit tests (for host).
# ========================================================
ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_MODULE := libchrome_test_helpers-host
LOCAL_SHARED_LIBRARIES := libchrome
LOCAL_CPP_EXTENSION := $(libchromeCommonCppExtension)
LOCAL_CFLAGS := $(libchromeCommonCFlags) $(libchromeTestCFlags)
LOCAL_CLANG := $(libchromeUseClang)
LOCAL_C_INCLUDES := $(libchromeCommonCIncludes)
LOCAL_SRC_FILES := base/test/simple_test_clock.cc
include $(BUILD_HOST_STATIC_LIBRARY)

# Host unit tests. Run (from repo root) with:
# ./out/host/<arch>/bin/libchrome_test
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libchrome_test
ifdef BRILLO
  LOCAL_MODULE_TAGS := debug
endif
LOCAL_SRC_FILES := $(libchromeCommonUnittestSrc)
LOCAL_CPP_EXTENSION := $(libchromeCommonCppExtension)
LOCAL_CFLAGS := $(libchromeCommonCFlags) $(libchromeTestCFlags) $(libchromeHostCFlags) -DUNIT_TEST
LOCAL_CLANG := $(libchromeUseClang)
LOCAL_C_INCLUDES := $(libchromeCommonCIncludes)
LOCAL_SHARED_LIBRARIES := libchrome libevent-host
LOCAL_STATIC_LIBRARIES := libgmock_host libgtest_host
LOCAL_LDLIBS := -lrt
include $(BUILD_HOST_NATIVE_TEST)
endif

# Native unit tests. Run with:
# adb shell /data/nativetest/libchrome_test/libchrome_test
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libchrome_test
ifdef BRILLO
  LOCAL_MODULE_TAGS := eng
endif
LOCAL_SRC_FILES := $(libchromeCryptoUnittestSrc) $(libchromeCommonUnittestSrc)
LOCAL_CPP_EXTENSION := $(libchromeCommonCppExtension)
LOCAL_CFLAGS := $(libchromeCommonCFlags) $(libchromeTestCFlags) -DUNIT_TEST -DDONT_EMBED_BUILD_METADATA
LOCAL_CLANG := $(libchromeUseClang)
LOCAL_C_INCLUDES := $(libchromeCommonCIncludes)
LOCAL_SHARED_LIBRARIES := libchrome libchrome-crypto libevent
LOCAL_STATIC_LIBRARIES := libgmock libgtest
include $(BUILD_NATIVE_TEST)
