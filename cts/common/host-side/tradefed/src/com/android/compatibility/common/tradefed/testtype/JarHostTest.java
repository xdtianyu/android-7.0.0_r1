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
package com.android.compatibility.common.tradefed.testtype;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.testtype.HostTest;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IRuntimeHintProvider;
import com.android.tradefed.util.TimeVal;

import junit.framework.Test;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Modifier;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

/**
 * Test runner for host-side JUnit tests.
 */
public class JarHostTest extends HostTest implements IAbiReceiver, IBuildReceiver,
        IRuntimeHintProvider {

    @Option(name="jar", description="The jars containing the JUnit test class to run.",
            importance = Importance.IF_UNSET)
    private Set<String> mJars = new HashSet<>();

    @Option(name = "runtime-hint",
            isTimeVal = true,
            description="The hint about the test's runtime.")
    private long mRuntimeHint = 60000;// 1 minute

    private IAbi mAbi;
    private IBuildInfo mBuild;
    private CompatibilityBuildHelper mHelper;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo build) {
        mBuild = build;
        mHelper = new CompatibilityBuildHelper(build);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public long getRuntimeHint() {
        return mRuntimeHint;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected List<Class<?>> getClasses() throws IllegalArgumentException  {
        List<Class<?>> classes = super.getClasses();
        for (String jarName : mJars) {
            JarFile jarFile = null;
            try {
                File file = new File(mHelper.getTestsDir(), jarName);
                jarFile = new JarFile(file);
                Enumeration<JarEntry> e = jarFile.entries();
                URL[] urls = {
                        new URL(String.format("jar:file:%s!/", file.getAbsolutePath()))
                };
                URLClassLoader cl = URLClassLoader.newInstance(urls);

                while (e.hasMoreElements()) {
                    JarEntry je = e.nextElement();
                    if (je.isDirectory() || !je.getName().endsWith(".class")
                            || je.getName().contains("$")) {
                        continue;
                    }
                    String className = getClassName(je.getName());
                    try {
                        Class<?> cls = cl.loadClass(className);
                        int modifiers = cls.getModifiers();
                        if ((IRemoteTest.class.isAssignableFrom(cls)
                                || Test.class.isAssignableFrom(cls))
                                && !Modifier.isStatic(modifiers)
                                && !Modifier.isPrivate(modifiers)
                                && !Modifier.isProtected(modifiers)
                                && !Modifier.isInterface(modifiers)
                                && !Modifier.isAbstract(modifiers)) {
                            classes.add(cls);
                        }
                    } catch (ClassNotFoundException cnfe) {
                        throw new IllegalArgumentException(
                                String.format("Cannot find test class %s", className));
                    }
                }
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                if (jarFile != null) {
                    try {
                        jarFile.close();
                    } catch (IOException e) {
                        // Ignored
                    }
                }
            }
        }
        return classes;
    }

    private static String getClassName(String name) {
        // -6 because of .class
        return name.substring(0, name.length() - 6).replace('/', '.');
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected Object loadObject(Class<?> classObj) throws IllegalArgumentException {
        Object object = super.loadObject(classObj);
        if (object instanceof IAbiReceiver) {
            ((IAbiReceiver) object).setAbi(mAbi);
        }
        if (object instanceof IBuildReceiver) {
            ((IBuildReceiver) object).setBuild(mBuild);
        }
        return object;
    }
}
