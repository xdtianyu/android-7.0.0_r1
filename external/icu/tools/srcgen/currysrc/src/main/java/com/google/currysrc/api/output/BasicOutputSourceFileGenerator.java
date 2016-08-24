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
package com.google.currysrc.api.output;

import com.google.currysrc.api.process.ast.PackageMatcher;

import org.eclipse.jdt.core.dom.CompilationUnit;

import java.io.File;

/**
 * Generate the output source file name from a CompilationUnit's package information.
 */
public final class BasicOutputSourceFileGenerator implements OutputSourceFileGenerator {

  private final File baseDir;

  public BasicOutputSourceFileGenerator(File baseDir) {
    this.baseDir = baseDir;
  }

  @Override
  public File generate(CompilationUnit cu, File inputFile) {
    String sourceFileName = inputFile.getName();
    String fqn = PackageMatcher.getPackageName(cu);
    String packageSubDir = fqn.replace(".", File.separator);
    return new File(new File(baseDir, packageSubDir), sourceFileName);
  }
}
