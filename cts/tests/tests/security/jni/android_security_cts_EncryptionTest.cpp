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

#include <cpu-features.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <jni.h>
#include <JNIHelp.h>
#include <openssl/aes.h>
#include <openssl/cpu.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <string.h>
#include <sys/vfs.h>
#include <time.h>
#include <new>

#define TEST_EVP_CIPHER     EVP_aes_256_cbc()
#define TEST_BUFSIZE        (1 * 1024 * 1024) /* 1 MiB */
#define TEST_ITERATIONS     100 /* MiB */
#define TEST_THRESHOLD      2000 /* ms */

/*
 * Detect if filesystem is already encrypted looking at the file
 * system type. It should be possible to check this first but fall
 * back to checking a property value if this is not possible to
 * verify.
 */
static jboolean checkEncryptedFileSystem() {
    struct statfs buf;
    if ((-1 != statfs("/data", &buf)) &&
        (buf.f_type == 0xf15f /* ecryptfs */)) {
        return true;
    }
    return false;
}

/*
 * Function: deviceIsEncrypted
 * Purpose: Check the device is encrypted
 * Parameters: none
 * Returns: boolean: (true) if encrypted, (false) otherwise
 * Exceptions: none
 */
static jboolean android_security_cts_EncryptionTest_deviceIsEncrypted(JNIEnv *, jobject)
{
    if (checkEncryptedFileSystem()) {
        return true;
    }

    char prop_value[PROP_VALUE_MAX];
    property_get("ro.crypto.state", prop_value, "");

    jboolean rc = !strcmp(prop_value, "encrypted");
    ALOGE("EncryptionTest::deviceIsEncrypted: %d", rc);

    return rc;
}

/*
 * Function: cpuHasAes
 * Purpose: Check if we have an ARM CPU with AES instruction
 * Parameters: none
 * Returns: boolean: (true) if AES is available, (false) otherwise
 * Exceptions: none
 */
static jboolean android_security_cts_EncryptionTest_cpuHasAes(JNIEnv *, jobject)
{
    jboolean rc = false;
    AndroidCpuFamily family = android_getCpuFamily();
    uint64_t features = android_getCpuFeatures();

    if (family == ANDROID_CPU_FAMILY_ARM) {
        rc = (features & ANDROID_CPU_ARM_FEATURE_AES) != 0;
    } else if (family == ANDROID_CPU_FAMILY_ARM64) {
        rc = (features & ANDROID_CPU_ARM64_FEATURE_AES) != 0;
    }

    ALOGE("EncryptionTest::cpuHasAes: %d", rc);
    return rc;
}

/*
 * Function: cpuHasNeon
 * Purpose: Check if we have an ARM CPU with NEON instructions
 * Parameters: none
 * Returns: boolean: (true) if NEON is available, (false) otherwise
 * Exceptions: none
 */
static jboolean android_security_cts_EncryptionTest_cpuHasNeon(JNIEnv *, jobject)
{
    jboolean rc = false;
    AndroidCpuFamily family = android_getCpuFamily();

    if (family == ANDROID_CPU_FAMILY_ARM) {
        rc = (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0;
    } else {
        rc = (family == ANDROID_CPU_FAMILY_ARM64);
    }

    ALOGE("EncryptionTest::cpuHasNeon: %d", rc);
    return rc;
}

/*
 * Function: neonIsEnabled
 * Purpose: Check if libcrypto is compiled with NEON support
 * Parameters: none
 * Returns: boolean: (true) if NEON is available, (false) otherwise
 * Exceptions: none
 */
static jboolean android_security_cts_EncryptionTest_neonIsEnabled(JNIEnv *, jobject)
{
#if defined(OPENSSL_ARM) || defined(OPENSSL_AARCH64)
    jboolean rc = CRYPTO_is_NEON_capable();
#else
    jboolean rc = false;
#endif

    ALOGE("EncryptionTest::neonIsEnabled: %d", rc);
    return rc;
}

static inline uint64_t ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

/*
 * Function: aesIsFast
 * Purpose: Test if AES performance is sufficient to require encryption
 * Parameters: none
 * Returns: boolean: (true) if AES performance is acceptable, (false) otherwise
 * Exceptions: InvalidKeyException if EVP_DecryptInit fails, OutOfMemoryError
 *             if memory allocation fails.
 */
static jboolean android_security_cts_EncryptionTest_aesIsFast(JNIEnv *env, jobject)
{
    EVP_CIPHER_CTX ctx;
    uint8_t *buf;
    uint8_t key[EVP_CIPHER_key_length(TEST_EVP_CIPHER)];
    uint8_t iv[EVP_CIPHER_iv_length(TEST_EVP_CIPHER)];

    memset(key, 0x42, sizeof(key));
    memset(iv,  0x11, sizeof(iv));

    EVP_CIPHER_CTX_init(&ctx);

    if (!EVP_DecryptInit(&ctx, TEST_EVP_CIPHER, key, iv)) {
        jniThrowException(env, "java/security/InvalidKeyException",
            "EVP_DecryptInit failed");
        return false;
    }

    buf = new (std::nothrow) uint8_t[TEST_BUFSIZE +
                EVP_CIPHER_block_size(TEST_EVP_CIPHER)];

    if (!buf) {
        jniThrowException(env, "java/lang/OutOfMemoryError",
            "Failed to allocate test buffer");
        return false;
    }

    memset(buf, 0xF0, TEST_BUFSIZE);

    int len;
    uint64_t t = ns();

    for (int i = 0; i < TEST_ITERATIONS; ++i) {
        EVP_DecryptUpdate(&ctx, buf, &len, buf, TEST_BUFSIZE);
    }

    t = ns() - t;

    delete[] buf;

    unsigned long ms = (unsigned long)(t / 1000000);
    double speed =
        (double)(TEST_ITERATIONS * TEST_BUFSIZE / (1024 * 1024)) * 1000.0 / ms;

    ALOGE("EncryptionTest::aesIsFast: %u iterations in %lu ms (%.01lf MiB/s) "
        "(threshold %u ms)", TEST_ITERATIONS, ms, speed, TEST_THRESHOLD);

    return ms < TEST_THRESHOLD;
}

static JNINativeMethod gMethods[] = {
    { "deviceIsEncrypted", "()Z",
            (void *) android_security_cts_EncryptionTest_deviceIsEncrypted },
    { "cpuHasAes", "()Z",
            (void *) android_security_cts_EncryptionTest_cpuHasAes },
    { "cpuHasNeon", "()Z",
            (void *) android_security_cts_EncryptionTest_cpuHasNeon },
    { "neonIsEnabled", "()Z",
            (void *) android_security_cts_EncryptionTest_neonIsEnabled },
    { "aesIsFast", "()Z",
            (void *) android_security_cts_EncryptionTest_aesIsFast }
};

int register_android_security_cts_EncryptionTest(JNIEnv* env)
{
    jclass clazz = env->FindClass("android/security/cts/EncryptionTest");
    return env->RegisterNatives(clazz, gMethods,
            sizeof(gMethods) / sizeof(JNINativeMethod));
}
