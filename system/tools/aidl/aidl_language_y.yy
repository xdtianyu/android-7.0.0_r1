%{
#include "aidl_language.h"
#include "aidl_language_y.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex(yy::parser::semantic_type *, yy::parser::location_type *, void *);

#define lex_scanner ps->Scanner()

%}

%parse-param { Parser* ps }
%lex-param { void *lex_scanner }

%pure-parser
%skeleton "glr.cc"

%union {
    AidlToken* token;
    int integer;
    std::string *str;
    AidlType::Annotation annotation;
    AidlType::Annotation annotation_list;
    AidlType* type;
    AidlType* unannotated_type;
    AidlArgument* arg;
    AidlArgument::Direction direction;
    std::vector<std::unique_ptr<AidlArgument>>* arg_list;
    AidlMethod* method;
    AidlConstant* constant;
    std::vector<std::unique_ptr<AidlMember>>* members;
    AidlQualifiedName* qname;
    AidlInterface* interface_obj;
    AidlParcelable* parcelable;
    AidlDocument* parcelable_list;
}

%token<token> IDENTIFIER INTERFACE ONEWAY C_STR
%token<integer> INTVALUE

%token '(' ')' ',' '=' '[' ']' '<' '>' '.' '{' '}' ';'
%token IN OUT INOUT PACKAGE IMPORT PARCELABLE CPP_HEADER CONST INT
%token ANNOTATION_NULLABLE ANNOTATION_UTF8 ANNOTATION_UTF8_CPP

%type<parcelable_list> parcelable_decls
%type<parcelable> parcelable_decl
%type<members> members
%type<interface_obj> interface_decl
%type<method> method_decl
%type<constant> constant_decl
%type<annotation> annotation
%type<annotation_list>annotation_list
%type<type> type
%type<unannotated_type> unannotated_type
%type<arg_list> arg_list
%type<arg> arg
%type<direction> direction
%type<str> generic_list
%type<qname> qualified_name

%type<token> identifier error
%%
document
 : package imports parcelable_decls
  { ps->SetDocument($3); }
 | package imports interface_decl
  { ps->SetDocument(new AidlDocument($3)); };

/* A couple of tokens that are keywords elsewhere are identifiers when
 * occurring in the identifier position. Therefore identifier is a
 * non-terminal, which is either an IDENTIFIER token, or one of the
 * aforementioned keyword tokens.
 */
identifier
 : IDENTIFIER
  { $$ = $1; }
 | CPP_HEADER
  { $$ = new AidlToken("cpp_header", ""); }
 | INT
  { $$ = new AidlToken("int", ""); };

package
 : {}
 | PACKAGE qualified_name ';'
  { ps->SetPackage($2); };

imports
 : {}
 | import imports {};

import
 : IMPORT qualified_name ';'
  { ps->AddImport($2, @1.begin.line); };

qualified_name
 : identifier {
    $$ = new AidlQualifiedName($1->GetText(), $1->GetComments());
    delete $1;
  }
 | qualified_name '.' identifier
  { $$ = $1;
    $$->AddTerm($3->GetText());
  };

parcelable_decls
 :
  { $$ = new AidlDocument(); }
 | parcelable_decls parcelable_decl {
   $$ = $1;
   $$->AddParcelable($2);
  }
 | parcelable_decls error {
    fprintf(stderr, "%s:%d: syntax error don't know what to do with \"%s\"\n",
            ps->FileName().c_str(),
            @2.begin.line, $2->GetText().c_str());
    $$ = $1;
  };

parcelable_decl
 : PARCELABLE qualified_name ';' {
    $$ = new AidlParcelable($2, @2.begin.line, ps->Package());
  }
 | PARCELABLE qualified_name CPP_HEADER C_STR ';' {
    $$ = new AidlParcelable($2, @2.begin.line, ps->Package(), $4->GetText());
  }
 | PARCELABLE ';' {
    fprintf(stderr, "%s:%d syntax error in parcelable declaration. Expected type name.\n",
            ps->FileName().c_str(), @1.begin.line);
    $$ = NULL;
  }
 | PARCELABLE error ';' {
    fprintf(stderr, "%s:%d syntax error in parcelable declaration. Expected type name, saw \"%s\".\n",
            ps->FileName().c_str(), @2.begin.line, $2->GetText().c_str());
    $$ = NULL;
  };

interface_decl
 : INTERFACE identifier '{' members '}' {
    $$ = new AidlInterface($2->GetText(), @2.begin.line, $1->GetComments(),
                           false, $4, ps->Package());
    delete $1;
    delete $2;
  }
 | ONEWAY INTERFACE identifier '{' members '}' {
    $$ = new AidlInterface($3->GetText(), @3.begin.line, $1->GetComments(),
                           true, $5, ps->Package());
    delete $1;
    delete $2;
    delete $3;
  }
 | INTERFACE error '{' members '}' {
    fprintf(stderr, "%s:%d: syntax error in interface declaration.  Expected type name, saw \"%s\"\n",
            ps->FileName().c_str(), @2.begin.line, $2->GetText().c_str());
    $$ = NULL;
    delete $1;
    delete $2;
  }
 | INTERFACE error '}' {
    fprintf(stderr, "%s:%d: syntax error in interface declaration.  Expected type name, saw \"%s\"\n",
            ps->FileName().c_str(), @2.begin.line, $2->GetText().c_str());
    $$ = NULL;
    delete $1;
    delete $2;
  };

members
 :
  { $$ = new std::vector<std::unique_ptr<AidlMember>>(); }
 | members method_decl
  { $1->push_back(std::unique_ptr<AidlMember>($2)); }
 | members constant_decl
  { $1->push_back(std::unique_ptr<AidlMember>($2)); }
 | members error ';' {
    fprintf(stderr, "%s:%d: syntax error before ';' "
                    "(expected method or constant declaration)\n",
            ps->FileName().c_str(), @3.begin.line);
    $$ = $1;
  };

constant_decl
 : CONST INT identifier '=' INTVALUE ';' {
    $$ = new AidlConstant($3->GetText(), $5);
 };

method_decl
 : type identifier '(' arg_list ')' ';' {
    $$ = new AidlMethod(false, $1, $2->GetText(), $4, @2.begin.line,
                        $1->GetComments());
    delete $2;
  }
 | ONEWAY type identifier '(' arg_list ')' ';' {
    $$ = new AidlMethod(true, $2, $3->GetText(), $5, @3.begin.line,
                        $1->GetComments());
    delete $1;
    delete $3;
  }
 | type identifier '(' arg_list ')' '=' INTVALUE ';' {
    $$ = new AidlMethod(false, $1, $2->GetText(), $4, @2.begin.line,
                        $1->GetComments(), $7);
    delete $2;
  }
 | ONEWAY type identifier '(' arg_list ')' '=' INTVALUE ';' {
    $$ = new AidlMethod(true, $2, $3->GetText(), $5, @3.begin.line,
                        $1->GetComments(), $8);
    delete $1;
    delete $3;
  };

arg_list
 :
  { $$ = new std::vector<std::unique_ptr<AidlArgument>>(); }
 | arg {
    $$ = new std::vector<std::unique_ptr<AidlArgument>>();
    $$->push_back(std::unique_ptr<AidlArgument>($1));
  }
 | arg_list ',' arg {
    $$ = $1;
    $$->push_back(std::unique_ptr<AidlArgument>($3));
  }
 | error {
    fprintf(stderr, "%s:%d: syntax error in parameter list\n",
            ps->FileName().c_str(), @1.begin.line);
    $$ = new std::vector<std::unique_ptr<AidlArgument>>();
  };

arg
 : direction type identifier {
    $$ = new AidlArgument($1, $2, $3->GetText(), @3.begin.line);
    delete $3;
  };
 | type identifier {
    $$ = new AidlArgument($1, $2->GetText(), @2.begin.line);
    delete $2;
  };

unannotated_type
 : qualified_name {
    $$ = new AidlType($1->GetDotName(), @1.begin.line, $1->GetComments(), false);
    delete $1;
  }
 | qualified_name '[' ']' {
    $$ = new AidlType($1->GetDotName(), @1.begin.line, $1->GetComments(),
                      true);
    delete $1;
  }
 | qualified_name '<' generic_list '>' {
    $$ = new AidlType($1->GetDotName() + "<" + *$3 + ">", @1.begin.line,
                      $1->GetComments(), false);
    delete $1;
    delete $3;
  };

type
 : annotation_list unannotated_type {
    $$ = $2;
    $2->Annotate($1);
  }
 | unannotated_type {
    $$ = $1;
  };

generic_list
 : qualified_name {
    $$ = new std::string($1->GetDotName());
    delete $1;
  }
 | generic_list ',' qualified_name {
    $$ = new std::string(*$1 + "," + $3->GetDotName());
    delete $1;
    delete $3;
  };

annotation_list
 : annotation_list annotation
  { $$ = static_cast<AidlType::Annotation>($1 | $2); }
 | annotation
  { $$ = $1; };

annotation
 : ANNOTATION_NULLABLE
  { $$ = AidlType::AnnotationNullable; }
 | ANNOTATION_UTF8
  { $$ = AidlType::AnnotationUtf8; }
 | ANNOTATION_UTF8_CPP
  { $$ = AidlType::AnnotationUtf8InCpp; };

direction
 : IN
  { $$ = AidlArgument::IN_DIR; }
 | OUT
  { $$ = AidlArgument::OUT_DIR; }
 | INOUT
  { $$ = AidlArgument::INOUT_DIR; };

%%

#include <ctype.h>
#include <stdio.h>

void yy::parser::error(const yy::parser::location_type& l,
                       const std::string& errstr) {
  ps->ReportError(errstr, l.begin.line);
}
