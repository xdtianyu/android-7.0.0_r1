// Copyright 2015 The Android Open Source Project
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

// This file is left empty deliberately to work around some dependency
// generation in GYP. In GYP for custom actions to run properly, the 'sources'
// section must contain at least one recognizable source file. Since libwebserv
// inherits all its source from libwebserv_common target and has no additional
// C++ source files, this file is used for this purpose, even though it doesn't
// produce any compiled code.
