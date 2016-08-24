/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.cts.useslibrary;

import android.content.pm.PackageManager;
import android.os.Environment;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiSelector;
import android.test.InstrumentationTestCase;

import dalvik.system.BaseDexClassLoader;
import dalvik.system.DexFile;
import dalvik.system.PathClassLoader;

import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

public class UsesLibraryTest extends InstrumentationTestCase {
    private static final String TAG = "UsesLibraryTest";

    public void testUsesLibrary() throws Exception {
        ClassLoader loader = getClass().getClassLoader();
        if (loader instanceof BaseDexClassLoader) {
            Object[] dexElements = getDexElementsFromClassLoader((BaseDexClassLoader) loader);
            for (Object dexElement : dexElements) {
                DexFile dexFile = getDexFileFromDexElement(dexElement);
                assertTrue(isDexFileBackedByOatFile(dexFile));
            }
        }
    }

    public void testMissingLibrary() throws Exception {
        ClassLoader loader = getClass().getClassLoader();
        if (loader instanceof BaseDexClassLoader) {
            Object[] dexElements = getDexElementsFromClassLoader((BaseDexClassLoader) loader);
            assertTrue(dexElements != null && dexElements.length > 1);

            DexFile dexFile = getDexFileFromDexElement(dexElements[1]);
            String testApkPath = dexFile.getName();
            PathClassLoader testLoader = new PathClassLoader(testApkPath, null);
            Object[] testDexElements = getDexElementsFromClassLoader(testLoader);
            assertTrue(testDexElements != null && testDexElements.length == 1);

            DexFile testDexFile = getDexFileFromDexElement(testDexElements[0]);
            assertTrue(isDexFileBackedByOatFile(testDexFile));
        }
    }

    public void testDuplicateLibrary() throws Exception {
        ClassLoader loader = getClass().getClassLoader();
        if (loader instanceof BaseDexClassLoader) {
            Object[] dexElements = getDexElementsFromClassLoader((BaseDexClassLoader) loader);
            assertTrue(dexElements != null && dexElements.length > 1);

            DexFile libDexFile = getDexFileFromDexElement(dexElements[0]);
            String libPath = libDexFile.getName();
            DexFile apkDexFile = getDexFileFromDexElement(dexElements[1]);
            String apkPath = apkDexFile.getName();
            String testPath = libPath + File.pathSeparator + apkPath + File.pathSeparator + apkPath;
            PathClassLoader testLoader = new PathClassLoader(testPath, null);
            Object[] testDexElements = getDexElementsFromClassLoader(testLoader);
            assertTrue(testDexElements != null && testDexElements.length == 3);

            DexFile testDexFile = getDexFileFromDexElement(testDexElements[2]);
            assertFalse(isDexFileBackedByOatFile(testDexFile));
        }
    }

    private Object[] getDexElementsFromClassLoader(BaseDexClassLoader loader) throws Exception {
        Field pathListField = BaseDexClassLoader.class.getDeclaredField("pathList");
        pathListField.setAccessible(true);
        // This is a DexPathList, but that class is package private.
        Object pathList = pathListField.get(loader);
        Field dexElementsField = pathList.getClass().getDeclaredField("dexElements");
        dexElementsField.setAccessible(true);
        // The objects in this array are Elements, but that class is package private.
        return (Object[]) dexElementsField.get(pathList);
    }

    // The argument must be a DexPathList.Element.
    private DexFile getDexFileFromDexElement(Object dexElement) throws Exception {
        Field dexFileField = dexElement.getClass().getDeclaredField("dexFile");
        dexFileField.setAccessible(true);
        return (DexFile) dexFileField.get(dexElement);
    }

    private boolean isDexFileBackedByOatFile(DexFile dexFile) throws Exception {
        Method isBackedByOatFileMethod = DexFile.class.getDeclaredMethod("isBackedByOatFile");
        isBackedByOatFileMethod.setAccessible(true);
        return (boolean) isBackedByOatFileMethod.invoke(dexFile);
    }
}
