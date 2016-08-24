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

import com.google.currysrc.api.input.CompoundDirectoryInputFileGenerator;
import com.google.currysrc.api.input.DirectoryInputFileGenerator;
import com.google.currysrc.api.input.FilesInputFileGenerator;
import com.google.currysrc.api.input.InputFileGenerator;
import com.google.currysrc.api.match.SourceMatchers;
import com.google.currysrc.api.output.BasicOutputSourceFileGenerator;
import com.google.currysrc.api.process.DefaultRule;
import com.google.currysrc.api.process.Processor;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Useful chunks of {@link com.google.currysrc.api.Rules} code shared between various tools.
 */
public class Icu4jTransformRules {
  private Icu4jTransformRules() {}

  public static CompoundDirectoryInputFileGenerator createInputFileGenerator(String[] dirNames) {
    List<InputFileGenerator> dirs = new ArrayList<>(dirNames.length);
    for (int i = 0; i < dirNames.length; i++) {
      File inputFile = new File(dirNames[i]);
      InputFileGenerator inputFileGenerator;
      if (isValidDir(inputFile)) {
        inputFileGenerator = new DirectoryInputFileGenerator(inputFile);
      } else if (isValidFile(inputFile)) {
        inputFileGenerator = new FilesInputFileGenerator(inputFile);
      } else {
        throw new IllegalArgumentException("Input arg [" + inputFile + "] does not exist.");
      }
      dirs.add(inputFileGenerator);
    }
    return new CompoundDirectoryInputFileGenerator(dirs);
  }

  public static BasicOutputSourceFileGenerator createOutputFileGenerator(String outputDirName) {
    File outputDir = new File(outputDirName);
    if (!isValidDir(outputDir)) {
      throw new IllegalArgumentException("Output dir [" + outputDir + "] does not exist.");
    }
    return new BasicOutputSourceFileGenerator(outputDir);
  }

  public static DefaultRule createMandatoryRule(Processor processor) {
    return new DefaultRule(processor, SourceMatchers.all(), true /* mustModify */);
  }

  public static DefaultRule createOptionalRule(Processor processor) {
    return new DefaultRule(processor, SourceMatchers.all(), false /* mustModify */);
  }

  private static boolean isValidDir(File dir) {
    return dir.exists() && dir.isDirectory();
  }

  private static boolean isValidFile(File file) {
    return file.exists() && file.isFile();
  }

}
