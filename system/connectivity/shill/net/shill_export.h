//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef SHILL_NET_SHILL_EXPORT_H_
#define SHILL_NET_SHILL_EXPORT_H_

// Use SHILL_EXPORT attribute to decorate your classes, methods and variables
// that need to be exported out of libshill By default, any symbol not
// explicitly marked with SHILL_EXPORT attribute is not exported.

// Put SHILL_EXPORT in front of methods or variables and in between the
// class and the tag name:
/*

SHILL_EXPORT void foo();

class SHILL_EXPORT Bar {
 public:
  void baz();  // Exported since it is a member of an exported class.
};

*/

// Exporting a class automatically exports all of its members. However there are
// no export entries for non-static member variables since they are not accessed
// directly, but rather through "this" pointer. Class methods, type information,
// virtual table (if any), and static member variables are exported.

// Finally, template functions and template members of a class may not be
// inlined by the compiler automatically and the out-of-line version will not
// be exported and fail to link. Marking those inline explicitly might help.
// Alternatively, exporting specific instantiation of the template could be
// used with "extern template" and combining this with SHILL_EXPORT.
#define SHILL_EXPORT __attribute__((__visibility__("default")))

// On occasion you might need to disable exporting a particular symbol if
// you don't want the clients to see it. For example, you can explicitly
// hide a member of an exported class:
/*

class SHILL_EXPORT Foo {
 public:
  void bar();  // Exported since it is a member of an exported class.

 private:
  SHILL_PRIVATE void baz();  // Explicitly removed from export table.
};

*/

// Note that even though a class may have a private member it doesn't mean
// that it must not be exported, since the compiler might still need it.
// For example, an inline public method calling a private method will not link
// if that private method is not exported.
// So be careful with hiding members if you don't want to deal with obscure
// linker errors.
#define SHILL_PRIVATE __attribute__((__visibility__("hidden")))

#endif  // SHILL_NET_SHILL_EXPORT_H_
