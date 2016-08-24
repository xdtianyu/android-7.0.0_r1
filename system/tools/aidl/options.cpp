/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "options.h"

#include <cstring>
#include <iostream>
#include <stdio.h>

#include "logging.h"
#include "os.h"

using std::cerr;
using std::endl;
using std::string;
using std::unique_ptr;
using std::vector;

namespace android {
namespace aidl {
namespace {

unique_ptr<JavaOptions> java_usage() {
  fprintf(stderr,
          "usage: aidl OPTIONS INPUT [OUTPUT]\n"
          "       aidl --preprocess OUTPUT INPUT...\n"
          "\n"
          "OPTIONS:\n"
          "   -I<DIR>    search path for import statements.\n"
          "   -d<FILE>   generate dependency file.\n"
          "   -a         generate dependency file next to the output file with "
          "the name based on the input file.\n"
          "   -p<FILE>   file created by --preprocess to import.\n"
          "   -o<FOLDER> base output folder for generated files.\n"
          "   -b         fail when trying to compile a parcelable.\n"
          "\n"
          "INPUT:\n"
          "   An aidl interface file.\n"
          "\n"
          "OUTPUT:\n"
          "   The generated interface files.\n"
          "   If omitted and the -o option is not used, the input filename is "
          "used, with the .aidl extension changed to a .java extension.\n"
          "   If the -o option is used, the generated files will be placed in "
          "the base output folder, under their package folder\n");
  return unique_ptr<JavaOptions>(nullptr);
}

}  // namespace

unique_ptr<JavaOptions> JavaOptions::Parse(int argc, const char* const* argv) {
  unique_ptr<JavaOptions> options(new JavaOptions());
  int i = 1;

  if (argc >= 2 && 0 == strcmp(argv[1], "--preprocess")) {
    if (argc < 4) {
      return java_usage();
    }
    options->output_file_name_ = argv[2];
    for (int i = 3; i < argc; i++) {
      options->files_to_preprocess_.push_back(argv[i]);
    }
    options->task = PREPROCESS_AIDL;
    return options;
  }

  options->task = COMPILE_AIDL_TO_JAVA;
  // OPTIONS
  while (i < argc) {
    const char* s = argv[i];
    const size_t len = strlen(s);
    if (s[0] != '-') {
      break;
    }
    if (len <= 1) {
      fprintf(stderr, "unknown option (%d): %s\n", i, s);
      return java_usage();
    }
    // -I<system-import-path>
    if (s[1] == 'I') {
      if (len > 2) {
        options->import_paths_.push_back(s + 2);
      } else {
        fprintf(stderr, "-I option (%d) requires a path.\n", i);
        return java_usage();
      }
    } else if (s[1] == 'd') {
      if (len > 2) {
        options->dep_file_name_ = s + 2;
      } else {
        fprintf(stderr, "-d option (%d) requires a file.\n", i);
        return java_usage();
      }
    } else if (strcmp(s, "-a") == 0) {
      options->auto_dep_file_ = true;
    } else if (s[1] == 'p') {
      if (len > 2) {
        options->preprocessed_files_.push_back(s + 2);
      } else {
        fprintf(stderr, "-p option (%d) requires a file.\n", i);
        return java_usage();
      }
    } else if (s[1] == 'o') {
      if (len > 2) {
        options->output_base_folder_= s + 2;
      } else {
        fprintf(stderr, "-o option (%d) requires a path.\n", i);
        return java_usage();
      }
    } else if (strcmp(s, "-b") == 0) {
      options->fail_on_parcelable_ = true;
    } else {
      // s[1] is not known
      fprintf(stderr, "unknown option (%d): %s\n", i, s);
      return java_usage();
    }
    i++;
  }
  // INPUT
  if (i < argc) {
    options->input_file_name_ = argv[i];
    i++;
  } else {
    fprintf(stderr, "INPUT required\n");
    return java_usage();
  }
  if (!EndsWith(options->input_file_name_, ".aidl")) {
    cerr << "Expected .aidl file for input but got "
         << options->input_file_name_ << endl;
    return java_usage();
  }

  // OUTPUT
  if (i < argc) {
    options->output_file_name_ = argv[i];
    i++;
  } else if (options->output_base_folder_.empty()) {
    // copy input into output and change the extension from .aidl to .java
    options->output_file_name_= options->input_file_name_;
    if (!ReplaceSuffix(".aidl", ".java", &options->output_file_name_)) {
      // we should never get here since we validated the suffix.
      LOG(FATAL) << "Internal aidl error.";
      return java_usage();
    }
  }

  // anything remaining?
  if (i != argc) {
    fprintf(stderr, "unknown option%s:",
            (i == argc - 1 ? (const char*)"" : (const char*)"s"));
    for (; i < argc - 1; i++) {
      fprintf(stderr, " %s", argv[i]);
    }
    fprintf(stderr, "\n");
    return java_usage();
  }

  return options;
}

string JavaOptions::DependencyFilePath() const {
  if (auto_dep_file_) {
    return output_file_name_ + ".d";
  }
  return dep_file_name_;
}

namespace {

unique_ptr<CppOptions> cpp_usage() {
  cerr << "usage: aidl-cpp INPUT_FILE HEADER_DIR OUTPUT_FILE" << endl
       << endl
       << "OPTIONS:" << endl
       << "   -I<DIR>   search path for import statements" << endl
       << "   -d<FILE>  generate dependency file" << endl
       << endl
       << "INPUT_FILE:" << endl
       << "   an aidl interface file" << endl
       << "HEADER_DIR:" << endl
       << "   empty directory to put generated headers" << endl
       << "OUTPUT_FILE:" << endl
       << "   path to write generated .cpp code" << endl;
  return unique_ptr<CppOptions>(nullptr);
}

}  // namespace

unique_ptr<CppOptions> CppOptions::Parse(int argc, const char* const* argv) {
  unique_ptr<CppOptions> options(new CppOptions());
  int i = 1;

  // Parse flags, all of which start with '-'
  for ( ; i < argc; ++i) {
    const size_t len = strlen(argv[i]);
    const char *s = argv[i];
    if (s[0] != '-') {
      break;  // On to the positional arguments.
    }
    if (len < 2) {
      cerr << "Invalid argument '" << s << "'." << endl;
      return cpp_usage();
    }
    const string the_rest = s + 2;
    if (s[1] == 'I') {
      options->import_paths_.push_back(the_rest);
    } else if (s[1] == 'd') {
      options->dep_file_name_ = the_rest;
    } else {
      cerr << "Invalid argument '" << s << "'." << endl;
      return cpp_usage();
    }
  }

  // There are exactly three positional arguments.
  const int remaining_args = argc - i;
  if (remaining_args != 3) {
    cerr << "Expected 3 positional arguments but got " << remaining_args << "." << endl;
    return cpp_usage();
  }

  options->input_file_name_ = argv[i];
  options->output_header_dir_ = argv[i + 1];
  options->output_file_name_ = argv[i + 2];

  if (!EndsWith(options->input_file_name_, ".aidl")) {
    cerr << "Expected .aidl file for input but got " << options->input_file_name_ << endl;
    return cpp_usage();
  }

  return options;
}

bool EndsWith(const string& str, const string& suffix) {
  if (str.length() < suffix.length()) {
    return false;
  }
  return std::equal(str.crbegin(), str.crbegin() + suffix.length(),
                    suffix.crbegin());
}

bool ReplaceSuffix(const string& old_suffix,
                   const string& new_suffix,
                   string* str) {
  if (!EndsWith(*str, old_suffix)) return false;
  str->replace(str->length() - old_suffix.length(),
               old_suffix.length(),
               new_suffix);
  return true;
}



}  // namespace android
}  // namespace aidl
