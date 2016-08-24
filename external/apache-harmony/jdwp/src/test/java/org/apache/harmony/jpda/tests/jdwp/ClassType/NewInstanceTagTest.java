/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package org.apache.harmony.jpda.tests.jdwp.ClassType;

import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Value;

import java.util.List;

/**
 * JDWP unit test for ClassType.NewInstance command for particular reference
 * types.
 *
 * <p>Note:
 * <ul>
 *  <li>we do not test JT_CLASS because we cannot create java.lang.Class objects.</li>
 *  <li>We do not test JT_ARRAY because arrays are created with ArrayType.NewInstance.</li>
 * </ul></p>
 */
public class NewInstanceTagTest extends AbstractNewInstanceTestCase {

    @Override
    protected String getDebuggeeClassName() {
        return NewInstanceTagDebuggee.class.getName();
    }

    /**
     * Test ClassType.NewInstance of java.lang.Object returns JT_OBJECT tag.
     */
    public void testNewInstance_Object() {
        checkNewInstanceTag("Ljava/lang/Object;", "()V", new NoConstructorArgumentProvider(),
                JDWPConstants.Tag.OBJECT_TAG);
    }

    /**
     * Test ClassType.NewInstance of a subclass of java.lang.Object returns JT_OBJECT tag.
     */
    public void testNewInstance_MyObject() {
        String subclassSig = getClassSignature(NewInstanceTagDebuggee.MyObject.class);
        checkNewInstanceTag(subclassSig, "()V", new NoConstructorArgumentProvider(),
                JDWPConstants.Tag.OBJECT_TAG);
    }

    /**
     * Test ClassType.NewInstance of java.lang.String returns JT_STRING tag.
     */
    public void testNewInstance_String() {
        checkNewInstanceTag("Ljava/lang/String;", "()V", new NoConstructorArgumentProvider(),
                JDWPConstants.Tag.STRING_TAG);
    }

    /**
     * Test ClassType.NewInstance of a subclass of java.lang.ClassLoader returns
     * JT_CLASS_LOADER tag.
     *
     * Note: we use a subclass only because java.lang.ClassLoader is an abstract
     * class.
     */
    public void testNewInstance_ClassLoader() {
        String subclassSig = getClassSignature(NewInstanceTagDebuggee.MyClassLoader.class);
        checkNewInstanceTag(subclassSig, "()V", new NoConstructorArgumentProvider(),
                JDWPConstants.Tag.CLASS_LOADER_TAG);
    }

    /**
     * Test ClassType.NewInstance of java.lang.Thread returns JT_THREAD tag.
     */
    public void testNewInstance_Thread() {
        checkNewInstanceTag("Ljava/lang/Thread;", "()V", new NoConstructorArgumentProvider(),
                JDWPConstants.Tag.THREAD_TAG);
    }

    /**
     * Test ClassType.NewInstance of a subclass of java.lang.Thread returns
     * JT_THREAD tag.
     */
    public void testNewInstance_MyThread() {
        String subclassSig = getClassSignature(NewInstanceTagDebuggee.MyThread.class);
        checkNewInstanceTag(subclassSig, "()V", new NoConstructorArgumentProvider(),
                JDWPConstants.Tag.THREAD_TAG);
    }

    /**
     * Test ClassType.NewInstance of java.lang.ThreadGroup returns
     * JT_THREAD_GROUP tag.
     */
    public void testNewInstance_ThreadGroup() {
        checkNewInstanceTag("Ljava/lang/ThreadGroup;", "(Ljava/lang/String;)V",
                new ConstructorArgumentsProvider() {
                    @Override
                    public void provideConstructorArguments(List<Value> constructorArguments) {
                        // Create string "foo".
                        long stringId = debuggeeWrapper.vmMirror.createString("foo");
                        assertTrue("Invalid string id", stringId != -1);
                        assertTrue("Null string id", stringId != 0);
                        // Pass created string to constructor.
                        constructorArguments.add(new Value(JDWPConstants.Tag.STRING_TAG, stringId));
                    }
                }, JDWPConstants.Tag.THREAD_GROUP_TAG);
    }

    /**
     * Test ClassType.NewInstance of a subclass of java.lang.ThreadGroup returns
     * JT_THREAD_GROUP tag.
     */
    public void testNewInstance_MyThreadGroup() {
        String subclassSig = getClassSignature(NewInstanceTagDebuggee.MyThreadGroup.class);
        checkNewInstanceTag(subclassSig, "()V", new NoConstructorArgumentProvider(),
                JDWPConstants.Tag.THREAD_GROUP_TAG);
    }
}
