/*
 * Copyright (C) 2016 The Android Open Source Project.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.doclava;

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public class StubsTest extends TestCase {

    public void testLicenseWithPackage() throws IOException {
        assertEquals("// Foo\n", parseLicenseHeader(
                "// Foo",
                "package foo",
                ""));

        assertEquals("// Foo\n// Bar\n", parseLicenseHeader(
                "// Foo",
                "// Bar",
                "package foo",
                ""));

        assertEquals("// Foo\n// Bar\n", parseLicenseHeader(
                "// Foo",
                "// Bar",
                "   package foo",
                ""));

        assertEquals("// Foo\n// Bar package bar food\n", parseLicenseHeader(
                "// Foo",
                "// Bar package bar food",
                "package foo",
                ""));
    }

    public void testLicenseWithImport() throws IOException {
        assertEquals("// Foo\n", parseLicenseHeader(
                "// Foo",
                "import foo",
                ""));

        assertEquals("// Foo\n// Bar\n", parseLicenseHeader(
                "// Foo",
                "// Bar",
                "import foo",
                ""));

        assertEquals("// Foo\n// Bar\n", parseLicenseHeader(
                "// Foo",
                "// Bar",
                "   import foo;",
                ""));

        assertEquals("// Foo\n// Bar import bar food\n", parseLicenseHeader(
                "// Foo",
                "// Bar import bar food",
                "import foo",
                ""));
    }

    public void testLineStripping() throws IOException {
        assertEquals("/*\n* Foo\n*/\n//\n// Bar\n//\n", parseLicenseHeader(
                "/*",
                "          * Foo",
                "                        */",
                "//",
                "   // Bar",
                "            //",
                "package foo",
                ""));
    }

    public void testMultipleComments() throws IOException {
        assertEquals("/*\n* Foo\n*/\n/*\n* Bar\n*/\n", parseLicenseHeader(
                "/*",
                " * Foo",
                " */",
                "/*",
                " * Bar",
                " */",
                "package foo",
                ""));
    }

    private static String parseLicenseHeader(String... lines) throws IOException {
        StringBuilder builder = new StringBuilder(8192);
        for (String line : lines) {
            builder.append(line);
            builder.append('\n');
        }

        return Stubs.parseLicenseHeader(
                new ByteArrayInputStream(builder.toString().getBytes(StandardCharsets.UTF_8)));
    }
}

