/*
 * Copyright 2016 The Android Open Source Project
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
 *
 */

#define LOG_TAG "VulkanFeaturesTest"

#include <android/log.h>
#include <jni.h>
#include <vkjson.h>

// TODO(jessehall): Remove this once we update the NDK vulkan.h to a newer
// version that defines this.
#ifndef VK_API_VERSION_1_0
#define VK_API_VERSION_1_0 VK_MAKE_VERSION(1, 0, 0)
#endif

#define ALOGI(msg, ...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, (msg), __VA_ARGS__)
#define ALOGE(msg, ...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, (msg), __VA_ARGS__)

namespace {

VkResult getVkJSON(std::string& vkjson) {
    VkResult result;

    const VkApplicationInfo app_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr,
        "VulkanFeaturesTest", 0,    /* app name, version */
        "vkjson", 0,                /* engine name, version */
        VK_API_VERSION_1_0,
    };
    const VkInstanceCreateInfo instance_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr,
        0,              /* flags */
        &app_info,
        0, nullptr,     /* layers */
        0, nullptr,     /* extensions */
    };
    VkInstance instance;
    result = vkCreateInstance(&instance_info, nullptr, &instance);
    if (result != VK_SUCCESS) {
        ALOGE("vkCreateInstance failed: %d", result);
        return result;
    }

    uint32_t ngpu = 0;
    result = vkEnumeratePhysicalDevices(instance, &ngpu, nullptr);
    if (result != VK_SUCCESS) {
        ALOGE("vkEnumeratePhysicalDevices failed: %d", result);
        vkDestroyInstance(instance, nullptr);
        return result;
    }
    std::vector<VkPhysicalDevice> gpus(ngpu, VK_NULL_HANDLE);
    result = vkEnumeratePhysicalDevices(instance, &ngpu, gpus.data());
    if (result != VK_SUCCESS) {
        ALOGE("vkEnumeratePhysicalDevices failed: %d", result);
        vkDestroyInstance(instance, nullptr);
        return result;
    }

    vkjson.assign("[\n");
    for (size_t i = 0, n = gpus.size(); i < n; i++) {
        auto props = VkJsonGetAllProperties(gpus[i]);
        vkjson.append(VkJsonAllPropertiesToJson(props));
        if (i < n - 1)
            vkjson.append(",\n");
    }
    vkjson.append("]");

    vkDestroyInstance(instance, nullptr);

    return VK_SUCCESS;
}

jstring android_graphics_cts_VulkanFeaturesTest_nativeGetVkJSON(JNIEnv* env,
    jclass /*clazz*/)
{
    std::string vkjson;
    if (getVkJSON(vkjson) < 0)
        return nullptr;
    return env->NewStringUTF(vkjson.c_str());
}

static JNINativeMethod gMethods[] = {
    {   "nativeGetVkJSON", "()Ljava/lang/String;",
        (void*) android_graphics_cts_VulkanFeaturesTest_nativeGetVkJSON },
};

} // anonymous namespace

int register_android_graphics_cts_VulkanFeaturesTest(JNIEnv* env) {
    jclass clazz = env->FindClass("android/graphics/cts/VulkanFeaturesTest");
    return env->RegisterNatives(clazz, gMethods,
            sizeof(gMethods) / sizeof(JNINativeMethod));
}
