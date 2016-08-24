// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_COMMANDS_SCHEMA_CONSTANTS_H_
#define LIBWEAVE_SRC_COMMANDS_SCHEMA_CONSTANTS_H_

namespace weave {

namespace errors {
namespace commands {

// Common command definition error codes.
extern const char kTypeMismatch[];
extern const char kInvalidPropValue[];
extern const char kPropertyMissing[];
extern const char kInvalidCommandName[];
extern const char kCommandFailed[];
extern const char kInvalidMinimalRole[];
extern const char kCommandDestroyed[];
extern const char kInvalidState[];
}  // namespace commands
}  // namespace errors

namespace commands {
namespace attributes {
// Command description JSON schema attributes.
extern const char kCommand_Id[];
extern const char kCommand_Name[];
extern const char kCommand_Component[];
extern const char kCommand_Parameters[];
extern const char kCommand_Progress[];
extern const char kCommand_Results[];
extern const char kCommand_State[];
extern const char kCommand_Error[];

extern const char kCommand_Role[];
extern const char kCommand_Role_Manager[];
extern const char kCommand_Role_Owner[];
extern const char kCommand_Role_User[];
extern const char kCommand_Role_Viewer[];

}  // namespace attributes
}  // namespace commands

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMMANDS_SCHEMA_CONSTANTS_H_
