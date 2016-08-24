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
import com.google.common.base.Splitter;
import com.google.common.collect.Lists;
import com.google.currysrc.api.match.TypeName;

import org.eclipse.jdt.core.dom.ASTNode;
import org.eclipse.jdt.core.dom.AbstractTypeDeclaration;
import org.eclipse.jdt.core.dom.BodyDeclaration;
import org.eclipse.jdt.core.dom.CompilationUnit;

import java.util.Iterator;
import java.util.List;

/**
 * Locates the {@link org.eclipse.jdt.core.dom.BodyDeclaration} associated with an class
 * declaration.
 */
public final class TypeLocator implements BodyDeclarationLocator {

  static final String LOCATOR_TYPE_NAME = "type";

  private final PackageMatcher packageMatcher;
  private final List<String> classNameElements;

  /**
   * Creates a {@code TypeLocator} for a fully-qualified class name.
   *
   * @param fullyQualifiedClassName the package name (if any) and the class,
   *     e.g. foo.bar.baz.FooBar$Baz
   */
  public TypeLocator(String fullyQualifiedClassName) {
    this(TypeName.fromFullyQualifiedClassName(fullyQualifiedClassName));
  }

  /**
   * Creates a {@code TypeLocator} for a {@link TypeName}.
   */
  public TypeLocator(TypeName typeName) {
    this(typeName.packageName(), typeName.className());
  }

  /**
   * Creates a {@code TypeLocator} using an explicit package and class name spec.
   *
   * @param packageName the fully-qualified package name. e.g. foo.bar.baz, or ""
   * @param className the class name with $ as the separator for nested/inner classes. e.g. FooBar,
   *     FooBar$Baz.
   */
  public TypeLocator(String packageName, String className) {
    this.packageMatcher = new PackageMatcher(packageName);
    this.classNameElements = classNameElements(className);
    if (classNameElements.isEmpty()) {
      throw new IllegalArgumentException("Empty className");
    }
  }

  /**
   * Creates a {@code TypeLocator} for the specified {@link ASTNode}.
   */
  public TypeLocator(final AbstractTypeDeclaration typeDeclaration) {
    if (typeDeclaration.isLocalTypeDeclaration()) {
      throw new IllegalArgumentException("Local types not supported: " + typeDeclaration);
    }

    CompilationUnit cu = (CompilationUnit) typeDeclaration.getRoot();
    this.packageMatcher = new PackageMatcher(PackageMatcher.getPackageName(cu));

    // Traverse the type declarations towards the root, building up a list of type names in
    // reverse order.
    List<String> typeNames = Lists.newArrayList();
    AbstractTypeDeclaration currentNode = typeDeclaration;
    while (typeDeclaration != null) {
      typeNames.add(currentNode.getName().getFullyQualifiedName());
      // Handle nested / inner classes.
      ASTNode parentNode = currentNode.getParent();
      if (parentNode != null) {
        if (parentNode == cu) {
          break;
        }
        if (!(parentNode instanceof AbstractTypeDeclaration)) {
          throw new AssertionError(
              "Unexpected parent for nested/inner class: parent=" + parentNode + " of "
                  + currentNode);
        }
      }
      currentNode = (AbstractTypeDeclaration) parentNode;
    }
    this.classNameElements = Lists.reverse(typeNames);
  }

  @Override public TypeLocator getTypeLocator() {
    return this;
  }

  @Override
  public boolean matches(BodyDeclaration node) {
    if (!(node instanceof AbstractTypeDeclaration)) {
      return false;
    }
    if (!packageMatcher.matches((CompilationUnit) node.getRoot())) {
      return false;
    }

    Iterable<String> reverseClassNames = Lists.reverse(classNameElements);
    Iterator<String> reverseClassNamesIterator = reverseClassNames.iterator();
    return matchNested(reverseClassNamesIterator, (AbstractTypeDeclaration) node);
  }

  private boolean matchNested(Iterator<String> reverseClassNamesIterator,
      AbstractTypeDeclaration node) {
    String subClassName = reverseClassNamesIterator.next();
    if (!node.getName().getFullyQualifiedName().equals(subClassName)) {
      return false;
    }
    if (!reverseClassNamesIterator.hasNext()) {
      return true;
    }

    ASTNode parentNode = node.getParent();
    // This won't work with method-declared types. But they're not documented so...?
    if (!(parentNode instanceof AbstractTypeDeclaration)) {
      return false;
    }
    return matchNested(reverseClassNamesIterator, (AbstractTypeDeclaration) parentNode);
  }

  @Override
  public AbstractTypeDeclaration find(CompilationUnit cu) {
    if (!packageMatcher.matches(cu)) {
      return null;
    }

    Iterator<String> classNameIterator = classNameElements.iterator();
    String topLevelClassName = classNameIterator.next();
    for (AbstractTypeDeclaration abstractTypeDeclaration
        : (List<AbstractTypeDeclaration>) cu.types()) {
      if (abstractTypeDeclaration.getName().getFullyQualifiedName().equals(topLevelClassName)) {
        // Top-level interface / class / enum match.
        return findNested(classNameIterator, abstractTypeDeclaration);
      }
    }
    return null;
  }

  @Override public String getStringFormType() {
    return LOCATOR_TYPE_NAME;
  }

  @Override public String getStringFormTarget() {
    return packageMatcher.toStringForm() + "." + Joiner.on('$').join(classNameElements);
  }

  private AbstractTypeDeclaration findNested(Iterator<String> classNameIterator,
      AbstractTypeDeclaration typeDeclaration) {
    if (!classNameIterator.hasNext()) {
      return typeDeclaration;
    }

    String subClassName = classNameIterator.next();
    for (BodyDeclaration bodyDeclaration
        : (List<BodyDeclaration>) typeDeclaration.bodyDeclarations()) {
      if (bodyDeclaration instanceof AbstractTypeDeclaration) {
        AbstractTypeDeclaration subTypeDeclaration = (AbstractTypeDeclaration) bodyDeclaration;
        if (subTypeDeclaration.getName().getFullyQualifiedName().equals(subClassName)) {
          return findNested(classNameIterator, subTypeDeclaration);
        }
      }
    }
    return null;
  }

  @Override
  public String toString() {
    return "TypeLocator{" +
        "packageMatcher=" + packageMatcher +
        ", classNameElements=" + classNameElements +
        '}';
  }

  /** Returns the enclosing type for nested/inner classes, {@code null} otherwise. */
  public static AbstractTypeDeclaration findEnclosingTypeDeclaration(
      AbstractTypeDeclaration typeDeclaration) {
    if (typeDeclaration.isPackageMemberTypeDeclaration()) {
      return null;
    }
    ASTNode enclosingNode = typeDeclaration.getParent();
    return enclosingNode instanceof AbstractTypeDeclaration
        ? (AbstractTypeDeclaration) enclosingNode : null;
  }

  /**
   * Finds the type declaration associated with the supplied {@code bodyDeclaration}. Can
   * return {@code bodyDeclaration} if the node itself is a type declaration. Can return null (e.g.)
   * if the bodyDeclaration is declared on an anonymous type.
   */
  public static AbstractTypeDeclaration findTypeDeclarationNode(BodyDeclaration bodyDeclaration) {
    if (bodyDeclaration instanceof AbstractTypeDeclaration) {
      return (AbstractTypeDeclaration) bodyDeclaration;
    }
    ASTNode parentNode = bodyDeclaration.getParent();
    if (parentNode instanceof AbstractTypeDeclaration) {
      return (AbstractTypeDeclaration) parentNode;
    }
    return null;
  }

  private static List<String> classNameElements(String className) {
    return Splitter.on('$').splitToList(className);
  }
}
