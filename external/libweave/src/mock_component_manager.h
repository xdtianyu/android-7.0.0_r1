// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_MOCK_COMPONENT_MANAGER_H_
#define LIBWEAVE_SRC_MOCK_COMPONENT_MANAGER_H_

#include "src/component_manager.h"

#include <gmock/gmock.h>

namespace weave {

class MockComponentManager : public ComponentManager {
 public:
  ~MockComponentManager() override {}
  MOCK_METHOD2(LoadTraits,
               bool(const base::DictionaryValue& dict, ErrorPtr* error));
  MOCK_METHOD2(LoadTraits, bool(const std::string& json, ErrorPtr* error));
  MOCK_METHOD1(AddTraitDefChangedCallback, void(const base::Closure& callback));
  MOCK_METHOD4(AddComponent,
               bool(const std::string& path,
                    const std::string& name,
                    const std::vector<std::string>& traits,
                    ErrorPtr* error));
  MOCK_METHOD4(AddComponentArrayItem,
               bool(const std::string& path,
                    const std::string& name,
                    const std::vector<std::string>& traits,
                    ErrorPtr* error));
  MOCK_METHOD3(RemoveComponent,
               bool(const std::string& path,
                    const std::string& name,
                    ErrorPtr* error));
  MOCK_METHOD4(RemoveComponentArrayItem,
               bool(const std::string& path,
                    const std::string& name,
                    size_t index,
                    ErrorPtr* error));
  MOCK_METHOD1(AddComponentTreeChangedCallback,
               void(const base::Closure& callback));
  MOCK_METHOD1(MockAddCommand, void(CommandInstance* command_instance));
  MOCK_METHOD5(MockParseCommandInstance,
               CommandInstance*(const base::DictionaryValue& command,
                                Command::Origin command_origin,
                                UserRole role,
                                std::string* id,
                                ErrorPtr* error));
  MOCK_METHOD1(FindCommand, CommandInstance*(const std::string& id));
  MOCK_METHOD1(AddCommandAddedCallback,
               void(const CommandQueue::CommandCallback& callback));
  MOCK_METHOD1(AddCommandRemovedCallback,
               void(const CommandQueue::CommandCallback& callback));
  MOCK_METHOD3(AddCommandHandler,
               void(const std::string& component_path,
                    const std::string& command_name,
                    const Device::CommandHandlerCallback& callback));
  MOCK_CONST_METHOD2(FindComponent,
                     const base::DictionaryValue*(const std::string& path,
                                                  ErrorPtr* error));
  MOCK_CONST_METHOD1(FindTraitDefinition,
                     const base::DictionaryValue*(const std::string& name));
  MOCK_CONST_METHOD1(
      FindCommandDefinition,
      const base::DictionaryValue*(const std::string& command_name));
  MOCK_CONST_METHOD3(GetMinimalRole,
                     bool(const std::string& command_name,
                          UserRole* minimal_role,
                          ErrorPtr* error));
  MOCK_CONST_METHOD0(GetTraits, const base::DictionaryValue&());
  MOCK_CONST_METHOD0(GetComponents, const base::DictionaryValue&());
  MOCK_METHOD3(SetStateProperties,
               bool(const std::string& component_path,
                    const base::DictionaryValue& dict,
                    ErrorPtr* error));
  MOCK_METHOD3(SetStatePropertiesFromJson,
               bool(const std::string& component_path,
                    const std::string& json,
                    ErrorPtr* error));
  MOCK_CONST_METHOD3(GetStateProperty,
                     const base::Value*(const std::string& component_path,
                                        const std::string& name,
                                        ErrorPtr* error));
  MOCK_METHOD4(SetStateProperty,
               bool(const std::string& component_path,
                    const std::string& name,
                    const base::Value& value,
                    ErrorPtr* error));
  MOCK_METHOD1(AddStateChangedCallback, void(const base::Closure& callback));
  MOCK_METHOD0(MockGetAndClearRecordedStateChanges, StateSnapshot&());
  MOCK_METHOD1(NotifyStateUpdatedOnServer, void(UpdateID id));
  MOCK_CONST_METHOD0(GetLastStateChangeId, UpdateID());
  MOCK_METHOD1(MockAddServerStateUpdatedCallback,
               base::CallbackList<void(UpdateID)>::Subscription*(
                   const base::Callback<void(UpdateID)>& callback));
  MOCK_CONST_METHOD1(FindComponentWithTrait,
                     std::string(const std::string& trait));
  MOCK_METHOD2(AddLegacyCommandDefinitions,
               bool(const base::DictionaryValue& dict, ErrorPtr* error));
  MOCK_METHOD2(AddLegacyStateDefinitions,
               bool(const base::DictionaryValue& dict, ErrorPtr* error));
  MOCK_CONST_METHOD0(GetLegacyState, const base::DictionaryValue&());
  MOCK_CONST_METHOD0(GetLegacyCommandDefinitions,
                     const base::DictionaryValue&());

 private:
  void AddCommand(std::unique_ptr<CommandInstance> command_instance) override {
    MockAddCommand(command_instance.get());
  }
  std::unique_ptr<CommandInstance> ParseCommandInstance(
      const base::DictionaryValue& command,
      Command::Origin command_origin,
      UserRole role,
      std::string* id,
      ErrorPtr* error) {
    return std::unique_ptr<CommandInstance>{
        MockParseCommandInstance(command, command_origin, role, id, error)};
  }
  StateSnapshot GetAndClearRecordedStateChanges() override {
    return std::move(MockGetAndClearRecordedStateChanges());
  }
  Token AddServerStateUpdatedCallback(
      const base::Callback<void(UpdateID)>& callback) override {
    return Token{MockAddServerStateUpdatedCallback(callback)};
  }
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMPONENT_MANAGER_H_
