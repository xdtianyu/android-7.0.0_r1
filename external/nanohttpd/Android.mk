LOCAL_PATH := $(call my-dir)

# This module target includes just the single core file: NanoHTTPD.java, which
# is enough for HTTP 1.1 support and nothing else.
# ============================================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, core/src/main)
LOCAL_MODULE := libnanohttpd
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := current

include $(BUILD_STATIC_JAVA_LIBRARY)

# This module target includes SimpleWebServer that supports additional functionality
# such as serving files from a specified location, resume of downloads, etc.
# ============================================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, core/src/main) \
                   $(call all-java-files-under, webserver/src/main)
LOCAL_MODULE := nanohttpd-webserver
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := current

include $(BUILD_STATIC_JAVA_LIBRARY)