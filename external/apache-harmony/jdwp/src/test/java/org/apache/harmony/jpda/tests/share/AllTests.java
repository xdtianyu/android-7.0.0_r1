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

package org.apache.harmony.jpda.tests.share;


public class AllTests {
  public static void main(String[] args) {
    junit.framework.TestResult result = junit.textui.TestRunner.run(suite());
    if (!result.wasSuccessful()) {
        System.exit(1);
    }
  }

  public static junit.framework.Test suite() {
    junit.framework.TestSuite suite = new junit.framework.TestSuite();

    //
    // "TODO".
    //

    // I haven't yet found an IDE that will use these, but we might want to implement them anyway.
    //suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.MonitorContendedEnteredTest.class);
    //suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.MonitorContendedEnterTest.class);
    //suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.MonitorWaitedTest.class);
    //suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.MonitorWaitTest.class);

    // I don't know when these are ever used, but they're not obviously useless.
    //suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.DebuggerOnDemand.OnthrowDebuggerLaunchTest.class);
    //suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.NestedTypesTest.class);
    //suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.HoldEventsTest.class);
    //suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.ReleaseEventsTest.class);

    //
    // "Will not fix".
    //

    // It's not obvious how to translate this into our world, or what debuggers would do with it.
    //suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.ClassFileVersionTest.class);

    // We don't implement Thread.stop at all, so it doesn't make sense for us to implement the JDWP.
    //suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.StopTest.class);

    // We don't implement class unloading.
    //suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.ClassUnloadTest.class);

    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ArrayReference.GetValuesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ArrayReference.LengthTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ArrayReference.SetValues002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ArrayReference.SetValues003Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ArrayReference.SetValuesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ArrayType.NewInstanceTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassLoaderReference.VisibleClassesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassObjectReference.ReflectedType002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassObjectReference.ReflectedTypeTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.InvokeMethod002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.InvokeMethod003Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.InvokeMethodAfterMultipleThreadSuspensionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.InvokeMethodWithSuspensionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.InvokeMethodTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.NewInstance002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.NewInstanceTagTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.NewInstanceAfterMultipleThreadSuspensionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.NewInstanceStringTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.NewInstanceTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.NewInstanceWithSuspensionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.SetValues002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.SetValuesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ClassType.SuperClassTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Deoptimization.DeoptimizationWithExceptionHandlingTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.EventModifiers.CountModifierTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.EventModifiers.InstanceOnlyModifierTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.EventModifiers.ThreadOnlyModifierTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.Breakpoint002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.BreakpointMultipleTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.BreakpointOnCatchTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.BreakpointTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.ClassPrepareTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.CombinedEvents002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.CombinedEvents003Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.CombinedEventsTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.CombinedExceptionEventsTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.EventWithExceptionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.ExceptionCaughtTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.ExceptionUncaughtTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.ExceptionWithLocationTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.FieldAccessTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.FieldModification002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.FieldModificationTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.FieldWithLocationTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.MethodEntryTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.MethodExitTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.MethodExitWithReturnValueTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.SingleStepTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.SingleStepThroughReflectionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.SingleStepWithLocationTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.SingleStepWithPendingExceptionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.ThreadEndTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.ThreadStartTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.VMDeath002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Events.VMDeathTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.InterfaceType.InvokeMethodTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Method.BytecodesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Method.IsObsoleteTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Method.LineTableTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Method.VariableTableTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.Method.VariableTableWithGenericTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.AttachConnectorTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.BreakpointTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.ClassObjectIDTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.ClassPrepareTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.EnableCollectionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.ExceptionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.FieldAccessTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.FieldModificationTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.ListenConnectorTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.MethodEntryExitTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.RefTypeIDTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.ResumeTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.SingleStepTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.MultiSession.VMDeathTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.DisableCollectionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.EnableCollectionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.GetValues002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.GetValues003Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.GetValuesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.InvokeMethod002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.InvokeMethod003Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.InvokeMethodTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.InvokeMethodAfterMultipleThreadSuspensionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.InvokeMethodDefault002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.InvokeMethodDefaultTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.InvokeMethodWithSuspensionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.IsCollectedTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.MonitorInfoTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.ReferenceTypeTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.ReferringObjectsTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.SetValues002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.SetValues003Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.SetValues004Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ObjectReference.SetValuesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.ClassLoaderTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.ClassObjectTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.ConstantPoolTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.FieldsTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.FieldsWithGenericTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.GetValues002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.GetValues003Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.GetValues004Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.GetValues005Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.GetValuesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.InstancesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.InterfacesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.MethodsTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.MethodsWithGenericTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.ModifiersTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.Signature002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.SignatureTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.SignatureWithGenericTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.SourceDebugExtensionTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.SourceFileTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ReferenceType.StatusTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.StackFrame.GetValues002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.StackFrame.GetValuesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.StackFrame.PopFrames002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.StackFrame.PopFramesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.StackFrame.ProxyThisObjectTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.StackFrame.SetValues002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.StackFrame.SetValuesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.StackFrame.ThisObjectTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.StringReference.ValueTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadGroupReference.ChildrenTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadGroupReference.NameTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadGroupReference.ParentTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.CurrentContendedMonitorTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.ForceEarlyReturn002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.ForceEarlyReturn003Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.ForceEarlyReturn004Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.ForceEarlyReturn005Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.ForceEarlyReturn006Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.ForceEarlyReturnTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.FrameCountTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.FramesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.InterruptTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.NameTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.OwnedMonitorsStackDepthInfoTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.OwnedMonitorsTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.ResumeTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.Status002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.Status003Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.Status004Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.Status005Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.Status006Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.StatusTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.SuspendCountTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.SuspendTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.ThreadGroup002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.ThreadReference.ThreadGroupTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.AllClassesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.AllClassesWithGenericTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.AllThreadsTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.CapabilitiesNewTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.CapabilitiesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.ClassesBySignatureTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.ClassPathsTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.CreateStringTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.DisposeDuringInvokeTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.DisposeTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.DisposeObjectsTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.ExitTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.IDSizesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.InstanceCountsTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.RedefineClassesTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.Resume002Test.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.ResumeTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.SetDefaultStratumTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.SuspendTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.TopLevelThreadGroupsTest.class);
    suite.addTestSuite(org.apache.harmony.jpda.tests.jdwp.VirtualMachine.VersionTest.class);
    return suite;
  }
}
