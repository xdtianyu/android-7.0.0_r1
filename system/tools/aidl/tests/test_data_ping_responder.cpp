/*
 * Copyright (C) 2015, The Android Open Source Project
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

#include "tests/test_data.h"

namespace android {
namespace aidl {
namespace test_data {
namespace ping_responder {

const char kCanonicalName[] = "android.os.IPingResponder";
const char kInterfaceDefinition[] = R"(
package android.os;

import bar.Unused;

interface IPingResponder {
  String Ping(String input);

  @nullable
  String NullablePing(@nullable String input);

  @utf8InCpp
  String Utf8Ping(@utf8InCpp String input);

  @utf8InCpp @nullable
  String NullableUtf8Ping(@utf8InCpp @nullable String input);
}
)";

const char kCppOutputPath[] = "some/path/to/output.cpp";

const char kCppParcelableHeader[] = "cpp-header-str";

const char* kImportedParcelables[] = {
  "bar.Unused",
  nullptr,
};

const char* kImportedInterfaces[] = {
  nullptr,
};

const char kGenHeaderDir[] = "some/path";
const char kGenInterfaceHeaderPath[] = "some/path/android/os/IPingResponder.h";
const char kGenClientHeaderPath[] = "some/path/android/os/BpPingResponder.h";
const char kGenServerHeaderPath[] = "some/path/android/os/BnPingResponder.h";

const char kExpectedCppDepsOutput[] =
R"(some/path/to/output.cpp : \
  android/os/IPingResponder.aidl \
  ./bar/Unused.aidl

android/os/IPingResponder.aidl :
./bar/Unused.aidl :

some/path/android/os/BpPingResponder.h \
    some/path/android/os/BnPingResponder.h \
    some/path/android/os/IPingResponder.h : \
    android/os/IPingResponder.aidl \
    ./bar/Unused.aidl
)";

const char kExpectedCppOutput[] =
R"(#include <android/os/IPingResponder.h>
#include <android/os/BpPingResponder.h>

namespace android {

namespace os {

IMPLEMENT_META_INTERFACE(PingResponder, "android.os.IPingResponder");

}  // namespace os

}  // namespace android
#include <android/os/BpPingResponder.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

BpPingResponder::BpPingResponder(const ::android::sp<::android::IBinder>& _aidl_impl)
    : BpInterface<IPingResponder>(_aidl_impl){
}

::android::binder::Status BpPingResponder::Ping(const ::android::String16& input, ::android::String16* _aidl_return) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeString16(input);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IPingResponder::PING, _aidl_data, &_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_status.readFromParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
if (!_aidl_status.isOk()) {
return _aidl_status;
}
_aidl_ret_status = _aidl_reply.readString16(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpPingResponder::NullablePing(const ::std::unique_ptr<::android::String16>& input, ::std::unique_ptr<::android::String16>* _aidl_return) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeString16(input);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IPingResponder::NULLABLEPING, _aidl_data, &_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_status.readFromParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
if (!_aidl_status.isOk()) {
return _aidl_status;
}
_aidl_ret_status = _aidl_reply.readString16(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpPingResponder::Utf8Ping(const ::std::string& input, ::std::string* _aidl_return) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeUtf8AsUtf16(input);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IPingResponder::UTF8PING, _aidl_data, &_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_status.readFromParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
if (!_aidl_status.isOk()) {
return _aidl_status;
}
_aidl_ret_status = _aidl_reply.readUtf8FromUtf16(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpPingResponder::NullableUtf8Ping(const ::std::unique_ptr<::std::string>& input, ::std::unique_ptr<::std::string>* _aidl_return) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeUtf8AsUtf16(input);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IPingResponder::NULLABLEUTF8PING, _aidl_data, &_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_status.readFromParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
if (!_aidl_status.isOk()) {
return _aidl_status;
}
_aidl_ret_status = _aidl_reply.readUtf8FromUtf16(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

}  // namespace os

}  // namespace android
#include <android/os/BnPingResponder.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

::android::status_t BnPingResponder::onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) {
::android::status_t _aidl_ret_status = ::android::OK;
switch (_aidl_code) {
case Call::PING:
{
::android::String16 in_input;
::android::String16 _aidl_return;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readString16(&in_input);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(Ping(in_input, &_aidl_return));
_aidl_ret_status = _aidl_status.writeToParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
if (!_aidl_status.isOk()) {
break;
}
_aidl_ret_status = _aidl_reply->writeString16(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
}
break;
case Call::NULLABLEPING:
{
::std::unique_ptr<::android::String16> in_input;
::std::unique_ptr<::android::String16> _aidl_return;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readString16(&in_input);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(NullablePing(in_input, &_aidl_return));
_aidl_ret_status = _aidl_status.writeToParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
if (!_aidl_status.isOk()) {
break;
}
_aidl_ret_status = _aidl_reply->writeString16(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
}
break;
case Call::UTF8PING:
{
::std::string in_input;
::std::string _aidl_return;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readUtf8FromUtf16(&in_input);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(Utf8Ping(in_input, &_aidl_return));
_aidl_ret_status = _aidl_status.writeToParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
if (!_aidl_status.isOk()) {
break;
}
_aidl_ret_status = _aidl_reply->writeUtf8AsUtf16(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
}
break;
case Call::NULLABLEUTF8PING:
{
::std::unique_ptr<::std::string> in_input;
::std::unique_ptr<::std::string> _aidl_return;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readUtf8FromUtf16(&in_input);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(NullableUtf8Ping(in_input, &_aidl_return));
_aidl_ret_status = _aidl_status.writeToParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
if (!_aidl_status.isOk()) {
break;
}
_aidl_ret_status = _aidl_reply->writeUtf8AsUtf16(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
}
break;
default:
{
_aidl_ret_status = ::android::BBinder::onTransact(_aidl_code, _aidl_data, _aidl_reply, _aidl_flags);
}
break;
}
if (_aidl_ret_status == ::android::UNEXPECTED_NULL) {
_aidl_ret_status = ::android::binder::Status::fromExceptionCode(::android::binder::Status::EX_NULL_POINTER).writeToParcel(_aidl_reply);
}
return _aidl_ret_status;
}

}  // namespace os

}  // namespace android
)";

const char kExpectedIHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_I_PING_RESPONDER_H_
#define AIDL_GENERATED_ANDROID_OS_I_PING_RESPONDER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <memory>
#include <string>
#include <utils/String16.h>
#include <utils/StrongPointer.h>

namespace android {

namespace os {

class IPingResponder : public ::android::IInterface {
public:
DECLARE_META_INTERFACE(PingResponder);
virtual ::android::binder::Status Ping(const ::android::String16& input, ::android::String16* _aidl_return) = 0;
virtual ::android::binder::Status NullablePing(const ::std::unique_ptr<::android::String16>& input, ::std::unique_ptr<::android::String16>* _aidl_return) = 0;
virtual ::android::binder::Status Utf8Ping(const ::std::string& input, ::std::string* _aidl_return) = 0;
virtual ::android::binder::Status NullableUtf8Ping(const ::std::unique_ptr<::std::string>& input, ::std::unique_ptr<::std::string>* _aidl_return) = 0;
enum Call {
  PING = ::android::IBinder::FIRST_CALL_TRANSACTION + 0,
  NULLABLEPING = ::android::IBinder::FIRST_CALL_TRANSACTION + 1,
  UTF8PING = ::android::IBinder::FIRST_CALL_TRANSACTION + 2,
  NULLABLEUTF8PING = ::android::IBinder::FIRST_CALL_TRANSACTION + 3,
};
};  // class IPingResponder

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_I_PING_RESPONDER_H_)";

const char kExpectedBpHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_BP_PING_RESPONDER_H_
#define AIDL_GENERATED_ANDROID_OS_BP_PING_RESPONDER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/os/IPingResponder.h>

namespace android {

namespace os {

class BpPingResponder : public ::android::BpInterface<IPingResponder> {
public:
explicit BpPingResponder(const ::android::sp<::android::IBinder>& _aidl_impl);
virtual ~BpPingResponder() = default;
::android::binder::Status Ping(const ::android::String16& input, ::android::String16* _aidl_return) override;
::android::binder::Status NullablePing(const ::std::unique_ptr<::android::String16>& input, ::std::unique_ptr<::android::String16>* _aidl_return) override;
::android::binder::Status Utf8Ping(const ::std::string& input, ::std::string* _aidl_return) override;
::android::binder::Status NullableUtf8Ping(const ::std::unique_ptr<::std::string>& input, ::std::unique_ptr<::std::string>* _aidl_return) override;
};  // class BpPingResponder

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_BP_PING_RESPONDER_H_)";

const char kExpectedBnHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_BN_PING_RESPONDER_H_
#define AIDL_GENERATED_ANDROID_OS_BN_PING_RESPONDER_H_

#include <binder/IInterface.h>
#include <android/os/IPingResponder.h>

namespace android {

namespace os {

class BnPingResponder : public ::android::BnInterface<IPingResponder> {
public:
::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags = 0) override;
};  // class BnPingResponder

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_BN_PING_RESPONDER_H_)";

}  // namespace ping_responder
}  // namespace test_data
}  // namespace aidl
}  // namespace android
