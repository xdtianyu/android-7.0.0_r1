// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_COMPONENT_MANAGER_H_
#define LIBWEAVE_SRC_COMPONENT_MANAGER_H_

#include <map>
#include <memory>

#include <base/callback_list.h>
#include <base/time/clock.h>
#include <base/values.h>
#include <weave/error.h>

#include "src/commands/command_queue.h"

namespace weave {

class CommandInstance;

enum class UserRole {
  kViewer,
  kUser,
  kManager,
  kOwner,
};

// A simple notification record event to track component state changes.
// The |timestamp| records the time of the state change.
// |changed_properties| contains a property set with the new property values
// which were updated at the time the event was recorded.
struct ComponentStateChange {
  ComponentStateChange(base::Time time,
                       const std::string& path,
                       std::unique_ptr<base::DictionaryValue> properties)
      : timestamp{time},
        component{path},
        changed_properties{std::move(properties)} {}
  base::Time timestamp;
  std::string component;
  std::unique_ptr<base::DictionaryValue> changed_properties;
};

class ComponentManager {
 public:
  using UpdateID = uint64_t;
  using Token =
      std::unique_ptr<base::CallbackList<void(UpdateID)>::Subscription>;
  struct StateSnapshot {
    UpdateID update_id;
    std::vector<ComponentStateChange> state_changes;
  };

  ComponentManager() {}
  virtual ~ComponentManager() {}

  // Loads trait definition schema.
  virtual bool LoadTraits(const base::DictionaryValue& dict,
                          ErrorPtr* error) = 0;

  // Same as the overload above, but takes a json string to read the trait
  // definitions from.
  virtual bool LoadTraits(const std::string& json, ErrorPtr* error) = 0;

  // Sets callback which is called when new trait definitions are added.
  virtual void AddTraitDefChangedCallback(const base::Closure& callback) = 0;

  // Adds a new component instance to device.
  // |path| is a path to the parent component (or empty string if a root-level
  // component is being added).
  // |name| is a component name being added.
  // |traits| is a list of trait names this component supports.
  virtual bool AddComponent(const std::string& path,
                            const std::string& name,
                            const std::vector<std::string>& traits,
                            ErrorPtr* error) = 0;

  // Adds a new component instance to device, as a part of component array.
  // |path| is a path to the parent component.
  // |name| is an array root element inside the child components.
  // |traits| is a list of trait names this component supports.
  virtual bool AddComponentArrayItem(const std::string& path,
                                     const std::string& name,
                                     const std::vector<std::string>& traits,
                                     ErrorPtr* error) = 0;

  // Removes an existing component instance from device.
  // |path| is a path to the parent component (or empty string if a root-level
  // component is being removed).
  // |name| is a name of the component to be removed.
  virtual bool RemoveComponent(const std::string& path,
                               const std::string& name,
                               ErrorPtr* error) = 0;

  // Removes an element from component array.
  // |path| is a path to the parent component.
  // |index| is a zero-based element index in the component array.
  virtual bool RemoveComponentArrayItem(const std::string& path,
                                        const std::string& name,
                                        size_t index,
                                        ErrorPtr* error) = 0;

  // Sets callback which is called when new components are added.
  virtual void AddComponentTreeChangedCallback(
      const base::Closure& callback) = 0;

  // Adds a new command instance to the command queue. The command specified in
  // |command_instance| must be fully initialized and have its name, component,
  // id populated.
  virtual void
  AddCommand(std::unique_ptr<CommandInstance> command_instance) = 0;

  // Parses the command definition from a json dictionary. The resulting command
  // instance is populated with all the required fields and partially validated
  // against syntax/schema.
  // The new command ID is returned through optional |id| param.
  virtual std::unique_ptr<CommandInstance> ParseCommandInstance(
      const base::DictionaryValue& command,
      Command::Origin command_origin,
      UserRole role,
      std::string* id,
      ErrorPtr* error) = 0;

  // Find a command instance with the given ID in the command queue.
  virtual CommandInstance* FindCommand(const std::string& id) = 0;

  // Command queue monitoring callbacks (called when a new command is added to
  // or removed from the queue).
  virtual void AddCommandAddedCallback(
      const CommandQueue::CommandCallback& callback) = 0;
  virtual void AddCommandRemovedCallback(
      const CommandQueue::CommandCallback& callback) = 0;

  // Adds a command handler for specific component's command.
  // |component_path| is a path to target component (e.g. "stove.burners[2]").
  // |command_name| is a full path of the command, including trait name and
  // command name (e.g. "burner.setPower").
  virtual void AddCommandHandler(
      const std::string& component_path,
      const std::string& command_name,
      const Device::CommandHandlerCallback& callback) = 0;

  // Finds a component instance by its full path.
  virtual const base::DictionaryValue* FindComponent(const std::string& path,
                                                     ErrorPtr* error) const = 0;
  // Finds a definition of trait with the given |name|.
  virtual const base::DictionaryValue* FindTraitDefinition(
      const std::string& name) const = 0;

  // Finds a command definition, where |command_name| is in the form of
  // "trait.command".
  virtual const base::DictionaryValue* FindCommandDefinition(
      const std::string& command_name) const = 0;

  // Checks the minimum required user role for a given command.
  virtual bool GetMinimalRole(const std::string& command_name,
                              UserRole* minimal_role,
                              ErrorPtr* error) const = 0;

  // Returns the full JSON dictionary containing trait definitions.
  virtual const base::DictionaryValue& GetTraits() const = 0;

  // Returns the full JSON dictionary containing component instances.
  virtual const base::DictionaryValue& GetComponents() const = 0;

  // Component state manipulation methods.
  virtual bool SetStateProperties(const std::string& component_path,
                                  const base::DictionaryValue& dict,
                                  ErrorPtr* error) = 0;
  virtual bool SetStatePropertiesFromJson(const std::string& component_path,
                                          const std::string& json,
                                          ErrorPtr* error) = 0;
  virtual const base::Value* GetStateProperty(const std::string& component_path,
                                              const std::string& name,
                                              ErrorPtr* error) const = 0;
  virtual bool SetStateProperty(const std::string& component_path,
                                const std::string& name,
                                const base::Value& value,
                                ErrorPtr* error) = 0;

  virtual void AddStateChangedCallback(const base::Closure& callback) = 0;

  // Returns the recorded state changes since last time this method was called.
  virtual StateSnapshot GetAndClearRecordedStateChanges() = 0;

  // Called to notify that the state patch with |id| has been successfully sent
  // to the server and processed.
  virtual void NotifyStateUpdatedOnServer(UpdateID id) = 0;

  // Returns an ID of last state change update. Each SetStatePropertyNNN()
  // invocation increments this value by 1.
  virtual UpdateID GetLastStateChangeId() const = 0;

  // Subscribes for device state update notifications from cloud server.
  // The |callback| will be called every time a state patch with given ID is
  // successfully received and processed by Weave server.
  // Returns a subscription token. As soon as this token is destroyed, the
  // respective callback is removed from the callback list.
  virtual Token AddServerStateUpdatedCallback(
      const base::Callback<void(UpdateID)>& callback) = 0;

  // Helper method for legacy API to obtain first component that implements
  // the given trait. This is useful for routing commands that have no component
  // path specified.
  // Returns empty string if no components are found.
  // This method only searches for component on the top level of components
  // tree. No sub-components are searched.
  virtual std::string FindComponentWithTrait(
      const std::string& trait) const = 0;

  // Support for legacy APIs. Setting command and state definitions.
  // This translates into modifying a trait definition.
  virtual bool AddLegacyCommandDefinitions(const base::DictionaryValue& dict,
                                           ErrorPtr* error) = 0;
  virtual bool AddLegacyStateDefinitions(const base::DictionaryValue& dict,
                                         ErrorPtr* error) = 0;
  // Returns device state for legacy APIs.
  virtual const base::DictionaryValue& GetLegacyState() const = 0;
  // Returns command definitions for legacy APIs.
  virtual const base::DictionaryValue& GetLegacyCommandDefinitions() const = 0;

  DISALLOW_COPY_AND_ASSIGN(ComponentManager);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMPONENT_MANAGER_H_
