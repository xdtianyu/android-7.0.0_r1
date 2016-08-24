/*
 * Copyright (C) 2016, The Android Open Source Project
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

#include "generate_java.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

#include <android-base/macros.h>

#include "type_java.h"

using std::string;

namespace android {
namespace aidl {
namespace java {

// =================================================
class StubClass : public Class {
 public:
  StubClass(const Type* type, const InterfaceType* interfaceType,
            JavaTypeNamespace* types);
  virtual ~StubClass() = default;

  Variable* transact_code;
  Variable* transact_data;
  Variable* transact_reply;
  Variable* transact_flags;
  SwitchStatement* transact_switch;

 private:
  void make_as_interface(const InterfaceType* interfaceType,
                         JavaTypeNamespace* types);

  DISALLOW_COPY_AND_ASSIGN(StubClass);
};

StubClass::StubClass(const Type* type, const InterfaceType* interfaceType,
                     JavaTypeNamespace* types)
    : Class() {
  this->comment = "/** Local-side IPC implementation stub class. */";
  this->modifiers = PUBLIC | ABSTRACT | STATIC;
  this->what = Class::CLASS;
  this->type = type;
  this->extends = types->BinderNativeType();
  this->interfaces.push_back(interfaceType);

  // descriptor
  Field* descriptor =
      new Field(STATIC | FINAL | PRIVATE,
                new Variable(types->StringType(), "DESCRIPTOR"));
  descriptor->value = "\"" + interfaceType->JavaType() + "\"";
  this->elements.push_back(descriptor);

  // ctor
  Method* ctor = new Method;
  ctor->modifiers = PUBLIC;
  ctor->comment =
      "/** Construct the stub at attach it to the "
      "interface. */";
  ctor->name = "Stub";
  ctor->statements = new StatementBlock;
  MethodCall* attach =
      new MethodCall(THIS_VALUE, "attachInterface", 2, THIS_VALUE,
                     new LiteralExpression("DESCRIPTOR"));
  ctor->statements->Add(attach);
  this->elements.push_back(ctor);

  // asInterface
  make_as_interface(interfaceType, types);

  // asBinder
  Method* asBinder = new Method;
  asBinder->modifiers = PUBLIC | OVERRIDE;
  asBinder->returnType = types->IBinderType();
  asBinder->name = "asBinder";
  asBinder->statements = new StatementBlock;
  asBinder->statements->Add(new ReturnStatement(THIS_VALUE));
  this->elements.push_back(asBinder);

  // onTransact
  this->transact_code = new Variable(types->IntType(), "code");
  this->transact_data = new Variable(types->ParcelType(), "data");
  this->transact_reply = new Variable(types->ParcelType(), "reply");
  this->transact_flags = new Variable(types->IntType(), "flags");
  Method* onTransact = new Method;
  onTransact->modifiers = PUBLIC | OVERRIDE;
  onTransact->returnType = types->BoolType();
  onTransact->name = "onTransact";
  onTransact->parameters.push_back(this->transact_code);
  onTransact->parameters.push_back(this->transact_data);
  onTransact->parameters.push_back(this->transact_reply);
  onTransact->parameters.push_back(this->transact_flags);
  onTransact->statements = new StatementBlock;
  onTransact->exceptions.push_back(types->RemoteExceptionType());
  this->elements.push_back(onTransact);
  this->transact_switch = new SwitchStatement(this->transact_code);

  onTransact->statements->Add(this->transact_switch);
  MethodCall* superCall = new MethodCall(
      SUPER_VALUE, "onTransact", 4, this->transact_code, this->transact_data,
      this->transact_reply, this->transact_flags);
  onTransact->statements->Add(new ReturnStatement(superCall));
}

void StubClass::make_as_interface(const InterfaceType* interfaceType,
                                  JavaTypeNamespace* types) {
  Variable* obj = new Variable(types->IBinderType(), "obj");

  Method* m = new Method;
  m->comment = "/**\n * Cast an IBinder object into an ";
  m->comment += interfaceType->JavaType();
  m->comment += " interface,\n";
  m->comment += " * generating a proxy if needed.\n */";
  m->modifiers = PUBLIC | STATIC;
  m->returnType = interfaceType;
  m->name = "asInterface";
  m->parameters.push_back(obj);
  m->statements = new StatementBlock;

  IfStatement* ifstatement = new IfStatement();
  ifstatement->expression = new Comparison(obj, "==", NULL_VALUE);
  ifstatement->statements = new StatementBlock;
  ifstatement->statements->Add(new ReturnStatement(NULL_VALUE));
  m->statements->Add(ifstatement);

  // IInterface iin = obj.queryLocalInterface(DESCRIPTOR)
  MethodCall* queryLocalInterface = new MethodCall(obj, "queryLocalInterface");
  queryLocalInterface->arguments.push_back(new LiteralExpression("DESCRIPTOR"));
  IInterfaceType* iinType = new IInterfaceType(types);
  Variable* iin = new Variable(iinType, "iin");
  VariableDeclaration* iinVd =
      new VariableDeclaration(iin, queryLocalInterface, NULL);
  m->statements->Add(iinVd);

  // Ensure the instance type of the local object is as expected.
  // One scenario where this is needed is if another package (with a
  // different class loader) runs in the same process as the service.

  // if (iin != null && iin instanceof <interfaceType>) return (<interfaceType>)
  // iin;
  Comparison* iinNotNull = new Comparison(iin, "!=", NULL_VALUE);
  Comparison* instOfCheck =
      new Comparison(iin, " instanceof ",
                     new LiteralExpression(interfaceType->JavaType()));
  IfStatement* instOfStatement = new IfStatement();
  instOfStatement->expression = new Comparison(iinNotNull, "&&", instOfCheck);
  instOfStatement->statements = new StatementBlock;
  instOfStatement->statements->Add(
      new ReturnStatement(new Cast(interfaceType, iin)));
  m->statements->Add(instOfStatement);

  NewExpression* ne = new NewExpression(interfaceType->GetProxy());
  ne->arguments.push_back(obj);
  m->statements->Add(new ReturnStatement(ne));

  this->elements.push_back(m);
}

// =================================================
class ProxyClass : public Class {
 public:
  ProxyClass(const JavaTypeNamespace* types, const Type* type,
             const InterfaceType* interfaceType);
  virtual ~ProxyClass();

  Variable* mRemote;
  bool mOneWay;
};

ProxyClass::ProxyClass(const JavaTypeNamespace* types, const Type* type,
                       const InterfaceType* interfaceType)
    : Class() {
  this->modifiers = PRIVATE | STATIC;
  this->what = Class::CLASS;
  this->type = type;
  this->interfaces.push_back(interfaceType);

  mOneWay = interfaceType->OneWay();

  // IBinder mRemote
  mRemote = new Variable(types->IBinderType(), "mRemote");
  this->elements.push_back(new Field(PRIVATE, mRemote));

  // Proxy()
  Variable* remote = new Variable(types->IBinderType(), "remote");
  Method* ctor = new Method;
  ctor->name = "Proxy";
  ctor->statements = new StatementBlock;
  ctor->parameters.push_back(remote);
  ctor->statements->Add(new Assignment(mRemote, remote));
  this->elements.push_back(ctor);

  // IBinder asBinder()
  Method* asBinder = new Method;
  asBinder->modifiers = PUBLIC | OVERRIDE;
  asBinder->returnType = types->IBinderType();
  asBinder->name = "asBinder";
  asBinder->statements = new StatementBlock;
  asBinder->statements->Add(new ReturnStatement(mRemote));
  this->elements.push_back(asBinder);
}

ProxyClass::~ProxyClass() {}

// =================================================
static void generate_new_array(const Type* t, StatementBlock* addTo,
                               Variable* v, Variable* parcel,
                               JavaTypeNamespace* types) {
  Variable* len = new Variable(types->IntType(), v->name + "_length");
  addTo->Add(new VariableDeclaration(len, new MethodCall(parcel, "readInt")));
  IfStatement* lencheck = new IfStatement();
  lencheck->expression = new Comparison(len, "<", new LiteralExpression("0"));
  lencheck->statements->Add(new Assignment(v, NULL_VALUE));
  lencheck->elseif = new IfStatement();
  lencheck->elseif->statements->Add(
      new Assignment(v, new NewArrayExpression(t, len)));
  addTo->Add(lencheck);
}

static void generate_write_to_parcel(const Type* t, StatementBlock* addTo,
                                     Variable* v, Variable* parcel, int flags) {
  t->WriteToParcel(addTo, v, parcel, flags);
}

static void generate_create_from_parcel(const Type* t, StatementBlock* addTo,
                                        Variable* v, Variable* parcel,
                                        Variable** cl) {
  t->CreateFromParcel(addTo, v, parcel, cl);
}

static void generate_read_from_parcel(const Type* t, StatementBlock* addTo,
                                      Variable* v, Variable* parcel,
                                      Variable** cl) {
  t->ReadFromParcel(addTo, v, parcel, cl);
}

static void generate_constant(const AidlConstant& constant, Class* interface) {
  Constant* decl = new Constant;
  decl->name = constant.GetName();
  decl->value = constant.GetValue();

  interface->elements.push_back(decl);
}

static void generate_method(const AidlMethod& method, Class* interface,
                            StubClass* stubClass, ProxyClass* proxyClass,
                            int index, JavaTypeNamespace* types) {
  int i;
  bool hasOutParams = false;

  const bool oneway = proxyClass->mOneWay || method.IsOneway();

  // == the TRANSACT_ constant =============================================
  string transactCodeName = "TRANSACTION_";
  transactCodeName += method.GetName();

  char transactCodeValue[60];
  sprintf(transactCodeValue, "(android.os.IBinder.FIRST_CALL_TRANSACTION + %d)",
          index);

  Field* transactCode = new Field(
      STATIC | FINAL, new Variable(types->IntType(), transactCodeName));
  transactCode->value = transactCodeValue;
  stubClass->elements.push_back(transactCode);

  // == the declaration in the interface ===================================
  Method* decl = new Method;
  decl->comment = method.GetComments();
  decl->modifiers = PUBLIC;
  decl->returnType = method.GetType().GetLanguageType<Type>();
  decl->returnTypeDimension = method.GetType().IsArray() ? 1 : 0;
  decl->name = method.GetName();

  for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
    decl->parameters.push_back(
        new Variable(arg->GetType().GetLanguageType<Type>(), arg->GetName(),
                     arg->GetType().IsArray() ? 1 : 0));
  }

  decl->exceptions.push_back(types->RemoteExceptionType());

  interface->elements.push_back(decl);

  // == the stub method ====================================================

  Case* c = new Case(transactCodeName);

  MethodCall* realCall = new MethodCall(THIS_VALUE, method.GetName());

  // interface token validation is the very first thing we do
  c->statements->Add(new MethodCall(stubClass->transact_data,
                                    "enforceInterface", 1,
                                    new LiteralExpression("DESCRIPTOR")));

  // args
  Variable* cl = NULL;
  VariableFactory stubArgs("_arg");
  for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
    const Type* t = arg->GetType().GetLanguageType<Type>();
    Variable* v = stubArgs.Get(t);
    v->dimension = arg->GetType().IsArray() ? 1 : 0;

    c->statements->Add(new VariableDeclaration(v));

    if (arg->GetDirection() & AidlArgument::IN_DIR) {
      generate_create_from_parcel(t, c->statements, v, stubClass->transact_data,
                                  &cl);
    } else {
      if (!arg->GetType().IsArray()) {
        c->statements->Add(new Assignment(v, new NewExpression(v->type)));
      } else {
        generate_new_array(v->type, c->statements, v, stubClass->transact_data,
                           types);
      }
    }

    realCall->arguments.push_back(v);
  }

  // the real call
  Variable* _result = NULL;
  if (method.GetType().GetName() == "void") {
    c->statements->Add(realCall);

    if (!oneway) {
      // report that there were no exceptions
      MethodCall* ex =
          new MethodCall(stubClass->transact_reply, "writeNoException", 0);
      c->statements->Add(ex);
    }
  } else {
    _result =
        new Variable(decl->returnType, "_result", decl->returnTypeDimension);
    c->statements->Add(new VariableDeclaration(_result, realCall));

    if (!oneway) {
      // report that there were no exceptions
      MethodCall* ex =
          new MethodCall(stubClass->transact_reply, "writeNoException", 0);
      c->statements->Add(ex);
    }

    // marshall the return value
    generate_write_to_parcel(decl->returnType, c->statements, _result,
                             stubClass->transact_reply,
                             Type::PARCELABLE_WRITE_RETURN_VALUE);
  }

  // out parameters
  i = 0;
  for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
    const Type* t = arg->GetType().GetLanguageType<Type>();
    Variable* v = stubArgs.Get(i++);

    if (arg->GetDirection() & AidlArgument::OUT_DIR) {
      generate_write_to_parcel(t, c->statements, v, stubClass->transact_reply,
                               Type::PARCELABLE_WRITE_RETURN_VALUE);
      hasOutParams = true;
    }
  }

  // return true
  c->statements->Add(new ReturnStatement(TRUE_VALUE));
  stubClass->transact_switch->cases.push_back(c);

  // == the proxy method ===================================================
  Method* proxy = new Method;
  proxy->comment = method.GetComments();
  proxy->modifiers = PUBLIC | OVERRIDE;
  proxy->returnType = method.GetType().GetLanguageType<Type>();
  proxy->returnTypeDimension = method.GetType().IsArray() ? 1 : 0;
  proxy->name = method.GetName();
  proxy->statements = new StatementBlock;
  for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
    proxy->parameters.push_back(
        new Variable(arg->GetType().GetLanguageType<Type>(), arg->GetName(),
                     arg->GetType().IsArray() ? 1 : 0));
  }
  proxy->exceptions.push_back(types->RemoteExceptionType());
  proxyClass->elements.push_back(proxy);

  // the parcels
  Variable* _data = new Variable(types->ParcelType(), "_data");
  proxy->statements->Add(new VariableDeclaration(
      _data, new MethodCall(types->ParcelType(), "obtain")));
  Variable* _reply = NULL;
  if (!oneway) {
    _reply = new Variable(types->ParcelType(), "_reply");
    proxy->statements->Add(new VariableDeclaration(
        _reply, new MethodCall(types->ParcelType(), "obtain")));
  }

  // the return value
  _result = NULL;
  if (method.GetType().GetName() != "void") {
    _result = new Variable(proxy->returnType, "_result",
                           method.GetType().IsArray() ? 1 : 0);
    proxy->statements->Add(new VariableDeclaration(_result));
  }

  // try and finally
  TryStatement* tryStatement = new TryStatement();
  proxy->statements->Add(tryStatement);
  FinallyStatement* finallyStatement = new FinallyStatement();
  proxy->statements->Add(finallyStatement);

  // the interface identifier token: the DESCRIPTOR constant, marshalled as a
  // string
  tryStatement->statements->Add(new MethodCall(
      _data, "writeInterfaceToken", 1, new LiteralExpression("DESCRIPTOR")));

  // the parameters
  for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
    const Type* t = arg->GetType().GetLanguageType<Type>();
    Variable* v =
        new Variable(t, arg->GetName(), arg->GetType().IsArray() ? 1 : 0);
    AidlArgument::Direction dir = arg->GetDirection();
    if (dir == AidlArgument::OUT_DIR && arg->GetType().IsArray()) {
      IfStatement* checklen = new IfStatement();
      checklen->expression = new Comparison(v, "==", NULL_VALUE);
      checklen->statements->Add(
          new MethodCall(_data, "writeInt", 1, new LiteralExpression("-1")));
      checklen->elseif = new IfStatement();
      checklen->elseif->statements->Add(
          new MethodCall(_data, "writeInt", 1, new FieldVariable(v, "length")));
      tryStatement->statements->Add(checklen);
    } else if (dir & AidlArgument::IN_DIR) {
      generate_write_to_parcel(t, tryStatement->statements, v, _data, 0);
    }
  }

  // the transact call
  MethodCall* call = new MethodCall(
      proxyClass->mRemote, "transact", 4,
      new LiteralExpression("Stub." + transactCodeName), _data,
      _reply ? _reply : NULL_VALUE,
      new LiteralExpression(oneway ? "android.os.IBinder.FLAG_ONEWAY" : "0"));
  tryStatement->statements->Add(call);

  // throw back exceptions.
  if (_reply) {
    MethodCall* ex = new MethodCall(_reply, "readException", 0);
    tryStatement->statements->Add(ex);
  }

  // returning and cleanup
  if (_reply != NULL) {
    if (_result != NULL) {
      generate_create_from_parcel(proxy->returnType, tryStatement->statements,
                                  _result, _reply, &cl);
    }

    // the out/inout parameters
    for (const std::unique_ptr<AidlArgument>& arg : method.GetArguments()) {
      const Type* t = arg->GetType().GetLanguageType<Type>();
      Variable* v =
          new Variable(t, arg->GetName(), arg->GetType().IsArray() ? 1 : 0);
      if (arg->GetDirection() & AidlArgument::OUT_DIR) {
        generate_read_from_parcel(t, tryStatement->statements, v, _reply, &cl);
      }
    }

    finallyStatement->statements->Add(new MethodCall(_reply, "recycle"));
  }
  finallyStatement->statements->Add(new MethodCall(_data, "recycle"));

  if (_result != NULL) {
    proxy->statements->Add(new ReturnStatement(_result));
  }
}

static void generate_interface_descriptors(StubClass* stub, ProxyClass* proxy,
                                           const JavaTypeNamespace* types) {
  // the interface descriptor transaction handler
  Case* c = new Case("INTERFACE_TRANSACTION");
  c->statements->Add(new MethodCall(stub->transact_reply, "writeString", 1,
                                    new LiteralExpression("DESCRIPTOR")));
  c->statements->Add(new ReturnStatement(TRUE_VALUE));
  stub->transact_switch->cases.push_back(c);

  // and the proxy-side method returning the descriptor directly
  Method* getDesc = new Method;
  getDesc->modifiers = PUBLIC;
  getDesc->returnType = types->StringType();
  getDesc->returnTypeDimension = 0;
  getDesc->name = "getInterfaceDescriptor";
  getDesc->statements = new StatementBlock;
  getDesc->statements->Add(
      new ReturnStatement(new LiteralExpression("DESCRIPTOR")));
  proxy->elements.push_back(getDesc);
}

Class* generate_binder_interface_class(const AidlInterface* iface,
                                       JavaTypeNamespace* types) {
  const InterfaceType* interfaceType = iface->GetLanguageType<InterfaceType>();

  // the interface class
  Class* interface = new Class;
  interface->comment = iface->GetComments();
  interface->modifiers = PUBLIC;
  interface->what = Class::INTERFACE;
  interface->type = interfaceType;
  interface->interfaces.push_back(types->IInterfaceType());

  // the stub inner class
  StubClass* stub =
      new StubClass(interfaceType->GetStub(), interfaceType, types);
  interface->elements.push_back(stub);

  // the proxy inner class
  ProxyClass* proxy =
      new ProxyClass(types, interfaceType->GetProxy(), interfaceType);
  stub->elements.push_back(proxy);

  // stub and proxy support for getInterfaceDescriptor()
  generate_interface_descriptors(stub, proxy, types);

  // all the declared constants of the interface
  for (const auto& item : iface->GetConstants()) {
    generate_constant(*item, interface);
  }

  // all the declared methods of the interface
  for (const auto& item : iface->GetMethods()) {
    generate_method(*item, interface, stub, proxy, item->GetId(), types);
  }

  return interface;
}

}  // namespace java
}  // namespace android
}  // namespace aidl
