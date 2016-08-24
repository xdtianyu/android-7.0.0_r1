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
import vogar.Mode;
import vogar.ModeId;
import vogar.SshTarget;
import vogar.Target;
import vogar.Variant;
import vogar.commands.Command;
import vogar.commands.VmCommandBuilder;

import static org.junit.Assert.assertEquals;

/**
 * Test the behaviour of the {@link DeviceRuntime} class when run with {@link SshTarget}.
 */
@RunWith(MockitoJUnitRunner.class)
public class DeviceRuntimeSshTargetTest extends AbstractModeTest {

    @Override
    protected Target createTarget() {
        return new SshTarget(console, "host:99");
    }

    @Test
    @VogarArgs({"action"})
    public void testSshTarget()
            throws IOException {

        Mode deviceRuntime = new DeviceRuntime(run, ModeId.DEVICE, Variant.X32,
                deviceUserNameSupplier);

        VmCommandBuilder builder = newVmCommandBuilder(deviceRuntime)
                .classpath(classpath)
                .mainClass("mainclass")
                .args("-x", "a b");
        Command command = builder.build(run.target);
        List<String> args = command.getArgs();
        assertEquals(Arrays.asList(
                "ssh", "-p", "99", "host", "-C", ""
                        + "cd /work"
                        + " &&"
                        + " ANDROID_DATA=runner"
                        + " dalvikvm32"
                        + " -classpath"
                        + " classes"
                        + " -Duser.home=runner/dir/user.home"
                        + " -Duser.name=fred"
                        + " -Duser.language=en"
                        + " -Duser.region=US"
                        + " -Xcheck:jni"
                        + " -Xjnigreflimit:2000"
                        + " mainclass"
                        + " -x a\\ b"), args);
    }
}
