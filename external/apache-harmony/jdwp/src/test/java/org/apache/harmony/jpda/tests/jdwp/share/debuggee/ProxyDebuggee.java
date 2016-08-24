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

package org.apache.harmony.jpda.tests.jdwp.share.debuggee;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

public class ProxyDebuggee extends SyncDebuggee {
    public static Object checkedProxyObject = null;

    public static final int ARG_INT = 5;
    public static final long ARG_LONG = 135l;
    public static final String ARG_OBJECT = "a string argument";

    public void breakpointMethodFromProxy() {
        System.out.println("breakpointMethodFromProxy");
    }

    static interface InterfaceForProxy {
        public void call(int i, long l, Object obj);
    }

    static class ProxyInvocationHandler implements InvocationHandler {
        private final ProxyDebuggee mDebuggee;

        public ProxyInvocationHandler(ProxyDebuggee debuggee) {
            mDebuggee = debuggee;
        }

        @Override
        public Object invoke(Object proxy, Method method, Object[] args)
                throws Throwable {
            mDebuggee.breakpointMethodFromProxy();
            return null;
        }
    }

    public void invokeBreakpointMethodFromProxy() {
        checkedProxyObject = Proxy.newProxyInstance(getClass().getClassLoader(),
                new Class<?>[] { InterfaceForProxy.class },
                new ProxyInvocationHandler(this));
        InterfaceForProxy proxy = (InterfaceForProxy) checkedProxyObject;
        proxy.call(ARG_INT, ARG_LONG, ARG_OBJECT);
    }

    @Override
    public void run() {
        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);

        logWriter.println("ProxyDebuggee started");

        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);

        invokeBreakpointMethodFromProxy();

        logWriter.println("ProxyDebuggee finished");
    }

    public static void main(String[] args) {
        runDebuggee(ProxyDebuggee.class);
    }

}
