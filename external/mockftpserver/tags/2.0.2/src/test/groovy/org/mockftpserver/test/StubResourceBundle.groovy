/*
 * Copyright 2008 the original author or authors.
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
package org.mockftpserver.test

/**
 * Stub implementation of ResourceBundle for testing. Provide an optional Map of entries in the constructor,
 * and allow dynamic adding or changing of map contents. Automatically define default value for key if no entry
 * exists for the key.
 */
class StubResourceBundle extends ResourceBundle {

    Map map

    StubResourceBundle(Map map = [:]) {
        this.map = map
    }

    void put(String key, String value) {
        map.put(key, value)
    }

    Object handleGetObject(String key) {
        // Return default if no entry is defined
        return map[key] ?: "key=$key arg0={0} arg1={1}".toString()
    }

    public Enumeration getKeys() {
        return new Vector(map.keySet()).elements()
    }

}