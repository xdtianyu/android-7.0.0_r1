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

import com.google.common.base.Joiner;

import org.eclipse.jdt.core.dom.MethodDeclaration;
import org.eclipse.jdt.core.dom.SingleVariableDeclaration;

import java.util.ArrayList;
import java.util.List;

/**
 * Matches the parameters associated with a method.
 */
public final class ParameterMatcher {

  private final List<String> parameterTypes;

  public ParameterMatcher(List<String> parameterTypes) {
    this.parameterTypes = parameterTypes;
  }

  public boolean matches(MethodDeclaration methodDeclaration) {
    List<String> actualParameterTypes = getParameterTypeNames(methodDeclaration);
    if (actualParameterTypes.size() != parameterTypes.size()) {
      return false;
    }
    for (int i = 0; i < parameterTypes.size(); i++) {
      String actualTypeName = actualParameterTypes.get(i);
      String expectedTypeName = parameterTypes.get(i);
      if (!actualTypeName.equals(expectedTypeName)) {
        return false;
      }
    }
    return true;
  }

  /**
   * Returns the (simple) type names of the parameters for the supplied {@code methodDeclaration}.
   */
  public static List<String> getParameterTypeNames(MethodDeclaration methodDeclaration) {
    List<SingleVariableDeclaration> parameters =
        (List<SingleVariableDeclaration>) methodDeclaration.parameters();
    List<String> toReturn = new ArrayList<>(parameters.size());
    for (SingleVariableDeclaration singleVariableDeclaration : parameters) {
      // toString() does the right thing in all cases.
      String actualTypeName = singleVariableDeclaration.getType().toString();
      toReturn.add(actualTypeName);
    }
    return toReturn;
  }

  /**
   * Returns the parameter types of this matcher in a comma separated list.
   */
  public String toStringForm() {
    return Joiner.on(",").join(parameterTypes);
  }

  @Override
  public String toString() {
    return "ParameterMatcher{" +
        "parameterTypes=" + parameterTypes +
        '}';
  }
}
