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

package org.apache.harmony.jpda.tests.framework.jdwp;

import java.util.ArrayList;
import java.util.List;

/**
 * This class allows to build an {@link Event} with multiple modifiers.
 */
public class EventBuilder {
    private final byte eventKind;
    private final byte suspendPolicy;
    private final List<EventMod> modifiers = new ArrayList<EventMod>();

    public EventBuilder(byte eventKind, byte suspendPolicy) {
        this.eventKind = eventKind;
        this.suspendPolicy = suspendPolicy;
    }

    /**
     * Sets the Count modifier.
     *
     * @param count the count before event
     * @return a reference to this object
     */
    public EventBuilder setCount(int count) {
        EventMod mod = createModifier(EventMod.ModKind.Count);
        mod.count = count;
        modifiers.add(mod);
        return this;
    }

    /**
     * Sets the ThreadOnly modifier.
     *
     * @param thread the ID of the required thread object
     * @return a reference to this object
     */
    public EventBuilder setThreadOnly(long thread) {
        EventMod mod = createModifier(EventMod.ModKind.ThreadOnly);
        mod.thread = thread;
        modifiers.add(mod);
        return this;
    }

    /**
     * Sets the ClassMatch modifier.
     *
     * @param pattern the required class pattern
     * @return a reference to this object
     */
    public EventBuilder setClassMatch(String pattern) {
        EventMod mod = createModifier(EventMod.ModKind.ClassMatch);
        mod.classPattern = pattern;
        modifiers.add(mod);
        return this;
    }

    /**
     * Sets the LocationOnly modifier.
     *
     * @param location the required location
     * @return a reference to this object
     */
    public EventBuilder setLocationOnly(Location location) {
        EventMod mod = createModifier(EventMod.ModKind.LocationOnly);
        mod.loc = location;
        modifiers.add(mod);
        return this;
    }

    /**
     * Sets the ExceptionOnly modifier.
     *
     * @param exceptionClassID the reference type ID of the exception class
     * @param caught true to report caught exception, false otherwise.
     * @param uncaught true to report uncaught exception, false otherwise
     * @return a reference to this object
     */
    public EventBuilder setExceptionOnly(long exceptionClassID, boolean caught,
            boolean uncaught) {
        EventMod mod = createModifier(EventMod.ModKind.ExceptionOnly);
        mod.caught = caught;
        mod.uncaught = uncaught;
        mod.exceptionOrNull = exceptionClassID;
        modifiers.add(mod);
        return this;
    }

    /**
     * Sets the FieldOnly modifier.
     *
     * @param typeID the reference type ID of the field's declaring class.
     * @param fieldID the required field ID
     * @return a reference to this object
     */
    public EventBuilder setFieldOnly(long typeID, long fieldID) {
        EventMod mod = createModifier(EventMod.ModKind.FieldOnly);
        mod.declaring = typeID;
        mod.fieldID = fieldID;
        modifiers.add(mod);
        return this;
    }

    /**
     * Sets the InstanceOnly modifier.
     *
     * @param instance the required object ID
     * @return a reference to this object
     */
    public EventBuilder setInstanceOnly(long instance) {
        EventMod mod = createModifier(EventMod.ModKind.InstanceOnly);
        mod.instance = instance;
        modifiers.add(mod);
        return this;
    }

    /**
     * Builds the event with all added modifiers.
     *
     * @return an {@link Event}
     */
    public Event build() {
        EventMod[] mods = new EventMod[modifiers.size()];
        mods = modifiers.toArray(mods);
        return new Event(eventKind, suspendPolicy, mods);
    }

    private static EventMod createModifier(byte modifierKind) {
        EventMod mod = new EventMod();
        mod.modKind = modifierKind;
        return mod;
    }
}
