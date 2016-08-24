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

import com.google.currysrc.api.match.SourceMatcher;

import org.eclipse.jdt.core.dom.CompilationUnit;

/**
 * Default implementation of {@link Rule} that delegates to a {@link Processor} for the
 * actual processing.
 */
public class DefaultRule implements Rule {

  protected final SourceMatcher matcher;

  protected final boolean mustModify;

  protected final Processor processor;

  public DefaultRule(Processor processor, SourceMatcher matcher, boolean mustModify) {
    this.matcher = matcher;
    this.mustModify = mustModify;
    this.processor = processor;
  }

  @Override public void process(Context context, CompilationUnit cu) {
    processor.process(context, cu);
  }

  @Override
  public boolean matches(CompilationUnit cu) {
    return matcher.matches(cu);
  }

  @Override
  public boolean mustModify() {
    return mustModify;
  }
}
