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

#include "ast_java.h"

#include "code_writer.h"
#include "type_java.h"

using std::vector;
using std::string;

namespace android {
namespace aidl {
namespace java {

void WriteModifiers(CodeWriter* to, int mod, int mask) {
  int m = mod & mask;

  if (m & OVERRIDE) {
    to->Write("@Override ");
  }

  if ((m & SCOPE_MASK) == PUBLIC) {
    to->Write("public ");
  } else if ((m & SCOPE_MASK) == PRIVATE) {
    to->Write("private ");
  } else if ((m & SCOPE_MASK) == PROTECTED) {
    to->Write("protected ");
  }

  if (m & STATIC) {
    to->Write("static ");
  }

  if (m & FINAL) {
    to->Write("final ");
  }

  if (m & ABSTRACT) {
    to->Write("abstract ");
  }
}

void WriteArgumentList(CodeWriter* to, const vector<Expression*>& arguments) {
  size_t N = arguments.size();
  for (size_t i = 0; i < N; i++) {
    arguments[i]->Write(to);
    if (i != N - 1) {
      to->Write(", ");
    }
  }
}

Field::Field(int m, Variable* v) : ClassElement(), modifiers(m), variable(v) {}

void Field::Write(CodeWriter* to) const {
  if (this->comment.length() != 0) {
    to->Write("%s\n", this->comment.c_str());
  }
  WriteModifiers(to, this->modifiers, SCOPE_MASK | STATIC | FINAL | OVERRIDE);
  to->Write("%s %s", this->variable->type->JavaType().c_str(),
            this->variable->name.c_str());
  if (this->value.length() != 0) {
    to->Write(" = %s", this->value.c_str());
  }
  to->Write(";\n");
}

LiteralExpression::LiteralExpression(const string& v) : value(v) {}

void LiteralExpression::Write(CodeWriter* to) const {
  to->Write("%s", this->value.c_str());
}

StringLiteralExpression::StringLiteralExpression(const string& v) : value(v) {}

void StringLiteralExpression::Write(CodeWriter* to) const {
  to->Write("\"%s\"", this->value.c_str());
}

Variable::Variable(const Type* t, const string& n)
    : type(t), name(n), dimension(0) {}

Variable::Variable(const Type* t, const string& n, int d)
    : type(t), name(n), dimension(d) {}

void Variable::WriteDeclaration(CodeWriter* to) const {
  string dim;
  for (int i = 0; i < this->dimension; i++) {
    dim += "[]";
  }
  to->Write("%s%s %s", this->type->JavaType().c_str(), dim.c_str(),
            this->name.c_str());
}

void Variable::Write(CodeWriter* to) const { to->Write("%s", name.c_str()); }

FieldVariable::FieldVariable(Expression* o, const string& n)
    : object(o), clazz(NULL), name(n) {}

FieldVariable::FieldVariable(const Type* c, const string& n)
    : object(NULL), clazz(c), name(n) {}

void FieldVariable::Write(CodeWriter* to) const {
  if (this->object != NULL) {
    this->object->Write(to);
  } else if (this->clazz != NULL) {
    to->Write("%s", this->clazz->JavaType().c_str());
  }
  to->Write(".%s", name.c_str());
}

void StatementBlock::Write(CodeWriter* to) const {
  to->Write("{\n");
  int N = this->statements.size();
  for (int i = 0; i < N; i++) {
    this->statements[i]->Write(to);
  }
  to->Write("}\n");
}

void StatementBlock::Add(Statement* statement) {
  this->statements.push_back(statement);
}

void StatementBlock::Add(Expression* expression) {
  this->statements.push_back(new ExpressionStatement(expression));
}

ExpressionStatement::ExpressionStatement(Expression* e) : expression(e) {}

void ExpressionStatement::Write(CodeWriter* to) const {
  this->expression->Write(to);
  to->Write(";\n");
}

Assignment::Assignment(Variable* l, Expression* r)
    : lvalue(l), rvalue(r), cast(NULL) {}

Assignment::Assignment(Variable* l, Expression* r, const Type* c)
    : lvalue(l), rvalue(r), cast(c) {}

void Assignment::Write(CodeWriter* to) const {
  this->lvalue->Write(to);
  to->Write(" = ");
  if (this->cast != NULL) {
    to->Write("(%s)", this->cast->JavaType().c_str());
  }
  this->rvalue->Write(to);
}

MethodCall::MethodCall(const string& n) : name(n) {}

MethodCall::MethodCall(const string& n, int argc = 0, ...) : name(n) {
  va_list args;
  va_start(args, argc);
  init(argc, args);
  va_end(args);
}

MethodCall::MethodCall(Expression* o, const string& n) : obj(o), name(n) {}

MethodCall::MethodCall(const Type* t, const string& n) : clazz(t), name(n) {}

MethodCall::MethodCall(Expression* o, const string& n, int argc = 0, ...)
    : obj(o), name(n) {
  va_list args;
  va_start(args, argc);
  init(argc, args);
  va_end(args);
}

MethodCall::MethodCall(const Type* t, const string& n, int argc = 0, ...)
    : clazz(t), name(n) {
  va_list args;
  va_start(args, argc);
  init(argc, args);
  va_end(args);
}

void MethodCall::init(int n, va_list args) {
  for (int i = 0; i < n; i++) {
    Expression* expression = (Expression*)va_arg(args, void*);
    this->arguments.push_back(expression);
  }
}

void MethodCall::Write(CodeWriter* to) const {
  if (this->obj != NULL) {
    this->obj->Write(to);
    to->Write(".");
  } else if (this->clazz != NULL) {
    to->Write("%s.", this->clazz->JavaType().c_str());
  }
  to->Write("%s(", this->name.c_str());
  WriteArgumentList(to, this->arguments);
  to->Write(")");
}

Comparison::Comparison(Expression* l, const string& o, Expression* r)
    : lvalue(l), op(o), rvalue(r) {}

void Comparison::Write(CodeWriter* to) const {
  to->Write("(");
  this->lvalue->Write(to);
  to->Write("%s", this->op.c_str());
  this->rvalue->Write(to);
  to->Write(")");
}

NewExpression::NewExpression(const Type* t) : type(t) {}

NewExpression::NewExpression(const Type* t, int argc = 0, ...) : type(t) {
  va_list args;
  va_start(args, argc);
  init(argc, args);
  va_end(args);
}

void NewExpression::init(int n, va_list args) {
  for (int i = 0; i < n; i++) {
    Expression* expression = (Expression*)va_arg(args, void*);
    this->arguments.push_back(expression);
  }
}

void NewExpression::Write(CodeWriter* to) const {
  to->Write("new %s(", this->type->InstantiableName().c_str());
  WriteArgumentList(to, this->arguments);
  to->Write(")");
}

NewArrayExpression::NewArrayExpression(const Type* t, Expression* s)
    : type(t), size(s) {}

void NewArrayExpression::Write(CodeWriter* to) const {
  to->Write("new %s[", this->type->JavaType().c_str());
  size->Write(to);
  to->Write("]");
}

Ternary::Ternary(Expression* a, Expression* b, Expression* c)
    : condition(a), ifpart(b), elsepart(c) {}

void Ternary::Write(CodeWriter* to) const {
  to->Write("((");
  this->condition->Write(to);
  to->Write(")?(");
  this->ifpart->Write(to);
  to->Write("):(");
  this->elsepart->Write(to);
  to->Write("))");
}

Cast::Cast(const Type* t, Expression* e) : type(t), expression(e) {}

void Cast::Write(CodeWriter* to) const {
  to->Write("((%s)", this->type->JavaType().c_str());
  expression->Write(to);
  to->Write(")");
}

VariableDeclaration::VariableDeclaration(Variable* l, Expression* r,
                                         const Type* c)
    : lvalue(l), cast(c), rvalue(r) {}

VariableDeclaration::VariableDeclaration(Variable* l) : lvalue(l) {}

void VariableDeclaration::Write(CodeWriter* to) const {
  this->lvalue->WriteDeclaration(to);
  if (this->rvalue != NULL) {
    to->Write(" = ");
    if (this->cast != NULL) {
      to->Write("(%s)", this->cast->JavaType().c_str());
    }
    this->rvalue->Write(to);
  }
  to->Write(";\n");
}

void IfStatement::Write(CodeWriter* to) const {
  if (this->expression != NULL) {
    to->Write("if (");
    this->expression->Write(to);
    to->Write(") ");
  }
  this->statements->Write(to);
  if (this->elseif != NULL) {
    to->Write("else ");
    this->elseif->Write(to);
  }
}

ReturnStatement::ReturnStatement(Expression* e) : expression(e) {}

void ReturnStatement::Write(CodeWriter* to) const {
  to->Write("return ");
  this->expression->Write(to);
  to->Write(";\n");
}

void TryStatement::Write(CodeWriter* to) const {
  to->Write("try ");
  this->statements->Write(to);
}

CatchStatement::CatchStatement(Variable* e)
    : statements(new StatementBlock), exception(e) {}

void CatchStatement::Write(CodeWriter* to) const {
  to->Write("catch ");
  if (this->exception != NULL) {
    to->Write("(");
    this->exception->WriteDeclaration(to);
    to->Write(") ");
  }
  this->statements->Write(to);
}

void FinallyStatement::Write(CodeWriter* to) const {
  to->Write("finally ");
  this->statements->Write(to);
}

Case::Case(const string& c) { cases.push_back(c); }

void Case::Write(CodeWriter* to) const {
  int N = this->cases.size();
  if (N > 0) {
    for (int i = 0; i < N; i++) {
      string s = this->cases[i];
      if (s.length() != 0) {
        to->Write("case %s:\n", s.c_str());
      } else {
        to->Write("default:\n");
      }
    }
  } else {
    to->Write("default:\n");
  }
  statements->Write(to);
}

SwitchStatement::SwitchStatement(Expression* e) : expression(e) {}

void SwitchStatement::Write(CodeWriter* to) const {
  to->Write("switch (");
  this->expression->Write(to);
  to->Write(")\n{\n");
  int N = this->cases.size();
  for (int i = 0; i < N; i++) {
    this->cases[i]->Write(to);
  }
  to->Write("}\n");
}

void Break::Write(CodeWriter* to) const { to->Write("break;\n"); }

void Method::Write(CodeWriter* to) const {
  size_t N, i;

  if (this->comment.length() != 0) {
    to->Write("%s\n", this->comment.c_str());
  }

  WriteModifiers(to, this->modifiers,
                 SCOPE_MASK | STATIC | ABSTRACT | FINAL | OVERRIDE);

  if (this->returnType != NULL) {
    string dim;
    for (i = 0; i < this->returnTypeDimension; i++) {
      dim += "[]";
    }
    to->Write("%s%s ", this->returnType->JavaType().c_str(), dim.c_str());
  }

  to->Write("%s(", this->name.c_str());

  N = this->parameters.size();
  for (i = 0; i < N; i++) {
    this->parameters[i]->WriteDeclaration(to);
    if (i != N - 1) {
      to->Write(", ");
    }
  }

  to->Write(")");

  N = this->exceptions.size();
  for (i = 0; i < N; i++) {
    if (i == 0) {
      to->Write(" throws ");
    } else {
      to->Write(", ");
    }
    to->Write("%s", this->exceptions[i]->JavaType().c_str());
  }

  if (this->statements == NULL) {
    to->Write(";\n");
  } else {
    to->Write("\n");
    this->statements->Write(to);
  }
}

void Constant::Write(CodeWriter* to) const {
  WriteModifiers(to, STATIC | FINAL | PUBLIC, ALL_MODIFIERS);
  to->Write("int %s = %d;\n", name.c_str(), value);
}

void Class::Write(CodeWriter* to) const {
  size_t N, i;

  if (this->comment.length() != 0) {
    to->Write("%s\n", this->comment.c_str());
  }

  WriteModifiers(to, this->modifiers, ALL_MODIFIERS);

  if (this->what == Class::CLASS) {
    to->Write("class ");
  } else {
    to->Write("interface ");
  }

  string name = this->type->JavaType();
  size_t pos = name.rfind('.');
  if (pos != string::npos) {
    name = name.c_str() + pos + 1;
  }

  to->Write("%s", name.c_str());

  if (this->extends != NULL) {
    to->Write(" extends %s", this->extends->JavaType().c_str());
  }

  N = this->interfaces.size();
  if (N != 0) {
    if (this->what == Class::CLASS) {
      to->Write(" implements");
    } else {
      to->Write(" extends");
    }
    for (i = 0; i < N; i++) {
      to->Write(" %s", this->interfaces[i]->JavaType().c_str());
    }
  }

  to->Write("\n");
  to->Write("{\n");

  N = this->elements.size();
  for (i = 0; i < N; i++) {
    this->elements[i]->Write(to);
  }

  to->Write("}\n");
}

static string escape_backslashes(const string& str) {
  string result;
  const size_t I = str.length();
  for (size_t i = 0; i < I; i++) {
    char c = str[i];
    if (c == '\\') {
      result += "\\\\";
    } else {
      result += c;
    }
  }
  return result;
}

Document::Document(const std::string& comment,
                   const std::string& package,
                   const std::string& original_src,
                   std::unique_ptr<Class> clazz)
    : comment_(comment),
      package_(package),
      original_src_(original_src),
      clazz_(std::move(clazz)) {
}

void Document::Write(CodeWriter* to) const {
  if (!comment_.empty()) {
    to->Write("%s\n", comment_.c_str());
  }
  to->Write(
      "/*\n"
      " * This file is auto-generated.  DO NOT MODIFY.\n"
      " * Original file: %s\n"
      " */\n",
      escape_backslashes(original_src_).c_str());
  if (!package_.empty()) {
    to->Write("package %s;\n", package_.c_str());
  }

  if (clazz_) {
    clazz_->Write(to);
  }
}

}  // namespace java
}  // namespace aidl
}  // namespace android
