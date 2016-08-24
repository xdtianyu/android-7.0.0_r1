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

#define LOG_NDEBUG 0
#define LOG_TAG "CAR.INPUT"

#include <string.h>
#include <sys/time.h>
#include <linux/input.h>
#include <jni.h>
#include <JNIHelp.h>
#include <android/keycodes.h>
#include <cutils/log.h>
#include <utils/Errors.h>


namespace android {

static int androidKeyCodeToLinuxKeyCode(int androidKeyCode) {
    switch (androidKeyCode) {
    case AKEYCODE_VOLUME_UP:
        return KEY_VOLUMEUP;
    case AKEYCODE_VOLUME_DOWN:
        return KEY_VOLUMEDOWN;
    case AKEYCODE_CALL:
        return KEY_SEND;
    case AKEYCODE_ENDCALL:
        return KEY_END;
    /* TODO add more keys like these:
    case AKEYCODE_MEDIA_PLAY_PAUSE:
    case AKEYCODE_MEDIA_STOP:
    case AKEYCODE_MEDIA_NEXT:
    case AKEYCODE_MEDIA_PREVIOUS:*/
    case AKEYCODE_VOICE_ASSIST:
        return KEY_MICMUTE;
    default:
        ALOGW("Unmapped android key code %d dropped", androidKeyCode);
        return 0;
    }
}

/*
 * Class:     com_android_car_CarInputService
 * Method:    nativeInjectKeyEvent
 * Signature: (IIZ)I
 */
static jint com_android_car_CarInputService_nativeInjectKeyEvent
  (JNIEnv *env, jobject /*object*/, jint fd, jint keyCode, jboolean down) {
    int linuxKeyCode = androidKeyCodeToLinuxKeyCode(keyCode);
    if (linuxKeyCode == 0) {
        return BAD_VALUE;
    }
    struct input_event ev[2];
    memset(reinterpret_cast<void*>(&ev), 0, sizeof(ev));
    struct timeval now;
    gettimeofday(&now, NULL);
    // kernel driver is not using time now, but set it to be safe.
    ev[0].time = now;
    ev[0].type = EV_KEY;
    ev[0].code = linuxKeyCode;
    ev[0].value = (down ? 1 : 0);
    // force delivery and flushing
    ev[1].time = now;
    ev[1].type = EV_SYN;
    ev[1].code = SYN_REPORT;
    ev[1].value = 0;
    ALOGI("injectKeyEvent down %d keyCode %d, value %d", down, ev[0].code, ev[0].value);
    int r = write(fd, reinterpret_cast<void*>(&ev), sizeof(ev));
    if (r != sizeof(ev)) {
        return -EIO;
    }
    return 0;
}

static JNINativeMethod gMethods[] = {
    { "nativeInjectKeyEvent", "(IIZ)I",
            (void*)com_android_car_CarInputService_nativeInjectKeyEvent },
};

int register_com_android_car_CarInputService(JNIEnv *env) {
    return jniRegisterNativeMethods(env, "com/android/car/CarInputService",
            gMethods, NELEM(gMethods));
}

} // namespace android
