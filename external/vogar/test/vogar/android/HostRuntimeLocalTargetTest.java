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

package vogar.android;

import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;
import vogar.LocalTarget;
import vogar.Mode;
import vogar.ModeId;
import vogar.Target;
import vogar.Variant;
import vogar.commands.Command;
import vogar.commands.VmCommandBuilder;

import static org.junit.Assert.assertEquals;

/**
 * Test the behaviour of the {@link HostRuntime} class when run with {@link LocalTarget}.
 */
@RunWith(MockitoJUnitRunner.class)
public class HostRuntimeLocalTargetTest extends AbstractModeTest {

    @Override
    protected Target createTarget() {
        return new LocalTarget(console, mkdir, rm);
    }

    @Test
    @VogarArgs({"action"})
    public void testLocalTarget()
            throws IOException {

        Mode hostRuntime = new HostRuntime(run, ModeId.HOST, Variant.X32);

        VmCommandBuilder builder = newVmCommandBuilder(hostRuntime)
                .classpath(classpath)
                .mainClass("mainclass")
                .args("-x", "a b");
        Command command = builder.build(run.target);
        List<String> args = command.getArgs();
        assertEquals(Arrays.asList(
                "sh", "-c", ""
                        + "ANDROID_PRINTF_LOG=tag"
                        + " ANDROID_LOG_TAGS=*:i"
                        + " ANDROID_DATA=" + run.localFile("android-data")
                        + " ANDROID_ROOT=out/host/linux-x86"
                        + " LD_LIBRARY_PATH=out/host/linux-x86/lib"
                        + " DYLD_LIBRARY_PATH=out/host/linux-x86/lib"
                        + " LD_USE_LOAD_BIAS=1"
                        + " out/host/linux-x86/bin/dalvikvm32"
                        + " -classpath classes"
                        + " -Xbootclasspath"
                        + ":out/host/linux-x86/framework/core-libart-hostdex.jar"
                        + ":out/host/linux-x86/framework/conscrypt-hostdex.jar"
                        + ":out/host/linux-x86/framework/okhttp-hostdex.jar"
                        + ":out/host/linux-x86/framework/bouncycastle-hostdex.jar"
                        + " -Duser.language=en"
                        + " -Duser.region=US"
                        + " -Xcheck:jni"
                        + " -Xjnigreflimit:2000"
                        + " mainclass"
                        + " -x a\\ b"), args);
    }

    @Test
    @VogarArgs({"--benchmark", "action"})
    public void testLocalTarget_Benchmark()
            throws IOException {

        Mode hostRuntime = new HostRuntime(run, ModeId.HOST, Variant.X32);

        VmCommandBuilder builder = newVmCommandBuilder(hostRuntime)
                .classpath(classpath)
                .mainClass("mainclass")
                .args("-x", "a b");
        Command command = builder.build(run.target);
        List<String> args = command.getArgs();
        assertEquals(Arrays.asList(
                "sh", "-c", ""
                        + "ANDROID_PRINTF_LOG=tag"
                        + " ANDROID_LOG_TAGS=*:i"
                        + " ANDROID_DATA=" + run.localFile("android-data")
                        + " ANDROID_ROOT=out/host/linux-x86"
                        + " LD_LIBRARY_PATH=out/host/linux-x86/lib"
                        + " DYLD_LIBRARY_PATH=out/host/linux-x86/lib"
                        + " LD_USE_LOAD_BIAS=1"
                        + " out/host/linux-x86/bin/dalvikvm32"
                        + " -classpath classes"
                        + " -Xbootclasspath"
                        + ":out/host/linux-x86/framework/core-libart-hostdex.jar"
                        + ":out/host/linux-x86/framework/conscrypt-hostdex.jar"
                        + ":out/host/linux-x86/framework/okhttp-hostdex.jar"
                        + ":out/host/linux-x86/framework/bouncycastle-hostdex.jar"
                        + " -Duser.language=en"
                        + " -Duser.region=US"
                        + " -Xjnigreflimit:2000"
                        + " mainclass"
                        + " -x a\\ b"), args);
    }
}
