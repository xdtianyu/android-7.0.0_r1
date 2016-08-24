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

package vogar.commands;

import java.io.File;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import vogar.Classpath;
import vogar.Log;
import vogar.Target;

import static com.google.common.base.Preconditions.checkNotNull;

/**
 * Builds a virtual machine command.
 */
public final class VmCommandBuilder {
    private final Log log;
    private File temp;
    private boolean classpathViaProperty;
    private Classpath bootClasspath = new Classpath();
    private Classpath classpath = new Classpath();
    private File userDir;
    private Integer debugPort;
    private String mainClass;
    private PrintStream output;
    private int maxLength = -1;
    private List<String> vmCommand = Collections.singletonList("java");
    private List<String> vmArgs = new ArrayList<String>();
    private List<String> args = new ArrayList<String>();
    private File workingDirectory;
    private Map<String, String> env = new LinkedHashMap<String, String>();

    public VmCommandBuilder(Log log) {
        this.log = log;
    }

    public VmCommandBuilder vmCommand(List<String> vmCommand) {
        this.vmCommand = new ArrayList<String>(vmCommand);
        return this;
    }

    /**
     * Set the working directory of the target process.
     */
    public VmCommandBuilder workingDirectory(File workingDirectory) {
        this.workingDirectory = workingDirectory;
        return this;
    }

    public VmCommandBuilder temp(File temp) {
        this.temp = temp;
        return this;
    }

    public VmCommandBuilder bootClasspath(Classpath bootClasspath) {
        this.bootClasspath.addAll(bootClasspath);
        return this;
    }

    public VmCommandBuilder classpath(Classpath classpath) {
        this.classpath.addAll(classpath);
        return this;
    }

    public VmCommandBuilder classpathViaProperty(boolean classpathViaProperty) {
        this.classpathViaProperty = classpathViaProperty;
        return this;
    }

    /**
     * The user dir on the target. This directory might not exist on the
     * local disk.
     */
    public VmCommandBuilder userDir(File userDir) {
        this.userDir = userDir;
        return this;
    }

    /**
     * Add a setting for an environment variable in the target process.
     */
    public VmCommandBuilder env(String key, String value) {
        env.put(key, value);
        return this;
    }

    public VmCommandBuilder debugPort(Integer debugPort) {
        this.debugPort = debugPort;
        return this;
    }

    public VmCommandBuilder mainClass(String mainClass) {
        this.mainClass = mainClass;
        return this;
    }

    public VmCommandBuilder output(PrintStream output) {
        this.output = output;
        return this;
    }

    public VmCommandBuilder maxLength(int maxLength) {
        this.maxLength = maxLength;
        return this;
    }

    public VmCommandBuilder vmArgs(String... vmArgs) {
        return vmArgs(Arrays.asList(vmArgs));
    }

    public VmCommandBuilder vmArgs(Collection<String> vmArgs) {
        this.vmArgs.addAll(vmArgs);
        return this;
    }

    public VmCommandBuilder args(String... args) {
        return args(Arrays.asList(args));
    }

    public VmCommandBuilder args(Collection<String> args) {
        this.args.addAll(args);
        return this;
    }

    public Command build(Target target) {
        // Make sure that the main class to run has been specified.
        checkNotNull(mainClass, "mainClass may not be null");

        Target.ScriptBuilder builder = target.newScriptBuilder();

        if (workingDirectory != null) {
            builder.workingDirectory(workingDirectory);
        }

        builder.env(env);

        builder.tokens(vmCommand);
        if (classpathViaProperty) {
            builder.tokens("-Djava.class.path=" + classpath);
        } else {
            builder.tokens("-classpath", classpath.toString());
        }
        // Only output this if there's something on the boot classpath,
        // otherwise dalvikvm gets upset.
        if (!bootClasspath.isEmpty()) {
            builder.tokens("-Xbootclasspath/a:" + bootClasspath);
        }
        if (userDir != null) {
            builder.tokens("-Duser.dir=" + userDir);
        }

        if (temp != null) {
            builder.tokens("-Djava.io.tmpdir=" + temp);
        }

        if (debugPort != null) {
            builder.tokens("-Xrunjdwp:transport=dt_socket,address="
                    + debugPort + ",server=y,suspend=y");
        }

        builder.tokens(vmArgs);
        builder.tokens(mainClass);
        builder.tokens(args);

        return new Command.Builder(log)
                .args(builder.commandLine())
                .tee(output)
                .maxLength(maxLength)
                .build();
    }
}
