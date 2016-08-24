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

package vogar.commands;

import com.google.common.collect.Lists;

import java.io.File;
import java.util.Collection;
import java.util.List;

import vogar.Log;
import vogar.util.Strings;

/**
 * Runs the Jack compiler to generate dex files.
 */
public class Jack {
    private static final File JACK_SCRIPT;
    private static final File JACK_JAR;

    // Initialise the files for jack and jill, letting them be null if the files
    // cannot be found.
    static {
        String sdkTop = System.getenv("ANDROID_BUILD_TOP");

        final File jackScript = new File(sdkTop + "/prebuilts/sdk/tools/jack");
        final File jackJar = new File(sdkTop + "/prebuilts/sdk/tools/jack.jar");

        // If the system environment variable JACK_JAR is set then use that,
        // otherwise find the jar relative to the AOSP source.
        String jackJarEnv = System.getenv("JACK_JAR");

        final File jackJarFromEnv = (jackJarEnv != null) ? new File(jackJarEnv) : null;

        if (!jackScript.exists()) {
            JACK_SCRIPT = null;
        } else {
            JACK_SCRIPT = jackScript;
        }

        if (jackJarEnv != null && jackJarFromEnv.exists()) {
            JACK_JAR = jackJarFromEnv;
        } else {
            if (!jackJar.exists()) {
                JACK_JAR = null;
            } else {
                JACK_JAR = jackJar;
            }
        }
    }

    /**
     * Get an instance of the jack command with appropriate path settings.
     *
     * @return an instance of a jack command with appropriate paths to its dependencies if needed.
     * @throws IllegalStateException when the jack command cannot be found.
     */
    public static Jack getJackCommand(Log log) throws IllegalStateException {
        if (JACK_SCRIPT != null) {
            // Configure jack compiler with right JACK_SCRIPT path.
            return new Jack(log, Lists.newArrayList(JACK_SCRIPT.getAbsolutePath()));
        }
        if (JACK_JAR != null) {
            // Fallback to jack.jar, for previous releases.
            return new Jack(log, Lists.newArrayList("java", "-jar", JACK_JAR.getAbsolutePath()));
        }
        throw new IllegalStateException("Jack library not found, cannot use jack.");
    }

    private final Command.Builder builder;

    private Jack(Log log, Collection<String> jackArgs) {
        this.builder = new Command.Builder(log);
        builder.args(jackArgs);
    }

    public Jack importFile(String path) {
        builder.args("--import", path);
        return this;
    }

    public Jack importMeta(String dir) {
        builder.args("--import-meta", dir);
        return this;
    }

    public Jack importResource(String dir) {
        builder.args("--import-resource", dir);
        return this;
    }

    public Jack incrementalFolder(String dir) {
        builder.args("--incremental--folder", dir);
        return this;
    }

    public Jack multiDex(String mode) {
        builder.args("--multi-dex", mode);
        return this;
    }

    public Jack sourceVersion(String version) {
        setProperty("jack.java.source.version=" + version);
        return this;
    }

    public Jack minApiLevel(String minApiLevel) {
        setProperty("jack.android.min-api-level=" + minApiLevel);
        return this;
    }

    public Jack outputDex(String dir) {
        builder.args("--output-dex", dir);
        return this;
    }

    public Jack outputDexZip(String zipFile) {
        builder.args("--output-dex-zip", zipFile);
        return this;
    }

    public Jack outputJack(String path) {
        builder.args("--output-jack", path);
        return this;
    }

    public Jack processor(String names) {
        builder.args("--processor", names);
        return this;
    }

    public Jack processorPath(String path) {
        builder.args("--processorpath", path);
        return this;
    }

    public Jack verbose(String mode) {
        builder.args("--verbose", mode);
        return this;
    }

    public Jack addAnnotationProcessor(String processor) {
        builder.args("-A", processor);
        return this;
    }

    public Jack setProperty(String property) {
        builder.args("-D", property);
        return this;
    }

    public Jack setClassPath(String classPath) {
        builder.args("-cp", classPath);
        return this;
    }

    public Jack setDebug() {
        builder.args("-g");
        return this;
    }

    public Jack setEnvVar(String key, String value) {
        builder.env(key, value);
        return this;
    }

    /**
     * Runs the command with the preconfigured options on Jack, and returns the outcome.
     *
     * @return A list of output lines from running the command.
     */
    public List<String> invoke() {
        return builder.execute();
    }

    /**
     * Runs the command with the preconfigured options on Jack, and returns the outcome.
     * This method does not dirty the existing Jack instance, and can be safely reused
     * to compile other files.
     * @param files The files to compile.
     * @return A list of output lines from running the command.
     */
    public List<String> compile(Collection<File> files) {
        return new Command.Builder(builder)
                .args((Object[]) Strings.objectsToStrings(files))
                .execute();
    }
}
