//
// Copyright (C) 2015 The Android Open Source Project
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

#include "tpm_manager/server/tpm_manager_service.h"

#include <base/callback.h>
#include <base/command_line.h>
#include <brillo/bind_lambda.h>

namespace tpm_manager {

TpmManagerService::TpmManagerService(bool wait_for_ownership,
                                     LocalDataStore* local_data_store,
                                     TpmStatus* tpm_status,
                                     TpmInitializer* tpm_initializer,
                                     TpmNvram* tpm_nvram)
    : local_data_store_(local_data_store),
      tpm_status_(tpm_status),
      tpm_initializer_(tpm_initializer),
      tpm_nvram_(tpm_nvram),
      wait_for_ownership_(wait_for_ownership),
      weak_factory_(this) {}

bool TpmManagerService::Initialize() {
  LOG(INFO) << "TpmManager service started.";
  worker_thread_.reset(new base::Thread("TpmManager Service Worker"));
  worker_thread_->StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
  base::Closure task = base::Bind(&TpmManagerService::InitializeTask,
                                  base::Unretained(this));
  worker_thread_->task_runner()->PostNonNestableTask(FROM_HERE, task);
  return true;
}

void TpmManagerService::InitializeTask() {
  if (!tpm_status_->IsTpmEnabled()) {
    LOG(WARNING) << __func__ << ": TPM is disabled.";
    return;
  }
  if (!wait_for_ownership_) {
    VLOG(1) << "Initializing TPM.";
    if (!tpm_initializer_->InitializeTpm()) {
      LOG(WARNING) << __func__ << ": TPM initialization failed.";
      return;
    }
  }
}

void TpmManagerService::GetTpmStatus(const GetTpmStatusRequest& request,
                                     const GetTpmStatusCallback& callback) {
  PostTaskToWorkerThread<GetTpmStatusReply>(
      request, callback, &TpmManagerService::GetTpmStatusTask);
}

void TpmManagerService::GetTpmStatusTask(
    const GetTpmStatusRequest& request,
    const std::shared_ptr<GetTpmStatusReply>& result) {
  VLOG(1) << __func__;
  result->set_enabled(tpm_status_->IsTpmEnabled());
  result->set_owned(tpm_status_->IsTpmOwned());
  LocalData local_data;
  if (local_data_store_ && local_data_store_->Read(&local_data)) {
    *result->mutable_local_data() = local_data;
  }
  int counter;
  int threshold;
  bool lockout;
  int lockout_time_remaining;
  if (tpm_status_->GetDictionaryAttackInfo(&counter, &threshold, &lockout,
                                           &lockout_time_remaining)) {
    result->set_dictionary_attack_counter(counter);
    result->set_dictionary_attack_threshold(threshold);
    result->set_dictionary_attack_lockout_in_effect(lockout);
    result->set_dictionary_attack_lockout_seconds_remaining(
        lockout_time_remaining);
  }
  result->set_status(STATUS_SUCCESS);
}

void TpmManagerService::TakeOwnership(const TakeOwnershipRequest& request,
                                      const TakeOwnershipCallback& callback) {
  PostTaskToWorkerThread<TakeOwnershipReply>(
      request, callback, &TpmManagerService::TakeOwnershipTask);
}

void TpmManagerService::TakeOwnershipTask(
    const TakeOwnershipRequest& request,
    const std::shared_ptr<TakeOwnershipReply>& result) {
  VLOG(1) << __func__;
  if (!tpm_status_->IsTpmEnabled()) {
    result->set_status(STATUS_NOT_AVAILABLE);
    return;
  }
  if (!tpm_initializer_->InitializeTpm()) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_status(STATUS_SUCCESS);
}

void TpmManagerService::RemoveOwnerDependency(
    const RemoveOwnerDependencyRequest& request,
    const RemoveOwnerDependencyCallback& callback) {
  PostTaskToWorkerThread<RemoveOwnerDependencyReply>(
      request, callback, &TpmManagerService::RemoveOwnerDependencyTask);
}

void TpmManagerService::RemoveOwnerDependencyTask(
    const RemoveOwnerDependencyRequest& request,
    const std::shared_ptr<RemoveOwnerDependencyReply>& result) {
  VLOG(1) << __func__;
  LocalData local_data;
  if (!local_data_store_->Read(&local_data)) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  RemoveOwnerDependency(request.owner_dependency(), &local_data);
  if (!local_data_store_->Write(local_data)) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_status(STATUS_SUCCESS);
}

void TpmManagerService::RemoveOwnerDependency(
    const std::string& owner_dependency, LocalData* local_data) {
  google::protobuf::RepeatedPtrField<std::string>* dependencies =
      local_data->mutable_owner_dependency();
  for (int i = 0; i < dependencies->size(); i++) {
    if (dependencies->Get(i) == owner_dependency) {
      dependencies->SwapElements(i, (dependencies->size() - 1));
      dependencies->RemoveLast();
      break;
    }
  }
  if (dependencies->empty()) {
    local_data->clear_owner_password();
    local_data->clear_endorsement_password();
    local_data->clear_lockout_password();
  }
}

void TpmManagerService::DefineNvram(const DefineNvramRequest& request,
                                    const DefineNvramCallback& callback) {
  PostTaskToWorkerThread<DefineNvramReply>(
      request, callback, &TpmManagerService::DefineNvramTask);
}

void TpmManagerService::DefineNvramTask(
    const DefineNvramRequest& request,
    const std::shared_ptr<DefineNvramReply>& result) {
  VLOG(1) << __func__;
  if (!tpm_nvram_->DefineNvram(request.index(), request.length())) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_status(STATUS_SUCCESS);
}

void TpmManagerService::DestroyNvram(const DestroyNvramRequest& request,
                                     const DestroyNvramCallback& callback) {
  PostTaskToWorkerThread<DestroyNvramReply>(
      request, callback, &TpmManagerService::DestroyNvramTask);
}

void TpmManagerService::DestroyNvramTask(
    const DestroyNvramRequest& request,
    const std::shared_ptr<DestroyNvramReply>& result) {
  VLOG(1) << __func__;
  if (!tpm_nvram_->DestroyNvram(request.index())) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_status(STATUS_SUCCESS);
}

void TpmManagerService::WriteNvram(const WriteNvramRequest& request,
                                   const WriteNvramCallback& callback) {
  PostTaskToWorkerThread<WriteNvramReply>(
      request, callback, &TpmManagerService::WriteNvramTask);
}

void TpmManagerService::WriteNvramTask(
    const WriteNvramRequest& request,
    const std::shared_ptr<WriteNvramReply>& result) {
  VLOG(1) << __func__;
  if (!tpm_nvram_->WriteNvram(request.index(), request.data())) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_status(STATUS_SUCCESS);
}

void TpmManagerService::ReadNvram(const ReadNvramRequest& request,
                                  const ReadNvramCallback& callback) {
  PostTaskToWorkerThread<ReadNvramReply>(
      request, callback, &TpmManagerService::ReadNvramTask);
}

void TpmManagerService::ReadNvramTask(
    const ReadNvramRequest& request,
    const std::shared_ptr<ReadNvramReply>& result) {
  VLOG(1) << __func__;
  if (!tpm_nvram_->ReadNvram(request.index(), result->mutable_data())) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_status(STATUS_SUCCESS);
}

void TpmManagerService::IsNvramDefined(const IsNvramDefinedRequest& request,
                                       const IsNvramDefinedCallback& callback) {
  PostTaskToWorkerThread<IsNvramDefinedReply>(
      request, callback, &TpmManagerService::IsNvramDefinedTask);
}

void TpmManagerService::IsNvramDefinedTask(
    const IsNvramDefinedRequest& request,
    const std::shared_ptr<IsNvramDefinedReply>& result) {
  VLOG(1) << __func__;
  bool defined;
  if (!tpm_nvram_->IsNvramDefined(request.index(), &defined)) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_is_defined(defined);
  result->set_status(STATUS_SUCCESS);
}

void TpmManagerService::IsNvramLocked(const IsNvramLockedRequest& request,
                                      const IsNvramLockedCallback& callback) {
  PostTaskToWorkerThread<IsNvramLockedReply>(
      request, callback, &TpmManagerService::IsNvramLockedTask);
}

void TpmManagerService::IsNvramLockedTask(
    const IsNvramLockedRequest& request,
    const std::shared_ptr<IsNvramLockedReply>& result) {
  VLOG(1) << __func__;
  bool locked;
  if (!tpm_nvram_->IsNvramLocked(request.index(), &locked)) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_is_locked(locked);
  result->set_status(STATUS_SUCCESS);
}

void TpmManagerService::GetNvramSize(const GetNvramSizeRequest& request,
                                     const GetNvramSizeCallback& callback) {
  PostTaskToWorkerThread<GetNvramSizeReply>(
      request, callback, &TpmManagerService::GetNvramSizeTask);
}

void TpmManagerService::GetNvramSizeTask(
    const GetNvramSizeRequest& request,
    const std::shared_ptr<GetNvramSizeReply>& result) {
  VLOG(1) << __func__;
  size_t size;
  if (!tpm_nvram_->GetNvramSize(request.index(), &size)) {
    result->set_status(STATUS_UNEXPECTED_DEVICE_ERROR);
    return;
  }
  result->set_size(size);
  result->set_status(STATUS_SUCCESS);
}

template<typename ReplyProtobufType>
void TpmManagerService::TaskRelayCallback(
    const base::Callback<void(const ReplyProtobufType&)> callback,
    const std::shared_ptr<ReplyProtobufType>& reply) {
  callback.Run(*reply);
}

template<typename ReplyProtobufType,
         typename RequestProtobufType,
         typename ReplyCallbackType,
         typename TaskType>
void TpmManagerService::PostTaskToWorkerThread(RequestProtobufType& request,
                                               ReplyCallbackType& callback,
                                               TaskType task) {
  auto result = std::make_shared<ReplyProtobufType>();
  base::Closure background_task = base::Bind(task,
                                             base::Unretained(this),
                                             request,
                                             result);
  base::Closure reply = base::Bind(
      &TpmManagerService::TaskRelayCallback<ReplyProtobufType>,
      weak_factory_.GetWeakPtr(),
      callback,
      result);
  worker_thread_->task_runner()->PostTaskAndReply(FROM_HERE,
                                                  background_task,
                                                  reply);
}

}  // namespace tpm_manager
