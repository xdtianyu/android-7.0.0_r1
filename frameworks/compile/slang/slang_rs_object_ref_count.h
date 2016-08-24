/*
 * Copyright 2010, The Android Open Source Project
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

#ifndef _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_OBJECT_REF_COUNT_H_  // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_OBJECT_REF_COUNT_H_

#include <list>
#include <stack>

#include "clang/AST/StmtVisitor.h"

#include "slang_assert.h"
#include "slang_rs_export_type.h"

namespace clang {
  class Expr;
  class Stmt;
}

namespace slang {

// Recursive check
bool HasRSObjectType(const clang::Type *T);

// This class provides the overall reference counting mechanism for handling
// local variables of RS object types (rs_font, rs_allocation, ...). This
// class ensures that appropriate functions (rsSetObject, rsClearObject) are
// called at proper points in the object's lifetime.
// 1) Each local object of appropriate type must be zero-initialized to
// prevent corruption during subsequent rsSetObject()/rsClearObject() calls.
// 2) Assignments using these types must also be converted into the
// appropriate (possibly a series of) rsSetObject() calls.
// 3) Finally, rsClearObject() must be called for each local object when it goes
// out of scope.
class RSObjectRefCount : public clang::StmtVisitor<RSObjectRefCount> {
 private:
  class Scope {
   private:
    clang::CompoundStmt *mCS;         // Associated compound statement ({ ... })
    clang::Stmt *mCurrent;            // The statement currently being analyzed
    std::list<clang::VarDecl*> mRSO;  // Declared RS objects in this scope (but
                                      // not any scopes nested)

   public:
    explicit Scope(clang::CompoundStmt *CS) : mCS(CS) {
    }

    bool hasRSObject() const { return !mRSO.empty(); }

    inline void addRSObject(clang::VarDecl* VD) {
      mRSO.push_back(VD);
    }

    void ReplaceRSObjectAssignment(clang::BinaryOperator *AS);

    void AppendRSObjectInit(clang::VarDecl *VD,
                            clang::DeclStmt *DS,
                            DataType DT,
                            clang::Expr *InitExpr);

    // Inserts rsClearObject() calls at the end and at all exiting points of the
    // current scope. At each statement that exits the current scope -- e.g.,
    // a return, break, or continue statement in the current or a nested scope
    // -- rsClearObject() calls are inserted for local variables defined in the
    // current scope before that point.
    // Note goto statements are not handled. (See the DestructorVisitor class in
    // the .cpp file.)
    // Also note this function is called for every nested scope. As a result, for a
    // return statement, each rsObject declared in all its (nested) enclosing
    // scopes would have a rsClearObject() call properly inserted before
    // the return statement.
    void InsertLocalVarDestructors();

    // Sets the current statement being analyzed
    void setCurrentStmt(clang::Stmt *S) { mCurrent = S; }

    // Inserts a statement before the current statement
    void InsertStmt(const clang::ASTContext &C, clang::Stmt *NewStmt);

    // Replaces the current statement with NewStmt;
    void ReplaceStmt(const clang::ASTContext &C, clang::Stmt *NewStmt);

    // Replaces OldExpr with NewExpr in the current statement
    void ReplaceExpr(const clang::ASTContext& C, clang::Expr* OldExpr,
                     clang::Expr* NewExpr);

    static clang::Stmt *ClearRSObject(clang::VarDecl *VD,
                                      clang::DeclContext *DC);
  };

  clang::ASTContext &mCtx;
  std::deque<Scope*> mScopeStack;  // A deque used as a stack to store scopes, but also
                                   // accessed through its iterator in read-only mode.
  clang::DeclContext* mCurrentDC;
  bool RSInitFD;
  unsigned mTempID;  // A unique id that can be used to distinguish temporary variables

  // RSSetObjectFD and RSClearObjectFD holds FunctionDecl of rsSetObject()
  // and rsClearObject() in the current ASTContext.
  static clang::FunctionDecl *RSSetObjectFD[];
  static clang::FunctionDecl *RSClearObjectFD[];

  inline bool emptyScope() const { return mScopeStack.empty(); }

  inline Scope *getCurrentScope() {
    return mScopeStack.back();
  }

  // Returns the next available unique id for temporary variables
  unsigned getNextID() { return mTempID++; }

  // Initialize RSSetObjectFD and RSClearObjectFD.
  static void GetRSRefCountingFunctions(clang::ASTContext &C);

  // Return false if the type of variable declared in VD does not contain
  // an RS object type.
  static bool InitializeRSObject(clang::VarDecl *VD,
                                 DataType *DT,
                                 clang::Expr **InitExpr);

  // Return an empty list initializer expression at the appropriate location.
  // This construct can then be used to cheaply construct a zero-initializer
  // for any RenderScript objects (like rs_allocation) or rs_matrix* types
  // (possibly even embedded within other types). These types are expected to
  // be zero-initialized always, and so we can use this helper to ensure that
  // they at least have an empty initializer.
  static clang::Expr *CreateEmptyInitListExpr(
      clang::ASTContext &C,
      const clang::SourceLocation &Loc);

  // Given a return statement RS that returns an rsObject, creates a temporary
  // variable, and sets it to the original return expression using rsSetObject().
  // Creates a new return statement that returns the temporary variable.
  // Returns a new compound statement that contains the new variable declaration,
  // the rsSetOjbect() call, and the new return statement.
  static clang::CompoundStmt* CreateRetStmtWithTempVar(
      clang::ASTContext& C,
      clang::DeclContext* DC,
      clang::ReturnStmt* RS,
      const unsigned id);

 public:
  explicit RSObjectRefCount(clang::ASTContext &C)
      : mCtx(C), RSInitFD(false), mTempID(0) {
  }

  void Init() {
    if (!RSInitFD) {
      GetRSRefCountingFunctions(mCtx);
      RSInitFD = true;
    }
  }

  static clang::FunctionDecl *GetRSSetObjectFD(DataType DT) {
    slangAssert(RSExportPrimitiveType::IsRSObjectType(DT));
    if (DT >= 0 && DT < DataTypeMax) {
      return RSSetObjectFD[DT];
    } else {
      slangAssert(false && "incorrect type");
      return nullptr;
    }
  }

  static clang::FunctionDecl *GetRSSetObjectFD(const clang::Type *T) {
    return GetRSSetObjectFD(RSExportPrimitiveType::GetRSSpecificType(T));
  }

  static clang::FunctionDecl *GetRSClearObjectFD(DataType DT) {
    slangAssert(RSExportPrimitiveType::IsRSObjectType(DT));
    if (DT >= 0 && DT < DataTypeMax) {
      return RSClearObjectFD[DT];
    } else {
      slangAssert(false && "incorrect type");
      return nullptr;
    }
  }

  static clang::FunctionDecl *GetRSClearObjectFD(const clang::Type *T) {
    return GetRSClearObjectFD(RSExportPrimitiveType::GetRSSpecificType(T));
  }

  void SetDeclContext(clang::DeclContext* DC) { mCurrentDC = DC; }
  clang::DeclContext* GetDeclContext() const { return mCurrentDC; }

  void VisitStmt(clang::Stmt *S);
  void VisitCallExpr(clang::CallExpr *CE);
  void VisitDeclStmt(clang::DeclStmt *DS);
  void VisitCompoundStmt(clang::CompoundStmt *CS);
  void VisitBinAssign(clang::BinaryOperator *AS);
  void VisitReturnStmt(clang::ReturnStmt *RS);
  // We believe that RS objects are never involved in CompoundAssignOperator.
  // I.e., rs_allocation foo; foo += bar;

  // Emit a global destructor to clean up RS objects.
  clang::FunctionDecl *CreateStaticGlobalDtor();
};

}  // namespace slang

#endif  // _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_OBJECT_REF_COUNT_H_  NOLINT
