// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_ERROR_H_
#define LIBWEAVE_INCLUDE_WEAVE_ERROR_H_

#include <memory>
#include <string>

#include <base/callback.h>
#include <base/compiler_specific.h>
#include <base/location.h>
#include <base/macros.h>
#include <weave/export.h>

namespace weave {

class Error;  // Forward declaration.

using ErrorPtr = std::unique_ptr<Error>;

class LIBWEAVE_EXPORT Error final {
 public:
  ~Error() = default;

  class AddToTypeProxy {
   public:
    operator bool() const { return false; }

    template <class T>
    operator std::unique_ptr<T>() const {
      return nullptr;
    }

    template <class T>
    operator T*() const {
      return nullptr;
    }
  };

  // Creates an instance of Error class.
  static ErrorPtr Create(const tracked_objects::Location& location,
                         const std::string& code,
                         const std::string& message);
  static ErrorPtr Create(const tracked_objects::Location& location,
                         const std::string& code,
                         const std::string& message,
                         ErrorPtr inner_error);
  // If |error| is not nullptr, creates another instance of Error class,
  // initializes it with specified arguments and adds it to the head of
  // the error chain pointed to by |error|.
  static AddToTypeProxy AddTo(ErrorPtr* error,
                              const tracked_objects::Location& location,
                              const std::string& code,
                              const std::string& message);
  // Same as the Error::AddTo above, but allows to pass in a printf-like
  // format string and optional parameters to format the error message.
  static AddToTypeProxy AddToPrintf(ErrorPtr* error,
                                    const tracked_objects::Location& location,
                                    const std::string& code,
                                    const char* format,
                                    ...) PRINTF_FORMAT(4, 5);

  // Clones error with all inner errors.
  ErrorPtr Clone() const;

  // Returns the error code and message
  const std::string& GetCode() const { return code_; }
  const std::string& GetMessage() const { return message_; }

  // Returns the location of the error in the source code.
  const tracked_objects::LocationSnapshot& GetLocation() const {
    return location_;
  }

  // Checks if this or any of the inner errors in the chain matches the
  // specified error code.
  bool HasError(const std::string& code) const;

  // Gets a pointer to the inner error, if present. Returns nullptr otherwise.
  const Error* GetInnerError() const { return inner_error_.get(); }

  // Gets a pointer to the first error occurred.
  // Returns itself if no inner error are available.
  const Error* GetFirstError() const;

  // Finds an error with the given code in the error chain stating at
  // |error_chain_start|. Returns the pointer to the first matching error
  // object.
  // Returns nullptr if no match is found or if |error_chain_start| is nullptr.
  static const Error* FindError(const Error* error_chain_start,
                                const std::string& code);

 protected:
  // Constructor is protected since this object is supposed to be
  // created via the Create factory methods.
  Error(const tracked_objects::Location& location,
        const std::string& code,
        const std::string& message,
        ErrorPtr inner_error);

  Error(const tracked_objects::LocationSnapshot& location,
        const std::string& code,
        const std::string& message,
        ErrorPtr inner_error);

  // Error code. A unique error code identifier.
  std::string code_;
  // Human-readable error message.
  std::string message_;
  // Error origin in the source code.
  tracked_objects::LocationSnapshot location_;
  // Pointer to inner error, if any. This forms a chain of errors.
  ErrorPtr inner_error_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Error);
};

// Default callback type for async operations.
// Function having this callback as argument should call the callback exactly
// one time.
// Successfully completed operation should run callback with |error| set to
// null. Failed operation should run callback with |error| containing error
// details.
using DoneCallback = base::Callback<void(ErrorPtr error)>;

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_ERROR_H_
