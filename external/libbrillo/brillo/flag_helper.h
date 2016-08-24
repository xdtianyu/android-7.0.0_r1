// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a helper class for dealing with command line flags.  It uses
// base/command_line.h to parse flags from argv, but provides an API similar
// to gflags.  Command line arguments with either '-' or '--' prefixes are
// treated as flags.  Flags can optionally have a value set using an '='
// delimeter, e.g. "--flag=value".  An argument of "--" will terminate flag
// parsing, so that any subsequent arguments will be treated as non-flag
// arguments, regardless of prefix.  Non-flag arguments are outside the scope
// of this class, and can instead be accessed through the GetArgs() function
// of the base::CommandLine singleton after FlagHelper initialization.
//
// The FlagHelper class will automatically take care of the --help flag, as
// well as aborting the program when unknown flags are passed to the
// application and when passed in parameters cannot be correctly parsed to
// their respective types.  Developers define flags at compile time using the
// following macros from within main():
//
//    DEFINE_bool(name, default_value, help)
//    DEFINE_int32(name, default_value, help)
//    DEFINE_int64(name, default_value, help)
//    DEFINE_uint64(name, default_value, help)
//    DEFINE_double(name, default_value, help)
//    DEFINE_string(name, default_value, help)
//
// Using the macro will create a scoped variable of the appropriate type
// with the name FLAGS_<name>, that can be used to access the flag's
// value within the program.  Here is an example of how the FlagHelper
// class is to be used:
//
// --
//
//  #include <brillo/flag_helper.h>
//  #include <stdio.h>
//
//  int main(int argc, char** argv) {
//    DEFINE_int32(example, 0, "Example int flag");
//    brillo::FlagHelper::Init(argc, argv, "Test application.");
//
//    printf("You passed in %d to --example command line flag\n",
//           FLAGS_example);
//    return 0;
//  }
//
// --
//
// In order to update the FLAGS_xxxx values from their defaults to the
// values passed in to the command line, Init(...) must be called after
// all the DEFINE_xxxx macros have instantiated the variables.

#ifndef LIBBRILLO_BRILLO_FLAG_HELPER_H_
#define LIBBRILLO_BRILLO_FLAG_HELPER_H_

#include <map>
#include <memory>
#include <string>

#include <base/command_line.h>
#include <base/macros.h>
#include <brillo/brillo_export.h>

namespace brillo {

// The corresponding class representation of a command line flag, used
// to keep track of pointers to the FLAGS_xxxx variables so that they
// can be updated.
class Flag {
 public:
  Flag(const char* name,
       const char* default_value,
       const char* help,
       bool visible);
  virtual ~Flag() = default;

  // Sets the associated FLAGS_xxxx value, taking into account the flag type
  virtual bool SetValue(const std::string& value) = 0;

  // Returns the type of the flag as a char array, for use in the help message
  virtual const char* GetType() const = 0;

  const char* name_;
  const char* default_value_;
  const char* help_;
  bool visible_;
};

class BRILLO_EXPORT BoolFlag final : public Flag {
 public:
  BoolFlag(const char* name,
           bool* value,
           bool* no_value,
           const char* default_value,
           const char* help,
           bool visible);
  bool SetValue(const std::string& value) override;

  const char* GetType() const override;

 private:
  bool* value_;
  bool* no_value_;
};

class BRILLO_EXPORT Int32Flag final : public Flag {
 public:
  Int32Flag(const char* name,
            int* value,
            const char* default_value,
            const char* help,
            bool visible);
  bool SetValue(const std::string& value) override;

  const char* GetType() const override;

 private:
  int* value_;
};

class BRILLO_EXPORT Int64Flag final : public Flag {
 public:
  Int64Flag(const char* name,
            int64_t* value,
            const char* default_value,
            const char* help,
            bool visible);
  bool SetValue(const std::string& value) override;

  const char* GetType() const override;

 private:
  int64_t* value_;
};

class BRILLO_EXPORT UInt64Flag final : public Flag {
 public:
  UInt64Flag(const char* name,
             uint64_t* value,
             const char* default_value,
             const char* help,
             bool visible);
  bool SetValue(const std::string& value) override;

  const char* GetType() const override;

 private:
  uint64_t* value_;
};

class BRILLO_EXPORT DoubleFlag final : public Flag {
 public:
  DoubleFlag(const char* name,
             double* value,
             const char* default_value,
             const char* help,
             bool visible);
  bool SetValue(const std::string& value) override;

  const char* GetType() const override;

 private:
  double* value_;
};

class BRILLO_EXPORT StringFlag final : public Flag {
 public:
  StringFlag(const char* name,
             std::string* value,
             const char* default_value,
             const char* help,
             bool visible);
  bool SetValue(const std::string& value) override;

  const char* GetType() const override;

 private:
  std::string* value_;
};

// The following macros are to be used from within main() to create
// scoped FLAGS_xxxx variables for easier access to command line flag
// values.  FLAGS_noxxxx variables are also created, which are used to
// set bool flags to false.  Creating the FLAGS_noxxxx variables here
// will also ensure a compiler error will be thrown if another flag
// is created with a conflicting name.
#define DEFINE_type(type, classtype, name, value, help)                     \
  type FLAGS_##name = value;                                                \
  brillo::FlagHelper::GetInstance()->AddFlag(std::unique_ptr<brillo::Flag>( \
      new brillo::classtype(#name, &FLAGS_##name, #value, help, true)));

#define DEFINE_int32(name, value, help) \
  DEFINE_type(int, Int32Flag, name, value, help)
#define DEFINE_int64(name, value, help) \
  DEFINE_type(int64_t, Int64Flag, name, value, help)
#define DEFINE_uint64(name, value, help) \
  DEFINE_type(uint64_t, UInt64Flag, name, value, help)
#define DEFINE_double(name, value, help) \
  DEFINE_type(double, DoubleFlag, name, value, help)
#define DEFINE_string(name, value, help) \
  DEFINE_type(std::string, StringFlag, name, value, help)

// Due to the FLAGS_no##name variables, can't re-use the same DEFINE_type macro
// for defining bool flags
#define DEFINE_bool(name, value, help)                                  \
  bool FLAGS_##name = value;                                            \
  bool FLAGS_no##name = !value;                                         \
  brillo::FlagHelper::GetInstance()->AddFlag(                           \
      std::unique_ptr<brillo::Flag>(new brillo::BoolFlag(               \
          #name, &FLAGS_##name, &FLAGS_no##name, #value, help, true))); \
  brillo::FlagHelper::GetInstance()->AddFlag(                           \
      std::unique_ptr<brillo::Flag>(new brillo::BoolFlag(               \
          "no" #name, &FLAGS_no##name, &FLAGS_##name, #value, help, false)));

// The FlagHelper class is a singleton class used for registering command
// line flags and pointers to their associated scoped variables, so that
// the variables can be updated once the command line arguments have been
// parsed by base::CommandLine.
class BRILLO_EXPORT FlagHelper final {
 public:
  // The singleton accessor function.
  static FlagHelper* GetInstance();

  // Resets the singleton object.  Developers shouldn't ever need to use this,
  // however it is required to be run at the end of every unit test to prevent
  // Flag definitions from carrying over from previous tests.
  static void ResetForTesting();

  // Initializes the base::CommandLine class, then calls UpdateFlagValues().
  static void Init(int argc, const char* const* argv, std::string help_usage);

  // Only to be used for running unit tests.
  void set_command_line_for_testing(base::CommandLine* command_line) {
    command_line_ = command_line;
  }

  // Checks all the parsed command line flags.  This iterates over the switch
  // map from base::CommandLine, and finds the corresponding Flag in order to
  // update the FLAGS_xxxx values to the parsed value.  If the --help flag is
  // passed in, it outputs a help message and exits the program.  If an unknown
  // flag is passed in, it outputs an error message and exits the program with
  // exit code EX_USAGE.
  void UpdateFlagValues();

  // Adds a flag to be tracked and updated once the command line is actually
  // parsed.  This function is an implementation detail, and is not meant
  // to be used directly by developers.  Developers should instead use the
  // DEFINE_xxxx macros to register a command line flag.
  void AddFlag(std::unique_ptr<Flag> flag);

  // Sets the usage message, which is prepended to the --help message.
  void SetUsageMessage(std::string help_usage);

 private:
  FlagHelper();
  ~FlagHelper();

  // Generates a help message from the Usage Message and registered flags.
  std::string GetHelpMessage() const;

  std::string help_usage_;
  std::map<std::string, std::unique_ptr<Flag>> defined_flags_;

  // base::CommandLine object for parsing the command line switches.  This
  // object isn't owned by this class, so don't need to delete it in the
  // destructor.
  base::CommandLine* command_line_;

  DISALLOW_COPY_AND_ASSIGN(FlagHelper);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_FLAG_HELPER_H_
