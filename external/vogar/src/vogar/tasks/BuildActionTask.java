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
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Collections;
import java.util.HashSet;
import java.util.Properties;
import java.util.Set;
import java.util.regex.Pattern;

import com.google.common.collect.Sets;

import vogar.Action;
import vogar.Classpath;
import vogar.Driver;
import vogar.Mode;
import vogar.Outcome;
import vogar.Result;
import vogar.Run;
import vogar.TestProperties;
import vogar.commands.Command;
import vogar.commands.CommandFailedException;
import vogar.commands.Jack;
import vogar.commands.Javac;

/**
 * Compiles classes for the given action and makes them ready for execution.
 */
public final class BuildActionTask extends Task {
    private static final Pattern JAVA_SOURCE_PATTERN = Pattern.compile("\\/(\\w)+\\.java$");

    private final Action action;
    private final Run run;
    private final Driver driver;
    private final File outputFile;

    public BuildActionTask(Run run, Action action, Driver driver, File outputFile) {
        super("build " + action.getName());
        this.run = run;
        this.action = action;
        this.driver = driver;
        this.outputFile = outputFile;
    }

    @Override protected Result execute() throws Exception {
        try {
            if (run.useJack) {
                compileWithJack(action, outputFile);
            } else {
                compile(action, outputFile);
            }
            return Result.SUCCESS;
        } catch (CommandFailedException e) {
            driver.addEarlyResult(new Outcome(action.getName(), Result.COMPILE_FAILED,
                    e.getOutputLines()));
            return Result.COMPILE_FAILED;
        } catch (IOException e) {
            driver.addEarlyResult(new Outcome(action.getName(), Result.ERROR, e));
            return Result.ERROR;
        }
    }

    /**
     * Returns the .jar file containing the action's compiled classes.
     *
     * @throws CommandFailedException if javac fails
     */
    private void compile(Action action, File jar) throws IOException {
        File classesDir = run.localFile(action, "classes");
        run.mkdir.mkdirs(classesDir);
        createJarMetadataFiles(action, classesDir);

        Set<File> sourceFiles = new HashSet<File>();
        File javaFile = action.getJavaFile();
        Javac javac = new Javac(run.log, run.javaPath("javac"));
        if (run.debugging) {
            javac.debug();
        }
        if (javaFile != null) {
            if (!JAVA_SOURCE_PATTERN.matcher(javaFile.toString()).find()) {
                throw new CommandFailedException(Collections.<String>emptyList(),
                        Collections.singletonList("Cannot compile: " + javaFile));
            }
            sourceFiles.add(javaFile);
            Classpath sourceDirs = Classpath.of(action.getSourcePath());
            sourceDirs.addAll(run.sourcepath);
            javac.sourcepath(sourceDirs.getElements());
        }
        if (!sourceFiles.isEmpty()) {
            if (!run.buildClasspath.isEmpty()) {
                javac.bootClasspath(run.buildClasspath);
            }
            javac.classpath(run.classpath)
                    .destination(classesDir)
                    .javaVersion(run.language.getJavacSourceAndTarget())
                    .extra(run.javacArgs)
                    .compile(sourceFiles);
        }

        new Command(run.log, run.javaPath("jar"), "cvfM", jar.getPath(),
                "-C", classesDir.getPath(), "./").execute();
    }



    /**
     * Compile sources using the Jack compiler.
     */
    private void compileWithJack(Action action, File jackFile) throws IOException {
        // Create a folder for resources.
        File resourcesDir = run.localFile(action, "resources");
        run.mkdir.mkdirs(resourcesDir);
        createJarMetadataFiles(action, resourcesDir);

        File javaFile = action.getJavaFile();
        Jack compiler = Jack.getJackCommand(run.log);

        if (run.debugging) {
            compiler.setDebug();
        }
        compiler.sourceVersion(run.language.getJackSourceVersion());
        compiler.minApiLevel(String.valueOf(run.language.getJackMinApilevel()));
        Set<File> sourceFiles = Sets.newHashSet();

        // Add the source files to be compiled.
        // The javac compiler supports the -sourcepath directive although jack
        // does not have this (see b/22382563) so for now only the files given
        // are actually compiled.
        if (javaFile != null) {
            if (!JAVA_SOURCE_PATTERN.matcher(javaFile.toString()).find()) {
                throw new CommandFailedException(Collections.<String>emptyList(),
                        Collections.singletonList("There is no source to compile here: "
                                + javaFile));
            }
            sourceFiles.add(javaFile);
        }

        // Compile if there is anything to compile.
        if (!sourceFiles.isEmpty()) {
            if (!run.buildClasspath.isEmpty()) {
                compiler.setClassPath(run.buildClasspath.toString() + ":"
                        + run.classpath.toString());
            }
        }

        compiler.outputJack(jackFile.getPath())
                .importResource(resourcesDir.getPath())
                .compile(sourceFiles);
    }

    /**
     * Writes files to {@code classesDir} to be included in the .jar file for
     * {@code action}.
     */
    private void createJarMetadataFiles(Action action, File classesDir) throws IOException {
        OutputStream propertiesOut
                = new FileOutputStream(new File(classesDir, TestProperties.FILE));
        Properties properties = new Properties();
        fillInProperties(properties, action);
        properties.store(propertiesOut, "generated by " + Mode.class.getName());
        propertiesOut.close();
    }

    /**
     * Fill in properties for running in this mode
     */
    private void fillInProperties(Properties properties, Action action) {
        properties.setProperty(TestProperties.TEST_CLASS_OR_PACKAGE, action.getTargetClass());
        properties.setProperty(TestProperties.QUALIFIED_NAME, action.getName());
        properties.setProperty(TestProperties.MONITOR_PORT, Integer.toString(run.firstMonitorPort));
        properties.setProperty(TestProperties.TIMEOUT, Integer.toString(run.timeoutSeconds));
        properties.setProperty(TestProperties.PROFILE, Boolean.toString(run.profile));
        properties.setProperty(TestProperties.PROFILE_DEPTH, Integer.toString(run.profileDepth));
        properties.setProperty(TestProperties.PROFILE_INTERVAL,
                Integer.toString(run.profileInterval));
        properties.setProperty(TestProperties.PROFILE_FILE, run.profileFile.getName());
        properties.setProperty(TestProperties.PROFILE_THREAD_GROUP,
                Boolean.toString(run.profileThreadGroup));
        properties.setProperty(TestProperties.TEST_ONLY, Boolean.toString(run.testOnly));
    }
}
