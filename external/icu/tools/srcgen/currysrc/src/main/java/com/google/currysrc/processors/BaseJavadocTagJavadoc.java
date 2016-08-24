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

import com.google.currysrc.api.process.JavadocUtils;
import com.google.currysrc.api.process.Reporter;

import org.eclipse.jdt.core.dom.Javadoc;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;

/**
 * Inserts tag text to the Javadoc for any Javadoc comment that matches
 * {@link #mustTag(org.eclipse.jdt.core.dom.Javadoc)}.
 */
public abstract class BaseJavadocTagJavadoc extends BaseJavadocNodeScanner {

  private final String tagText;

  protected BaseJavadocTagJavadoc(String tagText) {
    this.tagText = tagText;
  }

  @Override protected final void visit(Reporter reporter, Javadoc javadoc, ASTRewrite rewrite) {
    if (mustTag(javadoc)) {
      JavadocUtils.addJavadocTag(rewrite, javadoc, tagText);
    }
  }

  protected abstract boolean mustTag(Javadoc node);
}
