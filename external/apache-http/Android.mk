# Copyright (C) 2014 The Android Open Source Project
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
#
LOCAL_PATH:= $(call my-dir)

apache_http_src_files := \
    $(call all-java-files-under,src) \
    $(call all-java-files-under,android)

apache_http_java_libs := conscrypt

apache_http_packages := $(strip \
  com.android.internal.http.multipart \
  org.apache.commons.logging \
  org.apache.commons.logging.impl \
  org.apache.commons.codec \
  org.apache.commons.codec.net \
  org.apache.commons.codec.language \
  org.apache.commons.codec.binary \
  org.apache.http.params \
  org.apache.http \
  org.apache.http.client.params \
  org.apache.http.client \
  org.apache.http.client.utils \
  org.apache.http.client.protocol \
  org.apache.http.client.methods \
  org.apache.http.client.entity \
  org.apache.http.protocol \
  org.apache.http.impl \
  org.apache.http.impl.client \
  org.apache.http.impl.auth \
  org.apache.http.impl.cookie \
  org.apache.http.impl.entity \
  org.apache.http.impl.io \
  org.apache.http.impl.conn \
  org.apache.http.impl.conn.tsccm \
  org.apache.http.message \
  org.apache.http.auth.params \
  org.apache.http.auth \
  org.apache.http.cookie.params \
  org.apache.http.cookie \
  org.apache.http.util \
  org.apache.http.entity \
  org.apache.http.io \
  org.apache.http.conn.params \
  org.apache.http.conn \
  org.apache.http.conn.routing \
  org.apache.http.conn.scheme \
  org.apache.http.conn.util \
  android.net.compatibility \
  android.net.http \
)

include $(CLEAR_VARS)
LOCAL_MODULE := org.apache.http.legacy.boot
LOCAL_MODULE_TAGS := optional
LOCAL_JAVA_LIBRARIES := $(apache_http_java_libs)
LOCAL_SRC_FILES := $(apache_http_src_files)
LOCAL_SDK_VERSION := 21
LOCAL_MODULE_TAGS := optional
include $(BUILD_JAVA_LIBRARY)

##############################################
# Generate the stub source files
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(apache_http_src_files)
LOCAL_SRC_FILES += \
    ../../frameworks/base/core/java/org/apache/http/conn/ConnectTimeoutException.java \
    ../../frameworks/base/core/java/org/apache/http/conn/scheme/HostNameResolver.java \
    ../../frameworks/base/core/java/org/apache/http/conn/scheme/LayeredSocketFactory.java \
    ../../frameworks/base/core/java/org/apache/http/conn/scheme/SocketFactory.java \
    ../../frameworks/base/core/java/org/apache/http/conn/ssl/AbstractVerifier.java \
    ../../frameworks/base/core/java/org/apache/http/conn/ssl/AllowAllHostnameVerifier.java \
    ../../frameworks/base/core/java/org/apache/http/conn/ssl/AndroidDistinguishedNameParser.java \
    ../../frameworks/base/core/java/org/apache/http/conn/ssl/BrowserCompatHostnameVerifier.java \
    ../../frameworks/base/core/java/org/apache/http/conn/ssl/SSLSocketFactory.java \
    ../../frameworks/base/core/java/org/apache/http/conn/ssl/StrictHostnameVerifier.java \
    ../../frameworks/base/core/java/org/apache/http/conn/ssl/X509HostnameVerifier.java \
    ../../frameworks/base/core/java/org/apache/http/params/CoreConnectionPNames.java \
    ../../frameworks/base/core/java/org/apache/http/params/HttpConnectionParams.java \
    ../../frameworks/base/core/java/org/apache/http/params/HttpParams.java \
    ../../frameworks/base/core/java/android/net/http/HttpResponseCache.java \
    ../../frameworks/base/core/java/android/net/http/SslCertificate.java \
    ../../frameworks/base/core/java/android/net/http/SslError.java \


LOCAL_JAVA_LIBRARIES := $(apache_http_java_libs)
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_DROIDDOC_SOURCE_PATH := $(LOCAL_PATH)/src \
  $(LOCAL_PATH)/android \
  $(LOCAL_PATH)/../../frameworks/base/core/java/org/apache

LOCAL_DROIDDOC_OPTIONS:= \
    -stubpackages $(subst $(space),:,$(apache_http_packages)) \
    -stubs $(TARGET_OUT_COMMON_INTERMEDIATES)/JAVA_LIBRARIES/org.apache.http.legacy_intermediates/src \
    -nodocs

LOCAL_SDK_VERSION := 21
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_MODULE := apache-http-stubs-gen

include $(BUILD_DROIDDOC)
apache_http_stubs_gen_stamp := $(full_target)

# For unbundled build we'll use the prebuilt jar from prebuilts/sdk.
ifeq (,$(TARGET_BUILD_APPS)$(filter true,$(TARGET_BUILD_PDK)))
###############################################
# Build the stub source files into a jar.
include $(CLEAR_VARS)
LOCAL_MODULE := org.apache.http.legacy
LOCAL_SOURCE_FILES_ALL_GENERATED := true
LOCAL_SDK_VERSION := 21
include $(BUILD_STATIC_JAVA_LIBRARY)

# Make sure to run droiddoc first to generate the stub source files.
$(full_classes_compiled_jar) : $(apache_http_stubs_gen_stamp)
$(full_classes_jack) : $(apache_http_stubs_gen_stamp)

# Archive a copy of the classes.jar in SDK build.
$(call dist-for-goals,sdk win_sdk,$(full_classes_jar):org.apache.http.legacy.jar)
endif  # not TARGET_BUILD_APPS

apache_http_src_files :=
apache_http_java_libs :=
apache_http_packages :=
apache_http_stubs_gen_stamp :=
