#
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
#

LOCAL_PATH := $(my-dir)

# Default values for the USE flags. Override these USE flags from your product
# by setting BRILLO_USE_* values. Note that we define local variables like
# local_use_* to prevent leaking our default setting for other packages.
local_use_binder := $(if $(BRILLO_USE_BINDER),$(BRILLO_USE_BINDER),1)
local_use_dbus := $(if $(BRILLO_USE_DBUS),$(BRILLO_USE_DBUS),0)
local_use_hwid_override := \
    $(if $(BRILLO_USE_HWID_OVERRIDE),$(BRILLO_USE_HWID_OVERRIDE),0)
# "libcros" gates the LibCrosService exposed by the Chrome OS' chrome browser to
# the system layer.
local_use_libcros := $(if $(BRILLO_USE_LIBCROS),$(BRILLO_USE_LIBCROS),0)
local_use_mtd := $(if $(BRILLO_USE_MTD),$(BRILLO_USE_MTD),0)
local_use_power_management := \
    $(if $(BRILLO_USE_POWER_MANAGEMENT),$(BRILLO_USE_POWER_MANAGEMENT),0)
local_use_weave := $(if $(BRILLO_USE_WEAVE),$(BRILLO_USE_WEAVE),0)

ue_common_cflags := \
    -DUSE_BINDER=$(local_use_binder) \
    -DUSE_DBUS=$(local_use_dbus) \
    -DUSE_HWID_OVERRIDE=$(local_use_hwid_override) \
    -DUSE_LIBCROS=$(local_use_libcros) \
    -DUSE_MTD=$(local_use_mtd) \
    -DUSE_POWER_MANAGEMENT=$(local_use_power_management) \
    -DUSE_WEAVE=$(local_use_weave) \
    -D_FILE_OFFSET_BITS=64 \
    -D_POSIX_C_SOURCE=199309L \
    -Wa,--noexecstack \
    -Wall \
    -Werror \
    -Wextra \
    -Wformat=2 \
    -Wno-psabi \
    -Wno-unused-parameter \
    -ffunction-sections \
    -fstack-protector-strong \
    -fvisibility=hidden
ue_common_cppflags := \
    -Wnon-virtual-dtor \
    -fno-strict-aliasing \
    -std=gnu++11
ue_common_ldflags := \
    -Wl,--gc-sections
ue_common_c_includes := \
    $(LOCAL_PATH)/client_library/include \
    external/gtest/include \
    system
ue_common_shared_libraries := \
    libbrillo \
    libbrillo-http \
    libbrillo-stream \
    libchrome

ifeq ($(local_use_dbus),1)

# update_engine_client-dbus-proxies (from generate-dbus-proxies.gypi)
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := update_engine_client-dbus-proxies
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_SRC_FILES := \
    dbus_bindings/dbus-service-config.json \
    dbus_bindings/org.chromium.UpdateEngineInterface.dbus-xml
LOCAL_DBUS_PROXY_PREFIX := update_engine
include $(BUILD_STATIC_LIBRARY)

endif  # local_use_dbus == 1

# update_metadata-protos (type: static_library)
# ========================================================
# Protobufs.
ue_update_metadata_protos_exported_static_libraries := \
    update_metadata-protos
ue_update_metadata_protos_exported_shared_libraries := \
    libprotobuf-cpp-lite

ue_update_metadata_protos_src_files := \
    update_metadata.proto

# Build for the host.
include $(CLEAR_VARS)
LOCAL_MODULE := update_metadata-protos
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_IS_HOST_MODULE := true
generated_sources_dir := $(call local-generated-sources-dir)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(generated_sources_dir)/proto/system
LOCAL_SRC_FILES := $(ue_update_metadata_protos_src_files)
include $(BUILD_HOST_STATIC_LIBRARY)

# Build for the target.
include $(CLEAR_VARS)
LOCAL_MODULE := update_metadata-protos
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
generated_sources_dir := $(call local-generated-sources-dir)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(generated_sources_dir)/proto/system
LOCAL_SRC_FILES := $(ue_update_metadata_protos_src_files)
include $(BUILD_STATIC_LIBRARY)

ifeq ($(local_use_dbus),1)

# update_engine-dbus-adaptor (from generate-dbus-adaptors.gypi)
# ========================================================
# Chrome D-Bus bindings.
include $(CLEAR_VARS)
LOCAL_MODULE := update_engine-dbus-adaptor
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_SRC_FILES := \
    dbus_bindings/org.chromium.UpdateEngineInterface.dbus-xml
include $(BUILD_STATIC_LIBRARY)

# update_engine-dbus-libcros-client (from generate-dbus-proxies.gypi)
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := update_engine-dbus-libcros-client
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_SRC_FILES := \
    dbus_bindings/org.chromium.LibCrosService.dbus-xml
LOCAL_DBUS_PROXY_PREFIX := libcros
include $(BUILD_STATIC_LIBRARY)

endif  # local_use_dbus == 1

# libpayload_consumer (type: static_library)
# ========================================================
# The payload application component and common dependencies.
ue_libpayload_consumer_exported_static_libraries := \
    update_metadata-protos \
    libxz-host \
    libbz \
    $(ue_update_metadata_protos_exported_static_libraries)
ue_libpayload_consumer_exported_shared_libraries := \
    libcrypto-host \
    libcurl-host \
    libssl-host \
    $(ue_update_metadata_protos_exported_shared_libraries)

ue_libpayload_consumer_src_files := \
    common/action_processor.cc \
    common/boot_control_stub.cc \
    common/certificate_checker.cc \
    common/clock.cc \
    common/constants.cc \
    common/cpu_limiter.cc \
    common/error_code_utils.cc \
    common/hash_calculator.cc \
    common/http_common.cc \
    common/http_fetcher.cc \
    common/hwid_override.cc \
    common/libcurl_http_fetcher.cc \
    common/multi_range_http_fetcher.cc \
    common/platform_constants_android.cc \
    common/prefs.cc \
    common/subprocess.cc \
    common/terminator.cc \
    common/utils.cc \
    payload_consumer/bzip_extent_writer.cc \
    payload_consumer/delta_performer.cc \
    payload_consumer/download_action.cc \
    payload_consumer/extent_writer.cc \
    payload_consumer/file_descriptor.cc \
    payload_consumer/file_writer.cc \
    payload_consumer/filesystem_verifier_action.cc \
    payload_consumer/install_plan.cc \
    payload_consumer/payload_constants.cc \
    payload_consumer/payload_verifier.cc \
    payload_consumer/postinstall_runner_action.cc \
    payload_consumer/xz_extent_writer.cc

ifeq ($(HOST_OS),linux)
# Build for the host.
include $(CLEAR_VARS)
LOCAL_MODULE := libpayload_consumer
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES := \
    $(ue_common_c_includes) \
    external/e2fsprogs/lib
LOCAL_STATIC_LIBRARIES := \
    update_metadata-protos \
    $(ue_libpayload_consumer_exported_static_libraries) \
    $(ue_update_metadata_protos_exported_static_libraries)
LOCAL_SHARED_LIBRARIES := \
    $(ue_common_shared_libraries) \
    $(ue_libpayload_consumer_exported_shared_libraries) \
    $(ue_update_metadata_protos_exported_shared_libraries)
LOCAL_SRC_FILES := $(ue_libpayload_consumer_src_files)
include $(BUILD_HOST_STATIC_LIBRARY)
endif  # HOST_OS == linux

# Build for the target.
include $(CLEAR_VARS)
LOCAL_MODULE := libpayload_consumer
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES := \
    $(ue_common_c_includes) \
    external/e2fsprogs/lib
LOCAL_STATIC_LIBRARIES := \
    update_metadata-protos \
    $(ue_libpayload_consumer_exported_static_libraries:-host=) \
    $(ue_update_metadata_protos_exported_static_libraries)
LOCAL_SHARED_LIBRARIES := \
    $(ue_common_shared_libraries) \
    $(ue_libpayload_consumer_exported_shared_libraries:-host=) \
    $(ue_update_metadata_protos_exported_shared_libraries)
LOCAL_SRC_FILES := $(ue_libpayload_consumer_src_files)
include $(BUILD_STATIC_LIBRARY)

ifdef BRILLO

# libupdate_engine (type: static_library)
# ========================================================
# The main daemon static_library with all the code used to check for updates
# with Omaha and expose a DBus daemon.
ue_libupdate_engine_exported_c_includes := \
    $(LOCAL_PATH)/include \
    external/cros/system_api/dbus
ue_libupdate_engine_exported_static_libraries := \
    libpayload_consumer \
    update_metadata-protos \
    update_engine-dbus-adaptor \
    update_engine-dbus-libcros-client \
    update_engine_client-dbus-proxies \
    libbz \
    libfs_mgr \
    $(ue_libpayload_consumer_exported_static_libraries) \
    $(ue_update_metadata_protos_exported_static_libraries)
ue_libupdate_engine_exported_shared_libraries := \
    libdbus \
    libbrillo-dbus \
    libchrome-dbus \
    libmetrics \
    libshill-client \
    libexpat \
    libbrillo-policy \
    libhardware \
    libcutils \
    $(ue_libpayload_consumer_exported_shared_libraries) \
    $(ue_update_metadata_protos_exported_shared_libraries)
ifeq ($(local_use_binder),1)
ue_libupdate_engine_exported_shared_libraries += \
    libbinder \
    libbinderwrapper \
    libbrillo-binder \
    libutils
endif  # local_use_binder == 1
ifeq ($(local_use_weave),1)
ue_libupdate_engine_exported_shared_libraries += \
    libbinderwrapper \
    libbrillo-binder \
    libweaved
endif  # local_use_weave == 1

include $(CLEAR_VARS)
LOCAL_MODULE := libupdate_engine
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(ue_libupdate_engine_exported_c_includes)
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES := \
    $(ue_common_c_includes) \
    $(ue_libupdate_engine_exported_c_includes)
LOCAL_STATIC_LIBRARIES := \
    libpayload_consumer \
    update_metadata-protos \
    update_engine-dbus-adaptor \
    update_engine-dbus-libcros-client \
    update_engine_client-dbus-proxies \
    $(ue_libupdate_engine_exported_static_libraries:-host=) \
    $(ue_libpayload_consumer_exported_static_libraries:-host=) \
    $(ue_update_metadata_protos_exported_static_libraries)
LOCAL_SHARED_LIBRARIES := \
    $(ue_common_shared_libraries) \
    $(ue_libupdate_engine_exported_shared_libraries:-host=) \
    $(ue_libpayload_consumer_exported_shared_libraries:-host=) \
    $(ue_update_metadata_protos_exported_shared_libraries)
LOCAL_SRC_FILES := \
    boot_control_android.cc \
    common_service.cc \
    connection_manager.cc \
    daemon.cc \
    dbus_service.cc \
    hardware_android.cc \
    image_properties_android.cc \
    libcros_proxy.cc \
    metrics.cc \
    metrics_utils.cc \
    omaha_request_action.cc \
    omaha_request_params.cc \
    omaha_response_handler_action.cc \
    p2p_manager.cc \
    payload_state.cc \
    proxy_resolver.cc \
    real_system_state.cc \
    shill_proxy.cc \
    update_attempter.cc \
    update_manager/boxed_value.cc \
    update_manager/chromeos_policy.cc \
    update_manager/default_policy.cc \
    update_manager/evaluation_context.cc \
    update_manager/policy.cc \
    update_manager/real_config_provider.cc \
    update_manager/real_device_policy_provider.cc \
    update_manager/real_random_provider.cc \
    update_manager/real_shill_provider.cc \
    update_manager/real_system_provider.cc \
    update_manager/real_time_provider.cc \
    update_manager/real_updater_provider.cc \
    update_manager/state_factory.cc \
    update_manager/update_manager.cc \
    update_status_utils.cc \
    weave_service_factory.cc
ifeq ($(local_use_binder),1)
LOCAL_AIDL_INCLUDES += $(LOCAL_PATH)/binder_bindings
LOCAL_SRC_FILES += \
    binder_bindings/android/brillo/IUpdateEngine.aidl \
    binder_bindings/android/brillo/IUpdateEngineStatusCallback.aidl \
    binder_service_brillo.cc \
    parcelable_update_engine_status.cc
endif  # local_use_binder == 1
ifeq ($(local_use_weave),1)
LOCAL_SRC_FILES += \
    weave_service.cc
endif  # local_use_weave == 1
ifeq ($(local_use_libcros),1)
LOCAL_SRC_FILES += \
    chrome_browser_proxy_resolver.cc
endif  # local_use_libcros == 1
include $(BUILD_STATIC_LIBRARY)

else  # !defined(BRILLO)

ifneq ($(local_use_binder),1)
$(error USE_BINDER is disabled but is required in non-Brillo devices.)
endif  # local_use_binder == 1

# libupdate_engine_android (type: static_library)
# ========================================================
# The main daemon static_library used in Android (non-Brillo). This only has a
# loop to apply payloads provided by the upper layer via a Binder interface.
ue_libupdate_engine_android_exported_static_libraries := \
    libpayload_consumer \
    libfs_mgr \
    $(ue_libpayload_consumer_exported_static_libraries)
ue_libupdate_engine_android_exported_shared_libraries := \
    $(ue_libpayload_consumer_exported_shared_libraries) \
    libbinder \
    libbinderwrapper \
    libbrillo-binder \
    libcutils \
    libhardware \
    libutils

include $(CLEAR_VARS)
LOCAL_MODULE := libupdate_engine_android
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES :=  $(ue_common_c_includes)
#TODO(deymo): Remove external/cros/system_api/dbus once the strings are moved
# out of the DBus interface.
LOCAL_C_INCLUDES += \
    external/cros/system_api/dbus
LOCAL_STATIC_LIBRARIES := \
    $(ue_libupdate_engine_android_exported_static_libraries:-host=)
LOCAL_SHARED_LIBRARIES += \
    $(ue_common_shared_libraries) \
    $(ue_libupdate_engine_android_exported_shared_libraries:-host=)
LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/binder_bindings
LOCAL_SRC_FILES += \
    binder_bindings/android/os/IUpdateEngine.aidl \
    binder_bindings/android/os/IUpdateEngineCallback.aidl \
    binder_service_android.cc \
    boot_control_android.cc \
    daemon.cc \
    daemon_state_android.cc \
    hardware_android.cc \
    proxy_resolver.cc \
    update_attempter_android.cc \
    update_status_utils.cc
include $(BUILD_STATIC_LIBRARY)

endif  # !defined(BRILLO)

# update_engine (type: executable)
# ========================================================
# update_engine daemon.
include $(CLEAR_VARS)
LOCAL_MODULE := update_engine
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_REQUIRED_MODULES := \
    bspatch \
    cacerts_google
ifeq ($(local_use_weave),1)
LOCAL_REQUIRED_MODULES += updater.json
endif  # local_use_weave == 1
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES := \
    $(ue_common_c_includes)
LOCAL_SHARED_LIBRARIES := \
    $(ue_common_shared_libraries)
LOCAL_SRC_FILES := \
    main.cc

ifdef BRILLO
LOCAL_C_INCLUDES += \
    $(ue_libupdate_engine_exported_c_includes)
LOCAL_STATIC_LIBRARIES := \
    libupdate_engine \
    $(ue_libupdate_engine_exported_static_libraries:-host=)
LOCAL_SHARED_LIBRARIES += \
    $(ue_libupdate_engine_exported_shared_libraries:-host=)
else  # !defined(BRILLO)
LOCAL_STATIC_LIBRARIES := \
    libupdate_engine_android \
    $(ue_libupdate_engine_android_exported_static_libraries:-host=)
LOCAL_SHARED_LIBRARIES += \
    $(ue_libupdate_engine_android_exported_shared_libraries:-host=)
endif  # !defined(BRILLO)

LOCAL_INIT_RC := update_engine.rc
include $(BUILD_EXECUTABLE)

# libupdate_engine_client (type: shared_library)
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libupdate_engine_client
LOCAL_CFLAGS := \
    -Wall \
    -Werror \
    -Wno-unused-parameter \
    -DUSE_DBUS=$(local_use_dbus) \
    -DUSE_BINDER=$(local_use_binder)
LOCAL_CLANG := true
LOCAL_CPP_EXTENSION := .cc
# TODO(deymo): Remove "external/cros/system_api/dbus" when dbus is not used.
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/client_library/include \
    external/cros/system_api/dbus \
    system \
    external/gtest/include
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/client_library/include
LOCAL_SHARED_LIBRARIES := \
    libchrome \
    libbrillo
LOCAL_SRC_FILES := \
    client_library/client.cc \
    update_status_utils.cc

# We can only compile support for one IPC mechanism. If both "binder" and "dbus"
# are defined, we prefer binder.
ifeq ($(local_use_binder),1)
LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/binder_bindings
LOCAL_SHARED_LIBRARIES += \
    libbinder \
    libbrillo-binder \
    libutils
LOCAL_SRC_FILES += \
    binder_bindings/android/brillo/IUpdateEngine.aidl \
    binder_bindings/android/brillo/IUpdateEngineStatusCallback.aidl \
    client_library/client_binder.cc \
    parcelable_update_engine_status.cc
else  # local_use_binder != 1
LOCAL_STATIC_LIBRARIES := \
    update_engine_client-dbus-proxies
LOCAL_SHARED_LIBRARIES += \
    libchrome-dbus \
    libbrillo-dbus
LOCAL_SRC_FILES += \
    client_library/client_dbus.cc
endif  # local_use_binder == 1

include $(BUILD_SHARED_LIBRARY)

# update_engine_client (type: executable)
# ========================================================
# update_engine console client.
include $(CLEAR_VARS)
LOCAL_MODULE := update_engine_client
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES := $(ue_common_c_includes)
LOCAL_SHARED_LIBRARIES := $(ue_common_shared_libraries)
ifdef BRILLO
LOCAL_SHARED_LIBRARIES += \
    libupdate_engine_client
LOCAL_SRC_FILES := \
    update_engine_client.cc \
    common/error_code_utils.cc
else  # !defined(BRILLO)
#TODO(deymo): Remove external/cros/system_api/dbus once the strings are moved
# out of the DBus interface.
LOCAL_C_INCLUDES += \
    external/cros/system_api/dbus
LOCAL_SHARED_LIBRARIES += \
    libbinder \
    libbinderwrapper \
    libbrillo-binder \
    libutils
LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/binder_bindings
LOCAL_SRC_FILES := \
    binder_bindings/android/os/IUpdateEngine.aidl \
    binder_bindings/android/os/IUpdateEngineCallback.aidl \
    common/error_code_utils.cc \
    update_engine_client_android.cc \
    update_status_utils.cc
endif  # !defined(BRILLO)
include $(BUILD_EXECUTABLE)

# libpayload_generator (type: static_library)
# ========================================================
# server-side code. This is used for delta_generator and unittests but not
# for any client code.
ue_libpayload_generator_exported_static_libraries := \
    libpayload_consumer \
    update_metadata-protos \
    liblzma \
    $(ue_libpayload_consumer_exported_static_libraries) \
    $(ue_update_metadata_protos_exported_static_libraries)
ue_libpayload_generator_exported_shared_libraries := \
    libext2fs-host \
    $(ue_libpayload_consumer_exported_shared_libraries) \
    $(ue_update_metadata_protos_exported_shared_libraries)

ue_libpayload_generator_src_files := \
    payload_generator/ab_generator.cc \
    payload_generator/annotated_operation.cc \
    payload_generator/blob_file_writer.cc \
    payload_generator/block_mapping.cc \
    payload_generator/bzip.cc \
    payload_generator/cycle_breaker.cc \
    payload_generator/delta_diff_generator.cc \
    payload_generator/delta_diff_utils.cc \
    payload_generator/ext2_filesystem.cc \
    payload_generator/extent_ranges.cc \
    payload_generator/extent_utils.cc \
    payload_generator/full_update_generator.cc \
    payload_generator/graph_types.cc \
    payload_generator/graph_utils.cc \
    payload_generator/inplace_generator.cc \
    payload_generator/payload_file.cc \
    payload_generator/payload_generation_config.cc \
    payload_generator/payload_signer.cc \
    payload_generator/raw_filesystem.cc \
    payload_generator/tarjan.cc \
    payload_generator/topological_sort.cc \
    payload_generator/xz_android.cc

ifeq ($(HOST_OS),linux)
# Build for the host.
include $(CLEAR_VARS)
LOCAL_MODULE := libpayload_generator
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES := $(ue_common_c_includes)
LOCAL_STATIC_LIBRARIES := \
    libpayload_consumer \
    update_metadata-protos \
    liblzma \
    $(ue_libpayload_consumer_exported_static_libraries) \
    $(ue_update_metadata_protos_exported_static_libraries)
LOCAL_SHARED_LIBRARIES := \
    $(ue_common_shared_libraries) \
    $(ue_libpayload_generator_exported_shared_libraries) \
    $(ue_libpayload_consumer_exported_shared_libraries) \
    $(ue_update_metadata_protos_exported_shared_libraries)
LOCAL_SRC_FILES := $(ue_libpayload_generator_src_files)
include $(BUILD_HOST_STATIC_LIBRARY)
endif  # HOST_OS == linux

# Build for the target.
include $(CLEAR_VARS)
LOCAL_MODULE := libpayload_generator
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES := $(ue_common_c_includes)
LOCAL_STATIC_LIBRARIES := \
    libpayload_consumer \
    update_metadata-protos \
    liblzma \
    $(ue_libpayload_consumer_exported_static_libraries:-host=) \
    $(ue_update_metadata_protos_exported_static_libraries)
LOCAL_SHARED_LIBRARIES := \
    $(ue_common_shared_libraries) \
    $(ue_libpayload_generator_exported_shared_libraries:-host=) \
    $(ue_libpayload_consumer_exported_shared_libraries:-host=) \
    $(ue_update_metadata_protos_exported_shared_libraries)
LOCAL_SRC_FILES := $(ue_libpayload_generator_src_files)
include $(BUILD_STATIC_LIBRARY)

# delta_generator (type: executable)
# ========================================================
# server-side delta generator.
ue_delta_generator_src_files := \
    payload_generator/generate_delta_main.cc

ifeq ($(HOST_OS),linux)
# Build for the host.
include $(CLEAR_VARS)
LOCAL_MODULE := delta_generator
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES := $(ue_common_c_includes)
LOCAL_STATIC_LIBRARIES := \
    libpayload_consumer \
    libpayload_generator \
    $(ue_libpayload_consumer_exported_static_libraries) \
    $(ue_libpayload_generator_exported_static_libraries)
LOCAL_SHARED_LIBRARIES := \
    $(ue_common_shared_libraries) \
    $(ue_libpayload_consumer_exported_shared_libraries) \
    $(ue_libpayload_generator_exported_shared_libraries)
LOCAL_SRC_FILES := $(ue_delta_generator_src_files)
include $(BUILD_HOST_EXECUTABLE)
endif  # HOST_OS == linux

# Build for the target.
include $(CLEAR_VARS)
LOCAL_MODULE := delta_generator
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES := $(ue_common_c_includes)
LOCAL_STATIC_LIBRARIES := \
    libpayload_consumer \
    libpayload_generator \
    $(ue_libpayload_consumer_exported_static_libraries:-host=) \
    $(ue_libpayload_generator_exported_static_libraries:-host=)
LOCAL_SHARED_LIBRARIES := \
    $(ue_common_shared_libraries) \
    $(ue_libpayload_consumer_exported_shared_libraries:-host=) \
    $(ue_libpayload_generator_exported_shared_libraries:-host=)
LOCAL_SRC_FILES := $(ue_delta_generator_src_files)
include $(BUILD_EXECUTABLE)

# TODO(deymo): Enable the unittest binaries in non-Brillo builds once the DBus
# dependencies are removed or placed behind the USE_DBUS flag.
ifdef BRILLO

# Private and public keys for unittests.
# ========================================================
# Generate a module that installs a prebuilt private key and a module that
# installs a public key generated from the private key.
#
# $(1): The path to the private key in pem format.
define ue-unittest-keys
    $(eval include $(CLEAR_VARS)) \
    $(eval LOCAL_MODULE := ue_$(1).pem) \
    $(eval LOCAL_MODULE_CLASS := ETC) \
    $(eval $(ifeq $(BRILLO), 1, LOCAL_MODULE_TAGS := eng)) \
    $(eval LOCAL_SRC_FILES := $(1).pem) \
    $(eval LOCAL_MODULE_PATH := \
        $(TARGET_OUT_DATA_NATIVE_TESTS)/update_engine_unittests) \
    $(eval LOCAL_MODULE_STEM := $(1).pem) \
    $(eval include $(BUILD_PREBUILT)) \
    \
    $(eval include $(CLEAR_VARS)) \
    $(eval LOCAL_MODULE := ue_$(1).pub.pem) \
    $(eval LOCAL_MODULE_CLASS := ETC) \
    $(eval $(ifeq $(BRILLO), 1, LOCAL_MODULE_TAGS := eng)) \
    $(eval LOCAL_MODULE_PATH := \
        $(TARGET_OUT_DATA_NATIVE_TESTS)/update_engine_unittests) \
    $(eval LOCAL_MODULE_STEM := $(1).pub.pem) \
    $(eval include $(BUILD_SYSTEM)/base_rules.mk) \
    $(eval $(LOCAL_BUILT_MODULE) : $(LOCAL_PATH)/$(1).pem ; \
        openssl rsa -in $$< -pubout -out $$@)
endef

$(call ue-unittest-keys,unittest_key)
$(call ue-unittest-keys,unittest_key2)

# Sample images for unittests.
# ========================================================
# Generate a prebuilt module that installs a sample image from the compressed
# sample_images.tar.bz2 file used by the unittests.
#
# $(1): The filename in the sample_images.tar.bz2
define ue-unittest-sample-image
    $(eval include $(CLEAR_VARS)) \
    $(eval LOCAL_MODULE := ue_unittest_$(1)) \
    $(eval LOCAL_MODULE_CLASS := EXECUTABLES) \
    $(eval $(ifeq $(BRILLO), 1, LOCAL_MODULE_TAGS := eng)) \
    $(eval LOCAL_MODULE_PATH := \
        $(TARGET_OUT_DATA_NATIVE_TESTS)/update_engine_unittests/gen) \
    $(eval LOCAL_MODULE_STEM := $(1)) \
    $(eval include $(BUILD_SYSTEM)/base_rules.mk) \
    $(eval $(LOCAL_BUILT_MODULE) : \
        $(LOCAL_PATH)/sample_images/sample_images.tar.bz2 ; \
        tar -jxf $$< -C $$(dir $$@) $$(notdir $$@) && touch $$@)
endef

$(call ue-unittest-sample-image,disk_ext2_1k.img)
$(call ue-unittest-sample-image,disk_ext2_4k.img)
$(call ue-unittest-sample-image,disk_ext2_4k_empty.img)
$(call ue-unittest-sample-image,disk_ext2_unittest.img)

# Zlib Fingerprint
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := zlib_fingerprint
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_NATIVE_TESTS)/update_engine_unittests
LOCAL_PREBUILT_MODULE_FILE := $(TARGET_OUT_COMMON_GEN)/zlib_fingerprint
include $(BUILD_PREBUILT)

# test_http_server (type: executable)
# ========================================================
# Test HTTP Server.
include $(CLEAR_VARS)
LOCAL_MODULE := test_http_server
ifdef BRILLO
  LOCAL_MODULE_TAGS := eng
endif
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_NATIVE_TESTS)/update_engine_unittests
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES := $(ue_common_c_includes)
LOCAL_SHARED_LIBRARIES := $(ue_common_shared_libraries)
LOCAL_SRC_FILES := \
    common/http_common.cc \
    test_http_server.cc
include $(BUILD_EXECUTABLE)

# update_engine_unittests (type: executable)
# ========================================================
# Main unittest file.
include $(CLEAR_VARS)
LOCAL_MODULE := update_engine_unittests
ifdef BRILLO
  LOCAL_MODULE_TAGS := eng
endif
LOCAL_REQUIRED_MODULES := \
    ue_unittest_disk_ext2_1k.img \
    ue_unittest_disk_ext2_4k.img \
    ue_unittest_disk_ext2_4k_empty.img \
    ue_unittest_disk_ext2_unittest.img \
    ue_unittest_key.pem \
    ue_unittest_key.pub.pem \
    ue_unittest_key2.pem \
    ue_unittest_key2.pub.pem \
    zlib_fingerprint
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_CPP_EXTENSION := .cc
LOCAL_CLANG := true
LOCAL_CFLAGS := $(ue_common_cflags)
LOCAL_CPPFLAGS := $(ue_common_cppflags)
LOCAL_LDFLAGS := $(ue_common_ldflags)
LOCAL_C_INCLUDES := \
    $(ue_common_c_includes) \
    $(ue_libupdate_engine_exported_c_includes)
LOCAL_STATIC_LIBRARIES := \
    libupdate_engine \
    libpayload_generator \
    libbrillo-test-helpers \
    libgmock \
    libgtest \
    libchrome_test_helpers \
    $(ue_libupdate_engine_exported_static_libraries:-host=) \
    $(ue_libpayload_generator_exported_static_libraries:-host=)
LOCAL_SHARED_LIBRARIES := \
    $(ue_common_shared_libraries) \
    $(ue_libupdate_engine_exported_shared_libraries:-host=) \
    $(ue_libpayload_generator_exported_shared_libraries:-host=)
LOCAL_SRC_FILES := \
    common/action_pipe_unittest.cc \
    common/action_processor_unittest.cc \
    common/action_unittest.cc \
    common/certificate_checker_unittest.cc \
    common/cpu_limiter_unittest.cc \
    common/fake_prefs.cc \
    common/hash_calculator_unittest.cc \
    common/http_fetcher_unittest.cc \
    common/hwid_override_unittest.cc \
    common/mock_http_fetcher.cc \
    common/prefs_unittest.cc \
    common/subprocess_unittest.cc \
    common/terminator_unittest.cc \
    common/test_utils.cc \
    common/utils_unittest.cc \
    common_service_unittest.cc \
    connection_manager_unittest.cc \
    fake_shill_proxy.cc \
    fake_system_state.cc \
    metrics_utils_unittest.cc \
    omaha_request_action_unittest.cc \
    omaha_request_params_unittest.cc \
    omaha_response_handler_action_unittest.cc \
    p2p_manager_unittest.cc \
    payload_consumer/bzip_extent_writer_unittest.cc \
    payload_consumer/delta_performer_integration_test.cc \
    payload_consumer/delta_performer_unittest.cc \
    payload_consumer/download_action_unittest.cc \
    payload_consumer/extent_writer_unittest.cc \
    payload_consumer/file_writer_unittest.cc \
    payload_consumer/filesystem_verifier_action_unittest.cc \
    payload_consumer/postinstall_runner_action_unittest.cc \
    payload_consumer/xz_extent_writer_unittest.cc \
    payload_generator/ab_generator_unittest.cc \
    payload_generator/blob_file_writer_unittest.cc \
    payload_generator/block_mapping_unittest.cc \
    payload_generator/cycle_breaker_unittest.cc \
    payload_generator/delta_diff_utils_unittest.cc \
    payload_generator/ext2_filesystem_unittest.cc \
    payload_generator/extent_ranges_unittest.cc \
    payload_generator/extent_utils_unittest.cc \
    payload_generator/fake_filesystem.cc \
    payload_generator/full_update_generator_unittest.cc \
    payload_generator/graph_utils_unittest.cc \
    payload_generator/inplace_generator_unittest.cc \
    payload_generator/payload_file_unittest.cc \
    payload_generator/payload_generation_config_unittest.cc \
    payload_generator/payload_signer_unittest.cc \
    payload_generator/tarjan_unittest.cc \
    payload_generator/topological_sort_unittest.cc \
    payload_generator/zip_unittest.cc \
    payload_state_unittest.cc \
    update_attempter_unittest.cc \
    update_manager/boxed_value_unittest.cc \
    update_manager/chromeos_policy_unittest.cc \
    update_manager/evaluation_context_unittest.cc \
    update_manager/generic_variables_unittest.cc \
    update_manager/prng_unittest.cc \
    update_manager/real_config_provider_unittest.cc \
    update_manager/real_device_policy_provider_unittest.cc \
    update_manager/real_random_provider_unittest.cc \
    update_manager/real_shill_provider_unittest.cc \
    update_manager/real_system_provider_unittest.cc \
    update_manager/real_time_provider_unittest.cc \
    update_manager/real_updater_provider_unittest.cc \
    update_manager/umtest_utils.cc \
    update_manager/update_manager_unittest.cc \
    update_manager/variable_unittest.cc \
    testrunner.cc
ifeq ($(local_use_libcros),1)
LOCAL_SRC_FILES += \
    chrome_browser_proxy_resolver_unittest.cc
endif  # local_use_libcros == 1
include $(BUILD_NATIVE_TEST)
endif  # BRILLO

# Weave schema files
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := updater.json
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/weaved/traits
LOCAL_SRC_FILES := weaved/traits/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# Update payload signing public key.
# ========================================================
ifdef BRILLO
include $(CLEAR_VARS)
LOCAL_MODULE := brillo-update-payload-key
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/update_engine
LOCAL_MODULE_STEM := update-payload-key.pub.pem
LOCAL_SRC_FILES := update_payload_key/brillo-update-payload-key.pub.pem
LOCAL_BUILT_MODULE_STEM := update_payload_key/brillo-update-payload-key.pub.pem
include $(BUILD_PREBUILT)
endif  # BRILLO

# Brillo update payload generation script
# ========================================================
ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := scripts/brillo_update_payload
LOCAL_MODULE := brillo_update_payload
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    delta_generator \
    shflags
include $(BUILD_PREBUILT)
endif  # HOST_OS == linux
