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
package vogar.target;

import com.google.caliper.runner.BenchmarkClassChecker;
import com.google.caliper.util.InvalidCommandException;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;
import javax.annotation.Nullable;
import vogar.monitor.TargetMonitor;

/**
 * Supports treating a class as a Caliper benchmark.
 */
public class CaliperRunnerFactory implements RunnerFactory {

    @Nullable
    private final BenchmarkClassChecker benchmarkClassChecker;

    public CaliperRunnerFactory(List<String> argsList) {
        BenchmarkClassChecker benchmarkClassChecker;
        try {
            // Command line arguments can affect the set of available instruments so pass that
            // information on to the checker. Unfortunately, at this point we do not know whether
            // the arguments are actually valid for Caliper, if they are not then this will fail.
            // If that happens then we simply have to assume that the classes being tested are not
            // Caliper benchmarks.
            benchmarkClassChecker = BenchmarkClassChecker.create(argsList);
        } catch (InvalidCommandException e) {
            // The arguments are invalid for Caliper.
            System.out.println("Warning: Arguments are invalid for Caliper: " + e.getMessage());
            benchmarkClassChecker = null;
        }

        this.benchmarkClassChecker = benchmarkClassChecker;
    }

    @Override @Nullable
    public Runner newRunner(TargetMonitor monitor, String qualification,
            Class<?> klass, AtomicReference<String> skipPastReference,
            TestEnvironment testEnvironment, int timeoutSeconds, boolean profile,
            String[] args) {
        if (benchmarkClassChecker != null && benchmarkClassChecker.isBenchmark(klass)) {
            return new CaliperRunner(monitor, profile, klass, args);
        } else {
            return null;
        }
    }
}
