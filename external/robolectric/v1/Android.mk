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

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := robolectric
LOCAL_SRC_FILES := $(call all-java-files-under, src/main/java)

LOCAL_STATIC_JAVA_LIBRARIES := \
        robolectric-android \
        robolectric-android-support-v4 \
        robolectric-commons-codec \
        robolectric-commons-logging \
        robolectric-h2 \
        hamcrest-host \
        robolectric-httpclient \
        robolectric-httpcore \
        robolectric-javassist \
        robolectric-json \
        junit \
        robolectric-maps \
        objenesis-host \
        robolectric-sqlite-jdbc \
        robolectric-xpp3

include $(BUILD_HOST_JAVA_LIBRARY)

#############################################################
# Pre-built dependency jars
#############################################################

include $(CLEAR_VARS)

# TODO: Figure out how to not use a prebuilt android API 18 jar and grab from
#       prebuilts

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \
        robolectric-android:lib/main/android.jar \
        robolectric-android-support-v4:lib/main/android-support-v4.jar \
        robolectric-commons-codec:lib/main/commons-codec-1.6.jar \
        robolectric-commons-logging:lib/main/commons-logging-1.1.1.jar \
        robolectric-h2:lib/main/commons-logging-1.1.1.jar \
        robolectric-httpclient:lib/main/httpclient-4.0.3.jar \
        robolectric-httpcore:lib/main/httpcore-4.0.1.jar \
        robolectric-javassist:lib/main/javassist-3.14.0-GA.jar \
        robolectric-json:lib/main/json-20080701.jar \
        robolectric-maps:lib/main/maps_v16.jar \
        robolectric-sqlite-jdbc:lib/main/sqlite-jdbc-3.7.2.jar \
        robolectric-xpp3:lib/main/xpp3-1.1.4c.jar

include $(BUILD_HOST_PREBUILT)
