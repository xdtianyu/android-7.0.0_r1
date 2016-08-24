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

#include <string.h>
#include <cutils/log.h>
#include <hardware/hardware.h>
#include <hardware/vehicle_camera.h>
#include <jni.h>
#include <JNIHelp.h>
#include <system/window.h>

namespace android {

/*
 * Class:     com_android_car_CarCameraService
 * Method:    nativeOpen
 * Signature: ()J
 */
static jlong com_android_car_CarCameraService_nativeOpen
  (JNIEnv *env, jobject obj)
{
    vehicle_camera_module_t *module = NULL;

    hw_get_module(VEHICLE_CAMERA_HARDWARE_MODULE_ID, (const hw_module_t**)&module);

    if (module == NULL) {
        ALOGE("JNI Camera:  nativeOpen failed!");
    }
    return (jlong)module;
}

/*
 * Class:     com_android_car_CarCameraService
 * Method:    nativeClose
 * Signature: (J)V
 */
static void com_android_car_CarCameraService_nativeClose
  (JNIEnv *env, jobject, jlong dev)
{
    vehicle_camera_device_t *device = (vehicle_camera_device_t*)dev;

    if (device != NULL) {
        device->common.close((struct hw_device_t*)device);
    }
}

/*
 * Class:     com_android_car_CarCameraService
 * Method:    nativeGetSupportedCameras
 * Signature: (J)[I
 */
static jintArray com_android_car_CarCameraService_nativeGetSupportedCameras
  (JNIEnv *env, jobject, jlong mod)
{
    int i;
    vehicle_camera_module_t *module = (vehicle_camera_module_t*)mod;
    const uint32_t *camera_list;
    uint32_t numCameras;
    jintArray outArray = NULL;

    camera_list = module->get_camera_device_list(&numCameras);

    if ((numCameras > 0) && (camera_list != NULL)) {
        outArray = env->NewIntArray(numCameras);
    }

    if (outArray == NULL) {
        return NULL;
    }
    // Copy camera list to output array
    env->SetIntArrayRegion(outArray, 0, numCameras, (jint*)camera_list);
    return outArray;
}

/*
 * Class:     com_android_car_CarCameraService
 * Method:    nativeGetDevice
 * Signature: (JI)J
 */
static jlong com_android_car_CarCameraService_nativeGetDevice
  (JNIEnv *env, jobject, jlong mod, jint cameraType)
{
    const char *cameraTypeString[] = {
        VEHICLE_CAMERA_RVC_DEVICE,
    };
    vehicle_camera_module_t *module = (vehicle_camera_module_t*)mod;
    hw_device_t *device = NULL;

    if (module != NULL) {
        module->common.methods->open((hw_module_t*)module, cameraTypeString[cameraType], &device);
    }

    if (device == NULL) {
        ALOGE("JNI Camera:  nativeGetDevice failed!");
    }
    return (jlong)device;
}


/*
 * Class:     com_android_car_CarCameraService
 * Method:    nativeGetCapabilities
 * Signature: (J)I
 */
static jint com_android_car_CarCameraService_nativeGetCapabilities
  (JNIEnv *env, jobject, jlong dev)
{
    vehicle_camera_cap_t cap;
    jint capabilities = 0;
    vehicle_camera_device_t *device = (vehicle_camera_device_t*)dev;
    if (device != NULL) {
        // Fetch the capabilities
        int errCode = device->get_capabilities(device, &cap);

        if (errCode == 0) {
            capabilities = cap.capabilites_flags;
        } else {
            ALOGE("JNI Camera:  nativeGetCapabilites errCode = %d", errCode);
        }
    } else {
        ALOGE("JNI Camera:  nativeGetCapabilities!");
    }

    return capabilities;
}

/*
 * Class:     com_android_car_CarCameraService
 * Method:    nativeGetCameraCrop
 * Signature: (J)Landroid/graphics/Rect;
 */
static jobject com_android_car_CarCameraService_nativeGetCameraCrop
  (JNIEnv *env, jobject, jlong dev)
{
    vehicle_camera_device_t *device = (vehicle_camera_device_t*)dev;
    jobject retObj = NULL;

    if (device != NULL) {
        // Get the crop values
        android_native_rect_t rect;
        device->get_camera_crop(device, &rect);

        // Get the class
        jclass cls = env->FindClass("android/graphics/Rect");
        jmethodID midInit = env->GetMethodID(cls, "<init>", "(IIII)V");

        // Instantiate a new java CarRect objected with the current values
        retObj = env->NewObject(cls, midInit, rect.left, rect.top, rect.right, rect.bottom);
    }

    return retObj;
}

/*
 * Class:     com_android_car_CarCameraService
 * Method:    nativeSetCameraCrop
 * Signature: (JLandroid/graphics/Rect;)V
 */
static void com_android_car_CarCameraService_nativeSetCameraCrop
  (JNIEnv *env, jobject, jlong dev, jobject jrect)
{
    vehicle_camera_device_t *device = (vehicle_camera_device_t*)dev;
    if (device != NULL) {
        android_native_rect_t rect;

        jclass   cls       = env->GetObjectClass(jrect);
        jfieldID fidLeft   = env->GetFieldID(cls, "left",   "I");
        jfieldID fidTop    = env->GetFieldID(cls, "top",    "I");
        jfieldID fidRight  = env->GetFieldID(cls, "right",  "I");
        jfieldID fidBottom = env->GetFieldID(cls, "bottom", "I");

        rect.left   = (uint32_t)env->GetIntField(jrect, fidLeft);
        rect.top    = (uint32_t)env->GetIntField(jrect, fidTop);
        rect.right  = (uint32_t)env->GetIntField(jrect, fidRight);
        rect.bottom = (uint32_t)env->GetIntField(jrect, fidBottom);

        device->set_camera_crop(device, &rect);
    }
}

/*
 * Class:     com_android_car_CarCameraService
 * Method:    nativeGetCameraPosition
 * Signature: (J)Landroid/graphics/Rect;
 */
static jobject com_android_car_CarCameraService_nativeGetCameraPosition
  (JNIEnv *env, jobject, jlong dev)
{
    vehicle_camera_device_t *device = (vehicle_camera_device_t*)dev;
    jobject retObj = NULL;

    if (device != NULL) {
        // Get the position values
        android_native_rect_t rect;
        device->get_camera_position(device, &rect);

        // Get the class
        jclass cls = env->FindClass("android/graphics/Rect");
        jmethodID midInit = env->GetMethodID(cls, "<init>", "(IIII)V");

        // Instantiate a new java CarRect objected with the current values
        retObj = env->NewObject(cls, midInit, rect.left, rect.top, rect.right, rect.bottom);
    }

    return retObj;
}


/*
 * Class:     com_android_car_CarCameraService
 * Method:    nativeSetCameraPosition
 * Signature: (JLandroid/graphics/Rect;)V
 */
static void com_android_car_CarCameraService_nativeSetCameraPosition
  (JNIEnv *env, jobject, jlong dev, jobject jrect)
{
    vehicle_camera_device_t *device = (vehicle_camera_device_t*)dev;
    if (device != NULL) {
        android_native_rect_t rect;

        jclass   cls       = env->GetObjectClass(jrect);
        jfieldID fidLeft   = env->GetFieldID(cls, "left",   "I");
        jfieldID fidTop    = env->GetFieldID(cls, "top",    "I");
        jfieldID fidRight  = env->GetFieldID(cls, "right",  "I");
        jfieldID fidBottom = env->GetFieldID(cls, "bottom", "I");

        rect.left   = (uint32_t)env->GetIntField(jrect, fidLeft);
        rect.top    = (uint32_t)env->GetIntField(jrect, fidTop);
        rect.right  = (uint32_t)env->GetIntField(jrect, fidRight);
        rect.bottom = (uint32_t)env->GetIntField(jrect, fidBottom);

        device->set_camera_position(device, &rect);
    }
}

/*
 * Class:     com_android_car_CarCameraService
 * Method:    nativeGetCameraState
 * Signature: (J)Landroid/car/hardware/camera/CarCameraState;
 */
static jobject com_android_car_CarCameraService_nativeGetCameraState
  (JNIEnv *env, jobject, jlong dev)
{
    vehicle_camera_device_t *device = (vehicle_camera_device_t*)dev;
    jobject retObj = NULL;

    if (device != NULL) {
        vehicle_camera_state_t state;
        device->get_camera_state(device, &state);

        // Get the class
        jclass cls = env->FindClass("android/car/hardware/camera/CarCameraState");
        jmethodID midInit = env->GetMethodID(cls, "<init>", "(ZZ)V");

        // Instantiate a new java CarRect objected with the current values
        retObj = env->NewObject(cls, midInit, state.overlay_on, state.camera_on);
    }

    return retObj;
}


/*
 * Class:     com_android_car_CarCameraService
 * Method:    nativeSetCameraState
 * Signature: (JLandroid/car/hardware/camera/CarCameraState;)V
 */
static void com_android_car_CarCameraService_nativeSetCameraState
  (JNIEnv *env, jobject, jlong dev, jobject jstate)
{
    vehicle_camera_device_t *device = (vehicle_camera_device_t*)dev;
    if (device != NULL) {
        vehicle_camera_state_t state;

        jclass cls = env->GetObjectClass(jstate);
        jmethodID midCamera  = env->GetMethodID(cls, "getCameraIsOn",  "()Z");
        jmethodID midOverlay = env->GetMethodID(cls, "getOverlayIsOn", "()Z");

        state.overlay_on = (uint32_t)env->CallBooleanMethod(jstate, midOverlay);
        state.camera_on  = (uint32_t)env->CallBooleanMethod(jstate, midCamera);

        device->set_camera_state(device, &state);
    }
}

static JNINativeMethod gMethods[] = {
    { "nativeOpen", "()J", (void*)com_android_car_CarCameraService_nativeOpen },
    { "nativeClose", "(J)V", (void*)com_android_car_CarCameraService_nativeClose },
    { "nativeGetSupportedCameras", "(J)[I", (void*)com_android_car_CarCameraService_nativeGetSupportedCameras },
    { "nativeGetDevice", "(JI)J", (void*)com_android_car_CarCameraService_nativeGetDevice },
    { "nativeGetCapabilities", "(J)I", (void*)com_android_car_CarCameraService_nativeGetCapabilities },
    { "nativeGetCameraCrop", "(J)Landroid/graphics/Rect;", (void*)com_android_car_CarCameraService_nativeGetCameraCrop },
    { "nativeSetCameraCrop", "(JLandroid/graphics/Rect;)V", (void*)com_android_car_CarCameraService_nativeSetCameraCrop },
    { "nativeGetCameraPosition", "(J)Landroid/graphics/Rect;", (void*)com_android_car_CarCameraService_nativeGetCameraPosition },
    { "nativeSetCameraPosition", "(JLandroid/graphics/Rect;)V", (void*)com_android_car_CarCameraService_nativeSetCameraPosition },
    { "nativeGetCameraState", "(J)Landroid/car/hardware/camera/CarCameraState;", (void*)com_android_car_CarCameraService_nativeGetCameraState },
    { "nativeSetCameraState", "(JLandroid/car/hardware/camera/CarCameraState;)V", (void*)com_android_car_CarCameraService_nativeSetCameraState },
};

int register_com_android_car_CarCameraService(JNIEnv *env) {
    return jniRegisterNativeMethods(env, "com/android/car/CarCameraService",
            gMethods, NELEM(gMethods));
}

} // namespace android


