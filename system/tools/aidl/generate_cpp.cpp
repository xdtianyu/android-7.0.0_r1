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

#include "generate_cpp.h"

#include <cctype>
#include <cstring>
#include <memory>
#include <random>
#include <set>
#include <string>

#include <android-base/stringprintf.h>

#include "aidl_language.h"
#include "ast_cpp.h"
#include "code_writer.h"
#include "logging.h"
#include "os.h"

using android::base::StringPrintf;
using std::string;
using std::unique_ptr;
using std::vector;
using std::set;

namespace android {
namespace aidl {
namespace cpp {
namespace internals {
namespace {

const char kAndroidStatusVarName[] = "_aidl_ret_status";
const char kCodeVarName[] = "_aidl_code";
const char kFlagsVarName[] = "_aidl_flags";
const char kDataVarName[] = "_aidl_data";
const char kErrorLabel[] = "_aidl_error";
const char kImplVarName[] = "_aidl_impl";
const char kReplyVarName[] = "_aidl_reply";
const char kReturnVarName[] = "_aidl_return";
const char kStatusVarName[] = "_aidl_status";
const char kAndroidParcelLiteral[] = "::android::Parcel";
const char kAndroidStatusLiteral[] = "::android::status_t";
const char kAndroidStatusOk[] = "::android::OK";
const char kBinderStatusLiteral[] = "::android::binder::Status";
const char kIBinderHeader[] = "binder/IBinder.h";
const char kIInterfaceHeader[] = "binder/IInterface.h";
const char kParcelHeader[] = "binder/Parcel.h";
const char kStatusHeader[] = "binder/Status.h";
const char kStrongPointerHeader[] = "utils/StrongPointer.h";

unique_ptr<AstNode> BreakOnStatusNotOk() {
  IfStatement* ret = new IfStatement(new Comparison(
      new LiteralExpression(kAndroidStatusVarName), "!=",
      new LiteralExpression(kAndroidStatusOk)));
  ret->OnTrue()->AddLiteral("break");
  return unique_ptr<AstNode>(ret);
}

unique_ptr<AstNode> GotoErrorOnBadStatus() {
  IfStatement* ret = new IfStatement(new Comparison(
      new LiteralExpression(kAndroidStatusVarName), "!=",
      new LiteralExpression(kAndroidStatusOk)));
  ret->OnTrue()->AddLiteral(StringPrintf("goto %s", kErrorLabel));
  return unique_ptr<AstNode>(ret);
}


unique_ptr<AstNode> ReturnOnStatusNotOk() {
  IfStatement* ret = new IfStatement(new Comparison(
      new LiteralExpression(kAndroidStatusVarName), "!=",
      new LiteralExpression(kAndroidStatusOk)));
  ret->OnTrue()->AddLiteral(StringPrintf("return %s", kAndroidStatusVarName));
  return unique_ptr<AstNode>(ret);
}

string UpperCase(const std::string& s) {
  string result = s;
  for (char& c : result)
    c = toupper(c);
  return result;
}

string BuildVarName(const AidlArgument& a) {
  string prefix = "out_";
  if (a.GetDirection() & AidlArgument::IN_DIR) {
    prefix = "in_";
  }
  return prefix + a.GetName();
}

ArgList BuildArgList(const TypeNamespace& types,
                     const AidlMethod& method,
                     bool for_declaration) {
  // Build up the argument list for the server method call.
  vector<string> method_arguments;
  for (const unique_ptr<AidlArgument>& a : method.GetArguments()) {
    string literal;
    if (for_declaration) {
      // Method declarations need types, pointers to out params, and variable
      // names that match the .aidl specification.
      const Type* type = a->GetType().GetLanguageType<Type>();

      literal = type->CppType();

      if (a->IsOut()) {
        literal = literal + "*";
      } else {
        // We pass in parameters that are not primitives by const reference.
        // Arrays of primitives are not primitives.
        if (!type->IsCppPrimitive() || a->GetType().IsArray()) {
          literal = "const " + literal + "&";
        }
      }

      literal += " " + a->GetName();
    } else {
      if (a->IsOut()) { literal = "&"; }
      literal += BuildVarName(*a);
    }
    method_arguments.push_back(literal);
  }

  const Type* return_type = method.GetType().GetLanguageType<Type>();

  if (return_type != types.VoidType()) {
    string literal;
    if (for_declaration) {
      literal = StringPrintf(
          "%s* %s", return_type->CppType().c_str(),
          kReturnVarName);
    } else {
      literal = string{"&"} + kReturnVarName;
    }
    method_arguments.push_back(literal);
  }

  return ArgList(method_arguments);
}

unique_ptr<Declaration> BuildMethodDecl(const AidlMethod& method,
                                        const TypeNamespace& types,
                                        bool for_interface) {
  uint32_t modifiers = 0;
  if (for_interface) {
    modifiers |= MethodDecl::IS_VIRTUAL;
    modifiers |= MethodDecl::IS_PURE_VIRTUAL;
  } else {
    modifiers |= MethodDecl::IS_OVERRIDE;
  }

  return unique_ptr<Declaration>{
      new MethodDecl{kBinderStatusLiteral,
                     method.GetName(),
                     BuildArgList(types, method, true /* for method decl */),
                     modifiers}};
}

unique_ptr<CppNamespace> NestInNamespaces(
    vector<unique_ptr<Declaration>> decls,
    const vector<string>& package) {
  if (package.empty()) {
    // We should also be checking this before we get this far, but do it again
    // for the sake of unit tests and meaningful errors.
    LOG(FATAL) << "C++ generation requires a package declaration "
                  "for namespacing";
  }
  auto it = package.crbegin();  // Iterate over the namespaces inner to outer
  unique_ptr<CppNamespace> inner{new CppNamespace{*it, std::move(decls)}};
  ++it;
  for (; it != package.crend(); ++it) {
    inner.reset(new CppNamespace{*it, std::move(inner)});
  }
  return inner;
}

unique_ptr<CppNamespace> NestInNamespaces(unique_ptr<Declaration> decl,
                                          const vector<string>& package) {
  vector<unique_ptr<Declaration>> decls;
  decls.push_back(std::move(decl));
  return NestInNamespaces(std::move(decls), package);
}

bool DeclareLocalVariable(const TypeNamespace& types, const AidlArgument& a,
                          StatementBlock* b) {
  const Type* cpp_type = a.GetType().GetLanguageType<Type>();
  if (!cpp_type) { return false; }

  string type = cpp_type->CppType();

  b->AddLiteral(type + " " + BuildVarName(a));
  return true;
}

string ClassName(const AidlInterface& interface, ClassNames type) {
  string c_name = interface.GetName();

  if (c_name.length() >= 2 && c_name[0] == 'I' && isupper(c_name[1]))
    c_name = c_name.substr(1);

  switch (type) {
    case ClassNames::CLIENT:
      c_name = "Bp" + c_name;
      break;
    case ClassNames::SERVER:
      c_name = "Bn" + c_name;
      break;
    case ClassNames::INTERFACE:
      c_name = "I" + c_name;
      break;
    case ClassNames::BASE:
      break;
  }
  return c_name;
}

string BuildHeaderGuard(const AidlInterface& interface,
                        ClassNames header_type) {
  string class_name = ClassName(interface, header_type);
  for (size_t i = 1; i < class_name.size(); ++i) {
    if (isupper(class_name[i])) {
      class_name.insert(i, "_");
      ++i;
    }
  }
  string ret = StringPrintf("AIDL_GENERATED_%s_%s_H_",
                            interface.GetPackage().c_str(),
                            class_name.c_str());
  for (char& c : ret) {
    if (c == '.') {
      c = '_';
    }
    c = toupper(c);
  }
  return ret;
}

unique_ptr<Declaration> DefineClientTransaction(const TypeNamespace& types,
                                                const AidlInterface& interface,
                                                const AidlMethod& method) {
  const string i_name = ClassName(interface, ClassNames::INTERFACE);
  const string bp_name = ClassName(interface, ClassNames::CLIENT);
  unique_ptr<MethodImpl> ret{new MethodImpl{
      kBinderStatusLiteral, bp_name, method.GetName(),
      ArgList{BuildArgList(types, method, true /* for method decl */)}}};
  StatementBlock* b = ret->GetStatementBlock();

  // Declare parcels to hold our query and the response.
  b->AddLiteral(StringPrintf("%s %s", kAndroidParcelLiteral, kDataVarName));
  // Even if we're oneway, the transact method still takes a parcel.
  b->AddLiteral(StringPrintf("%s %s", kAndroidParcelLiteral, kReplyVarName));

  // Declare the status_t variable we need for error handling.
  b->AddLiteral(StringPrintf("%s %s = %s", kAndroidStatusLiteral,
                             kAndroidStatusVarName,
                             kAndroidStatusOk));
  // We unconditionally return a Status object.
  b->AddLiteral(StringPrintf("%s %s", kBinderStatusLiteral, kStatusVarName));

  // Add the name of the interface we're hoping to call.
  b->AddStatement(new Assignment(
      kAndroidStatusVarName,
      new MethodCall(StringPrintf("%s.writeInterfaceToken",
                                  kDataVarName),
                     "getInterfaceDescriptor()")));
  b->AddStatement(GotoErrorOnBadStatus());

  // Serialization looks roughly like:
  //     _aidl_ret_status = _aidl_data.WriteInt32(in_param_name);
  //     if (_aidl_ret_status != ::android::OK) { goto error; }
  for (const AidlArgument* a : method.GetInArguments()) {
    const Type* type = a->GetType().GetLanguageType<Type>();
    string method = type->WriteToParcelMethod();

    string var_name = ((a->IsOut()) ? "*" : "") + a->GetName();
    var_name = type->WriteCast(var_name);
    b->AddStatement(new Assignment(
        kAndroidStatusVarName,
        new MethodCall(StringPrintf("%s.%s", kDataVarName, method.c_str()),
                       ArgList(var_name))));
    b->AddStatement(GotoErrorOnBadStatus());
  }

  // Invoke the transaction on the remote binder and confirm status.
  string transaction_code = StringPrintf(
      "%s::%s", i_name.c_str(), UpperCase(method.GetName()).c_str());

  vector<string> args = {transaction_code, kDataVarName,
                         StringPrintf("&%s", kReplyVarName)};

  if (interface.IsOneway() || method.IsOneway()) {
    args.push_back("::android::IBinder::FLAG_ONEWAY");
  }

  b->AddStatement(new Assignment(
      kAndroidStatusVarName,
      new MethodCall("remote()->transact",
                     ArgList(args))));
  b->AddStatement(GotoErrorOnBadStatus());

  if (!interface.IsOneway() && !method.IsOneway()) {
    // Strip off the exception header and fail if we see a remote exception.
    // _aidl_ret_status = _aidl_status.readFromParcel(_aidl_reply);
    // if (_aidl_ret_status != ::android::OK) { goto error; }
    // if (!_aidl_status.isOk()) { return _aidl_ret_status; }
    b->AddStatement(new Assignment(
        kAndroidStatusVarName,
        StringPrintf("%s.readFromParcel(%s)", kStatusVarName, kReplyVarName)));
    b->AddStatement(GotoErrorOnBadStatus());
    IfStatement* exception_check = new IfStatement(
        new LiteralExpression(StringPrintf("!%s.isOk()", kStatusVarName)));
    b->AddStatement(exception_check);
    exception_check->OnTrue()->AddLiteral(
        StringPrintf("return %s", kStatusVarName));
  }

  // Type checking should guarantee that nothing below emits code until "return
  // status" if we are a oneway method, so no more fear of accessing reply.

  // If the method is expected to return something, read it first by convention.
  const Type* return_type = method.GetType().GetLanguageType<Type>();
  if (return_type != types.VoidType()) {
    string method_call = return_type->ReadFromParcelMethod();
    b->AddStatement(new Assignment(
        kAndroidStatusVarName,
        new MethodCall(StringPrintf("%s.%s", kReplyVarName,
                                    method_call.c_str()),
                       ArgList(kReturnVarName))));
    b->AddStatement(GotoErrorOnBadStatus());
  }

  for (const AidlArgument* a : method.GetOutArguments()) {
    // Deserialization looks roughly like:
    //     _aidl_ret_status = _aidl_reply.ReadInt32(out_param_name);
    //     if (_aidl_status != ::android::OK) { goto _aidl_error; }
    string method =
      a->GetType().GetLanguageType<Type>()->ReadFromParcelMethod();

    b->AddStatement(new Assignment(
        kAndroidStatusVarName,
        new MethodCall(StringPrintf("%s.%s", kReplyVarName,
                                    method.c_str()),
                       ArgList(a->GetName()))));
    b->AddStatement(GotoErrorOnBadStatus());
  }

  // If we've gotten to here, one of two things is true:
  //   1) We've read some bad status_t
  //   2) We've only read status_t == OK and there was no exception in the
  //      response.
  // In both cases, we're free to set Status from the status_t and return.
  b->AddLiteral(StringPrintf("%s:\n", kErrorLabel), false /* no semicolon */);
  b->AddLiteral(
      StringPrintf("%s.setFromStatusT(%s)", kStatusVarName,
                   kAndroidStatusVarName));
  b->AddLiteral(StringPrintf("return %s", kStatusVarName));

  return unique_ptr<Declaration>(ret.release());
}

}  // namespace

unique_ptr<Document> BuildClientSource(const TypeNamespace& types,
                                       const AidlInterface& interface) {
  vector<string> include_list = {
      HeaderFile(interface, ClassNames::CLIENT, false),
      kParcelHeader
  };
  vector<unique_ptr<Declaration>> file_decls;

  // The constructor just passes the IBinder instance up to the super
  // class.
  const string i_name = ClassName(interface, ClassNames::INTERFACE);
  file_decls.push_back(unique_ptr<Declaration>{new ConstructorImpl{
      ClassName(interface, ClassNames::CLIENT),
      ArgList{StringPrintf("const ::android::sp<::android::IBinder>& %s",
                           kImplVarName)},
      { "BpInterface<" + i_name + ">(" + kImplVarName + ")" }}});

  // Clients define a method per transaction.
  for (const auto& method : interface.GetMethods()) {
    unique_ptr<Declaration> m = DefineClientTransaction(
        types, interface, *method);
    if (!m) { return nullptr; }
    file_decls.push_back(std::move(m));
  }
  return unique_ptr<Document>{new CppSource{
      include_list,
      NestInNamespaces(std::move(file_decls), interface.GetSplitPackage())}};
}

namespace {

bool HandleServerTransaction(const TypeNamespace& types,
                             const AidlMethod& method,
                             StatementBlock* b) {
  // Declare all the parameters now.  In the common case, we expect no errors
  // in serialization.
  for (const unique_ptr<AidlArgument>& a : method.GetArguments()) {
    if (!DeclareLocalVariable(types, *a, b)) { return false; }
  }

  // Declare a variable to hold the return value.
  const Type* return_type = method.GetType().GetLanguageType<Type>();
  if (return_type != types.VoidType()) {
    b->AddLiteral(StringPrintf(
        "%s %s", return_type->CppType().c_str(),
        kReturnVarName));
  }

  // Check that the client is calling the correct interface.
  IfStatement* interface_check = new IfStatement(
      new MethodCall(StringPrintf("%s.checkInterface",
                                  kDataVarName), "this"),
      true /* invert the check */);
  b->AddStatement(interface_check);
  interface_check->OnTrue()->AddStatement(
      new Assignment(kAndroidStatusVarName, "::android::BAD_TYPE"));
  interface_check->OnTrue()->AddLiteral("break");

  // Deserialize each "in" parameter to the transaction.
  for (const AidlArgument* a : method.GetInArguments()) {
    // Deserialization looks roughly like:
    //     _aidl_ret_status = _aidl_data.ReadInt32(&in_param_name);
    //     if (_aidl_ret_status != ::android::OK) { break; }
    const Type* type = a->GetType().GetLanguageType<Type>();
    string readMethod = type->ReadFromParcelMethod();

    b->AddStatement(new Assignment{
        kAndroidStatusVarName,
        new MethodCall{string(kDataVarName) + "." + readMethod,
                       "&" + BuildVarName(*a)}});
    b->AddStatement(BreakOnStatusNotOk());
  }

  // Call the actual method.  This is implemented by the subclass.
  vector<unique_ptr<AstNode>> status_args;
  status_args.emplace_back(new MethodCall(
          method.GetName(),
          BuildArgList(types, method, false /* not for method decl */)));
  b->AddStatement(new Statement(new MethodCall(
      StringPrintf("%s %s", kBinderStatusLiteral, kStatusVarName),
      ArgList(std::move(status_args)))));

  // Write exceptions during transaction handling to parcel.
  if (!method.IsOneway()) {
    b->AddStatement(new Assignment(
        kAndroidStatusVarName,
        StringPrintf("%s.writeToParcel(%s)", kStatusVarName, kReplyVarName)));
    b->AddStatement(BreakOnStatusNotOk());
    IfStatement* exception_check = new IfStatement(
        new LiteralExpression(StringPrintf("!%s.isOk()", kStatusVarName)));
    b->AddStatement(exception_check);
    exception_check->OnTrue()->AddLiteral("break");
  }

  // If we have a return value, write it first.
  if (return_type != types.VoidType()) {
    string writeMethod =
        string(kReplyVarName) + "->" +
        return_type->WriteToParcelMethod();
    b->AddStatement(new Assignment{
        kAndroidStatusVarName, new MethodCall{writeMethod,
        ArgList{return_type->WriteCast(kReturnVarName)}}});
    b->AddStatement(BreakOnStatusNotOk());
  }

  // Write each out parameter to the reply parcel.
  for (const AidlArgument* a : method.GetOutArguments()) {
    // Serialization looks roughly like:
    //     _aidl_ret_status = data.WriteInt32(out_param_name);
    //     if (_aidl_ret_status != ::android::OK) { break; }
    const Type* type = a->GetType().GetLanguageType<Type>();
    string writeMethod = type->WriteToParcelMethod();

    b->AddStatement(new Assignment{
        kAndroidStatusVarName,
        new MethodCall{string(kReplyVarName) + "->" + writeMethod,
                       type->WriteCast(BuildVarName(*a))}});
    b->AddStatement(BreakOnStatusNotOk());
  }

  return true;
}

}  // namespace

unique_ptr<Document> BuildServerSource(const TypeNamespace& types,
                                       const AidlInterface& interface) {
  const string bn_name = ClassName(interface, ClassNames::SERVER);
  vector<string> include_list{
      HeaderFile(interface, ClassNames::SERVER, false),
      kParcelHeader
  };
  unique_ptr<MethodImpl> on_transact{new MethodImpl{
      kAndroidStatusLiteral, bn_name, "onTransact",
      ArgList{{StringPrintf("uint32_t %s", kCodeVarName),
               StringPrintf("const %s& %s", kAndroidParcelLiteral,
                            kDataVarName),
               StringPrintf("%s* %s", kAndroidParcelLiteral, kReplyVarName),
               StringPrintf("uint32_t %s", kFlagsVarName)}}
      }};

  // Declare the status_t variable
  on_transact->GetStatementBlock()->AddLiteral(
      StringPrintf("%s %s = %s", kAndroidStatusLiteral, kAndroidStatusVarName,
                   kAndroidStatusOk));

  // Add the all important switch statement, but retain a pointer to it.
  SwitchStatement* s = new SwitchStatement{kCodeVarName};
  on_transact->GetStatementBlock()->AddStatement(s);

  // The switch statement has a case statement for each transaction code.
  for (const auto& method : interface.GetMethods()) {
    StatementBlock* b = s->AddCase("Call::" + UpperCase(method->GetName()));
    if (!b) { return nullptr; }

    if (!HandleServerTransaction(types, *method, b)) { return nullptr; }
  }

  // The switch statement has a default case which defers to the super class.
  // The superclass handles a few pre-defined transactions.
  StatementBlock* b = s->AddCase("");
  b->AddLiteral(StringPrintf(
                "%s = ::android::BBinder::onTransact(%s, %s, "
                "%s, %s)", kAndroidStatusVarName, kCodeVarName,
                kDataVarName, kReplyVarName, kFlagsVarName));

  // If we saw a null reference, we can map that to an appropriate exception.
  IfStatement* null_check = new IfStatement(
      new LiteralExpression(string(kAndroidStatusVarName) +
                            " == ::android::UNEXPECTED_NULL"));
  on_transact->GetStatementBlock()->AddStatement(null_check);
  null_check->OnTrue()->AddStatement(new Assignment(
      kAndroidStatusVarName,
      StringPrintf("%s::fromExceptionCode(%s::EX_NULL_POINTER)"
                   ".writeToParcel(%s)",
                   kBinderStatusLiteral, kBinderStatusLiteral,
                   kReplyVarName)));

  // Finally, the server's onTransact method just returns a status code.
  on_transact->GetStatementBlock()->AddLiteral(
      StringPrintf("return %s", kAndroidStatusVarName));

  return unique_ptr<Document>{new CppSource{
      include_list,
      NestInNamespaces(std::move(on_transact), interface.GetSplitPackage())}};
}

unique_ptr<Document> BuildInterfaceSource(const TypeNamespace& /* types */,
                                          const AidlInterface& interface) {
  vector<string> include_list{
      HeaderFile(interface, ClassNames::INTERFACE, false),
      HeaderFile(interface, ClassNames::CLIENT, false),
  };

  string fq_name = ClassName(interface, ClassNames::INTERFACE);
  if (!interface.GetPackage().empty()) {
    fq_name = interface.GetPackage() + "." + fq_name;
  }

  unique_ptr<ConstructorDecl> meta_if{new ConstructorDecl{
      "IMPLEMENT_META_INTERFACE",
      ArgList{vector<string>{ClassName(interface, ClassNames::BASE),
                             '"' + fq_name + '"'}}}};

  return unique_ptr<Document>{new CppSource{
      include_list,
      NestInNamespaces(std::move(meta_if), interface.GetSplitPackage())}};
}

unique_ptr<Document> BuildClientHeader(const TypeNamespace& types,
                                       const AidlInterface& interface) {
  const string i_name = ClassName(interface, ClassNames::INTERFACE);
  const string bp_name = ClassName(interface, ClassNames::CLIENT);

  unique_ptr<ConstructorDecl> constructor{new ConstructorDecl{
      bp_name,
      ArgList{StringPrintf("const ::android::sp<::android::IBinder>& %s",
                           kImplVarName)},
      ConstructorDecl::IS_EXPLICIT
  }};
  unique_ptr<ConstructorDecl> destructor{new ConstructorDecl{
      "~" + bp_name,
      ArgList{},
      ConstructorDecl::IS_VIRTUAL | ConstructorDecl::IS_DEFAULT}};

  vector<unique_ptr<Declaration>> publics;
  publics.push_back(std::move(constructor));
  publics.push_back(std::move(destructor));

  for (const auto& method: interface.GetMethods()) {
    publics.push_back(BuildMethodDecl(*method, types, false));
  }

  unique_ptr<ClassDecl> bp_class{
      new ClassDecl{bp_name,
                    "::android::BpInterface<" + i_name + ">",
                    std::move(publics),
                    {}
      }};

  return unique_ptr<Document>{new CppHeader{
      BuildHeaderGuard(interface, ClassNames::CLIENT),
      {kIBinderHeader,
       kIInterfaceHeader,
       "utils/Errors.h",
       HeaderFile(interface, ClassNames::INTERFACE, false)},
      NestInNamespaces(std::move(bp_class), interface.GetSplitPackage())}};
}

unique_ptr<Document> BuildServerHeader(const TypeNamespace& /* types */,
                                       const AidlInterface& interface) {
  const string i_name = ClassName(interface, ClassNames::INTERFACE);
  const string bn_name = ClassName(interface, ClassNames::SERVER);

  unique_ptr<Declaration> on_transact{new MethodDecl{
      kAndroidStatusLiteral, "onTransact",
      ArgList{{StringPrintf("uint32_t %s", kCodeVarName),
               StringPrintf("const %s& %s", kAndroidParcelLiteral,
                            kDataVarName),
               StringPrintf("%s* %s", kAndroidParcelLiteral, kReplyVarName),
               StringPrintf("uint32_t %s = 0", kFlagsVarName)}},
      MethodDecl::IS_OVERRIDE
  }};

  std::vector<unique_ptr<Declaration>> publics;
  publics.push_back(std::move(on_transact));

  unique_ptr<ClassDecl> bn_class{
      new ClassDecl{bn_name,
                    "::android::BnInterface<" + i_name + ">",
                    std::move(publics),
                    {}
      }};

  return unique_ptr<Document>{new CppHeader{
      BuildHeaderGuard(interface, ClassNames::SERVER),
      {"binder/IInterface.h",
       HeaderFile(interface, ClassNames::INTERFACE, false)},
      NestInNamespaces(std::move(bn_class), interface.GetSplitPackage())}};
}

unique_ptr<Document> BuildInterfaceHeader(const TypeNamespace& types,
                                          const AidlInterface& interface) {
  set<string> includes = { kIBinderHeader, kIInterfaceHeader,
                           kStatusHeader, kStrongPointerHeader };

  for (const auto& method : interface.GetMethods()) {
    for (const auto& argument : method->GetArguments()) {
      const Type* type = argument->GetType().GetLanguageType<Type>();
      type->GetHeaders(&includes);
    }

    const Type* return_type = method->GetType().GetLanguageType<Type>();
    return_type->GetHeaders(&includes);
  }

  unique_ptr<ClassDecl> if_class{
      new ClassDecl{ClassName(interface, ClassNames::INTERFACE),
                    "::android::IInterface"}};
  if_class->AddPublic(unique_ptr<Declaration>{new ConstructorDecl{
      "DECLARE_META_INTERFACE",
      ArgList{vector<string>{ClassName(interface, ClassNames::BASE)}}}});

  unique_ptr<Enum> constant_enum{new Enum{"", "int32_t"}};
  for (const auto& constant : interface.GetConstants()) {
    constant_enum->AddValue(
        constant->GetName(), std::to_string(constant->GetValue()));
  }
  if (constant_enum->HasValues()) {
    if_class->AddPublic(std::move(constant_enum));
  }

  unique_ptr<Enum> call_enum{new Enum{"Call"}};
  for (const auto& method : interface.GetMethods()) {
    // Each method gets an enum entry and pure virtual declaration.
    if_class->AddPublic(BuildMethodDecl(*method, types, true));
    call_enum->AddValue(
        UpperCase(method->GetName()),
        StringPrintf("::android::IBinder::FIRST_CALL_TRANSACTION + %d",
                     method->GetId()));
  }
  if_class->AddPublic(std::move(call_enum));

  return unique_ptr<Document>{new CppHeader{
      BuildHeaderGuard(interface, ClassNames::INTERFACE),
      vector<string>(includes.begin(), includes.end()),
      NestInNamespaces(std::move(if_class), interface.GetSplitPackage())}};
}

bool WriteHeader(const CppOptions& options,
                 const TypeNamespace& types,
                 const AidlInterface& interface,
                 const IoDelegate& io_delegate,
                 ClassNames header_type) {
  unique_ptr<Document> header;
  switch (header_type) {
    case ClassNames::INTERFACE:
      header = BuildInterfaceHeader(types, interface);
      break;
    case ClassNames::CLIENT:
      header = BuildClientHeader(types, interface);
      break;
    case ClassNames::SERVER:
      header = BuildServerHeader(types, interface);
      break;
    default:
      LOG(FATAL) << "aidl internal error";
  }
  if (!header) {
    LOG(ERROR) << "aidl internal error: Failed to generate header.";
    return false;
  }

  const string header_path = options.OutputHeaderDir() + OS_PATH_SEPARATOR +
                             HeaderFile(interface, header_type);
  unique_ptr<CodeWriter> code_writer(io_delegate.GetCodeWriter(header_path));
  header->Write(code_writer.get());

  const bool success = code_writer->Close();
  if (!success) {
    io_delegate.RemovePath(header_path);
  }

  return success;
}

}  // namespace internals

using namespace internals;

string HeaderFile(const AidlInterface& interface,
                  ClassNames class_type,
                  bool use_os_sep) {
  string file_path = interface.GetPackage();
  for (char& c: file_path) {
    if (c == '.') {
      c = (use_os_sep) ? OS_PATH_SEPARATOR : '/';
    }
  }
  if (!file_path.empty()) {
    file_path += (use_os_sep) ? OS_PATH_SEPARATOR : '/';
  }
  file_path += ClassName(interface, class_type);
  file_path += ".h";

  return file_path;
}

bool GenerateCpp(const CppOptions& options,
                 const TypeNamespace& types,
                 const AidlInterface& interface,
                 const IoDelegate& io_delegate) {
  auto interface_src = BuildInterfaceSource(types, interface);
  auto client_src = BuildClientSource(types, interface);
  auto server_src = BuildServerSource(types, interface);

  if (!interface_src || !client_src || !server_src) {
    return false;
  }

  if (!io_delegate.CreatedNestedDirs(options.OutputHeaderDir(),
                                     interface.GetSplitPackage())) {
    LOG(ERROR) << "Failed to create directory structure for headers.";
    return false;
  }

  if (!WriteHeader(options, types, interface, io_delegate,
                   ClassNames::INTERFACE) ||
      !WriteHeader(options, types, interface, io_delegate,
                   ClassNames::CLIENT) ||
      !WriteHeader(options, types, interface, io_delegate,
                   ClassNames::SERVER)) {
    return false;
  }

  unique_ptr<CodeWriter> writer = io_delegate.GetCodeWriter(
      options.OutputCppFilePath());
  interface_src->Write(writer.get());
  client_src->Write(writer.get());
  server_src->Write(writer.get());

  const bool success = writer->Close();
  if (!success) {
    io_delegate.RemovePath(options.OutputCppFilePath());
  }

  return success;
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android
