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

import org.eclipse.jdt.core.dom.AbstractTypeDeclaration;
import org.eclipse.jdt.core.dom.BodyDeclaration;
import org.eclipse.jdt.core.dom.CompilationUnit;
import org.eclipse.jdt.core.dom.FieldDeclaration;
import org.eclipse.jdt.core.dom.VariableDeclarationFragment;

import java.util.ArrayList;
import java.util.List;

/**
 * Locates the {@link org.eclipse.jdt.core.dom.BodyDeclaration} associated with an field
 * declaration.
 */
public final class FieldLocator implements BodyDeclarationLocator {

  static final String LOCATOR_TYPE_NAME = "field";

  private final TypeLocator typeLocator;

  private final String fieldName;

  public FieldLocator(String packageName, String typeName, String fieldName) {
    this(new TypeLocator(packageName, typeName), fieldName);
  }

  public FieldLocator(TypeLocator typeLocator, String fieldName) {
    this.typeLocator = typeLocator;
    this.fieldName = fieldName;
  }

  @Override public TypeLocator getTypeLocator() {
    return typeLocator;
  }

  @Override
  public boolean matches(BodyDeclaration node) {
    if (node instanceof FieldDeclaration) {
      FieldDeclaration fieldDeclaration = (FieldDeclaration) node;
      for (VariableDeclarationFragment variableDeclarationFragment
          : (List<VariableDeclarationFragment>) fieldDeclaration.fragments()) {
        String nodeFieldName = variableDeclarationFragment.getName().getFullyQualifiedName();
        if (nodeFieldName.equals(fieldName)) {
          BodyDeclaration parentNode = (BodyDeclaration) node.getParent();
          return typeLocator.matches(parentNode);
        }
      }
    }
    return false;
  }

  @Override
  public FieldDeclaration find(CompilationUnit cu) {
    AbstractTypeDeclaration typeDeclaration = typeLocator.find(cu);
    if (typeDeclaration == null) {
      return null;
    }
    for (BodyDeclaration bodyDeclaration
        : (List<BodyDeclaration>) typeDeclaration.bodyDeclarations()) {
      if (bodyDeclaration instanceof FieldDeclaration) {
        FieldDeclaration fieldDeclaration = (FieldDeclaration) bodyDeclaration;
        for (VariableDeclarationFragment variableDeclarationFragment
            : (List<VariableDeclarationFragment>) fieldDeclaration.fragments()) {
          String nodeFieldName = variableDeclarationFragment.getName().getFullyQualifiedName();
          if (nodeFieldName.equals(fieldName)) {
            return fieldDeclaration;
          }
        }
      }
    }
    return null;
  }

  @Override public String getStringFormType() {
    return LOCATOR_TYPE_NAME;
  }

  @Override public String getStringFormTarget() {
    return typeLocator.getStringFormTarget() + "#" + fieldName;
  }

  /**
   * Returns the names of the fields declared in the supplied {@code fieldDeclarationNode}.
   */
  public static List<String> getFieldNames(FieldDeclaration fieldDeclarationNode) {
    List<VariableDeclarationFragment> fieldDeclarations = fieldDeclarationNode.fragments();
    List<String> fieldNames = new ArrayList<>(fieldDeclarations.size());
    for (VariableDeclarationFragment fieldDeclaration : fieldDeclarations) {
      fieldNames.add(fieldDeclaration.getName().getIdentifier());
    }
    return fieldNames;
  }

  @Override
  public String toString() {
    return "FieldLocator{" +
        "typeLocator=" + typeLocator +
        ", fieldName='" + fieldName + '\'' +
        '}';
  }
}
