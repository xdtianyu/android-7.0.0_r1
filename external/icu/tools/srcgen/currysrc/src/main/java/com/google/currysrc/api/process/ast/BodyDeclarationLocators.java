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

import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableList;

import org.eclipse.jdt.core.dom.ASTNode;
import org.eclipse.jdt.core.dom.AbstractTypeDeclaration;
import org.eclipse.jdt.core.dom.BodyDeclaration;
import org.eclipse.jdt.core.dom.EnumConstantDeclaration;
import org.eclipse.jdt.core.dom.FieldDeclaration;
import org.eclipse.jdt.core.dom.MethodDeclaration;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Utility methods associated with {@link BodyDeclarationLocator} and its standard implementations.
 */
public final class BodyDeclarationLocators {

  private BodyDeclarationLocators() {
  }

  public static List<BodyDeclarationLocator> createLocatorsFromStrings(String[] locatorStrings) {
    ImmutableList.Builder<BodyDeclarationLocator> locatorListBuilder = ImmutableList.builder();
    for (String locatorString : locatorStrings) {
      BodyDeclarationLocator locator = BodyDeclarationLocators.fromStringForm(locatorString);
      locatorListBuilder.add(locator);
    }
    return locatorListBuilder.build();
  }

  /**
   * Generates strings that can be used with {@link #fromStringForm(String)} to generate
   * {@link BodyDeclarationLocator} instances capable of locating the supplied node. Usually returns
   * a single element, except for fields declarations.
   */
  public static List<String> toLocatorStringForms(BodyDeclaration bodyDeclaration) {
    List<BodyDeclarationLocator> locators = createLocators(bodyDeclaration);
    List<String> stringForms = new ArrayList<>(locators.size());
    for (BodyDeclarationLocator locator : locators) {
      stringForms.add(locator.getStringFormType() + ":" + locator.getStringFormTarget());
    }
    return stringForms;
  }

  /**
   * Creates {@link BodyDeclarationLocator} objects that can find the supplied
   * {@link BodyDeclaration}. Usually returns a single element, except for fields declarations.
   */
  public static List<BodyDeclarationLocator> createLocators(BodyDeclaration bodyDeclaration) {
    AbstractTypeDeclaration typeDeclaration = TypeLocator.findTypeDeclarationNode(bodyDeclaration);
    if (typeDeclaration == null) {
      throw new AssertionError("Unable to find type declaration for " + typeDeclaration);
    }
    TypeLocator typeLocator = new TypeLocator(typeDeclaration);

    int nodeType = bodyDeclaration.getNodeType();
    switch (nodeType) {
      case ASTNode.FIELD_DECLARATION:
        List<String> fieldNames = FieldLocator.getFieldNames((FieldDeclaration) bodyDeclaration);
        List<BodyDeclarationLocator> fieldLocators = new ArrayList<>(fieldNames.size());
        for (String fieldName : fieldNames) {
          fieldLocators.add(new FieldLocator(typeLocator, fieldName));
        }
        return fieldLocators;
      case ASTNode.METHOD_DECLARATION:
        MethodDeclaration methodDeclaration = (MethodDeclaration) bodyDeclaration;
        List<String> parameterTypeNames = ParameterMatcher.getParameterTypeNames(methodDeclaration);
        return ImmutableList.<BodyDeclarationLocator>of(
            new MethodLocator(typeLocator, methodDeclaration.getName().getIdentifier(),
            parameterTypeNames));
      case ASTNode.TYPE_DECLARATION:
      case ASTNode.ENUM_DECLARATION:
        return ImmutableList.<BodyDeclarationLocator>of(typeLocator);
      case ASTNode.ENUM_CONSTANT_DECLARATION:
        EnumConstantDeclaration enumConstantDeclaration = (EnumConstantDeclaration) bodyDeclaration;
        String constantName = enumConstantDeclaration.getName().getIdentifier();
        return ImmutableList.<BodyDeclarationLocator>of(
            new EnumConstantLocator(typeLocator, constantName));
      default:
        throw new IllegalArgumentException("Unsupported node type: " + nodeType);
    }
  }

  /**
   * Creates a {@link BodyDeclarationLocator} from a string form of a body declaration.
   * See {@link #toLocatorStringForms(BodyDeclaration)}.
   */
  public static BodyDeclarationLocator fromStringForm(String stringForm) {
    List<String> components = splitInTwo(stringForm, ":");
    String locatorTypeName = components.get(0);
    String locatorString = components.get(1);
    switch (locatorTypeName) {
      case FieldLocator.LOCATOR_TYPE_NAME:
        List<String> typeAndField = splitInTwo(locatorString, "#");
        return new FieldLocator(new TypeLocator(typeAndField.get(0)), typeAndField.get(1));
      case MethodLocator.LOCATOR_TYPE_NAME:
        List<String> typeAndMethod = splitInTwo(locatorString, "#");
        String methodNameAndParameters = typeAndMethod.get(1);
        int parameterStartIndex = methodNameAndParameters.indexOf('(');
        if (parameterStartIndex == -1) {
          throw new IllegalArgumentException("No '(' found in " + methodNameAndParameters);
        }
        String methodName = methodNameAndParameters.substring(0, parameterStartIndex);
        String parametersString = methodNameAndParameters.substring(parameterStartIndex);
        List<String> parameterTypes = extractParameterTypes(parametersString);
        return new MethodLocator(new TypeLocator(typeAndMethod.get(0)), methodName, parameterTypes);
      case TypeLocator.LOCATOR_TYPE_NAME:
        return new TypeLocator(locatorString);
      case EnumConstantLocator.LOCATOR_TYPE_NAME:
        List<String> typeAndConstant = splitInTwo(locatorString, "#");
        return new EnumConstantLocator(
            new TypeLocator(typeAndConstant.get(0)), typeAndConstant.get(1));
      default:
        throw new IllegalArgumentException("Unsupported locator type: " + locatorTypeName);
    }
  }

  public static boolean matchesAny(List<BodyDeclarationLocator> locators, BodyDeclaration node) {
    for (BodyDeclarationLocator locator : locators) {
      if (locator.matches(node)) {
        return true;
      }
    }
    return false;
  }

  /**
   * Finds the declaration associated with a given node. If the node is not a child of a declaration
   * {@code null} is returned.
   */
  public static BodyDeclaration findDeclarationNode(ASTNode node) {
    ASTNode ancestor = node;
    while (ancestor != null && !(ancestor instanceof BodyDeclaration)) {
      ancestor = ancestor.getParent();
    }

    return ancestor instanceof BodyDeclaration ? (BodyDeclaration) ancestor : null;
  }

  private static List<String> extractParameterTypes(String parametersString) {
    if (!(parametersString.startsWith("(") && parametersString.endsWith(")"))) {
      throw new IllegalArgumentException("Expected \"(<types>)\" but was " + parametersString);
    }
    parametersString = parametersString.substring(1, parametersString.length() - 1);
    if (parametersString.isEmpty()) {
      return Collections.emptyList();
    }
    return Splitter.on(',').splitToList(parametersString);
  }

  private static List<String> splitInTwo(String string, String separator) {
    List<String> components = Splitter.on(separator).splitToList(string);
    if (components.size() != 2) {
      throw new IllegalArgumentException("Cannot split " + string + " on " + separator);
    }
    return components;
  }
}
