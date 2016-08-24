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

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import vogar.target.Profiler;

public class AndroidProfiler extends Profiler {
    // SamplingProfiler methods
    private final Method newArrayThreadSet;
    private final Method newThreadGroupThreadSet;
    private final Constructor newThreadSet;
    private final Method start;
    private final Method stop;
    private final Method shutdown;
    private final Method write;
    private final Method getHprofData;

    public AndroidProfiler() throws Exception {
        String packageName = "dalvik.system.profiler";
        Class<?> ThreadSet = Class.forName(packageName + ".SamplingProfiler$ThreadSet");
        Class<?> SamplingProfiler = Class.forName(packageName + ".SamplingProfiler");
        Class<?> HprofData = Class.forName(packageName + ".HprofData");
        Class<?> Writer = Class.forName(packageName + ".AsciiHprofWriter");
        newArrayThreadSet = SamplingProfiler.getMethod("newArrayThreadSet", Thread[].class);
        newThreadGroupThreadSet = SamplingProfiler.getMethod("newThreadGroupThreadSet",
                                                             ThreadGroup.class);
        newThreadSet = SamplingProfiler.getConstructor(Integer.TYPE, ThreadSet);
        start = SamplingProfiler.getMethod("start", Integer.TYPE);
        stop = SamplingProfiler.getMethod("stop");
        shutdown = SamplingProfiler.getMethod("shutdown");
        getHprofData = SamplingProfiler.getMethod("getHprofData");
        write = Writer.getMethod("write", HprofData, OutputStream.class);
    }

    private Thread[] thread = new Thread[1];
    private Object profiler;
    private int interval;

    @Override public void setup(boolean profileThreadGroup, int depth, int interval) {
        try {
            Thread t = Thread.currentThread();
            thread[0] = t;
            Object threadSet;
            if (profileThreadGroup) {
                threadSet = newThreadGroupThreadSet.invoke(null, t.getThreadGroup());
            } else {
                threadSet = newArrayThreadSet.invoke(null, (Object)thread);
            }
            this.profiler = newThreadSet.newInstance(depth, threadSet);
            this.interval = interval;
        } catch (Exception e) {
            throw new AssertionError(e);
        }

    }

    @Override public void start() {
        try {
            // If using the array thread set, switch to the current
            // thread.  Sometimes for timeout reasons Runners use
            // separate threads for test execution.
            this.thread[0] = Thread.currentThread();
            start.invoke(profiler, interval);
        } catch (Exception e) {
            throw new AssertionError(e);
        }
    }

    @Override public void stop() {
        try {
            stop.invoke(profiler);
        } catch (Exception e) {
            throw new AssertionError(e);
        }
    }

    @Override public void shutdown(File file) {
        try {
            shutdown.invoke(profiler);

            FileOutputStream out = new FileOutputStream(file);
            write.invoke(null, getHprofData.invoke(profiler), out);
            out.close();
        } catch (Exception e) {
            throw new AssertionError(e);
        }
    }
}
