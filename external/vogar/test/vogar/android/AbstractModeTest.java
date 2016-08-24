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

import com.google.common.base.Joiner;
import com.google.common.base.Supplier;
import java.io.File;
import java.io.IOException;
import junit.framework.AssertionFailedError;
import org.junit.Before;
import org.junit.Rule;
import org.mockito.Mock;
import vogar.Action;
import vogar.Classpath;
import vogar.Console;
import vogar.HostFileCache;
import vogar.Mode;
import vogar.Run;
import vogar.Target;
import vogar.Vogar;
import vogar.commands.Mkdir;
import vogar.commands.Rm;
import vogar.commands.VmCommandBuilder;

/**
 * Base class for tests of {@link Mode} implementations.
 */
public abstract class AbstractModeTest {

    @Rule
    public VogarArgsRule vogarArgsRule = new VogarArgsRule();

    @Mock protected Console console;

    protected Mkdir mkdir;

    protected Rm rm;

    protected AndroidSdk androidSdk;

    protected Classpath classpath;

    protected Run run;

    protected Supplier<String> deviceUserNameSupplier = new Supplier<String>() {
        @Override
        public String get() {
            return "fred";
        }
    };

    @Before
    public void setUp() throws IOException {
        mkdir = new Mkdir(console);
        rm = new Rm(console);

        androidSdk = new AndroidSdk(console, mkdir,
                new File[] {new File("classpath")}, "android.jar",
                new HostFileCache(console, mkdir));
        Target target = createTarget();

        final Vogar vogar = new Vogar();
        String[] args = vogarArgsRule.getTestSpecificArgs();
        if (!vogar.parseArgs(args)) {
            throw new AssertionFailedError("Parse error in: " + Joiner.on(",").join(args)
                    + ". Please check stdout.");
        }

        run = new Run(vogar, false, console, mkdir, androidSdk, new Rm(console), target,
                new File("runner/dir"));

        classpath = new Classpath();
        classpath.addAll(new File("classes"));
    }

    protected abstract Target createTarget();

    protected VmCommandBuilder newVmCommandBuilder(Mode mode) {
        Action action = new Action("action", "blah", new File("resources"), new File("source"),
                new File("java"));
        return mode.newVmCommandBuilder(action, new File("/work"));
    }
}
