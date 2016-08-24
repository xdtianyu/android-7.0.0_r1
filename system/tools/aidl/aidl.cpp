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

#include "aidl.h"

#include <fcntl.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#endif

#include <android-base/strings.h>

#include "aidl_language.h"
#include "generate_cpp.h"
#include "generate_java.h"
#include "import_resolver.h"
#include "logging.h"
#include "options.h"
#include "os.h"
#include "type_cpp.h"
#include "type_java.h"
#include "type_namespace.h"

#ifndef O_BINARY
#  define O_BINARY  0
#endif

using android::base::Join;
using android::base::Split;
using std::cerr;
using std::endl;
using std::map;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

namespace android {
namespace aidl {
namespace {

// The following are gotten as the offset from the allowable id's between
// android.os.IBinder.FIRST_CALL_TRANSACTION=1 and
// android.os.IBinder.LAST_CALL_TRANSACTION=16777215
const int kMinUserSetMethodId = 0;
const int kMaxUserSetMethodId = 16777214;

bool check_filename(const std::string& filename,
                    const std::string& package,
                    const std::string& name,
                    unsigned line) {
    const char* p;
    string expected;
    string fn;
    size_t len;
    bool valid = false;

    if (!IoDelegate::GetAbsolutePath(filename, &fn)) {
      return false;
    }

    if (!package.empty()) {
        expected = package;
        expected += '.';
    }

    len = expected.length();
    for (size_t i=0; i<len; i++) {
        if (expected[i] == '.') {
            expected[i] = OS_PATH_SEPARATOR;
        }
    }

    expected.append(name, 0, name.find('.'));

    expected += ".aidl";

    len = fn.length();
    valid = (len >= expected.length());

    if (valid) {
        p = fn.c_str() + (len - expected.length());

#ifdef _WIN32
        if (OS_PATH_SEPARATOR != '/') {
            // Input filename under cygwin most likely has / separators
            // whereas the expected string uses \\ separators. Adjust
            // them accordingly.
          for (char *c = const_cast<char *>(p); *c; ++c) {
                if (*c == '/') *c = OS_PATH_SEPARATOR;
            }
        }
#endif

        // aidl assumes case-insensitivity on Mac Os and Windows.
#if defined(__linux__)
        valid = (expected == p);
#else
        valid = !strcasecmp(expected.c_str(), p);
#endif
    }

    if (!valid) {
        fprintf(stderr, "%s:%d interface %s should be declared in a file"
                " called %s.\n",
                filename.c_str(), line, name.c_str(), expected.c_str());
    }

    return valid;
}

bool check_filenames(const std::string& filename, const AidlDocument* doc) {
  if (!doc)
    return true;

  const AidlInterface* interface = doc->GetInterface();

  if (interface) {
    return check_filename(filename, interface->GetPackage(),
                          interface->GetName(), interface->GetLine());
  }

  bool success = true;

  for (const auto& item : doc->GetParcelables()) {
    success &= check_filename(filename, item->GetPackage(), item->GetName(),
                              item->GetLine());
  }

  return success;
}

bool gather_types(const std::string& filename,
                  const AidlDocument* doc,
                  TypeNamespace* types) {
  bool success = true;

  const AidlInterface* interface = doc->GetInterface();

  if (interface)
    return types->AddBinderType(*interface, filename);

  for (const auto& item : doc->GetParcelables()) {
    success &= types->AddParcelableType(*item, filename);
  }

  return success;
}

int check_types(const string& filename,
                const AidlInterface* c,
                TypeNamespace* types) {
  int err = 0;

  // Has to be a pointer due to deleting copy constructor. No idea why.
  map<string, const AidlMethod*> method_names;
  for (const auto& m : c->GetMethods()) {
    bool oneway = m->IsOneway() || c->IsOneway();

    if (!types->MaybeAddContainerType(m->GetType())) {
      err = 1;  // return type is invalid
    }

    const ValidatableType* return_type =
        types->GetReturnType(m->GetType(), filename);

    if (!return_type) {
      err = 1;
    }

    m->GetMutableType()->SetLanguageType(return_type);

    if (oneway && m->GetType().GetName() != "void") {
        cerr << filename << ":" << m->GetLine()
            << " oneway method '" << m->GetName() << "' cannot return a value"
            << endl;
        err = 1;
    }

    int index = 1;
    for (const auto& arg : m->GetArguments()) {
      if (!types->MaybeAddContainerType(arg->GetType())) {
        err = 1;
      }

      const ValidatableType* arg_type =
          types->GetArgType(*arg, index, filename);

      if (!arg_type) {
        err = 1;
      }

      arg->GetMutableType()->SetLanguageType(arg_type);

      if (oneway && arg->IsOut()) {
        cerr << filename << ":" << m->GetLine()
            << " oneway method '" << m->GetName()
            << "' cannot have out parameters" << endl;
        err = 1;
      }
    }

    auto it = method_names.find(m->GetName());
    // prevent duplicate methods
    if (it == method_names.end()) {
      method_names[m->GetName()] = m.get();
    } else {
      cerr << filename << ":" << m->GetLine()
           << " attempt to redefine method " << m->GetName() << "," << endl
           << filename << ":" << it->second->GetLine()
           << "    previously defined here." << endl;
      err = 1;
    }
  }
  return err;
}

void write_common_dep_file(const string& output_file,
                           const vector<string>& aidl_sources,
                           CodeWriter* writer) {
  // Encode that the output file depends on aidl input files.
  writer->Write("%s : \\\n", output_file.c_str());
  writer->Write("  %s", Join(aidl_sources, " \\\n  ").c_str());
  writer->Write("\n\n");

  // Output "<input_aidl_file>: " so make won't fail if the input .aidl file
  // has been deleted, moved or renamed in incremental build.
  for (const auto& src : aidl_sources) {
    writer->Write("%s :\n", src.c_str());
  }
}

bool write_java_dep_file(const JavaOptions& options,
                         const vector<unique_ptr<AidlImport>>& imports,
                         const IoDelegate& io_delegate,
                         const string& output_file_name) {
  string dep_file_name = options.DependencyFilePath();
  if (dep_file_name.empty()) {
    return true;  // nothing to do
  }
  CodeWriterPtr writer = io_delegate.GetCodeWriter(dep_file_name);
  if (!writer) {
    LOG(ERROR) << "Could not open dependency file: " << dep_file_name;
    return false;
  }

  vector<string> source_aidl = {options.input_file_name_};
  for (const auto& import : imports) {
    if (!import->GetFilename().empty()) {
      source_aidl.push_back(import->GetFilename());
    }
  }

  write_common_dep_file(output_file_name, source_aidl, writer.get());

  return true;
}

bool write_cpp_dep_file(const CppOptions& options,
                        const AidlInterface& interface,
                        const vector<unique_ptr<AidlImport>>& imports,
                        const IoDelegate& io_delegate) {
  using ::android::aidl::cpp::HeaderFile;
  using ::android::aidl::cpp::ClassNames;

  string dep_file_name = options.DependencyFilePath();
  if (dep_file_name.empty()) {
    return true;  // nothing to do
  }
  CodeWriterPtr writer = io_delegate.GetCodeWriter(dep_file_name);
  if (!writer) {
    LOG(ERROR) << "Could not open dependency file: " << dep_file_name;
    return false;
  }

  vector<string> source_aidl = {options.InputFileName()};
  for (const auto& import : imports) {
    if (!import->GetFilename().empty()) {
      source_aidl.push_back(import->GetFilename());
    }
  }

  vector<string> headers;
  for (ClassNames c : {ClassNames::CLIENT,
                       ClassNames::SERVER,
                       ClassNames::INTERFACE}) {
    headers.push_back(options.OutputHeaderDir() + '/' +
                      HeaderFile(interface, c, false /* use_os_sep */));
  }

  write_common_dep_file(options.OutputCppFilePath(), source_aidl, writer.get());
  writer->Write("\n");

  // Generated headers also depend on the source aidl files.
  writer->Write("%s : \\\n    %s\n", Join(headers, " \\\n    ").c_str(),
                Join(source_aidl, " \\\n    ").c_str());

  return true;
}

string generate_outputFileName(const JavaOptions& options,
                               const AidlInterface& interface) {
    string name = interface.GetName();
    string package = interface.GetPackage();
    string result;

    // create the path to the destination folder based on the
    // interface package name
    result = options.output_base_folder_;
    result += OS_PATH_SEPARATOR;

    string packageStr = package;
    size_t len = packageStr.length();
    for (size_t i=0; i<len; i++) {
        if (packageStr[i] == '.') {
            packageStr[i] = OS_PATH_SEPARATOR;
        }
    }

    result += packageStr;

    // add the filename by replacing the .aidl extension to .java
    result += OS_PATH_SEPARATOR;
    result.append(name, 0, name.find('.'));
    result += ".java";

    return result;
}

int check_and_assign_method_ids(const char * filename,
                                const std::vector<std::unique_ptr<AidlMethod>>& items) {
    // Check whether there are any methods with manually assigned id's and any that are not.
    // Either all method id's must be manually assigned or all of them must not.
    // Also, check for duplicates of user set id's and that the id's are within the proper bounds.
    set<int> usedIds;
    bool hasUnassignedIds = false;
    bool hasAssignedIds = false;
    for (const auto& item : items) {
        if (item->HasId()) {
            hasAssignedIds = true;
            // Ensure that the user set id is not duplicated.
            if (usedIds.find(item->GetId()) != usedIds.end()) {
                // We found a duplicate id, so throw an error.
                fprintf(stderr,
                        "%s:%d Found duplicate method id (%d) for method: %s\n",
                        filename, item->GetLine(),
                        item->GetId(), item->GetName().c_str());
                return 1;
            }
            // Ensure that the user set id is within the appropriate limits
            if (item->GetId() < kMinUserSetMethodId ||
                    item->GetId() > kMaxUserSetMethodId) {
                fprintf(stderr, "%s:%d Found out of bounds id (%d) for method: %s\n",
                        filename, item->GetLine(),
                        item->GetId(), item->GetName().c_str());
                fprintf(stderr, "    Value for id must be between %d and %d inclusive.\n",
                        kMinUserSetMethodId, kMaxUserSetMethodId);
                return 1;
            }
            usedIds.insert(item->GetId());
        } else {
            hasUnassignedIds = true;
        }
        if (hasAssignedIds && hasUnassignedIds) {
            fprintf(stderr,
                    "%s: You must either assign id's to all methods or to none of them.\n",
                    filename);
            return 1;
        }
    }

    // In the case that all methods have unassigned id's, set a unique id for them.
    if (hasUnassignedIds) {
        int newId = 0;
        for (const auto& item : items) {
            item->SetId(newId++);
        }
    }

    // success
    return 0;
}

// TODO: Remove this in favor of using the YACC parser b/25479378
bool ParsePreprocessedLine(const string& line, string* decl,
                           vector<string>* package, string* class_name) {
  // erase all trailing whitespace and semicolons
  const size_t end = line.find_last_not_of(" ;\t");
  if (end == string::npos) {
    return false;
  }
  if (line.rfind(';', end) != string::npos) {
    return false;
  }

  decl->clear();
  string type;
  vector<string> pieces = Split(line.substr(0, end + 1), " \t");
  for (const string& piece : pieces) {
    if (piece.empty()) {
      continue;
    }
    if (decl->empty()) {
      *decl = std::move(piece);
    } else if (type.empty()) {
      type = std::move(piece);
    } else {
      return false;
    }
  }

  // Note that this logic is absolutely wrong.  Given a parcelable
  // org.some.Foo.Bar, the class name is Foo.Bar, but this code will claim that
  // the class is just Bar.  However, this was the way it was done in the past.
  //
  // See b/17415692
  size_t dot_pos = type.rfind('.');
  if (dot_pos != string::npos) {
    *class_name = type.substr(dot_pos + 1);
    *package = Split(type.substr(0, dot_pos), ".");
  } else {
    *class_name = type;
    package->clear();
  }

  return true;
}

}  // namespace

namespace internals {

bool parse_preprocessed_file(const IoDelegate& io_delegate,
                             const string& filename, TypeNamespace* types) {
  bool success = true;
  unique_ptr<LineReader> line_reader = io_delegate.GetLineReader(filename);
  if (!line_reader) {
    LOG(ERROR) << "cannot open preprocessed file: " << filename;
    success = false;
    return success;
  }

  string line;
  unsigned lineno = 1;
  for ( ; line_reader->ReadLine(&line); ++lineno) {
    if (line.empty() || line.compare(0, 2, "//") == 0) {
      // skip comments and empty lines
      continue;
    }

    string decl;
    vector<string> package;
    string class_name;
    if (!ParsePreprocessedLine(line, &decl, &package, &class_name)) {
      success = false;
      break;
    }

    if (decl == "parcelable") {
      AidlParcelable doc(new AidlQualifiedName(class_name, ""),
                         lineno, package);
      types->AddParcelableType(doc, filename);
    } else if (decl == "interface") {
      auto temp = new std::vector<std::unique_ptr<AidlMember>>();
      AidlInterface doc(class_name, lineno, "", false, temp, package);
      types->AddBinderType(doc, filename);
    } else {
      success = false;
      break;
    }
  }
  if (!success) {
    LOG(ERROR) << filename << ':' << lineno
               << " malformed preprocessed file line: '" << line << "'";
  }

  return success;
}

AidlError load_and_validate_aidl(
    const std::vector<std::string> preprocessed_files,
    const std::vector<std::string> import_paths,
    const std::string& input_file_name,
    const IoDelegate& io_delegate,
    TypeNamespace* types,
    std::unique_ptr<AidlInterface>* returned_interface,
    std::vector<std::unique_ptr<AidlImport>>* returned_imports) {
  AidlError err = AidlError::OK;

  std::map<AidlImport*,std::unique_ptr<AidlDocument>> docs;

  // import the preprocessed file
  for (const string& s : preprocessed_files) {
    if (!parse_preprocessed_file(io_delegate, s, types)) {
      err = AidlError::BAD_PRE_PROCESSED_FILE;
    }
  }
  if (err != AidlError::OK) {
    return err;
  }

  // parse the input file
  Parser p{io_delegate};
  if (!p.ParseFile(input_file_name)) {
    return AidlError::PARSE_ERROR;
  }

  AidlDocument* parsed_doc = p.GetDocument();

  unique_ptr<AidlInterface> interface(parsed_doc->ReleaseInterface());

  if (!interface) {
    LOG(ERROR) << "refusing to generate code from aidl file defining "
                  "parcelable";
    return AidlError::FOUND_PARCELABLE;
  }

  if (!check_filename(input_file_name.c_str(), interface->GetPackage(),
                      interface->GetName(), interface->GetLine()) ||
      !types->IsValidPackage(interface->GetPackage())) {
    LOG(ERROR) << "Invalid package declaration '" << interface->GetPackage()
               << "'";
    return AidlError::BAD_PACKAGE;
  }

  // parse the imports of the input file
  ImportResolver import_resolver{io_delegate, import_paths};
  for (auto& import : p.GetImports()) {
    if (types->HasImportType(*import)) {
      // There are places in the Android tree where an import doesn't resolve,
      // but we'll pick the type up through the preprocessed types.
      // This seems like an error, but legacy support demands we support it...
      continue;
    }
    string import_path = import_resolver.FindImportFile(import->GetNeededClass());
    if (import_path.empty()) {
      cerr << import->GetFileFrom() << ":" << import->GetLine()
           << ": couldn't find import for class "
           << import->GetNeededClass() << endl;
      err = AidlError::BAD_IMPORT;
      continue;
    }
    import->SetFilename(import_path);

    Parser p{io_delegate};
    if (!p.ParseFile(import->GetFilename())) {
      cerr << "error while parsing import for class "
           << import->GetNeededClass() << endl;
      err = AidlError::BAD_IMPORT;
      continue;
    }

    std::unique_ptr<AidlDocument> document(p.ReleaseDocument());
    if (!check_filenames(import->GetFilename(), document.get()))
      err = AidlError::BAD_IMPORT;
    docs[import.get()] = std::move(document);
  }
  if (err != AidlError::OK) {
    return err;
  }

  // gather the types that have been declared
  if (!types->AddBinderType(*interface.get(), input_file_name)) {
    err = AidlError::BAD_TYPE;
  }

  interface->SetLanguageType(types->GetInterfaceType(*interface));

  for (const auto& import : p.GetImports()) {
    // If we skipped an unresolved import above (see comment there) we'll have
    // an empty bucket here.
    const auto import_itr = docs.find(import.get());
    if (import_itr == docs.cend()) {
      continue;
    }

    if (!gather_types(import->GetFilename(), import_itr->second.get(), types)) {
      err = AidlError::BAD_TYPE;
    }
  }

  // check the referenced types in parsed_doc to make sure we've imported them
  if (check_types(input_file_name, interface.get(), types) != 0) {
    err = AidlError::BAD_TYPE;
  }
  if (err != AidlError::OK) {
    return err;
  }


  // assign method ids and validate.
  if (check_and_assign_method_ids(input_file_name.c_str(),
                                  interface->GetMethods()) != 0) {
    return AidlError::BAD_METHOD_ID;
  }

  if (returned_interface)
    *returned_interface = std::move(interface);

  if (returned_imports)
    p.ReleaseImports(returned_imports);

  return AidlError::OK;
}

} // namespace internals

int compile_aidl_to_cpp(const CppOptions& options,
                        const IoDelegate& io_delegate) {
  unique_ptr<AidlInterface> interface;
  std::vector<std::unique_ptr<AidlImport>> imports;
  unique_ptr<cpp::TypeNamespace> types(new cpp::TypeNamespace());
  types->Init();
  AidlError err = internals::load_and_validate_aidl(
      std::vector<std::string>{},  // no preprocessed files
      options.ImportPaths(),
      options.InputFileName(),
      io_delegate,
      types.get(),
      &interface,
      &imports);
  if (err != AidlError::OK) {
    return 1;
  }

  if (!write_cpp_dep_file(options, *interface, imports, io_delegate)) {
    return 1;
  }

  return (cpp::GenerateCpp(options, *types, *interface, io_delegate)) ? 0 : 1;
}

int compile_aidl_to_java(const JavaOptions& options,
                         const IoDelegate& io_delegate) {
  unique_ptr<AidlInterface> interface;
  std::vector<std::unique_ptr<AidlImport>> imports;
  unique_ptr<java::JavaTypeNamespace> types(new java::JavaTypeNamespace());
  types->Init();
  AidlError aidl_err = internals::load_and_validate_aidl(
      options.preprocessed_files_,
      options.import_paths_,
      options.input_file_name_,
      io_delegate,
      types.get(),
      &interface,
      &imports);
  if (aidl_err == AidlError::FOUND_PARCELABLE && !options.fail_on_parcelable_) {
    // We aborted code generation because this file contains parcelables.
    // However, we were not told to complain if we find parcelables.
    // Just generate a dep file and exit quietly.  The dep file is for a legacy
    // use case by the SDK.
    write_java_dep_file(options, imports, io_delegate, "");
    return 0;
  }
  if (aidl_err != AidlError::OK) {
    return 1;
  }

  string output_file_name = options.output_file_name_;
  // if needed, generate the output file name from the base folder
  if (output_file_name.empty() && !options.output_base_folder_.empty()) {
    output_file_name = generate_outputFileName(options, *interface);
  }

  // make sure the folders of the output file all exists
  if (!io_delegate.CreatePathForFile(output_file_name)) {
    return 1;
  }

  if (!write_java_dep_file(options, imports, io_delegate, output_file_name)) {
    return 1;
  }

  return generate_java(output_file_name, options.input_file_name_.c_str(),
                       interface.get(), types.get(), io_delegate);
}

bool preprocess_aidl(const JavaOptions& options,
                     const IoDelegate& io_delegate) {
  unique_ptr<CodeWriter> writer =
      io_delegate.GetCodeWriter(options.output_file_name_);

  for (const auto& file : options.files_to_preprocess_) {
    Parser p{io_delegate};
    if (!p.ParseFile(file))
      return false;
    AidlDocument* doc = p.GetDocument();
    string line;

    const AidlInterface* interface = doc->GetInterface();

    if (interface != nullptr &&
        !writer->Write("interface %s;\n",
                       interface->GetCanonicalName().c_str())) {
      return false;
    }

    for (const auto& parcelable : doc->GetParcelables()) {
      if (!writer->Write("parcelable %s;\n",
                         parcelable->GetCanonicalName().c_str())) {
        return false;
      }
    }
  }

  return writer->Close();
}

}  // namespace android
}  // namespace aidl
