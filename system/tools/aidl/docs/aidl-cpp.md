# Generating C++ Binder Interfaces with `aidl-cpp`

## Background

“aidl” refers to several related but distinct concepts:

 - the AIDL interface [definition language](http://developer.android.com/guide/components/aidl.html)
 - .aidl files (which contain AIDL)
 - the aidl generator which transforms AIDL into client/server IPC interfaces

The _aidl generator_ is a command line tool that generates client and server
stubs for Binder interfaces from a specification in a file with the .aidl
extension.  For Java interfaces, the executable is called `aidl` while for C++
the binary is called `aidl-cpp`.  In this document, we’ll use AIDL to describe
the language of .aidl files and _aidl generator_ to refer to the code generation
tool that takes an .aidl file, parses the AIDL, and outputs code.

Previously, the _aidl generator_ only generated Java interface/stub/proxy
objects.  C++ Binder interfaces were handcrafted with various degrees of
compatibility with the Java equivalents.  The Brillo project added support for
generating C++ with the _aidl generator_.  This generated C++ is cross-language
compatible (e.g. Java clients are tested to interoperate with native services).

## Overview

This document describes how C++ generation works with attention to:

 - build interface
 - cross-language type mapping
 - C++ parcelables
 - cross-language error reporting
 - cross-language null reference handling
 - cross-language integer constants

## Detailed Design

### Build Interface

Write AIDL in .aidl files and add them to `LOCAL_SRC_FILES` in your Android.mk.
If your build target is a binary (e.g. you include `$(BUILD_SHARED_LIBRARY)`),
then the generated code will be C++, not Java.

AIDL definitions should be hosted from the same repository as the
implementation.  Any system that needs the definition will also need the
implementation (for both parcelables and interface).  If there are multiple
implementations (i.e. one in Java and one in C++), keep the definition with the
native implementation.  Android
[now has systems](https://developers.google.com/brillo/?hl=en) that run the
native components of the system without the Java.

If you use an import statement in your AIDL, even from the same package, you
need to add a path to `LOCAL_AIDL_INCLUDES`.  This path should be relative to
the root of the Android tree.  For instance, a file IFoo.aidl defining
com.example.IFoo might sit in a folder hierarchy
something/something-else/com/example/IFoo.aidl.  Then we would write:

```
LOCAL_AIDL_INCLUDES := something/something-else
```

Generated C++ ends up in nested namespaces corresponding to the interface’s
package.  The generated header also corresponds to the interface package.  So
com.example.IFoo becomes ::com::example::IFoo in header “com/example/IFoo.h”.

Similar to how Java works, the suffix of the path to a .aidl file must match
the package.  So if IFoo.aidl declares itself to be in package com.example, the
folder structure (as given to `LOCAL_SRC_FILES`) must look like:
`some/prefix/com/example/IFoo.aidl`.

To generate code from .aidl files from another build target (e.g. another
binary or java), just add a relative path to the .aidl files to
`LOCAL_SRC_FILES`.  Remember that importing AIDL works the same, even for code
in other directory hierarchies: add the include root path relative to the
checkout root to `LOCAL_AIDL_INCLUDES`.

### Type Mapping

The following table summarizes the equivalent C++ types for common Java types
and whether those types may be used as in/out/inout parameters in AIDL
interfaces.

| Java Type             | C++ Type            | inout | Notes                                                 |
|-----------------------|---------------------|-------|-------------------------------------------------------|
| boolean               | bool                | in    | "These 8 types are all considered primitives.         |
| byte                  | int8\_t             | in    |                                                       |
| char                  | char16\_t           | in    |                                                       |
| int                   | int32\_t            | in    |                                                       |
| long                  | int64\_t            | in    |                                                       |
| float                 | float               | in    |                                                       |
| double                | double              | in    |                                                       |
| String                | String16            | in    | Supports null references.                             |
| android.os.Parcelable | android::Parcelable | inout |                                                       |
| T extends IBinder     | sp<T>               | in    |                                                       |
| Arrays (T[])          | vector<T>           | inout | May contain only primitives, Strings and parcelables. |
| List<String>          | vector<String16>    | inout |                                                       |
| PersistableBundle     | PersistableBundle   | inout | binder/PersistableBundle.h                            |
| List<IBinder>         | vector<sp<IBinder>> | inout |                                                       |
| FileDescriptor        | ScopedFd            | inout | nativehelper/ScopedFd.h                               |

Note that java.util.Map and java.utils.List are not good candidates for cross
language communication because they may contain arbitrary types on the Java
side.  For instance, Map is cast to Map<String,Object> and then the object
values dynamically inspected and serialized as type/value pairs.  Support
exists for sending arbitrary Java serializables, Android Bundles, etc.

### C++ Parcelables

In Java, a parcelable should extend android.os.Parcelable and provide a static
final CREATOR field that acts as a factory for new instances/arrays of
instances of the parcelable.  In addition, in order to be used as an out
parameter, a parcelable class must define a readFromParcel method.

In C++, parcelables must implement android::Parcelable from binder/Parcelable.h
in libbinder.  Parcelables must define a constructor that takes no arguments.
In order to be used in arrays, a parcelable must implement a copy or move
constructor (called implicitly in vector).

The C++ generator needs to know what header defines the C++ parcelable.  It
learns this from the `cpp_header` directive shown below.  The generator takes
this string and uses it as the literal include statement in generated code.
The idea here is that you generate your code once, link it into a library along
with parcelable implementations, and export appropriate header paths.  This
header include must make sense in the context of the Android.mk that compiles
this generated code.

```
// ExampleParcelable.aidl
package com.example.android;

// Native types must be aliased at their declaration in the appropriate .aidl
// file.  This allows multiple interfaces to use a parcelable and its C++
// equivalent without duplicating the mapping between the C++ and Java types.
// Generator will assume bar/foo.h declares class
// com::example::android::ExampleParcelable
parcelable ExampleParcelable cpp_header "bar/foo.h";
```

### Null Reference Handling

The aidl generator for both C++ and Java languages has been expanded to
understand nullable annotations.

Given an interface definition like:

```
interface IExample {
  void ReadStrings(String neverNull, in @nullable String maybeNull);
};
```

the generated C++ header code looks like:

```
class IExample {
  android::binder::Status ReadStrings(
      const android::String16& in_neverNull,
      const std::unique_ptr<android::String16>& in_maybeNull);
};
```

Note that by default, the generated C++ passes a const reference to the value
of a parameter and rejects null references with a NullPointerException sent
back the caller.  Parameters marked with @nullable are passed by pointer,
allowing native services to explicitly control whether they allow method
overloading via null parameters.  Java stubs and proxies currently do nothing
with the @nullable annotation.

### Exception Reporting

C++ methods generated by the aidl generator return `android::binder::Status`
objects, rather than `android::status_t`.  This Status object allows generated
C++ code to send and receive exceptions (an exception type and a String16 error
message) since we do not use real exceptions in C++.  More background on Status
objects can be found here.

For legacy support and migration ease, the Status object includes a mechanism
to report a `android::status_t`.  However, that return code is interpreted by a
different code path and does not include a helpful String message.

For situations where your native service needs to throw an error code specific
to the service, use `Status::fromServiceSpecificError()`.  This kind of
exception comes with a helpful message and an integer error code.  Make your
error codes consistent across services by using interface constants (see
below).

### Integer Constants

AIDL has been enhanced to support defining integer constants as part of an
interface:

```
interface IMyInterface {
    const int CONST_A = 1;
    const int CONST_B = 2;
    const int CONST_C = 3;
    ...
}
```

These map to appropriate 32 bit integer class constants in Java and C++ (e.g.
`IMyInterface.CONST_A` and `IMyInterface::CONST_A` respectively).
