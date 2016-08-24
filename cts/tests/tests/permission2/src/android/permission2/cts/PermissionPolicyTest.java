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

package android.permission2.cts;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PermissionGroupInfo;
import android.content.pm.PermissionInfo;
import android.test.AndroidTestCase;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.Log;
import android.util.Xml;
import org.xmlpull.v1.XmlPullParser;

import java.io.InputStream;
import java.lang.String;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Tests for permission policy on the platform.
 */
public class PermissionPolicyTest extends AndroidTestCase {
    private static final String LOG_TAG = "PermissionProtectionTest";

    private static final String PLATFORM_PACKAGE_NAME = "android";

    private static final String PLATFORM_ROOT_NAMESPACE = "android.";

    private static final String TAG_PERMISSION = "permission";

    private static final String ATTR_NAME = "name";
    private static final String ATTR_PERMISSION_GROUP = "permissionGroup";
    private static final String ATTR_PROTECTION_LEVEL = "protectionLevel";

    public void testPlatformPermissionPolicyUnaltered() throws Exception {
        PackageInfo platformPackage = getContext().getPackageManager()
                .getPackageInfo(PLATFORM_PACKAGE_NAME, PackageManager.GET_PERMISSIONS);
        Map<String, PermissionInfo> declaredPermissionsMap = new ArrayMap<>();
        for (PermissionInfo declaredPermission : platformPackage.permissions) {
            declaredPermissionsMap.put(declaredPermission.name, declaredPermission);
        }

        List<PermissionGroupInfo> declaredGroups = getContext().getPackageManager()
                .getAllPermissionGroups(0);
        Set<String> declaredGroupsSet = new ArraySet<>();
        for (PermissionGroupInfo declaredGroup : declaredGroups) {
            declaredGroupsSet.add(declaredGroup.name);
        }

        Set<String> expectedPermissionGroups = new ArraySet<String>();

        for (PermissionInfo expectedPermission : loadExpectedPermissions()) {
            // OEMs cannot remove permissions
            String expectedPermissionName = expectedPermission.name;
            PermissionInfo declaredPermission = declaredPermissionsMap.get(expectedPermissionName);
            assertNotNull("Permission " + expectedPermissionName
                    + " must be declared", declaredPermission);

            // We want to end up with OEM defined permissions and groups to check their namespace
            declaredPermissionsMap.remove(expectedPermissionName);
            // Collect expected groups to check if OEM defined groups aren't in platform namespace
            expectedPermissionGroups.add(expectedPermission.group);

            // OEMs cannot change permission protection
            final int expectedProtection = expectedPermission.protectionLevel
                    & PermissionInfo.PROTECTION_MASK_BASE;
            final int declaredProtection = declaredPermission.protectionLevel
                    & PermissionInfo.PROTECTION_MASK_BASE;
            assertEquals("Permission " + expectedPermissionName + " invalid protection level",
                    expectedProtection, declaredProtection);

            // OEMs cannot change permission protection flags
            final int expectedProtectionFlags = expectedPermission.protectionLevel
                    & PermissionInfo.PROTECTION_MASK_FLAGS;
            final int declaredProtectionFlags = declaredPermission.protectionLevel
                    & PermissionInfo.PROTECTION_MASK_FLAGS;
            assertEquals("Permission " + expectedPermissionName + " invalid enforced protection"
                    + " level flags", expectedProtectionFlags, declaredProtectionFlags);

            // OEMs cannot change permission grouping
            if ((declaredPermission.protectionLevel & PermissionInfo.PROTECTION_DANGEROUS) != 0) {
                assertEquals("Permission " + expectedPermissionName + " not in correct group",
                        expectedPermission.group, declaredPermission.group);
                assertTrue("Permission group " + expectedPermission.group + "must be defined",
                        declaredGroupsSet.contains(declaredPermission.group));
            }
        }

        // OEMs cannot define permissions in the platform namespace
        for (String permission : declaredPermissionsMap.keySet()) {
            assertFalse("Cannot define permission " + permission + " in android namespace",
                    permission.startsWith(PLATFORM_ROOT_NAMESPACE));
        }

        // OEMs cannot define groups in the platform namespace
        for (PermissionGroupInfo declaredGroup : declaredGroups) {
            if (!expectedPermissionGroups.contains(declaredGroup.name)) {
                assertFalse("Cannot define group " + declaredGroup.name + " in android namespace",
                        declaredGroup.name != null
                                && declaredGroup.packageName.equals(PLATFORM_PACKAGE_NAME)
                                && declaredGroup.name.startsWith(PLATFORM_ROOT_NAMESPACE));
            }
        }
    }

    private List<PermissionInfo> loadExpectedPermissions() throws Exception {
        List<PermissionInfo> permissions = new ArrayList<>();
        try (
                InputStream in = getContext().getResources()
                        .openRawResource(android.permission2.cts.R.raw.android_manifest)
        ) {
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(in, null);

            final int outerDepth = parser.getDepth();
            int type;
            while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                    && (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {
                if (type == XmlPullParser.END_TAG || type == XmlPullParser.TEXT) {
                    continue;
                }
                if (TAG_PERMISSION.equals(parser.getName())) {
                    PermissionInfo permissionInfo = new PermissionInfo();
                    permissionInfo.name = parser.getAttributeValue(null, ATTR_NAME);
                    permissionInfo.group = parser.getAttributeValue(null, ATTR_PERMISSION_GROUP);
                    permissionInfo.protectionLevel = parseProtectionLevel(
                            parser.getAttributeValue(null, ATTR_PROTECTION_LEVEL));
                    permissions.add(permissionInfo);
                } else {
                    Log.e(LOG_TAG, "Unknown tag " + parser.getName());
                }
            }
        }
        return permissions;
    }

    private static int parseProtectionLevel(String protectionLevelString) {
        int protectionLevel = 0;
        String[] fragments = protectionLevelString.split("\\|");
        for (String fragment : fragments) {
            switch (fragment.trim()) {
                case "normal": {
                    protectionLevel |= PermissionInfo.PROTECTION_NORMAL;
                } break;
                case "dangerous": {
                    protectionLevel |= PermissionInfo.PROTECTION_DANGEROUS;
                } break;
                case "signature": {
                    protectionLevel |= PermissionInfo.PROTECTION_SIGNATURE;
                } break;
                case "signatureOrSystem": {
                    protectionLevel |= PermissionInfo.PROTECTION_SIGNATURE;
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_SYSTEM;
                } break;
                case "system": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_SYSTEM;
                } break;
                case "installer": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_INSTALLER;
                } break;
                case "verifier": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_VERIFIER;
                } break;
                case "preinstalled": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_PREINSTALLED;
                } break;
                case "pre23": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_PRE23;
                } break;
                case "appop": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_APPOP;
                } break;
                case "development": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_DEVELOPMENT;
                } break;
                case "privileged": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_PRIVILEGED;
                } break;
                case "setup": {
                    protectionLevel |= PermissionInfo.PROTECTION_FLAG_SETUP;
                } break;
            }
        }
        return protectionLevel;
    }
}
