// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/flag_helper.h"

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sysexits.h>

#include <base/base_switches.h>
#include <base/command_line.h>
#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <base/strings/string_number_conversions.h>

namespace brillo {

Flag::Flag(const char* name,
           const char* default_value,
           const char* help,
           bool visible)
    : name_(name),
      default_value_(default_value),
      help_(help),
      visible_(visible) {
}

class HelpFlag : public brillo::Flag {
 public:
  HelpFlag() : Flag("help", "false", "Show this help message", true) {}

  bool SetValue(const std::string& /* value */) override { return true; };
  const char* GetType() const override { return "bool"; }
};

BoolFlag::BoolFlag(const char* name,
                   bool* value,
                   bool* no_value,
                   const char* default_value,
                   const char* help,
                   bool visible)
    : Flag(name, default_value, help, visible),
      value_(value),
      no_value_(no_value) {
}

bool BoolFlag::SetValue(const std::string& value) {
  if (value.empty()) {
    *value_ = true;
  } else {
    if (!value.compare("true"))
      *value_ = true;
    else if (!value.compare("false"))
      *value_ = false;
    else
      return false;
  }

  *no_value_ = !*value_;

  return true;
}

const char* BoolFlag::GetType() const {
  return "bool";
}

Int32Flag::Int32Flag(const char* name,
                     int* value,
                     const char* default_value,
                     const char* help,
                     bool visible)
    : Flag(name, default_value, help, visible), value_(value) {
}

bool Int32Flag::SetValue(const std::string& value) {
  return base::StringToInt(value, value_);
}

const char* Int32Flag::GetType() const {
  return "int";
}

Int64Flag::Int64Flag(const char* name,
                     int64_t* value,
                     const char* default_value,
                     const char* help,
                     bool visible)
    : Flag(name, default_value, help, visible), value_(value) {
}

bool Int64Flag::SetValue(const std::string& value) {
  return base::StringToInt64(value, value_);
}

const char* Int64Flag::GetType() const {
  return "int64";
}

UInt64Flag::UInt64Flag(const char* name,
                       uint64_t* value,
                       const char* default_value,
                       const char* help,
                       bool visible)
    : Flag(name, default_value, help, visible), value_(value) {
}

bool UInt64Flag::SetValue(const std::string& value) {
  return base::StringToUint64(value, value_);
}

const char* UInt64Flag::GetType() const {
  return "uint64";
}

DoubleFlag::DoubleFlag(const char* name,
                       double* value,
                       const char* default_value,
                       const char* help,
                       bool visible)
    : Flag(name, default_value, help, visible), value_(value) {
}

bool DoubleFlag::SetValue(const std::string& value) {
  return base::StringToDouble(value, value_);
}

const char* DoubleFlag::GetType() const {
  return "double";
}

StringFlag::StringFlag(const char* name,
                       std::string* value,
                       const char* default_value,
                       const char* help,
                       bool visible)
    : Flag(name, default_value, help, visible), value_(value) {
}

bool StringFlag::SetValue(const std::string& value) {
  value_->assign(value);

  return true;
}

const char* StringFlag::GetType() const {
  return "string";
}

namespace {
brillo::FlagHelper* instance_ = nullptr;
}  // namespace

FlagHelper::FlagHelper() : command_line_(nullptr) {
  AddFlag(std::unique_ptr<Flag>(new HelpFlag()));
}

FlagHelper::~FlagHelper() {
}

brillo::FlagHelper* FlagHelper::GetInstance() {
  if (!instance_)
    instance_ = new FlagHelper();

  return instance_;
}

void FlagHelper::ResetForTesting() {
  delete instance_;
  instance_ = nullptr;
}

void FlagHelper::Init(int argc,
                      const char* const* argv,
                      std::string help_usage) {
  brillo::FlagHelper* helper = GetInstance();
  if (!helper->command_line_) {
    if (!base::CommandLine::InitializedForCurrentProcess())
      base::CommandLine::Init(argc, argv);
    helper->command_line_ = base::CommandLine::ForCurrentProcess();
  }

  GetInstance()->SetUsageMessage(help_usage);

  GetInstance()->UpdateFlagValues();
}

void FlagHelper::UpdateFlagValues() {
  std::string error_msg;
  int error_code = EX_OK;

  // Check that base::CommandLine has been initialized.
  CHECK(base::CommandLine::InitializedForCurrentProcess());

  // If the --help flag exists, print out help message and exit.
  if (command_line_->HasSwitch("help")) {
    puts(GetHelpMessage().c_str());
    exit(EX_OK);
  }

  // Iterate over the base::CommandLine switches.  Update the value
  // of the corresponding Flag if it exists, or output an error message
  // if the flag wasn't defined.
  const base::CommandLine::SwitchMap& switch_map = command_line_->GetSwitches();

  for (const auto& pair : switch_map) {
    const std::string& key = pair.first;
    // Make sure we allow the standard logging switches (--v and --vmodule).
    if (key == switches::kV || key == switches::kVModule)
      continue;

    const std::string& value = pair.second;

    auto df_it = defined_flags_.find(key);
    if (df_it != defined_flags_.end()) {
      Flag* flag = df_it->second.get();
      if (!flag->SetValue(value)) {
        base::StringAppendF(
            &error_msg,
            "ERROR: illegal value '%s' specified for %s flag '%s'\n",
            value.c_str(),
            flag->GetType(),
            flag->name_);
        error_code = EX_DATAERR;
      }
    } else {
      base::StringAppendF(
          &error_msg, "ERROR: unknown command line flag '%s'\n", key.c_str());
      error_code = EX_USAGE;
    }
  }

  if (error_code != EX_OK) {
    puts(error_msg.c_str());
    exit(error_code);
  }
}

void FlagHelper::AddFlag(std::unique_ptr<Flag> flag) {
  defined_flags_.emplace(flag->name_, std::move(flag));
}

void FlagHelper::SetUsageMessage(std::string help_usage) {
  help_usage_.assign(std::move(help_usage));
}

std::string FlagHelper::GetHelpMessage() const {
  std::string help = help_usage_;
  help.append("\n\n");
  for (const auto& pair : defined_flags_) {
    const Flag* flag = pair.second.get();
    if (flag->visible_) {
      base::StringAppendF(&help,
                          "  --%s  (%s)  type: %s  default: %s\n",
                          flag->name_,
                          flag->help_,
                          flag->GetType(),
                          flag->default_value_);
    }
  }
  return help;
}

}  // namespace brillo
