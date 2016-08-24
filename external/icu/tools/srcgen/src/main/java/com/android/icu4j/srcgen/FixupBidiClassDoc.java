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
package com.android.icu4j.srcgen;

import com.google.currysrc.api.match.SourceMatcher;
import com.google.currysrc.api.match.SourceMatchers;
import com.google.currysrc.api.process.Context;
import com.google.currysrc.api.process.Processor;
import com.google.currysrc.api.process.ast.TypeLocator;

import org.eclipse.jdt.core.dom.CompilationUnit;
import org.eclipse.jdt.core.dom.Javadoc;
import org.eclipse.jdt.core.dom.TypeDeclaration;
import org.eclipse.jface.text.BadLocationException;
import org.eclipse.jface.text.Document;

/**
 * Hack to fix upstream code comments before removing @stable tags. Failure to do this would cause
 * use to remove all the text that followed.
 */
public class FixupBidiClassDoc implements Processor {

  private final static TypeLocator LOCATOR = new TypeLocator("android.icu.text.Bidi");

  /**
   * These tags appear in the middle of some javadoc, making the parser think they are related to
   * the text that follows it. We just remove it here.
   */
  private static final String BAD_TEXT =
      " * @author Simon Montagu, Matitiahu Allouche (ported from C code written by Markus W. "
          + "Scherer)\n * @stable ICU 3.8\n";

  public SourceMatcher matcher() {
    return SourceMatchers.contains(LOCATOR);
  }

  @Override public void process(Context context, CompilationUnit cu) {
    TypeDeclaration classNode = (TypeDeclaration) LOCATOR.find(cu);
    Javadoc javadoc = classNode.getJavadoc();
    try {
      Document document = context.document();
      String commentText = document.get(javadoc.getStartPosition(), javadoc.getLength());
      String newCommentText = commentText.replace(BAD_TEXT, "");
      document.replace(javadoc.getStartPosition(), javadoc.getLength(), newCommentText);
    } catch (BadLocationException e) {
      throw new AssertionError(e);
    }
  }
}
