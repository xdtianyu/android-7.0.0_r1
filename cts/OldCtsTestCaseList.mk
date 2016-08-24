# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cts_security_apps_list := \
    CtsAppAccessData \
    CtsAppWithData \
    CtsDocumentProvider \
    CtsDocumentClient \
    CtsEncryptionApp \
    CtsExternalStorageApp \
    CtsInstrumentationAppDiffCert \
    CtsNetSecPolicyUsesCleartextTrafficFalse \
    CtsNetSecPolicyUsesCleartextTrafficTrue \
    CtsNetSecPolicyUsesCleartextTrafficUnspecified \
    CtsNoRestartBase \
    CtsNoRestartFeature \
    CtsUsePermissionApp22 \
    CtsUsePermissionApp23 \
    CtsUsePermissionApp24 \
    CtsPermissionDeclareApp \
    CtsPermissionDeclareAppCompat \
    CtsPrivilegedUpdateTests \
    CtsReadExternalStorageApp \
    CtsSharedUidInstall \
    CtsSharedUidInstallDiffCert \
    CtsShimPrivUpgradePrebuilt \
    CtsShimPrivUpgradeWrongSHAPrebuilt \
    CtsSimpleAppInstall \
    CtsSimpleAppInstallDiffCert \
    CtsSplitApp \
    CtsSplitApp_x86 \
    CtsSplitApp_x86_64 \
    CtsSplitApp_armeabi-v7a \
    CtsSplitApp_armeabi \
    CtsSplitApp_arm64-v8a \
    CtsSplitApp_mips64 \
    CtsSplitApp_mips \
    CtsSplitAppDiffRevision \
    CtsSplitAppDiffVersion \
    CtsSplitAppDiffCert \
    CtsSplitAppFeature \
    CtsTargetInstrumentationApp \
    CtsUsePermissionDiffCert \
    CtsUsesLibraryApp \
    CtsWriteExternalStorageApp \
    CtsMultiUserStorageApp

cts_security_keysets_list := \
    CtsKeySetTestApp \
    CtsKeySetPermDefSigningA \
    CtsKeySetPermDefSigningB\
    CtsKeySetPermUseSigningA \
    CtsKeySetPermUseSigningB \
    CtsKeySetSigningAUpgradeA \
    CtsKeySetSigningBUpgradeA \
    CtsKeySetSigningAUpgradeAAndB \
    CtsKeySetSigningAUpgradeAOrB \
    CtsKeySetSigningAUpgradeB \
    CtsKeySetSigningBUpgradeB \
    CtsKeySetSigningAAndBUpgradeA \
    CtsKeySetSigningAAndCUpgradeB \
    CtsKeySetSigningAUpgradeNone \
    CtsKeySetSharedUserSigningAUpgradeB \
    CtsKeySetSharedUserSigningBUpgradeB \
    CtsKeySetSigningABadUpgradeB \
    CtsKeySetSigningCBadAUpgradeAB \
    CtsKeySetSigningANoDefUpgradeB \
    CtsKeySetSigningAUpgradeEcA \
    CtsKeySetSigningEcAUpgradeA

cts_account_support_packages := \
    CtsUnaffiliatedAccountAuthenticators

cts_support_packages := \
    CtsAbiOverrideTestApp \
    CtsAccountManagementDevicePolicyApp \
    CtsAdminApp \
    CtsAlarmClockService \
    CtsAppRestrictionsManagingApp \
    CtsAppRestrictionsTargetApp \
    CtsAppTestStubs \
    CtsAppUsageTestApp \
    CtsAssistService \
    CtsAssistApp \
    CtsAtraceTestApp \
    CtsBackupApp \
    CtsCertInstallerApp \
    CtsContactDirectoryProvider \
    CtsCustomizationApp \
    CtsCppToolsApp \
    CtsDeviceAdminApp23 \
    CtsDeviceAdminApp24 \
    CtsDeviceAndProfileOwnerApp23 \
    CtsDeviceAndProfileOwnerApp \
    CtsDeviceInfo \
    CtsDeviceOsTestApp \
    CtsDeviceOwnerApp \
    CtsDeviceServicesTestApp \
    CtsDeviceTaskSwitchingAppA \
    CtsDeviceTaskSwitchingAppB \
    CtsDeviceTaskSwitchingControl \
    CtsDragAndDropSourceApp \
    CtsDragAndDropTargetApp \
    CtsExternalServiceService \
    CtsHostsideNetworkTestsApp \
    CtsHostsideNetworkTestsApp2 \
    CtsIntentReceiverApp \
    CtsIntentSenderApp \
    CtsLauncherAppsTests \
    CtsLauncherAppsTestsSupport \
    CtsLeanbackJankApp \
    CtsManagedProfileApp \
    CtsMonkeyApp \
    CtsMonkeyApp2 \
    CtsPackageInstallerApp \
    CtsPermissionApp \
    CtsProfileOwnerApp \
    CtsSimpleApp \
    CtsSimplePreMApp \
    CtsSomeAccessibilityServices \
    CtsSystemUiDeviceApp \
    CtsThemeDeviceApp \
    CtsUsbSerialTestApp \
    CtsVoiceInteractionService \
    CtsVoiceInteractionApp \
    CtsVoiceSettingsService \
    CtsVpnFirewallApp \
    CtsWidgetProviderApp \
    CtsWifiConfigCreator \
    TestDeviceSetup \
    $(cts_account_support_packages) \
    $(cts_security_apps_list) \
    $(cts_security_keysets_list)

cts_external_packages := \
    com.replica.replicaisland \
    com.drawelements.deqp

# Any APKs that need to be copied to the CTS distribution's testcases
# directory but do not require an associated test package XML.
CTS_TEST_CASE_LIST := \
    $(cts_support_packages) \
    $(cts_external_packages)

# Test packages that require an associated test package XML.
cts_test_packages := \
    CtsIcuTestCases \
    CtsAccelerationTestCases \
    CtsAccountManagerTestCases \
    CtsAccessibilityServiceTestCases \
    CtsAccessibilityTestCases \
    CtsAdminTestCases \
    CtsAlarmClockTestCases \
    CtsAnimationTestCases \
    CtsAppTestCases \
    CtsAppWidgetTestCases \
    CtsAssistTestCases \
    CtsBackupTestCases \
    CtsBluetoothTestCases \
    CtsCalendarcommon2TestCases \
    CtsCallLogTestCases \
    CtsCameraTestCases \
    CtsCarTestCases \
    CtsContentTestCases \
    CtsDatabaseTestCases \
    CtsDisplayTestCases \
    CtsDpiTestCases \
    CtsDpiTestCases2 \
    CtsDramTestCases \
    CtsDreamsTestCases \
    CtsDrmTestCases \
    CtsEffectTestCases \
    CtsExternalServiceTestCases \
    CtsFileSystemTestCases \
    CtsGestureTestCases \
    CtsGraphicsTestCases \
    CtsGraphics2TestCases \
    CtsHardwareTestCases \
    CtsJankDeviceTestCases \
    CtsLeanbackJankTestCases \
    CtsJobSchedulerTestCases \
    CtsJniTestCases \
    CtsKeystoreTestCases \
    CtsLibcoreLegacy22TestCases \
    CtsLocationTestCases \
    CtsLocation2TestCases \
    CtsMediaStressTestCases \
    CtsMediaTestCases \
    CtsMidiTestCases \
    CtsMultiUserTestCases \
    CtsNdefTestCases \
    CtsNetSecPolicyUsesCleartextTrafficFalseTestCases \
    CtsNetSecPolicyUsesCleartextTrafficTrueTestCases \
    CtsNetSecPolicyUsesCleartextTrafficUnspecifiedTestCases \
    CtsNetTestCases \
    CtsNetTestCasesLegacyApi22 \
    CtsNetTestCasesLegacyPermission22 \
    CtsNetSecConfigAttributeTestCases \
    CtsNetSecConfigCleartextTrafficTestCases \
    CtsNetSecConfigBasicDebugDisabledTestCases \
    CtsNetSecConfigBasicDebugEnabledTestCases \
    CtsNetSecConfigBasicDomainConfigTestCases \
    CtsNetSecConfigInvalidPinTestCases \
    CtsNetSecConfigNestedDomainConfigTestCases \
    CtsNetSecConfigResourcesSrcTestCases \
    CtsOpenGLTestCases \
    CtsOpenGlPerfTestCases \
    CtsOpenGlPerf2TestCases \
    CtsOsTestCases \
    CtsPermissionTestCases \
    CtsPermission2TestCases \
    CtsPreferenceTestCases \
    CtsPreference2TestCases \
    CtsPrintTestCases \
    CtsProviderTestCases \
    CtsRenderscriptTestCases \
    CtsRenderscriptLegacyTestCases \
    CtsRsBlasTestCases \
    CtsRsCppTestCases \
    CtsSaxTestCases \
    CtsSecurityTestCases \
    CtsSignatureTestCases \
    CtsSimpleCpuTestCases \
    CtsSpeechTestCases \
    CtsSystemUiTestCases \
    CtsTelecomTestCases \
    CtsTelecomTestCases2 \
    CtsTelephonyTestCases \
    CtsTextTestCases \
    CtsTextureViewTestCases \
    CtsThemeDeviceTestCases \
    CtsTransitionTestCases \
    CtsTvProviderTestCases \
    CtsTvTestCases \
    CtsUiAutomationTestCases \
    CtsUiRenderingTestCases \
    CtsUiDeviceTestCases \
    CtsUsageStatsTestCases \
    CtsUtilTestCases \
    CtsVideoTestCases \
    CtsViewTestCases \
    CtsVoiceInteractionTestCases \
    CtsVoiceSettingsTestCases \
    CtsWebkitTestCases \
    CtsWidgetTestCases

# All APKs that need to be scanned by the coverage utilities.
CTS_COVERAGE_TEST_CASE_LIST := \
    $(cts_support_packages) \
    $(cts_test_packages)

# Host side only tests
cts_host_libraries := \
    CtsAadbHostTestCases \
    CtsAbiOverrideHostTestCases \
    CtsAppSecurityHostTestCases \
    CtsAppUsageHostTestCases \
    CtsAtraceHostTestCases \
    CtsCppToolsTestCases \
    CtsDevicePolicyManagerTestCases \
    CtsDragAndDropHostTestCases \
    CtsDumpsysHostTestCases \
    CtsHostsideNetworkTests \
    CtsJdwpSecurityHostTestCases \
    CtsMonkeyTestCases \
    CtsOsHostTestCases \
    CtsSecurityHostTestCases \
    CtsServicesHostTestCases \
    CtsThemeHostTestCases \
    CtsUiHostTestCases \
    CtsUsbTests \
    CtsSystemUiHostTestCases

# List of native tests. For 32 bit targets, assumes that there will be
# one test executable, and it will end in 32. For 64 bit targets, assumes
# that there will be two executables, one that ends in 32 for the 32
# bit executable and one that ends in 64 for the 64 bit executable.
cts_native_tests := \
    CtsNativeMediaSlTestCases \
    CtsNativeMediaXaTestCases \

ifeq ($(HOST_OS)-$(HOST_ARCH),$(filter $(HOST_OS)-$(HOST_ARCH),linux-x86 linux-x86_64))
cts_native_tests += CtsBionicTestCases
cts_native_tests += CtsSimpleperfTestCases
endif

cts_device_jars := \
    CtsJdwpApp

cts_target_junit_tests := \
    CtsJdwp \
    CtsLibcoreOj

cts_deqp_test_apis := \
    egl \
    gles2 \
    gles3 \
    gles31

# All the files that will end up under the repository/testcases
# directory of the final CTS distribution.
CTS_TEST_CASES := $(call cts-get-lib-paths,$(cts_host_libraries)) \
    $(call cts-get-package-paths,$(cts_test_packages)) \
    $(call cts-get-ui-lib-paths,$(cts_device_jars)) \
    $(call cts-get-ui-lib-paths,$(cts_target_junit_tests)) \
    $(call cts-get-executable-paths,$(cts_device_executables)) \
    $(call cts-get-native-paths,$(cts_native_tests),32)

ifeq ($(TARGET_IS_64_BIT),true)
CTS_TEST_CASES += $(call cts-get-native-paths,$(cts_native_tests),64)
endif

# All the XMLs that will end up under the repository/testcases
# and that need to be created before making the final CTS distribution.
CTS_TEST_XMLS := $(call cts-get-test-xmls,$(cts_host_libraries)) \
    $(call cts-get-test-xmls,$(cts_test_packages)) \
    $(call cts-get-test-xmls,$(cts_native_tests)) \
    $(call cts-get-test-xmls,$(cts_target_junit_tests)) \
    $(call cts-get-deqp-test-xmls,$(cts_deqp_test_apis))

# The following files will be placed in the tools directory of the CTS distribution
CTS_TOOLS_LIST :=
