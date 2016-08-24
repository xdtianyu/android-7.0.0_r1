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
import java.util.List;
import vogar.commands.Command;
import vogar.commands.Mkdir;

public class HostFileCache implements FileCache {
    private final File CACHE_ROOT = new File("/tmp/vogar-md5-cache");

    private final Log log;
    private final Mkdir mkdir;

    public HostFileCache(Log log, Mkdir mkdir) {
        this.log = log;
        this.mkdir = mkdir;
    }

    private void cp(File source, File destination) {
        List<String> rawResult = new Command.Builder(log).args("cp", source, destination).execute();
        // A successful copy returns no results.
        if (!rawResult.isEmpty()) {
            throw new RuntimeException("Couldn't copy " + source + " to " + destination
                    + ": " + rawResult.get(0));
        }
    }

    private void mv(File source, File destination) {
        List<String> rawResult = new Command.Builder(log).args("mv", source, destination).execute();
        // A successful move returns no results.
        if (!rawResult.isEmpty()) {
            throw new RuntimeException("Couldn't move " + source + " to " + destination
                    + ": " + rawResult.get(0));
        }
    }

    public void copyFromCache(String key, File destination) {
        File cachedFile = new File(CACHE_ROOT, key);
        cp(cachedFile, destination);
    }

    public void copyToCache(File source, String key) {
        File cachedFile = new File(CACHE_ROOT, key);
        mkdir.mkdirs(CACHE_ROOT);
        // Copy it onto the same file system first, then atomically move it into place.
        // That way, if we fail, we don't leave anything dangerous lying around.
        File temporary = new File(cachedFile + ".tmp");
        cp(source, temporary);
        mv(temporary, cachedFile);
    }

    public boolean existsInCache(String key) {
        return new File(CACHE_ROOT, key).exists();
    }
}
