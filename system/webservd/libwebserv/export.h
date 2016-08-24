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

#ifndef WEBSERVER_LIBWEBSERV_EXPORT_H_
#define WEBSERVER_LIBWEBSERV_EXPORT_H_

// See detailed explanation of the purpose of LIBWEBSERV_EXPORT in
// brillo/brillo_export.h for similar attribute - BRILLO_EXPORT.
#define LIBWEBSERV_EXPORT __attribute__((__visibility__("default")))
#define LIBWEBSERV_PRIVATE __attribute__((__visibility__("hidden")))

#endif  // WEBSERVER_LIBWEBSERV_EXPORT_H_
