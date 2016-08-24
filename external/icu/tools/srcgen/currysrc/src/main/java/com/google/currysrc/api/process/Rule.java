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

import org.eclipse.jdt.core.dom.CompilationUnit;

/**
 * The interface for rules. Rules can be matched against a {@link CompilationUnit} and, if applied,
 * can indicate whether they must have made a modification to be considered successful.
 */
public interface Rule {

  /**
   * Returns {@code true} if this rule should be applied to the supplied {@link CompilationUnit}.
   */
  boolean matches(CompilationUnit cu);

  /**
   * Process the supplied {@link CompilationUnit}. If making changes, Rules must not modify
   * the {@code compilationUnit} directly: they must request an appropriate tool from the
   * {@code context}. Only one modification tool can be requested from {@code context}.
   */
  void process(Context context, CompilationUnit cu);

  /** Returns {@code true} if the rule must make a modification to be considered successful. */
  boolean mustModify();
}
