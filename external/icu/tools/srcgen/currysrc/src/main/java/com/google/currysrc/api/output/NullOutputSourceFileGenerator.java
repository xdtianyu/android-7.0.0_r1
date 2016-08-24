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

import org.eclipse.jdt.core.dom.CompilationUnit;

import java.io.File;

/**
 * Always returns {@code null}, preventing output file generation.
 */
public final class NullOutputSourceFileGenerator implements OutputSourceFileGenerator {

  /** The instance to use. */
  public final static NullOutputSourceFileGenerator INSTANCE = new NullOutputSourceFileGenerator();

  protected NullOutputSourceFileGenerator() {
  }

  @Override public File generate(CompilationUnit cu, File inputFile) {
    return null;
  }
}
