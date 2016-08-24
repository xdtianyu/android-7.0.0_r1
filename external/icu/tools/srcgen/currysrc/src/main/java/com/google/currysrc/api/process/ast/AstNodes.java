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
package com.google.currysrc.api.process.ast;

import org.eclipse.jdt.core.dom.AST;
import org.eclipse.jdt.core.dom.TagElement;
import org.eclipse.jdt.core.dom.TextElement;

/**
 * Static utility methods for creating ASTNode instances.
 */
public final class AstNodes {

  private AstNodes() {}

  public static TagElement createTextTagElement(AST ast, String text) {
    TagElement element = (TagElement) ast.createInstance(TagElement.class);
    TextElement textElement = createTextElement(ast, text);
    element.fragments().add(textElement);
    return element;
  }

  public static TextElement createTextElement(AST ast, String text) {
    TextElement textElement = ast.newTextElement();
    textElement.setText(text);
    return textElement;
  }

}
