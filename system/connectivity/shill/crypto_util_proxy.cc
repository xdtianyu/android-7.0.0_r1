//
// Copyright (C) 2013 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "shill/crypto_util_proxy.h"

#include <iterator>
#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/posix/eintr_wrapper.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <brillo/data_encoding.h>

#include "shill/event_dispatcher.h"
#include "shill/file_io.h"
#include "shill/process_manager.h"

using base::Bind;
using base::Callback;
using base::StringPrintf;
using shill_protos::EncryptDataMessage;
using shill_protos::EncryptDataResponse;
using shill_protos::VerifyCredentialsMessage;
using shill_protos::VerifyCredentialsResponse;
using std::distance;
using std::string;
using std::vector;

namespace shill {

// statics
const char CryptoUtilProxy::kCommandEncrypt[] = "encrypt";
const char CryptoUtilProxy::kCommandVerify[] = "verify";
const char CryptoUtilProxy::kCryptoUtilShimPath[] = SHIMDIR "/crypto-util";
const char CryptoUtilProxy::kDestinationVerificationUser[] = "shill-crypto";
const uint64_t CryptoUtilProxy::kRequiredCapabilities = 0;
const int CryptoUtilProxy::kShimJobTimeoutMilliseconds = 30 * 1000;

namespace {
void DoNothingWithExitStatus(int /* exit_status */) {
}
}  // namespace

CryptoUtilProxy::CryptoUtilProxy(EventDispatcher* dispatcher)
    : dispatcher_(dispatcher),
      process_manager_(ProcessManager::GetInstance()),
      file_io_(FileIO::GetInstance()),
      input_buffer_(),
      next_input_byte_(),
      output_buffer_(),
      shim_stdin_(-1),
      shim_stdout_(-1),
      shim_pid_(0) {
}

CryptoUtilProxy::~CryptoUtilProxy() {
  // Just in case we had a pending operation.
  HandleShimError(Error(Error::kOperationAborted));
}

bool CryptoUtilProxy::VerifyDestination(
    const string& certificate,
    const string& public_key,
    const string& nonce,
    const string& signed_data,
    const string& destination_udn,
    const vector<uint8_t>& ssid,
    const string& bssid,
    const ResultBoolCallback& result_callback,
    Error* error) {
  string unsigned_data(reinterpret_cast<const char*>(&ssid[0]),
                       ssid.size());
  string upper_case_bssid(base::ToUpperASCII(bssid));
  unsigned_data.append(StringPrintf(",%s,%s,%s,%s",
                                    destination_udn.c_str(),
                                    upper_case_bssid.c_str(),
                                    public_key.c_str(),
                                    nonce.c_str()));
  string decoded_signed_data;
  if (!brillo::data_encoding::Base64Decode(signed_data, &decoded_signed_data)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                          "Failed to decode signed data.");
    return false;
  }

  VerifyCredentialsMessage message;
  message.set_certificate(certificate);
  message.set_signed_data(decoded_signed_data);
  message.set_unsigned_data(unsigned_data);
  message.set_mac_address(bssid);

  string raw_bytes;
  if (!message.SerializeToString(&raw_bytes)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                          "Failed to send arguments to shim.");
    return false;
  }
  StringCallback wrapped_result_handler = Bind(
      &CryptoUtilProxy::HandleVerifyResult,
      AsWeakPtr(), result_callback);
  if (!StartShimForCommand(kCommandVerify, raw_bytes,
                           wrapped_result_handler)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                          "Failed to start shim to verify credentials.");
    return false;
  }
  LOG(INFO) << "Started credential verification";
  return true;
}

bool CryptoUtilProxy::EncryptData(
    const string& public_key,
    const string& data,
    const ResultStringCallback& result_callback,
    Error* error) {
  string decoded_public_key;
  if (!brillo::data_encoding::Base64Decode(public_key, &decoded_public_key)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                          "Unable to decode public key.");
    return false;
  }

  EncryptDataMessage message;
  message.set_public_key(decoded_public_key);
  message.set_data(data);
  string raw_bytes;
  if (!message.SerializeToString(&raw_bytes)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                          "Failed to send arguments to shim.");
    return false;
  }
  StringCallback wrapped_result_handler = Bind(
      &CryptoUtilProxy::HandleEncryptResult,
      AsWeakPtr(), result_callback);
  if (!StartShimForCommand(kCommandEncrypt, raw_bytes,
                           wrapped_result_handler)) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                          "Failed to start shim to verify credentials.");
    return false;
  }
  LOG(INFO) << "Started data signing";
  return true;
}

bool CryptoUtilProxy::StartShimForCommand(
    const string& command,
    const string& input,
    const StringCallback& result_handler) {
  if (shim_pid_) {
    LOG(ERROR) << "Can't run concurrent shim operations.";
    return false;
  }
  if (input.length() < 1) {
    LOG(ERROR) << "Refusing to start a shim with no input data.";
    return false;
  }
  shim_pid_ = process_manager_->StartProcessInMinijailWithPipes(
      FROM_HERE,
      base::FilePath(kCryptoUtilShimPath),
      vector<string>{command},
      kDestinationVerificationUser,
      kDestinationVerificationUser,
      kRequiredCapabilities,
      base::Bind(DoNothingWithExitStatus),
      &shim_stdin_,
      &shim_stdout_,
      nullptr);
  if (shim_pid_ == -1) {
    LOG(ERROR) << "Minijail couldn't run our child process";
    return false;
  }
  // Invariant: if the shim process could be in flight, shim_pid_ != 0 and we
  // have a callback scheduled to kill the shim process.
  input_buffer_ = input;
  next_input_byte_ = input_buffer_.begin();
  output_buffer_.clear();
  result_handler_ = result_handler;
  shim_job_timeout_callback_.Reset(Bind(&CryptoUtilProxy::HandleShimTimeout,
                                        AsWeakPtr()));
  dispatcher_->PostDelayedTask(shim_job_timeout_callback_.callback(),
                                kShimJobTimeoutMilliseconds);
  do {
    if (file_io_->SetFdNonBlocking(shim_stdin_) ||
        file_io_->SetFdNonBlocking(shim_stdout_)) {
      LOG(ERROR) << "Unable to set shim pipes to be non blocking.";
      break;
    }
    shim_stdout_handler_.reset(dispatcher_->CreateInputHandler(
        shim_stdout_,
        Bind(&CryptoUtilProxy::HandleShimOutput, AsWeakPtr()),
        Bind(&CryptoUtilProxy::HandleShimReadError, AsWeakPtr())));
    shim_stdin_handler_.reset(dispatcher_->CreateReadyHandler(
        shim_stdin_,
        IOHandler::kModeOutput,
        Bind(&CryptoUtilProxy::HandleShimStdinReady, AsWeakPtr())));
    LOG(INFO) << "Started crypto-util shim at " << shim_pid_;
    return true;
  } while (false);
  // We've started a shim, but failed to set up the plumbing to communicate
  // with it.  Since we can't go forward, go backward and clean it up.
  // Kill the callback, since we're signalling failure by returning false.
  result_handler_.Reset();
  HandleShimError(Error(Error::kOperationAborted));
  return false;
}

void CryptoUtilProxy::CleanupShim(const Error& shim_result) {
  LOG(INFO) << __func__;
  shim_result_.CopyFrom(shim_result);
  if (shim_stdin_ > -1) {
    file_io_->Close(shim_stdin_);
    shim_stdin_ = -1;
  }
  if (shim_stdout_ > -1) {
    file_io_->Close(shim_stdout_);
    shim_stdout_ = -1;
  }
  // Leave the output buffer so that we use it with the result handler.
  input_buffer_.clear();

  shim_stdout_handler_.reset();
  shim_stdin_handler_.reset();

  if (shim_pid_) {
    process_manager_->UpdateExitCallback(shim_pid_,
                                         Bind(&CryptoUtilProxy::OnShimDeath,
                                              AsWeakPtr()));
    process_manager_->StopProcess(shim_pid_);
  } else {
    const int kExitStatus = -1;
    OnShimDeath(kExitStatus);
  }
}

void CryptoUtilProxy::OnShimDeath(int /* exit_status */) {
  // Make sure the proxy is completely clean before calling back out.  This
  // requires we copy some state locally.
  shim_pid_ = 0;
  shim_job_timeout_callback_.Cancel();
  StringCallback handler(result_handler_);
  result_handler_.Reset();
  string output(output_buffer_);
  output_buffer_.clear();
  Error result;
  result.CopyFrom(shim_result_);
  shim_result_.Reset();
  if (!handler.is_null()) {
    handler.Run(output, result);
  }
}

void CryptoUtilProxy::HandleShimStdinReady(int fd) {
  CHECK(fd == shim_stdin_);
  CHECK(shim_pid_);
  size_t bytes_to_write = distance<string::const_iterator>(next_input_byte_,
                                                           input_buffer_.end());
  ssize_t bytes_written = file_io_->Write(shim_stdin_,
                                          &(*next_input_byte_),
                                          bytes_to_write);
  if (bytes_written < 0) {
    HandleShimError(Error(Error::kOperationFailed,
                          "Failed to write any bytes to output buffer"));
    return;
  }
  next_input_byte_ += bytes_written;
  if (next_input_byte_ == input_buffer_.end()) {
    LOG(INFO) << "Finished writing output buffer to shim.";
    // Done writing out the proto buffer, close the pipe so that the shim
    // knows that's all there is.  Close our handler first.
    shim_stdin_handler_.reset();
    file_io_->Close(shim_stdin_);
    shim_stdin_ = -1;
    input_buffer_.clear();
    next_input_byte_ = input_buffer_.begin();
  }
}

void CryptoUtilProxy::HandleShimOutput(InputData* data) {
  CHECK(shim_pid_);
  CHECK(!result_handler_.is_null());
  if (data->len > 0) {
    // Everyone is shipping features and I'm just here copying bytes from one
    // buffer to another.
    output_buffer_.append(reinterpret_cast<char*>(data->buf), data->len);
    return;
  }
  // EOF -> we're done!
  LOG(INFO) << "Finished reading " << output_buffer_.length()
            << " bytes from shim.";
  shim_stdout_handler_.reset();
  file_io_->Close(shim_stdout_);
  shim_stdout_ = -1;
  Error no_error;
  CleanupShim(no_error);
}

void CryptoUtilProxy::HandleShimError(const Error& error) {
  // Abort abort abort.  There is very little we can do here.
  output_buffer_.clear();
  CleanupShim(error);
}

void CryptoUtilProxy::HandleShimReadError(const string& error_msg) {
  Error e(Error::kOperationFailed, error_msg);
  HandleShimError(e);
}

void CryptoUtilProxy::HandleShimTimeout() {
  Error e(Error::kOperationTimeout);
  HandleShimError(e);
}

void CryptoUtilProxy::HandleVerifyResult(
    const ResultBoolCallback& result_handler,
    const std::string& result,
    const Error& error) {
  if (!error.IsSuccess()) {
    result_handler.Run(error, false);
    return;
  }
  VerifyCredentialsResponse response;
  Error e;

  if (!response.ParseFromString(result) || !response.has_ret()) {
    e.Populate(Error::kInternalError, "Failed parsing shim result.");
    result_handler.Run(e, false);
    return;
  }

  result_handler.Run(e, ParseResponseReturnCode(response.ret(), &e));
}

// static
bool CryptoUtilProxy::ParseResponseReturnCode(int proto_return_code,
                                              Error* e) {
  bool success = false;
  switch (proto_return_code) {
  case shill_protos::OK:
    success = true;
    break;
  case shill_protos::ERROR_UNKNOWN:
    e->Populate(Error::kInternalError, "Internal shim error.");
    break;
  case shill_protos::ERROR_OUT_OF_MEMORY:
    e->Populate(Error::kInternalError, "Shim is out of memory.");
    break;
  case shill_protos::ERROR_CRYPTO_OPERATION_FAILED:
    e->Populate(Error::kOperationFailed, "Invalid credentials.");
    break;
  case shill_protos::ERROR_INVALID_ARGUMENTS:
    e->Populate(Error::kInvalidArguments, "Invalid arguments.");
    break;
  default:
    e->Populate(Error::kInternalError, "Unknown error.");
    break;
  }
  return success;
}

void CryptoUtilProxy::HandleEncryptResult(
    const ResultStringCallback& result_handler,
    const std::string& result,
    const Error& error) {
  if (!error.IsSuccess()) {
    result_handler.Run(error, "");
    return;
  }
  EncryptDataResponse response;
  Error e;

  if (!response.ParseFromString(result) || !response.has_ret()) {
    e.Populate(Error::kInternalError, "Failed parsing shim result.");
    result_handler.Run(e, "");
    return;
  }

  if (!ParseResponseReturnCode(response.ret(), &e)) {
    result_handler.Run(e, "");
    return;
  }

  if (!response.has_encrypted_data() ||
      response.encrypted_data().empty()) {
    e.Populate(Error::kInternalError,
               "Shim returned successfully, but included no encrypted data.");
    result_handler.Run(e, "");
    return;
  }

  string encoded_data(
      brillo::data_encoding::Base64Encode(response.encrypted_data()));
  result_handler.Run(e, encoded_data);
}

}  // namespace shill
