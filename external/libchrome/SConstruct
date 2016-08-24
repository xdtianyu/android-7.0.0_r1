# -*- python -*-

# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file is used to build the libchrome package for Chrome OS:
# https://www.chromium.org/chromium-os/packages/libchrome

import os

env = Environment()

BASE_VER = os.environ.get('BASE_VER', '0')
PKG_CONFIG = os.environ.get('PKG_CONFIG', 'pkg-config')
CHROME_INCLUDE_PATH = os.environ.get('CHROME_INCLUDE_PATH', '.')

# This block will need updating whenever libchrome gets updated. The order of
# the libs below doesn't matter (as scons will take care of building things in
# the required order).  The split between them is purely to reduce excess
# linking of third-party libraries, i.e. 'core' should require only a minimal
# set of libraries, and other third-party libraries should get a unique 'xxx'
# name.
base_name = 'base'
base_libs = [
  {
    'name' : 'core',
    'sources' : """
                allocator/allocator_extension.cc
                at_exit.cc
                base64.cc
                base64url.cc
                base_switches.cc
                bind_helpers.cc
                build_time.cc
                callback_helpers.cc
                callback_internal.cc
                command_line.cc
                cpu.cc
                debug/alias.cc
                debug/debugger.cc
                debug/debugger_posix.cc
                debug/stack_trace.cc
                debug/stack_trace_posix.cc
                debug/task_annotator.cc
                environment.cc
                files/file.cc
                files/file_enumerator.cc
                files/file_enumerator_posix.cc
                files/file_path.cc
                files/file_path_constants.cc
                files/file_path_watcher.cc
                files/file_path_watcher_linux.cc
                files/file_posix.cc
                files/file_tracing.cc
                files/file_util.cc
                files/file_util_linux.cc
                files/file_util_posix.cc
                files/important_file_writer.cc
                files/memory_mapped_file.cc
                files/memory_mapped_file_posix.cc
                files/scoped_file.cc
                files/scoped_temp_dir.cc
                guid.cc
                guid_posix.cc
                json/json_file_value_serializer.cc
                json/json_parser.cc
                json/json_reader.cc
                json/json_string_value_serializer.cc
                json/json_value_converter.cc
                json/json_writer.cc
                json/string_escape.cc
                lazy_instance.cc
                location.cc
                logging.cc
                md5.cc
                memory/ref_counted.cc
                memory/ref_counted_memory.cc
                memory/singleton.cc
                memory/weak_ptr.cc
                message_loop/incoming_task_queue.cc
                message_loop/message_loop.cc
                message_loop/message_loop_task_runner.cc
                message_loop/message_pump.cc
                message_loop/message_pump_default.cc
                message_loop/message_pump_glib.cc
                message_loop/message_pump_libevent.cc
                metrics/bucket_ranges.cc
                metrics/field_trial.cc
                metrics/metrics_hashes.cc
                metrics/histogram_base.cc
                metrics/histogram.cc
                metrics/histogram_samples.cc
                metrics/histogram_snapshot_manager.cc
                metrics/sample_map.cc
                metrics/sample_vector.cc
                metrics/sparse_histogram.cc
                metrics/statistics_recorder.cc
                pending_task.cc
                pickle.cc
                posix/file_descriptor_shuffle.cc
                posix/global_descriptors.cc
                posix/safe_strerror.cc
                posix/unix_domain_socket_linux.cc
                process/internal_linux.cc
                process/kill.cc
                process/kill_posix.cc
                process/launch.cc
                process/launch_posix.cc
                process/process_handle_linux.cc
                process/process_iterator.cc
                process/process_iterator_linux.cc
                process/process_handle_posix.cc
                process/process_metrics.cc
                process/process_metrics_linux.cc
                process/process_metrics_posix.cc
                process/process_posix.cc
                profiler/alternate_timer.cc
                profiler/scoped_profile.cc
                profiler/scoped_tracker.cc
                profiler/tracked_time.cc
                rand_util.cc
                rand_util_posix.cc
                run_loop.cc
                sequence_checker_impl.cc
                sequenced_task_runner.cc
                sha1_portable.cc
                strings/pattern.cc
                strings/safe_sprintf.cc
                strings/string16.cc
                strings/string_number_conversions.cc
                strings/string_piece.cc
                strings/stringprintf.cc
                strings/string_split.cc
                strings/string_util.cc
                strings/string_util_constants.cc
                strings/sys_string_conversions_posix.cc
                strings/utf_string_conversions.cc
                strings/utf_string_conversion_utils.cc
                synchronization/cancellation_flag.cc
                synchronization/condition_variable_posix.cc
                synchronization/lock.cc
                synchronization/lock_impl_posix.cc
                synchronization/waitable_event_posix.cc
                synchronization/waitable_event_watcher_posix.cc
                sync_socket_posix.cc
                sys_info.cc
                sys_info_chromeos.cc
                sys_info_linux.cc
                sys_info_posix.cc
                task_runner.cc
                task/cancelable_task_tracker.cc
                third_party/icu/icu_utf.cc
                third_party/nspr/prtime.cc
                threading/non_thread_safe_impl.cc
                threading/platform_thread_internal_posix.cc
                threading/platform_thread_linux.cc
                threading/platform_thread_posix.cc
                threading/post_task_and_reply_impl.cc
                threading/sequenced_worker_pool.cc
                threading/simple_thread.cc
                threading/thread.cc
                threading/thread_checker_impl.cc
                threading/thread_collision_warner.cc
                threading/thread_id_name_manager.cc
                threading/thread_local_posix.cc
                threading/thread_local_storage.cc
                threading/thread_local_storage_posix.cc
                threading/thread_restrictions.cc
                threading/worker_pool.cc
                threading/worker_pool_posix.cc
                thread_task_runner_handle.cc
                timer/elapsed_timer.cc
                timer/timer.cc
                time/clock.cc
                time/default_clock.cc
                time/default_tick_clock.cc
                time/tick_clock.cc
                time/time.cc
                time/time_posix.cc
                trace_event/malloc_dump_provider.cc
                trace_event/heap_profiler_allocation_context.cc
                trace_event/heap_profiler_allocation_context_tracker.cc
                trace_event/heap_profiler_stack_frame_deduplicator.cc
                trace_event/heap_profiler_type_name_deduplicator.cc
                trace_event/memory_allocator_dump.cc
                trace_event/memory_allocator_dump_guid.cc
                trace_event/memory_dump_manager.cc
                trace_event/memory_dump_request_args.cc
                trace_event/memory_dump_session_state.cc
                trace_event/process_memory_dump.cc
                trace_event/process_memory_maps.cc
                trace_event/process_memory_maps_dump_provider.cc
                trace_event/process_memory_totals.cc
                trace_event/process_memory_totals_dump_provider.cc
                trace_event/trace_buffer.cc
                trace_event/trace_config.cc
                trace_event/trace_event_argument.cc
                trace_event/trace_event_impl.cc
                trace_event/trace_event_memory_overhead.cc
                trace_event/trace_event_synthetic_delay.cc
                trace_event/trace_log.cc
                trace_event/trace_log_constants.cc
                trace_event/trace_sampling_thread.cc
                tracked_objects.cc
                tracking_info.cc
                values.cc
                vlog.cc
                """,
    'prefix' : 'base',
    'libs' : 'pthread rt libmodp_b64',
    'pc_libs' : 'glib-2.0 libevent',
  },
  {
    'name' : 'dl',
    'sources' : """
                native_library_posix.cc
                """,
    'prefix' : 'base',
    'libs' : 'dl',
    'pc_libs' : '',
  },
  {
    'name' : 'dbus',
    'sources' : """
                bus.cc
                dbus_statistics.cc
                exported_object.cc
                file_descriptor.cc
                message.cc
                object_manager.cc
                object_path.cc
                object_proxy.cc
                property.cc
                scoped_dbus_error.cc
                string_util.cc
                util.cc
                values_util.cc
                """,
    'prefix' : 'dbus',
    'libs' : '',
    'pc_libs' : 'dbus-1 protobuf-lite',
  },
  {
    'name' : 'timers',
    'sources' : """
                alarm_timer_chromeos.cc
                """,
    'prefix' : 'components/timers',
    'libs' : '',
    'pc_libs' : '',
  },
  {
    'name' : 'crypto',
    'sources' : """
                hmac.cc
                hmac_nss.cc
                nss_key_util.cc
                nss_util.cc
                p224.cc
                p224_spake.cc
                random.cc
                rsa_private_key.cc
                rsa_private_key_nss.cc
                scoped_test_nss_db.cc
                secure_hash_default.cc
                secure_util.cc
                sha2.cc
                signature_creator_nss.cc
                signature_verifier_nss.cc
                symmetric_key_nss.cc
                third_party/nss/rsawrapr.c
                third_party/nss/sha512.cc
                """,
    'prefix' : 'crypto',
    'libs' : '%s-dl-%s' % (base_name, BASE_VER),
    'pc_libs' : 'nss',
  },
  {
    'name' : 'sandbox',
    'sources' : """
                linux/bpf_dsl/bpf_dsl.cc
                linux/bpf_dsl/codegen.cc
                linux/bpf_dsl/dump_bpf.cc
                linux/bpf_dsl/policy.cc
                linux/bpf_dsl/policy_compiler.cc
                linux/bpf_dsl/syscall_set.cc
                linux/bpf_dsl/verifier.cc
                linux/seccomp-bpf/die.cc
                linux/seccomp-bpf/sandbox_bpf.cc
                linux/seccomp-bpf/syscall.cc
                linux/seccomp-bpf/trap.cc

                linux/seccomp-bpf-helpers/baseline_policy.cc
                linux/seccomp-bpf-helpers/sigsys_handlers.cc
                linux/seccomp-bpf-helpers/syscall_parameters_restrictions.cc
                linux/seccomp-bpf-helpers/syscall_sets.cc

                linux/services/init_process_reaper.cc
                linux/services/proc_util.cc
                linux/services/resource_limits.cc
                linux/services/scoped_process.cc
                linux/services/syscall_wrappers.cc
                linux/services/thread_helpers.cc
                linux/services/yama.cc
                linux/syscall_broker/broker_channel.cc
                linux/syscall_broker/broker_client.cc
                linux/syscall_broker/broker_file_permission.cc
                linux/syscall_broker/broker_host.cc
                linux/syscall_broker/broker_policy.cc
                linux/syscall_broker/broker_process.cc

                linux/services/credentials.cc
                linux/services/namespace_sandbox.cc
                linux/services/namespace_utils.cc
                """,
    'prefix' : 'sandbox',
    'libs' : '',
    'pc_libs' : '',
  },
]

env.Append(
  CPPPATH=['files'],
  CCFLAGS=['-g']
)
for key in Split('CC CXX AR RANLIB LD NM CFLAGS CXXFLAGS LDFLAGS'):
  value = os.environ.get(key)
  if value:
    env[key] = Split(value)
if os.environ.has_key('CPPFLAGS'):
  env['CCFLAGS'] += Split(os.environ['CPPFLAGS'])

env['CCFLAGS'] += ['-DOS_CHROMEOS',
                   '-DUSE_NSS_CERTS',
                   '-DUSE_SYSTEM_LIBEVENT',
                   '-fPIC',
                   '-fno-exceptions',
                   '-Wall',
                   '-Werror',
                   '-Wno-deprecated-register',
                   '-Wno-narrowing',
                   '-Wno-psabi',
                   '-Wno-unused-local-typedefs',
                   # Various #defines are hardcoded near the top of
                   # build_config.h to ensure that they'll be set both when
                   # libchrome is built and when other packages include
                   # libchrome's headers.
                   '-I%s' % CHROME_INCLUDE_PATH]

env.Append(
  CXXFLAGS=['-std=c++11']
)

# Flags for clang taken from build/common.gypi in the clang==1 section.
CLANG_FLAGS = (
  '-Wno-char-subscripts',
)

env['CCFLAGS'] += ['-Xclang-only=%s' % x for x in CLANG_FLAGS]

# Fix issue with scons not passing some vars through the environment.
for key in Split('PKG_CONFIG SYSROOT'):
  if os.environ.has_key(key):
    env['ENV'][key] = os.environ[key]

all_base_libs = []
all_pc_libs = ''
all_libs = []
all_scons_libs = []

# Build all the shared libraries.
for lib in base_libs:
  pc_libs = lib['pc_libs'].replace('${bslot}', BASE_VER)
  all_pc_libs += ' ' + pc_libs

  libs = Split(lib['libs'].replace('${bslot}', BASE_VER))
  all_libs += libs

  name = '%s-%s-%s' % (base_name, lib['name'], BASE_VER)
  all_base_libs += [name]
  corename = '%s-core-%s' % (base_name, BASE_VER)
  # Automatically link the sub-libs against the main core lib.
  # This is to keep from having to explicitly mention it in the
  # table above (i.e. lazy).
  if name != corename:
    libs += [corename]

  e = env.Clone()
  e.Append(
    LIBS = Split(libs),
    LIBPATH = ['.'],
    LINKFLAGS = ['-Wl,--as-needed', '-Wl,-z,defs',
                 '-Wl,-soname,lib%s.so' % name],
  )
  if pc_libs:
    e.ParseConfig(PKG_CONFIG + ' --cflags --libs %s' % pc_libs)

  # Prepend prefix to source filenames.
  sources = [os.path.join(lib['prefix'], x) for x in Split(lib['sources'])]

  all_scons_libs += [ e.SharedLibrary(name, sources) ]


# Build a static library of mocks for unittests to link against.
# Being static allows us to mask this library out of the image.

all_base_test_libs = []
all_test_pc_libs = ''
all_test_libs = []

test_libs = [
  {
    'name': 'base_test_support',
    'sources': """
               simple_test_clock.cc
               simple_test_tick_clock.cc
               test_file_util.cc
               test_file_util_linux.cc
               test_switches.cc
               test_timeouts.cc
               """,
    'prefix': 'base/test',
    'libs': '',
    'pc_libs': '',
  },
  {
    'name': 'dbus_test_support',
    'sources': """
               mock_bus.cc
               mock_exported_object.cc
               mock_object_manager.cc
               mock_object_proxy.cc
               """,
    'prefix': 'dbus',
    'libs': '',  # TODO(wiley) what should go here?
    'pc_libs': 'dbus-1 protobuf-lite',
  },
  {
    'name': 'timer_test_support',
    'sources': """
               mock_timer.cc
               """,
    'prefix': 'base/timer',
    'libs': '',
    'pc_libs': '',
  },
]

for lib in test_libs:
  pc_libs = lib['pc_libs'].replace('${bslot}', BASE_VER)
  all_test_pc_libs += ' ' + pc_libs

  libs = Split(lib['libs'].replace('${bslot}', BASE_VER))
  all_test_libs += libs

  name = '%s-%s-%s' % (base_name, lib['name'], BASE_VER)
  all_base_test_libs += [name]

  static_env = env.Clone()
  if pc_libs:
    static_env.ParseConfig(PKG_CONFIG + ' --cflags --libs %s' % pc_libs)
  sources = [os.path.join(lib['prefix'], x)
             for x in Split(lib['sources'])]
  static_env.StaticLibrary(name, sources)

# Build the random text files (pkg-config and linker script).

def lib_list(libs):
  return ' '.join(['-l' + l for l in libs])

prod_subst_dict = {
  '@BSLOT@': BASE_VER,
  '@PRIVATE_PC@': all_pc_libs,
  '@BASE_LIBS@': lib_list(all_base_libs),
  '@LIBS@': lib_list(all_libs),
  '@NAME@': 'libchrome',
  '@PKG_CFG_NAME@': 'libchrome-%s.pc' % BASE_VER,
  '@LIB_NAME@': 'libbase-%s.so' % BASE_VER,
  '@DESCRIPTION@': 'chrome base library',
  # scons, in its infinite wisdom sees fit to expand this string if
  # if we don't escape the $.
  '@TARGET_LIB@': 'base-$${bslot}',
}

# Similarly, build text files related to the test libraries.
test_subst_dict = {
  '@BSLOT@': BASE_VER,
  '@PRIVATE_PC@': all_test_pc_libs,
  '@BASE_LIBS@': lib_list(all_base_test_libs),
  '@LIBS@': lib_list(all_test_libs),
  '@NAME@': 'libchrome-test',
  '@PKG_CFG_NAME@': 'libchrome-test-%s.pc' % BASE_VER,
  '@LIB_NAME@': 'libbase-test-%s.a' % BASE_VER,
  '@DESCRIPTION@': 'chrome base test library',
  # scons, in its infinite wisdom sees fit to expand this string if
  # if we don't escape the $.
  '@TARGET_LIB@': 'base-test-$${bslot}',
}

pc_file_contents = """
prefix=/usr
includedir=${prefix}/include
bslot=@BSLOT@

Name: @NAME@
Description: @DESCRIPTION@
Version: ${bslot}
Requires:
Requires.private: @PRIVATE_PC@
Libs: -l@TARGET_LIB@
Libs.private: @BASE_LIBS@ @LIBS@
Cflags: -I${includedir}/@TARGET_LIB@ -Wno-c++11-extensions -Wno-unused-local-typedefs -DBASE_VER=${bslot}
"""

# https://sourceware.org/binutils/docs/ld/Scripts.html
so_file_contents = """GROUP ( AS_NEEDED ( @BASE_LIBS@ ) )"""

for subst_dict in (test_subst_dict, prod_subst_dict):
  env = Environment(tools=['textfile'], SUBST_DICT=subst_dict)
  env.Substfile(subst_dict['@LIB_NAME@'], [Value(so_file_contents)])
  env.Substfile(subst_dict['@PKG_CFG_NAME@'], [Value(pc_file_contents)])
