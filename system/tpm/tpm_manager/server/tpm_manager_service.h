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

#ifndef TPM_MANAGER_SERVER_TPM_MANAGER_SERVICE_H_
#define TPM_MANAGER_SERVER_TPM_MANAGER_SERVICE_H_

#include <memory>

#include <base/callback.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/threading/thread.h>
#include <brillo/bind_lambda.h>

#include "tpm_manager/common/tpm_nvram_interface.h"
#include "tpm_manager/common/tpm_ownership_interface.h"
#include "tpm_manager/server/local_data_store.h"
#include "tpm_manager/server/tpm_initializer.h"
#include "tpm_manager/server/tpm_nvram.h"
#include "tpm_manager/server/tpm_status.h"

namespace tpm_manager {

// This class implements the core tpm_manager service. All Tpm access is
// asynchronous, except for the initial setup in Initialize().
// Usage:
//   std::unique_ptr<TpmManagerService> tpm_manager = new TpmManagerService();
//   CHECK(tpm_manager->Initialize());
//   tpm_manager->GetTpmStatus(...);
//
// THREADING NOTES:
// This class runs a worker thread and delegates all calls to it. This keeps the
// public methods non-blocking while allowing complex implementation details
// with dependencies on the TPM, network, and filesystem to be coded in a more
// readable way. It also serves to serialize method execution which reduces
// complexity with TPM state.
//
// Tasks that run on the worker thread are bound with base::Unretained which is
// safe because the thread is owned by this class (so it is guaranteed not to
// process a task after destruction). Weak pointers are used to post replies
// back to the main thread.
class TpmManagerService : public TpmNvramInterface,
                          public TpmOwnershipInterface {
 public:
  // If |wait_for_ownership| is set, TPM initialization will be postponed until
  // an explicit TakeOwnership request is received. Does not take ownership of
  // |local_data_store|, |tpm_status| or |tpm_initializer|.
  explicit TpmManagerService(bool wait_for_ownership,
                             LocalDataStore* local_data_store,
                             TpmStatus* tpm_status,
                             TpmInitializer* tpm_initializer,
                             TpmNvram* tpm_nvram);
  ~TpmManagerService() override = default;

  // Performs initialization tasks. This method must be called before calling
  // any other method in this class. Returns true on success.
  bool Initialize();

  // TpmOwnershipInterface methods.
  void GetTpmStatus(const GetTpmStatusRequest& request,
                    const GetTpmStatusCallback& callback) override;
  void TakeOwnership(const TakeOwnershipRequest& request,
                     const TakeOwnershipCallback& callback) override;
  void RemoveOwnerDependency(
      const RemoveOwnerDependencyRequest& request,
      const RemoveOwnerDependencyCallback& callback) override;

  // TpmNvramInterface methods.
  void DefineNvram(const DefineNvramRequest& request,
                   const DefineNvramCallback& callback) override;
  void DestroyNvram(const DestroyNvramRequest& request,
                    const DestroyNvramCallback& callback) override;
  void WriteNvram(const WriteNvramRequest& request,
                  const WriteNvramCallback& callback) override;
  void ReadNvram(const ReadNvramRequest& request,
                 const ReadNvramCallback& callback) override;
  void IsNvramDefined(const IsNvramDefinedRequest& request,
                      const IsNvramDefinedCallback& callback) override;
  void IsNvramLocked(const IsNvramLockedRequest& request,
                     const IsNvramLockedCallback& callback) override;
  void GetNvramSize(const GetNvramSizeRequest& request,
                    const GetNvramSizeCallback& callback) override;

 private:
  // A relay callback which allows the use of weak pointer semantics for a reply
  // to TaskRunner::PostTaskAndReply.
  template<typename ReplyProtobufType>
  void TaskRelayCallback(
      const base::Callback<void(const ReplyProtobufType&)> callback,
      const std::shared_ptr<ReplyProtobufType>& reply);

  // This templated method posts the provided |TaskType| to the background
  // thread with the provided |RequestProtobufType|. When |TaskType| finishes
  // executing, the |ReplyCallbackType| is called with the |ReplyProtobufType|.
  template<typename ReplyProtobufType,
           typename RequestProtobufType,
           typename ReplyCallbackType,
           typename TaskType>
  void PostTaskToWorkerThread(RequestProtobufType& request,
                              ReplyCallbackType& callback,
                              TaskType task);

  // Synchronously initializes the TPM according to the current configuration.
  // If an initialization process was interrupted it will be continued. If the
  // TPM is already initialized or cannot yet be initialized, this method has no
  // effect.
  void InitializeTask();

  // Blocking implementation of GetTpmStatus that can be executed on the
  // background worker thread.
  void GetTpmStatusTask(const GetTpmStatusRequest& request,
                        const std::shared_ptr<GetTpmStatusReply>& result);

  // Blocking implementation of TakeOwnership that can be executed on the
  // background worker thread.
  void TakeOwnershipTask(const TakeOwnershipRequest& request,
                         const std::shared_ptr<TakeOwnershipReply>& result);

  // Blocking implementation of RemoveOwnerDependency that can be executed on
  // the background worker thread.
  void RemoveOwnerDependencyTask(
      const RemoveOwnerDependencyRequest& request,
      const std::shared_ptr<RemoveOwnerDependencyReply>& result);

  // Removes a |owner_dependency| from the list of owner dependencies in
  // |local_data|. If |owner_dependency| is not present in |local_data|,
  // this method does nothing.
  static void RemoveOwnerDependency(const std::string& owner_dependency,
                                    LocalData* local_data);

  // Blocking implementation of DefineNvram that can be executed on the
  // background worker thread.
  void DefineNvramTask(const DefineNvramRequest& request,
                       const std::shared_ptr<DefineNvramReply>& result);

  // Blocking implementation of DestroyNvram that can be executed on the
  // background worker thread.
  void DestroyNvramTask(const DestroyNvramRequest& request,
                        const std::shared_ptr<DestroyNvramReply>& result);

  // Blocking implementation of WriteNvram that can be executed on the
  // background worker thread.
  void WriteNvramTask(const WriteNvramRequest& request,
                      const std::shared_ptr<WriteNvramReply>& result);

  // Blocking implementation of ReadNvram that can be executed on the
  // background worker thread.
  void ReadNvramTask(const ReadNvramRequest& request,
                     const std::shared_ptr<ReadNvramReply>& result);

  // Blocking implementation of IsNvramDefined that can be executed on the
  // background worker thread.
  void IsNvramDefinedTask(const IsNvramDefinedRequest& request,
                          const std::shared_ptr<IsNvramDefinedReply>& result);

  // Blocking implementation of IsNvramLocked that can be executed on the
  // background worker thread.
  void IsNvramLockedTask(const IsNvramLockedRequest& request,
                         const std::shared_ptr<IsNvramLockedReply>& result);

  // Blocking implementation of GetNvramSize that can be executed on the
  // background worker thread.
  void GetNvramSizeTask(const GetNvramSizeRequest& request,
                        const std::shared_ptr<GetNvramSizeReply>& result);

  LocalDataStore* local_data_store_;
  TpmStatus* tpm_status_;
  TpmInitializer* tpm_initializer_;
  TpmNvram* tpm_nvram_;
  // Whether to wait for an explicit call to 'TakeOwnership' before initializing
  // the TPM. Normally tracks the --wait_for_ownership command line option.
  bool wait_for_ownership_;
  // Background thread to allow processing of potentially lengthy TPM requests
  // in the background.
  std::unique_ptr<base::Thread> worker_thread_;
  // Declared last so any weak pointers are destroyed first.
  base::WeakPtrFactory<TpmManagerService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TpmManagerService);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM_MANAGER_SERVICE_H_
