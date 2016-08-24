/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.cts.tradefed.testtype;

import com.android.compatibility.common.util.AbiUtils;
import com.android.ddmlib.Log;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.util.xml.AbstractXmlParser.ParseException;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FilenameFilter;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Retrieves CTS test package definitions from the repository.
 */
public class TestPackageRepo implements ITestPackageRepo {

    private static final String LOG_TAG = "TestCaseRepo";

    /** mapping of ABI to a mapping of appPackageName to test definition */
    private final Map<String, Map<String, TestPackageDef>> mTestMap;
    private final boolean mIncludeKnownFailures;

    /**
     * Creates a {@link TestPackageRepo}, initialized from provided repo files
     *
     * @param testCaseDir directory containing all test case definition xml and build files
     * ABIs supported by the device under test.
     * @param includeKnownFailures Whether to run tests which are known to fail.
     */
    public TestPackageRepo(File testCaseDir, boolean includeKnownFailures) {
        mTestMap = new HashMap<>();
        mIncludeKnownFailures = includeKnownFailures;
        parse(testCaseDir);
    }

    /**
     * Builds mTestMap based on directory contents
     */
    private void parse(File dir) {
        File[] xmlFiles = dir.listFiles(new XmlFilter());
        for (File xmlFile : xmlFiles) {
            parseModuleTestConfigs(xmlFile);
        }
    }

    /**
     * Infer package preparer config from package XML definition file and return if exists
     * @param pkgXml {@link File} instance referencing the package XML definition
     * @return the matching package preparer if exists, <code>null</code> otherwise
     */
    private File getPreparerDefForPackage(File pkgXml) {
        String fullPath = pkgXml.getAbsolutePath();
        int lastDot = fullPath.lastIndexOf('.');
        if (lastDot == -1) {
            // huh?
            return null;
        }
        File preparer = new File(fullPath.substring(0, lastDot) + ".config");
        if (preparer.exists()) {
            return preparer;
        }
        return null;
    }

    /**
     * Processes test module definition XML file, and stores parsed data structure in class member
     * variable. Parsed config objects will be associated with each applicable ABI type so multiple
     * {@link TestPackageDef}s will be generated accordingly. In addition, based on
     * &lt;module name&gt;.config file naming convention, this method also looks for the optional
     * module test config, and attaches defined configuration objects to the {@link TestPackageDef}
     * representing the module accordingly.
     * @param xmlFile the module definition XML
     */
    private void parseModuleTestConfigs(File xmlFile)  {
        TestPackageXmlParser parser = new TestPackageXmlParser(mIncludeKnownFailures);
        try {
            parser.parse(createStreamFromFile(xmlFile));
            // based on test module XML file path, and the <module name>.config naming convention,
            // infers the module test config file, and parses it
            File preparer = getPreparerDefForPackage(xmlFile);
            IConfiguration config = null;
            if (preparer != null) {
                try {
                    // invokes parser to process the test module config file
                    config = ConfigurationFactory.getInstance().createConfigurationFromArgs(
                            new String[]{preparer.getAbsolutePath()});
                } catch (ConfigurationException e) {
                    throw new RuntimeException(
                            String.format("error parsing config file: %s", xmlFile.getName()), e);
                }
            }
            Set<TestPackageDef> defs = parser.getTestPackageDefs();
            if (defs.isEmpty()) {
                Log.w(LOG_TAG, String.format("Could not find test package info in xml file %s",
                        xmlFile.getAbsolutePath()));
            }
            // loops over multiple package defs defined for each ABI type
            for (TestPackageDef def : defs) {
                String name = def.getAppPackageName();
                String abi = def.getAbi().getName();
                if (config != null) {
                    def.setPackagePreparers(config.getTargetPreparers());
                }
                if (!mTestMap.containsKey(abi)) {
                    mTestMap.put(abi, new HashMap<String, TestPackageDef>());
                }
                mTestMap.get(abi).put(name, def);
            }
        } catch (FileNotFoundException e) {
            Log.e(LOG_TAG, String.format("Could not find test case xml file %s",
                    xmlFile.getAbsolutePath()));
            Log.e(LOG_TAG, e);
        } catch (ParseException e) {
            Log.e(LOG_TAG, String.format("Failed to parse test case xml file %s",
                    xmlFile.getAbsolutePath()));
            Log.e(LOG_TAG, e);
        }
    }

    /**
     * Helper method to create a stream to read data from given file
     * <p/>
     * Exposed for unit testing
     *
     * @param xmlFile The file containing the xml description of the package
     * @return stream to read data
     *
     */
    InputStream createStreamFromFile(File xmlFile) throws FileNotFoundException {
        return new BufferedInputStream(new FileInputStream(xmlFile));
    }

    private static class XmlFilter implements FilenameFilter {

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean accept(File dir, String name) {
            return name.endsWith(".xml");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestPackageDef getTestPackage(String id) {
        String[] parts = AbiUtils.parseId(id);
        String abi = parts[0];
        String name = parts[1];
        if (mTestMap.containsKey(abi) && mTestMap.get(abi).containsKey(name)) {
            return mTestMap.get(abi).get(name);
        }
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<String> getPackageIds() {
        Set<String> ids = new HashSet<>();
        for (String abi : mTestMap.keySet()) {
            Map<String, TestPackageDef> testNameMap = mTestMap.get(abi);
            for (TestPackageDef testPackageDef : testNameMap.values()) {
                ids.add(testPackageDef.getId());
            }
        }
        List<String> idList = new ArrayList<>(ids);
        Collections.sort(idList);
        return idList;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<String> getPackageNames() {
        Set<String> nameSet = new HashSet<String>();
        for (String abi : mTestMap.keySet()) {
            Map<String, TestPackageDef> testNameMap = mTestMap.get(abi);
            for (TestPackageDef testPackageDef : testNameMap.values()) {
                nameSet.add(AbiUtils.parseTestName(testPackageDef.getId()));
            }
        }
        List<String> nameList = new ArrayList<>(nameSet);
        Collections.sort(nameList);
        return nameList;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Map<String, List<ITestPackageDef>> getTestPackageDefsByName() {
        Map<String, List<ITestPackageDef>> packageDefMap =
                new HashMap<String, List<ITestPackageDef>>();

        for (String abi : mTestMap.keySet()) {
            Map<String, TestPackageDef> testNameMap = mTestMap.get(abi);
            for (String packageName : testNameMap.keySet()) {
                if (!packageDefMap.containsKey(packageName)) {
                    packageDefMap.put(packageName, new ArrayList<ITestPackageDef>());
                }
                packageDefMap.get(packageName).add(testNameMap.get(packageName));
            }
        }
        return packageDefMap;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<String> findPackageIdsForTest(String testClassName) {
        Set<String> ids = new HashSet<String>();
        for (String abi : mTestMap.keySet()) {
            for (String name : mTestMap.get(abi).keySet()) {
                if (mTestMap.get(abi).get(name).isKnownTestClass(testClassName)) {
                    ids.add(AbiUtils.createId(abi, name));
                }
            }
        }
        List<String> idList = new ArrayList<String>(ids);
        Collections.sort(idList);
        return idList;
    }
}
