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

import com.google.currysrc.api.process.Reporter;

import org.eclipse.jdt.core.dom.ASTVisitor;
import org.eclipse.jdt.core.dom.Javadoc;
import org.eclipse.jdt.core.dom.TagElement;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;

/**
 * A base-class for scanners that might modify {@link TagElement} nodes within Javadoc.
 */
public abstract class BaseTagElementNodeScanner extends BaseJavadocNodeScanner {

  @Override protected final void visit(final Reporter reporter, Javadoc javadoc,
      final ASTRewrite rewrite) {

    javadoc.accept(new ASTVisitor(true /* visitDocTags */) {
      @Override public boolean visit(TagElement node) {
        return visitTagElement(reporter, rewrite, node);
      }
    });
  }

  protected abstract boolean visitTagElement(Reporter reporter, ASTRewrite rewrite, TagElement tag);
}
