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

import com.google.currysrc.api.process.ast.TypeLocator;

import org.eclipse.jdt.core.dom.AbstractTypeDeclaration;
import org.eclipse.jdt.core.dom.Modifier;

import java.util.List;

/**
 * Hides any public class that is not found in the whitelist.
 */
public final class HidePublicClasses extends BaseJavadocTagClasses {

  private final List<TypeLocator> whitelist;

  public HidePublicClasses(List<TypeLocator> whitelist, String reason) {
    super("@hide " + reason);
    this.whitelist = whitelist;
  }

  @Override
  protected boolean mustTag(AbstractTypeDeclaration node) {
    boolean mustTag = false;
    if ((node.getModifiers() & Modifier.PUBLIC) > 0) {
      mustTag = true;
      for (TypeLocator whitelistedType : whitelist) {
        if (whitelistedType.matches(node)) {
          mustTag = false;
          break;
        }
      }
    }
    return mustTag;
  }

  @Override
  public String toString() {
    return "HidePublicClasses{" +
        "whitelist=" + whitelist +
        '}';
  }
}
