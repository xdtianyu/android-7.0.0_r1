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

package vogar.target;

import com.google.caliper.runner.CaliperMain;
import com.google.common.collect.ImmutableList;
import java.io.PrintWriter;
import vogar.Result;
import vogar.monitor.TargetMonitor;

/**
 * Runs a <a href="http://code.google.com/p/caliper/">Caliper</a> benchmark.
 */
public final class CaliperRunner implements vogar.target.Runner {

    private final TargetMonitor monitor;
    private final boolean profile;
    private final Class<?> testClass;
    private final String[] args;

    public CaliperRunner(TargetMonitor monitor, boolean profile, Class<?> testClass,
                         String[] args) {
        this.monitor = monitor;
        this.profile = profile;
        this.testClass = testClass;
        this.args = args;
    }

    public boolean run(Profiler profiler) {
        monitor.outcomeStarted(getClass(), testClass.getName());
        ImmutableList.Builder<String> builder = ImmutableList.<String>builder()
            .add(testClass.getName())
            .add(args);

        // Make sure that the results are output to the correct location so that vogar will
        // copy them back to the ./vogar-results/ directory.
        builder.add("-Cresults.file.options.dir=" + System.getProperty("java.io.tmpdir"));

        // TODO(paulduffin): Remove once caliper supports suitable defaults for Android.
        // Temporary change to force caliper to use a heap of 256M for each of it's workers when
        // running on Android.
        if (System.getProperty("java.specification.name").equals("Dalvik Core Library")) {
            builder.add("-Cvm.args=-Xmx256M -Xms256M");
        }

        if (profile) {
            // The --dry-run option causes Caliper to run the benchmark once, rather than hundreds
            // if not thousands of times, and to run it in the main Caliper process rather than in
            // a separate Worker process, one for each benchmark run. That is needed when profiling
            // as otherwise the profiler just profiles the main Caliper process rather than the
            // benchmark.
            builder.add("--dry-run");
        }
        ImmutableList<String> argList = builder.build();
        String[] arguments = argList.toArray(new String[argList.size()]);
        try {
            if (profiler != null) {
                profiler.start();
            }
          PrintWriter stdout = new PrintWriter(System.out);
          PrintWriter stderr = new PrintWriter(System.err);
          CaliperMain.exitlessMain(arguments, stdout, stderr);
        } catch (Exception ex) {
            ex.printStackTrace();
        } finally {
            if (profiler != null) {
                profiler.stop();
            }
        }
        monitor.outcomeFinished(Result.SUCCESS);
        return true;
    }
}
