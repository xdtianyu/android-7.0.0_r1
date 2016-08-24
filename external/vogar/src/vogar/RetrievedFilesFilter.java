/*
 * Copyright (C) 2011 The Android Open Source Project
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
import java.io.FileFilter;

/**
 * Selects files to be kept from a test run.
 */
public final class RetrievedFilesFilter implements FileFilter {
    private final boolean profile;
    private final File profileFile;

    public RetrievedFilesFilter(boolean profile, File profileFile) {
        this.profile = profile;
        this.profileFile = profileFile;
    }

    @Override public boolean accept(File file) {
        if (file.getName().equals("prefs.xml")) {
            return false;
        }
        if (file.getName().endsWith(".xml")
                || file.getName().equals("caliper-results")
                || file.getName().endsWith(".json")
                || (profile && file.getName().equals(profileFile.getName()))) {
            return true;
        }
        return false;
    }
}
