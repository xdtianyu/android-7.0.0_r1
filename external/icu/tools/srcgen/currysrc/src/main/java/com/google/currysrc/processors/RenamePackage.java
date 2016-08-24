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

import org.eclipse.jdt.core.dom.CompilationUnit;
import org.eclipse.jdt.core.dom.Name;
import org.eclipse.jdt.core.dom.PackageDeclaration;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;

/**
 * Change the package name for a {@link CompilationUnit}.
 */
public class RenamePackage implements Processor {

  private final String toMatch;

  private final String replacement;

  public RenamePackage(String toMatch, String replacement) {
    this.toMatch = toMatch;
    this.replacement = replacement;
  }

  @Override
  public void process(Context context, CompilationUnit cu) {
    PackageDeclaration packageDeclaration = cu.getPackage();
    if (packageDeclaration == null) {
      return;
    }
    String fqn = packageDeclaration.getName().getFullyQualifiedName();
    if (!fqn.startsWith(toMatch)) {
      return;
    }
    String newFqn = replacement + fqn.substring(toMatch.length());
    Name newName = cu.getAST().newName(newFqn);
    ASTRewrite rewrite = context.rewrite();
    rewrite.replace(packageDeclaration.getName(), newName, null /* editGroup */);
  }

  @Override
  public String toString() {
    return "RenamePackage{" +
        "toMatch='" + toMatch + '\'' +
        ", replacement='" + replacement + '\'' +
        '}';
  }
}
