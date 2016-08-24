###################################4########################
## TableGen: Compile .td files to .inc.
###########################################################
ifeq ($(LOCAL_MODULE_CLASS),)
    LOCAL_MODULE_CLASS := STATIC_LIBRARIES
endif

ifneq ($(strip $(TBLGEN_TABLES)),)

define transform-clang-td-to-out
$(if $(LOCAL_IS_HOST_MODULE),	\
	$(call transform-host-clang-td-to-out,$(1)),	\
	$(call transform-device-clang-td-to-out,$(1)))
endef

define transform-td-to-out
$(if $(LOCAL_IS_HOST_MODULE),	\
	$(call transform-host-td-to-out,$(1)),	\
	$(call transform-device-td-to-out,$(1)))
endef

generated_sources := $(call local-generated-sources-dir)

ifneq ($(findstring AttrDump.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/AttrDump.inc
$(generated_sources)/include/clang/AST/AttrDump.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/AttrDump.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-dump)
endif

ifneq ($(findstring AttrImpl.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/AttrImpl.inc
$(generated_sources)/include/clang/AST/AttrImpl.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/AttrImpl.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-impl)
endif

ifneq ($(findstring AttrHasAttributeImpl.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Basic/AttrHasAttributeImpl.inc
$(generated_sources)/include/clang/Basic/AttrHasAttributeImpl.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Basic/AttrHasAttributeImpl.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-has-attribute-impl)
endif

ifneq ($(findstring AttrList.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Basic/AttrList.inc
$(generated_sources)/include/clang/Basic/AttrList.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Basic/AttrList.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-list)
endif

ifneq ($(findstring AttrSpellingListIndex.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Sema/AttrSpellingListIndex.inc
$(generated_sources)/include/clang/Sema/AttrSpellingListIndex.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Sema/AttrSpellingListIndex.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-spelling-index)
endif

ifneq ($(findstring AttrPCHRead.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Serialization/AttrPCHRead.inc
$(generated_sources)/include/clang/Serialization/AttrPCHRead.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Serialization/AttrPCHRead.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-pch-read)
endif

ifneq ($(findstring AttrPCHWrite.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Serialization/AttrPCHWrite.inc
$(generated_sources)/include/clang/Serialization/AttrPCHWrite.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Serialization/AttrPCHWrite.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-pch-write)
endif

ifneq ($(findstring Attrs.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/Attrs.inc
$(generated_sources)/include/clang/AST/Attrs.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/Attrs.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-classes)
endif

ifneq ($(findstring AttrParserStringSwitches.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Parse/AttrParserStringSwitches.inc
$(generated_sources)/include/clang/Parse/AttrParserStringSwitches.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Parse/AttrParserStringSwitches.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-parser-string-switches)
endif

ifneq ($(findstring AttrVisitor.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/AttrVisitor.inc
$(generated_sources)/include/clang/AST/AttrVisitor.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/AttrVisitor.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-ast-visitor)
endif

ifneq ($(findstring AttrParsedAttrKinds.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Sema/AttrParsedAttrKinds.inc
$(generated_sources)/include/clang/Sema/AttrParsedAttrKinds.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Sema/AttrParsedAttrKinds.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-parsed-attr-kinds)
endif

ifneq ($(findstring AttrParsedAttrImpl.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Sema/AttrParsedAttrImpl.inc
$(generated_sources)/include/clang/Sema/AttrParsedAttrImpl.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Sema/AttrParsedAttrImpl.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-parsed-attr-impl)
endif

ifneq ($(findstring AttrParsedAttrList.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Sema/AttrParsedAttrList.inc
$(generated_sources)/include/clang/Sema/AttrParsedAttrList.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Sema/AttrParsedAttrList.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-parsed-attr-list)
endif

ifneq ($(findstring AttrTemplateInstantiate.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Sema/AttrTemplateInstantiate.inc
$(generated_sources)/include/clang/Sema/AttrTemplateInstantiate.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Sema/AttrTemplateInstantiate.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Attr.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-attr-template-instantiate)
endif

ifneq ($(findstring Checkers.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/Checkers.inc
$(generated_sources)/Checkers.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/Checkers.inc: \
  $(CLANG_ROOT_PATH)/lib/StaticAnalyzer/Checkers/Checkers.td \
  $(CLANG_ROOT_PATH)/include/clang/StaticAnalyzer/Checkers/CheckerBase.td \
  $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-sa-checkers)
endif

ifneq ($(findstring CommentCommandInfo.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/CommentCommandInfo.inc
$(generated_sources)/include/clang/AST/CommentCommandInfo.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/CommentCommandInfo.inc: \
  $(CLANG_ROOT_PATH)/include/clang/AST/CommentCommands.td \
  $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-comment-command-info)
endif

ifneq ($(findstring CommentCommandList.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/CommentCommandList.inc
$(generated_sources)/include/clang/AST/CommentCommandList.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/CommentCommandList.inc: \
  $(CLANG_ROOT_PATH)/include/clang/AST/CommentCommands.td \
  $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-comment-command-list)
endif

ifneq ($(findstring CommentHTMLNamedCharacterReferences.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/CommentHTMLNamedCharacterReferences.inc
$(generated_sources)/include/clang/AST/CommentHTMLNamedCharacterReferences.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/CommentHTMLNamedCharacterReferences.inc: \
  $(CLANG_ROOT_PATH)/include/clang/AST/CommentHTMLNamedCharacterReferences.td \
  $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-comment-html-named-character-references)
endif

ifneq ($(findstring CommentHTMLTagsProperties.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/CommentHTMLTagsProperties.inc
$(generated_sources)/include/clang/AST/CommentHTMLTagsProperties.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/CommentHTMLTagsProperties.inc: \
  $(CLANG_ROOT_PATH)/include/clang/AST/CommentHTMLTags.td \
  $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-comment-html-tags-properties)
endif

ifneq ($(findstring CommentHTMLTags.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/CommentHTMLTags.inc
$(generated_sources)/include/clang/AST/CommentHTMLTags.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/CommentHTMLTags.inc: \
  $(CLANG_ROOT_PATH)/include/clang/AST/CommentHTMLTags.td \
  $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-comment-html-tags)
endif

ifneq ($(findstring CommentNodes.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/CommentNodes.inc
$(generated_sources)/include/clang/AST/CommentNodes.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/CommentNodes.inc: \
  $(CLANG_ROOT_PATH)/include/clang/Basic/CommentNodes.td \
  $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-comment-nodes)
endif

ifneq ($(filter Diagnostic%Kinds.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(addprefix $(generated_sources)/include/clang/Basic/,$(filter Diagnostic%Kinds.inc,$(TBLGEN_TABLES)))
$(generated_sources)/include/clang/Basic/Diagnostic%Kinds.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Basic/Diagnostic%Kinds.inc: \
  $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td \
  $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic%Kinds.td \
  $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-diags-defs -clang-component=$(patsubst Diagnostic%Kinds.inc,%,$(@F)))
endif

ifneq ($(findstring DiagnosticGroups.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Basic/DiagnosticGroups.inc
$(generated_sources)/include/clang/Basic/DiagnosticGroups.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Basic/DiagnosticGroups.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td $(CLANG_ROOT_PATH)/include/clang/Basic/DiagnosticGroups.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-diag-groups)
endif

ifneq ($(findstring DiagnosticIndexName.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Basic/DiagnosticIndexName.inc
$(generated_sources)/include/clang/Basic/DiagnosticIndexName.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Basic/DiagnosticIndexName.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/Diagnostic.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-diag-groups)
endif

ifneq ($(findstring DeclNodes.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/DeclNodes.inc
$(generated_sources)/include/clang/AST/DeclNodes.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/DeclNodes.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/DeclNodes.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-decl-nodes)
endif

ifneq ($(findstring StmtNodes.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/AST/StmtNodes.inc
$(generated_sources)/include/clang/AST/StmtNodes.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/AST/StmtNodes.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/StmtNodes.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,clang-stmt-nodes)
endif

ifneq ($(findstring arm_neon.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Basic/arm_neon.inc
$(generated_sources)/include/clang/Basic/arm_neon.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Basic/arm_neon.inc: $(CLANG_ROOT_PATH)/include/clang/Basic/arm_neon.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,arm-neon-sema)
endif

ifneq ($(findstring Options.inc,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Driver/Options.inc
$(generated_sources)/include/clang/Driver/Options.inc: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Driver/Options.inc: $(CLANG_ROOT_PATH)/include/clang/Driver/Options.td $(LLVM_ROOT_PATH)/include/llvm/Option/OptParser.td $(CLANG_ROOT_PATH)/include/clang/Driver/CC1Options.td \
    $(CLANG_TBLGEN) $(LLVM_TBLGEN)
	$(call transform-td-to-out,opt-parser-defs)
endif

ifneq ($(findstring arm_neon.h,$(TBLGEN_TABLES)),)
LOCAL_GENERATED_SOURCES += $(generated_sources)/include/clang/Basic/arm_neon.h
$(generated_sources)/include/clang/Basic/arm_neon.h: TBLGEN_LOCAL_MODULE := $(LOCAL_MODULE)
$(generated_sources)/include/clang/Basic/arm_neon.h: $(CLANG_ROOT_PATH)/include/clang/Basic/arm_neon.td $(CLANG_TBLGEN)
	$(call transform-clang-td-to-out,arm-neon)
endif

LOCAL_C_INCLUDES += $(generated_sources)/include

endif
