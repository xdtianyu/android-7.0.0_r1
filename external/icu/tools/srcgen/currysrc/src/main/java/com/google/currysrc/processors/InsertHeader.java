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

import com.google.currysrc.api.process.Context;
import com.google.currysrc.api.process.Processor;

import org.eclipse.jdt.core.dom.CompilationUnit;
import org.eclipse.jface.text.BadLocationException;
import org.eclipse.jface.text.Document;

/**
 * Inserts a header to the beginning of a {@link CompilationUnit}.
 */
public class InsertHeader implements Processor {

  private final String header;

  public InsertHeader(String header) {
    this.header = header;
  }

  @Override
  public void process(Context context, CompilationUnit cu) {
    Document document = context.document();
    try {
      document.replace(0, 0, header);
    } catch (BadLocationException e) {
      throw new AssertionError(e);
    }
  }

  @Override
  public String toString() {
    return "InsertHeader";
  }
}
