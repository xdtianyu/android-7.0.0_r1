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
package com.google.currysrc.api.process;

import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;
import org.eclipse.jface.text.Document;

/**
 * Enables a {@link Processor} to request what it needs to support transformation and/or reporting.
 * Only one method of {@link #rewrite()} or {@link #document()} can be called.
 */
public interface Context {
  /** Returns an ASTRewrite for AST modifications. */
  ASTRewrite rewrite();

  /** Returns a Document for direct text manipulation. */
  Document document();

  /** Returns an object that can be used to report information or warnings. */
  Reporter reporter();
}
