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

package vogar.testing;

import com.google.common.base.Joiner;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.io.UnsupportedEncodingException;
import java.util.EnumMap;
import java.util.EnumSet;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

/**
 * A {@link TestRule} that will intercept content written to {@link System#out} and/or
 * {@link System#err} and collate it for use by the test.
 */
public class InterceptOutputStreams implements TestRule {

    /**
     * The streams that can be intercepted.
     */
    public enum Stream {
        OUT {
            @Override
            PrintStream get() {
                return System.out;
            }

            @Override
            void set(PrintStream stream) {
                System.setOut(stream);
            }
        },
        ERR {
            @Override
            PrintStream get() {
                return System.err;
            }

            @Override
            void set(PrintStream stream) {
                System.setErr(stream);
            }
        };

        abstract PrintStream get();

        abstract void set(PrintStream stream);
    }

    /**
     * The streams to intercept.
     */
    private final EnumSet<Stream> streams;
    private final EnumMap<Stream, State> streams2State;

    /**
     * The streams to intercept.
     */
    public InterceptOutputStreams(Stream... streams) {
        this.streams = EnumSet.of(streams[0], streams);
        streams2State = new EnumMap<>(Stream.class);
    }

    /**
     * Get the intercepted contents for the stream.
     * @param stream the stream whose contents are required.
     * @return the intercepted contents.
     * @throws IllegalStateException if the stream contents are not being intercepted (in which
     *     case the developer needs to add {@code stream} to the constructor parameters), or if the
     *     test is not actually running at the moment.
     */
    public String contents(Stream stream) {
        if (!streams.contains(stream)) {
            EnumSet<Stream> extra = streams.clone();
            extra.add(stream);
            String message = "Not intercepting " + stream + " output, try:\n"
                    + "    new " + InterceptOutputStreams.class.getSimpleName() + "("
                    + Joiner.on(", ").join(extra)
                    + ")";
            throw new IllegalStateException(message);
        }

        State state = streams2State.get(stream);
        if (state == null) {
            throw new IllegalStateException(
                    "Attempting to access stream contents outside the test");
        }

        return state.contents();
    }

    @Override
    public Statement apply(final Statement base, Description description) {
        return new Statement() {
            @Override
            public void evaluate() throws Throwable {
                for (Stream stream : streams) {
                    State state = new State(stream);
                    streams2State.put(stream, state);
                }

                try {
                    base.evaluate();
                } finally {
                    for (State state : streams2State.values()) {
                        state.reset();
                    }
                    streams2State.clear();
                }
            }
        };
    }

    private static class State {
        private final PrintStream original;
        private final ByteArrayOutputStream baos;
        private final Stream stream;

        State(Stream stream) throws IOException {
            this.stream = stream;
            original = stream.get();
            baos = new ByteArrayOutputStream();
            stream.set(new PrintStream(baos, true, "UTF-8"));
        }

        String contents() {
            try {
                return baos.toString("UTF-8");
            } catch (UnsupportedEncodingException e) {
                throw new RuntimeException(e);
            }
        }

        void reset() {
            stream.set(original);
        }
    }
}
