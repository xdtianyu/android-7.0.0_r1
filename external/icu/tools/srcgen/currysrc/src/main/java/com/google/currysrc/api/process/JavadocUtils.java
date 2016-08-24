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
package com.google.currysrc.api.process;

import com.google.currysrc.api.process.ast.AstNodes;

import org.eclipse.jdt.core.dom.AST;
import org.eclipse.jdt.core.dom.ASTNode;
import org.eclipse.jdt.core.dom.BodyDeclaration;
import org.eclipse.jdt.core.dom.Javadoc;
import org.eclipse.jdt.core.dom.TagElement;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;
import org.eclipse.jdt.core.dom.rewrite.ListRewrite;

/**
 * Basic utility methods for modifying Javadoc.
 */
public final class JavadocUtils {

  private JavadocUtils() {
  }

  public static void addJavadocTag(ASTRewrite rewrite, BodyDeclaration node, String tagText) {
    Javadoc javadoc = node.getJavadoc();
    if (javadoc == null) {
      AST ast = node.getAST();
      javadoc = (Javadoc) ast.createInstance(Javadoc.class);
      rewrite.set(node, node.getJavadocProperty(), javadoc, null /* editGroup */);
    }
    addJavadocTag(rewrite, javadoc, tagText);
  }

  public static void addJavadocTag(ASTRewrite rewrite, Javadoc javadoc, String tagText) {
    AST ast = javadoc.getAST();
    TagElement tagElement = AstNodes.createTextTagElement(ast, tagText);
    ListRewrite listRewrite = rewrite.getListRewrite(javadoc, Javadoc.TAGS_PROPERTY);
    listRewrite.insertLast(tagElement, null /* editGroup */);
  }

  /**
   * Returns {@code true} if the BodyDeclaration is one that is normally documented. e.g. returns
   * false for initializers.
   */
  public static boolean isNormallyDocumented(BodyDeclaration node) {
    int nodeType = node.getNodeType();
    return nodeType != ASTNode.INITIALIZER;
  }
}
