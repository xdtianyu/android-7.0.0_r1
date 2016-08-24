// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/schema_constants.h"

namespace weave {

namespace errors {
namespace commands {

const char kTypeMismatch[] = "type_mismatch";
const char kInvalidPropValue[] = "invalid_parameter_value";
const char kPropertyMissing[] = "parameter_missing";
const char kInvalidCommandName[] = "invalid_command_name";
const char kCommandFailed[] = "command_failed";
const char kInvalidMinimalRole[] = "invalid_minimal_role";
const char kCommandDestroyed[] = "command_destroyed";
const char kInvalidState[] = "invalid_state";
}  // namespace commands
}  // namespace errors

namespace commands {
namespace attributes {

const char kCommand_Id[] = "id";
const char kCommand_Name[] = "name";
const char kCommand_Component[] = "component";
const char kCommand_Parameters[] = "parameters";
const char kCommand_Progress[] = "progress";
const char kCommand_Results[] = "results";
const char kCommand_State[] = "state";
const char kCommand_Error[] = "error";

const char kCommand_Role[] = "minimalRole";
const char kCommand_Role_Manager[] = "manager";
const char kCommand_Role_Owner[] = "owner";
const char kCommand_Role_User[] = "user";
const char kCommand_Role_Viewer[] = "viewer";

}  // namespace attributes
}  // namespace commands

}  // namespace weave
