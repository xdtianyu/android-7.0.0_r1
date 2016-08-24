LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES :=  \
	AttrList.inc  \
	AttrParsedAttrList.inc  \
	Attrs.inc  \
        AttrVisitor.inc \
        AttrParserStringSwitches.inc \
	CommentCommandList.inc \
	CommentNodes.inc \
	DeclNodes.inc  \
	DiagnosticParseKinds.inc  \
        DiagnosticCommonKinds.inc  \
	DiagnosticSemaKinds.inc	\
	StmtNodes.inc

clang_parse_SRC_FILES :=  \
	ParseAST.cpp  \
	ParseCXXInlineMethods.cpp  \
	ParseDecl.cpp  \
	ParseDeclCXX.cpp  \
	ParseExpr.cpp  \
	ParseExprCXX.cpp  \
	ParseInit.cpp  \
	ParseObjc.cpp  \
	ParseOpenMP.cpp  \
	ParsePragma.cpp  \
	ParseStmt.cpp  \
	ParseStmtAsm.cpp  \
	ParseTemplate.cpp  \
	ParseTentative.cpp  \
	Parser.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_parse_SRC_FILES)
LOCAL_MODULE:= libclangParse
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_parse_SRC_FILES)
LOCAL_MODULE:= libclangParse
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_STATIC_LIBRARY)
