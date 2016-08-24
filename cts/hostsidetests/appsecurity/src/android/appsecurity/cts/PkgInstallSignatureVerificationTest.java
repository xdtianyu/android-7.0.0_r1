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

package android.appsecurity.cts;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Locale;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IBuildReceiver;

/**
 * Tests for APK signature verification during installation.
 */
public class PkgInstallSignatureVerificationTest extends DeviceTestCase implements IBuildReceiver {

    private static final String TEST_PKG = "android.appsecurity.cts.tinyapp";
    private static final String TEST_APK_RESOURCE_PREFIX = "/pkgsigverify/";

    private static final String[] DSA_KEY_NAMES = {"1024", "2048", "3072"};
    private static final String[] EC_KEY_NAMES = {"p256", "p384", "p521"};
    private static final String[] RSA_KEY_NAMES = {"1024", "2048", "3072", "4096", "8192", "16384"};
    private static final String[] RSA_KEY_NAMES_2048_AND_LARGER =
            {"2048", "3072", "4096", "8192", "16384"};


    /** Device under test. */
    private ITestDevice mDevice;

    private IBuildInfo mCtsBuild;

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mCtsBuild = buildInfo;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDevice = getDevice();
        assertNotNull(mCtsBuild);
        uninstallPackage();
    }

    @Override
    protected void tearDown() throws Exception {
        try {
            uninstallPackage();
        } catch (DeviceNotAvailableException ignored) {
        } finally {
            super.tearDown();
        }
    }

    public void testInstallOriginalSucceeds() throws Exception {
        // APK signed with v1 and v2 schemes. Obtained by building
        // cts/hostsidetests/appsecurity/test-apps/tinyapp.
        assertInstallSucceeds("original.apk");
    }

    public void testInstallV1OneSignerMD5withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-rsa-pkcs1-md5-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA1withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-rsa-pkcs1-sha1-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA224withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-rsa-pkcs1-sha224-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA256withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-rsa-pkcs1-sha256-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA384withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-rsa-pkcs1-sha384-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA512withRSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-rsa-pkcs1-sha512-%s.apk", RSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA1withECDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-ecdsa-sha1-%s.apk", EC_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA224withECDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-ecdsa-sha224-%s.apk", EC_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA256withECDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-ecdsa-sha256-%s.apk", EC_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA384withECDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-ecdsa-sha384-%s.apk", EC_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA512withECDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-ecdsa-sha512-%s.apk", EC_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA1withDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-dsa-sha1-%s.apk", DSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA224withDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-dsa-sha224-%s.apk", DSA_KEY_NAMES);
    }

    public void testInstallV1OneSignerSHA256withDSA() throws Exception {
        // APK signed with v1 scheme only, one signer.
        assertInstallSucceedsForEach("v1-only-with-dsa-sha256-%s.apk", DSA_KEY_NAMES);
    }

//  Android platform doesn't support DSA with SHA-384 and SHA-512.
//    public void testInstallV1OneSignerSHA384withDSA() throws Exception {
//        // APK signed with v1 scheme only, one signer.
//        assertInstallSucceedsForEach("v1-only-with-dsa-sha384-%s.apk", DSA_KEY_NAMES);
//    }
//
//    public void testInstallV1OneSignerSHA512withDSA() throws Exception {
//        // APK signed with v1 scheme only, one signer.
//        assertInstallSucceedsForEach("v1-only-with-dsa-sha512-%s.apk", DSA_KEY_NAMES);
//    }

    public void testInstallV2StrippedFails() throws Exception {
        // APK signed with v1 and v2 schemes, but v2 signature was stripped from the file (by using
        // zipalign).
        // This should fail because the v1 signature indicates that the APK was supposed to be
        // signed with v2 scheme as well, making the platform's anti-stripping protections reject
        // the APK.
        assertInstallFailsWithError("v2-stripped.apk", "Signature stripped");

        // Similar to above, but the X-Android-APK-Signed anti-stripping header in v1 signature
        // lists unknown signature schemes in addition to APK Signature Scheme v2. Unknown schemes
        // should be ignored.
        assertInstallFailsWithError(
                "v2-stripped-with-ignorable-signing-schemes.apk", "Signature stripped");
    }

    public void testInstallV2OneSignerOneSignature() throws Exception {
        // APK signed with v2 scheme only, one signer, one signature.
        assertInstallSucceedsForEach("v2-only-with-dsa-sha256-%s.apk", DSA_KEY_NAMES);
        assertInstallSucceedsForEach("v2-only-with-ecdsa-sha256-%s.apk", EC_KEY_NAMES);
        assertInstallSucceedsForEach("v2-only-with-rsa-pkcs1-sha256-%s.apk", RSA_KEY_NAMES);
        assertInstallSucceedsForEach("v2-only-with-rsa-pss-sha256-%s.apk", RSA_KEY_NAMES);

        // DSA with SHA-512 is not supported by Android platform and thus APK Signature Scheme v2
        // does not support that either
        // assertInstallSucceedsForEach("v2-only-with-dsa-sha512-%s.apk", DSA_KEY_NAMES);
        assertInstallSucceedsForEach("v2-only-with-ecdsa-sha512-%s.apk", EC_KEY_NAMES);
        assertInstallSucceedsForEach("v2-only-with-rsa-pkcs1-sha512-%s.apk", RSA_KEY_NAMES);
        assertInstallSucceedsForEach(
                "v2-only-with-rsa-pss-sha512-%s.apk",
                RSA_KEY_NAMES_2048_AND_LARGER // 1024-bit key is too short for PSS with SHA-512
                );
    }

    public void testInstallV2SignatureDoesNotVerify() throws Exception {
        // APK signed with v2 scheme only, but the signature over signed-data does not verify.
        String error = "signature did not verify";

        // Bitflip in certificate field inside signed-data. Based on
        // v2-only-with-dsa-sha256-1024.apk.
        assertInstallFailsWithError("v2-only-with-dsa-sha256-1024-sig-does-not-verify.apk", error);

        // Signature claims to be RSA PKCS#1 v1.5 with SHA-256, but is actually using SHA-512.
        // Based on v2-only-with-rsa-pkcs1-sha256-2048.apk.
        assertInstallFailsWithError(
                "v2-only-with-rsa-pkcs1-sha256-2048-sig-does-not-verify.apk", error);

        // Signature claims to be RSA PSS with SHA-256 and 32 bytes of salt, but is actually using 0
        // bytes of salt. Based on v2-only-with-rsa-pkcs1-sha256-2048.apk. Obtained by modifying APK
        // signer to use the wrong amount of salt.
        assertInstallFailsWithError(
                "v2-only-with-rsa-pss-sha256-2048-sig-does-not-verify.apk", error);

        // Bitflip in the ECDSA signature. Based on v2-only-with-ecdsa-sha256-p256.apk.
        assertInstallFailsWithError(
                "v2-only-with-ecdsa-sha256-p256-sig-does-not-verify.apk", error);
    }

    public void testInstallV2ContentDigestMismatch() throws Exception {
        // APK signed with v2 scheme only, but the digest of contents does not match the digest
        // stored in signed-data.
        String error = "digest of contents did not verify";

        // Based on v2-only-with-rsa-pkcs1-sha512-4096.apk. Obtained by modifying APK signer to
        // flip the leftmost bit in content digest before signing signed-data.
        assertInstallFailsWithError(
                "v2-only-with-rsa-pkcs1-sha512-4096-digest-mismatch.apk", error);

        // Based on v2-only-with-ecdsa-sha256-p256.apk. Obtained by modifying APK signer to flip the
        // leftmost bit in content digest before signing signed-data.
        assertInstallFailsWithError(
                "v2-only-with-ecdsa-sha256-p256-digest-mismatch.apk", error);
    }

    public void testInstallNoApkSignatureSchemeBlock() throws Exception {
        // APK signed with v2 scheme only, but the rules for verifying APK Signature Scheme v2
        // signatures say that this APK must not be verified using APK Signature Scheme v2.

        // Obtained from v2-only-with-rsa-pkcs1-sha512-4096.apk by flipping a bit in the magic
        // field in the footer of APK Signing Block. This makes the APK Signing Block disappear.
        assertInstallFails("v2-only-wrong-apk-sig-block-magic.apk");

        // Obtained by modifying APK signer to insert "GARBAGE" between ZIP Central Directory and
        // End of Central Directory. The APK is otherwise fine and is signed with APK Signature
        // Scheme v2. Based on v2-only-with-rsa-pkcs1-sha256.apk.
        assertInstallFails("v2-only-garbage-between-cd-and-eocd.apk");

        // Obtained by modifying APK signer to truncate the ZIP Central Directory by one byte. The
        // APK is otherwise fine and is signed with APK Signature Scheme v2. Based on
        // v2-only-with-rsa-pkcs1-sha256.apk
        assertInstallFails("v2-only-truncated-cd.apk");

        // Obtained by modifying the size in APK Signature Block header. Based on
        // v2-only-with-ecdsa-sha512-p521.apk.
        assertInstallFails("v2-only-apk-sig-block-size-mismatch.apk");

        // Obtained by modifying the ID under which APK Signature Scheme v2 Block is stored in
        // APK Signing Block and by modifying the APK signer to not insert anti-stripping
        // protections into JAR Signature. The APK should appear as having no APK Signature Scheme
        // v2 Block and should thus successfully verify using JAR Signature Scheme.
        assertInstallSucceeds("v1-with-apk-sig-block-but-without-apk-sig-scheme-v2-block.apk");
    }

    public void testInstallV2UnknownPairIgnoredInApkSigningBlock() throws Exception {
        // Obtained by modifying APK signer to emit an unknown ID-value pair into APK Signing Block
        // before the ID-value pair containing the APK Signature Scheme v2 Block. The unknown
        // ID-value should be ignored.
        assertInstallSucceeds("v2-only-unknown-pair-in-apk-sig-block.apk");
    }

    public void testInstallV2IgnoresUnknownSignatureAlgorithms() throws Exception {
        // APK is signed with a known signature algorithm and with a couple of unknown ones.
        // Obtained by modifying APK signer to use "unknown" signature algorithms in addition to
        // known ones.
        assertInstallSucceeds("v2-only-with-ignorable-unsupported-sig-algs.apk");
    }

    public void testInstallV2RejectsMismatchBetweenSignaturesAndDigestsBlocks() throws Exception {
        // APK is signed with a single signature algorithm, but the digests block claims that it is
        // signed with two different signature algorithms. Obtained by modifying APK Signer to
        // emit an additional digest record with signature algorithm 0x12345678.
        assertInstallFailsWithError(
                "v2-only-signatures-and-digests-block-mismatch.apk",
                "Signature algorithms don't match between digests and signatures records");
    }

    public void testInstallV2RejectsMismatchBetweenPublicKeyAndCertificate() throws Exception {
        // APK is signed with v2 only. The public key field does not match the public key in the
        // leaf certificate. Obtained by modifying APK signer to write out a modified leaf
        // certificate where the RSA modulus has a bitflip.
        assertInstallFailsWithError(
                "v2-only-cert-and-public-key-mismatch.apk",
                "Public key mismatch between certificate and signature record");
    }

    public void testInstallV2RejectsSignerBlockWithNoCertificates() throws Exception {
        // APK is signed with v2 only. There are no certificates listed in the signer block.
        // Obtained by modifying APK signer to output no certificates.
        assertInstallFailsWithError("v2-only-no-certs-in-sig.apk", "No certificates listed");
    }

    public void testInstallTwoSigners() throws Exception {
        // APK signed by two different signers.
        assertInstallSucceeds("two-signers.apk");
        // Because the install attempt below is an update, it also tests that the signing
        // certificates exposed by v2 signatures above are the same as the one exposed by v1
        // signatures in this APK.
        assertInstallSucceeds("v1-only-two-signers.apk");
        assertInstallSucceeds("v2-only-two-signers.apk");
    }

    public void testInstallV2TwoSignersRejectsWhenOneBroken() throws Exception {
        // Bitflip in the ECDSA signature of second signer. Based on two-signers.apk.
        // This asserts that breakage in any signer leads to rejection of the APK.
        assertInstallFailsWithError(
                "two-signers-second-signer-v2-broken.apk", "signature did not verify");
    }

    public void testInstallV2TwoSignersRejectsWhenOneWithoutSignatures() throws Exception {
        // APK v2-signed by two different signers. However, there are no signatures for the second
        // signer.
        assertInstallFailsWithError(
                "v2-only-two-signers-second-signer-no-sig.apk", "No signatures");
    }

    public void testInstallV2TwoSignersRejectsWhenOneWithoutSupportedSignatures() throws Exception {
        // APK v2-signed by two different signers. However, there are no supported signatures for
        // the second signer.
        assertInstallFailsWithError(
                "v2-only-two-signers-second-signer-no-supported-sig.apk",
                "No supported signatures");
    }

    public void testInstallV2RejectsWhenMissingCode() throws Exception {
        // Obtained by removing classes.dex from original.apk and then signing with v2 only.
        // Although this has nothing to do with v2 signature verification, package manager wants
        // signature verification / certificate collection to reject APKs with missing code
        // (classes.dex) unless requested otherwise.
        assertInstallFailsWithError("v2-only-missing-classes.dex.apk", "code is missing");
    }

    public void testCorrectCertUsedFromPkcs7SignedDataCertsSet() throws Exception {
        // Obtained by prepending the rsa-1024 certificate to the PKCS#7 SignedData certificates set
        // of v1-only-with-rsa-pkcs1-sha1-2048.apk META-INF/CERT.RSA. The certs (in the order of
        // appearance in the file) are thus: rsa-1024, rsa-2048. The package's signing cert is
        // rsa-2048.
        assertInstallSucceeds("v1-only-pkcs7-cert-bag-first-cert-not-used.apk");

        // Check that rsa-1024 was not used as the previously installed package's signing cert.
        assertInstallFailsWithError(
                "v1-only-with-rsa-pkcs1-sha1-1024.apk", "signatures do not match");

        // Check that rsa-2048 was used as the previously installed package's signing cert.
        assertInstallSucceeds("v1-only-with-rsa-pkcs1-sha1-2048.apk");
    }

    private void assertInstallSucceeds(String apkFilenameInResources) throws Exception {
        String installResult = installPackageFromResource(apkFilenameInResources);
        if (installResult != null) {
            fail("Failed to install " + apkFilenameInResources + ": " + installResult);
        }
    }

    private void assertInstallSucceedsForEach(
            String apkFilenamePatternInResources, String[] args) throws Exception {
        for (String arg : args) {
            String apkFilenameInResources =
                    String.format(Locale.US, apkFilenamePatternInResources, arg);
            String installResult = installPackageFromResource(apkFilenameInResources);
            if (installResult != null) {
                fail("Failed to install " + apkFilenameInResources + ": " + installResult);
            }
            try {
                uninstallPackage();
            } catch (Exception e) {
                throw new RuntimeException(
                        "Failed to uninstall after installing " + apkFilenameInResources, e);
            }
        }
    }

    private void assertInstallFailsWithError(
            String apkFilenameInResources, String errorSubstring) throws Exception {
        String installResult = installPackageFromResource(apkFilenameInResources);
        if (installResult == null) {
            fail("Install of " + apkFilenameInResources + " succeeded but was expected to fail"
                    + " with \"" + errorSubstring + "\"");
        }
        assertContains(
                "Install failure message of " + apkFilenameInResources,
                errorSubstring,
                installResult);
    }

    private void assertInstallFails(String apkFilenameInResources) throws Exception {
        String installResult = installPackageFromResource(apkFilenameInResources);
        if (installResult == null) {
            fail("Install of " + apkFilenameInResources + " succeeded but was expected to fail");
        }
    }

    private static void assertContains(String message, String expectedSubstring, String actual) {
        String errorPrefix = ((message != null) && (message.length() > 0)) ? (message + ": ") : "";
        if (actual == null) {
            fail(errorPrefix + "Expected to contain \"" + expectedSubstring + "\", but was null");
        }
        if (!actual.contains(expectedSubstring)) {
            fail(errorPrefix + "Expected to contain \"" + expectedSubstring + "\", but was \""
                    + actual + "\"");
        }
    }

    private String installPackageFromResource(String apkFilenameInResources)
            throws IOException, DeviceNotAvailableException {
        // ITestDevice.installPackage API requires the APK to be install to be a File. We thus
        // copy the requested resource into a temporary file, attempt to install it, and delete the
        // file during cleanup.

        String fullResourceName = TEST_APK_RESOURCE_PREFIX + apkFilenameInResources;
        File apkFile = File.createTempFile("pkginstalltest", ".apk");
        try {
            try (InputStream in = getClass().getResourceAsStream(fullResourceName);
                    OutputStream out = new BufferedOutputStream(new FileOutputStream(apkFile))) {
                if (in == null) {
                    throw new IllegalArgumentException("Resource not found: " + fullResourceName);
                }
                byte[] buf = new byte[65536];
                int chunkSize;
                while ((chunkSize = in.read(buf)) != -1) {
                    out.write(buf, 0, chunkSize);
                }
            }
            return mDevice.installPackage(apkFile, true);
        } finally {
            apkFile.delete();
        }
    }

    private String uninstallPackage() throws DeviceNotAvailableException {
        return mDevice.uninstallPackage(TEST_PKG);
    }
}
