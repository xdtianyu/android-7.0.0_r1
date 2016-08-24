// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_BRILLO_EXPORT_H_
#define LIBBRILLO_BRILLO_BRILLO_EXPORT_H_

// Use BRILLO_EXPORT attribute to decorate your classes, methods and variables
// that need to be exported out of libbrillo. By default, any symbol not
// explicitly marked with BRILLO_EXPORT attribute is not exported.

// Put BRILLO_EXPORT in front of methods or variables and in between the
// class and the tag name:
/*

BRILLO_EXPORT void foo();

class BRILLO_EXPORT Bar {
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
// used with "extern template" and combining this with BRILLO_EXPORT.
#define BRILLO_EXPORT __attribute__((__visibility__("default")))

// On occasion you might need to disable exporting a particular symbol if
// you don't want the clients to see it. For example, you can explicitly
// hide a member of an exported class:
/*

class BRILLO_EXPORT Foo {
 public:
  void bar();  // Exported since it is a member of an exported class.

 private:
  BRILLO_PRIVATE void baz();  // Explicitly removed from export table.
};

*/

// Note that even though a class may have a private member it doesn't mean
// that it must not be exported, since the compiler might still need it.
// For example, an inline public method calling a private method will not link
// if that private method is not exported.
// So be careful with hiding members if you don't want to deal with obscure
// linker errors.
#define BRILLO_PRIVATE __attribute__((__visibility__("hidden")))

#endif  // LIBBRILLO_BRILLO_BRILLO_EXPORT_H_
