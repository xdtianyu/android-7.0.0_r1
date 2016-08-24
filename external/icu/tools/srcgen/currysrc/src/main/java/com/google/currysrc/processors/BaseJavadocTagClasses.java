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

import com.google.common.collect.Lists;
import com.google.currysrc.api.process.Context;
import com.google.currysrc.api.process.JavadocUtils;
import com.google.currysrc.api.process.Processor;

import org.eclipse.jdt.core.dom.ASTVisitor;
import org.eclipse.jdt.core.dom.AbstractTypeDeclaration;
import org.eclipse.jdt.core.dom.CompilationUnit;
import org.eclipse.jdt.core.dom.EnumDeclaration;
import org.eclipse.jdt.core.dom.TypeDeclaration;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;

import java.util.List;

/**
 * Inserts the tag text to the Javadoc for any type declaration that matches
 * {@link #mustTag(AbstractTypeDeclaration)}.
 */
public abstract class BaseJavadocTagClasses implements Processor {

  private final String tagText;

  protected BaseJavadocTagClasses(String tagText) {
    this.tagText = tagText;
  }

  @Override public final void process(Context context, CompilationUnit cu) {
    final List<AbstractTypeDeclaration> toHide = Lists.newArrayList();
    cu.accept(new ASTVisitor() {
      @Override
      public boolean visit(TypeDeclaration node) {
        return visitAbstract(node);
      }

      @Override
      public boolean visit(EnumDeclaration node) {
        return visitAbstract(node);
      }

      private boolean visitAbstract(AbstractTypeDeclaration node) {
        if (mustTag(node)) {
          toHide.add(node);
        }
        return false;
      }
    });
    ASTRewrite rewrite = context.rewrite();
    for (AbstractTypeDeclaration node : Lists.reverse(toHide)) {
      JavadocUtils.addJavadocTag(rewrite, node, tagText);
    }
  }

  protected abstract boolean mustTag(AbstractTypeDeclaration node);
}
