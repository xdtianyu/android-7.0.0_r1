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

package vogar.android;

import com.google.common.collect.Iterables;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import vogar.Action;
import vogar.Classpath;
import vogar.Mode;
import vogar.ModeId;
import vogar.Run;
import vogar.Variant;
import vogar.commands.VmCommandBuilder;
import vogar.tasks.MkdirTask;
import vogar.tasks.RunActionTask;
import vogar.tasks.Task;

/**
 * Executes actions on a Dalvik or ART runtime on a Linux desktop.
 */
public final class HostRuntime implements Mode {
    private final Run run;
    private final ModeId modeId;
    private final Variant variant;

    public HostRuntime(Run run, ModeId modeId, Variant variant) {
        if (!modeId.isHost() || !modeId.supportsVariant(variant)) {
            throw new IllegalArgumentException("Unsupported mode:" + modeId +
                    " or variant: " + variant);
        }
        this.run = run;
        this.modeId = modeId;
        this.variant = variant;
    }

    @Override public Task executeActionTask(Action action, boolean useLargeTimeout) {
        return new RunActionTask(run, action, useLargeTimeout);
    }

    private File dalvikCache() {
        return run.localFile("android-data", run.dalvikCache);
    }

    @Override public Set<Task> installTasks() {
        Set<Task> result = new HashSet<Task>();
        for (File classpathElement : run.classpath.getElements()) {
            // Libraries need to be dex'ed and put in the temporary directory.
            String name = run.basenameOfJar(classpathElement);
            File localDex = run.localDexFile(name);
            result.add(createCreateDexJarTask(run.classpath, classpathElement, name,
                    null /* action */, localDex));
        }
        result.add(new MkdirTask(run.mkdir, dalvikCache()));
        return result;
    }

    @Override public Set<Task> cleanupTasks(Action action) {
        return Collections.emptySet();
    }

    @Override public Set<Task> installActionTasks(Action action, File jar) {
        File localDexFile = run.localDexFile(action.getName());
        Task createDexJarTask = createCreateDexJarTask(Classpath.of(jar), jar, action.getName(),
                action, localDexFile);
        return Collections.singleton(createDexJarTask);
    }

    @Override public VmCommandBuilder newVmCommandBuilder(Action action, File workingDirectory) {
        String hostOut = System.getenv("ANDROID_HOST_OUT");
        if (hostOut == null || hostOut.length() == 0) {
          hostOut = System.getenv("ANDROID_BUILD_TOP");
          if (hostOut == null) {
            hostOut = "";
          } else {
            hostOut += "/";
          }
          hostOut += "out/host/linux-x86";
        }

        List<File> jars = new ArrayList<File>();
        for (String jar : modeId.getJarNames()) {
            jars.add(new File(hostOut, "framework/" + jar + ".jar"));
        }
        Classpath bootClasspath = Classpath.of(jars);

        String libDir = hostOut;
        if (variant == Variant.X32) {
            libDir += "/lib";
        } else if (variant == Variant.X64) {
            libDir += "/lib64";
        } else {
            throw new AssertionError("Unsupported variant:" + variant);
        }

        List<String> vmCommand = new ArrayList<String>();
        Iterables.addAll(vmCommand, run.invokeWith());
        vmCommand.add(hostOut + "/bin/" + run.vmCommand);

        // If you edit this, see also DeviceRuntime...
        VmCommandBuilder builder = new VmCommandBuilder(run.log)
                .env("ANDROID_PRINTF_LOG", "tag")
                .env("ANDROID_LOG_TAGS", "*:i")
                .env("ANDROID_DATA", dalvikCache().getParent())
                .env("ANDROID_ROOT", hostOut)
                .env("LD_LIBRARY_PATH", libDir)
                .env("DYLD_LIBRARY_PATH", libDir)
                // This is needed on the host so that the linker loads core.oat at the necessary
                // address.
                .env("LD_USE_LOAD_BIAS", "1")
                .vmCommand(vmCommand)
                .vmArgs("-Xbootclasspath:" + bootClasspath.toString())
                .vmArgs("-Duser.language=en")
                .vmArgs("-Duser.region=US");
        if (run.debugPort != null) {
            builder.vmArgs("-Xcompiler-option", "--debuggable");
        }
        if (!run.benchmark && run.checkJni) {
            builder.vmArgs("-Xcheck:jni");
        }
        // dalvikvm defaults to no limit, but the framework sets the limit at 2000.
        builder.vmArgs("-Xjnigreflimit:2000");
        return builder;
    }

    @Override public Classpath getRuntimeClasspath(Action action) {
        Classpath result = new Classpath();
        result.addAll(run.localDexFile(action.getName()));
        for (File classpathElement : run.classpath.getElements()) {
            result.addAll(run.localDexFile(run.basenameOfJar(classpathElement)));
        }
        result.addAll(run.resourceClasspath);
        return result;
    }

    private Task createCreateDexJarTask(Classpath classpath, File classpathElement, String name,
            Action action, File localDex) {
        Task dex;
        if (run.useJack) {
            dex = new JackDexTask(run, classpath, run.benchmark, name, classpathElement, action,
                    localDex);
        } else {
            dex = new DexTask(run.androidSdk, classpath, run.benchmark, name, classpathElement,
                    action, localDex);
        }
        return dex;
    }
}
