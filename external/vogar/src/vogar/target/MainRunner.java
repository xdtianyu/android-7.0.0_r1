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

import java.lang.reflect.Method;
import vogar.Result;
import vogar.monitor.TargetMonitor;

/**
 * Runs a Java class with a main method. This includes jtreg tests.
 */
public final class MainRunner implements Runner {

    private final TargetMonitor monitor;
    private final Class<?> mainClass;
    private final String[] args;
    private final Method main;

    public MainRunner(TargetMonitor monitor, Class<?> mainClass, String[] args) {
        this.monitor = monitor;
        this.mainClass = mainClass;
        this.args = args;
        try {
            this.main = mainClass.getMethod("main", String[].class);
        } catch (NoSuchMethodException e) {
            // Don't create a MainRunner without first checking supports().
            throw new IllegalArgumentException(e);
        }
    }

    public boolean run(Profiler profiler) {
        monitor.outcomeStarted(getClass(), mainClass.getName());
        try {
            if (profiler != null) {
                profiler.start();
            }
            main.invoke(null, new Object[] { args });
            monitor.outcomeFinished(Result.SUCCESS);
        } catch (Throwable ex) {
            ex.printStackTrace();
            monitor.outcomeFinished(Result.EXEC_FAILED);
        } finally {
            if (profiler != null) {
                profiler.stop();
            }
        }
        return true;
    }
}
