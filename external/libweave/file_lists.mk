# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

WEAVE_SRC_FILES := \
	src/access_api_handler.cc \
	src/access_black_list_manager_impl.cc \
	src/backoff_entry.cc \
	src/base_api_handler.cc \
	src/commands/cloud_command_proxy.cc \
	src/commands/command_instance.cc \
	src/commands/command_queue.cc \
	src/commands/schema_constants.cc \
	src/component_manager_impl.cc \
	src/config.cc \
	src/data_encoding.cc \
	src/device_manager.cc \
	src/device_registration_info.cc \
	src/error.cc \
	src/http_constants.cc \
	src/json_error_codes.cc \
	src/notification/notification_parser.cc \
	src/notification/pull_channel.cc \
	src/notification/xml_node.cc \
	src/notification/xmpp_channel.cc \
	src/notification/xmpp_iq_stanza_handler.cc \
	src/notification/xmpp_stream_parser.cc \
	src/privet/auth_manager.cc \
	src/privet/cloud_delegate.cc \
	src/privet/constants.cc \
	src/privet/device_delegate.cc \
	src/privet/device_ui_kind.cc \
	src/privet/openssl_utils.cc \
	src/privet/privet_handler.cc \
	src/privet/privet_manager.cc \
	src/privet/privet_types.cc \
	src/privet/publisher.cc \
	src/privet/security_manager.cc \
	src/privet/wifi_bootstrap_manager.cc \
	src/privet/wifi_ssid_generator.cc \
	src/registration_status.cc \
	src/states/state_change_queue.cc \
	src/streams.cc \
	src/string_utils.cc \
	src/utils.cc

WEAVE_TEST_SRC_FILES := \
	src/test/fake_stream.cc \
	src/test/fake_task_runner.cc \
	src/test/unittest_utils.cc

WEAVE_UNITTEST_SRC_FILES := \
	src/access_api_handler_unittest.cc \
	src/access_black_list_manager_impl_unittest.cc \
	src/backoff_entry_unittest.cc \
	src/base_api_handler_unittest.cc \
	src/commands/cloud_command_proxy_unittest.cc \
	src/commands/command_instance_unittest.cc \
	src/commands/command_queue_unittest.cc \
	src/component_manager_unittest.cc \
	src/config_unittest.cc \
	src/data_encoding_unittest.cc \
	src/device_registration_info_unittest.cc \
	src/error_unittest.cc \
	src/notification/notification_parser_unittest.cc \
	src/notification/xml_node_unittest.cc \
	src/notification/xmpp_channel_unittest.cc \
	src/notification/xmpp_iq_stanza_handler_unittest.cc \
	src/notification/xmpp_stream_parser_unittest.cc \
	src/privet/auth_manager_unittest.cc \
	src/privet/privet_handler_unittest.cc \
	src/privet/security_manager_unittest.cc \
	src/privet/wifi_ssid_generator_unittest.cc \
	src/states/state_change_queue_unittest.cc \
	src/streams_unittest.cc \
	src/string_utils_unittest.cc \
	src/test/weave_testrunner.cc

WEAVE_EXPORTS_UNITTEST_SRC_FILES := \
	src/weave_unittest.cc

EXAMPLES_PROVIDER_SRC_FILES := \
	examples/provider/avahi_client.cc \
	examples/provider/bluez_client.cc \
	examples/provider/curl_http_client.cc \
	examples/provider/event_http_server.cc \
	examples/provider/event_network.cc \
	examples/provider/event_task_runner.cc \
	examples/provider/file_config_store.cc \
	examples/provider/ssl_stream.cc \
	examples/provider/wifi_manager.cc

THIRD_PARTY_CHROMIUM_BASE_SRC_FILES := \
	third_party/chromium/base/bind_helpers.cc \
	third_party/chromium/base/callback_internal.cc \
	third_party/chromium/base/guid_posix.cc \
	third_party/chromium/base/json/json_parser.cc \
	third_party/chromium/base/json/json_reader.cc \
	third_party/chromium/base/json/json_writer.cc \
	third_party/chromium/base/json/string_escape.cc \
	third_party/chromium/base/location.cc \
	third_party/chromium/base/logging.cc \
	third_party/chromium/base/memory/ref_counted.cc \
	third_party/chromium/base/memory/weak_ptr.cc \
	third_party/chromium/base/rand_util.cc \
	third_party/chromium/base/rand_util_posix.cc \
	third_party/chromium/base/strings/string_number_conversions.cc \
	third_party/chromium/base/strings/string_piece.cc \
	third_party/chromium/base/strings/stringprintf.cc \
	third_party/chromium/base/strings/string_util.cc \
	third_party/chromium/base/strings/string_util_constants.cc \
	third_party/chromium/base/strings/utf_string_conversion_utils.cc \
	third_party/chromium/base/third_party/dmg_fp/dtoa.cc \
	third_party/chromium/base/third_party/dmg_fp/g_fmt.cc \
	third_party/chromium/base/third_party/icu/icu_utf.cc \
	third_party/chromium/base/time/clock.cc \
	third_party/chromium/base/time/default_clock.cc \
	third_party/chromium/base/time/time.cc \
	third_party/chromium/base/time/time_posix.cc \
	third_party/chromium/base/values.cc

THIRD_PARTY_CHROMIUM_BASE_UNITTEST_SRC_FILES := \
	third_party/chromium/base/bind_unittest.cc \
	third_party/chromium/base/callback_list_unittest.cc \
	third_party/chromium/base/callback_unittest.cc \
	third_party/chromium/base/guid_unittest.cc \
	third_party/chromium/base/json/json_parser_unittest.cc \
	third_party/chromium/base/json/json_reader_unittest.cc \
	third_party/chromium/base/json/json_writer_unittest.cc \
	third_party/chromium/base/json/string_escape_unittest.cc \
	third_party/chromium/base/logging_unittest.cc \
	third_party/chromium/base/memory/ref_counted_unittest.cc \
	third_party/chromium/base/memory/scoped_ptr_unittest.cc \
	third_party/chromium/base/memory/weak_ptr_unittest.cc \
	third_party/chromium/base/numerics/safe_numerics_unittest.cc \
	third_party/chromium/base/observer_list_unittest.cc \
	third_party/chromium/base/rand_util_unittest.cc \
	third_party/chromium/base/scoped_clear_errno_unittest.cc \
	third_party/chromium/base/strings/string_number_conversions_unittest.cc \
	third_party/chromium/base/strings/string_piece_unittest.cc \
	third_party/chromium/base/strings/string_util_unittest.cc \
	third_party/chromium/base/strings/stringprintf_unittest.cc \
	third_party/chromium/base/template_util_unittest.cc \
	third_party/chromium/base/time/time_unittest.cc \
	third_party/chromium/base/tuple_unittest.cc \
	third_party/chromium/base/values_unittest.cc

THIRD_PARTY_CHROMIUM_CRYPTO_SRC_FILES := \
	third_party/chromium/crypto/p224.cc \
	third_party/chromium/crypto/p224_spake.cc \
	third_party/chromium/crypto/sha2.cc

THIRD_PARTY_CHROMIUM_CRYPTO_UNITTEST_SRC_FILES := \
	third_party/chromium/crypto/p224_spake_unittest.cc \
	third_party/chromium/crypto/p224_unittest.cc \
	third_party/chromium/crypto/sha2_unittest.cc

THIRD_PARTY_MODP_B64_SRC_FILES := \
	third_party/modp_b64/modp_b64.cc

THIRD_PARTY_LIBUWEAVE_SRC_FILES := \
	third_party/libuweave/src/crypto_hmac.c \
	third_party/libuweave/src/crypto_utils.c \
	third_party/libuweave/src/macaroon.c \
	third_party/libuweave/src/macaroon_caveat.c \
	third_party/libuweave/src/macaroon_context.c \
	third_party/libuweave/src/macaroon_encoding.c
