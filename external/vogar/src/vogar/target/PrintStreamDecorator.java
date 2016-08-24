/*
 * Copyright (C) 2011 The Android Open Source Project
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

import java.io.PrintStream;

public abstract class PrintStreamDecorator extends PrintStream {
    public PrintStreamDecorator(PrintStream delegate) {
        super(delegate);
    }

    @Override public final void print(long l) {
        print(String.valueOf(l));
    }

    @Override public final void print(int i) {
        print(String.valueOf(i));
    }

    @Override public final void print(float f) {
        print(String.valueOf(f));
    }

    @Override public final void print(double d) {
        print(String.valueOf(d));
    }

    @Override public final void print(char[] s) {
        print(String.valueOf(s));
    }

    @Override public final void print(char c) {
        print(String.valueOf(c));
    }

    @Override public final void print(Object obj) {
        print(obj != null ? obj.toString() : "null");
    }

    @Override public abstract void print(String str);

    @Override public final void println() {
        print("\n");
    }

    /**
     * Although println() is documented to be equivalent to print()
     * followed by println(), this isn't the behavior on HotSpot
     * and we must manually override println(String) to ensure that
     * newlines aren't dropped.
     */
    @Override public final void println(String s) {
        print(s + "\n");
    }

    @Override public final void println(long l) {
        println(String.valueOf(l));
    }

    @Override public final void println(int i) {
        println(String.valueOf(i));
    }

    @Override public final void println(float f) {
        println(String.valueOf(f));
    }

    @Override public final void println(double d) {
        println(String.valueOf(d));
    }

    @Override public final void println(char[] s) {
        println(String.valueOf(s));
    }

    @Override public final void println(char c) {
        println(String.valueOf(c));
    }

    @Override public final void println(Object obj) {
        println(obj != null ? obj.toString() : "null");
    }
}
