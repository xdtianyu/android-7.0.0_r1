/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.compatibility.common.util;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * Utility class for handling device ABIs
 */
public class AbiUtils {

    /**
     * The set of 32Bit ABIs.
     */
    private static final Set<String> ABIS_32BIT = new HashSet<String>();

    /**
     * The set of 64Bit ABIs.
     */
    private static final Set<String> ABIS_64BIT = new HashSet<String>();

    /**
     * The set of ARM ABIs.
     */
    private static final Set<String> ARM_ABIS = new HashSet<String>();

    /**
     * The set of Intel ABIs.
     */
    private static final Set<String> INTEL_ABIS = new HashSet<String>();

    /**
     * The set of Mips ABIs.
     */
    private static final Set<String> MIPS_ABIS = new HashSet<String>();

    /**
     * The set of ABI names which Compatibility supports.
     */
    private static final Set<String> ABIS_SUPPORTED_BY_COMPATIBILITY = new HashSet<String>();

    /**
     * The map of architecture to ABI.
     */
    private static final Map<String, Set<String>> ARCH_TO_ABIS = new HashMap<String, Set<String>>();
    static {
        ABIS_32BIT.add("armeabi-v7a");
        ABIS_32BIT.add("x86");
        ABIS_32BIT.add("mips");

        ABIS_64BIT.add("arm64-v8a");
        ABIS_64BIT.add("x86_64");
        ABIS_64BIT.add("mips64");

        ARM_ABIS.add("armeabi-v7a");
        ARM_ABIS.add("arm64-v8a");

        INTEL_ABIS.add("x86");
        INTEL_ABIS.add("x86_64");

        MIPS_ABIS.add("mips");
        MIPS_ABIS.add("mips64");

        ARCH_TO_ABIS.put("arm", ARM_ABIS);
        ARCH_TO_ABIS.put("arm64", ARM_ABIS);
        ARCH_TO_ABIS.put("x86", INTEL_ABIS);
        ARCH_TO_ABIS.put("x86_64", INTEL_ABIS);
        ARCH_TO_ABIS.put("mips", MIPS_ABIS);
        ARCH_TO_ABIS.put("mips64", MIPS_ABIS);

        ABIS_SUPPORTED_BY_COMPATIBILITY.addAll(ARM_ABIS);
        ABIS_SUPPORTED_BY_COMPATIBILITY.addAll(INTEL_ABIS);
        ABIS_SUPPORTED_BY_COMPATIBILITY.addAll(MIPS_ABIS);
    }

    /**
     * Private constructor to avoid instantiation.
     */
    private AbiUtils() {}

    /**
     * Returns the set of ABIs associated with the given architecture.
     * @param arch The architecture to look up.
     * @return a new Set containing the ABIs.
     */
    public static Set<String> getAbisForArch(String arch) {
        if (arch == null || arch.isEmpty() || !ARCH_TO_ABIS.containsKey(arch)) {
            return getAbisSupportedByCompatibility();
        }
        return new HashSet<String>(ARCH_TO_ABIS.get(arch));
    }

    /**
     * Returns the set of ABIs supported by Compatibility.
     * @return a new Set containing the supported ABIs.
     */
    public static Set<String> getAbisSupportedByCompatibility() {
        return new HashSet<String>(ABIS_SUPPORTED_BY_COMPATIBILITY);
    }

    /**
     * @param abi The ABI name to test.
     * @return true if the given ABI is supported by Compatibility.
     */
    public static boolean isAbiSupportedByCompatibility(String abi) {
        return ABIS_SUPPORTED_BY_COMPATIBILITY.contains(abi);
    }

    /**
     * Creates a flag for the given ABI.
     * @param abi the ABI to create the flag for.
     * @return a string which can be add to a command sent to ADB.
     */
    public static String createAbiFlag(String abi) {
        if (abi == null || abi.isEmpty() || !isAbiSupportedByCompatibility(abi)) {
            return "";
        }
        return String.format("--abi %s ", abi);
    }

    /**
     * Creates a unique id from the given ABI and name.
     * @param abi The ABI to use.
     * @param name The name to use.
     * @return a string which uniquely identifies a run.
     */
    public static String createId(String abi, String name) {
        return String.format("%s %s", abi, name);
    }

    /**
     * Parses a unique id into the ABI and name.
     * @param id The id to parse.
     * @return a string array containing the ABI and name.
     */
    public static String[] parseId(String id) {
        if (id == null || !id.contains(" ")) {
            return new String[] {"", ""};
        }
        return id.split(" ");
    }

    /**
     * @return the test name portion of the test id.
     *         e.g. armeabi-v7a android.mytest = android.mytest
     */
    public static String parseTestName(String id) {
        return parseId(id)[1];
    }

    /**
     * @return the abi portion of the test id.
     *         e.g. armeabi-v7a android.mytest = armeabi-v7a
     */
    public static String parseAbi(String id) {
        return parseId(id)[0];
    }

    /**
     * @param name The name of the ABI.
     * @return The bitness of the ABI with the given name
     */
    public static String getBitness(String name) {
        return ABIS_32BIT.contains(name) ? "32" : "64";
    }

    /**
     * @param unsupportedAbiDescription A comma separated string containing abis.
     * @return A List of Strings containing valid ABIs.
     */
    public static Set<String> parseAbiList(String unsupportedAbiDescription) {
        Set<String> abiSet = new HashSet<>();
        String[] descSegments = unsupportedAbiDescription.split(":");
        if (descSegments.length == 2) {
            for (String abi : descSegments[1].split(",")) {
                String trimmedAbi = abi.trim();
                if (isAbiSupportedByCompatibility(trimmedAbi)) {
                    abiSet.add(trimmedAbi);
                }
            }
        }
        return abiSet;
    }
}
