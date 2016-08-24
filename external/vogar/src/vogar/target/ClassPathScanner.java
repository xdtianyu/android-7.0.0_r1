/*
 * Copyright (C) 2008 The Android Open Source Project
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

import dalvik.system.DexFile;
import java.io.File;
import java.io.IOException;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Set;
import java.util.TreeSet;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * Inspects the classpath to return the classes in a requested package. This
 * class doesn't yet traverse directories on the classpath.
 *
 * <p>Adapted from android.test.ClassPathPackageInfo. Unlike that class, this
 * runs on both Dalvik and Java VMs.
 */
final class ClassPathScanner {

    static final Comparator<Class<?>> ORDER_CLASS_BY_NAME = new Comparator<Class<?>>() {
        @Override public int compare(Class<?> a, Class<?> b) {
            return a.getName().compareTo(b.getName());
        }
    };
    private static final String DOT_CLASS = ".class";

    private final String[] classPath;
    private final ClassFinder classFinder;

    ClassPathScanner() {
        classPath = getClassPath();
        classFinder = "Dalvik".equals(System.getProperty("java.vm.name"))
                ? new ApkClassFinder()
                : new JarClassFinder();
    }

    /**
     * Returns a package describing the loadable classes whose package name is
     * {@code packageName}.
     */
    public Package scan(String packageName) throws IOException {
        Set<String> subpackageNames = new TreeSet<String>();
        Set<String> classNames = new TreeSet<String>();
        Set<Class<?>> topLevelClasses = new TreeSet<Class<?>>(ORDER_CLASS_BY_NAME);
        findClasses(packageName, classNames, subpackageNames);
        for (String className : classNames) {
            try {
                topLevelClasses.add(Class.forName(className, false, getClass().getClassLoader()));
            } catch (ClassNotFoundException e) {
                throw new RuntimeException(e);
            }
        }
        return new Package(this, subpackageNames, topLevelClasses);
    }

    /**
     * Finds all classes and subpackages that are below the packageName and
     * add them to the respective sets. Searches the package on the whole class
     * path.
     */
    private void findClasses(String packageName, Set<String> classNames,
            Set<String> subpackageNames) throws IOException {
        String packagePrefix = packageName + '.';
        String pathPrefix = packagePrefix.replace('.', '/');
        for (String entry : classPath) {
            File entryFile = new File(entry);
            if (entryFile.exists() && !entryFile.isDirectory()) {
                classFinder.find(entryFile, pathPrefix, packageName, classNames, subpackageNames);
            }
        }
    }

    interface ClassFinder {
        void find(File classPathEntry, String pathPrefix, String packageName,
                Set<String> classNames, Set<String> subpackageNames) throws IOException;
    }

    /**
     * Finds all classes and subpackages that are below the packageName and
     * add them to the respective sets. Searches the package in a single jar file.
     */
    static class JarClassFinder implements ClassFinder {
        public void find(File classPathEntry, String pathPrefix, String packageName,
                Set<String> classNames, Set<String> subpackageNames) throws IOException {
            Set<String> entryNames = getJarEntries(classPathEntry);
            // check if the Jar contains the package.
            if (!entryNames.contains(pathPrefix)) {
                return;
            }
            int prefixLength = pathPrefix.length();
            for (String entryName : entryNames) {
                if (entryName.startsWith(pathPrefix)) {
                    if (entryName.endsWith(DOT_CLASS)) {
                        // check if the class is in the package itself or in one of its
                        // subpackages.
                        int index = entryName.indexOf('/', prefixLength);
                        if (index >= 0) {
                            String p = entryName.substring(0, index).replace('/', '.');
                            subpackageNames.add(p);
                        } else if (isToplevelClass(entryName)) {
                            classNames.add(getClassName(entryName).replace('/', '.'));
                        }
                    }
                }
            }
        }

        /**
         * Gets the class and package entries from a Jar.
         */
        private Set<String> getJarEntries(File jarFile) throws IOException {
            Set<String> entryNames = new HashSet<String>();
            ZipFile zipFile = new ZipFile(jarFile);
            for (Enumeration<? extends ZipEntry> e = zipFile.entries(); e.hasMoreElements(); ) {
                String entryName = e.nextElement().getName();
                if (!entryName.endsWith(DOT_CLASS)) {
                    continue;
                }

                entryNames.add(entryName);

                // add the entry name of the classes package, i.e. the entry name of
                // the directory that the class is in. Used to quickly skip jar files
                // if they do not contain a certain package.
                //
                // Also add parent packages so that a JAR that contains
                // pkg1/pkg2/Foo.class will be marked as containing pkg1/ in addition
                // to pkg1/pkg2/ and pkg1/pkg2/Foo.class.  We're still interested in
                // JAR files that contains subpackages of a given package, even if
                // an intermediate package contains no direct classes.
                //
                // Classes in the default package will cause a single package named
                // "" to be added instead.
                int lastIndex = entryName.lastIndexOf('/');
                do {
                    String packageName = entryName.substring(0, lastIndex + 1);
                    entryNames.add(packageName);
                    lastIndex = entryName.lastIndexOf('/', lastIndex - 1);
                } while (lastIndex > 0);
            }

            return entryNames;
        }
    }

    /**
     * Finds all classes and sub packages that are below the packageName and
     * add them to the respective sets. Searches the package in a single APK.
     *
     * <p>This class uses the Android-only class DexFile. This class will fail
     * to load on non-Android VMs.
     */
    static class ApkClassFinder implements ClassFinder {
        public void find(File classPathEntry, String pathPrefix, String packageName,
                Set<String> classNames, Set<String> subpackageNames) {
            DexFile dexFile = null;
            try {
                dexFile = new DexFile(classPathEntry);
                Enumeration<String> apkClassNames = dexFile.entries();
                while (apkClassNames.hasMoreElements()) {
                    String className = apkClassNames.nextElement();
                    if (!className.startsWith(packageName)) {
                        continue;
                    }

                    String subPackageName = packageName;
                    int lastPackageSeparator = className.lastIndexOf('.');
                    if (lastPackageSeparator > 0) {
                        subPackageName = className.substring(0, lastPackageSeparator);
                    }
                    if (subPackageName.length() > packageName.length()) {
                        subpackageNames.add(subPackageName);
                    } else if (isToplevelClass(className)) {
                        classNames.add(className);
                    }
                }
            } catch (IOException ignore) {
                // okay, presumably the dex file didn't contain any classes
            } finally {
                if (dexFile != null) {
                    try {
                        dexFile.close();
                    } catch (IOException ignore) {
                    }
                }
            }
        }
    }

    /**
     * Returns true if a given file name represents a toplevel class.
     */
    private static boolean isToplevelClass(String fileName) {
        return fileName.indexOf('$') < 0;
    }

    /**
     * Given the absolute path of a class file, return the class name.
     */
    private static String getClassName(String className) {
        int classNameEnd = className.length() - DOT_CLASS.length();
        return className.substring(0, classNameEnd);
    }

    /**
     * Gets the class path from the System Property "java.class.path" and splits
     * it up into the individual elements.
     */
    public static String[] getClassPath() {
        String classPath = System.getProperty("java.class.path");
        String separator = System.getProperty("path.separator", ":");
        return classPath.split(Pattern.quote(separator));
    }
}
