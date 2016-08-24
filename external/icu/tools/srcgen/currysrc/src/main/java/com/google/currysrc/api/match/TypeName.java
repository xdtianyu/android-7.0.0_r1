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
package com.google.currysrc.api.match;

/**
 * The name of a type: a class or an enum. The className is expected to contain $ to indicate
 * nested / inner classes.
 */
public final class TypeName {

  private final String packageName;

  private final String className;

  public TypeName(String packageName, String className) {
    this.packageName = packageName;
    this.className = className;
  }

  public String packageName() {
    return packageName;
  }

  public String className() {
    return className;
  }

  public static TypeName fromFullyQualifiedClassName(String fullyQualifiedClassName) {
    int packageSeparatorIndex = fullyQualifiedClassName.lastIndexOf('.');
    String packageName;
    String className;
    if (packageSeparatorIndex == -1) {
      packageName = "";
      className = fullyQualifiedClassName;
    } else {
      packageName = fullyQualifiedClassName.substring(0, packageSeparatorIndex);
      className = fullyQualifiedClassName.substring(packageSeparatorIndex + 1);
    }
    return new TypeName(packageName, className);
  }
}
