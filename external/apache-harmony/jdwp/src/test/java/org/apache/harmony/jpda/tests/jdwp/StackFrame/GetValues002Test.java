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

package org.apache.harmony.jpda.tests.jdwp.StackFrame;

import org.apache.harmony.jpda.tests.framework.jdwp.Value;

/**
 * JDWP Unit test for StackFrame.GetValues command.
 */
public class GetValues002Test extends JDWPStackFrameAccessTest {
    /**
     * Tests we correctly read value of boolean variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues001_Boolean() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.BOOLEAN_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.BOOLEAN_PARAM_VALUE);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointBoolean");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointBoolean");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of byte variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues002_Byte() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.BYTE_SIGNAL);
        Value expectedValue = new Value(StackTrace002Debuggee.BYTE_PARAM_VALUE);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointByte");
        suspensionMethodInfo.addVariable("param", expectedValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointByte");
        methodInfo.addVariable("param", expectedValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of char variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues003_Char() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.CHAR_SIGNAL);
        Value expectedValue = new Value(StackTrace002Debuggee.CHAR_PARAM_VALUE);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointChar");
        suspensionMethodInfo.addVariable("param", expectedValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointChar");
        methodInfo.addVariable("param", expectedValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of short variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues004_Short() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.SHORT_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.SHORT_PARAM_VALUE);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointShort");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointShort");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of int variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues005_Int() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.INT_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointInt");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointInt");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of int variables in the stack: one param and one local.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues005_Int2() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.INT_METHOD2_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE);
        Value nextValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE * 2);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointInt2");
        suspensionMethodInfo.addVariable("param", oldValue, null /* no set value */, nextValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointInt2");
        methodInfo.addVariable("param", oldValue);
        methodInfo.addVariable("local", oldValue, null /* no set value */, nextValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of long variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues006_Long() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.LONG_METHOD_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.LONG_PARAM_VALUE);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointLong");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointLong");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of float variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues007_Float() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.FLOAT_METHOD);
        Value oldValue = new Value(StackTrace002Debuggee.FLOAT_PARAM_VALUE);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointFloat");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointFloat");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of double variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues008_Double() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.DOUBLE_METHOD);
        Value oldValue = new Value(StackTrace002Debuggee.DOUBLE_PARAM_VALUE);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointDouble");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointDouble");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.Object variable 'this' in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues009_ThisObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value thisValue = getStaticFieldValue(classID, "THIS_OBJECT");

        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("this", thisValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("this", thisValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.Object variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues009_Object() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "OBJECT_PARAM_VALUE");

        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of Array variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues010_Array() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "ARRAY_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.ARRAY_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointArray");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointArray");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.Object variable containing
     * Array in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues010_ArrayAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "ARRAY_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.ARRAY_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.Class variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues011_Class() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "CLASS_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.CLASS_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointClass");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointClass");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.Object variable containing
     * java.lang.Class in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues011_ClassAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "CLASS_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.CLASS_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.ClassLoader variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues012_ClassLoader() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "CLASS_LOADER_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.CLASS_LOADER_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointClassLoader");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointClassLoader");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.Object variable containing
     * java.lang.ClassLoader in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues012_ClassLoaderAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "CLASS_LOADER_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.CLASS_LOADER_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.String variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues013_String() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "STRING_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.STRING_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointString");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointString");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.Object variable containing
     * java.lang.String in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues013_StringAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "STRING_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.STRING_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.Thread variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues014_Thread() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "THREAD_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.THREAD_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointThread");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointThread");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.Object variable containing
     * java.lang.Thread in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues014_ThreadAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "THREAD_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.THREAD_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.ThreadGroup variable in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues015_ThreadGroup() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "THREAD_GROUP_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.THREAD_GROUP_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointThreadGroup");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointThreadGroup");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly read value of java.lang.Object variable containing
     * java.lang.ThreadGroup in the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testGetValues015_ThreadGroupAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "THREAD_GROUP_PARAM_VALUE");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.THREAD_GROUP_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }
}
