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
package com.android.icu4j.srcgen.checker;

import com.google.common.collect.Lists;
import com.google.currysrc.api.Rules;
import com.google.currysrc.api.input.InputFileGenerator;
import com.google.currysrc.api.output.NullOutputSourceFileGenerator;
import com.google.currysrc.api.output.OutputSourceFileGenerator;
import com.google.currysrc.api.process.Rule;

import java.io.File;
import java.util.List;
import java.util.Set;

import static com.android.icu4j.srcgen.Icu4jTransformRules.createOptionalRule;

/**
 * Rules for processing ICU4J source and looking for problems.
 */
class CheckAndroidIcu4jSourceRules implements Rules {

  private final InputFileGenerator inputFileGenerator;

  private final List<Rule> rules;

  public CheckAndroidIcu4jSourceRules(InputFileGenerator inputFileGenerator,
      Set<String> publicMembers) {

    this.inputFileGenerator = inputFileGenerator;
    // Add more as we find issues.
    rules = Lists.<Rule>newArrayList(
        createOptionalRule(new CheckForBrokenJciteTag(publicMembers))
    );
  }

  @Override
  public List<Rule> getRuleList(File file) {
    return rules;
  }

  @Override
  public InputFileGenerator getInputFileGenerator() {
    return inputFileGenerator;
  }

  @Override
  public OutputSourceFileGenerator getOutputSourceFileGenerator() {
    return NullOutputSourceFileGenerator.INSTANCE;
  }
}
