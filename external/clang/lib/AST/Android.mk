LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES :=    \
	AttrDump.inc	\
	AttrImpl.inc	\
	AttrList.inc	\
	Attrs.inc	\
        AttrVisitor.inc \
	CommentCommandInfo.inc \
	CommentCommandList.inc \
	CommentHTMLNamedCharacterReferences.inc \
	CommentHTMLTags.inc \
	CommentHTMLTagsProperties.inc \
	CommentNodes.inc \
	DeclNodes.inc	\
	DiagnosticASTKinds.inc	\
	DiagnosticCommentKinds.inc \
	DiagnosticCommonKinds.inc	\
	DiagnosticFrontendKinds.inc \
	DiagnosticSemaKinds.inc	\
	StmtNodes.inc

clang_ast_SRC_FILES := \
  APValue.cpp \
  ASTConsumer.cpp \
  ASTContext.cpp \
  ASTDiagnostic.cpp \
  ASTDumper.cpp \
  ASTImporter.cpp \
  ASTTypeTraits.cpp \
  AttrImpl.cpp \
  CommentBriefParser.cpp \
  CommentCommandTraits.cpp \
  Comment.cpp \
  CommentLexer.cpp \
  CommentParser.cpp \
  CommentSema.cpp \
  CXXInheritance.cpp \
  DeclarationName.cpp \
  DeclBase.cpp \
  Decl.cpp \
  DeclCXX.cpp \
  DeclFriend.cpp \
  DeclGroup.cpp \
  DeclObjC.cpp \
  DeclOpenMP.cpp \
  DeclPrinter.cpp \
  DeclTemplate.cpp \
  ExprClassification.cpp \
  ExprConstant.cpp \
  Expr.cpp \
  ExprCXX.cpp \
  ExprObjC.cpp \
  ExternalASTSource.cpp \
  InheritViz.cpp \
  ItaniumCXXABI.cpp \
  ItaniumMangle.cpp \
  Mangle.cpp \
  MicrosoftCXXABI.cpp \
  MicrosoftMangle.cpp \
  NestedNameSpecifier.cpp \
  NSAPI.cpp \
  OpenMPClause.cpp \
  ParentMap.cpp \
  RawCommentList.cpp \
  RecordLayoutBuilder.cpp \
  RecordLayout.cpp \
  SelectorLocationsKind.cpp \
  Stmt.cpp \
  StmtCXX.cpp \
  StmtIterator.cpp \
  StmtObjC.cpp \
  StmtOpenMP.cpp \
  StmtPrinter.cpp \
  StmtProfile.cpp \
  StmtViz.cpp \
  TemplateBase.cpp \
  TemplateName.cpp \
  Type.cpp \
  TypeLoc.cpp \
  TypePrinter.cpp \
  VTableBuilder.cpp \
  VTTBuilder.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_ast_SRC_FILES)
LOCAL_MODULE:= libclangAST
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_ast_SRC_FILES)
LOCAL_MODULE:= libclangAST
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_STATIC_LIBRARY)
