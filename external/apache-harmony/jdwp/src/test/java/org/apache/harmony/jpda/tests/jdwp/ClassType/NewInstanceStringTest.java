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
 * JDWP unit test for ClassType.NewInstance command with java.lang.String class.
 */
public class NewInstanceStringTest extends AbstractNewInstanceTestCase {

    @Override
    protected String getDebuggeeClassName() {
        return NewInstanceStringDebuggee.class.getName();
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String()</code>.
     */
    public void testNewInstanceString_NoArgConstructor() {
        runTestNewInstanceString("()V", new NoConstructorArgumentProvider());
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(byte[])</code>.
     */
    public void testNewInstanceString_ByteArrayArgConstructor() {
        runTestNewInstanceString("([B)V", new ConstructorArgumentsProvider() {

            @Override
            public void provideConstructorArguments(List<Value> constructorArguments) {
                // Pass a reference to BYTE_ARRAY static field.
                long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                Value byteArrayValue = getStaticFieldValue(debuggeeClassId, "BYTE_ARRAY");
                constructorArguments.add(byteArrayValue);
            }
        });
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(byte[], int, int)</code>.
     */
    public void testNewInstanceString_ByteArrayIntIntConstructor() {
        runTestNewInstanceString("([BII)V", new ConstructorArgumentsProvider() {
            @Override
            public void provideConstructorArguments(List<Value> constructorArguments) {
                // Pass a reference to BYTE_ARRAY static field.
                long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                Value byteArrayValue = getStaticFieldValue(debuggeeClassId, "BYTE_ARRAY");
                constructorArguments.add(byteArrayValue);
                constructorArguments.add(new Value(0));
                constructorArguments.add(new Value(1));
            }
        });
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(byte[], int, int, java.lang.String)</code>.
     */
    public void testNewInstanceString_ByteArrayIntIntStringConstructor() {
        runTestNewInstanceString("([BIILjava/lang/String;)V", new ConstructorArgumentsProvider() {
            @Override
            public void provideConstructorArguments(List<Value> constructorArguments) {
                // Pass a reference to BYTE_ARRAY and STRING_CHARSET static
                // fields.
                long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                Value byteArrayValue = getStaticFieldValue(debuggeeClassId, "BYTE_ARRAY");
                Value stringCharsetValue = getStaticFieldValue(debuggeeClassId, "STRING_CHARSET");
                constructorArguments.add(byteArrayValue);
                constructorArguments.add(new Value(0));
                constructorArguments.add(new Value(1));
                constructorArguments.add(stringCharsetValue);
            }
        });
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(byte[], java.lang.String)</code>.
     */
    public void testNewInstanceString_ByteArrayStringConstructor() {
        runTestNewInstanceString("([BLjava/lang/String;)V", new ConstructorArgumentsProvider() {
            @Override
            public void provideConstructorArguments(List<Value> constructorArguments) {
                // Pass a reference to BYTE_ARRAY and STRING_CHARSET static
                // fields.
                long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                Value byteArrayValue = getStaticFieldValue(debuggeeClassId, "BYTE_ARRAY");
                Value stringCharsetValue = getStaticFieldValue(debuggeeClassId, "STRING_CHARSET");
                constructorArguments.add(byteArrayValue);
                constructorArguments.add(stringCharsetValue);
            }
        });
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(byte[], int, int, java.nio.charset.Charset)</code>
     * .
     */
    public void testNewInstanceString_ByteArrayIntIntCharsetConstructor() {
        runTestNewInstanceString("([BIILjava/nio/charset/Charset;)V",
                new ConstructorArgumentsProvider() {
                    @Override
                    public void provideConstructorArguments(List<Value> constructorArguments) {
                        // Pass a reference to BYTE_ARRAY and CHARSET static
                        // fields.
                        long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                        Value byteArrayValue = getStaticFieldValue(debuggeeClassId, "BYTE_ARRAY");
                        Value charsetValue = getStaticFieldValue(debuggeeClassId, "CHARSET");
                        constructorArguments.add(byteArrayValue);
                        constructorArguments.add(new Value(0));
                        constructorArguments.add(new Value(1));
                        constructorArguments.add(charsetValue);
                    }
                });
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(byte[], java.nio.charset.Charset)</code>.
     */
    public void testNewInstanceString_ByteArrayCharsetConstructor() {
        runTestNewInstanceString("([BLjava/nio/charset/Charset;)V",
                new ConstructorArgumentsProvider() {
                    @Override
                    public void provideConstructorArguments(List<Value> constructorArguments) {
                        // Pass a reference to BYTE_ARRAY and CHARSET static
                        // fields.
                        long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                        Value byteArrayValue = getStaticFieldValue(debuggeeClassId, "BYTE_ARRAY");
                        Value charsetValue = getStaticFieldValue(debuggeeClassId, "CHARSET");
                        constructorArguments.add(byteArrayValue);
                        constructorArguments.add(charsetValue);
                    }
                });
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(char[])</code>.
     */
    public void testNewInstanceString_CharArrayConstructor() {
        runTestNewInstanceString("([C)V", new ConstructorArgumentsProvider() {
            @Override
            public void provideConstructorArguments(List<Value> constructorArguments) {
                // Pass a reference to CHAR_ARRAY static field.
                long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                Value charArrayValue = getStaticFieldValue(debuggeeClassId, "CHAR_ARRAY");
                constructorArguments.add(charArrayValue);
            }
        });
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(char[], int, int)</code>.
     */
    public void testNewInstanceString_CharArrayIntIntConstructor() {
        runTestNewInstanceString("([CII)V", new ConstructorArgumentsProvider() {
            @Override
            public void provideConstructorArguments(List<Value> constructorArguments) {
                // Pass a reference to CHAR_ARRAY static field.
                long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                Value charArrayValue = getStaticFieldValue(debuggeeClassId, "CHAR_ARRAY");
                constructorArguments.add(charArrayValue);
                constructorArguments.add(new Value(0));
                constructorArguments.add(new Value(1));
            }
        });
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(java.lang.String)</code>.
     */
    public void testNewInstanceString_StringConstructor() {
        runTestNewInstanceString("(Ljava/lang/String;)V", new ConstructorArgumentsProvider() {
            @Override
            public void provideConstructorArguments(List<Value> constructorArguments) {
                // Pass a reference to TEST_STRING static field.
                long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                Value testStringValue = getStaticFieldValue(debuggeeClassId, "TEST_STRING");
                constructorArguments.add(testStringValue);
            }
        });
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(java.lang.StringBuffer)</code>.
     */
    public void testNewInstanceString_StringBufferConstructor() {
        runTestNewInstanceString("(Ljava/lang/StringBuffer;)V", new ConstructorArgumentsProvider() {
            @Override
            public void provideConstructorArguments(List<Value> constructorArguments) {
                // Pass a reference to STRING_BUFFER static field.
                long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                Value stringBufferValue = getStaticFieldValue(debuggeeClassId, "STRING_BUFFER");
                constructorArguments.add(stringBufferValue);
            }
        });
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(int[], * int, int)</code>.
     */
    public void testNewInstanceString_IntArrayIntIntConstructor() {
        runTestNewInstanceString("([III)V", new ConstructorArgumentsProvider() {
            @Override
            public void provideConstructorArguments(List<Value> constructorArguments) {
                // Pass a reference to INT_ARRAY static field.
                long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                Value intArrayValue = getStaticFieldValue(debuggeeClassId, "INT_ARRAY");
                constructorArguments.add(intArrayValue);
                constructorArguments.add(new Value(0));
                constructorArguments.add(new Value(1));
            }
        });
    }

    /**
     * Test ClassType.NewInstance using the constructor
     * <code>java.lang.String(java.lang.StringBuilder)</code>.
     */
    public void testNewInstanceString_StringBuilderConstructor() {
        runTestNewInstanceString("(Ljava/lang/StringBuilder;)V",
                new ConstructorArgumentsProvider() {
                    @Override
                    public void provideConstructorArguments(List<Value> constructorArguments) {
                        // Pass a reference to STRING_BUILDER static field.
                        long debuggeeClassId = getClassIDBySignature(getDebuggeeClassSignature());
                        Value stringBuilderValue = getStaticFieldValue(debuggeeClassId,
                                "STRING_BUILDER");
                        constructorArguments.add(stringBuilderValue);
                    }
                });
    }

    /**
     * Exercises ClassType.NewInstance command for java.lang.String.
     */
    private void runTestNewInstanceString(String constructorSignature,
            ConstructorArgumentsProvider provider) {
        checkNewInstanceTag("Ljava/lang/String;", constructorSignature, provider,
                JDWPConstants.Tag.STRING_TAG);
    }

    private Value getStaticFieldValue(long classId, String fieldName) {
        long fieldId = checkField(classId, fieldName);
        return debuggeeWrapper.vmMirror.getReferenceTypeValue(classId, fieldId);
    }
}
