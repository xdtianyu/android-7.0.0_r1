#include "aidl_language.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <android-base/strings.h>

#include "aidl_language_y.h"
#include "logging.h"

#ifdef _WIN32
int isatty(int  fd)
{
    return (fd == 0);
}
#endif

using android::aidl::IoDelegate;
using android::base::Join;
using android::base::Split;
using std::cerr;
using std::endl;
using std::string;
using std::unique_ptr;

void yylex_init(void **);
void yylex_destroy(void *);
void yyset_in(FILE *f, void *);
int yyparse(Parser*);
YY_BUFFER_STATE yy_scan_buffer(char *, size_t, void *);
void yy_delete_buffer(YY_BUFFER_STATE, void *);

AidlToken::AidlToken(const std::string& text, const std::string& comments)
    : text_(text),
      comments_(comments) {}

AidlType::AidlType(const std::string& name, unsigned line,
                   const std::string& comments, bool is_array)
    : name_(name),
      line_(line),
      is_array_(is_array),
      comments_(comments) {}

string AidlType::ToString() const {
  return name_ + (is_array_ ? "[]" : "");
}

AidlArgument::AidlArgument(AidlArgument::Direction direction, AidlType* type,
                           std::string name, unsigned line)
    : type_(type),
      direction_(direction),
      direction_specified_(true),
      name_(name),
      line_(line) {}

AidlArgument::AidlArgument(AidlType* type, std::string name, unsigned line)
    : type_(type),
      direction_(AidlArgument::IN_DIR),
      direction_specified_(false),
      name_(name),
      line_(line) {}

string AidlArgument::ToString() const {
  string ret;

  if (direction_specified_) {
    switch(direction_) {
    case AidlArgument::IN_DIR:
      ret += "in ";
      break;
    case AidlArgument::OUT_DIR:
      ret += "out ";
      break;
    case AidlArgument::INOUT_DIR:
      ret += "inout ";
      break;
    }
  }

  ret += type_->ToString();
  ret += " ";
  ret += name_;

  return ret;
}

AidlConstant::AidlConstant(std::string name, int32_t value)
    : name_(name),
      value_(value) {}

AidlMethod::AidlMethod(bool oneway, AidlType* type, std::string name,
                       std::vector<std::unique_ptr<AidlArgument>>* args,
                       unsigned line, const std::string& comments, int id)
    : oneway_(oneway),
      comments_(comments),
      type_(type),
      name_(name),
      line_(line),
      arguments_(std::move(*args)),
      id_(id) {
  has_id_ = true;
  delete args;
  for (const unique_ptr<AidlArgument>& a : arguments_) {
    if (a->IsIn()) { in_arguments_.push_back(a.get()); }
    if (a->IsOut()) { out_arguments_.push_back(a.get()); }
  }
}

AidlMethod::AidlMethod(bool oneway, AidlType* type, std::string name,
                       std::vector<std::unique_ptr<AidlArgument>>* args,
                       unsigned line, const std::string& comments)
    : AidlMethod(oneway, type, name, args, line, comments, 0) {
  has_id_ = false;
}

Parser::Parser(const IoDelegate& io_delegate)
    : io_delegate_(io_delegate) {
  yylex_init(&scanner_);
}

AidlParcelable::AidlParcelable(AidlQualifiedName* name, unsigned line,
                               const std::vector<std::string>& package,
                               const std::string& cpp_header)
    : name_(name),
      line_(line),
      package_(package),
      cpp_header_(cpp_header) {
  // Strip off quotation marks if we actually have a cpp header.
  if (cpp_header_.length() >= 2) {
    cpp_header_ = cpp_header_.substr(1, cpp_header_.length() - 2);
  }
}

std::string AidlParcelable::GetPackage() const {
  return Join(package_, '.');
}

std::string AidlParcelable::GetCanonicalName() const {
  if (package_.empty()) {
    return GetName();
  }
  return GetPackage() + "." + GetName();
}

AidlInterface::AidlInterface(const std::string& name, unsigned line,
                             const std::string& comments, bool oneway,
                             std::vector<std::unique_ptr<AidlMember>>* members,
                             const std::vector<std::string>& package)
    : name_(name),
      comments_(comments),
      line_(line),
      oneway_(oneway),
      package_(package) {
  for (auto& member : *members) {
    AidlMember* local = member.release();
    AidlMethod* method = local->AsMethod();
    AidlConstant* constant = local->AsConstant();

    if (method) {
      methods_.emplace_back(method);
    } else if (constant) {
      constants_.emplace_back(constant);
    } else {
      LOG(FATAL) << "Member is neither method nor constant!";
    }
  }

  delete members;
}

std::string AidlInterface::GetPackage() const {
  return Join(package_, '.');
}

std::string AidlInterface::GetCanonicalName() const {
  if (package_.empty()) {
    return GetName();
  }
  return GetPackage() + "." + GetName();
}

AidlDocument::AidlDocument(AidlInterface* interface)
    : interface_(interface) {}

AidlQualifiedName::AidlQualifiedName(std::string term,
                                     std::string comments)
    : terms_({term}),
      comments_(comments) {
  if (term.find('.') != string::npos) {
    terms_ = Split(term, ".");
    for (const auto& term: terms_) {
      if (term.empty()) {
        LOG(FATAL) << "Malformed qualified identifier: '" << term << "'";
      }
    }
  }
}

void AidlQualifiedName::AddTerm(std::string term) {
  terms_.push_back(term);
}

AidlImport::AidlImport(const std::string& from,
                       const std::string& needed_class, unsigned line)
    : from_(from),
      needed_class_(needed_class),
      line_(line) {}

Parser::~Parser() {
  if (raw_buffer_) {
    yy_delete_buffer(buffer_, scanner_);
    raw_buffer_.reset();
  }
  yylex_destroy(scanner_);
}

bool Parser::ParseFile(const string& filename) {
  // Make sure we can read the file first, before trashing previous state.
  unique_ptr<string> new_buffer = io_delegate_.GetFileContents(filename);
  if (!new_buffer) {
    LOG(ERROR) << "Error while opening file for parsing: '" << filename << "'";
    return false;
  }

  // Throw away old parsing state if we have any.
  if (raw_buffer_) {
    yy_delete_buffer(buffer_, scanner_);
    raw_buffer_.reset();
  }

  raw_buffer_ = std::move(new_buffer);
  // We're going to scan this buffer in place, and yacc demands we put two
  // nulls at the end.
  raw_buffer_->append(2u, '\0');
  filename_ = filename;
  package_.reset();
  error_ = 0;
  document_.reset();

  buffer_ = yy_scan_buffer(&(*raw_buffer_)[0], raw_buffer_->length(), scanner_);

  if (yy::parser(this).parse() != 0 || error_ != 0) {
    return false;}

  if (document_.get() != nullptr)
    return true;

  LOG(ERROR) << "Parser succeeded but yielded no document!";
  return false;
}

void Parser::ReportError(const string& err, unsigned line) {
  cerr << filename_ << ":" << line << ": " << err << endl;
  error_ = 1;
}

std::vector<std::string> Parser::Package() const {
  if (!package_) {
    return {};
  }
  return package_->GetTerms();
}

void Parser::AddImport(AidlQualifiedName* name, unsigned line) {
  imports_.emplace_back(new AidlImport(this->FileName(),
                                       name->GetDotName(), line));
  delete name;
}
