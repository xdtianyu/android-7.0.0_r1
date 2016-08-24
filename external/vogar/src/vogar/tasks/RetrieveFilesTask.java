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

package vogar.tasks;

import java.io.File;
import java.io.FileFilter;
import java.io.FileNotFoundException;
import vogar.Result;
import vogar.Run;

public final class RetrieveFilesTask extends Task {
    private final Run run;
    private final File deviceFile;

    public RetrieveFilesTask(Run run, File deviceFile) {
        super("retrieve files " + deviceFile);
        this.run = run;
        this.deviceFile = deviceFile;
    }

    @Override protected Result execute() throws Exception {
        retrieveFiles(new File("./vogar-results"), deviceFile, run.retrievedFiles);
        return Result.SUCCESS;
    }

    /**
     * Scans {@code dir} for files to grab.
     */
    private void retrieveFiles(File destination, File source, FileFilter filenameFilter)
            throws FileNotFoundException {
        for (File file : run.target.ls(source)) {
            if (filenameFilter.accept(file)) {
                run.log.info("Moving " + file + " to " + destination);
                run.mkdir.mkdirs(destination);
                run.target.pull(file, destination);
            }
        }
    }
}
