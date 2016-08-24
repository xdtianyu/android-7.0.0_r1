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
package com.google.currysrc.api.input;

import com.google.common.collect.Iterables;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * An {@link InputFileGenerator} that can combine the output of other {@link InputFileGenerator}
 * instances.
 */
public final class CompoundDirectoryInputFileGenerator implements InputFileGenerator {

  private List<InputFileGenerator> generators;

  public CompoundDirectoryInputFileGenerator(List<InputFileGenerator> fileGenerators) {
    this.generators = fileGenerators;
  }

  @Override public Iterable<? extends File> generate() {
    List<Iterable<? extends File>> iterables = new ArrayList<>(generators.size());
    for (InputFileGenerator generator : generators) {
      iterables.add(generator.generate());
    }
    return Iterables.concat(iterables);
  }
}
