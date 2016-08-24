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

import com.google.currysrc.api.process.Reporter;
import com.google.currysrc.api.process.ast.AstNodes;
import com.google.currysrc.processors.BaseTagElementNodeScanner;

import org.eclipse.jdt.core.dom.AST;
import org.eclipse.jdt.core.dom.IDocElement;
import org.eclipse.jdt.core.dom.TagElement;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;

import java.util.List;

/**
 * Replaces {@icu}, {@icuenhanced} and {@icunote} with text.
 */
public class ReplaceIcuTags extends BaseTagElementNodeScanner {

  @Override
  protected boolean visitTagElement(Reporter reporter, ASTRewrite rewrite, TagElement tag) {
    String tagName = tag.getTagName();
    if (tagName != null) {
      AST ast = tag.getAST();
      List<IDocElement> fragments = tag.fragments();
      if (tagName.equalsIgnoreCase("@icu")) {
        // ICU replaces {@icu __usage__} with "Methods, fields, and other functionality specific to
        // ICU are labeled '[icu]'"
        // ICU replaces {@icu} with [icu]
        if (fragments.size() == 0) {
          rewrite.replace(tag, createIcuMarker(ast), null /* editGroup */);
        } else {
          IDocElement element = fragments.get(0);
          if (element.toString().trim().equalsIgnoreCase("_usage_")) {
            rewrite.replace(tag, createIcuUsageText(ast), null /* editGroup */);
          } else {
            throw new AssertionError("Unknown Javadoc tag: " + tag);
          }
        }
        return false;
      } else if (tagName.equalsIgnoreCase("@icuenhanced")) {
        // ICU replaces {@icuenhanced <classname>} with "[icu enhancement] ICU's replacement for
        // <classname>"
        IDocElement element = fragments.get(0);
        rewrite.replace(tag, createIcuEnhancementText(ast, element), null /* editGroup */);
        return false;
      } else if (tagName.equalsIgnoreCase("@icunote")) {
        // ICU replaces {@icunote} with "[icu] Note:"
        rewrite.replace(tag, createIcuNoteText(ast), null /* editGroup */);
        return false;
      }
    }
    return true;
  }

  private static TagElement createIcuEnhancementText(AST ast, IDocElement fragment) {
    return AstNodes.createTextTagElement(ast,
        "<strong>[icu enhancement]</strong> ICU's replacement for {@link" + fragment.toString()
            + "}");
  }

  private static TagElement createIcuUsageText(AST ast) {
    // Use of &nbsp; is a hacky way to preserve whitespace.
    return AstNodes.createTextTagElement(ast,
        "&nbsp;Methods, fields, and other functionality specific to ICU are labeled"
            + " '<strong>[icu]</strong>'.");
  }

  private static TagElement createIcuNoteText(AST ast) {
    return AstNodes.createTextTagElement(ast, "<strong>[icu] Note:</strong>");
  }

  private static TagElement createIcuMarker(AST ast) {
    return AstNodes.createTextTagElement(ast, "<strong>[icu]</strong>");
  }
}
