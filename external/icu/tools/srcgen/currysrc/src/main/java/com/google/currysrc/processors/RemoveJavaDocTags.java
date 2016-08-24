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

import com.google.common.collect.ImmutableSet;
import com.google.currysrc.api.process.Reporter;

import org.eclipse.jdt.core.dom.Javadoc;
import org.eclipse.jdt.core.dom.TagElement;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;

import java.util.List;
import java.util.Set;

/**
 * Remove the specified JavaDoc tags from the AST. Assumes the Javadoc is well-formed.
 */
public final class RemoveJavaDocTags extends BaseJavadocNodeScanner {

  private final Set<String> tagsToRemove;

  public RemoveJavaDocTags(String... tags) {
    ImmutableSet.Builder<String> builder = ImmutableSet.builder();
    for (String tag : tags) {
      builder.add(tag.toLowerCase());
    }
    tagsToRemove = builder.build();
  }

  @Override
  protected void visit(Reporter reporter, Javadoc javadoc, ASTRewrite rewrite) {
    for (TagElement tagElement : (List<TagElement>) javadoc.tags()) {
      String tagName = tagElement.getTagName();
      if (tagName == null) {
        continue;
      }
      if (tagsToRemove.contains(tagName.toLowerCase())) {
        rewrite.remove(tagElement, null /* editGroup */);
      }
    }
  }

  @Override public String toString() {
    return "RemoveJavaDocTags{" +
        "tagsToRemove=" + tagsToRemove +
        '}';
  }
}
