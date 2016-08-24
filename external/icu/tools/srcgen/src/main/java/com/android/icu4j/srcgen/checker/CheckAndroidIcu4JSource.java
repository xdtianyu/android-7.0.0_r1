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

import com.google.common.collect.Sets;
import com.google.currysrc.Main;
import com.google.currysrc.api.input.InputFileGenerator;

import com.android.icu4j.srcgen.Icu4jTransformRules;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.util.List;
import java.util.Set;

/**
 * Checks that the source in android_icu4j doesn't have obvious problems.
 */
public class CheckAndroidIcu4JSource {

  private static final boolean DEBUG = false;

  private CheckAndroidIcu4JSource() {
  }

  /**
   * Usage:
   * java com.android.icu4j.srcgen.CheckAndroidIcu4JSource {android_icu4j src directories}
   *   {report output file path}
   */
  public static void main(String[] args) throws Exception {
    if (args.length < 2) {
      throw new IllegalArgumentException("At least 2 argument required.");
    }

    Main main = new Main(DEBUG);

    // We assume we only need to look at ICU4J code for this for both passes.
    String[] inputDirs = new String[args.length - 1];
    System.arraycopy(args, 0, inputDirs, 0, inputDirs.length);
    InputFileGenerator inputFileGenerator = Icu4jTransformRules.createInputFileGenerator(inputDirs);

    // Pass 1: Establish the set of classes and members that are public in the Android API.
    System.out.println("Establishing Android public ICU4J API");
    RecordPublicApiRules recordPublicApiRulesRules = new RecordPublicApiRules(inputFileGenerator);
    main.execute(recordPublicApiRulesRules);
    List<String> publicMemberLocatorStrings = recordPublicApiRulesRules.publicMembers();
    System.out.println("Public API is:");
    for (String publicMemberLocatorString : publicMemberLocatorStrings) {
      System.out.println(publicMemberLocatorString);
    }

    // Pass 2: Check for issues.
    System.out.println("Checking for issues");
    Set<String> publicMembersSet = Sets.newHashSet(publicMemberLocatorStrings);
    File outputReportFile = new File(args[args.length - 1]);
    FileWriter out = new FileWriter(outputReportFile, false /* append */);
    try (BufferedWriter reportWriter = new BufferedWriter(out)) {
      reportWriter.append("Beginning of report:\n");
      CheckAndroidIcu4jSourceRules reportRules =
          new CheckAndroidIcu4jSourceRules(inputFileGenerator, publicMembersSet);
      main.execute(reportRules, reportWriter);
      reportWriter.append("End of report\n");
    }

    System.out.println("Report file: " + outputReportFile);
  }

}
