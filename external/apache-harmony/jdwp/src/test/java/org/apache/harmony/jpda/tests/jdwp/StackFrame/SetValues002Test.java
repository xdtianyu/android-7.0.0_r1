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
 * JDWP Unit test for StackFrame.SetValues command.
 */
public class SetValues002Test extends JDWPStackFrameAccessTest {
    /**
     * Tests we correctly write value of boolean variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues001_Boolean() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.BOOLEAN_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.BOOLEAN_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.BOOLEAN_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointBoolean");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointBoolean");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of byte variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues002_Byte() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.BYTE_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.BYTE_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.BYTE_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointByte");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointByte");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of char variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues003_Char() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.CHAR_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.CHAR_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.CHAR_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointChar");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointChar");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of short variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues004_Short() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.SHORT_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.SHORT_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.SHORT_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointShort");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointShort");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of int variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues005_Int() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.INT_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointInt");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointInt");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of int variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues005_IntConstant() {
        StackFrameTester tester = new StackFrameTester(
                StackTrace002Debuggee.INT_CONSTANT_METHOD_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointIntConstant");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointIntConstant");
        methodInfo.addVariable("local", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write values of two int variables from the same frame into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues005_IntTwoConstants() {
        StackFrameTester tester = new StackFrameTester(
                StackTrace002Debuggee.INT_TWO_CONSTANTS_METHOD_SIGNAL);
        Value oldValueForLocal1 = new Value(StackTrace002Debuggee.INT_PARAM_VALUE);
        Value newValueForLocal1 = new Value(StackTrace002Debuggee.INT_PARAM_VALUE_TO_SET);
        Value oldValueForLocal2 = new Value(-StackTrace002Debuggee.INT_PARAM_VALUE);
        Value newValueForLocal2 = new Value(-StackTrace002Debuggee.INT_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointIntTwoConstants");
        suspensionMethodInfo.addVariable("param1", oldValueForLocal1, newValueForLocal1);
        suspensionMethodInfo.addVariable("param2", oldValueForLocal2, newValueForLocal2);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointIntTwoConstants");
        methodInfo.addVariable("local1", oldValueForLocal1, newValueForLocal1);
        methodInfo.addVariable("local2", oldValueForLocal2, newValueForLocal2);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of int variable into the stack even if an exception has
     * been thrown then caught in the meantime.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues005_IntConstantWithException() {
        StackFrameTester tester = new StackFrameTester(
                StackTrace002Debuggee.INT_CONSTANT_METHOD_WITH_EXCEPTION_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo =
                tester.addTestMethod("breakpointIntConstantWithException");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointIntConstantWithException");
        methodInfo.addVariable("local", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of int variable into the stack even if an exception has
     * been thrown then caught in the callee.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues005_IntConstantWithExceptionInCallee() {
        StackFrameTester tester = new StackFrameTester(
                StackTrace002Debuggee.INT_CONSTANT_METHOD_WITH_EXCEPTION_IN_CALLEE_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo =
                tester.addTestMethod("breakpointIntConstantWithException");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo =
                tester.addTestMethod("runBreakpointIntConstantWithExceptionInCallee");
        methodInfo.addVariable("local", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of int variable into the stack even if an exception has
     * been thrown then caught in the callee.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues005_IntConstantWithExceptionInCaller() {
        StackFrameTester tester = new StackFrameTester(
                StackTrace002Debuggee.INT_CONSTANT_METHOD_WITH_EXCEPTION_IN_CALLER_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE_TO_SET);
        // Throwing an exception will unwind the frame where we set the value. So we expect to
        // read the initial value on second suspension.
        Value expectedValueAfterSet = oldValue;
        MethodInfo suspensionMethodInfo =
                tester.addTestMethod("breakpointIntConstantWithException");
        suspensionMethodInfo.addVariable("param", oldValue, newValue, expectedValueAfterSet);
        MethodInfo methodInfo =
                tester.addTestMethod("runBreakpointIntConstantWithExceptionInCallerImpl");
        methodInfo.addVariable("local", oldValue, newValue, expectedValueAfterSet);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of int variable into the stack even if an exception has
     * been thrown then caught in the callee.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues005_IntConstantWithExceptionAndNativeTransition() {
        StackFrameTester tester = new StackFrameTester(
                StackTrace002Debuggee.INT_CONSTANT_METHOD_WITH_EXCEPTION_FROM_NATIVE_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.INT_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo =
                tester.addTestMethod("breakpointIntConstantWithException");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo =
                tester.addTestMethod("runBreakpointIntConstantWithExceptionAndNativeTransition");
        methodInfo.addVariable("local", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of long variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues006_Long() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.LONG_METHOD_SIGNAL);
        Value oldValue = new Value(StackTrace002Debuggee.LONG_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.LONG_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointLong");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointLong");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of float variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues007_Float() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.FLOAT_METHOD);
        Value oldValue = new Value(StackTrace002Debuggee.FLOAT_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.FLOAT_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointFloat");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointFloat");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of double variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues008_Double() {
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.DOUBLE_METHOD);
        Value oldValue = new Value(StackTrace002Debuggee.DOUBLE_PARAM_VALUE);
        Value newValue = new Value(StackTrace002Debuggee.DOUBLE_PARAM_VALUE_TO_SET);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointDouble");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointDouble");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.Object variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues009_Object() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "OBJECT_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "OBJECT_PARAM_VALUE_TO_SET");

        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.Object variable into the stack even if an
     * exception has been thrown then caught in the meantime.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues009_ObjectWithException() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "OBJECT_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "OBJECT_PARAM_VALUE_TO_SET");

        StackFrameTester tester =
                new StackFrameTester(StackTrace002Debuggee.OBJECT_WITH_EXCEPTION_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObjectWithException");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObjectWithException");
        methodInfo.addVariable("local", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.Object variable into the stack even if an
     * exception has been thrown then caught in the callee.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues009_ObjectConstantWithExceptionInCallee() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "OBJECT_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "OBJECT_PARAM_VALUE_TO_SET");

        StackFrameTester tester = new StackFrameTester(
                StackTrace002Debuggee.OBJECT_METHOD_WITH_EXCEPTION_IN_CALLEE_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObjectWithException");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo =
                tester.addTestMethod("runBreakpointObjectWithExceptionInCallee");
        methodInfo.addVariable("local", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.Object variable into the stack even if an
     * exception has been thrown then caught in the callee.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues009_ObjectConstantWithExceptionInCaller() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "OBJECT_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "OBJECT_PARAM_VALUE_TO_SET");

        StackFrameTester tester = new StackFrameTester(
                StackTrace002Debuggee.OBJECT_METHOD_WITH_EXCEPTION_IN_CALLER_SIGNAL);
        // Throwing an exception will unwind the frame where we set the value. So we expect to
        // read the initial value on second suspension.
        Value expectedValueAfterSet = oldValue;
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObjectWithException");
        suspensionMethodInfo.addVariable("param", oldValue, newValue, expectedValueAfterSet);
        MethodInfo methodInfo =
                tester.addTestMethod("runBreakpointObjectWithExceptionInCallerImpl");
        methodInfo.addVariable("local", oldValue, newValue, expectedValueAfterSet);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.Object variable into the stack even if an
     * exception has been thrown then caught in the callee.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues009_ObjectConstantWithExceptionAndNativeTransition() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "OBJECT_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "OBJECT_PARAM_VALUE_TO_SET");

        StackFrameTester tester = new StackFrameTester(
                StackTrace002Debuggee.OBJECT_METHOD_WITH_EXCEPTION_FROM_NATIVE_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObjectWithException");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo =
                tester.addTestMethod("runBreakpointObjectWithExceptionAndNativeTransition");
        methodInfo.addVariable("local", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of Array variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues010_Array() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "ARRAY_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "ARRAY_PARAM_VALUE_TO_SET");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.ARRAY_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointArray");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointArray");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of Array into a local variable declared as
     * java.lang.Object.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues010_ArrayAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "ARRAY_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "ARRAY_PARAM_VALUE_TO_SET");
        StackFrameTester tester =
                new StackFrameTester(StackTrace002Debuggee.ARRAY_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.Class variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues011_Class() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "CLASS_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "CLASS_PARAM_VALUE_TO_SET");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.CLASS_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointClass");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointClass");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.Class into a local variable declared as
     * java.lang.Object.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues011_ClassAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "CLASS_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "CLASS_PARAM_VALUE_TO_SET");
        StackFrameTester tester =
                new StackFrameTester(StackTrace002Debuggee.CLASS_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.ClassLoader variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues012_ClassLoader() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "CLASS_LOADER_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "CLASS_LOADER_PARAM_VALUE_TO_SET");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.CLASS_LOADER_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointClassLoader");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointClassLoader");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.ClassLoader into a local variable declared as
     * java.lang.Object.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues012_ClassLoaderAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "CLASS_LOADER_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "CLASS_LOADER_PARAM_VALUE_TO_SET");
        StackFrameTester tester =
                new StackFrameTester(StackTrace002Debuggee.CLASS_LOADER_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.String variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues013_String() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "STRING_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "STRING_PARAM_VALUE_TO_SET");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.STRING_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointString");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointString");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.String into a local variable declared as
     * java.lang.Object.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues013_StringAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "STRING_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "STRING_PARAM_VALUE_TO_SET");
        StackFrameTester tester =
                new StackFrameTester(StackTrace002Debuggee.STRING_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.Thread variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues014_Thread() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "THREAD_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "THREAD_PARAM_VALUE_TO_SET");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.THREAD_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointThread");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointThread");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.Thread into a local variable declared as
     * java.lang.Object.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues014_ThreadAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "THREAD_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "THREAD_PARAM_VALUE_TO_SET");
        StackFrameTester tester =
                new StackFrameTester(StackTrace002Debuggee.THREAD_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.ThreadGroup variable into the stack.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues015_ThreadGroup() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "THREAD_GROUP_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "THREAD_GROUP_PARAM_VALUE_TO_SET");
        StackFrameTester tester = new StackFrameTester(StackTrace002Debuggee.THREAD_GROUP_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointThreadGroup");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointThreadGroup");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }

    /**
     * Tests we correctly write value of java.lang.ThreadGroup into a local variable declared as
     * java.lang.Object.
     *
     * Refer to {@link JDWPStackFrameAccessTest#runStackFrameTest}
     * method for the sequence of the test.
     */
    public void testSetValues015_ThreadGroupAsObject() {
        long classID = getClassIDBySignature(getDebuggeeClassSignature());
        Value oldValue = getStaticFieldValue(classID, "THREAD_GROUP_PARAM_VALUE");
        Value newValue = getStaticFieldValue(classID, "THREAD_GROUP_PARAM_VALUE_TO_SET");
        StackFrameTester tester =
                new StackFrameTester(StackTrace002Debuggee.THREAD_GROUP_AS_OBJECT_SIGNAL);
        MethodInfo suspensionMethodInfo = tester.addTestMethod("breakpointObject");
        suspensionMethodInfo.addVariable("param", oldValue, newValue);
        MethodInfo methodInfo = tester.addTestMethod("runBreakpointObject");
        methodInfo.addVariable("param", oldValue, newValue);
        runStackFrameTest(tester, suspensionMethodInfo);
    }
}
