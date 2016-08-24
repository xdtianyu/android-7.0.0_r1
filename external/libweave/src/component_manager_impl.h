// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_COMPONENT_MANAGER_IMPL_H_
#define LIBWEAVE_SRC_COMPONENT_MANAGER_IMPL_H_

#include <base/time/default_clock.h>

#include "src/commands/command_queue.h"
#include "src/component_manager.h"
#include "src/states/state_change_queue.h"

namespace weave {

class ComponentManagerImpl final : public ComponentManager {
 public:
  explicit ComponentManagerImpl(provider::TaskRunner* task_runner,
                                base::Clock* clock = nullptr);
  ~ComponentManagerImpl() override;

  // Loads trait definition schema.
  bool LoadTraits(const base::DictionaryValue& dict, ErrorPtr* error) override;

  // Same as the overload above, but takes a json string to read the trait
  // definitions from.
  bool LoadTraits(const std::string& json, ErrorPtr* error) override;

  // Sets callback which is called when new trait definitions are added.
  void AddTraitDefChangedCallback(const base::Closure& callback) override;

  // Adds a new component instance to device.
  // |path| is a path to the parent component (or empty string if a root-level
  // component is being added).
  // |name| is a component name being added.
  // |traits| is a list of trait names this component supports.
  bool AddComponent(const std::string& path,
                    const std::string& name,
                    const std::vector<std::string>& traits,
                    ErrorPtr* error) override;

  // Adds a new component instance to device, as a part of component array.
  // |path| is a path to the parent component.
  // |name| is an array root element inside the child components.
  // |traits| is a list of trait names this component supports.
  bool AddComponentArrayItem(const std::string& path,
                             const std::string& name,
                             const std::vector<std::string>& traits,
                             ErrorPtr* error) override;

  // Removes an existing component instance from device.
  // |path| is a path to the parent component (or empty string if a root-level
  // component is being removed).
  // |name| is a name of the component to be removed.
  bool RemoveComponent(const std::string& path,
                       const std::string& name,
                       ErrorPtr* error) override;

  // Removes an element from component array.
  // |path| is a path to the parent component.
  // |name| is an array root element inside the child components.
  // |index| is a zero-based element index in the component array.
  bool RemoveComponentArrayItem(const std::string& path,
                                const std::string& name,
                                size_t index,
                                ErrorPtr* error) override;

  // Sets callback which is called when new components are added.
  void AddComponentTreeChangedCallback(const base::Closure& callback) override;

  // Adds a new command instance to the command queue. The command specified in
  // |command_instance| must be fully initialized and have its name, component,
  // id populated.
  void AddCommand(std::unique_ptr<CommandInstance> command_instance) override;

  // Parses the command definition from a json dictionary. The resulting command
  // instance is populated with all the required fields and partially validated
  // against syntax/schema.
  // The new command ID is returned through optional |id| param.
  std::unique_ptr<CommandInstance> ParseCommandInstance(
      const base::DictionaryValue& command,
      Command::Origin command_origin,
      UserRole role,
      std::string* id,
      ErrorPtr* error) override;

  // Find a command instance with the given ID in the command queue.
  CommandInstance* FindCommand(const std::string& id) override;

  // Command queue monitoring callbacks (called when a new command is added to
  // or removed from the queue).
  void AddCommandAddedCallback(
      const CommandQueue::CommandCallback& callback) override;
  void AddCommandRemovedCallback(
      const CommandQueue::CommandCallback& callback) override;

  // Adds a command handler for specific component's command.
  // |component_path| is a path to target component (e.g. "stove.burners[2]").
  // |command_name| is a full path of the command, including trait name and
  // command name (e.g. "burner.setPower").
  void AddCommandHandler(
      const std::string& component_path,
      const std::string& command_name,
      const Device::CommandHandlerCallback& callback) override;

  // Finds a component instance by its full path.
  const base::DictionaryValue* FindComponent(const std::string& path,
                                             ErrorPtr* error) const override;
  // Finds a definition of trait with the given |name|.
  const base::DictionaryValue* FindTraitDefinition(
      const std::string& name) const override;

  // Finds a command definition, where |command_name| is in the form of
  // "trait.command".
  const base::DictionaryValue* FindCommandDefinition(
      const std::string& command_name) const override;

  // Checks the minimum required user role for a given command.
  bool GetMinimalRole(const std::string& command_name,
                      UserRole* minimal_role,
                      ErrorPtr* error) const override;

  // Returns the full JSON dictionary containing trait definitions.
  const base::DictionaryValue& GetTraits() const override { return traits_; }

  // Returns the full JSON dictionary containing component instances.
  const base::DictionaryValue& GetComponents() const override {
    return components_;
  }

  // Component state manipulation methods.
  bool SetStateProperties(const std::string& component_path,
                          const base::DictionaryValue& dict,
                          ErrorPtr* error) override;
  bool SetStatePropertiesFromJson(const std::string& component_path,
                                  const std::string& json,
                                  ErrorPtr* error) override;
  const base::Value* GetStateProperty(const std::string& component_path,
                                      const std::string& name,
                                      ErrorPtr* error) const override;
  bool SetStateProperty(const std::string& component_path,
                        const std::string& name,
                        const base::Value& value,
                        ErrorPtr* error) override;

  void AddStateChangedCallback(const base::Closure& callback) override;

  // Returns the recorded state changes since last time this method was called.
  StateSnapshot GetAndClearRecordedStateChanges() override;

  // Called to notify that the state patch with |id| has been successfully sent
  // to the server and processed.
  void NotifyStateUpdatedOnServer(UpdateID id) override;

  // Returns an ID of last state change update. Each SetStatePropertyNNN()
  // invocation increments this value by 1.
  UpdateID GetLastStateChangeId() const override {
    return last_state_change_id_;
  }

  // Subscribes for device state update notifications from cloud server.
  // The |callback| will be called every time a state patch with given ID is
  // successfully received and processed by Weave server.
  // Returns a subscription token. As soon as this token is destroyed, the
  // respective callback is removed from the callback list.
  Token AddServerStateUpdatedCallback(
      const base::Callback<void(UpdateID)>& callback) override;

  // Helper method for legacy API to obtain first component that implements
  // the given trait. This is useful for routing commands that have no component
  // path specified.
  // Returns empty string if no components are found.
  // This method only searches for component on the top level of components
  // tree. No sub-components are searched.
  std::string FindComponentWithTrait(const std::string& trait) const override;

  // Support for legacy APIs. Setting command and state definitions.
  // This translates into modifying a trait definition.
  bool AddLegacyCommandDefinitions(const base::DictionaryValue& dict,
                                   ErrorPtr* error) override;
  bool AddLegacyStateDefinitions(const base::DictionaryValue& dict,
                                 ErrorPtr* error) override;
  // Returns device state for legacy APIs.
  const base::DictionaryValue& GetLegacyState() const override;
  // Returns command definitions for legacy APIs.
  const base::DictionaryValue& GetLegacyCommandDefinitions() const override;

 private:
  // A helper method to find a JSON element of component at |path| to add new
  // sub-components to.
  base::DictionaryValue* FindComponentGraftNode(const std::string& path,
                                                ErrorPtr* error);
  base::DictionaryValue* FindMutableComponent(const std::string& path,
                                              ErrorPtr* error);

  // Legacy API support: Helper function to support state/command definitions.
  // Adds the given trait to at least one component.
  // Searches for available components and if none of them already supports this
  // trait, it adds it to the first available component.
  void AddTraitToLegacyComponent(const std::string& trait);

  // Helper method to find a sub-component given a root node and a relative path
  // from the root to the target component.
  static const base::DictionaryValue* FindComponentAt(
      const base::DictionaryValue* root,
      const std::string& path,
      ErrorPtr* error);

  base::DefaultClock default_clock_;
  base::Clock* clock_{nullptr};

  // An ID of last state change update. Each NotifyPropertiesUpdated()
  // invocation increments this value by 1.
  UpdateID last_state_change_id_{0};
  // Callback list for state change queue event sinks.
  // This member must be defined before |command_queue_|.
  base::CallbackList<void(UpdateID)> on_server_state_updated_;

  base::DictionaryValue traits_;      // Trait definitions.
  base::DictionaryValue components_;  // Component instances.
  CommandQueue command_queue_;  // Command queue containing command instances.
  std::vector<base::Closure> on_trait_changed_;
  std::vector<base::Closure> on_componet_tree_changed_;
  std::vector<base::Closure> on_state_changed_;
  uint32_t next_command_id_{0};
  std::map<std::string, std::unique_ptr<StateChangeQueue>> state_change_queues_;

  // Legacy API support.
  mutable base::DictionaryValue legacy_state_;         // Device state.
  mutable base::DictionaryValue legacy_command_defs_;  // Command definitions.

  DISALLOW_COPY_AND_ASSIGN(ComponentManagerImpl);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMPONENT_MANAGER_IMPL_H_
