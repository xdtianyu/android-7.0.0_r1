#ifndef AIDL_AIDL_LANGUAGE_H_
#define AIDL_AIDL_LANGUAGE_H_

#include <memory>
#include <string>
#include <vector>

#include <android-base/macros.h>
#include <android-base/strings.h>

#include <io_delegate.h>

struct yy_buffer_state;
typedef yy_buffer_state* YY_BUFFER_STATE;

class AidlToken {
 public:
  AidlToken(const std::string& text, const std::string& comments);

  const std::string& GetText() const { return text_; }
  const std::string& GetComments() const { return comments_; }

 private:
  std::string text_;
  std::string comments_;

  DISALLOW_COPY_AND_ASSIGN(AidlToken);
};

class AidlNode {
 public:
  AidlNode() = default;
  virtual ~AidlNode() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(AidlNode);
};

namespace android {
namespace aidl {

class ValidatableType;

}  // namespace aidl
}  // namespace android

class AidlType : public AidlNode {
 public:
  enum Annotation : uint32_t {
    AnnotationNone = 0,
    AnnotationNullable = 1 << 0,
    AnnotationUtf8 = 1 << 1,
    AnnotationUtf8InCpp = 1 << 2,
  };

  AidlType(const std::string& name, unsigned line,
           const std::string& comments, bool is_array);
  virtual ~AidlType() = default;

  const std::string& GetName() const { return name_; }
  unsigned GetLine() const { return line_; }
  bool IsArray() const { return is_array_; }
  const std::string& GetComments() const { return comments_; }

  std::string ToString() const;

  void SetLanguageType(const android::aidl::ValidatableType* language_type) {
    language_type_ = language_type;
  }

  template<typename T>
  const T* GetLanguageType() const {
    return reinterpret_cast<const T*>(language_type_);
  }

  void Annotate(AidlType::Annotation annotation) { annotations_ = annotation; }
  bool IsNullable() const {
    return annotations_ & AnnotationNullable;
  }
  bool IsUtf8() const {
    return annotations_ & AnnotationUtf8;
  }
  bool IsUtf8InCpp() const {
    return annotations_ & AnnotationUtf8InCpp;
  }

 private:
  std::string name_;
  unsigned line_;
  bool is_array_;
  std::string comments_;
  const android::aidl::ValidatableType* language_type_ = nullptr;
  Annotation annotations_ = AnnotationNone;

  DISALLOW_COPY_AND_ASSIGN(AidlType);
};

class AidlArgument : public AidlNode {
 public:
  enum Direction { IN_DIR = 1, OUT_DIR = 2, INOUT_DIR = 3 };

  AidlArgument(AidlArgument::Direction direction, AidlType* type,
               std::string name, unsigned line);
  AidlArgument(AidlType* type, std::string name, unsigned line);
  virtual ~AidlArgument() = default;

  Direction GetDirection() const { return direction_; }
  bool IsOut() const { return direction_ & OUT_DIR; }
  bool IsIn() const { return direction_ & IN_DIR; }
  bool DirectionWasSpecified() const { return direction_specified_; }

  std::string GetName() const { return name_; }
  int GetLine() const { return line_; }
  const AidlType& GetType() const { return *type_; }
  AidlType* GetMutableType() { return type_.get(); }

  std::string ToString() const;

 private:
  std::unique_ptr<AidlType> type_;
  Direction direction_;
  bool direction_specified_;
  std::string name_;
  unsigned line_;

  DISALLOW_COPY_AND_ASSIGN(AidlArgument);
};

class AidlMethod;
class AidlConstant;
class AidlMember : public AidlNode {
 public:
  AidlMember() = default;
  virtual ~AidlMember() = default;

  virtual AidlMethod* AsMethod() { return nullptr; }
  virtual AidlConstant* AsConstant() { return nullptr; }

 private:
  DISALLOW_COPY_AND_ASSIGN(AidlMember);
};

class AidlConstant : public AidlMember {
 public:
  AidlConstant(std::string name, int32_t value);
  virtual ~AidlConstant() = default;

  const std::string& GetName() const { return name_; }
  int GetValue() const { return value_; }

  AidlConstant* AsConstant() override { return this; }

 private:
  std::string name_;
  int32_t value_;

  DISALLOW_COPY_AND_ASSIGN(AidlConstant);
};

class AidlMethod : public AidlMember {
 public:
  AidlMethod(bool oneway, AidlType* type, std::string name,
             std::vector<std::unique_ptr<AidlArgument>>* args,
             unsigned line, const std::string& comments);
  AidlMethod(bool oneway, AidlType* type, std::string name,
             std::vector<std::unique_ptr<AidlArgument>>* args,
             unsigned line, const std::string& comments, int id);
  virtual ~AidlMethod() = default;

  AidlMethod* AsMethod() override { return this; }

  const std::string& GetComments() const { return comments_; }
  const AidlType& GetType() const { return *type_; }
  AidlType* GetMutableType() { return type_.get(); }
  bool IsOneway() const { return oneway_; }
  const std::string& GetName() const { return name_; }
  unsigned GetLine() const { return line_; }
  bool HasId() const { return has_id_; }
  int GetId() { return id_; }
  void SetId(unsigned id) { id_ = id; }

  const std::vector<std::unique_ptr<AidlArgument>>& GetArguments() const {
    return arguments_;
  }
  // An inout parameter will appear in both GetInArguments()
  // and GetOutArguments().  AidlMethod retains ownership of the argument
  // pointers returned in this way.
  const std::vector<const AidlArgument*>& GetInArguments() const {
    return in_arguments_;
  }
  const std::vector<const AidlArgument*>& GetOutArguments() const {
    return out_arguments_;
  }

 private:
  bool oneway_;
  std::string comments_;
  std::unique_ptr<AidlType> type_;
  std::string name_;
  unsigned line_;
  const std::vector<std::unique_ptr<AidlArgument>> arguments_;
  std::vector<const AidlArgument*> in_arguments_;
  std::vector<const AidlArgument*> out_arguments_;
  bool has_id_;
  int id_;

  DISALLOW_COPY_AND_ASSIGN(AidlMethod);
};

class AidlParcelable;
class AidlInterface;
class AidlDocument : public AidlNode {
 public:
  AidlDocument() = default;
  AidlDocument(AidlInterface* interface);
  virtual ~AidlDocument() = default;

  const AidlInterface* GetInterface() const { return interface_.get(); }
  AidlInterface* ReleaseInterface() { return interface_.release(); }

  const std::vector<std::unique_ptr<AidlParcelable>>& GetParcelables() const {
    return parcelables_;
  }

  void AddParcelable(AidlParcelable* parcelable) {
    parcelables_.push_back(std::unique_ptr<AidlParcelable>(parcelable));
  }

 private:
  std::vector<std::unique_ptr<AidlParcelable>> parcelables_;
  std::unique_ptr<AidlInterface> interface_;

  DISALLOW_COPY_AND_ASSIGN(AidlDocument);
};

class AidlQualifiedName : public AidlNode {
 public:
  AidlQualifiedName(std::string term, std::string comments);
  virtual ~AidlQualifiedName() = default;

  const std::vector<std::string>& GetTerms() const { return terms_; }
  const std::string& GetComments() const { return comments_; }
  std::string GetDotName() const { return android::base::Join(terms_, '.'); }

  void AddTerm(std::string term);

 private:
  std::vector<std::string> terms_;
  std::string comments_;

  DISALLOW_COPY_AND_ASSIGN(AidlQualifiedName);
};

class AidlParcelable : public AidlNode {
 public:
  AidlParcelable(AidlQualifiedName* name, unsigned line,
                 const std::vector<std::string>& package,
                 const std::string& cpp_header = "");
  virtual ~AidlParcelable() = default;

  std::string GetName() const { return name_->GetDotName(); }
  unsigned GetLine() const { return line_; }
  std::string GetPackage() const;
  const std::vector<std::string>& GetSplitPackage() const { return package_; }
  std::string GetCppHeader() const { return cpp_header_; }
  std::string GetCanonicalName() const;

 private:
  std::unique_ptr<AidlQualifiedName> name_;
  unsigned line_;
  const std::vector<std::string> package_;
  std::string cpp_header_;

  DISALLOW_COPY_AND_ASSIGN(AidlParcelable);
};

class AidlInterface : public AidlNode {
 public:
  AidlInterface(const std::string& name, unsigned line,
                const std::string& comments, bool oneway_,
                std::vector<std::unique_ptr<AidlMember>>* members,
                const std::vector<std::string>& package);
  virtual ~AidlInterface() = default;

  const std::string& GetName() const { return name_; }
  unsigned GetLine() const { return line_; }
  const std::string& GetComments() const { return comments_; }
  bool IsOneway() const { return oneway_; }
  const std::vector<std::unique_ptr<AidlMethod>>& GetMethods() const
      { return methods_; }
  const std::vector<std::unique_ptr<AidlConstant>>& GetConstants() const
      { return constants_; }
  std::string GetPackage() const;
  std::string GetCanonicalName() const;
  const std::vector<std::string>& GetSplitPackage() const { return package_; }

  void SetLanguageType(const android::aidl::ValidatableType* language_type) {
    language_type_ = language_type;
  }

  template<typename T>
  const T* GetLanguageType() const {
    return reinterpret_cast<const T*>(language_type_);
  }

 private:
  std::string name_;
  std::string comments_;
  unsigned line_;
  bool oneway_;
  std::vector<std::unique_ptr<AidlMethod>> methods_;
  std::vector<std::unique_ptr<AidlConstant>> constants_;
  std::vector<std::string> package_;

  const android::aidl::ValidatableType* language_type_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AidlInterface);
};

class AidlImport : public AidlNode {
 public:
  AidlImport(const std::string& from, const std::string& needed_class,
             unsigned line);
  virtual ~AidlImport() = default;

  const std::string& GetFileFrom() const { return from_; }
  const std::string& GetFilename() const { return filename_; }
  const std::string& GetNeededClass() const { return needed_class_; }
  unsigned GetLine() const { return line_; }

  void SetFilename(const std::string& filename) { filename_ = filename; }

 private:
  std::string from_;
  std::string filename_;
  std::string needed_class_;
  unsigned line_;

  DISALLOW_COPY_AND_ASSIGN(AidlImport);
};

class Parser {
 public:
  explicit Parser(const android::aidl::IoDelegate& io_delegate);
  ~Parser();

  // Parse contents of file |filename|.
  bool ParseFile(const std::string& filename);

  void ReportError(const std::string& err, unsigned line);

  bool FoundNoErrors() const { return error_ == 0; }
  const std::string& FileName() const { return filename_; }
  void* Scanner() const { return scanner_; }

  void SetDocument(AidlDocument* doc) { document_.reset(doc); };

  void AddImport(AidlQualifiedName* name, unsigned line);

  std::vector<std::string> Package() const;
  void SetPackage(AidlQualifiedName* name) { package_.reset(name); }

  AidlDocument* GetDocument() const { return document_.get(); }
  AidlDocument* ReleaseDocument() { return document_.release(); }
  const std::vector<std::unique_ptr<AidlImport>>& GetImports() {
    return imports_;
  }

  void ReleaseImports(std::vector<std::unique_ptr<AidlImport>>* ret) {
      *ret = std::move(imports_);
      imports_.clear();
  }

 private:
  const android::aidl::IoDelegate& io_delegate_;
  int error_ = 0;
  std::string filename_;
  std::unique_ptr<AidlQualifiedName> package_;
  void* scanner_ = nullptr;
  std::unique_ptr<AidlDocument> document_;
  std::vector<std::unique_ptr<AidlImport>> imports_;
  std::unique_ptr<std::string> raw_buffer_;
  YY_BUFFER_STATE buffer_;

  DISALLOW_COPY_AND_ASSIGN(Parser);
};

#endif // AIDL_AIDL_LANGUAGE_H_
