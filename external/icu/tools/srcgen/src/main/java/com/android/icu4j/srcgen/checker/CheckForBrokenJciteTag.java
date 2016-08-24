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
package com.android.icu4j.srcgen.checker;

import com.google.currysrc.api.process.JavadocUtils;
import com.google.currysrc.api.process.Reporter;
import com.google.currysrc.api.process.ast.BodyDeclarationLocators;
import com.google.currysrc.processors.BaseModifyCommentScanner;

import com.android.icu4j.srcgen.TranslateJcite;

import org.eclipse.jdt.core.dom.BodyDeclaration;
import org.eclipse.jdt.core.dom.Comment;
import org.eclipse.jdt.core.dom.Javadoc;

import java.util.List;
import java.util.Set;

/**
 * Checks for escaped (rather than transformed) jcite tags and warns for any found in the
 * public APIs.
 */
class CheckForBrokenJciteTag extends BaseModifyCommentScanner {

  private final Set<String> publicMembers;

  public CheckForBrokenJciteTag(Set<String> publicMembers) {
    this.publicMembers = publicMembers;
  }

  @Override
  protected String processComment(Reporter reporter, Comment commentNode, String commentText) {
    if (commentNode instanceof Javadoc
        && commentText.contains(TranslateJcite.ESCAPED_JCITE_TAG)) {
      BodyDeclaration declaration = BodyDeclarationLocators.findDeclarationNode(commentNode);
      if (JavadocUtils.isNormallyDocumented(declaration)) {
        List<String> locatorStrings = BodyDeclarationLocators.toLocatorStringForms(declaration);
        for (String locatorString : locatorStrings) {
          if (publicMembers.contains(locatorString)) {
            reporter.info(
                commentNode, "Escaped JCITE tag found in public API docs:" + commentText);
          } else {
            reporter.info(
                commentNode, "Escaped JCITE tag found in non-public API docs (this is fine)");
          }
        }
      }
    }
    return null;
  }
}
