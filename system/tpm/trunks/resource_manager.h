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

#ifndef TRUNKS_RESOURCE_MANAGER_H_
#define TRUNKS_RESOURCE_MANAGER_H_

#include "trunks/command_transceiver.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include <base/location.h>
#include <base/macros.h>
#include <base/time/time.h>

#include "trunks/tpm_generated.h"
#include "trunks/trunks_factory.h"

namespace trunks {

// The ResourceManager class manages access to limited TPM resources.
//
// It is reactive to and synchronous with active TPM commands, it does not
// perform any background processing. It needs to inspect every TPM command and
// reply. It maintains all actual TPM handles and provides its own handles to
// callers. If a command fails because a resource is not available the resource
// manager will perform the necessary evictions and run the command again. If a
// command needs an object that has been evicted, that object will be loaded
// before the command is sent to the TPM.
//
// In terms of interface the ResourceManager is simply a CommandTranceiver but
// with the limitation that all calls are synchronous. The SendCommand method
// is supported but does not return until the callback has been called. Keeping
// ResourceManager synchronous simplifies the code and improves readability.
// This class works well with a BackgroundCommandTransceiver.
class ResourceManager : public CommandTransceiver {
 public:
  // The given |factory| will be used to create objects so mocks can be easily
  // injected. This class retains a reference to the factory; the factory must
  // remain valid for the duration of the ResourceManager lifetime. The
  // |next_transceiver| will be used to forward commands to the TPM, this class
  // does NOT take ownership of the pointer.
  ResourceManager(const TrunksFactory& factory,
                  CommandTransceiver* next_transceiver);
  ~ResourceManager() override;

  void Initialize();

  // CommandTransceiver methods.
  void SendCommand(const std::string& command,
                   const ResponseCallback& callback) override;

  std::string SendCommandAndWait(const std::string& command) override;

 private:
  struct MessageInfo {
    bool has_sessions;
    TPM_CC code;  // For a response message this is the TPM_RC response code.
    std::vector<TPM_HANDLE> handles;
    std::vector<TPM_HANDLE> session_handles;
    std::vector<bool> session_continued;
    std::string parameter_data;
  };

  struct HandleInfo {
    HandleInfo();
    // Initializes info for a loaded handle.
    void Init(TPM_HANDLE handle);

    bool is_loaded;
    // Valid only if |is_loaded| is true.
    TPM_HANDLE tpm_handle;
    // Valid only if |is_loaded| is false.
    TPMS_CONTEXT context;
    // Time when the handle is create.
    base::TimeTicks time_of_create;
    // Time when the handle was last used.
    base::TimeTicks time_of_last_use;
  };

  // Chooses an appropriate session for eviction (or flush) which is not one of
  // |sessions_to_retain| and assigns it to |session_to_evict|. Returns true on
  // success.
  bool ChooseSessionToEvict(const std::vector<TPM_HANDLE>& sessions_to_retain,
                            TPM_HANDLE* session_to_evict);

  // Cleans up all references to and information about |flushed_handle|.
  void CleanupFlushedHandle(TPM_HANDLE flushed_handle);

  // Creates a new virtual object handle. If the handle space is exhausted a
  // valid handle is flushed and re-used.
  TPM_HANDLE CreateVirtualHandle();

  // Given a session handle, ensures the session is loaded in the TPM.
  TPM_RC EnsureSessionIsLoaded(const MessageInfo& command_info,
                               TPM_HANDLE session_handle);

  // Evicts all loaded objects except those required by |command_info|. The
  // eviction is best effort; any errors will be ignored.
  void EvictObjects(const MessageInfo& command_info);

  // Evicts a session other than those required by |command_info|. The eviction
  // is best effort; any errors will be ignored.
  void EvictSession(const MessageInfo& command_info);

  // Returns a list of handles parsed from a given |buffer|. No more than
  // |number_of_handles| will be parsed.
  std::vector<TPM_HANDLE> ExtractHandlesFromBuffer(size_t number_of_handles,
                                                   std::string* buffer);

  // A context gap may occur when context counters for active sessions drift too
  // far apart for the TPM to manage. Basically, the TPM needs to reassign new
  // counters to saved sessions. See the TPM Library Specification Part 1
  // Section 30.5 Session Context Management for details.
  void FixContextGap(const MessageInfo& command_info);

  // Performs best-effort handling of actionable warnings. The |command_info|
  // must correspond with the current command being processed by the resource
  // manager. Returns true only if |result| represents an actionable warning and
  // it has been handled.
  bool FixWarnings(const MessageInfo& command_info, TPM_RC result);

  // Flushes a session other than those required by |command_info|. The flush is
  // best effort; any errors will be ignored.
  void FlushSession(const MessageInfo& command_info);

  // When a caller saves context, the resource manager retains that context and
  // possible trades it for new context data to fix a context gap (see
  // FixContextGap). So when the caller wants to load the original context again
  // it needs to be swapped with the latest actual context maintained by the
  // resource manager. This method finds the correct TPM context for a given
  // |external_context| previously returned to the caller. If not found,
  // |external_context| is returned.
  std::string GetActualContextFromExternalContext(
      const std::string& external_context);

  // Returns true iff |handle| is a transient object handle.
  bool IsObjectHandle(TPM_HANDLE handle) const;

  // Returns true iff |handle| is a session handle.
  bool IsSessionHandle(TPM_HANDLE handle) const;

  // Loads the context for a session or object handle. On success returns
  // TPM_RC_SUCCESS and ensures |handle_info| holds a valid handle (and invalid
  // context data).
  TPM_RC LoadContext(const MessageInfo& command_info, HandleInfo* handle_info);

  // Returns a resource manager error code given a particular |tpm_error| and
  // logs the occurrence of the error.
  TPM_RC MakeError(TPM_RC tpm_error,
                   const ::tracked_objects::Location& location);

  // Parses a |command|, sanity checking its format and extracting
  // |message_info| on success. Returns TPM_RC_SUCCESS on success.
  TPM_RC ParseCommand(const std::string& command, MessageInfo* message_info);

  // Parses a |response| to a command associated with |command_info|. The
  // response is sanity checked and |response_info| is extracted. Returns
  // TPM_RC_SUCCESS on success.
  TPM_RC ParseResponse(const MessageInfo& command_info,
                       const std::string& response,
                       MessageInfo* response_info);

  // Performs processing after a successful external ContextSave operation.
  // A subsequent call to GetActualContextFromExternalContext will succeed for
  // the context.
  void ProcessExternalContextSave(const MessageInfo& command_info,
                                  const MessageInfo& response_info);

  // Process an external flush context |command|.
  std::string ProcessFlushContext(const std::string& command,
                                  const MessageInfo& command_info);

  // Given a |virtual_handle| created by this resource manager, finds the
  // associated TPM |actual_handle|, restoring the object if necessary. The
  // current |command_info| must be provided. If |virtual_handle| is not an
  // object handle, then |actual_handle| is set to |virtual_handle|. Returns
  // TPM_RC_SUCCESS on success.
  TPM_RC ProcessInputHandle(const MessageInfo& command_info,
                            TPM_HANDLE virtual_handle,
                            TPM_HANDLE* actual_handle);

  // Given a TPM object handle, returns an associated virtual handle, generating
  // a new one if necessary.
  TPM_HANDLE ProcessOutputHandle(TPM_HANDLE object_handle);

  // Replaces all handles in a given |message| with |new_handles| and returns
  // the resulting modified message. The modified message is guaranteed to have
  // the same length as the input message.
  std::string ReplaceHandles(const std::string& message,
                             const std::vector<TPM_HANDLE>& new_handles);

  // Saves the context for a session or object handle. On success returns
  // TPM_RC_SUCCESS and ensures |handle_info| holds valid context data.
  TPM_RC SaveContext(const MessageInfo& command_info, HandleInfo* handle_info);

  const TrunksFactory& factory_;
  CommandTransceiver* next_transceiver_ = nullptr;
  TPM_HANDLE next_virtual_handle_ = TRANSIENT_FIRST;

  // A mapping of known virtual handles to corresponding HandleInfo.
  std::map<TPM_HANDLE, HandleInfo> virtual_object_handles_;
  // A mapping of loaded tpm object handles to the corresponding virtual handle.
  std::map<TPM_HANDLE, TPM_HANDLE> tpm_object_handles_;
  // A mapping of known session handles to corresponding HandleInfo.
  std::map<TPM_HANDLE, HandleInfo> session_handles_;
  // A mapping of external context blobs to current context blobs.
  std::map<std::string, std::string> external_context_to_actual_;
  // A mapping of actual context blobs to external context blobs.
  std::map<std::string, std::string> actual_context_to_external_;

  // The set of warnings already handled in the context of a FixWarnings() call.
  // Tracking this allows us to avoid re-entrance.
  std::set<TPM_RC> warnings_already_seen_;
  // Whether a FixWarnings() call is currently executing.
  bool fixing_warnings_ = false;

  DISALLOW_COPY_AND_ASSIGN(ResourceManager);
};

}  // namespace trunks

#endif  // TRUNKS_RESOURCE_MANAGER_H_
