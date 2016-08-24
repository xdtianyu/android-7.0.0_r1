/*
 * Copyright (C) 2010 The Android Open Source Project
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

package vogar;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Stores and presents information about jars the user may have forgotten to include.
 */
public final class JarSuggestions {
    private final Set<File> allSuggestedJars = new HashSet<File>();

    public Set<File> getAllSuggestedJars() {
        return allSuggestedJars;
    }

    public void addSuggestions(JarSuggestions jarSuggestions) {
        allSuggestedJars.addAll(jarSuggestions.getAllSuggestedJars());
    }

    public void addSuggestionsFromOutcome(Outcome outcome, ClassFileIndex classFileIndex,
            Classpath classpath) {
        Result result = outcome.getResult();
        if (result != Result.COMPILE_FAILED && result != Result.EXEC_FAILED) {
            return;
        }
        Set<File> suggestedJars = classFileIndex.suggestClasspaths(outcome.getOutput());
        // don't suggest adding a jar that's already on the classpath
        suggestedJars.removeAll(classpath.getElements());

        allSuggestedJars.addAll(suggestedJars);
    }

    public List<String> getStringList() {
        List<String> jarStringList = new ArrayList<String>();
        for (File jar : allSuggestedJars) {
            jarStringList.add(jar.getPath());
        }
        Collections.sort(jarStringList);
        return jarStringList;
    }
}
