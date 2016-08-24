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

import com.google.common.collect.ImmutableList;
import com.google.common.collect.Lists;
import com.google.currysrc.Main;
import com.google.currysrc.api.Rules;
import com.google.currysrc.api.input.InputFileGenerator;
import com.google.currysrc.api.output.NullOutputSourceFileGenerator;
import com.google.currysrc.api.output.OutputSourceFileGenerator;
import com.google.currysrc.api.process.Context;
import com.google.currysrc.api.process.Processor;
import com.google.currysrc.api.process.Rule;
import com.google.currysrc.api.process.ast.BodyDeclarationLocators;
import com.google.currysrc.api.process.ast.TypeLocator;

import org.eclipse.jdt.core.dom.AbstractTypeDeclaration;
import org.eclipse.jdt.core.dom.BodyDeclaration;
import org.eclipse.jdt.core.dom.CompilationUnit;
import org.eclipse.jdt.core.dom.EnumConstantDeclaration;
import org.eclipse.jdt.core.dom.Javadoc;
import org.eclipse.jdt.core.dom.TagElement;
import org.eclipse.jdt.core.dom.TypeDeclaration;

import java.io.File;
import java.lang.reflect.Modifier;
import java.util.Collections;
import java.util.List;

/**
 * Generates text that can be injected into Icu4JTransform for describing source elements that
 * should be hidden because they are deprecated. Only intended for use in capturing the
 * <em>initial set</em> of ICU elements to be hidden. Typically, anything that ICU deprecates in
 * future should remain public until they can safely be removed from Android's public APIs.
 */
public class CaptureDeprecatedElements {

  private static final boolean DEBUG = true;

  private static final String ANDROID_ICU_PREFIX = "android.icu.";
  private static final String ORIGINAL_ICU_PREFIX = "com.ibm.icu.";

  private CaptureDeprecatedElements() {
  }

  /**
   * Usage:
   * java com.android.icu4j.srcgen.CaptureDeprecatedMethods {one or more source directories}
   */
  public static void main(String[] args) throws Exception {
    CaptureDeprecatedMethodsRules rules = new CaptureDeprecatedMethodsRules(args);
    new Main(DEBUG).execute(rules);
    List<String> deprecatedElements = rules.getCaptureRule().getDeprecatedElements();

    // ASCII order for easier maintenance of the source this goes into.
    List<String> sortedDeprecatedElements = Lists.newArrayList(deprecatedElements);
    Collections.sort(sortedDeprecatedElements);
    for (String entry : sortedDeprecatedElements) {
      String entryInAndroid = entry.replace(ORIGINAL_ICU_PREFIX, ANDROID_ICU_PREFIX);
      System.out.println("      \"" + entryInAndroid + "\",");
    }
  }

  private static class CaptureDeprecatedMethodsRules implements Rules {

    private final InputFileGenerator inputFileGenerator;

    private final CaptureDeprecatedProcessor captureTransformer;

    public CaptureDeprecatedMethodsRules(String[] args) {
      if (args.length < 1) {
        throw new IllegalArgumentException("At least 1 argument required.");
      }
      inputFileGenerator = Icu4jTransformRules.createInputFileGenerator(args);

      ImmutableList.Builder<TypeLocator> apiClassesWhitelistBuilder = ImmutableList.builder();
      for (String publicClassName : Icu4jTransform.PUBLIC_API_CLASSES) {
        String originalIcuClassName = publicClassName.replace(ANDROID_ICU_PREFIX,
            ORIGINAL_ICU_PREFIX);
        apiClassesWhitelistBuilder.add(new TypeLocator(originalIcuClassName));
      }
      captureTransformer = new CaptureDeprecatedProcessor(apiClassesWhitelistBuilder.build());
    }

    @Override
    public List<Rule> getRuleList(File file) {
      return Lists.<Rule>newArrayList(
          Icu4jTransformRules.createOptionalRule(captureTransformer));
    }

    @Override
    public InputFileGenerator getInputFileGenerator() {
      return inputFileGenerator;
    }

    @Override
    public OutputSourceFileGenerator getOutputSourceFileGenerator() {
      return NullOutputSourceFileGenerator.INSTANCE;
    }

    public CaptureDeprecatedProcessor getCaptureRule() {
      return captureTransformer;
    }
  }

  private static class CaptureDeprecatedProcessor implements Processor {

    private final List<TypeLocator> publicClassLocators;
    private final List<String> deprecatedElements = Lists.newArrayList();

    public CaptureDeprecatedProcessor(List<TypeLocator> publicClassLocators) {
      this.publicClassLocators = publicClassLocators;
    }

    @Override public void process(Context context, CompilationUnit cu) {
      for (TypeLocator publicClassLocator : publicClassLocators) {
        AbstractTypeDeclaration matchedType = publicClassLocator.find(cu);
        if (matchedType != null) {
          if (isDeprecated(matchedType)) {
            List<String> locatorStrings = BodyDeclarationLocators.toLocatorStringForms(matchedType);
            deprecatedElements.addAll(locatorStrings);
          }
          trackDeprecationsRecursively(matchedType);
        }
      }
    }

    private void trackDeprecationsRecursively(AbstractTypeDeclaration matchedType) {
      for (BodyDeclaration bodyDeclaration
          : (List<BodyDeclaration>) matchedType.bodyDeclarations()) {
        if (isApiVisible(matchedType, bodyDeclaration) && isDeprecated(bodyDeclaration)) {
          deprecatedElements.addAll(BodyDeclarationLocators.toLocatorStringForms(bodyDeclaration));
          if (bodyDeclaration instanceof AbstractTypeDeclaration) {
            trackDeprecationsRecursively((AbstractTypeDeclaration) bodyDeclaration);
          }
        }
      }
    }

    private boolean isApiVisible(
        AbstractTypeDeclaration matchedType, BodyDeclaration bodyDeclaration) {
      // public members and those that might be inherited are ones that might show up in the APIs.
      if (matchedType instanceof TypeDeclaration) {
        TypeDeclaration typeDeclaration = (TypeDeclaration) matchedType;
        if (typeDeclaration.isInterface()) {
          // Interface declarations are public regardless of whether they are explicitly marked as
          // such.
          return true;
        }
      }
      if (bodyDeclaration instanceof EnumConstantDeclaration) {
        return true;
      }

      return (bodyDeclaration.getModifiers() & (Modifier.PROTECTED | Modifier.PUBLIC)) > 0;
    }

    private static boolean isDeprecated(BodyDeclaration bodyDeclaration) {
      Javadoc doc = bodyDeclaration.getJavadoc();
      // This only checks for the @deprecated javadoc tag, not the java.lang.Deprecated annotation.
      if (doc != null) {
        for (TagElement tag : (List<TagElement>) doc.tags()) {
          if (tag.getTagName() != null && tag.getTagName().equalsIgnoreCase("@deprecated")) {
            return true;
          }
        }
      }
      return false;
    }

    public List<String> getDeprecatedElements() {
      return deprecatedElements;
    }
  }
}
