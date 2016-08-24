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

import com.google.currysrc.api.process.ast.BodyDeclarationLocator;

import org.eclipse.jdt.core.dom.CompilationUnit;

/**
 * Useful factory methods for {@link SourceMatcher} instances.
 */
public final class SourceMatchers {

  private static final SourceMatcher ALL_MATCHER = new SourceMatcher() {
    @Override
    public boolean matches(CompilationUnit cu) {
      return true;
    }

    @Override public String toString() {
      return "{match all}";
    }
  };

  private SourceMatchers() {
  }

  /** Returns a {@link SourceMatcher} that matches any {@link CompilationUnit}. */
  public static SourceMatcher all() {
    return ALL_MATCHER;
  }

  /**
   * Returns a {@link SourceMatcher} that matches a {@link CompilationUnit} if it contains the
   * body declaration located by {@code locator}.
   */
  public static SourceMatcher contains(final BodyDeclarationLocator locator) {
    return new SourceMatcher() {
      @Override public boolean matches(CompilationUnit cu) {
        return locator.find(cu) != null;
      }
    };
  }

}
