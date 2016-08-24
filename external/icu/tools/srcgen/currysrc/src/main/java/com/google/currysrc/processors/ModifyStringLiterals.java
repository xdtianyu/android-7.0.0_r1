/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.currysrc.processors;

import com.google.currysrc.api.process.Context;
import com.google.currysrc.api.process.Processor;

import org.eclipse.jdt.core.dom.ASTVisitor;
import org.eclipse.jdt.core.dom.CompilationUnit;
import org.eclipse.jdt.core.dom.StringLiteral;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;

/**
 * Changes any string literals in the AST that start with {@code oldPrefix} to start with
 * {@code newPrefix} instead.
 */
public class ModifyStringLiterals implements Processor {

  private final String oldString;

  private final String newString;

  public ModifyStringLiterals(String oldString, final String newString) {
    this.oldString = oldString;
    this.newString = newString;
  }

  @Override
  public void process(Context context, CompilationUnit cu) {
    final ASTRewrite rewrite = context.rewrite();
    ASTVisitor visitor = new ASTVisitor(false /* visitDocTags */) {
      @Override
      public boolean visit(StringLiteral node) {
        String literalValue = node.getLiteralValue();
        // TODO Replace with Pattern
        if (literalValue.contains(oldString)) {
          String newLiteralValue = literalValue.replace(oldString, newString);
          StringLiteral newLiteral = node.getAST().newStringLiteral();
          newLiteral.setLiteralValue(newLiteralValue);
          rewrite.replace(node, newLiteral, null /* editGorup */);
        }
        return false;
      }
    };
    cu.accept(visitor);
  }

  @Override
  public String toString() {
    return "ModifyStringLiterals{" +
        "oldString='" + oldString + '\'' +
        ", newString='" + newString + '\'' +
        '}';
  }
}
