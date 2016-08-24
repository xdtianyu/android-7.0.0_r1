/*
 * Copyright (C) 2016 The Android Open Source Project
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

package vogar.android;

import java.io.File;
import java.util.LinkedList;

import vogar.Action;
import vogar.Classpath;
import vogar.Md5Cache;
import vogar.Result;
import vogar.Run;
import vogar.commands.Command;
import vogar.commands.Jack;
import vogar.tasks.Task;

/**
 * Generates .dex.jar files using Jack.
 */
public final class JackDexTask extends Task {
    private final Run run;
    private final Classpath classpath;
    private final boolean benchmark;
    private final File inputFile;
    private final Action action;
    private final File localDex;

    public JackDexTask(Run run, Classpath classpath, boolean benchmark, String name,
            File inputFile, Action action, File localDex) {
        super("jackdex " + name);
        this.run = run;
        this.classpath = classpath;
        this.benchmark = benchmark;
        this.inputFile = inputFile;
        this.action = action;
        this.localDex = localDex;
    }

    @Override protected Result execute() throws Exception {
        run.mkdir.mkdirs(localDex.getParentFile());

        // What we do depends on the nature of the nature of inputFile. Ultimately we want a
        // .dex.jar containing a classes.dex and resources.
        // For the inputFile we might be given:
        // 1) A .jack file: We must convert it to a dex.jar. Jack will handle including resources
        // for us.
        // 2) A .jar file containing .class files: We must ask Jack to convert it to a .dex.jar and
        // we have to handle resources.

        // There are several things below that seem fishy and could be improved with a different
        // task workflow:
        // 1) Having to deal with multiple classpath entries for inclusion: if the purpose is
        // to convert a .jack or .jar to a .dex.jar file we *may* not need supporting classes.
        // 2) The resource inclusion behavior is almost certainly incorrect and may need a change in
        // Jack if we persist with including the entire classpath (2).

        // Check the jack cache first.
        Md5Cache jackCache = run.jackCache;
        String classpathSubKey = jackCache.makeKey(classpath);
        String cacheKey = null;
        if (classpathSubKey != null) {
            // If the classpath contains things that are not .jar or .jack files we get null
            // classpathSubKey and do not cache.

            // cacheKey includes all the arguments that could affect the output.
            cacheKey = jackCache.makeKey(inputFile.toString(), classpathSubKey,
                run.language.toString(), Boolean.toString(run.debugging));

            if (jackCache.getFromCache(localDex, cacheKey)) {
                run.log.verbose("JackDexTask: Obtained " + localDex + " from jackCache");
                return Result.SUCCESS;
            }
        }
        run.log.verbose("JackDexTask: Could not obtain " + localDex + " from jackCache");

        Jack jack = Jack.getJackCommand(run.log).outputDexZip(localDex.getPath());
        jack.sourceVersion(run.language.getJackSourceVersion());
        jack.minApiLevel(String.valueOf(run.language.getJackMinApilevel()));
        if (run.debugging) {
            jack.setDebug();
        }

        // Jack imports resources from .jack files but not .jar files. We keep track of the .jar
        // files so we can unpack them in reverse order (to maintain classpath ordering).
        // Unfortunately, the inconsistency between .jack and .jar behavior makes it incorrect
        // in some cases.
        LinkedList<File> resourcesReverseClasspath = new LinkedList<>();
        addClassPathEntryToJack(jack, resourcesReverseClasspath, inputFile);
        if (benchmark && action != null) {
            for (File classpathElement : classpath.getElements()) {
                addClassPathEntryToJack(jack, resourcesReverseClasspath, classpathElement);
            }
        }

        if (!resourcesReverseClasspath.isEmpty()) {
            File resourcesDir = run.localFile(localDex.getName() + "_resources");
            run.mkdir.mkdirs(resourcesDir);
            // Unpack each classpath entry into resourcesDir.
            for (File classpathEntry : resourcesReverseClasspath) {
                unpackJar(classpathEntry, resourcesDir);
            }
            jack.importResource(resourcesDir.getPath());
        }
        jack.invoke();

        if (cacheKey != null) {
            // Store the result of the jack invocation in the jackCache.
            jackCache.insert(cacheKey, localDex);
        }

        return Result.SUCCESS;
    }

    private static void addClassPathEntryToJack(Jack jack,
            LinkedList<File> resourcesReverseClasspath, File classpathElement) {
        jack.importFile(classpathElement.getPath());
        if (!classpathElement.getName().endsWith(".jack")) {
            resourcesReverseClasspath.addFirst(classpathElement);
        }
    }

    private void unpackJar(File classpathEntry, File resourcesDir) {
        new Command.Builder(run.log)
                .workingDir(resourcesDir)
                .args(run.javaPath("jar"), "xf", classpathEntry.getPath())
                .execute();
    }
}
