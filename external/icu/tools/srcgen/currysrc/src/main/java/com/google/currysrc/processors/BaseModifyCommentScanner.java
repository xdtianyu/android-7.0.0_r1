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
import com.google.currysrc.api.process.Processor;
import com.google.currysrc.api.process.Reporter;

import org.eclipse.jdt.core.dom.Comment;
import org.eclipse.jdt.core.dom.CompilationUnit;
import org.eclipse.jface.text.BadLocationException;
import org.eclipse.jface.text.Document;

import java.util.List;

/**
 * A base-class for general comment processors. All comments of all types in a
 * {@link CompilationUnit} are considered. Subclasses determine whether to make a complete comment
 * replacement.
 */
public abstract class BaseModifyCommentScanner implements Processor {

  @Override
  public final void process(Context context, CompilationUnit cu) {
    Document document = context.document();
    Reporter reporter = context.reporter();
    List<Comment> comments = cu.getCommentList();
    try {
      for (Comment comment : Lists.reverse(comments)) {
        String commentText = document.get(comment.getStartPosition(), comment.getLength());
        String newCommentText = processComment(reporter, comment, commentText);
        if (newCommentText != null) {
          document.replace(comment.getStartPosition(), comment.getLength(), newCommentText);
        }
      }
    } catch (BadLocationException e) {
      throw new AssertionError(e);
    }
  }

  /**
   * Generates new text for the comment, or returns {@code null} if there is nothing to change.
   * Comments are passed in the reverse order that they appear in source code to ensure that
   * document offsets remain valid.
   */
  protected abstract String processComment(Reporter reporter, Comment commentNode,
      String commentText);
}
