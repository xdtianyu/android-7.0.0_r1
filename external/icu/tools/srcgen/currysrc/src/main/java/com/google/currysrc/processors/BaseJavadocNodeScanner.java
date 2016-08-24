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
import org.eclipse.jdt.core.dom.Javadoc;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;

import java.util.List;

/**
 * A base class for processors that process Javadoc nodes and may rewrite the AST. All Javadoc nodes
 * in a {@code CompilationUnit} are considered.
 */
public abstract class BaseJavadocNodeScanner implements Processor {

  @Override public final void process(Context context, CompilationUnit cu) {
    // This could just call cu.visit() but iterating over the comments should be more efficient.
    List<Comment> comments = cu.getCommentList();
    ASTRewrite rewrite = context.rewrite();
    Reporter reporter = context.reporter();
    for (Comment comment : Lists.reverse(comments)) {
      if (comment instanceof Javadoc) {
        Javadoc javadoc = (Javadoc) comment;
        visit(reporter, javadoc, rewrite);
      }
    }
  }

  protected abstract void visit(Reporter reporter, Javadoc javadoc, ASTRewrite rewrite);
}
