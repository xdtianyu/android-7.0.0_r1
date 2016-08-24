/*
 * Copyright (C) 2009 The Android Open Source Project
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

import com.google.common.base.Supplier;
import com.google.common.collect.Iterables;
import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import vogar.Action;
import vogar.Variant;
import vogar.Classpath;
import vogar.Mode;
import vogar.ModeId;
import vogar.Run;
import vogar.commands.VmCommandBuilder;
import vogar.tasks.RunActionTask;
import vogar.tasks.Task;

/**
 * Execute actions on an Android device or emulator using "app_process" or the runtime directly.
 */
public final class DeviceRuntime implements Mode {
    private final Run run;
    private final ModeId modeId;
    private final Supplier<String> deviceUserNameSupplier;

    public DeviceRuntime(Run run, ModeId modeId, Variant variant,
                         Supplier<String> deviceUserNameSupplier) {
        this.deviceUserNameSupplier = deviceUserNameSupplier;
        if (!modeId.isDevice() || !modeId.supportsVariant(variant)) {
            throw new IllegalArgumentException("Unsupported mode:" + modeId +
                    " or variant: " + variant);
        }
        this.run = run;
        this.modeId = modeId;
    }

    @Override public Set<Task> installTasks() {
        Set<Task> result = new HashSet<Task>();
        // dex everything on the classpath and push it to the device.
        for (File classpathElement : run.classpath.getElements()) {
            addCreateDexJarAndPushTasks(result, run.basenameOfJar(classpathElement),
                    classpathElement, null);
        }
        return result;
    }

    @Override public Set<Task> installActionTasks(Action action, File jar) {
        Set<Task> result = new HashSet<Task>();
        addCreateDexJarAndPushTasks(result, action.getName(), jar, action);
        return result;
    }

    @Override public Task executeActionTask(Action action, boolean useLargeTimeout) {
        return new RunActionTask(run, action, useLargeTimeout);
    }

    @Override public VmCommandBuilder newVmCommandBuilder(Action action, File workingDirectory) {
        List<String> vmCommand = new ArrayList<String>();
        Iterables.addAll(vmCommand, run.invokeWith());
        vmCommand.add(run.vmCommand);

        // If you edit this, see also HostRuntime...
        VmCommandBuilder vmCommandBuilder = new VmCommandBuilder(run.log)
                .env("ANDROID_DATA", run.getAndroidDataPath())
                .workingDirectory(workingDirectory)
                .vmCommand(vmCommand)
                .vmArgs("-Duser.home=" + run.deviceUserHome)
                .maxLength(1024);
        if (run.debugPort != null) {
            vmCommandBuilder.vmArgs("-Xcompiler-option", "--debuggable");
        }

        if (modeId == ModeId.APP_PROCESS) {
            return vmCommandBuilder
                .vmArgs(action.getUserDir().getPath())
                .classpathViaProperty(true);
        }

        vmCommandBuilder
                .vmArgs("-Duser.name=" + deviceUserNameSupplier.get())
                .vmArgs("-Duser.language=en")
                .vmArgs("-Duser.region=US");

        if (!run.benchmark && run.checkJni) {
            vmCommandBuilder.vmArgs("-Xcheck:jni");
        }
        // dalvikvm defaults to no limit, but the framework sets the limit at 2000.
        vmCommandBuilder.vmArgs("-Xjnigreflimit:2000");
        return vmCommandBuilder;
    }

    @Override public Set<Task> cleanupTasks(Action action) {
        return Collections.singleton(run.target.rmTask(action.getUserDir()));
    }

    @Override public Classpath getRuntimeClasspath(Action action) {
        Classpath result = new Classpath();
        result.addAll(run.targetDexFile(action.getName()));
        if (!run.benchmark) {
            for (File classpathElement : run.classpath.getElements()) {
                result.addAll(run.targetDexFile(run.basenameOfJar(classpathElement)));
            }
        }
        // Note we intentionally do not add run.resourceClasspath on
        // the device since it contains host path names.
        return result;
    }

    private void addCreateDexJarAndPushTasks(
            Set<Task> tasks, String name, File jar, Action action) {
        File localDex = run.localDexFile(name);
        File deviceDex = run.targetDexFile(name);
        Task createDexJarTask = newCreateDexJarTask(run.classpath, jar, name, action, localDex);
        tasks.add(createDexJarTask);
        tasks.add(run.target.pushTask(localDex, deviceDex).afterSuccess(createDexJarTask));
    }

    private Task newCreateDexJarTask(Classpath classpath, File classpathElement, String name,
            Action action, File localDex) {
        Task dex;
        if (run.useJack) {
            dex = new JackDexTask(run, classpath, run.benchmark, name, classpathElement,
                    action, localDex);
        } else {
            dex = new DexTask(run.androidSdk, classpath, run.benchmark, name, classpathElement,
                    action, localDex);
        }
        return dex;
    }
}
