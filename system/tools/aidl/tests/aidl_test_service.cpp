/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/Status.h>
#include <nativehelper/ScopedFd.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

#include "android/aidl/tests/BnTestService.h"
#include "android/aidl/tests/ITestService.h"

#include "android/aidl/tests/BnNamedCallback.h"
#include "android/aidl/tests/INamedCallback.h"

// Used implicitly.
#undef LOG_TAG
#define LOG_TAG "aidl_native_service"

// libutils:
using android::Looper;
using android::LooperCallback;
using android::OK;
using android::sp;
using android::String16;

// libbinder:
using android::BnInterface;
using android::defaultServiceManager;
using android::IInterface;
using android::IPCThreadState;
using android::Parcel;
using android::ProcessState;
using android::binder::Status;

// Generated code:
using android::aidl::tests::BnNamedCallback;
using android::aidl::tests::BnTestService;
using android::aidl::tests::INamedCallback;
using android::aidl::tests::SimpleParcelable;
using android::os::PersistableBundle;

// Standard library
using std::map;
using std::string;
using std::unique_ptr;
using std::vector;

namespace {

class BinderCallback : public LooperCallback {
 public:
  BinderCallback() {}
  ~BinderCallback() override {}

  int handleEvent(int /* fd */, int /* events */, void* /* data */) override {
    IPCThreadState::self()->handlePolledCommands();
    return 1;  // Continue receiving callbacks.
  }
};

class NamedCallback : public BnNamedCallback {
 public:
  NamedCallback(String16 name) : name_(name) {}

  Status GetName(String16* ret) {
    *ret = name_;
    return Status::ok();
  }

 private:
  String16 name_;
};

class NativeService : public BnTestService {
 public:
  NativeService() {}
  virtual ~NativeService() = default;

  void LogRepeatedStringToken(const String16& token) {
    ALOGI("Repeating '%s' of length=%zu", android::String8(token).string(),
          token.size());
  }

  template <typename T>
  void LogRepeatedToken(const T& token) {
    std::ostringstream token_str;
    token_str << token;
    ALOGI("Repeating token %s", token_str.str().c_str());
  }

  Status RepeatBoolean(bool token, bool* _aidl_return) override {
    LogRepeatedToken(token ? 1 : 0);
    *_aidl_return = token;
    return Status::ok();
  }
  Status RepeatByte(int8_t token, int8_t* _aidl_return) override {
    LogRepeatedToken(token);
    *_aidl_return = token;
    return Status::ok();
  }
  Status RepeatChar(char16_t token, char16_t* _aidl_return) override {
    LogRepeatedStringToken(String16(&token, 1));
    *_aidl_return = token;
    return Status::ok();
  }
  Status RepeatInt(int32_t token, int32_t* _aidl_return) override {
    LogRepeatedToken(token);
    *_aidl_return = token;
    return Status::ok();
  }
  Status RepeatLong(int64_t token, int64_t* _aidl_return) override {
    LogRepeatedToken(token);
    *_aidl_return = token;
    return Status::ok();
  }
  Status RepeatFloat(float token, float* _aidl_return) override {
    LogRepeatedToken(token);
    *_aidl_return = token;
    return Status::ok();
  }
  Status RepeatDouble(double token, double* _aidl_return) override {
    LogRepeatedToken(token);
    *_aidl_return = token;
    return Status::ok();
  }
  Status RepeatString(const String16& token, String16* _aidl_return) override {
    LogRepeatedStringToken(token);
    *_aidl_return = token;
    return Status::ok();
  }

  Status RepeatSimpleParcelable(const SimpleParcelable& input,
                                SimpleParcelable* repeat,
                                SimpleParcelable* _aidl_return) override {
    ALOGI("Repeated a SimpleParcelable %s", input.toString().c_str());
    *repeat = input;
    *_aidl_return = input;
    return Status::ok();
  }

  Status RepeatPersistableBundle(const PersistableBundle& input,
                                 PersistableBundle* _aidl_return) override {
    ALOGI("Repeated a PersistableBundle");
    *_aidl_return = input;
    return Status::ok();
  }

  template <typename T>
  Status ReverseArray(const vector<T>& input, vector<T>* repeated,
                      vector<T>* _aidl_return) {
    ALOGI("Reversing array of length %zu", input.size());
    *repeated = input;
    *_aidl_return = input;
    std::reverse(_aidl_return->begin(), _aidl_return->end());
    return Status::ok();
  }

  template<typename T>
  Status RepeatNullable(const unique_ptr<T>& input,
                        unique_ptr<T>* _aidl_return) {
    ALOGI("Repeating nullable value");

    _aidl_return->reset();
    if (input) {
      _aidl_return->reset(new T(*input));
    }

    return Status::ok();
  }

  Status ReverseBoolean(const vector<bool>& input,
                        vector<bool>* repeated,
                        vector<bool>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }
  Status ReverseByte(const vector<uint8_t>& input,
                     vector<uint8_t>* repeated,
                     vector<uint8_t>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }
  Status ReverseChar(const vector<char16_t>& input,
                     vector<char16_t>* repeated,
                     vector<char16_t>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }
  Status ReverseInt(const vector<int32_t>& input,
                    vector<int32_t>* repeated,
                    vector<int32_t>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }
  Status ReverseLong(const vector<int64_t>& input,
                     vector<int64_t>* repeated,
                     vector<int64_t>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }
  Status ReverseFloat(const vector<float>& input,
                      vector<float>* repeated,
                      vector<float>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }
  Status ReverseDouble(const vector<double>& input,
                       vector<double>* repeated,
                       vector<double>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }
  Status ReverseString(const vector<String16>& input,
                       vector<String16>* repeated,
                       vector<String16>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }
  Status ReverseSimpleParcelables(
      const vector<SimpleParcelable>& input,
      vector<SimpleParcelable>* repeated,
      vector<SimpleParcelable>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }
  Status ReversePersistableBundles(
      const vector<PersistableBundle>& input,
      vector<PersistableBundle>* repeated,
      vector<PersistableBundle>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }

  Status GetOtherTestService(const String16& name,
                             sp<INamedCallback>* returned_service) override {
    if (service_map_.find(name) == service_map_.end()) {
      sp<INamedCallback> new_item(new NamedCallback(name));
      service_map_[name] = new_item;
    }

    *returned_service = service_map_[name];
    return Status::ok();
  }

  Status VerifyName(const sp<INamedCallback>& service, const String16& name,
                    bool* returned_value) override {
    String16 foundName;
    Status status = service->GetName(&foundName);

    if (status.isOk()) {
      *returned_value = foundName == name;
    }

    return status;
  }

  Status ReverseStringList(const vector<String16>& input,
                           vector<String16>* repeated,
                           vector<String16>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }

  Status ReverseNamedCallbackList(const vector<sp<IBinder>>& input,
                                  vector<sp<IBinder>>* repeated,
                                  vector<sp<IBinder>>* _aidl_return) override {
    return ReverseArray(input, repeated, _aidl_return);
  }

  Status RepeatFileDescriptor(const ScopedFd& read,
                              ScopedFd* _aidl_return) override {
    ALOGE("Repeating file descriptor");
    *_aidl_return = ScopedFd(dup(read.get()));
    return Status::ok();
  }

  Status ReverseFileDescriptorArray(const vector<ScopedFd>& input,
                                    vector<ScopedFd>* repeated,
                                    vector<ScopedFd>* _aidl_return) override {
    ALOGI("Reversing descriptor array of length %zu", input.size());
    for (const auto& item : input) {
      repeated->push_back(ScopedFd(dup(item.get())));
      _aidl_return->push_back(ScopedFd(dup(item.get())));
    }
    std::reverse(_aidl_return->begin(), _aidl_return->end());
    return Status::ok();
  }

  Status ThrowServiceException(int code) override {
    return Status::fromServiceSpecificError(code);
  }

  Status RepeatNullableIntArray(const unique_ptr<vector<int32_t>>& input,
                                unique_ptr<vector<int32_t>>* _aidl_return) {
    return RepeatNullable(input, _aidl_return);
  }

  Status RepeatNullableStringList(
             const unique_ptr<vector<unique_ptr<String16>>>& input,
             unique_ptr<vector<unique_ptr<String16>>>* _aidl_return) {
    ALOGI("Repeating nullable string list");
    if (!input) {
      _aidl_return->reset();
      return Status::ok();
    }

    _aidl_return->reset(new vector<unique_ptr<String16>>);

    for (const auto& item : *input) {
      if (!item) {
        (*_aidl_return)->emplace_back(nullptr);
      } else {
        (*_aidl_return)->emplace_back(new String16(*item));
      }
    }

    return Status::ok();
  }

  Status RepeatNullableString(const unique_ptr<String16>& input,
                              unique_ptr<String16>* _aidl_return) {
    return RepeatNullable(input, _aidl_return);
  }

  Status RepeatNullableParcelable(const unique_ptr<SimpleParcelable>& input,
                              unique_ptr<SimpleParcelable>* _aidl_return) {
    return RepeatNullable(input, _aidl_return);
  }

  Status RepeatUtf8CppString(const string& token,
                             string* _aidl_return) override {
    ALOGI("Repeating utf8 string '%s' of length=%zu", token.c_str(), token.size());
    *_aidl_return = token;
    return Status::ok();
  }

  Status RepeatNullableUtf8CppString(
      const unique_ptr<string>& token,
      unique_ptr<string>* _aidl_return) override {
    if (!token) {
      ALOGI("Received null @utf8InCpp string");
      return Status::ok();
    }
    ALOGI("Repeating utf8 string '%s' of length=%zu",
          token->c_str(), token->size());
    _aidl_return->reset(new string(*token));
    return Status::ok();
  }

  Status ReverseUtf8CppString(const vector<string>& input,
                              vector<string>* repeated,
                              vector<string>* _aidl_return) {
    return ReverseArray(input, repeated, _aidl_return);
  }

  Status ReverseUtf8CppStringList(
      const unique_ptr<vector<unique_ptr<::string>>>& input,
      unique_ptr<vector<unique_ptr<string>>>* repeated,
      unique_ptr<vector<unique_ptr<string>>>* _aidl_return) {
    if (!input) {
      ALOGI("Received null list of utf8 strings");
      return Status::ok();
    }
    _aidl_return->reset(new vector<unique_ptr<string>>);
    repeated->reset(new vector<unique_ptr<string>>);

    for (const auto& item : *input) {
      (*repeated)->emplace_back(nullptr);
      (*_aidl_return)->emplace_back(nullptr);
      if (item) {
        (*repeated)->back().reset(new string(*item));
        (*_aidl_return)->back().reset(new string(*item));
      }
    }
    std::reverse((*_aidl_return)->begin(), (*_aidl_return)->end());

    return Status::ok();
  }

 private:
  map<String16, sp<INamedCallback>> service_map_;
};

int Run() {
  android::sp<NativeService> service = new NativeService;
  sp<Looper> looper(Looper::prepare(0 /* opts */));

  int binder_fd = -1;
  ProcessState::self()->setThreadPoolMaxThreadCount(0);
  IPCThreadState::self()->disableBackgroundScheduling(true);
  IPCThreadState::self()->setupPolling(&binder_fd);
  ALOGI("Got binder FD %d", binder_fd);
  if (binder_fd < 0) return -1;

  sp<BinderCallback> cb(new BinderCallback);
  if (looper->addFd(binder_fd, Looper::POLL_CALLBACK, Looper::EVENT_INPUT, cb,
                    nullptr) != 1) {
    ALOGE("Failed to add binder FD to Looper");
    return -1;
  }

  defaultServiceManager()->addService(service->getInterfaceDescriptor(),
                                      service);

  ALOGI("Entering loop");
  while (true) {
    const int result = looper->pollAll(-1 /* timeoutMillis */);
    ALOGI("Looper returned %d", result);
  }
  return 0;
}

}  // namespace

int main(int /* argc */, char* /* argv */ []) {
  return Run();
}
