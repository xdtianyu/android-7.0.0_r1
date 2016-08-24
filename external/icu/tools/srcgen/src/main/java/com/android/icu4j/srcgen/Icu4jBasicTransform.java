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

import com.google.common.collect.Lists;
import com.google.currysrc.Main;
import com.google.currysrc.api.Rules;
import com.google.currysrc.api.input.InputFileGenerator;
import com.google.currysrc.api.output.OutputSourceFileGenerator;
import com.google.currysrc.api.process.Rule;
import com.google.currysrc.processors.ReplaceTextCommentScanner;

import java.io.File;
import java.util.List;

import static com.android.icu4j.srcgen.Icu4jTransformRules.createOptionalRule;

/**
 * Applies basic Android's ICU4J source code transformation rules to code and fixes up the
 * jcite start/end tags so they can be used with doclava.
 *
 * <p>Intended for use when transforming sample code.
 */
public class Icu4jBasicTransform {

  private static final boolean DEBUG = false;

  private Icu4jBasicTransform() {
  }

  /**
   * Usage:
   * java com.android.icu4j.srcgen.Icu4jSampleTransform {source files/directories} {target dir}
   */
  public static void main(String[] args) throws Exception {
    new Main(DEBUG).execute(new Icu4jBasicRules(args));
  }

  private static class Icu4jBasicRules implements Rules {

    private final InputFileGenerator inputFileGenerator;

    private final List<Rule> rules;

    private final OutputSourceFileGenerator outputSourceFileGenerator;

    public Icu4jBasicRules(String[] args) {
      if (args.length < 2) {
        throw new IllegalArgumentException("At least 2 arguments required.");
      }

      String[] inputDirNames = new String[args.length - 1];
      System.arraycopy(args, 0, inputDirNames, 0, args.length - 1);
      inputFileGenerator = Icu4jTransformRules.createInputFileGenerator(inputDirNames);
      rules = createTransformRules();
      outputSourceFileGenerator = Icu4jTransformRules.createOutputFileGenerator(
          args[args.length - 1]);
    }

    @Override
    public List<Rule> getRuleList(File ignored) {
      return rules;
    }

    @Override
    public InputFileGenerator getInputFileGenerator() {
      return inputFileGenerator;
    }

    @Override
    public OutputSourceFileGenerator getOutputSourceFileGenerator() {
      return outputSourceFileGenerator;
    }

    private static List<Rule> createTransformRules() {
      List<Rule> rules =
          Lists.newArrayList(Icu4jTransform.Icu4jRules.getRepackagingRules());

      // Switch all embedded comment references from com.ibm.icu to android.icu.
      rules.add(
          createOptionalRule(new ReplaceTextCommentScanner(
              Icu4jTransform.ORIGINAL_ICU_PACKAGE, Icu4jTransform.ANDROID_ICU_PACKAGE)));

      // Change sample jcite begin / end tags ---XYZ to Androids 'BEGIN(XYZ)' / 'END(XYZ)'
      rules.add(createOptionalRule(new TranslateJcite.BeginEndTagsHandler()));

      return rules;
    }
  }
}
