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
package com.android.icu4j.srcgen;

import com.google.common.collect.ImmutableSet;
import com.google.currysrc.processors.BaseJavadocTagJavadoc;

import org.eclipse.jdt.core.dom.Javadoc;
import org.eclipse.jdt.core.dom.TagElement;

import java.util.List;
import java.util.Set;

/**
 * Adds {@literal @}hide to all JavaDoc comments that contain any of {@literal @}draft,
 * {@literal @}provisional, {@literal @}internal}.
 */
public class HideDraftProvisionalInternal extends BaseJavadocTagJavadoc {
  private static final Set<String> toMatch = ImmutableSet.of("@draft", "@provisional", "@internal");
  private static final String HIDE_HIDDEN_ON_ANDROID =
      "@hide draft / provisional / internal are hidden on Android";

  public HideDraftProvisionalInternal() {
    super(HIDE_HIDDEN_ON_ANDROID);
  }

  @Override
  protected boolean mustTag(Javadoc javadoc) {
    for (TagElement tagElement : (List<TagElement>) javadoc.tags()) {
      if (tagElement.getTagName() != null
          && HideDraftProvisionalInternal.toMatch.contains(tagElement.getTagName().toLowerCase())) {
        return true;
      }
    }
    return false;
  }

  @Override public String toString() {
    return "HideDraftProvisionalInternal{}";
  }
}
