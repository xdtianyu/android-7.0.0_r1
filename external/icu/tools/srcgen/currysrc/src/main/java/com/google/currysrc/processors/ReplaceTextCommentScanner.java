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

import org.eclipse.jdt.core.dom.Comment;

/**
 * Replaces all occurrences of {@code oldText} with {@code newText}.
 */
public final class ReplaceTextCommentScanner extends BaseModifyCommentScanner {
  private final String oldText;
  private final String newText;

  public ReplaceTextCommentScanner(String oldText, String newText) {
    this.oldText = oldText;
    this.newText = newText;
  }

  @Override
  protected String processComment(Reporter reporter, Comment commentNode, String commentText) {
    String newCommentText = commentText.replace(oldText, newText);
    if (newCommentText.equals(commentText)) {
      return null;
    }
    return newCommentText;
  }

  @Override
  public String toString() {
    return "ReplaceCommentText{" +
        "oldText='" + oldText + '\'' +
        ", newText='" + newText + '\'' +
        '}';
  }
}
