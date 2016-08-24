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

#include <string>

#include <android-base/stringprintf.h>
#include <gtest/gtest.h>

#include "aidl.h"
#include "aidl_language.h"
#include "ast_cpp.h"
#include "code_writer.h"
#include "generate_cpp.h"
#include "os.h"
#include "tests/fake_io_delegate.h"
#include "tests/test_util.h"
#include "type_cpp.h"

using ::android::aidl::test::FakeIoDelegate;
using ::android::base::StringPrintf;
using std::string;
using std::unique_ptr;

namespace android {
namespace aidl {
namespace cpp {
namespace {

const string kComplexTypeInterfaceAIDL =
R"(package android.os;
import foo.IFooType;
interface IComplexTypeInterface {
  const int MY_CONSTANT = 3;
  int[] Send(in @nullable int[] goes_in, inout double[] goes_in_and_out, out boolean[] goes_out);
  oneway void Piff(int times);
  IFooType TakesABinder(IFooType f);
  List<String> StringListMethod(in java.util.List<String> input, out List<String> output);
  List<IBinder> BinderListMethod(in java.util.List<IBinder> input, out List<IBinder> output);
  FileDescriptor TakesAFileDescriptor(in FileDescriptor f);
  FileDescriptor[] TakesAFileDescriptorArray(in FileDescriptor[] f);
})";

const char kExpectedComplexTypeClientHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_BP_COMPLEX_TYPE_INTERFACE_H_
#define AIDL_GENERATED_ANDROID_OS_BP_COMPLEX_TYPE_INTERFACE_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/os/IComplexTypeInterface.h>

namespace android {

namespace os {

class BpComplexTypeInterface : public ::android::BpInterface<IComplexTypeInterface> {
public:
explicit BpComplexTypeInterface(const ::android::sp<::android::IBinder>& _aidl_impl);
virtual ~BpComplexTypeInterface() = default;
::android::binder::Status Send(const ::std::unique_ptr<::std::vector<int32_t>>& goes_in, ::std::vector<double>* goes_in_and_out, ::std::vector<bool>* goes_out, ::std::vector<int32_t>* _aidl_return) override;
::android::binder::Status Piff(int32_t times) override;
::android::binder::Status TakesABinder(const ::android::sp<::foo::IFooType>& f, ::android::sp<::foo::IFooType>* _aidl_return) override;
::android::binder::Status StringListMethod(const ::std::vector<::android::String16>& input, ::std::vector<::android::String16>* output, ::std::vector<::android::String16>* _aidl_return) override;
::android::binder::Status BinderListMethod(const ::std::vector<::android::sp<::android::IBinder>>& input, ::std::vector<::android::sp<::android::IBinder>>* output, ::std::vector<::android::sp<::android::IBinder>>* _aidl_return) override;
::android::binder::Status TakesAFileDescriptor(const ::ScopedFd& f, ::ScopedFd* _aidl_return) override;
::android::binder::Status TakesAFileDescriptorArray(const ::std::vector<::ScopedFd>& f, ::std::vector<::ScopedFd>* _aidl_return) override;
};  // class BpComplexTypeInterface

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_BP_COMPLEX_TYPE_INTERFACE_H_)";

const char kExpectedComplexTypeClientSourceOutput[] =
R"(#include <android/os/BpComplexTypeInterface.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

BpComplexTypeInterface::BpComplexTypeInterface(const ::android::sp<::android::IBinder>& _aidl_impl)
    : BpInterface<IComplexTypeInterface>(_aidl_impl){
}

::android::binder::Status BpComplexTypeInterface::Send(const ::std::unique_ptr<::std::vector<int32_t>>& goes_in, ::std::vector<double>* goes_in_and_out, ::std::vector<bool>* goes_out, ::std::vector<int32_t>* _aidl_return) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt32Vector(goes_in);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeDoubleVector(*goes_in_and_out);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IComplexTypeInterface::SEND, _aidl_data, &_aidl_reply);
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
_aidl_ret_status = _aidl_reply.readInt32Vector(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_reply.readDoubleVector(goes_in_and_out);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_reply.readBoolVector(goes_out);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpComplexTypeInterface::Piff(int32_t times) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeInt32(times);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IComplexTypeInterface::PIFF, _aidl_data, &_aidl_reply, ::android::IBinder::FLAG_ONEWAY);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpComplexTypeInterface::TakesABinder(const ::android::sp<::foo::IFooType>& f, ::android::sp<::foo::IFooType>* _aidl_return) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeStrongBinder(::foo::IFooType::asBinder(f));
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IComplexTypeInterface::TAKESABINDER, _aidl_data, &_aidl_reply);
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
_aidl_ret_status = _aidl_reply.readStrongBinder(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpComplexTypeInterface::StringListMethod(const ::std::vector<::android::String16>& input, ::std::vector<::android::String16>* output, ::std::vector<::android::String16>* _aidl_return) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeString16Vector(input);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IComplexTypeInterface::STRINGLISTMETHOD, _aidl_data, &_aidl_reply);
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
_aidl_ret_status = _aidl_reply.readString16Vector(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_reply.readString16Vector(output);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpComplexTypeInterface::BinderListMethod(const ::std::vector<::android::sp<::android::IBinder>>& input, ::std::vector<::android::sp<::android::IBinder>>* output, ::std::vector<::android::sp<::android::IBinder>>* _aidl_return) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeStrongBinderVector(input);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IComplexTypeInterface::BINDERLISTMETHOD, _aidl_data, &_aidl_reply);
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
_aidl_ret_status = _aidl_reply.readStrongBinderVector(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_reply.readStrongBinderVector(output);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpComplexTypeInterface::TakesAFileDescriptor(const ::ScopedFd& f, ::ScopedFd* _aidl_return) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeUniqueFileDescriptor(f);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IComplexTypeInterface::TAKESAFILEDESCRIPTOR, _aidl_data, &_aidl_reply);
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
_aidl_ret_status = _aidl_reply.readUniqueFileDescriptor(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

::android::binder::Status BpComplexTypeInterface::TakesAFileDescriptorArray(const ::std::vector<::ScopedFd>& f, ::std::vector<::ScopedFd>* _aidl_return) {
::android::Parcel _aidl_data;
::android::Parcel _aidl_reply;
::android::status_t _aidl_ret_status = ::android::OK;
::android::binder::Status _aidl_status;
_aidl_ret_status = _aidl_data.writeInterfaceToken(getInterfaceDescriptor());
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = _aidl_data.writeUniqueFileDescriptorVector(f);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_ret_status = remote()->transact(IComplexTypeInterface::TAKESAFILEDESCRIPTORARRAY, _aidl_data, &_aidl_reply);
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
_aidl_ret_status = _aidl_reply.readUniqueFileDescriptorVector(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
goto _aidl_error;
}
_aidl_error:
_aidl_status.setFromStatusT(_aidl_ret_status);
return _aidl_status;
}

}  // namespace os

}  // namespace android
)";

const char kExpectedComplexTypeServerHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_BN_COMPLEX_TYPE_INTERFACE_H_
#define AIDL_GENERATED_ANDROID_OS_BN_COMPLEX_TYPE_INTERFACE_H_

#include <binder/IInterface.h>
#include <android/os/IComplexTypeInterface.h>

namespace android {

namespace os {

class BnComplexTypeInterface : public ::android::BnInterface<IComplexTypeInterface> {
public:
::android::status_t onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags = 0) override;
};  // class BnComplexTypeInterface

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_BN_COMPLEX_TYPE_INTERFACE_H_)";

const char kExpectedComplexTypeServerSourceOutput[] =
R"(#include <android/os/BnComplexTypeInterface.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

::android::status_t BnComplexTypeInterface::onTransact(uint32_t _aidl_code, const ::android::Parcel& _aidl_data, ::android::Parcel* _aidl_reply, uint32_t _aidl_flags) {
::android::status_t _aidl_ret_status = ::android::OK;
switch (_aidl_code) {
case Call::SEND:
{
::std::unique_ptr<::std::vector<int32_t>> in_goes_in;
::std::vector<double> in_goes_in_and_out;
::std::vector<bool> out_goes_out;
::std::vector<int32_t> _aidl_return;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readInt32Vector(&in_goes_in);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_data.readDoubleVector(&in_goes_in_and_out);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(Send(in_goes_in, &in_goes_in_and_out, &out_goes_out, &_aidl_return));
_aidl_ret_status = _aidl_status.writeToParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
if (!_aidl_status.isOk()) {
break;
}
_aidl_ret_status = _aidl_reply->writeInt32Vector(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_reply->writeDoubleVector(in_goes_in_and_out);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_reply->writeBoolVector(out_goes_out);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
}
break;
case Call::PIFF:
{
int32_t in_times;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readInt32(&in_times);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(Piff(in_times));
}
break;
case Call::TAKESABINDER:
{
::android::sp<::foo::IFooType> in_f;
::android::sp<::foo::IFooType> _aidl_return;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readStrongBinder(&in_f);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(TakesABinder(in_f, &_aidl_return));
_aidl_ret_status = _aidl_status.writeToParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
if (!_aidl_status.isOk()) {
break;
}
_aidl_ret_status = _aidl_reply->writeStrongBinder(::foo::IFooType::asBinder(_aidl_return));
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
}
break;
case Call::STRINGLISTMETHOD:
{
::std::vector<::android::String16> in_input;
::std::vector<::android::String16> out_output;
::std::vector<::android::String16> _aidl_return;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readString16Vector(&in_input);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(StringListMethod(in_input, &out_output, &_aidl_return));
_aidl_ret_status = _aidl_status.writeToParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
if (!_aidl_status.isOk()) {
break;
}
_aidl_ret_status = _aidl_reply->writeString16Vector(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_reply->writeString16Vector(out_output);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
}
break;
case Call::BINDERLISTMETHOD:
{
::std::vector<::android::sp<::android::IBinder>> in_input;
::std::vector<::android::sp<::android::IBinder>> out_output;
::std::vector<::android::sp<::android::IBinder>> _aidl_return;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readStrongBinderVector(&in_input);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(BinderListMethod(in_input, &out_output, &_aidl_return));
_aidl_ret_status = _aidl_status.writeToParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
if (!_aidl_status.isOk()) {
break;
}
_aidl_ret_status = _aidl_reply->writeStrongBinderVector(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
_aidl_ret_status = _aidl_reply->writeStrongBinderVector(out_output);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
}
break;
case Call::TAKESAFILEDESCRIPTOR:
{
::ScopedFd in_f;
::ScopedFd _aidl_return;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readUniqueFileDescriptor(&in_f);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(TakesAFileDescriptor(in_f, &_aidl_return));
_aidl_ret_status = _aidl_status.writeToParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
if (!_aidl_status.isOk()) {
break;
}
_aidl_ret_status = _aidl_reply->writeUniqueFileDescriptor(_aidl_return);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
}
break;
case Call::TAKESAFILEDESCRIPTORARRAY:
{
::std::vector<::ScopedFd> in_f;
::std::vector<::ScopedFd> _aidl_return;
if (!(_aidl_data.checkInterface(this))) {
_aidl_ret_status = ::android::BAD_TYPE;
break;
}
_aidl_ret_status = _aidl_data.readUniqueFileDescriptorVector(&in_f);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
::android::binder::Status _aidl_status(TakesAFileDescriptorArray(in_f, &_aidl_return));
_aidl_ret_status = _aidl_status.writeToParcel(_aidl_reply);
if (((_aidl_ret_status) != (::android::OK))) {
break;
}
if (!_aidl_status.isOk()) {
break;
}
_aidl_ret_status = _aidl_reply->writeUniqueFileDescriptorVector(_aidl_return);
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

const char kExpectedComplexTypeInterfaceHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_I_COMPLEX_TYPE_INTERFACE_H_
#define AIDL_GENERATED_ANDROID_OS_I_COMPLEX_TYPE_INTERFACE_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/Status.h>
#include <cstdint>
#include <foo/IFooType.h>
#include <nativehelper/ScopedFd.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>
#include <vector>

namespace android {

namespace os {

class IComplexTypeInterface : public ::android::IInterface {
public:
DECLARE_META_INTERFACE(ComplexTypeInterface);
enum  : int32_t {
  MY_CONSTANT = 3,
};
virtual ::android::binder::Status Send(const ::std::unique_ptr<::std::vector<int32_t>>& goes_in, ::std::vector<double>* goes_in_and_out, ::std::vector<bool>* goes_out, ::std::vector<int32_t>* _aidl_return) = 0;
virtual ::android::binder::Status Piff(int32_t times) = 0;
virtual ::android::binder::Status TakesABinder(const ::android::sp<::foo::IFooType>& f, ::android::sp<::foo::IFooType>* _aidl_return) = 0;
virtual ::android::binder::Status StringListMethod(const ::std::vector<::android::String16>& input, ::std::vector<::android::String16>* output, ::std::vector<::android::String16>* _aidl_return) = 0;
virtual ::android::binder::Status BinderListMethod(const ::std::vector<::android::sp<::android::IBinder>>& input, ::std::vector<::android::sp<::android::IBinder>>* output, ::std::vector<::android::sp<::android::IBinder>>* _aidl_return) = 0;
virtual ::android::binder::Status TakesAFileDescriptor(const ::ScopedFd& f, ::ScopedFd* _aidl_return) = 0;
virtual ::android::binder::Status TakesAFileDescriptorArray(const ::std::vector<::ScopedFd>& f, ::std::vector<::ScopedFd>* _aidl_return) = 0;
enum Call {
  SEND = ::android::IBinder::FIRST_CALL_TRANSACTION + 0,
  PIFF = ::android::IBinder::FIRST_CALL_TRANSACTION + 1,
  TAKESABINDER = ::android::IBinder::FIRST_CALL_TRANSACTION + 2,
  STRINGLISTMETHOD = ::android::IBinder::FIRST_CALL_TRANSACTION + 3,
  BINDERLISTMETHOD = ::android::IBinder::FIRST_CALL_TRANSACTION + 4,
  TAKESAFILEDESCRIPTOR = ::android::IBinder::FIRST_CALL_TRANSACTION + 5,
  TAKESAFILEDESCRIPTORARRAY = ::android::IBinder::FIRST_CALL_TRANSACTION + 6,
};
};  // class IComplexTypeInterface

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_I_COMPLEX_TYPE_INTERFACE_H_)";

const char kExpectedComplexTypeInterfaceSourceOutput[] =
R"(#include <android/os/IComplexTypeInterface.h>
#include <android/os/BpComplexTypeInterface.h>

namespace android {

namespace os {

IMPLEMENT_META_INTERFACE(ComplexTypeInterface, "android.os.IComplexTypeInterface");

}  // namespace os

}  // namespace android
)";

}  // namespace

class ASTTest : public ::testing::Test {
 protected:
  ASTTest(string file_path, string file_contents)
      : file_path_(file_path),
        file_contents_(file_contents) {
    types_.Init();
  }

  unique_ptr<AidlInterface> Parse() {
    io_delegate_.SetFileContents(file_path_, file_contents_);

    unique_ptr<AidlInterface> ret;
    std::vector<std::unique_ptr<AidlImport>> imports;
    AidlError err = ::android::aidl::internals::load_and_validate_aidl(
        {},  // no preprocessed files
        {"."},
        file_path_,
        io_delegate_,
        &types_,
        &ret,
        &imports);

    if (err != AidlError::OK)
      return nullptr;

    return ret;
  }

  void Compare(Document* doc, const char* expected) {
    string output;
    unique_ptr<CodeWriter> cw = GetStringWriter(&output);

    doc->Write(cw.get());

    if (expected == output) {
      return; // Success
    }

    test::PrintDiff(expected, output);
    FAIL() << "Document contents did not match expected contents";
  }

  const string file_path_;
  const string file_contents_;
  FakeIoDelegate io_delegate_;
  TypeNamespace types_;
};

class ComplexTypeInterfaceASTTest : public ASTTest {
 public:
  ComplexTypeInterfaceASTTest()
      : ASTTest("android/os/IComplexTypeInterface.aidl",
                kComplexTypeInterfaceAIDL) {
    io_delegate_.SetFileContents("foo/IFooType.aidl",
                                 "package foo; interface IFooType {}");
  }
};

TEST_F(ComplexTypeInterfaceASTTest, GeneratesClientHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildClientHeader(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeClientHeaderOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesClientSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildClientSource(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeClientSourceOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesServerHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildServerHeader(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeServerHeaderOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesServerSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildServerSource(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeServerSourceOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesInterfaceHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildInterfaceHeader(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeInterfaceHeaderOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesInterfaceSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildInterfaceSource(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeInterfaceSourceOutput);
}

namespace test_io_handling {

const char kInputPath[] = "a/IFoo.aidl";
const char kOutputPath[] = "output.cpp";
const char kHeaderDir[] = "headers";
const char kInterfaceHeaderRelPath[] = "a/IFoo.h";

}  // namespace test_io_handling

class IoErrorHandlingTest : public ASTTest {
 public:
  IoErrorHandlingTest ()
      : ASTTest(test_io_handling::kInputPath,
                "package a; interface IFoo {}"),
        options_(GetOptions()) {}

  const unique_ptr<CppOptions> options_;

 private:
  static unique_ptr<CppOptions> GetOptions() {
    using namespace test_io_handling;

    const int argc = 4;
    const char* cmdline[argc] = {
      "aidl-cpp", kInputPath, kHeaderDir, kOutputPath
    };
    return CppOptions::Parse(argc, cmdline);
  }
};

TEST_F(IoErrorHandlingTest, GenerateCorrectlyAbsentErrors) {
  // Confirm that this is working correctly without I/O problems.
  const unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  ASSERT_TRUE(GenerateCpp(*options_, types_, *interface, io_delegate_));
}

TEST_F(IoErrorHandlingTest, HandlesBadHeaderWrite) {
  using namespace test_io_handling;
  const unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);

  // Simulate issues closing the interface header.
  const string header_path =
      StringPrintf("%s%c%s", kHeaderDir, OS_PATH_SEPARATOR,
                   kInterfaceHeaderRelPath);
  io_delegate_.AddBrokenFilePath(header_path);
  ASSERT_FALSE(GenerateCpp(*options_, types_, *interface, io_delegate_));
  // We should never attempt to write the C++ file if we fail writing headers.
  ASSERT_FALSE(io_delegate_.GetWrittenContents(kOutputPath, nullptr));
  // We should remove partial results.
  ASSERT_TRUE(io_delegate_.PathWasRemoved(header_path));
}

TEST_F(IoErrorHandlingTest, HandlesBadCppWrite) {
  using test_io_handling::kOutputPath;
  const unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);

  // Simulate issues closing the cpp file.
  io_delegate_.AddBrokenFilePath(kOutputPath);
  ASSERT_FALSE(GenerateCpp(*options_, types_, *interface, io_delegate_));
  // We should remove partial results.
  ASSERT_TRUE(io_delegate_.PathWasRemoved(kOutputPath));
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android
